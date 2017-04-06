define([
        '../shared/Utilities',
    ], function (
        util
    ) {

    var RedundancyChecker = function () {
        function prepareCanvas(canvas) {
            var frag = document.createDocumentFragment();
            frag.appendChild(canvas);
            var gl = util.getWebGLContext(canvas);
            return gl;
        };
        this.canvas = document.createElement("canvas");
        var gl = this.gl = prepareCanvas(this.canvas);

        // Cache off uniform name so that we can retrieve it later
        var original_getUniformLocation = gl.getUniformLocation;
        gl.getUniformLocation = function () {
            var result = original_getUniformLocation.apply(gl, arguments);
            if (result) {
                var tracked = arguments[0].trackedObject;
                result.sourceProgram = tracked;
                result.sourceUniformName = arguments[1];
            }
            return result;
        };
    };

    var stateCacheModifiers = {
        activeTexture: function (texture) {
            this.stateCache["ACTIVE_TEXTURE"] = texture;
        },
        bindBuffer: function (target, buffer) {
            switch (target) {
                case this.ARRAY_BUFFER:
                    this.stateCache["ARRAY_BUFFER_BINDING"] = buffer;
                    break;
                case this.ELEMENT_ARRAY_BUFFER:
                    this.stateCache["ELEMENT_ARRAY_BUFFER_BINDING"] = buffer;
                    break;
            }
        },
        bindFramebuffer: function (target, framebuffer) {
            this.stateCache["FRAMEBUFFER_BINDING"] = framebuffer;
        },
        bindRenderbuffer: function (target, renderbuffer) {
            this.stateCache["RENDERBUFFER_BINDING"] = renderbuffer;
        },
        bindTexture: function (target, texture) {
            var activeTexture = (this.stateCache["ACTIVE_TEXTURE"] - this.TEXTURE0);
            switch (target) {
                case this.TEXTURE_2D:
                    this.stateCache["TEXTURE_BINDING_2D_" + activeTexture] = texture;
                    break;
                case this.TEXTURE_CUBE_MAP:
                    this.stateCache["TEXTURE_BINDING_CUBE_MAP_" + activeTexture] = texture;
                    break;
                case this.TEXTURE_3D:
                    this.stateCache["TEXTURE_BINDING_3D_" + activeTexture] = texture;
                    break;
                case this.TEXTURE_2D_ARRAY:
                    this.stateCache["TEXTURE_BINDING_2D_ARRAY_" + activeTexture] = texture;
                    break;
            }
        },
        blendEquation: function (mode) {
            this.stateCache["BLEND_EQUATION_RGB"] = mode;
            this.stateCache["BLEND_EQUATION_ALPHA"] = mode;
        },
        blendEquationSeparate: function (modeRGB, modeAlpha) {
            this.stateCache["BLEND_EQUATION_RGB"] = modeRGB;
            this.stateCache["BLEND_EQUATION_ALPHA"] = modeAlpha;
        },
        blendFunc: function (sfactor, dfactor) {
            this.stateCache["BLEND_SRC_RGB"] = sfactor;
            this.stateCache["BLEND_SRC_ALPHA"] = sfactor;
            this.stateCache["BLEND_DST_RGB"] = dfactor;
            this.stateCache["BLEND_DST_ALPHA"] = dfactor;
        },
        blendFuncSeparate: function (srcRGB, dstRGB, srcAlpha, dstAlpha) {
            this.stateCache["BLEND_SRC_RGB"] = srcRGB;
            this.stateCache["BLEND_SRC_ALPHA"] = srcAlpha;
            this.stateCache["BLEND_DST_RGB"] = dstRGB;
            this.stateCache["BLEND_DST_ALPHA"] = dstAlpha;
        },
        clearColor: function (red, green, blue, alpha) {
            this.stateCache["COLOR_CLEAR_VALUE"] = [red, green, blue, alpha];
        },
        clearDepth: function (depth) {
            this.stateCache["DEPTH_CLEAR_VALUE"] = depth;
        },
        clearStencil: function (s) {
            this.stateCache["STENCIL_CLEAR_VALUE"] = s;
        },
        colorMask: function (red, green, blue, alpha) {
            this.stateCache["COLOR_WRITEMASK"] = [red, green, blue, alpha];
        },
        cullFace: function (mode) {
            this.stateCache["CULL_FACE_MODE"] = mode;
        },
        depthFunc: function (func) {
            this.stateCache["DEPTH_FUNC"] = func;
        },
        depthMask: function (flag) {
            this.stateCache["DEPTH_WRITEMASK"] = flag;
        },
        depthRange: function (zNear, zFar) {
            this.stateCache["DEPTH_RANGE"] = [zNear, zFar];
        },
        disable: function (cap) {
            switch (cap) {
                case this.BLEND:
                    this.stateCache["BLEND"] = false;
                    break;
                case this.CULL_FACE:
                    this.stateCache["CULL_FACE"] = false;
                    break;
                case this.DEPTH_TEST:
                    this.stateCache["DEPTH_TEST"] = false;
                    break;
                case this.POLYGON_OFFSET_FILL:
                    this.stateCache["POLYGON_OFFSET_FILL"] = false;
                    break;
                case this.SAMPLE_ALPHA_TO_COVERAGE:
                    this.stateCache["SAMPLE_ALPHA_TO_COVERAGE"] = false;
                    break;
                case this.SAMPLE_COVERAGE:
                    this.stateCache["SAMPLE_COVERAGE"] = false;
                    break;
                case this.SCISSOR_TEST:
                    this.stateCache["SCISSOR_TEST"] = false;
                    break;
                case this.STENCIL_TEST:
                    this.stateCache["STENCIL_TEST"] = false;
                    break;
            }
        },
        disableVertexAttribArray: function (index) {
            this.stateCache["VERTEX_ATTRIB_ARRAY_ENABLED_" + index] = false;
        },
        enable: function (cap) {
            switch (cap) {
                case this.BLEND:
                    this.stateCache["BLEND"] = true;
                    break;
                case this.CULL_FACE:
                    this.stateCache["CULL_FACE"] = true;
                    break;
                case this.DEPTH_TEST:
                    this.stateCache["DEPTH_TEST"] = true;
                    break;
                case this.POLYGON_OFFSET_FILL:
                    this.stateCache["POLYGON_OFFSET_FILL"] = true;
                    break;
                case this.SAMPLE_ALPHA_TO_COVERAGE:
                    this.stateCache["SAMPLE_ALPHA_TO_COVERAGE"] = true;
                    break;
                case this.SAMPLE_COVERAGE:
                    this.stateCache["SAMPLE_COVERAGE"] = true;
                    break;
                case this.SCISSOR_TEST:
                    this.stateCache["SCISSOR_TEST"] = true;
                    break;
                case this.STENCIL_TEST:
                    this.stateCache["STENCIL_TEST"] = true;
                    break;
            }
        },
        enableVertexAttribArray: function (index) {
            this.stateCache["VERTEX_ATTRIB_ARRAY_ENABLED_" + index] = true;
        },
        frontFace: function (mode) {
            this.stateCache["FRONT_FACE"] = mode;
        },
        hint: function (target, mode) {
            switch (target) {
                case this.GENERATE_MIPMAP_HINT:
                    this.stateCache["GENERATE_MIPMAP_HINT"] = mode;
                    break;
            }
        },
        lineWidth: function (width) {
            this.stateCache["LINE_WIDTH"] = width;
        },
        pixelStorei: function (pname, param) {
            switch (pname) {
                case this.PACK_ALIGNMENT:
                    this.stateCache["PACK_ALIGNMENT"] = param;
                    break;
                case this.UNPACK_ALIGNMENT:
                    this.stateCache["UNPACK_ALIGNMENT"] = param;
                    break;
                case this.UNPACK_COLORSPACE_CONVERSION_WEBGL:
                    this.stateCache["UNPACK_COLORSPACE_CONVERSION_WEBGL"] = param;
                    break;
                case this.UNPACK_FLIP_Y_WEBGL:
                    this.stateCache["UNPACK_FLIP_Y_WEBGL"] = param;
                    break;
                case this.UNPACK_PREMULTIPLY_ALPHA_WEBGL:
                    this.stateCache["UNPACK_PREMULTIPLY_ALPHA_WEBGL"] = param;
                    break;
            }
        },
        polygonOffset: function (factor, units) {
            this.stateCache["POLYGON_OFFSET_FACTOR"] = factor;
            this.stateCache["POLYGON_OFFSET_UNITS"] = units;
        },
        sampleCoverage: function (value, invert) {
            this.stateCache["SAMPLE_COVERAGE_VALUE"] = value;
            this.stateCache["SAMPLE_COVERAGE_INVERT"] = invert;
        },
        scissor: function (x, y, width, height) {
            this.stateCache["SCISSOR_BOX"] = [x, y, width, height];
        },
        stencilFunc: function (func, ref, mask) {
            this.stateCache["STENCIL_FUNC"] = func;
            this.stateCache["STENCIL_REF"] = ref;
            this.stateCache["STENCIL_VALUE_MASK"] = mask;
            this.stateCache["STENCIL_BACK_FUNC"] = func;
            this.stateCache["STENCIL_BACK_REF"] = ref;
            this.stateCache["STENCIL_BACK_VALUE_MASK"] = mask;
        },
        stencilFuncSeparate: function (face, func, ref, mask) {
            switch (face) {
                case this.FRONT:
                    this.stateCache["STENCIL_FUNC"] = func;
                    this.stateCache["STENCIL_REF"] = ref;
                    this.stateCache["STENCIL_VALUE_MASK"] = mask;
                    break;
                case this.BACK:
                    this.stateCache["STENCIL_BACK_FUNC"] = func;
                    this.stateCache["STENCIL_BACK_REF"] = ref;
                    this.stateCache["STENCIL_BACK_VALUE_MASK"] = mask;
                    break;
                case this.FRONT_AND_BACK:
                    this.stateCache["STENCIL_FUNC"] = func;
                    this.stateCache["STENCIL_REF"] = ref;
                    this.stateCache["STENCIL_VALUE_MASK"] = mask;
                    this.stateCache["STENCIL_BACK_FUNC"] = func;
                    this.stateCache["STENCIL_BACK_REF"] = ref;
                    this.stateCache["STENCIL_BACK_VALUE_MASK"] = mask;
                    break;
            }
        },
        stencilMask: function (mask) {
            this.stateCache["STENCIL_WRITEMASK"] = mask;
            this.stateCache["STENCIL_BACK_WRITEMASK"] = mask;
        },
        stencilMaskSeparate: function (face, mask) {
            switch (face) {
                case this.FRONT:
                    this.stateCache["STENCIL_WRITEMASK"] = mask;
                    break;
                case this.BACK:
                    this.stateCache["STENCIL_BACK_WRITEMASK"] = mask;
                    break;
                case this.FRONT_AND_BACK:
                    this.stateCache["STENCIL_WRITEMASK"] = mask;
                    this.stateCache["STENCIL_BACK_WRITEMASK"] = mask;
                    break;
            }
        },
        stencilOp: function (fail, zfail, zpass) {
            this.stateCache["STENCIL_FAIL"] = fail;
            this.stateCache["STENCIL_PASS_DEPTH_FAIL"] = zfail;
            this.stateCache["STENCIL_PASS_DEPTH_PASS"] = zpass;
            this.stateCache["STENCIL_BACK_FAIL"] = fail;
            this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] = zfail;
            this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] = zpass;
        },
        stencilOpSeparate: function (face, fail, zfail, zpass) {
            switch (face) {
                case this.FRONT:
                    this.stateCache["STENCIL_FAIL"] = fail;
                    this.stateCache["STENCIL_PASS_DEPTH_FAIL"] = zfail;
                    this.stateCache["STENCIL_PASS_DEPTH_PASS"] = zpass;
                    break;
                case this.BACK:
                    this.stateCache["STENCIL_BACK_FAIL"] = fail;
                    this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] = zfail;
                    this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] = zpass;
                    break;
                case this.FRONT_AND_BACK:
                    this.stateCache["STENCIL_FAIL"] = fail;
                    this.stateCache["STENCIL_PASS_DEPTH_FAIL"] = zfail;
                    this.stateCache["STENCIL_PASS_DEPTH_PASS"] = zpass;
                    this.stateCache["STENCIL_BACK_FAIL"] = fail;
                    this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] = zfail;
                    this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] = zpass;
                    break;
            }
        },
        uniformN: function (location, v) {
            if (!location) {
                return;
            }
            var program = location.sourceProgram;
            if (v.slice !== undefined) {
                v = v.slice();
            } else {
                v = new Float32Array(v);
            }
            program.uniformCache[location.sourceUniformName] = v;
        },
        uniform1f: function (location, v0) {
            stateCacheModifiers.uniformN.call(this, location, [v0]);
        },
        uniform2f: function (location, v0, v1) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1]);
        },
        uniform3f: function (location, v0, v1, v2) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1, v2]);
        },
        uniform4f: function (location, v0, v1, v2, v3) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1, v2, v3]);
        },
        uniform1i: function (location, v0) {
            stateCacheModifiers.uniformN.call(this, location, [v0]);
        },
        uniform2i: function (location, v0, v1) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1]);
        },
        uniform3i: function (location, v0, v1, v2) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1, v2]);
        },
        uniform4i: function (location, v0, v1, v2, v3) {
            stateCacheModifiers.uniformN.call(this, location, [v0, v1, v2, v3]);
        },
        uniform1fv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform2fv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform3fv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform4fv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform1iv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform2iv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform3iv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniform4iv: function (location, v) {
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniformMatrix2fv: function (location, transpose, v) {
            // TODO: transpose
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniformMatrix3fv: function (location, transpose, v) {
            // TODO: transpose
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        uniformMatrix4fv: function (location, transpose, v) {
            // TODO: transpose
            stateCacheModifiers.uniformN.call(this, location, v);
        },
        useProgram: function (program) {
            this.stateCache["CURRENT_PROGRAM"] = program;
        },
        vertexAttrib1f: function (indx, x) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [x, 0, 0, 1];
        },
        vertexAttrib2f: function (indx, x, y) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [x, y, 0, 1];
        },
        vertexAttrib3f: function (indx, x, y, z) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [x, y, z, 1];
        },
        vertexAttrib4f: function (indx, x, y, z, w) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [x, y, z, w];
        },
        vertexAttrib1fv: function (indx, v) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [v[0], 0, 0, 1];
        },
        vertexAttrib2fv: function (indx, v) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [v[0], v[1], 0, 1];
        },
        vertexAttrib3fv: function (indx, v) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [v[0], v[1], v[2], 1];
        },
        vertexAttrib4fv: function (indx, v) {
            this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx] = [v[0], v[1], v[2], v[3]];
        },
        vertexAttribPointer: function (indx, size, type, normalized, stride, offset) {
            this.stateCache["VERTEX_ATTRIB_ARRAY_SIZE_" + indx] = size;
            this.stateCache["VERTEX_ATTRIB_ARRAY_TYPE_" + indx] = type;
            this.stateCache["VERTEX_ATTRIB_ARRAY_NORMALIZED_" + indx] = normalized;
            this.stateCache["VERTEX_ATTRIB_ARRAY_STRIDE_" + indx] = stride;
            this.stateCache["VERTEX_ATTRIB_ARRAY_POINTER_" + indx] = offset;
            this.stateCache["VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_" + indx] = this.stateCache["ARRAY_BUFFER_BINDING"];
        },
        viewport: function (x, y, width, height) {
            this.stateCache["VIEWPORT"] = [x, y, width, height];
        }
    };

    var redundantChecks = {
        activeTexture: function (texture) {
            return this.stateCache["ACTIVE_TEXTURE"] == texture;
        },
        bindBuffer: function (target, buffer) {
            switch (target) {
                case this.ARRAY_BUFFER:
                    return this.stateCache["ARRAY_BUFFER_BINDING"] == buffer;
                case this.ELEMENT_ARRAY_BUFFER:
                    return this.stateCache["ELEMENT_ARRAY_BUFFER_BINDING"] == buffer;
                default:
                    return false;
            }
        },
        bindFramebuffer: function (target, framebuffer) {
            return this.stateCache["FRAMEBUFFER_BINDING"] == framebuffer;
        },
        bindRenderbuffer: function (target, renderbuffer) {
            return this.stateCache["RENDERBUFFER_BINDING"] == renderbuffer;
        },
        bindTexture: function (target, texture) {
            var activeTexture = (this.stateCache["ACTIVE_TEXTURE"] - this.TEXTURE0);
            switch (target) {
                case this.TEXTURE_2D:
                    return this.stateCache["TEXTURE_BINDING_2D_" + activeTexture] == texture;
                case this.TEXTURE_CUBE_MAP:
                    return this.stateCache["TEXTURE_BINDING_CUBE_MAP_" + activeTexture] == texture;
                case this.TEXTURE_3D:
                    return this.stateCache["TEXTURE_BINDING_3D_" + activeTexture] == texture;
                case this.TEXTURE_2D_ARRAY:
                    return this.stateCache["TEXTURE_BINDING_2D_ARRAY_" + activeTexture] == texture;
            }
            return false;
        },
        blendEquation: function (mode) {
            return (this.stateCache["BLEND_EQUATION_RGB"] == mode) && (this.stateCache["BLEND_EQUATION_ALPHA"] == mode);
        },
        blendEquationSeparate: function (modeRGB, modeAlpha) {
            return (this.stateCache["BLEND_EQUATION_RGB"] == modeRGB) && (this.stateCache["BLEND_EQUATION_ALPHA"] == modeAlpha);
        },
        blendFunc: function (sfactor, dfactor) {
            return (this.stateCache["BLEND_SRC_RGB"] == sfactor) && (this.stateCache["BLEND_SRC_ALPHA"] == sfactor) &&
                   (this.stateCache["BLEND_DST_RGB"] == dfactor) && (this.stateCache["BLEND_DST_ALPHA"] == dfactor);
        },
        blendFuncSeparate: function (srcRGB, dstRGB, srcAlpha, dstAlpha) {
            return (this.stateCache["BLEND_SRC_RGB"] == srcRGB) && (this.stateCache["BLEND_SRC_ALPHA"] == srcAlpha) &&
                   (this.stateCache["BLEND_DST_RGB"] == dstRGB) && (this.stateCache["BLEND_DST_ALPHA"] == dstAlpha);
        },
        clearColor: function (red, green, blue, alpha) {
            return util.arrayCompare(this.stateCache["COLOR_CLEAR_VALUE"], [red, green, blue, alpha]);
        },
        clearDepth: function (depth) {
            return this.stateCache["DEPTH_CLEAR_VALUE"] == depth;
        },
        clearStencil: function (s) {
            return this.stateCache["STENCIL_CLEAR_VALUE"] == s;
        },
        colorMask: function (red, green, blue, alpha) {
            return util.arrayCompare(this.stateCache["COLOR_WRITEMASK"], [red, green, blue, alpha]);
        },
        cullFace: function (mode) {
            return this.stateCache["CULL_FACE_MODE"] == mode;
        },
        depthFunc: function (func) {
            return this.stateCache["DEPTH_FUNC"] == func;
        },
        depthMask: function (flag) {
            return this.stateCache["DEPTH_WRITEMASK"] == flag;
        },
        depthRange: function (zNear, zFar) {
            return util.arrayCompare(this.stateCache["DEPTH_RANGE"], [zNear, zFar]);
        },
        disable: function (cap) {
            switch (cap) {
                case this.BLEND:
                    return this.stateCache["BLEND"] == false;
                case this.CULL_FACE:
                    return this.stateCache["CULL_FACE"] == false;
                case this.DEPTH_TEST:
                    return this.stateCache["DEPTH_TEST"] == false;
                case this.POLYGON_OFFSET_FILL:
                    return this.stateCache["POLYGON_OFFSET_FILL"] == false;
                case this.SAMPLE_ALPHA_TO_COVERAGE:
                    return this.stateCache["SAMPLE_ALPHA_TO_COVERAGE"] == false;
                case this.SAMPLE_COVERAGE:
                    return this.stateCache["SAMPLE_COVERAGE"] == false;
                case this.SCISSOR_TEST:
                    return this.stateCache["SCISSOR_TEST"] == false;
                case this.STENCIL_TEST:
                    return this.stateCache["STENCIL_TEST"] == false;
                default:
                    return false;
            }
        },
        disableVertexAttribArray: function (index) {
            return this.stateCache["VERTEX_ATTRIB_ARRAY_ENABLED_" + index] == false;
        },
        enable: function (cap) {
            switch (cap) {
                case this.BLEND:
                    return this.stateCache["BLEND"] == true;
                case this.CULL_FACE:
                    return this.stateCache["CULL_FACE"] == true;
                case this.DEPTH_TEST:
                    return this.stateCache["DEPTH_TEST"] == true;
                case this.POLYGON_OFFSET_FILL:
                    return this.stateCache["POLYGON_OFFSET_FILL"] == true;
                case this.SAMPLE_ALPHA_TO_COVERAGE:
                    return this.stateCache["SAMPLE_ALPHA_TO_COVERAGE"] == true;
                case this.SAMPLE_COVERAGE:
                    return this.stateCache["SAMPLE_COVERAGE"] == true;
                case this.SCISSOR_TEST:
                    return this.stateCache["SCISSOR_TEST"] == true;
                case this.STENCIL_TEST:
                    return this.stateCache["STENCIL_TEST"] == true;
                default:
                    return false;
            }
        },
        enableVertexAttribArray: function (index) {
            return this.stateCache["VERTEX_ATTRIB_ARRAY_ENABLED_" + index] == true;
        },
        frontFace: function (mode) {
            return this.stateCache["FRONT_FACE"] == mode;
        },
        hint: function (target, mode) {
            switch (target) {
                case this.GENERATE_MIPMAP_HINT:
                    return this.stateCache["GENERATE_MIPMAP_HINT"] == mode;
                default:
                    return false;
            }
        },
        lineWidth: function (width) {
            return this.stateCache["LINE_WIDTH"] == width;
        },
        pixelStorei: function (pname, param) {
            switch (pname) {
                case this.PACK_ALIGNMENT:
                    return this.stateCache["PACK_ALIGNMENT"] == param;
                case this.UNPACK_ALIGNMENT:
                    return this.stateCache["UNPACK_ALIGNMENT"] == param;
                case this.UNPACK_COLORSPACE_CONVERSION_WEBGL:
                    return this.stateCache["UNPACK_COLORSPACE_CONVERSION_WEBGL"] == param;
                case this.UNPACK_FLIP_Y_WEBGL:
                    return this.stateCache["UNPACK_FLIP_Y_WEBGL"] == param;
                case this.UNPACK_PREMULTIPLY_ALPHA_WEBGL:
                    return this.stateCache["UNPACK_PREMULTIPLY_ALPHA_WEBGL"] == param;
                default:
                    return false;
            }
        },
        polygonOffset: function (factor, units) {
            return (this.stateCache["POLYGON_OFFSET_FACTOR"] == factor) && (this.stateCache["POLYGON_OFFSET_UNITS"] == units);
        },
        sampleCoverage: function (value, invert) {
            return (this.stateCache["SAMPLE_COVERAGE_VALUE"] == value) && (this.stateCache["SAMPLE_COVERAGE_INVERT"] == invert);
        },
        scissor: function (x, y, width, height) {
            return util.arrayCompare(this.stateCache["SCISSOR_BOX"], [x, y, width, height]);
        },
        stencilFunc: function (func, ref, mask) {
            return (this.stateCache["STENCIL_FUNC"] == func) && (this.stateCache["STENCIL_REF"] == ref) && (this.stateCache["STENCIL_VALUE_MASK"] == mask) &&
                   (this.stateCache["STENCIL_BACK_FUNC"] == func) && (this.stateCache["STENCIL_BACK_REF"] == ref) && (this.stateCache["STENCIL_BACK_VALUE_MASK"] == mask);
        },
        stencilFuncSeparate: function (face, func, ref, mask) {
            switch (face) {
                case this.FRONT:
                    return (this.stateCache["STENCIL_FUNC"] == func) && (this.stateCache["STENCIL_REF"] == ref) && (this.stateCache["STENCIL_VALUE_MASK"] == mask);
                case this.BACK:
                    return (this.stateCache["STENCIL_BACK_FUNC"] == func) && (this.stateCache["STENCIL_BACK_REF"] == ref) && (this.stateCache["STENCIL_BACK_VALUE_MASK"] == mask);
                case this.FRONT_AND_BACK:
                    return (this.stateCache["STENCIL_FUNC"] == func) && (this.stateCache["STENCIL_REF"] == ref) && (this.stateCache["STENCIL_VALUE_MASK"] == mask) &&
                           (this.stateCache["STENCIL_BACK_FUNC"] == func) && (this.stateCache["STENCIL_BACK_REF"] == ref) && (this.stateCache["STENCIL_BACK_VALUE_MASK"] == mask);
                default:
                    return false;
            }
        },
        stencilMask: function (mask) {
            return (this.stateCache["STENCIL_WRITEMASK"] == mask) && (this.stateCache["STENCIL_BACK_WRITEMASK"] == mask);
        },
        stencilMaskSeparate: function (face, mask) {
            switch (face) {
                case this.FRONT:
                    return this.stateCache["STENCIL_WRITEMASK"] == mask;
                case this.BACK:
                    return this.stateCache["STENCIL_BACK_WRITEMASK"] == mask;
                case this.FRONT_AND_BACK:
                    return (this.stateCache["STENCIL_WRITEMASK"] == mask) && (this.stateCache["STENCIL_BACK_WRITEMASK"] == mask);
                default:
                    return false;
            }
        },
        stencilOp: function (fail, zfail, zpass) {
            return (this.stateCache["STENCIL_FAIL"] == fail) && (this.stateCache["STENCIL_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_PASS_DEPTH_PASS"] == zpass) &&
                   (this.stateCache["STENCIL_BACK_FAIL"] == fail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] == zpass);
        },
        stencilOpSeparate: function (face, fail, zfail, zpass) {
            switch (face) {
                case this.FRONT:
                    return (this.stateCache["STENCIL_FAIL"] == fail) && (this.stateCache["STENCIL_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_PASS_DEPTH_PASS"] == zpass);
                case this.BACK:
                    return (this.stateCache["STENCIL_BACK_FAIL"] == fail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] == zpass);
                case this.FRONT_AND_BACK:
                    return (this.stateCache["STENCIL_FAIL"] == fail) && (this.stateCache["STENCIL_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_PASS_DEPTH_PASS"] == zpass) &&
                           (this.stateCache["STENCIL_BACK_FAIL"] == fail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_FAIL"] == zfail) && (this.stateCache["STENCIL_BACK_PASS_DEPTH_PASS"] == zpass);
                default:
                    return false;
            }
        },
        uniformN: function (location, v) {
            if (!location) {
                return true;
            }
            var program = location.sourceProgram;
            if (!program.uniformCache) return false;
            return util.arrayCompare(program.uniformCache[location.sourceUniformName], v);
        },
        uniform1f: function (location, v0) {
            return redundantChecks.uniformN.call(this, location, [v0]);
        },
        uniform2f: function (location, v0, v1) {
            return redundantChecks.uniformN.call(this, location, [v0, v1]);
        },
        uniform3f: function (location, v0, v1, v2) {
            return redundantChecks.uniformN.call(this, location, [v0, v1, v2]);
        },
        uniform4f: function (location, v0, v1, v2, v3) {
            return redundantChecks.uniformN.call(this, location, [v0, v1, v2, v3]);
        },
        uniform1i: function (location, v0) {
            return redundantChecks.uniformN.call(this, location, [v0]);
        },
        uniform2i: function (location, v0, v1) {
            return redundantChecks.uniformN.call(this, location, [v0, v1]);
        },
        uniform3i: function (location, v0, v1, v2) {
            return redundantChecks.uniformN.call(this, location, [v0, v1, v2]);
        },
        uniform4i: function (location, v0, v1, v2, v3) {
            return redundantChecks.uniformN.call(this, location, [v0, v1, v2, v3]);
        },
        uniform1fv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform2fv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform3fv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform4fv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform1iv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform2iv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform3iv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniform4iv: function (location, v) {
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniformMatrix2fv: function (location, transpose, v) {
            // TODO: transpose
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniformMatrix3fv: function (location, transpose, v) {
            // TODO: transpose
            return redundantChecks.uniformN.call(this, location, v);
        },
        uniformMatrix4fv: function (location, transpose, v) {
            // TODO: transpose
            return redundantChecks.uniformN.call(this, location, v);
        },
        useProgram: function (program) {
            return this.stateCache["CURRENT_PROGRAM"] == program;
        },
        vertexAttrib1f: function (indx, x) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [x, 0, 0, 1]);
        },
        vertexAttrib2f: function (indx, x, y) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [x, y, 0, 1]);
        },
        vertexAttrib3f: function (indx, x, y, z) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [x, y, z, 1]);
        },
        vertexAttrib4f: function (indx, x, y, z, w) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [x, y, z, w]);
        },
        vertexAttrib1fv: function (indx, v) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [v[0], 0, 0, 1]);
        },
        vertexAttrib2fv: function (indx, v) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [v[0], v[1], 0, 1]);
        },
        vertexAttrib3fv: function (indx, v) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], [v[0], v[1], v[2], 1]);
        },
        vertexAttrib4fv: function (indx, v) {
            return util.arrayCompare(this.stateCache["CURRENT_VERTEX_ATTRIB_" + indx], v);
        },
        vertexAttribPointer: function (indx, size, type, normalized, stride, offset) {
            return (this.stateCache["VERTEX_ATTRIB_ARRAY_SIZE_" + indx] == size) &&
                   (this.stateCache["VERTEX_ATTRIB_ARRAY_TYPE_" + indx] == type) &&
                   (this.stateCache["VERTEX_ATTRIB_ARRAY_NORMALIZED_" + indx] == normalized) &&
                   (this.stateCache["VERTEX_ATTRIB_ARRAY_STRIDE_" + indx] == stride) &&
                   (this.stateCache["VERTEX_ATTRIB_ARRAY_POINTER_" + indx] == offset) &&
                   (this.stateCache["VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_" + indx] == this.stateCache["ARRAY_BUFFER_BINDING"]);
        },
        viewport: function (x, y, width, height) {
            return util.arrayCompare(this.stateCache["VIEWPORT"], [x, y, width, height]);
        }
    };

    RedundancyChecker.prototype.initializeStateCache = function (gl) {
        var stateCache = {};

        var stateParameters = ["ACTIVE_TEXTURE", "ARRAY_BUFFER_BINDING", "BLEND", "BLEND_COLOR", "BLEND_DST_ALPHA", "BLEND_DST_RGB", "BLEND_EQUATION_ALPHA", "BLEND_EQUATION_RGB", "BLEND_SRC_ALPHA", "BLEND_SRC_RGB", "COLOR_CLEAR_VALUE", "COLOR_WRITEMASK", "CULL_FACE", "CULL_FACE_MODE", "CURRENT_PROGRAM", "DEPTH_FUNC", "DEPTH_RANGE", "DEPTH_WRITEMASK", "ELEMENT_ARRAY_BUFFER_BINDING", "FRAMEBUFFER_BINDING", "FRONT_FACE", "GENERATE_MIPMAP_HINT", "LINE_WIDTH", "PACK_ALIGNMENT", "POLYGON_OFFSET_FACTOR", "POLYGON_OFFSET_FILL", "POLYGON_OFFSET_UNITS", "RENDERBUFFER_BINDING", "POLYGON_OFFSET_FACTOR", "POLYGON_OFFSET_FILL", "POLYGON_OFFSET_UNITS", "SAMPLE_COVERAGE_INVERT", "SAMPLE_COVERAGE_VALUE", "SCISSOR_BOX", "SCISSOR_TEST", "STENCIL_BACK_FAIL", "STENCIL_BACK_FUNC", "STENCIL_BACK_PASS_DEPTH_FAIL", "STENCIL_BACK_PASS_DEPTH_PASS", "STENCIL_BACK_REF", "STENCIL_BACK_VALUE_MASK", "STENCIL_BACK_WRITEMASK", "STENCIL_CLEAR_VALUE", "STENCIL_FAIL", "STENCIL_FUNC", "STENCIL_PASS_DEPTH_FAIL", "STENCIL_PASS_DEPTH_PASS", "STENCIL_REF", "STENCIL_TEST", "STENCIL_VALUE_MASK", "STENCIL_WRITEMASK", "UNPACK_ALIGNMENT", "UNPACK_COLORSPACE_CONVERSION_WEBGL", "UNPACK_FLIP_Y_WEBGL", "UNPACK_PREMULTIPLY_ALPHA_WEBGL", "VIEWPORT"];
        for (var n = 0; n < stateParameters.length; n++) {
            try {
                stateCache[stateParameters[n]] = gl.getParameter(gl[stateParameters[n]]);
            } catch (e) {
                // Ignored
            }
        }
        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);
        var originalActiveTexture = gl.getParameter(gl.ACTIVE_TEXTURE);
        for (var n = 0; n < maxTextureUnits; n++) {
            gl.activeTexture(gl.TEXTURE0 + n);
            stateCache["TEXTURE_BINDING_2D_" + n] = gl.getParameter(gl.TEXTURE_BINDING_2D);
            stateCache["TEXTURE_BINDING_CUBE_MAP_" + n] = gl.getParameter(gl.TEXTURE_BINDING_CUBE_MAP);
        }
        gl.activeTexture(originalActiveTexture);
        var maxVertexAttribs = gl.getParameter(gl.MAX_VERTEX_ATTRIBS);
        for (var n = 0; n < maxVertexAttribs; n++) {
            stateCache["VERTEX_ATTRIB_ARRAY_ENABLED_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_ENABLED);
            stateCache["VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);
            stateCache["VERTEX_ATTRIB_ARRAY_SIZE_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_SIZE);
            stateCache["VERTEX_ATTRIB_ARRAY_STRIDE_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_STRIDE);
            stateCache["VERTEX_ATTRIB_ARRAY_TYPE_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_TYPE);
            stateCache["VERTEX_ATTRIB_ARRAY_NORMALIZED_" + n] = gl.getVertexAttrib(n, gl.VERTEX_ATTRIB_ARRAY_NORMALIZED);
            stateCache["VERTEX_ATTRIB_ARRAY_POINTER_" + n] = gl.getVertexAttribOffset(n, gl.VERTEX_ATTRIB_ARRAY_POINTER);
            stateCache["CURRENT_VERTEX_ATTRIB_" + n] = gl.getVertexAttrib(n, gl.CURRENT_VERTEX_ATTRIB);
        }

        return stateCache;
    };

    RedundancyChecker.prototype.cacheUniformValues = function (gl, frame) {
        var originalProgram = gl.getParameter(gl.CURRENT_PROGRAM);

        for (var n = 0; n < frame.uniformValues.length; n++) {
            var program = frame.uniformValues[n].program;
            var values = frame.uniformValues[n].values;

            var target = program.mirror.target;
            if (!target) {
                continue;
            }

            program.uniformCache = {};

            gl.useProgram(target);

            for (var name in values) {
                var data = values[name];
                var loc = gl.getUniformLocation(target, name);

                switch (data.type) {
                    case gl.FLOAT:
                    case gl.FLOAT_VEC2:
                    case gl.FLOAT_VEC3:
                    case gl.FLOAT_VEC4:
                    case gl.INT:
                    case gl.INT_VEC2:
                    case gl.INT_VEC3:
                    case gl.INT_VEC4:
                    case gl.BOOL:
                    case gl.BOOL_VEC2:
                    case gl.BOOL_VEC3:
                    case gl.BOOL_VEC4:
                    case gl.SAMPLER_2D:
                    case gl.SAMPLER_CUBE:
                        if (data.value && data.value.length !== undefined) {
                            program.uniformCache[name] = data.value;
                        } else {
                            program.uniformCache[name] = [data.value];
                        }
                        break;
                    case gl.FLOAT_MAT2:
                    case gl.FLOAT_MAT3:
                    case gl.FLOAT_MAT4:
                        program.uniformCache[name] = data.value;
                        break;
                }
            }
        }

        gl.useProgram(originalProgram);
    };

    RedundancyChecker.prototype.run = function (frame) {
        // TODO: if we every support editing, we may want to recheck
        if (frame.hasCheckedRedundancy) {
            return;
        }
        frame.hasCheckedRedundancy = true;

        var gl = this.gl;

        frame.switchMirrors("redundancy");
        frame.makeActive(gl, false, {
            ignoreBufferUploads: true,
            ignoreTextureUploads: true
        });

        // Setup initial state cache (important to do here so we have the frame initial state)
        gl.stateCache = this.initializeStateCache(gl);

        // Cache all uniform values for checking
        this.cacheUniformValues(gl, frame);

        var redundantCalls = 0;
        var calls = frame.calls;
        for (var n = 0; n < calls.length; n++) {
            var call = calls[n];
            if (call.type !== 1) {
                continue;
            }

            var redundantCheck = redundantChecks[call.name];
            var stateCacheModifier = stateCacheModifiers[call.name];
            if (!redundantCheck && !stateCacheModifier) {
                continue;
            }

            var args = call.transformArgs(gl);

            if (redundantCheck && redundantCheck.apply(gl, args)) {
                redundantCalls++;
                call.isRedundant = true;
            }

            if (stateCacheModifier) {
                stateCacheModifier.apply(gl, args);
            }
        }

        frame.redundantCalls = redundantCalls;

        frame.cleanup(gl);
        frame.switchMirrors();
    };

    var cachedChecker = null;
    RedundancyChecker.checkFrame = function (frame) {
        if (!cachedChecker) {
            cachedChecker = new RedundancyChecker();
        }

        cachedChecker.run(frame);
    };

    return RedundancyChecker;

});
