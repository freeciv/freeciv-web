define([
        '../../../shared/Info',
        '../../../shared/Settings',
        '../../../replay/RedundancyChecker',
        '../../pixelhistory/PixelHistory',
        '../../shared/PopupWindow',
        '../../shared/SurfaceInspector',
        '../../shared/TexturePreview',
        '../../shared/TraceLine',
        './TraceListing',
    ], function (
        info,
        settings,
        RedundancyChecker,
        PixelHistory,
        PopupWindow,
        SurfaceInspector,
        TexturePreviewGenerator,
        traceLine,
        TraceListing
    ) {

    var TraceMinibar = function (view, w, elementRoot) {
        var self = this;
        this.view = view;
        this.window = w;
        this.elements = {
            bar: elementRoot.getElementsByClassName("trace-minibar")[0]
        };
        this.buttons = {};
        this.toggles = {};

        this.controller = w.controller;

        this.controller.stepCompleted.addListener(this, function (callIndex) {
            if (callIndex == 0) {
                self.lastCallIndex = null;
            } else {
                self.lastCallIndex = callIndex - 1;
            }
        });

        var buttonHandlers = {};

        function addButton(bar, name, tip, callback) {
            var el = w.document.createElement("div");
            el.className = "trace-minibar-button trace-minibar-button-disabled trace-minibar-command-" + name;

            el.title = tip;
            el.textContent = " ";

            el.onclick = function () {
                if (el.className.indexOf("disabled") != -1) {
                    return;
                }
                callback.apply(self);
            };
            buttonHandlers[name] = callback;

            bar.appendChild(el);

            self.buttons[name] = el;
        };

        addButton(this.elements.bar, "run", "Playback entire frame (F9)", function () {
            this.controller.stepUntilEnd();
            this.refreshState();
        });
        addButton(this.elements.bar, "step-forward", "Step forward one call (F8)", function () {
            if (this.controller.stepForward() == false) {
                this.controller.reset();
                this.controller.openFrame(this.view.frame);
                this.controller.stepForward();
            }
            this.refreshState();
        });
        addButton(this.elements.bar, "step-back", "Step backward one call (F6)", function () {
            this.controller.stepBackward();
            this.refreshState();
        });
        addButton(this.elements.bar, "step-until-draw", "Skip to the next draw call (F7)", function () {
            this.controller.stepUntilDraw();
            this.refreshState();
        });
        /*
        addButton(this.elements.bar, "step-until-error", "Run until an error occurs", function () {
        alert("step-until-error");
        this.controller.stepUntilError();
        this.refreshState();
        });
        */
        addButton(this.elements.bar, "restart", "Restart from the beginning of the frame (F10)", function () {
            this.controller.openFrame(this.view.frame);
            this.controller.stepForward();
            this.refreshState();
        });

        // TODO: move to shared code
        function addToggle(bar, defaultValue, name, tip, callback) {
            var input = w.document.createElement("input");
            input.style.width = "inherit";
            input.style.height = "inherit";

            input.type = "checkbox";
            input.title = tip;
            input.checked = defaultValue;

            input.onchange = function () {
                callback.apply(self, [input.checked]);
            };

            var span = w.document.createElement("span");
            span.textContent = " " + name;

            span.onclick = function () {
                input.checked = !input.checked;
                callback.apply(self, [input.checked]);
            };

            var el = w.document.createElement("div");
            el.className = "trace-minibar-toggle";
            el.appendChild(input);
            el.appendChild(span);

            bar.appendChild(el);

            callback.apply(self, [defaultValue]);

            self.toggles[name] = input;
        };

        var traceCallRedundantBackgroundColor = "#FFFFD1";
        var redundantStylesheet = w.document.createElement("style");
        redundantStylesheet.type = "text/css";
        redundantStylesheet.appendChild(w.document.createTextNode(".trace-call-redundant { background-color: " + traceCallRedundantBackgroundColor + "; }"));
        w.document.getElementsByTagName("head")[0].appendChild(redundantStylesheet);
        var stylesheet = null;
        for (var n = 0; n < w.document.styleSheets.length; n++) {
            var ss = w.document.styleSheets[n];
            if (ss.ownerNode == redundantStylesheet) {
                stylesheet = ss;
                break;
            }
        }
        var redundantRule = null;
        // Grabbed on demand in case it hasn't loaded yet

        var defaultShowRedundant = settings.session.showRedundantCalls;
        addToggle(this.elements.bar, defaultShowRedundant, "Redundant Calls", "Display redundant calls in yellow", function (checked) {
            if (!stylesheet) {
                return;
            }
            if (!redundantRule) {
                for (var n = 0; n < stylesheet.cssRules.length; n++) {
                    var rule = stylesheet.cssRules[n];
                    if (rule.selectorText == ".trace-call-redundant") {
                        redundantRule = rule;
                        break;
                    }
                }
            }

            if (checked) {
                redundantRule.style.backgroundColor = traceCallRedundantBackgroundColor;
            } else {
                redundantRule.style.backgroundColor = "transparent";
            }

            settings.session.showRedundantCalls = checked;
            settings.save();
        });

        w.document.addEventListener("keydown", function (event) {
            var handled = false;
            switch (event.keyCode) {
                case 117: // F6
                    buttonHandlers["step-back"].apply(self);
                    handled = true;
                    break;
                case 118: // F7
                    buttonHandlers["step-until-draw"].apply(self);
                    handled = true;
                    break;
                case 119: // F8
                    buttonHandlers["step-forward"].apply(self);
                    handled = true;
                    break;
                case 120: // F9
                    buttonHandlers["run"].apply(self);
                    handled = true;
                    break;
                case 121: // F10
                    buttonHandlers["restart"].apply(self);
                    handled = true;
                    break;
            };

            if (handled) {
                event.preventDefault();
                event.stopPropagation();
            }
        }, false);

        //this.update();
    };
    TraceMinibar.prototype.refreshState = function (ignoreScroll) {
        //var newState = new StateCapture(this.replaygl);
        if (this.lastCallIndex != null) {
            this.view.traceListing.setActiveCall(this.lastCallIndex, ignoreScroll);
        }
        //this.window.stateHUD.showState(newState);
        //this.window.outputHUD.refresh();

        if (this.view.frame) {
            this.view.updateActiveFramebuffer();
        }
    };
    TraceMinibar.prototype.stepUntil = function (callIndex) {
        if (this.controller.callIndex > callIndex) {
            this.controller.reset();
            this.controller.openFrame(this.view.frame, true);
            this.controller.stepUntil(callIndex);
        } else {
            this.controller.stepUntil(callIndex);
        }
        this.refreshState();
    };
    TraceMinibar.prototype.reset = function () {
        this.update();
    };
    TraceMinibar.prototype.update = function () {
        var self = this;

        if (this.view.frame) {
            this.controller.reset();
            this.controller.runFrame(this.view.frame);
            this.controller.openFrame(this.view.frame);
        } else {
            this.controller.reset();
            // TODO: clear canvas
            //console.log("would clear canvas");
        }

        function toggleButton(name, enabled) {
            var el = self.buttons[name];
            if (el) {
                if (enabled) {
                    el.className = el.className.replace("trace-minibar-button-disabled", "trace-minibar-button-enabled");
                } else {
                    el.className = el.className.replace("trace-minibar-button-enabled", "trace-minibar-button-disabled");
                }
            }
        };

        for (var n in this.buttons) {
            toggleButton(n, false);
        }

        toggleButton("run", true);
        toggleButton("step-forward", true);
        toggleButton("step-back", true);
        toggleButton("step-until-error", true);
        toggleButton("step-until-draw", true);
        toggleButton("restart", true);

        this.refreshState();

        //this.window.outputHUD.refresh();
    };

    var TraceView = function (w, elementRoot) {
        var self = this;
        var context = w.context;
        this.window = w;
        this.elements = {};
        this.minibar = new TraceMinibar(this, w, elementRoot);
        this.traceListing = new TraceListing(this, w, elementRoot);
        this.inspectorElements = {
            "window-trace-outer": elementRoot.getElementsByClassName("window-trace-outer")[0],
            "window-trace": elementRoot.getElementsByClassName("window-trace")[0],
            "window-trace-inspector": elementRoot.getElementsByClassName("window-trace-inspector")[0],
            "trace-minibar": elementRoot.getElementsByClassName("trace-minibar")[0]
        };
        this.inspector = new SurfaceInspector(this, w, elementRoot, {
            splitterKey: 'traceSplitter',
            title: 'Replay Preview',
            selectionName: 'Buffer',
            selectionValues: null /* set later */
        });
        this.inspector.activeFramebuffers = [];
        this.inspector.querySize = function () {
            if (this.activeFramebuffers) {
                var framebuffer = this.activeFramebuffers[this.optionsList.selectedIndex];
                if (framebuffer) {
                    var gl = this.gl;
                    var originalFramebuffer = gl.getParameter(gl.FRAMEBUFFER_BINDING);
                    gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer.mirror.target);
                    var texture = gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
                    gl.bindFramebuffer(gl.FRAMEBUFFER, originalFramebuffer);
                    if (texture && texture.trackedObject) {
                        return texture.trackedObject.guessSize(gl);
                    }
                }
            }
            return [context.canvas.width, context.canvas.height];
        };
        this.inspector.reset = function () {
            this.layout();
            if (w.windows.pixelHistory) {
                if (w.windows.pixelHistory.isOpened()) {
                    w.windows.pixelHistory.clear();
                } else {
                    w.windows.pixelHistory.close();
                }
            }
            if (w.windows.drawInfo) {
                if (w.windows.drawInfo.isOpened()) {
                    w.windows.drawInfo.clear();
                } else {
                    w.windows.drawInfo.close();
                }
            }
        };
        this.inspector.inspectPixel = function (x, y, locationString) {
            if (!self.frame) {
                return;
            }
            PopupWindow.show(w.context, PixelHistory, "pixelHistory", function (popup) {
                popup.inspectPixel(self.frame, x, y, locationString);
            });
        };
        this.inspector.setupPreview = function () {
            if (this.previewer) {
                return;
            }
            this.previewer = new TexturePreviewGenerator(this.canvas, true);
            this.gl = this.previewer.gl;
        };
        this.inspector.updatePreview = function () {
            this.layout();

            var gl = this.gl;
            gl.flush();

            // NOTE: index 0 is always null
            var framebuffer = this.activeFramebuffers[this.optionsList.selectedIndex];
            if (!framebuffer) {
                // Default framebuffer - redraw everything up to the current call (required as we've thrown out everything)
                this.canvas.width = context.canvas.width;
                this.canvas.height = context.canvas.height;
            }

            var controller = self.window.controller;
            var callIndex = controller.callIndex;
            controller.reset();
            controller.openFrame(self.frame, true);
            controller.stepUntil(callIndex - 1);

            if (framebuffer) {
                // User framebuffer - draw quad with the contents of the framebuffer
                var originalFramebuffer = gl.getParameter(gl.FRAMEBUFFER_BINDING);
                gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer.mirror.target);
                var texture = gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
                gl.bindFramebuffer(gl.FRAMEBUFFER, originalFramebuffer);
                if (texture) {
                    texture = texture.trackedObject;
                }
                if (texture) {
                    var size = texture.guessSize(gl);
                    var desiredWidth = 1;
                    var desiredHeight = 1;
                    if (size) {
                        desiredWidth = size[0];
                        desiredHeight = size[1];
                        this.canvas.style.display = "";
                    } else {
                        this.canvas.style.display = "none";
                    }
                    this.previewer.draw(texture, texture.currentVersion, null, desiredWidth, desiredHeight);
                } else {
                    // ?
                    console.log("invalid framebuffer attachment");
                    this.canvas.style.display = "none";
                }
            }
        };

        var canvas = this.inspector.canvas;
        canvas.style.display = "";
        w.controller.setOutput(canvas);
        // TODO: watch for parent canvas size changes and update
        canvas.width = this.window.context.canvas.width;
        canvas.height = this.window.context.canvas.height;
    };

    TraceView.prototype.setInspectorWidth = function (newWidth) {
        //.window-trace-outer margin-left: -480px !important; /* -2 * window-inspector.width */
        //.window-trace margin-left: 240px !important;
        //.trace-minibar right: 240px; /* window-trace-inspector */
        //.trace-listing right: 240px; /* window-trace-inspector */
        this.inspectorElements["window-trace-outer"].style.marginLeft = (-2 * newWidth) + "px";
        this.inspectorElements["window-trace"].style.marginLeft = newWidth + "px";
        this.inspectorElements["window-trace-inspector"].style.width = newWidth + "px";
        this.inspectorElements["trace-minibar"].style.right = newWidth + "px";
        this.traceListing.elements.list.style.right = newWidth + "px";
    };

    TraceView.prototype.layout = function () {
        if (this.inspector) this.inspector.layout();
    };

    TraceView.prototype.reset = function () {
        this.frame = null;

        this.minibar.reset();
        this.traceListing.reset();
        this.inspector.reset();
    };

    TraceView.prototype.setFrame = function (frame) {
        var gl = this.window.context;

        this.reset();
        this.frame = frame;

        // Check for redundancy, if required
        RedundancyChecker.checkFrame(frame);

        // Find interesting calls
        var bindFramebufferCalls = [];
        var errorCalls = [];
        for (var n = 0; n < frame.calls.length; n++) {
            var call = frame.calls[n];
            if (call.name == "bindFramebuffer") {
                bindFramebufferCalls.push(call);
            }
            if (call.error) {
                errorCalls.push(call);
            }
        }

        // Setup support for multiple framebuffers
        var activeFramebuffers = [];
        if (bindFramebufferCalls.length > 0) {
            for (var n = 0; n < bindFramebufferCalls.length; n++) {
                var call = bindFramebufferCalls[n];
                var framebuffer = call.args[1];
                if (framebuffer) {
                    if (activeFramebuffers.indexOf(framebuffer) == -1) {
                        activeFramebuffers.push(framebuffer);
                    }
                }
            }
        }
        if (activeFramebuffers.length) {
            var names = [];
            // Index 0 is always default - push to activeFramebuffers to keep consistent
            activeFramebuffers.unshift(null);
            for (var n = 0; n < activeFramebuffers.length; n++) {
                var framebuffer = activeFramebuffers[n];
                if (framebuffer) {
                    names.push(framebuffer.getName());
                } else {
                    names.push("Default");
                }
            }
            this.inspector.setSelectionValues(names);
            this.inspector.elements.faces.style.display = "";
            this.inspector.optionsList.selectedIndex = 0;
        } else {
            this.inspector.setSelectionValues(null);
            this.inspector.elements.faces.style.display = "none";
        }
        this.inspector.activeOption = 0;
        this.inspector.activeFramebuffers = activeFramebuffers;

        // Print out errors to console
        if (errorCalls.length) {
            console.log(" ");
            console.log("Frame " + frame.frameNumber + " errors:");
            console.log("----------------------");
            for (var n = 0; n < errorCalls.length; n++) {
                var call = errorCalls[n];

                var callString = traceLine.populateCallString(this.window.context, call);
                var errorString = info.enumToString(call.error);
                console.log(" " + errorString + " <= " + callString);

                // Stack (if present)
                if (call.stack) {
                    for (var m = 0; m < call.stack.length; m++) {
                        console.log("   - " + call.stack[m]);
                    }
                }
            }
            console.log(" ");
        }

        // Run the frame
        this.traceListing.setFrame(frame);
        this.minibar.update();
        this.traceListing.scrollToCall(0);
    };

    TraceView.prototype.guessActiveFramebuffer = function (callIndex) {
        // Can't trust the current state, so walk the calls to try to find a bindFramebuffer call
        for (var n = this.minibar.lastCallIndex - 1; n >= 0; n--) {
            var call = this.frame.calls[n];
            if (call.info.name == "bindFramebuffer") {
                return call.args[1];
                break;
            }
        }
        return null;
    };

    TraceView.prototype.updateActiveFramebuffer = function () {
        var gl = this.window.controller.output.gl;

        var callIndex = this.minibar.lastCallIndex - 1;
        var framebuffer = this.guessActiveFramebuffer(callIndex);

        if (this.inspector.activeFramebuffers.length) {
            for (var n = 0; n < this.inspector.activeFramebuffers.length; n++) {
                if (this.inspector.activeFramebuffers[n] == framebuffer) {
                    // Found in list at index n
                    if (this.inspector.optionsList.selectedIndex != n) {
                        // Differs - update to current
                        this.inspector.optionsList.selectedIndex = n;
                        this.inspector.activeOption = n;
                        this.inspector.updatePreview();
                    } else {
                        // Same - nothing to do
                        this.inspector.updatePreview();
                    }
                    break;
                }
            }
        }
    };

    TraceView.prototype.stepUntil = function (callIndex) {
        this.minibar.stepUntil(callIndex);
    };

    TraceView.prototype.getScrollState = function () {
        return {
            listing: this.traceListing.getScrollState()
        };
    };

    TraceView.prototype.setScrollState = function (state) {
        if (!state) {
            return;
        }
        this.traceListing.setScrollState(state.listing);
    };

    return TraceView;
});
