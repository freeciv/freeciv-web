define([
        '../../shared/Base',
        '../../shared/Utilities',
        '../Resource',
    ], function (
        base,
        util,
        Resource) {

    var Buffer = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 0;

        this.defaultName = "Buffer " + this.id;

        this.type = gl.ARRAY_BUFFER; // ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER

        this.parameters = {};
        this.parameters[gl.BUFFER_SIZE] = 0;
        this.parameters[gl.BUFFER_USAGE] = gl.STATIC_DRAW;

        this.currentVersion.type = this.type;
        this.currentVersion.structure = null;
        this.currentVersion.lastDrawState = null;
        this.currentVersion.setParameters(this.parameters);

        this.estimatedSize = 0;
    };

    Buffer.prototype.refresh = function (gl) {
        var paramEnums = [gl.BUFFER_SIZE, gl.BUFFER_USAGE];
        for (var n = 0; n < paramEnums.length; n++) {
            this.parameters[paramEnums[n]] = gl.getBufferParameter(this.type, paramEnums[n]);
        }
    };

    Buffer.getTracked = function (gl, args) {
        var bindingEnum;
        switch (args[0]) {
            default:
            case gl.ARRAY_BUFFER:
                bindingEnum = gl.ARRAY_BUFFER_BINDING;
                break;
            case gl.ELEMENT_ARRAY_BUFFER:
                bindingEnum = gl.ELEMENT_ARRAY_BUFFER_BINDING;
                break;
        }
        var glbuffer = gl.rawgl.getParameter(bindingEnum);
        if (glbuffer == null) {
            // Going to fail
            return null;
        }
        return glbuffer.trackedObject;
    };

    Buffer.setCaptures = function (gl) {
        var original_bufferData = gl.bufferData;
        gl.bufferData = function () {
            // Track buffer writes
            var totalBytes = 0;
            if (arguments[1] && arguments[1].byteLength) {
                totalBytes = arguments[1].byteLength;
            } else {
                totalBytes = arguments[1];
            }
            gl.statistics.bufferWrites.value += totalBytes;

            var tracked = Buffer.getTracked(gl, arguments);
            tracked.type = arguments[0];

            // Track total buffer bytes consumed
            gl.statistics.bufferBytes.value -= tracked.estimatedSize;
            gl.statistics.bufferBytes.value += totalBytes;
            tracked.estimatedSize = totalBytes;

            tracked.markDirty(true);
            tracked.currentVersion.target = tracked.type;
            tracked.currentVersion.structure = null;
            tracked.currentVersion.lastDrawState = null;
            tracked.currentVersion.pushCall("bufferData", arguments);
            var result = original_bufferData.apply(gl, arguments);
            tracked.refresh(gl.rawgl);
            tracked.currentVersion.setParameters(tracked.parameters);
            return result;
        };

        var original_bufferSubData = gl.bufferSubData;
        gl.bufferSubData = function () {
            // Track buffer writes
            var totalBytes = 0;
            if (arguments[2]) {
                totalBytes = arguments[2].byteLength;
            }
            gl.statistics.bufferWrites.value += totalBytes;

            var tracked = Buffer.getTracked(gl, arguments);
            tracked.type = arguments[0];
            tracked.markDirty(false);
            tracked.currentVersion.target = tracked.type;
            tracked.currentVersion.structure = null;
            tracked.currentVersion.lastDrawState = null;
            tracked.currentVersion.pushCall("bufferSubData", arguments);
            return original_bufferSubData.apply(gl, arguments);
        };

        // This is constant, so fetch once
        var maxVertexAttribs = gl.rawgl.getParameter(gl.MAX_VERTEX_ATTRIBS);

        function assignDrawStructure () {
            var rawgl = gl.rawgl;
            var mode = arguments[0];

            var drawState = {
                mode: mode,
                elementArrayBuffer: null,
                elementArrayBufferType: null,
                first: 0,
                offset: 0,
                count: 0
            };
            if (arguments.length == 3) {
                // drawArrays
                drawState.first = arguments[1];
                drawState.count = arguments[2];
            } else {
                // drawElements
                var glelementArrayBuffer = rawgl.getParameter(rawgl.ELEMENT_ARRAY_BUFFER_BINDING);
                drawState.elementArrayBuffer = glelementArrayBuffer ? glelementArrayBuffer.trackedObject : null;
                drawState.elementArrayBufferType = arguments[2];
                drawState.offset = arguments[3];
                drawState.count = arguments[1];
            }

            // TODO: cache all draw state so that we don't have to query each time
            var allDatas = {};
            var allBuffers = [];
            for (var n = 0; n < maxVertexAttribs; n++) {
                if (rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_ENABLED)) {
                    var glbuffer = rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);
                    var buffer = glbuffer.trackedObject;
                    if (buffer.currentVersion.structure) {
                        continue;
                    }

                    var size = rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_SIZE);
                    var stride = rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_STRIDE);
                    var offset = rawgl.getVertexAttribOffset(n, rawgl.VERTEX_ATTRIB_ARRAY_POINTER);
                    var type = rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_TYPE);
                    var normalized = rawgl.getVertexAttrib(n, rawgl.VERTEX_ATTRIB_ARRAY_NORMALIZED);

                    var datas = allDatas[buffer.id];
                    if (!datas) {
                        datas = allDatas[buffer.id] = [];
                        allBuffers.push(buffer);
                    }

                    datas.push({
                        size: size,
                        stride: stride,
                        offset: offset,
                        type: type,
                        normalized: normalized
                    });
                }
            }

            // TODO: build structure
            for (var n = 0; n < allBuffers.length; n++) {
                var buffer = allBuffers[n];
                var datas = allDatas[buffer.id];
                datas.sort(function (a, b) {
                    return a.offset - b.offset;
                });

                buffer.currentVersion.structure = datas;
                buffer.currentVersion.lastDrawState = drawState;
            }
        };

        function calculatePrimitiveCount(gl, mode, count) {
            switch (mode) {
                case gl.POINTS:
                    return count;
                case gl.LINE_STRIP:
                    return count - 1;
                case gl.LINE_LOOP:
                    return count;
                case gl.LINES:
                    return count / 2;
                case gl.TRIANGLE_STRIP:
                    return count - 2;
                default:
                case gl.TRIANGLES:
                    return count / 3;
            }
        };

        var origin_drawArrays = gl.drawArrays;
        gl.drawArrays = function () {
            //void drawArrays(GLenum mode, GLint first, GLsizei count);
            if (gl.captureFrame) {
                assignDrawStructure.apply(null, arguments);
            }

            // Track draw stats
            var totalPrimitives = calculatePrimitiveCount(gl, arguments[0], arguments[2]);
            gl.statistics.drawsPerFrame.value++;
            gl.statistics.primitivesPerFrame.value += totalPrimitives;

            return origin_drawArrays.apply(gl, arguments);
        };

        var origin_drawElements = gl.drawElements;
        gl.drawElements = function () {
            //void drawElements(GLenum mode, GLsizei count, GLenum type, GLsizeiptr offset);
            if (gl.captureFrame) {
                assignDrawStructure.apply(null, arguments);
            }

            // Track draw stats
            var totalPrimitives = calculatePrimitiveCount(gl, arguments[0], arguments[1]);
            gl.statistics.drawsPerFrame.value++;
            gl.statistics.primitivesPerFrame.value += totalPrimitives

            return origin_drawElements.apply(gl, arguments);
        };
    };

    Buffer.prototype.createTarget = function (gl, version, options) {
        options = options || {};

        var buffer = gl.createBuffer();
        gl.bindBuffer(version.target, buffer);

        // Filter uploads if requested
        var uploadFilter = null;
        if (options.ignoreBufferUploads) {
            uploadFilter = function uploadFilter(call, args) {
                if ((call.name == "bufferData") || (call.name == "bufferSubData")) {
                    return false;
                }
                return true;
            };
        }

        this.replayCalls(gl, version, buffer, uploadFilter);

        return buffer;
    };

    Buffer.prototype.deleteTarget = function (gl, target) {
        gl.deleteBuffer(target);
    };

    Buffer.prototype.constructVersion = function (gl, version) {
        // Find the last bufferData call to initialize the data
        var subDataCalls = [];
        var data = null;
        for (var n = version.calls.length - 1; n >= 0; n--) {
            var call = version.calls[n];
            if (call.name == "bufferData") {
                var sourceArray = call.args[1];
                if (sourceArray.constructor == Number) {
                    // Size - create an empty array
                    data = new ArrayBuffer(sourceArray);
                    break;
                } else {
                    // Has to be an ArrayBuffer or ArrayBufferView
                    data = util.clone(sourceArray);
                    break;
                }
            } else if (call.name == "bufferSubData") {
                // Queue up for later
                subDataCalls.unshift(call);
            }
        }
        if (!data) {
            // No bufferData calls!
            return [];
        }

        // Apply bufferSubData calls
        for (var n = 0; n < subDataCalls.length; n++) {
            var call = subDataCalls[n];
            var offset = call.args[1];
            var sourceArray = call.args[2];

            var view;
            switch (base.typename(sourceArray)) {
                case "Int8Array":
                    view = new Int8Array(data, offset);
                    break;
                case "Uint8Array":
                    view = new Uint8Array(data, offset);
                    break;
                case "Int16Array":
                    view = new Int16Array(data, offset);
                    break;
                case "Uint16Array":
                    view = new Uint16Array(data, offset);
                    break;
                case "Int32Array":
                    view = new Int32Array(data, offset);
                    break;
                case "Uint32Array":
                    view = new Uint32Array(data, offset);
                    break;
                case "Float32Array":
                    view = new Float32Array(data, offset);
                    break;
            }
            for (var i = 0; i < sourceArray.length; i++) {
                view[i] = sourceArray[i];
            }
        }

        return data;
    };

    return Buffer;

});
