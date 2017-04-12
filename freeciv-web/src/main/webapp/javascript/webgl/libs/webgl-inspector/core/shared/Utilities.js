// Hack to always define a console
if (!window["console"]) {
    window.console = { log: function () { } };
}

define([
        './Base',
        './Extensions',
        './Hacks'
    ], function (
        base,
        extensions,
        hacks
    ) {

    const util = {};

    util.setWebGLVersion = function( gl ) {
        util.useWebGL2 = util.isWebGL2(gl);
    }

    util.getWebGLContext = function (canvas, baseAttrs, attrs) {
        var finalAttrs = {
            preserveDrawingBuffer: true
        };

        // baseAttrs are all required and attrs are ORed in
        if (baseAttrs) {
            for (var k in baseAttrs) {
                finalAttrs[k] = baseAttrs[k];
            }
        }
        if (attrs) {
            for (var k in attrs) {
                if (finalAttrs[k] === undefined) {
                    finalAttrs[k] = attrs[k];
                } else {
                    finalAttrs[k] |= attrs[k];
                }
            }
        }

        function getContext(contextName) {
          var gl = null;
          try {
              if (canvas.getContextRaw) {
                  gl = canvas.getContextRaw(contextName, finalAttrs);
              } else {
                  gl = canvas.getContext(contextName, finalAttrs);
              }
          } catch (e) {
          }
          return gl;
        }

        // WebGL2 should **mostly** be compatible with webgl. At least at the start
        // Otherwise we need to some how know what kind of context we need (1 or 2)
        let gl;
        if (util.useWebGL2) {
            gl = getContext("webgl2");
        } else {
            gl = getContext("webgl") || getContext("experimental-webgl");
        }
        if (!gl) {
          // ?
          alert("Unable to get WebGL context: " + e);
        }

        if (gl) {
            extensions.enableAllExtensions(gl);
            hacks.installAll(gl);
        }

        return gl;
    };

    // Adjust TypedArray types to have consistent toString methods
    var typedArrayToString = function () {
        var s = "";
        var maxIndex = Math.min(64, this.length);
        for (var n = 0; n < maxIndex; n++) {
            s += this[n];
            if (n < this.length - 1) {
                s += ",";
            }
        }
        if (maxIndex < this.length) {
            s += ",... (" + (this.length) + " total)";
        }
        return s;
    };
    Int8Array.prototype.toString = typedArrayToString;
    Uint8Array.prototype.toString = typedArrayToString;
    Int16Array.prototype.toString = typedArrayToString;
    Uint16Array.prototype.toString = typedArrayToString;
    Int32Array.prototype.toString = typedArrayToString;
    Uint32Array.prototype.toString = typedArrayToString;
    Float32Array.prototype.toString = typedArrayToString;

    util.typedArrayToString = function (array) {
        if (array) {
            return typedArrayToString.apply(array);
        } else {
            return "(null)";
        }
    };

    util.isTypedArray = function (value) {
        if (value) {
            var typename = base.typename(value);
            switch (typename) {
                case "Int8Array":
                case "Uint8Array":
                case "Int16Array":
                case "Uint16Array":
                case "Int32Array":
                case "Uint32Array":
                case "Float32Array":
                    return true;
            }
            return false;
        } else {
            return false;
        }
    };

    util.arrayCompare = function (a, b) {
        if (a && b && a.length == b.length) {
            for (var n = 0; n < a.length; n++) {
                if (a[n] !== b[n]) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    };

    util.isWebGL2 = function (gl) {
        //return gl.getParameter(gl.VERSION).substring(0, 9) === "WebGL 2.0";
        return gl.texStorage2D !== undefined;
    };

    util.isWebGLResource = function (value) {
        if (value) {
            var typename = base.typename(value);
            switch (typename) {
                case "WebGLBuffer":
                case "WebGLFramebuffer":
                case "WebGLProgram":
                case "WebGLRenderbuffer":
                case "WebGLShader":
                case "WebGLTexture":
                case "WebGLQuery":
                case "WebGLSampler":
                case "WebGLSync":
                case "WebGLTransformFeedback":
                case "WebGLVertexArrayObject":
                    return true;
            }
            return false;
        } else {
            return false;
        }
    }

    function prepareDocumentElement(el) {
        // FF requires all content be in a document before it'll accept it for playback
        if (window.navigator.product == "Gecko") {
            var frag = document.createDocumentFragment();
            frag.appendChild(el);
        }
    };

    util.clone = function (arg) {
        if (arg) {
            if ((arg.constructor == Number) || (arg.constructor == String)) {
                // Fast path for immutables
                return arg;
            } else if (arg.constructor == Array) {
                return arg.slice(); // ghetto clone
            } else if (arg instanceof ArrayBuffer) {
                // There may be a better way to do this, but I don't know it
                var target = new ArrayBuffer(arg.byteLength);
                var sourceView = new DataView(arg, 0, arg.byteLength);
                var targetView = new DataView(target, 0, arg.byteLength);
                for (var n = 0; n < arg.byteLength; n++) {
                    targetView.setUint8(n, sourceView.getUint8(n));
                }
                return target;
            } else if (util.isTypedArray(arg)) {
                //} else if (arg instanceof ArrayBufferView) {
                // HACK: at least Chromium doesn't have ArrayBufferView as a known type (for some reason)
                var target = null;
                if (arg instanceof Int8Array) {
                    target = new Int8Array(arg);
                } else if (arg instanceof Uint8Array) {
                    target = new Uint8Array(arg);
                } else if (arg instanceof Int16Array) {
                    target = new Int16Array(arg);
                } else if (arg instanceof Uint16Array) {
                    target = new Uint16Array(arg);
                } else if (arg instanceof Int32Array) {
                    target = new Int32Array(arg);
                } else if (arg instanceof Uint32Array) {
                    target = new Uint32Array(arg);
                } else if (arg instanceof Float32Array) {
                    target = new Float32Array(arg);
                } else {
                    target = arg;
                }
                return target;
            } else if (base.typename(arg) == "ImageData") {
                var dummyCanvas = document.createElement("canvas");
                var dummyContext = dummyCanvas.getContext("2d");
                var target = dummyContext.createImageData(arg);
                for (var n = 0; n < arg.data.length; n++) {
                    target.data[n] = arg.data[n];
                }
                return target;
            } else if (arg instanceof HTMLCanvasElement) {
                // TODO: better way of doing this?
                var target = arg.cloneNode(true);
                var ctx = target.getContext("2d");
                ctx.drawImage(arg, 0, 0);
                prepareDocumentElement(target);
                return target;
            } else if (arg instanceof HTMLImageElement) {
                // TODO: clone image data (src?)
                var target = arg.cloneNode(true);
                target.width = arg.width;
                target.height = arg.height;
                prepareDocumentElement(target);
                return target;
            } else if (arg instanceof HTMLVideoElement) {
                // TODO: clone video data (is this even possible? we want the exact frame at the time of upload - maybe preserve seek time?)
                var target = arg.cloneNode(true);
                prepareDocumentElement(target);
                return target;
            } else {
                return arg;
            }
        } else {
            return arg;
        }
    };

    return util;
});
