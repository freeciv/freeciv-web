define([
        '../shared/EventSource',
        '../shared/Info',
        '../shared/Utilities',
        '../host/StateSnapshot',
    ], function (
        EventSource,
        info,
        util,
        StateSnapshot
    ) {

    function clearToZeroAndRestoreParams(gl, bufferBits) {
        const oldColorMask = gl.getParameter(gl.COLOR_WRITEMASK);
        const oldColorClearValue = gl.getParameter(gl.COLOR_CLEAR_VALUE);
        const oldDepthMask = gl.getParameter(gl.DEPTH_WRITEMASK);
        const oldDepthClearValue = gl.getParameter(gl.DEPTH_CLEAR_VALUE);
        const oldStencilFrontMask = gl.getParameter(gl.STENCIL_WRITEMASK);
        const oldStencilBackMask = gl.getParameter(gl.STENCIL_BACK_WRITEMASK);
        const oldStencilClearValue = gl.getParameter(gl.STENCIL_CLEAR_VALUE);
        gl.colorMask(true, true, true, true);
        gl.clearColor(0, 0, 0, 0);
        gl.clearDepth(1);
        gl.stencilMask(0xFFFFFFFF);
        gl.clearStencil(0);
        gl.clear(bufferBits);
        gl.colorMask(oldColorMask[0], oldColorMask[1], oldColorMask[2], oldColorMask[3]);
        gl.clearColor(oldColorClearValue[0], oldColorClearValue[1], oldColorClearValue[2], oldColorClearValue[3]);
        gl.depthMask(oldDepthMask);
        gl.clearDepth(oldDepthClearValue);
        gl.stencilMaskSeparate(gl.FRONT, oldStencilFrontMask);
        gl.stencilMaskSeparate(gl.BACK, oldStencilBackMask);
        gl.clearStencil(oldStencilClearValue);
    }

    var Controller = function () {
        this.output = {};

        this.currentFrame = null;
        this.callIndex = 0;
        this.stepping = false;

        this.stepCompleted = new EventSource("stepCompleted");
    };

    Controller.prototype.setOutput = function (canvas) {
        this.output.canvas = canvas;

        // TODO: pull attributes from source somehow?
        var gl = this.output.gl = util.getWebGLContext(canvas, null, null);
        info.initialize(gl);
    };

    Controller.prototype.reset = function (force) {
        if (this.currentFrame) {
            var gl = this.output.gl;
            if (force) {
                this.currentFrame.cleanup(gl);
            }
        }

        this.currentFrame = null;
        this.callIndex = 0;
        this.stepping = false;
    };

    Controller.prototype.getCurrentState = function () {
        if (!this.output.gl) return null;
        return new StateSnapshot(this.output.gl);
    };

    Controller.prototype.openFrame = function (frame, suppressEvents, force, useDepthShader) {
        var gl = this.output.gl;

        // Canvas must match size when frame was captured otherwise viewport
        // and matrices etc will not match
        if (gl.canvas.width !== frame.canvasInfo.width || gl.canvas.heigth !== frame.canvasInfo.height) {
            gl.canvas.width = frame.canvasInfo.width;
            gl.canvas.height = frame.canvasInfo.height;
        }

        this.currentFrame = frame;

        if (useDepthShader) {
            frame.switchMirrors();
        } else {
            frame.switchMirrors("depth");
        }

        var depthShader = null;
        if (useDepthShader) {
            depthShader =
                "precision highp float;\n" +
                "vec4 packFloatToVec4i(const float value) {\n" +
                "   const vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);\n" +
                "   const vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);\n" +
                "   vec4 res = fract(value * bitSh);\n" +
                "   res -= res.xxyz * bitMsk;\n" +
                "   return res;\n" +
                "}\n" +
                "void main() {\n" +
                "   gl_FragColor = packFloatToVec4i(gl_FragCoord.z);\n" +
                //"   gl_FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);\n" +
                "}";
        }
        frame.makeActive(gl, force, {
            fragmentShaderOverride: depthShader,
            ignoreTextureUploads: useDepthShader
        });

        this.beginStepping();
        this.callIndex = 0;
        this.endStepping(suppressEvents);
    };

    function emitMark(mark) {
        console.log("mark hit");
    };

    Controller.prototype.issueCall = function (callIndex) {
        var gl = this.output.gl;

        if (this.currentFrame == null) {
            return false;
        }
        if (this.callIndex + 1 > this.currentFrame.calls.length) {
            return false;
        }

        if (callIndex >= 0) {
            this.callIndex = callIndex;
        } else {
            callIndex = this.callIndex;
        }

        var call = this.currentFrame.calls[callIndex];

        switch (call.type) {
            case 0: // MARK
                emitMark(call);
                break;
            case 1: // GL
                call.emit(gl);
                break;
        }

        return true;
    };

    Controller.prototype.beginStepping = function () {
        this.stepping = true;
    };

    Controller.prototype.endStepping = function (suppressEvents, overrideCallIndex) {
        this.stepping = false;
        if (!suppressEvents) {
            var callIndex = overrideCallIndex || this.callIndex;
            this.stepCompleted.fire(callIndex);
        }
    };

    Controller.prototype.stepUntil = function (callIndex) {
        if (this.callIndex > callIndex) {
            var frame = this.currentFrame;
            this.reset();
            this.openFrame(frame);
        }
        var wasStepping = this.stepping;
        if (!wasStepping) {
            this.beginStepping();
        }
        while (this.callIndex <= callIndex) {
            if (this.issueCall()) {
                this.callIndex++;
            } else {
                this.endStepping();
                return false;
            }
        }
        if (!wasStepping) {
            this.endStepping();
        }
        return true;
    };

    Controller.prototype.stepForward = function () {
        return this.stepUntil(this.callIndex);
    };

    Controller.prototype.stepBackward = function () {
        if (this.callIndex <= 1) {
            return false;
        }
        return this.stepUntil(this.callIndex - 2);
    };

    Controller.prototype.stepUntilError = function () {
        //
    };

    Controller.prototype.stepUntilDraw = function () {
        this.beginStepping();
        while (this.issueCall()) {
            var call = this.currentFrame.calls[this.callIndex];
            var funcInfo = info.functions[call.name];
            if (funcInfo.type == info.FunctionType.DRAW) {
                this.callIndex++;
                break;
            } else {
                this.callIndex++;
            }
        }
        this.endStepping();
    };

    Controller.prototype.stepUntilEnd = function () {
        this.beginStepping();
        while (this.stepForward());
        this.endStepping();
    };

    Controller.prototype.runFrame = function (frame) {
        const gl = this.output.gl;

        if (!frame.canvasInfo.attributes.preserveDrawingBuffer) {
            // TODO: should probably clear to 0, depth to 1, stencil to 0
            clearToZeroAndRestoreParams(gl, gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);
        }

        this.openFrame(frame);
        this.stepUntilEnd();
    };

    Controller.prototype.runIsolatedDraw = function (frame, targetCall) {
        this.openFrame(frame, true);

        var gl = this.output.gl;

        this.beginStepping();
        while (true) {
            var call = this.currentFrame.calls[this.callIndex];
            var shouldExec = false;

            if (call.name == "clear") {
                // Allow clear calls
                shouldExec = true;
            } else if (call == targetCall) {
                // The target call
                shouldExec = true;

                // Before executing the call, clear the color buffer
                clearToZeroAndRestoreParameters(g.COLOR_BUFFER_BIT);
            } else {
                var funcInfo = info.functions[call.name];
                if (funcInfo.type == info.FunctionType.DRAW) {
                    // Ignore all other draws
                    shouldExec = false;
                } else {
                    shouldExec = true;
                }
            }

            if (shouldExec) {
                if (!this.issueCall()) {
                    break;
                }
            }

            this.callIndex++;
            if (call == targetCall) {
                break;
            }
        }

        var finalCallIndex = this.callIndex;

        this.openFrame(frame, true);

        this.endStepping(false, finalCallIndex);
    };
    
    function packFloatToVec4i(value) {
       //vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
       //vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
       //vec4 res = fract(value * bitSh);
       var r = value * 256 * 256 * 256;
       var g = value * 256 * 256;
       var b = value * 256;
       var a = value;
       r = r - Math.floor(r);
       g = g - Math.floor(g);
       b = b - Math.floor(b);
       a = a - Math.floor(a);
       //res -= res.xxyz * bitMsk;
       g -= r / 256.0;
       b -= g / 256.0;
       a -= b / 256.0;
       return [r, g, b, a];
    };
    
    Controller.prototype.runDepthDraw = function (frame, targetCall) {
        this.openFrame(frame, true, undefined, true);

        var gl = this.output.gl;
        
        this.beginStepping();
        while (true) {
            var call = this.currentFrame.calls[this.callIndex];
            var shouldExec = true;
            
            var arg0;
            switch (call.name) {
            case "clear":
                arg0 = call.args[0];
                // Only allow depth clears if depth mask is set
                if (gl.getParameter(gl.DEPTH_WRITEMASK) == true) {
                    call.args[0] = call.args[0] & gl.DEPTH_BUFFER_BIT;
                    if (arg0 & gl.DEPTH_BUFFER_BIT) {
                        call.args[0] |= gl.COLOR_BUFFER_BIT;
                    }
                    var d = gl.getParameter(gl.DEPTH_CLEAR_VALUE);
                    var vd = packFloatToVec4i(d);
                    gl.clearColor(vd[0], vd[1], vd[2], vd[3]);
                } else {
                    shouldExec = false;
                }
                break;
            case "drawArrays":
            case "drawElements":
                // Only allow draws if depth mask is set
                if (gl.getParameter(gl.DEPTH_WRITEMASK) == true) {
                    // Reset state to what we need
                    gl.disable(gl.BLEND);
                    gl.colorMask(true, true, true, true);
                } else {
                    shouldExec = false;
                }
                break;
            default:
                break;
            }

            if (shouldExec) {
                if (!this.issueCall()) {
                    break;
                }
            }
            
            switch (call.name) {
            case "clear":
                call.args[0] = arg0;
                break;
            default:
                break;
            }

            this.callIndex++;
            if (call == targetCall) {
                break;
            }
        }

        var finalCallIndex = this.callIndex;

        this.openFrame(frame, true);

        this.endStepping(false, finalCallIndex);
    };

    return Controller;

});
