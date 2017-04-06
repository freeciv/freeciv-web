define([
        '../../shared/Base',
        '../Resource',
    ], function (
        base,
        Resource) {

    var VertexArray = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 2;

        this.defaultName = "VertexArray " + this.id;

        // Track the attributes (most importantly which buffers are attached
        // by watching vertexAttribPointer, vertexAttribIPointer,
        // and bindBuffer (do we need to track the individual values?
        this.attributes = {};
        this.parameters = {};
        this.elementBuffer = null;

        this.currentVersion.setParameters(this.parameters);
        this.currentVersion.setExtraParameters("attributes", this.attributes);
        this.currentVersion.setExtraParameters("elementBuffer", {buffer: this.elementBuffer});
    };

    VertexArray.prototype.getDependentResources = function () {
        var resources = [];
        Object.keys(this.attributes).forEach(ndx => {
            const attribute = this.attributes[ndx];
            const buffer = attribute.buffer;
            if (resources.indexOf(buffer) < 0) {
                resources.push(buffer);
            }
        });
        if (this.elementBuffer) {
            resources.push(this.elementBuffer);
        }
        return resources;
    };

    VertexArray.prototype.refresh = function (gl) {
    };

    VertexArray.getTracked = function (gl, args) {
        var glvao = gl.rawgl.getParameter(gl.VERTEX_ARRAY_BINDING);
        if (glvao == null) {
            // Going to fail
            return null;
        }
        return glvao.trackedObject;
    };

    function cloneAttributes(src) {
        const dst = {};
        Object.keys(src).forEach(ndx => {
            dst[ndx] = Object.assign({}, src[ndx]);
        });
        return dst;
    }

    VertexArray.setCaptures = function (gl) {
        const original_bindBuffer = gl.bindBuffer;
        gl.bindBuffer = function(target, buffer) {
            if (target == gl.ARRAY_BUFFER || target === gl.ELEMENT_ARRAY_BUFFER) {
                const tracked = VertexArray.getTracked(gl, arguments);
                if (tracked) {
                    tracked.currentVersion.pushCall("bindBuffer", arguments);
                    if (target === gl.ELEMENT_ARRAY_BUFFER) {
                        tracked.markDirty(false);
                        tracked.elementBuffer = buffer ? buffer.trackedObject : null;
                        tracked.currentVersion.setExtraParameters("elementBuffer", {buffer: tracked.elementBuffer});
                    }
                }
            }

            return original_bindBuffer.apply(gl, arguments);
        };

        const original_enableVertexAttribArray = gl.enableVertexAttribArray;
        gl.enableVertexAttribArray = function(index) {
            const tracked = VertexArray.getTracked(gl, arguments);
            if (tracked) {
              tracked.markDirty(false);
              tracked.currentVersion.pushCall("enableVertexAttribArray", arguments);
              const attribute = tracked.attributes[index] || {};
              tracked.attributes[index] = attribute;
              attribute.enabled = true;
              tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
            }
            return original_enableVertexAttribArray.apply(gl, arguments);
        }

        const original_disableVertexAttribArray = gl.disableVertexAttribArray;
        gl.disableVertexAttribArray = function(index) {
            const tracked = VertexArray.getTracked(gl, arguments);
            if (tracked) {
              tracked.markDirty(false);
              tracked.currentVersion.pushCall("disableVertexAttribArray", arguments);
              const attribute = tracked.attributes[index] || {};
              tracked.attributes[index] = attribute;
              attribute.enabled = false;
              tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
            }
            return original_disableVertexAttribArray.apply(gl, arguments);
        }

        const original_vertexAttribPointer = gl.vertexAttribPointer;
        gl.vertexAttribPointer = function(index, size, type, normalize, stride, offset) {
            const tracked = VertexArray.getTracked(gl, arguments);
            if (tracked) {
                tracked.markDirty(false);
                tracked.currentVersion.pushCall("vertexAttribPointer", arguments);
                const buffer = gl.getParameter(gl.ARRAY_BUFFER_BINDING);
                const attribute = tracked.attributes[index] || {};
                tracked.attributes[index] = attribute;
                attribute.size = size;
                attribute.type = type;
                attribute.normalize = normalize;
                attribute.stride = stride;
                attribute.offset = offset;
                attribute.buffer = buffer ? buffer.trackedObject : null;
                tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
            }
            return original_vertexAttribPointer.apply(gl, arguments);
        };

        const original_vertexAttribIPointer = gl.vertexAttribIPointer;
        gl.vertexAttribIPointer = function(index, size, type, stride, offset) {
            const tracked = VertexArray.getTracked(gl, arguments);
            if (tracked) {
                tracked.markDirty(false);
                tracked.currentVersion.pushCall("vertexAttribIPointer", arguments);
                const buffer = gl.rawgl.getParameter(gl.ARRAY_BUFFER_BINDING);
                const attribute = tracked.attributes[index] || {};
                tracked.attributes[index] = attribute;
                attribute.size = size;
                attribute.type = type;
                attribute.stride = stride;
                attribute.offset = offset;
                attribute.buffer = buffer ? buffer.trackedObject : null;
                tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
            }
            return original_vertexAttribIPointer.apply(gl, arguments);
        };

        const original_vertexAttribDivisor = gl.vertexAttribDivisor;
        gl.vertexAttribDivisor = function(index, divisor) {
            const tracked = VertexArray.getTracked(gl, arguments);
            if (tracked) {
              tracked.markDirty(false);
              tracked.currentVersion.pushCall("disableVertexAttribArray", arguments);
              const attribute = tracked.attributes[index] || {};
              tracked.attributes[index] = attribute;
              attribute.divisor = divisor;
              tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
            }
            return original_vertexAttribDivisor.apply(gl, arguments);
        };

        [
            "vertexAttrib1f",  // (GLuint indx, GLfloat x);
            "vertexAttrib2f",  // (GLuint indx, GLfloat x, GLfloat y);
            "vertexAttrib3f",  // (GLuint indx, GLfloat x, GLfloat y, GLfloat z);
            "vertexAttrib4f",  // (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
            "vertexAttribI4i",  // (GLuint index, GLint x, GLint y, GLint z, GLint w);
            "vertexAttribI4ui",  // (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
            "vertexAttrib1fv",  // (GLuint indx, Float32Array values);
            "vertexAttrib2fv",  // (GLuint indx, Float32Array values);
            "vertexAttrib3fv",  // (GLuint indx, Float32Array values);
            "vertexAttrib4fv",  // (GLuint indx, Float32Array values);
            "vertexAttribI4iv",  // (GLuint index, Int32List values);
            "vertexAttribI4uiv",  // (GLuint index, Uint32List values);
        ].forEach(funcName => {
          const original_vertexAttribFunc = gl[funcName];
          const numArgs = parseInt(/\d/.exec(funcName)[0]);
          const isArrayFn = funcName.substring(funcName.length - 1) === "v";
          const offset = isArrayFn ? 0 : 1;

          gl[funcName] = function(index) {
              const tracked = VertexArray.getTracked(gl, arguments);
              if (tracked) {
                  tracked.markDirty(false);
                  tracked.currentVersion.pushCall(funcName, arguments);
                  const attribute = tracked.attributes[index] || {};
                  const array = isArrayFn ? arguments : arguments[1];
                  tracked.attributes[index] = attribute;
                  attribute.value = [0, 0, 0, 1];
                  for (let i = 0; i < numArgs; ++i) {
                     attribute.value[i] = array[i + offset];
                  }
                  tracked.currentVersion.setExtraParameters("attributes", cloneAttributes(tracked.attributes));
              }
              return original_vertexAttribFunc.apply(gl, arguments);
          };
        });
    };

    VertexArray.prototype.createTarget = function (gl, version) {
        var vao = gl.createVertexArray();
        gl.bindVertexArray(vao);

        this.replayCalls(gl, version, vao);

        return vao;
    };

    VertexArray.prototype.deleteTarget = function (gl, target) {
        gl.deleteVertexArray(target);
    };

    return VertexArray;

});
