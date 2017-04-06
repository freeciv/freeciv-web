define([
        '../../host/CaptureContext',
        '../../shared/Base',
        '../../shared/Settings',
        '../../shared/Utilities',
        '../shared/PopupWindow',
        '../shared/TraceLine',
        '../Helpers',
    ], function (
        captureContext,
        base,
        settings,
        util,
        PopupWindow,
        traceLine,
        helpers
    ) {

    var PixelHistory = function (context, name) {
        base.subclass(PopupWindow, this, [context, name, "Pixel History", 926, 600]);
    };

    PixelHistory.prototype.setup = function () {
        var self = this;
        var context = this.context;
        var doc = this.browserWindow.document;

        var defaultShowDepthDiscarded = settings.session.showDepthDiscarded;
        this.addToolbarToggle("Show Depth Discarded Draws", "Display draws discarded by depth test", defaultShowDepthDiscarded, function (checked) {
            settings.session.showDepthDiscarded = checked;
            settings.save();

            if (self.current) {
                var current = self.current;
                self.inspectPixel(current.frame, current.x, current.y, current.locationString);
            }
        });

        var loadingMessage = this.loadingMessage = doc.createElement("div");
        loadingMessage.className = "pixelhistory-loading";
        loadingMessage.textContent = "Loading... (this may take awhile)";

        // TODO: move to shared code
        function prepareCanvas(canvas) {
            var frag = doc.createDocumentFragment();
            frag.appendChild(canvas);
            var gl = util.getWebGLContext(canvas, context.attributes, null);
            return gl;
        };
        this.canvas1 = doc.createElement("canvas");
        this.canvas2 = doc.createElement("canvas");
        this.gl1 = prepareCanvas(this.canvas1);
        this.gl2 = prepareCanvas(this.canvas2);
    };

    PixelHistory.prototype.dispose = function () {
        if (this.current) {
            var frame = this.current.frame;
            frame.switchMirrors("pixelhistory1");
            frame.cleanup(this.gl1);
            frame.switchMirrors("pixelhistory2");
            frame.cleanup(this.gl2);
            frame.switchMirrors();
        }
        this.current = null;
        this.canvas1 = this.canvas2 = null;
        this.gl1 = this.gl2 = null;
    };

    PixelHistory.prototype.clear = function () {
        this.current = null;

        this.browserWindow.document.title = "Pixel History";

        this.clearPanels();
    };

    PixelHistory.prototype.clearPanels = function () {
        var node = this.elements.innerDiv;
        node.scrollTop = 0;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
    };

    function addColor(doc, colorsLine, colorMask, name, canvas, subscript) {
        // Label
        // Canvas
        // rgba(r, g, b, a)

        var div = doc.createElement("div");
        div.className = "pixelhistory-color";

        var labelSpan = doc.createElement("span");
        labelSpan.className = "pixelhistory-color-label";
        labelSpan.textContent = name;
        div.appendChild(labelSpan);

        canvas.className = "gli-reset pixelhistory-color-canvas";
        div.appendChild(canvas);

        var rgba = getPixelRGBA(canvas.getContext("2d"));
        if (rgba) {
            var rgbaSpan = doc.createElement("span");
            rgbaSpan.className = "pixelhistory-color-rgba";
            var chanVals = {
                R: Math.floor(rgba[0] * 255),
                G: Math.floor(rgba[1] * 255),
                B: Math.floor(rgba[2] * 255),
                A: Math.floor(rgba[3] * 255),
            };
            Object.keys(chanVals).forEach(function (key, i) {
                var subscripthtml = document.createElement("sub");
                var strike = null;
                subscripthtml.textContent = subscript;
                rgbaSpan.appendChild(document.createTextNode(key));
                rgbaSpan.appendChild(subscripthtml);
                if (colorMask[i]) {
                    rgbaSpan.appendChild(document.createTextNode(": " + chanVals[key]));
                } else {
                    strike = document.createElement("strike");
                    strike.textContent = chanVals[key];
                    rgbaSpan.appendChild(document.createTextNode(": "));
                    rgbaSpan.appendChild(strike);
                }
                rgbaSpan.appendChild(document.createElement('br'));
            });
            div.appendChild(rgbaSpan);
        }

        colorsLine.appendChild(div);
    };

    PixelHistory.prototype.addPanel = function (gl, frame, call) {
        var doc = this.browserWindow.document;

        var panel = this.buildPanel();

        var callLine = doc.createElement("div");
        callLine.className = "pixelhistory-call";
        var callParent = callLine;
        if (call.history.isDepthDiscarded) {
            // If discarded by the depth test, strike out the line
            callParent = document.createElement("strike");
            callLine.appendChild(callParent);
        }
        traceLine.appendCallLine(this.context, callParent, frame, call);
        panel.appendChild(callLine);

        // Only add color info if not discarded
        if (!call.history.isDepthDiscarded) {
            var colorsLine = doc.createElement("div");
            colorsLine.className = "pixelhistory-colors";
            addColor(doc, colorsLine, call.history.colorMask, "Source", call.history.self, "s");
            addColor(doc, colorsLine, [true, true, true, true], "Dest", call.history.pre, "d");
            addColor(doc, colorsLine, [true, true, true, true], "Result", call.history.post, "r");

            if (call.history.blendEnabled) {
                var letters = ["R", "G", "B", "A"];
                var rgba_pre = getPixelRGBA(call.history.pre.getContext("2d"));
                var rgba_self = getPixelRGBA(call.history.self.getContext("2d"));
                var rgba_post = getPixelRGBA(call.history.post.getContext("2d"));
                var hasPixelValues = rgba_pre && rgba_self && rgba_post;
                var a_pre, a_self, a_post;
                if (hasPixelValues) {
                    a_pre = rgba_pre[3];
                    a_self = rgba_self[3];
                    a_post = rgba_post[3];
                }

                function genBlendString(index) {
                    var letter = letters[index];
                    var blendColor = call.history.blendColor[index];
                    var blendEqu;
                    var blendSrc;
                    var blendDst;
                    switch (index) {
                        case 0:
                        case 1:
                        case 2:
                            blendEqu = call.history.blendEquRGB;
                            blendSrc = call.history.blendSrcRGB;
                            blendDst = call.history.blendDstRGB;
                            break;
                        case 3:
                            blendEqu = call.history.blendEquAlpha;
                            blendSrc = call.history.blendSrcAlpha;
                            blendDst = call.history.blendDstAlpha;
                            break;
                    }

                    var x_pre = rgba_pre ? rgba_pre[index] : undefined;
                    var x_self = rgba_self ? rgba_self[index] : undefined;
                    var x_post = rgba_post ? rgba_post[index] : undefined;
                    function genFactor(factor) {
                        switch (factor) {
                            case gl.ZERO:
                                return ["0", 0];
                            case gl.ONE:
                            default:
                                return ["1", 1];
                            case gl.SRC_COLOR:
                                return [letter + "<sub>s</sub>", x_self];
                            case gl.ONE_MINUS_SRC_COLOR:
                                return ["1 - " + letter + "<sub>s</sub>", 1 - x_self];
                            case gl.DST_COLOR:
                                return [letter + "<sub>d</sub>", x_pre];
                            case gl.ONE_MINUS_DST_COLOR:
                                return ["1 - " + letter + "<sub>d</sub>", 1 - x_pre];
                            case gl.SRC_ALPHA:
                                return ["A<sub>s</sub>", a_self];
                            case gl.ONE_MINUS_SRC_ALPHA:
                                return ["1 - A<sub>s</sub>", 1 - a_self];
                            case gl.DST_ALPHA:
                                return ["A<sub>d</sub>", a_pre];
                            case gl.ONE_MINUS_DST_ALPHA:
                                return ["1 - A<sub>d</sub>", 1 - a_pre];
                            case gl.CONSTANT_COLOR:
                                return [letter + "<sub>c</sub>", blendColor[index]];
                            case gl.ONE_MINUS_CONSTANT_COLOR:
                                return ["1 - " + letter + "<sub>c</sub>", 1 - blendColor[index]];
                            case gl.CONSTANT_ALPHA:
                                return ["A<sub>c</sub>", blendColor[3]];
                            case gl.ONE_MINUS_CONSTANT_ALPHA:
                                return ["1 - A<sub>c</sub>", 1 - blendColor[3]];
                            case gl.SRC_ALPHA_SATURATE:
                                if (index == 3) {
                                    return ["1", 1];
                                } else {
                                    return ["i", Math.min(a_self, 1 - a_pre)];
                                }
                        }
                    };
                    var sfactor = genFactor(blendSrc);
                    var dfactor = genFactor(blendDst);
                    var s = letter + "<sub>s</sub>(" + sfactor[0] + ")";
                    var d = letter + "<sub>d</sub>(" + dfactor[0] + ")";
                    function fixFloat(n) {
                        var s = (Math.round(n * 10000) / 10000).toString();
                        if (s.length === 1) {
                            s += ".0000";
                        }
                        while (s.length < 6) {
                            s += "0";
                        }
                        return s;
                    };
                    var largs = ["s", "d"];
                    var args = [s, d];
                    var equstr = "";
                    switch (blendEqu) {
                        case gl.FUNC_ADD:
                            equstr = "+";
                            break;
                        case gl.FUNC_SUBTRACT:
                            equstr = "-";
                            break;
                        case gl.FUNC_REVERSE_SUBTRACT:
                            equstr = "-";
                            largs = ["d", "s"];
                            args = [d, s];
                            break;
                    }
                    var str = letter + "<sub>r</sub> = " + args[0] + " " + equstr + " " + args[1];
                    var nstr;
                    if (hasPixelValues) {
                        var ns = fixFloat(x_self) + "(" + fixFloat(sfactor[1]) + ")";
                        var nd = fixFloat(x_pre) + "(" + fixFloat(dfactor[1]) + ")";
                        var nargs = [ns, nd];
                        switch (blendEqu) {
                            case gl.FUNC_ADD:
                            case gl.FUNC_SUBTRACT:
                                break;
                            case gl.FUNC_REVERSE_SUBTRACT:
                                nargs = [nd, ns];
                                break;
                        }
                        nstr = fixFloat(x_post) + " = " + nargs[0] + "&nbsp;" + equstr + "&nbsp;" + nargs[1] + "<sub>&nbsp;</sub>"; // last sub for line height fix
                    } else {
                        nstr = "";
                    }
                    return [str, nstr];
                };
                var rs = genBlendString(0);
                var gs = genBlendString(1);
                var bs = genBlendString(2);
                var as = genBlendString(3);
                var blendingLine2 = doc.createElement("div");
                blendingLine2.className = "pixelhistory-blending pixelhistory-blending-equ";
                blendingLine2.appendChild(this.blendingLineFrag(rs[0], gs[0], bs[0], as[0]));
                colorsLine.appendChild(blendingLine2);
                if (hasPixelValues) {
                    var blendingLine1 = doc.createElement("div");
                    blendingLine1.className = "pixelhistory-blending pixelhistory-blending-values";
                    blendingLine1.appendChild(this.blendingLineFrag(rs[1], gs[1], bs[1], as[1]));
                    colorsLine.appendChild(blendingLine1);
                }
            } else {
                var blendingLine = doc.createElement("div");
                blendingLine.className = "pixelhistory-blending";
                blendingLine.textContent = "blending disabled";
                colorsLine.appendChild(blendingLine);
            }

            helpers.appendClear(colorsLine);
            panel.appendChild(colorsLine);
        }

        return panel;
    };

    PixelHistory.prototype.blendingLineFrag = function () {
      var frag = document.createDocumentFragment();
      for (var i = 0, len = arguments.length; i < len; ++i) {
        frag.appendChild(this.stringSubTagReplace(arguments[i]));
        frag.appendChild(document.createElement("br"));
      }
      return frag;
    };

    PixelHistory.prototype.stringSubTagReplace = function (str) {
      var frag = document.createDocumentFragment();
      var strs = str.replace(/&nbsp;/g, " ").split("</sub>");
      for (var i = 0, len = strs.length; i < len; ++i) {
        var pair = strs[i].split("<sub>");
        frag.appendChild(document.createTextNode(pair[0]));
        var sub = document.createElement("sub");
        sub.textContent = pair[1];
        frag.appendChild(sub);
      }
      return frag;
    };

    PixelHistory.prototype.addClear = function (gl, frame, call) {
        var panel = this.addPanel(gl, frame, call);

        //
    };

    PixelHistory.prototype.addDraw = function (gl, frame, call) {
        var panel = this.addPanel(gl, frame, call);

        //
    };

    function clearColorBuffer(gl) {
        var oldColorMask = gl.getParameter(gl.COLOR_WRITEMASK);
        var oldColorClearValue = gl.getParameter(gl.COLOR_CLEAR_VALUE);
        gl.colorMask(true, true, true, true);
        gl.clearColor(0, 0, 0, 0);
        gl.clear(gl.COLOR_BUFFER_BIT);
        gl.colorMask(oldColorMask[0], oldColorMask[1], oldColorMask[2], oldColorMask[3]);
        gl.clearColor(oldColorClearValue[0], oldColorClearValue[1], oldColorClearValue[2], oldColorClearValue[3]);
    };

    function getPixelRGBA(ctx) {
        var imageData = null;
        try {
            imageData = ctx.getImageData(0, 0, 1, 1);
        } catch (e) {
            // Likely a security error due to cross-domain dirty flag set on the canvas
        }
        if (imageData) {
            var r = imageData.data[0] / 255.0;
            var g = imageData.data[1] / 255.0;
            var b = imageData.data[2] / 255.0;
            var a = imageData.data[3] / 255.0;
            return [r, g, b, a];
        } else {
            console.log("unable to read back pixel");
            return null;
        }
    };

    function readbackRGBA(canvas, gl, x, y) {
        // NOTE: this call may fail due to security errors
        var pixel = new Uint8Array(4);
        try {
            gl.readPixels(x, canvas.height - y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
            return pixel;
        } catch (e) {
            console.log("unable to read back pixel");
            return null;
        }
    };

    function readbackPixel(canvas, gl, doc, x, y) {
        var readbackCanvas = doc.createElement("canvas");
        readbackCanvas.width = readbackCanvas.height = 1;
        var frag = doc.createDocumentFragment();
        frag.appendChild(readbackCanvas);
        var ctx = readbackCanvas.getContext("2d");

        // First attempt to read the pixel the fast way
        var pixel = readbackRGBA(canvas, gl, x, y);
        if (pixel) {
            // Fast - write to canvas and return
            var imageData = ctx.createImageData(1, 1);
            imageData.data[0] = pixel[0];
            imageData.data[1] = pixel[1];
            imageData.data[2] = pixel[2];
            imageData.data[3] = pixel[3];
            ctx.putImageData(imageData, 0, 0);
        } else {
            // Slow - blit entire canvas
            ctx.clearRect(0, 0, 1, 1);
            ctx.drawImage(canvas, x, y, 1, 1, 0, 0, 1, 1);
        }

        return readbackCanvas;
    };

    function gatherInterestingResources(gl, resourcesUsed) {
        var markResourceUsed = null;
        markResourceUsed = function (resource) {
            if (resourcesUsed.indexOf(resource) == -1) {
                resourcesUsed.push(resource);
            }
            if (resource.getDependentResources) {
                var dependentResources = resource.getDependentResources();
                for (var n = 0; n < dependentResources.length; n++) {
                    markResourceUsed(dependentResources[n]);
                }
            }
        };

        var currentProgram = gl.getParameter(gl.CURRENT_PROGRAM);
        if (currentProgram) {
            markResourceUsed(currentProgram.trackedObject);
        }

        var originalActiveTexture = gl.getParameter(gl.ACTIVE_TEXTURE);
        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);
        for (var n = 0; n < maxTextureUnits; n++) {
            gl.activeTexture(gl.TEXTURE0 + n);
            var tex2d = gl.getParameter(gl.TEXTURE_BINDING_2D);
            if (tex2d) {
                markResourceUsed(tex2d.trackedObject);
            }
            var texCube = gl.getParameter(gl.TEXTURE_BINDING_CUBE_MAP);
            if (texCube) {
                markResourceUsed(texCube.trackedObject);
            }
        }
        gl.activeTexture(originalActiveTexture);

        var indexBuffer = gl.getParameter(gl.ELEMENT_ARRAY_BUFFER_BINDING);
        if (indexBuffer) {
            markResourceUsed(indexBuffer.trackedObject);
        }

        var vertexBuffer = gl.getParameter(gl.ARRAY_BUFFER_BINDING);
        if (vertexBuffer) {
            markResourceUsed(vertexBuffer.trackedObject);
        }
        var maxVertexAttrs = gl.getParameter(gl.MAX_VERTEX_ATTRIBS);
        for (var n = 0; n < maxVertexAttrs; n++) {
            vertexBuffer = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);
            if (vertexBuffer) {
                markResourceUsed(vertexBuffer.trackedObject);
            }
        }
    };

    PixelHistory.prototype.beginLoading = function () {
        var doc = this.browserWindow.document;
        doc.body.style.cursor = "wait !important";
        this.elements.innerDiv.appendChild(this.loadingMessage);
    };

    PixelHistory.prototype.endLoading = function () {
        var doc = this.browserWindow.document;
        doc.body.style.cursor = "";
        this.elements.innerDiv.removeChild(this.loadingMessage);
    };

    PixelHistory.prototype.inspectPixel = function (frame, x, y, locationString) {
        var self = this;
        var doc = this.browserWindow.document;
        doc.title = "Pixel History: " + locationString;

        this.current = {
            frame: frame,
            x: x,
            y: y,
            locationString: locationString
        };

        this.clearPanels();
        this.beginLoading();

        captureContext.setTimeout(function () {
            self.inspectPixelCore(frame, x, y);
        }, 20);
    };

    PixelHistory.prototype.inspectPixelCore = function (frame, x, y) {
        var doc = this.browserWindow.document;

        var width = frame.canvasInfo.width;
        var height = frame.canvasInfo.height;

        var canvas1 = this.canvas1;
        var canvas2 = this.canvas2;
        canvas1.width = width; canvas1.height = height;
        canvas2.width = width; canvas2.height = height;
        var gl1 = this.gl1;
        var gl2 = this.gl2;

        // Canvas 1: no texture data, faked fragment shaders - for draw detection
        // Canvas 2: full playback - for color information

        // Prepare canvas 1 and hack all the programs
        var pass1Shader =
            "precision highp float;" +
            "void main() {" +
            "    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);" +
            "}";
        canvas1.width = 1; canvas1.width = width;
        frame.switchMirrors("pixelhistory1");
        frame.makeActive(gl1, false, {
            ignoreTextureUploads: true,
            fragmentShaderOverride: pass1Shader
        });

        // Issue all calls, read-back to detect changes, and mark the relevant calls
        var writeCalls = [];
        var resourcesUsed = [];
        frame.calls.forEach(function (call) {
            var needReadback = false;
            switch (call.name) {
                case "clear":
                    // Only deal with clears that affect the color buffer
                    if (call.args[0] & gl1.COLOR_BUFFER_BIT) {
                        needReadback = true;
                    }
                    break;
                case "drawArrays":
                case "drawElements":
                    needReadback = true;
                    break;
            }
            // If the current framebuffer is not the default one, skip the call
            // TODO: support pixel history on other framebuffers?
            if (gl1.getParameter(gl1.FRAMEBUFFER_BINDING)) {
                needReadback = false;
            }

            if (needReadback) {
                // Clear color buffer only (we need depth buffer to be valid)
                clearColorBuffer(gl1);
            }

            function applyPass1Call() {
                var originalBlendEnable = null;
                var originalColorMask = null;
                var unmungedColorClearValue = null;
                if (needReadback) {
                    // Disable blending during draws
                    originalBlendEnable = gl1.isEnabled(gl1.BLEND);
                    gl1.disable(gl1.BLEND);

                    // Enable all color channels to get fragment output
                    originalColorMask = gl1.getParameter(gl1.COLOR_WRITEMASK);
                    gl1.colorMask(true, true, true, true);

                    // Clear calls get munged so that we make sure we can see their effects
                    if (call.name == "clear") {
                        unmungedColorClearValue = gl1.getParameter(gl1.COLOR_CLEAR_VALUE);
                        gl1.clearColor(1, 1, 1, 1);
                    }
                }

                // Issue call
                call.emit(gl1);

                if (needReadback) {
                    // Restore blend mode
                    if (originalBlendEnable != null) {
                        if (originalBlendEnable) {
                            gl1.enable(gl1.BLEND);
                        } else {
                            gl1.disable(gl1.BLEND);
                        }
                    }

                    // Restore color mask
                    if (originalColorMask) {
                        gl1.colorMask(originalColorMask[0], originalColorMask[1], originalColorMask[2], originalColorMask[3]);
                    }

                    // Restore clear color
                    if (unmungedColorClearValue) {
                        gl1.clearColor(unmungedColorClearValue[0], unmungedColorClearValue[1], unmungedColorClearValue[2], unmungedColorClearValue[3]);
                    }
                }
            };
            applyPass1Call();

            var isWrite = false;
            function checkForPass1Write(isDepthDiscarded) {
                var rgba = readbackRGBA(canvas1, gl1, x, y);
                if (rgba && (rgba[0])) {
                    // Call had an effect!
                    isWrite = true;
                    call.history = {};
                    call.history.isDepthDiscarded = isDepthDiscarded;
                    call.history.colorMask = gl1.getParameter(gl1.COLOR_WRITEMASK);
                    call.history.blendEnabled = gl1.isEnabled(gl1.BLEND);
                    call.history.blendEquRGB = gl1.getParameter(gl1.BLEND_EQUATION_RGB);
                    call.history.blendEquAlpha = gl1.getParameter(gl1.BLEND_EQUATION_ALPHA);
                    call.history.blendSrcRGB = gl1.getParameter(gl1.BLEND_SRC_RGB);
                    call.history.blendSrcAlpha = gl1.getParameter(gl1.BLEND_SRC_ALPHA);
                    call.history.blendDstRGB = gl1.getParameter(gl1.BLEND_DST_RGB);
                    call.history.blendDstAlpha = gl1.getParameter(gl1.BLEND_DST_ALPHA);
                    call.history.blendColor = gl1.getParameter(gl1.BLEND_COLOR);
                    writeCalls.push(call);

                    // Stash off a bunch of useful resources
                    gatherInterestingResources(gl1, resourcesUsed);
                }
            };
            if (needReadback) {
                checkForPass1Write(false);
            }

            if (needReadback) {
                // If we are looking for depth discarded pixels and we were not picked up as a write, try again
                // NOTE: we only need to do this if depth testing is enabled!
                var isDepthTestEnabled = gl1.isEnabled(gl1.DEPTH_TEST);
                var isDraw = false;
                switch (call.name) {
                    case "drawArrays":
                    case "drawElements":
                        isDraw = true;
                        break;
                }
                if (isDraw && isDepthTestEnabled && !isWrite && settings.session.showDepthDiscarded) {
                    // Reset depth test settings
                    var originalDepthTest = gl1.isEnabled(gl1.DEPTH_TEST);
                    var originalDepthMask = gl1.getParameter(gl1.DEPTH_WRITEMASK);
                    gl1.disable(gl1.DEPTH_TEST);
                    gl1.depthMask(false);

                    // Call again
                    applyPass1Call();

                    // Restore depth test settings
                    if (originalDepthTest) {
                        gl1.enable(gl1.DEPTH_TEST);
                    } else {
                        gl1.disable(gl1.DEPTH_TEST);
                    }
                    gl1.depthMask(originalDepthMask);

                    // Check for write
                    checkForPass1Write(true);
                }
            }
        });

        // TODO: cleanup canvas 1 resources?

        // Find resources that were not used so we can exclude them
        var exclusions = [];
        // TODO: better search
        for (var n = 0; n < frame.resourcesUsed.length; n++) {
            var resource = frame.resourcesUsed[n];
            var typename = base.typename(resource.target);
            switch (typename) {
                case "WebGLTexture":
                case "WebGLProgram":
                case "WebGLShader":
                case "WebGLBuffer":
                    if (resourcesUsed.indexOf(resource) == -1) {
                        // Not used - exclude
                        exclusions.push(resource);
                    }
                    break;
            }
        }

        // Prepare canvas 2 for pulling out individual contribution
        canvas2.width = 1; canvas2.width = width;
        frame.switchMirrors("pixelhistory2");
        frame.makeActive(gl2, false, null, exclusions);

        for (var n = 0; n < frame.calls.length; n++) {
            var call = frame.calls[n];
            var isWrite = writeCalls.indexOf(call) >= 0;

            // Ignore things that don't affect this pixel
            var ignore = false;
            if (!isWrite) {
                switch (call.name) {
                    case "drawArrays":
                    case "drawElements":
                        ignore = true;
                        break;
                }
            }
            if (ignore) {
                continue;
            }

            var originalBlendEnable = null;
            var originalColorMask = null;
            if (isWrite) {
                // Clear color buffer only (we need depth buffer to be valid)
                clearColorBuffer(gl2);

                // Disable blending during draws
                originalBlendEnable = gl2.isEnabled(gl2.BLEND);
                gl2.disable(gl2.BLEND);

                // Enable all color channels to get fragment output
                originalColorMask = gl2.getParameter(gl2.COLOR_WRITEMASK);
                gl2.colorMask(true, true, true, true);
            }

            call.emit(gl2);

            if (isWrite) {
                // Restore blend mode
                if (originalBlendEnable != null) {
                    if (originalBlendEnable) {
                        gl2.enable(gl2.BLEND);
                    } else {
                        gl2.disable(gl2.BLEND);
                    }
                }

                // Restore color mask
                if (originalColorMask) {
                    gl2.colorMask(originalColorMask[0], originalColorMask[1], originalColorMask[2], originalColorMask[3]);
                }
            }

            if (isWrite) {
                // Read back the written fragment color
                call.history.self = readbackPixel(canvas2, gl2, doc, x, y);
            }
        }

        // Prepare canvas 2 for pulling out blending before/after
        canvas2.width = 1; canvas2.width = width;
        frame.makeActive(gl2, false, null, exclusions);

        for (var n = 0; n < frame.calls.length; n++) {
            var call = frame.calls[n];
            var isWrite = writeCalls.indexOf(call) >= 0;

            // Ignore things that don't affect this pixel
            var ignore = false;
            if (!isWrite) {
                switch (call.name) {
                    case "drawArrays":
                    case "drawElements":
                        ignore = true;
                        break;
                }
            }
            if (ignore) {
                continue;
            }

            if (isWrite) {
                // Read prior color
                call.history.pre = readbackPixel(canvas2, gl2, doc, x, y);
            }

            call.emit(gl2);

            if (isWrite) {
                // Read new color
                call.history.post = readbackPixel(canvas2, gl2, doc, x, y);
            }

            if (isWrite) {
                switch (call.name) {
                    case "clear":
                        this.addClear(gl2, frame, call);
                        break;
                    case "drawArrays":
                    case "drawElements":
                        this.addDraw(gl2, frame, call);
                        break;
                }
            }
        }

        // TODO: cleanup canvas 2 resources?

        // Restore all resource mirrors
        frame.switchMirrors(null);

        this.endLoading();
    };

    return PixelHistory;
});
