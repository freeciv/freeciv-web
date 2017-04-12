define([
        'StackTrace',
        '../shared/Base',
        '../shared/Extensions',
        '../shared/EventSource',
        '../shared/Hacks',
        '../shared/Info',
        '../shared/Settings',
        '../shared/Utilities',
        './Frame',
        './Notifier',
        './ResourceCache',
        './Statistics',
    ], function (
        printStackTrace,
        base,
        extensions,
        EventSource,
        hacks,
        info,
        settings,
        util,
        Frame,
        Notifier,
        ResourceCache,
        Statistics
    ) {
    const api = {};

    var dynamicContextProperties = {
        drawingBufferWidth: true,
        drawingBufferHeight: true,
    };

    function errorBreak() {
        throw "WebGL error!";
    };

    function startCapturing(context) {
        context.ignoreErrors();
        context.captureFrame = true;
        //context.notifier.postMessage("capturing frame " + context.frameNumber + "...");
    };

    function stopCapturing(context) {
        context.notifier.postMessage("captured frame " + (context.frameNumber - 1));
        context.captureFrame = false;
        context.ignoreErrors();

        var frame = context.currentFrame;

        context.markFrame(null); // mark end

        // Fire off callback (if present)
        if (context.captureCallback) {
            context.captureCallback(context, frame);
        }
    };

    function frameEnded(context) {
        if (context.inFrame) {
            context.inFrame = false;
            context.statistics.endFrame();
            context.frameCompleted.fire();
            context.ignoreErrors();
        }
    };

    function frameSeparator(context) {
        context.frameNumber++;

        // Start or stop capturing
        if (context.captureFrame) {
            if (context.captureFrameEnd == context.frameNumber) {
                stopCapturing(context);
            }
        } else {
            if (context.captureFrameStart == context.frameNumber) {
                startCapturing(context);
            }
        }

        if (context.captureFrame) {
            context.markFrame(context.frameNumber);
        }

        context.statistics.beginFrame();

        // Even though we are watching most timing methods, we can't be too safe
        original_setTimeout(function () {
            api.frameTerminator.fire();
        }, 0);
    };

    function wrapFunction(context, functionName) {
        var originalFunction = context.rawgl[functionName];
        var statistics = context.statistics;
        var callsPerFrame = statistics.callsPerFrame;
        return function () {
            var gl = context.rawgl;

            var stack = null;
            function generateStack() {
                // Generate stack trace
                var stackResult = printStackTrace();
                // ignore garbage
                stackResult = stackResult.slice(4);
                // Fix up our type
                stackResult[0] = stackResult[0].replace("[object Object].", "gl.");
                return stackResult;
            };

            if (context.inFrame == false) {
                // First call of a new frame!
                context.inFrame = true;
                frameSeparator(context);
            }

            // PRE:
            var call = null;
            if (context.captureFrame) {
                // NOTE: for timing purposes this should be the last thing before the actual call is made
                stack = stack || (context.options.resourceStacks ? generateStack() : null);
                call = context.currentFrame.allocateCall(functionName, arguments);
            }

            callsPerFrame.value++;

            if (context.captureFrame) {
                // Ignore all errors before this call is made
                gl.ignoreErrors();
            }

            // Call real function
            var result = originalFunction.apply(context.rawgl, arguments);

            // Get error state after real call - if we don't do this here, tracing/capture calls could mess things up
            var error = context.NO_ERROR;
            if (!context.options.ignoreErrors || context.captureFrame) {
                error = gl.getError();
            }

            // POST:
            if (context.captureFrame) {
                if (error != context.NO_ERROR) {
                    stack = stack || generateStack();
                }
                call.complete(result, error, stack);
            }

            if (error != context.NO_ERROR) {
                context.errorMap[error] = true;

                if (context.options.breakOnError) {
                    // TODO: backtrace?
                    errorBreak();
                }
            }

            // If this is the frame separator then handle it
            if (context.options.frameSeparators.indexOf(functionName) >= 0) {
                frameEnded(context);
            }

            return result;
        };
    };

    function wrapProperty(context, propertyName) {
        Object.defineProperty(context, propertyName, {
            configurable: false,
            enumerable: true,
            get: function() {
                return context.rawgl[propertyName];
            }
        });
    };

    var CaptureContext = function (canvas, rawgl, options) {
        var defaultOptions = {
            ignoreErrors: true,
            breakOnError: false,
            resourceStacks: false,
            callStacks: false,
            frameSeparators: settings.global.captureOn
        };
        options = options || defaultOptions;
        for (var n in defaultOptions) {
            if (options[n] === undefined) {
                options[n] = defaultOptions[n];
            }
        }

        this.options = options;
        this.canvas = canvas;
        this.rawgl = rawgl;
        this.isWrapped = true;

        this.notifier = new Notifier(api);

        this.rawgl.canvas = canvas;
        info.initialize(this.rawgl);

        this.attributes = rawgl.getContextAttributes ? rawgl.getContextAttributes() : {};

        this.statistics = new Statistics();

        this.frameNumber = 0;
        this.inFrame = false;

        // Function to call when capture completes
        this.captureCallback = null;
        // Frame range to capture (inclusive) - if inside a capture window, captureFrame == true
        this.captureFrameStart = null;
        this.captureFrameEnd = null;
        this.captureFrame = false;
        this.currentFrame = null;

        this.errorMap = {};

        this.enabledExtensions = [];

        this.frameCompleted = new EventSource("frameCompleted");
        this.frameCompleted.addListener(this, function() {
            frameSeparator(this);
        });

        // NOTE: this should happen ASAP so that we make sure to wrap the faked function, not the real-REAL one
        hacks.installAll(rawgl);

        // NOTE: this should also happen really early, but after hacks
        extensions.installExtensions(rawgl);

        // Listen for inferred frame termination and extension termination
        function frameEndedWrapper() {
            frameEnded(this);
        };
        api.frameTerminator.addListener(this, frameEndedWrapper);
        var ext = rawgl.getExtension("GLI_frame_terminator");
        ext.frameEvent.addListener(this, frameEndedWrapper);

        // Clone all properties in context and wrap all functions
        for (var propertyName in rawgl) {
            if (typeof rawgl[propertyName] == 'function') {
                // Functions
                this[propertyName] = wrapFunction(this, propertyName, rawgl[propertyName]);
            } else if (propertyName in dynamicContextProperties) {
                // Enums/constants/etc
                wrapProperty(this, propertyName);
            } else {
                this[propertyName] = rawgl[propertyName];
            }
        }

        // Rewrite getError so that it uses our version instead
        this.getError = function () {
            for (var error in this.errorMap) {
                if (this.errorMap[error]) {
                    this.errorMap[error] = false;
                    return error;
                }
            }
            return this.NO_ERROR;
        };

        // Unlogged pass-through of getContextAttributes and isContextLost
        this.isContextLost = function() {
            return rawgl.isContextLost();
        };
        this.getContextAttributes = function() {
            return rawgl.getContextAttributes();
        };

        // Capture all extension requests
        // We only support a few right now, so filter
        // New extensions that add tokens will needs to have support added in
        // the proper places, such as Info.js for enum values and the resource
        // system for new resources
        var validExts = [
            'GLI_frame_terminator',
            'OES_texture_float',
            'OES_texture_half_float',
            'OES_texture_float_linear',
            'OES_texture_half_float_linear',
            'OES_standard_derivatives',
            'OES_element_index_uint',
            'EXT_texture_filter_anisotropic',
            'EXT_shader_texture_lod',
            'OES_depth_texture',
            'WEBGL_depth_texture',
            'WEBGL_compressed_texture_s3tc',
        ];
        for (var n = 0, l = validExts.length; n < l; n++) {
            validExts.push('MOZ_' + validExts[n]);
            validExts.push('WEBKIT_' + validExts[n]);
        }
        function containsInsensitive(list, name) {
            name = name.toLowerCase();
            for (var n = 0, len = list.length; n < len; ++n) {
                if (list[n].toLowerCase() === name) return true;
            }
            return false;
        };
        var original_getSupportedExtensions = this.getSupportedExtensions;
        this.getSupportedExtensions = function() {
            var exts = original_getSupportedExtensions.call(this);
            var usableExts = [];
            for (var n = 0; n < exts.length; n++) {
                if (containsInsensitive(validExts, exts[n])) {
                    usableExts.push(exts[n]);
                }
            }
            return usableExts;
        };
        var original_getExtension = this.getExtension;
        this.getExtension = function (name) {
            if (!containsInsensitive(validExts, name)) {
                return null;
            }
            var result = original_getExtension.apply(this, arguments);
            if (result) {
                // Nasty, but I never wrote this to support new constants properly
                switch (name.toLowerCase()) {
                    case 'oes_texture_half_float':
                        this['HALF_FLOAT_OES'] = 0x8D61;
                        break;
                    case 'oes_standard_derivatives':
                        this['FRAGMENT_SHADER_DERIVATIVE_HINT_OES'] = 0x8B8B;
                        break;
                    case 'ext_texture_filter_anisotropic':
                    case 'moz_ext_texture_filter_anisotropic':
                    case 'webkit_ext_texture_filter_anisotropic':
                        this['TEXTURE_MAX_ANISOTROPY_EXT'] = 0x84FE;
                        this['MAX_TEXTURE_MAX_ANISOTROPY_EXT'] = 0x84FF;
                        break;
                    case 'webgl_compressed_texture_s3tc':
                    case 'moz_webgl_compressed_texture_s3tc':
                    case 'webkit_webgl_compressed_texture_s3tc':
                        this['COMPRESSED_RGB_S3TC_DXT1_EXT'] = 0x83F0;
                        this['COMPRESSED_RGBA_S3TC_DXT1_EXT'] = 0x83F1;
                        this['COMPRESSED_RGBA_S3TC_DXT3_EXT'] = 0x83F2;
                        this['COMPRESSED_RGBA_S3TC_DXT5_EXT'] = 0x83F3;
                        break;
                }

                this.enabledExtensions.push(name);
            }
            return result;
        };

        // Add a few helper methods
        this.ignoreErrors = rawgl.ignoreErrors = function () {
            while (this.getError() != this.NO_ERROR);
        };

        // Add debug methods
        this.mark = function () {
            if (context.captureFrame) {
                context.currentFrame.mark(arguments);
            }
        };

        // TODO: before or after we wrap? if we do it here (after), then timings won't be affected by our captures
        this.resources = new ResourceCache(this);
    };

    CaptureContext.prototype.markFrame = function (frameNumber) {
        if (this.currentFrame) {
            // Close the previous frame
            this.currentFrame.end(this.rawgl);
            this.currentFrame = null;
        }

        if (frameNumber == null) {
            // Abort if not a real frame
            return;
        }

        this.currentFrame = new Frame(this.canvas, this.rawgl, frameNumber, this.resources);
    };

    CaptureContext.prototype.requestCapture = function (callback) {
        this.captureCallback = callback;
        this.captureFrameStart = this.frameNumber + 1;
        this.captureFrameEnd = this.captureFrameStart + 1;
        this.captureFrame = false;
    };

    api.frameTerminator = new EventSource("frameTerminator");

    // This replaces setTimeout/setInterval with versions that, after the user code is called, try to end the frame
    // This should be a reliable way to bracket frame captures, unless the user is doing something crazy (like
    // rendering in mouse event handlers)
    var timerHijacking = {
        value: 0, // 0 = normal, N = ms between frames, Infinity = stopped
        activeIntervals: [],
        activeTimeouts: []
    };

    function hijackedDelay(delay) {
        var maxDelay = Math.max(isNaN(delay) ? 0 : delay, timerHijacking.value);
        if (!isFinite(maxDelay)) {
            maxDelay = 999999999;
        }
        return maxDelay;
    }

    CaptureContext.setFrameControl = function (value) {
        timerHijacking.value = value;

        // Reset all intervals
        var intervals = timerHijacking.activeIntervals;
        for (var n = 0; n < intervals.length; n++) {
            var interval = intervals[n];
            original_clearInterval(interval.currentId);
            var maxDelay = hijackedDelay(interval.delay);
            interval.currentId = original_setInterval(interval.wrappedCode, maxDelay);
        }

        // Reset all timeouts
        var timeouts = timerHijacking.activeTimeouts;
        for (var n = 0; n < timeouts.length; n++) {
            var timeout = timeouts[n];
            original_clearTimeout(timeout.originalId);
            var maxDelay = hijackedDelay(timeout.delay);
            timeout.currentId = original_setTimeout(timeout.wrappedCode, maxDelay);
        }
    };

    function wrapCode(code, args) {
        args = args ? Array.prototype.slice.call(args, 2) : [];
        return function () {
            if (code) {
                if (base.typename(code) == "String") {
                    original_setInterval(code, 0);
                    original_setInterval(api.frameTerminator.fire
                                             .bind(api.frameTerminator), 0);
                } else {
                    try {
                        code.apply(window, args);
                    } finally {
                        api.frameTerminator.fire();
                    }
                }
            }
        };
    };

    var original_setInterval = window.setInterval;
    window.setInterval = function (code, delay) {
        var maxDelay = hijackedDelay(delay);
        var wrappedCode = wrapCode(code, arguments);
        var intervalId = original_setInterval.apply(window, [wrappedCode, maxDelay]);
        timerHijacking.activeIntervals.push({
            originalId: intervalId,
            currentId: intervalId,
            code: code,
            wrappedCode: wrappedCode,
            delay: delay
        });
        return intervalId;
    };
    var original_clearInterval = window.clearInterval;
    window.clearInterval = function (intervalId) {
        for (var n = 0; n < timerHijacking.activeIntervals.length; n++) {
            if (timerHijacking.activeIntervals[n].originalId == intervalId) {
                var interval = timerHijacking.activeIntervals[n];
                timerHijacking.activeIntervals.splice(n, 1);
                return original_clearInterval.apply(window, [interval.currentId]);
            }
        }
        return original_clearInterval.apply(window, arguments);
    };
    var original_setTimeout = window.setTimeout;
    window.setTimeout = function (code, delay) {
        var maxDelay = hijackedDelay(delay);
        var wrappedCode = wrapCode(code, arguments);
        var cleanupCode = function () {
            // Need to remove from the active timeout list
            window.clearTimeout(timeoutId); // why is this here?
            wrappedCode();
        };
        var timeoutId = original_setTimeout.apply(window, [cleanupCode, maxDelay]);
        timerHijacking.activeTimeouts.push({
            originalId: timeoutId,
            currentId: timeoutId,
            code: code,
            wrappedCode: wrappedCode,
            delay: delay
        });
        return timeoutId;
    };
    var original_clearTimeout = window.clearTimeout;
    window.clearTimeout = function (timeoutId) {
        for (var n = 0; n < timerHijacking.activeTimeouts.length; n++) {
            if (timerHijacking.activeTimeouts[n].originalId == timeoutId) {
                var timeout = timerHijacking.activeTimeouts[n];
                timerHijacking.activeTimeouts.splice(n, 1);
                return original_clearTimeout.apply(window, [timeout.currentId]);
            }
        }
        return original_clearTimeout.apply(window, arguments);
    };

    // Some apps, like q3bsp, use the postMessage hack - because of that, we listen in and try to use it too
    // Note that there is a race condition here such that we may fire in BEFORE the app message, but oh well
    window.addEventListener("message", function () {
        api.frameTerminator.fire();
    }, false);

    // Support for requestAnimationFrame-like APIs
    var requestAnimationFrameNames = [
        "requestAnimationFrame",
        "webkitRequestAnimationFrame",
        "mozRequestAnimationFrame",
        "operaRequestAnimationFrame",
        "msAnimationFrame"
    ];
    for (var n = 0, len = requestAnimationFrameNames.length; n < len; ++n) {
        var name = requestAnimationFrameNames[n];
        if (window[name]) {
            (function(name) {
                var originalFn = window[name];
                var lastFrameTime = (new Date());
                window[name] = function(callback, element) {
                    var time = (new Date());
                    var delta = (time - lastFrameTime);
                    if (delta > timerHijacking.value) {
                        lastFrameTime = time;
                        var wrappedCallback = function() {
                            try {
                                callback.apply(window, arguments);
                            } finally {
                                api.frameTerminator.fire();
                            }
                        };
                        return originalFn.call(window, wrappedCallback, element);
                    } else {
                        window.setTimeout(function() {
                            callback(Date.now());
                        }, delta);
                        return null;
                    }
                };
            })(name);
        }
    }

    // Everything in the inspector should use these instead of the global values
    api.setInterval = function () {
        return original_setInterval.apply(window, arguments);
    };
    api.clearInterval = function () {
        return original_clearInterval.apply(window, arguments);
    };
    api.setTimeout = function () {
        return original_setTimeout.apply(window, arguments);
    };
    api.clearTimeout = function () {
        return original_clearTimeout.apply(window, arguments);
    };

    // options: {
    //     ignoreErrors: bool - ignore errors on calls (can drastically speed things up)
    //     breakOnError: bool - break on gl error
    //     resourceStacks: bool - collect resource creation/deletion callstacks
    //     callStacks: bool - collect callstacks for each call
    //     frameSeparators: ['finish'] / etc
    // }
    api.inspectContext = function (canvas, rawgl, options) {
        // Ignore if we have already wrapped the context
        if (rawgl.isWrapped) {
            // NOTE: if options differ we may want to unwrap and re-wrap
            return rawgl;
        }

        util.setWebGLVersion(rawgl);
        var wrapped = new CaptureContext(canvas, rawgl, options);

        return wrapped;
    };

    api.setFrameControl = CaptureContext.setFrameControl;

    EventSource.setSetTimeoutFn(api.setTimeout);

    return api;

});
