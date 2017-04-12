define([
        '../shared/Base',
        '../shared/GLConsts',
        '../shared/Utilities',
        './StateSnapshot',
    ], function (
        base,
        glc,
        util,
        StateSnapshot) {

    var CallType = {
        MARK: 0,
        GL: 1
    };

    var Call = function (ordinal, type, name, sourceArgs, frame) {
        this.ordinal = ordinal;
        this.time = (new Date()).getTime();

        this.type = type;
        this.name = name;
        this.stack = null;

        this.isRedundant = false;

        // Clone arguments
        var args = [];
        for (var n = 0; n < sourceArgs.length; n++) {
            if (sourceArgs[n] && sourceArgs[n].sourceUniformName) {
                args[n] = sourceArgs[n]; // TODO: pull out uniform reference
            } else {
                args[n] = util.clone(sourceArgs[n]);

                if (util.isWebGLResource(args[n])) {
                    var tracked = args[n].trackedObject;
                    args[n] = tracked;

                    // TODO: mark resource access based on type
                    if (true) {
                        frame.markResourceRead(tracked);
                    }
                    if (true) {
                        frame.markResourceWrite(tracked);
                    }
                }
            }
        }
        this.args = args;

        // Set upon completion
        this.duration = 0;
        this.result = null;
        this.error = null;
    };

    Call.prototype.complete = function (result, error, stack) {
        this.duration = (new Date()).getTime() - this.time;
        this.result = result;
        this.error = error;
        this.stack = stack;
    };

    Call.prototype.transformArgs = function (gl) {
        var args = [];
        for (var n = 0; n < this.args.length; n++) {
            args[n] = this.args[n];

            if (args[n]) {
                if (args[n].mirror) {
                    // Translate from resource -> mirror target
                    args[n] = args[n].mirror.target;
                } else if (args[n].sourceUniformName) {
                    // Get valid uniform location on new context
                    args[n] = gl.getUniformLocation(args[n].sourceProgram.mirror.target, args[n].sourceUniformName);
                }
            }
        }
        return args;
    };

    Call.prototype.emit = function (gl) {
        var args = this.transformArgs(gl);

        //while (gl.getError() != gl.NO_ERROR);

        // TODO: handle result?
        try {
            gl[this.name].apply(gl, args);
        } catch (e) {
            console.log("exception during replay of " + this.name + ": " + e);
        }
        //console.log("call " + call.name);

        //var error = gl.getError();
        //if (error != gl.NO_ERROR) {
        //    console.log(error);
        //}
    };

    var Frame = function (canvas, rawgl, frameNumber, resourceCache) {
        var attrs = rawgl.getContextAttributes ? rawgl.getContextAttributes() : {};
        this.canvasInfo = {
            width: canvas.width,
            height: canvas.height,
            attributes: attrs
        };

        this.frameNumber = frameNumber;
        this.initialState = new StateSnapshot(rawgl);
        this.screenshot = null;

        this.hasCheckedRedundancy = false;
        this.redundantCalls = 0;

        this.resourcesUsed = [];
        this.resourcesRead = [];
        this.resourcesWritten = [];

        this.calls = [];

        // Mark all bound resources as read
        for (var n in this.initialState) {
            var value = this.initialState[n];
            if (util.isWebGLResource(value)) {
                this.markResourceRead(value.trackedObject);
                // TODO: differentiate between framebuffers (as write) and the reads
            }
        }
        for (var n = 0; n < this.initialState.attribs.length; n++) {
            var attrib = this.initialState.attribs[n];
            for (var m in attrib) {
                var value = attrib[m];
                if (util.isWebGLResource(value)) {
                    this.markResourceRead(value.trackedObject);
                }
            }
        }

        this.resourceVersions = resourceCache.captureVersions();
        this.captureUniforms(rawgl, resourceCache.getPrograms());
    };

    Frame.prototype.captureUniforms = function (rawgl, allPrograms) {
        // Capture all program uniforms - nasty, but required to get accurate playback when not all uniforms are set each frame
        this.uniformValues = [];
        for (var n = 0; n < allPrograms.length; n++) {
            var program = allPrograms[n];
            var target = program.target;
            var values = {};

            var uniformCount = rawgl.getProgramParameter(target, rawgl.ACTIVE_UNIFORMS);
            for (var m = 0; m < uniformCount; m++) {
                var activeInfo = rawgl.getActiveUniform(target, m);
                if (activeInfo) {
                    var loc = rawgl.getUniformLocation(target, activeInfo.name);
                    var value = rawgl.getUniform(target, loc);
                    values[activeInfo.name] = {
                        size: activeInfo.size,
                        type: activeInfo.type,
                        value: value
                    };
                }
            }

            this.uniformValues.push({
                program: program,
                values: values
            });
        }
    };

    const uniformInfos = {}
    uniformInfos[glc.FLOAT]                         = { funcName: "uniform1f",          arrayFuncName: "uniform1fv"         };
    uniformInfos[glc.FLOAT_VEC2]                    = { funcName: "uniform2f",          arrayFuncName: "uniform2fv"         };
    uniformInfos[glc.FLOAT_VEC3]                    = { funcName: "uniform3f",          arrayFuncName: "uniform3fv"         };
    uniformInfos[glc.FLOAT_VEC4]                    = { funcName: "uniform4f",          arrayFuncName: "uniform4fv"         };
    uniformInfos[glc.INT]                           = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.INT_VEC2]                      = { funcName: "uniform2i",          arrayFuncName: "uniform2iv"         };
    uniformInfos[glc.INT_VEC3]                      = { funcName: "uniform3i",          arrayFuncName: "uniform3iv"         };
    uniformInfos[glc.INT_VEC4]                      = { funcName: "uniform4i",          arrayFuncName: "uniform4iv"         };
    uniformInfos[glc.UNSIGNED_INT]                  = { funcName: "uniform1ui",         arrayFuncName: "uniform1uiv"        };
    uniformInfos[glc.UNSIGNED_INT_VEC2]             = { funcName: "uniform2ui",         arrayFuncName: "uniform2uiv"        };
    uniformInfos[glc.UNSIGNED_INT_VEC3]             = { funcName: "uniform3ui",         arrayFuncName: "uniform3uiv"        };
    uniformInfos[glc.UNSIGNED_INT_VEC4]             = { funcName: "uniform4ui",         arrayFuncName: "uniform4uiv"        };
    uniformInfos[glc.BOOL]                          = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.BOOL_VEC2]                     = { funcName: "uniform2i",          arrayFuncName: "uniform2iv"         };
    uniformInfos[glc.BOOL_VEC3]                     = { funcName: "uniform3i",          arrayFuncName: "uniform3iv"         };
    uniformInfos[glc.BOOL_VEC4]                     = { funcName: "uniform4i",          arrayFuncName: "uniform4iv"         };
    uniformInfos[glc.FLOAT_MAT2]                    = { funcName: "uniformMatrix2fv",   arrayFuncName: "uniformMatrix2fv"   };
    uniformInfos[glc.FLOAT_MAT3]                    = { funcName: "uniformMatrix3fv",   arrayFuncName: "uniformMatrix3fv"   };
    uniformInfos[glc.FLOAT_MAT4]                    = { funcName: "uniformMatrix4fv",   arrayFuncName: "uniformMatrix4fv"   };
    uniformInfos[glc.FLOAT_MAT2x3]                  = { funcName: "uniformMatrix2x3fv", arrayFuncName: "uniformMatrix2x3fv" };
    uniformInfos[glc.FLOAT_MAT2x4]                  = { funcName: "uniformMatrix2x4fv", arrayFuncName: "uniformMatrix2x4fv" };
    uniformInfos[glc.FLOAT_MAT3x2]                  = { funcName: "uniformMatrix3x2fv", arrayFuncName: "uniformMatrix3x2fv" };
    uniformInfos[glc.FLOAT_MAT3x4]                  = { funcName: "uniformMatrix3x4fv", arrayFuncName: "uniformMatrix3x4fv" };
    uniformInfos[glc.FLOAT_MAT4x2]                  = { funcName: "uniformMatrix4x2fv", arrayFuncName: "uniformMatrix4x2fv" };
    uniformInfos[glc.FLOAT_MAT4x3]                  = { funcName: "uniformMatrix4x3fv", arrayFuncName: "uniformMatrix4x3fv" };
    uniformInfos[glc.SAMPLER_2D]                    = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_CUBE]                  = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_3D]                    = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_2D_SHADOW]             = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_2D_ARRAY]              = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_2D_ARRAY_SHADOW]       = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.SAMPLER_CUBE_SHADOW]           = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.INT_SAMPLER_2D]                = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.INT_SAMPLER_3D]                = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.INT_SAMPLER_CUBE]              = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.INT_SAMPLER_2D_ARRAY]          = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.UNSIGNED_INT_SAMPLER_2D]       = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.UNSIGNED_INT_SAMPLER_3D]       = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.UNSIGNED_INT_SAMPLER_CUBE]     = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };
    uniformInfos[glc.UNSIGNED_INT_SAMPLER_2D_ARRAY] = { funcName: "uniform1i",          arrayFuncName: "uniform1iv"         };

    Frame.prototype.applyUniforms = function (gl) {
        var originalProgram = gl.getParameter(gl.CURRENT_PROGRAM);

        for (var n = 0; n < this.uniformValues.length; n++) {
            var program = this.uniformValues[n].program;
            var values = this.uniformValues[n].values;

            var target = program.mirror.target;
            if (!target) {
                continue;
            }

            gl.useProgram(target);

            for (var name in values) {
                const data = values[name];
                const loc = gl.getUniformLocation(target, name);

                const info = uniformInfos[data.type];
                const funcName = (data.value && data.value.length !== undefined) ? info.arrayFuncName : info.funcName;

                if (funcName.indexOf("Matrix") != -1) {
                    gl[funcName].apply(gl, [loc, false, data.value]);
                } else {
                    gl[funcName].apply(gl, [loc, data.value]);
                }
            }
        }

        gl.useProgram(originalProgram);
    };

    Frame.prototype.end = function (rawgl) {
        var canvas = rawgl.canvas;

        // Take a picture! Note, this may fail for many reasons, but seems ok right now
        this.screenshot = document.createElement("canvas");
        var frag = document.createDocumentFragment();
        frag.appendChild(this.screenshot);
        this.screenshot.width = canvas.width;
        this.screenshot.height = canvas.height;
        var ctx2d = this.screenshot.getContext("2d");
        ctx2d.clearRect(0, 0, canvas.width, canvas.height);
        ctx2d.drawImage(canvas, 0, 0);
    };

    Frame.prototype.mark = function (args) {
        var call = new Call(this.calls.length, CallType.MARK, "mark", args, this);
        this.calls.push(call);
        call.complete(undefined, undefined); // needed?
        return call;
    };

    Frame.prototype.allocateCall = function (name, args) {
        var call = new Call(this.calls.length, CallType.GL, name, args, this);
        this.calls.push(call);
        return call;
    };

    Frame.prototype.findResourceVersion = function (resource) {
        for (var n = 0; n < this.resourceVersions.length; n++) {
            if (this.resourceVersions[n].resource == resource) {
                return this.resourceVersions[n].value;
            }
        }
        return null;
    };

    Frame.prototype.findResourceUsages = function (resource) {
        // Quick check to see if we have it marked as being used
        if (this.resourcesUsed.indexOf(resource) == -1) {
            // Unused this frame
            return null;
        }

        // Search all call args
        var usages = [];
        for (var n = 0; n < this.calls.length; n++) {
            var call = this.calls[n];
            for (var m = 0; m < call.args.length; m++) {
                if (call.args[m] == resource) {
                    usages.push(call);
                }
            }
        }
        return usages;
    };

    Frame.prototype.markResourceRead = function (resource) {
        // TODO: faster check (this can affect performance)
        if (resource) {
            if (this.resourcesUsed.indexOf(resource) == -1) {
                this.resourcesUsed.push(resource);
            }
            if (this.resourcesRead.indexOf(resource) == -1) {
                this.resourcesRead.push(resource);
            }
            if (resource.getDependentResources) {
                var dependentResources = resource.getDependentResources();
                for (var n = 0; n < dependentResources.length; n++) {
                    this.markResourceRead(dependentResources[n]);
                }
            }
        }
    };

    Frame.prototype.markResourceWrite = function (resource) {
        // TODO: faster check (this can affect performance)
        if (resource) {
            if (this.resourcesUsed.indexOf(resource) == -1) {
                this.resourcesUsed.push(resource);
            }
            if (this.resourcesWritten.indexOf(resource) == -1) {
                this.resourcesWritten.push(resource);
            }
            if (resource.getDependentResources) {
                var dependentResources = resource.getDependentResources();
                for (var n = 0; n < dependentResources.length; n++) {
                    this.markResourceWrite(dependentResources[n]);
                }
            }
        }
    };

    Frame.prototype.getResourcesUsedOfType = function (typename) {
        var results = [];
        for (var n = 0; n < this.resourcesUsed.length; n++) {
            var resource = this.resourcesUsed[n];
            if (!resource.target) {
                continue;
            }
            if (typename == base.typename(resource.target)) {
                results.push(resource);
            }
        }
        return results;
    };

    Frame.prototype._lookupResourceVersion = function (resource) {
        // TODO: faster lookup
        for (var m = 0; m < this.resourceVersions.length; m++) {
            if (this.resourceVersions[m].resource.id === resource.id) {
                return this.resourceVersions[m].value;
            }
        }
        return null;
    };

    Frame.prototype.makeActive = function (gl, force, options, exclusions) {
        options = options || {};
        exclusions = exclusions || [];

        // Sort resources by creation order - this ensures that shaders are ready before programs, etc
        // Since dependencies are fairly straightforward, this *should* be ok
        // 0 - Buffer
        // 1 - Texture
        // 2 - Renderbuffer
        // 3 - Framebuffer
        // 4 - Shader
        // 5 - Program
        this.resourcesUsed.sort(function (a, b) {
            return a.creationOrder - b.creationOrder;
        });

        for (var n = 0; n < this.resourcesUsed.length; n++) {
            var resource = this.resourcesUsed[n];
            if (exclusions.indexOf(resource) != -1) {
                continue;
            }

            var version = this._lookupResourceVersion(resource);
            if (!version) {
                continue;
            }

            resource.restoreVersion(gl, version, force, options);
        }

        this.initialState.apply(gl);
        this.applyUniforms(gl);
    };

    Frame.prototype.cleanup = function (gl) {
        // Unbind everything
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        gl.bindRenderbuffer(gl.RENDERBUFFER, null);
        gl.useProgram(null);
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
        var maxVertexAttrs = gl.getParameter(gl.MAX_VERTEX_ATTRIBS);
        var dummyBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, dummyBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Uint8Array(12), gl.STATIC_DRAW);
        for (var n = 0; n < maxVertexAttrs; n++) {
            gl.vertexAttribPointer(0, 1, gl.FLOAT, false, 0, 0);
        }
        gl.deleteBuffer(dummyBuffer);
        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);
        for (var n = 0; n < maxTextureUnits; n++) {
            gl.activeTexture(gl.TEXTURE0 + n);
            gl.bindTexture(gl.TEXTURE_2D, null);
            gl.bindTexture(gl.TEXTURE_CUBE_MAP, null);
        }

        // Dispose all objects
        for (var n = 0; n < this.resourcesUsed.length; n++) {
            var resource = this.resourcesUsed[n];
            resource.disposeMirror();
        }
    };

    Frame.prototype.switchMirrors = function (setName) {
        for (var n = 0; n < this.resourcesUsed.length; n++) {
            var resource = this.resourcesUsed[n];
            resource.switchMirror(setName);
        }
    };

    Frame.prototype.resetAllMirrors = function () {
        for (var n = 0; n < this.resourcesUsed.length; n++) {
            var resource = this.resourcesUsed[n];
            resource.disposeAllMirrors();
        }
    };

    Frame.CallType = CallType;
    Frame.Call = Call;

    return Frame;

});
