define([
        '../shared/Utilities',
    ], function (
        util
    ) {

    var stateParameters = null;
    function setupStateParameters(gl) {
        stateParameters = [
            { name: "ACTIVE_TEXTURE" },
            { name: "ALIASED_LINE_WIDTH_RANGE" },
            { name: "ALIASED_POINT_SIZE_RANGE" },
            { name: "ALPHA_BITS" },
            { name: "ARRAY_BUFFER_BINDING" },
            { name: "BLEND" },
            { name: "BLEND_COLOR" },
            { name: "BLEND_DST_ALPHA" },
            { name: "BLEND_DST_RGB" },
            { name: "BLEND_EQUATION_ALPHA" },
            { name: "BLEND_EQUATION_RGB" },
            { name: "BLEND_SRC_ALPHA" },
            { name: "BLEND_SRC_RGB" },
            { name: "BLUE_BITS" },
            { name: "COLOR_CLEAR_VALUE" },
            { name: "COLOR_WRITEMASK" },
            { name: "CULL_FACE" },
            { name: "CULL_FACE_MODE" },
            { name: "CURRENT_PROGRAM" },
            { name: "DEPTH_BITS" },
            { name: "DEPTH_CLEAR_VALUE" },
            { name: "DEPTH_FUNC" },
            { name: "DEPTH_RANGE" },
            { name: "DEPTH_TEST" },
            { name: "DEPTH_WRITEMASK" },
            { name: "DITHER" },
            { name: "ELEMENT_ARRAY_BUFFER_BINDING" },
            { name: "FRAMEBUFFER_BINDING" },
            { name: "FRONT_FACE" },
            { name: "GENERATE_MIPMAP_HINT" },
            { name: "GREEN_BITS" },
            { name: "LINE_WIDTH" },
            { name: "MAX_COMBINED_TEXTURE_IMAGE_UNITS" },
            { name: "MAX_CUBE_MAP_TEXTURE_SIZE" },
            { name: "MAX_FRAGMENT_UNIFORM_VECTORS" },
            { name: "MAX_RENDERBUFFER_SIZE" },
            { name: "MAX_TEXTURE_IMAGE_UNITS" },
            { name: "MAX_TEXTURE_SIZE" },
            { name: "MAX_VARYING_VECTORS" },
            { name: "MAX_VERTEX_ATTRIBS" },
            { name: "MAX_VERTEX_TEXTURE_IMAGE_UNITS" },
            { name: "MAX_VERTEX_UNIFORM_VECTORS" },
            { name: "MAX_VIEWPORT_DIMS" },
            { name: "PACK_ALIGNMENT" },
            { name: "POLYGON_OFFSET_FACTOR" },
            { name: "POLYGON_OFFSET_FILL" },
            { name: "POLYGON_OFFSET_UNITS" },
            { name: "RED_BITS" },
            { name: "RENDERBUFFER_BINDING" },
            { name: "RENDERER" },
            { name: "SAMPLE_BUFFERS" },
            { name: "SAMPLE_COVERAGE_INVERT" },
            { name: "SAMPLE_COVERAGE_VALUE" },
            { name: "SAMPLES" },
            { name: "SCISSOR_BOX" },
            { name: "SCISSOR_TEST" },
            { name: "SHADING_LANGUAGE_VERSION" },
            { name: "STENCIL_BACK_FAIL" },
            { name: "STENCIL_BACK_FUNC" },
            { name: "STENCIL_BACK_PASS_DEPTH_FAIL" },
            { name: "STENCIL_BACK_PASS_DEPTH_PASS" },
            { name: "STENCIL_BACK_REF" },
            { name: "STENCIL_BACK_VALUE_MASK" },
            { name: "STENCIL_BACK_WRITEMASK" },
            { name: "STENCIL_BITS" },
            { name: "STENCIL_CLEAR_VALUE" },
            { name: "STENCIL_FAIL" },
            { name: "STENCIL_FUNC" },
            { name: "STENCIL_PASS_DEPTH_FAIL" },
            { name: "STENCIL_PASS_DEPTH_PASS" },
            { name: "STENCIL_REF" },
            { name: "STENCIL_TEST" },
            { name: "STENCIL_VALUE_MASK" },
            { name: "STENCIL_WRITEMASK" },
            { name: "SUBPIXEL_BITS" },
            { name: "UNPACK_ALIGNMENT" },
            { name: "UNPACK_COLORSPACE_CONVERSION_WEBGL" },
            { name: "UNPACK_FLIP_Y_WEBGL" },
            { name: "UNPACK_PREMULTIPLY_ALPHA_WEBGL" },
            { name: "VENDOR" },
            { name: "VERSION" },
            { name: "VIEWPORT" }
        ];

        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);
        for (var n = 0; n < maxTextureUnits; n++) {
            var param = { name: "TEXTURE_BINDING_2D_" + n };
            param.getter = (function (n) {
                return function (gl) {
                    var existingBinding = gl.getParameter(gl.ACTIVE_TEXTURE);
                    gl.activeTexture(gl.TEXTURE0 + n);
                    var result = gl.getParameter(gl.TEXTURE_BINDING_2D);
                    gl.activeTexture(existingBinding);
                    return result;
                };
            })(n);
            stateParameters.push(param);
        }
        for (var n = 0; n < maxTextureUnits; n++) {
            var param = { name: "TEXTURE_BINDING_CUBE_MAP_" + n };
            param.getter = (function (n) {
                return function (gl) {
                    var existingBinding = gl.getParameter(gl.ACTIVE_TEXTURE);
                    gl.activeTexture(gl.TEXTURE0 + n);
                    var result = gl.getParameter(gl.TEXTURE_BINDING_CUBE_MAP);
                    gl.activeTexture(existingBinding);
                    return result;
                };
            })(n);
            stateParameters.push(param);
        }

        // Setup values
        for (var n = 0; n < stateParameters.length; n++) {
            var param = stateParameters[n];
            param.value = gl[param.name];
        }
    };

    function defaultGetParameter(gl, name) {
        try {
            return gl.getParameter(gl[name]);
        } catch (e) {
            console.log("unable to get state parameter " + name);
            return null;
        }
    };

    var StateSnapshot = function (gl) {
        if (stateParameters == null) {
            setupStateParameters(gl);
        }

        for (var n = 0; n < stateParameters.length; n++) {
            var param = stateParameters[n];
            var value = param.getter ? param.getter(gl) : defaultGetParameter(gl, param.name);
            this[param.value ? param.value : param.name] = value;
        }

        this.attribs = [];
        var attribEnums = [gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, gl.VERTEX_ATTRIB_ARRAY_ENABLED, gl.VERTEX_ATTRIB_ARRAY_SIZE, gl.VERTEX_ATTRIB_ARRAY_STRIDE, gl.VERTEX_ATTRIB_ARRAY_TYPE, gl.VERTEX_ATTRIB_ARRAY_NORMALIZED, gl.CURRENT_VERTEX_ATTRIB];
        var maxVertexAttribs = gl.getParameter(gl.MAX_VERTEX_ATTRIBS);
        for (var n = 0; n < maxVertexAttribs; n++) {
            var values = {};
            for (var m = 0; m < attribEnums.length; m++) {
                values[attribEnums[m]] = gl.getVertexAttrib(n, attribEnums[m]);
                // TODO: replace buffer binding with ref
            }
            values[0] = gl.getVertexAttribOffset(n, gl.VERTEX_ATTRIB_ARRAY_POINTER);
            this.attribs.push(values);
        }
    };

    StateSnapshot.prototype.clone = function () {
        var cloned = {};
        for (var k in this) {
            cloned[k] = util.clone(this[k]);
        }
        return cloned;
    };

    function getTargetValue(value) {
        if (value) {
            if (value.trackedObject) {
                return value.trackedObject.mirror.target;
            } else {
                return value;
            }
        } else {
            return null;
        }
    };

    StateSnapshot.prototype.apply = function (gl) {
        gl.bindFramebuffer(gl.FRAMEBUFFER, getTargetValue(this[gl.FRAMEBUFFER_BINDING]));
        gl.bindRenderbuffer(gl.RENDERBUFFER, getTargetValue(this[gl.RENDERBUFFER_BINDING]));

        gl.viewport(this[gl.VIEWPORT][0], this[gl.VIEWPORT][1], this[gl.VIEWPORT][2], this[gl.VIEWPORT][3]);

        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);
        for (var n = 0; n < maxTextureUnits; n++) {
            gl.activeTexture(gl.TEXTURE0 + n);
            if (this["TEXTURE_BINDING_2D_" + n]) {
                gl.bindTexture(gl.TEXTURE_2D, getTargetValue(this["TEXTURE_BINDING_2D_" + n]));
            }
            if (this["TEXTURE_BINDING_CUBE_MAP_" + n]) {
                gl.bindTexture(gl.TEXTURE_CUBE_MAP, getTargetValue(this["TEXTURE_BINDING_CUBE_MAP_" + n]));
            }
            if (this["TEXTURE_BINDING_3D_" + n]) {
                gl.bindTexture(gl.TEXTURE_3D, getTargetValue(this["TEXTURE_BINDING_3D_" + n]));
            }
            if (this["TEXTURE_BINDING_2D_ARRAY_" + n]) {
                gl.bindTexture(gl.TEXTURE_2D_ARRAY, getTargetValue(this["TEXTURE_BINDING_2D_ARRAY_" + n]));
            }
        }

        gl.clearColor(this[gl.COLOR_CLEAR_VALUE][0], this[gl.COLOR_CLEAR_VALUE][1], this[gl.COLOR_CLEAR_VALUE][2], this[gl.COLOR_CLEAR_VALUE][3]);
        gl.colorMask(this[gl.COLOR_WRITEMASK][0], this[gl.COLOR_WRITEMASK][1], this[gl.COLOR_WRITEMASK][2], this[gl.COLOR_WRITEMASK][3]);

        if (this[gl.DEPTH_TEST]) {
            gl.enable(gl.DEPTH_TEST);
        } else {
            gl.disable(gl.DEPTH_TEST);
        }
        gl.clearDepth(this[gl.DEPTH_CLEAR_VALUE]);
        gl.depthFunc(this[gl.DEPTH_FUNC]);
        gl.depthRange(this[gl.DEPTH_RANGE][0], this[gl.DEPTH_RANGE][1]);
        gl.depthMask(this[gl.DEPTH_WRITEMASK]);

        if (this[gl.BLEND]) {
            gl.enable(gl.BLEND);
        } else {
            gl.disable(gl.BLEND);
        }
        gl.blendColor(this[gl.BLEND_COLOR][0], this[gl.BLEND_COLOR][1], this[gl.BLEND_COLOR][2], this[gl.BLEND_COLOR][3]);
        gl.blendEquationSeparate(this[gl.BLEND_EQUATION_RGB], this[gl.BLEND_EQUATION_ALPHA]);
        gl.blendFuncSeparate(this[gl.BLEND_SRC_RGB], this[gl.BLEND_DST_RGB], this[gl.BLEND_SRC_ALPHA], this[gl.BLEND_DST_ALPHA]);

        //gl.DITHER, // ??????????????????????????????????????????????????????????

        if (this[gl.CULL_FACE]) {
            gl.enable(gl.CULL_FACE);
        } else {
            gl.disable(gl.CULL_FACE);
        }
        gl.cullFace(this[gl.CULL_FACE_MODE]);
        gl.frontFace(this[gl.FRONT_FACE]);

        gl.lineWidth(this[gl.LINE_WIDTH]);

        if (this[gl.POLYGON_OFFSET_FILL]) {
            gl.enable(gl.POLYGON_OFFSET_FILL);
        } else {
            gl.disable(gl.POLYGON_OFFSET_FILL);
        }
        gl.polygonOffset(this[gl.POLYGON_OFFSET_FACTOR], this[gl.POLYGON_OFFSET_UNITS]);

        if (this[gl.SAMPLE_COVERAGE]) {
            gl.enable(gl.SAMPLE_COVERAGE);
        } else {
            gl.disable(gl.SAMPLE_COVERAGE);
        }
        if (this[gl.SAMPLE_ALPHA_TO_COVERAGE]) {
            gl.enable(gl.SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            gl.disable(gl.SAMPLE_ALPHA_TO_COVERAGE);
        }
        gl.sampleCoverage(this[gl.SAMPLE_COVERAGE_VALUE], this[gl.SAMPLE_COVERAGE_INVERT]);

        if (this[gl.SCISSOR_TEST]) {
            gl.enable(gl.SCISSOR_TEST);
        } else {
            gl.disable(gl.SCISSOR_TEST);
        }
        gl.scissor(this[gl.SCISSOR_BOX][0], this[gl.SCISSOR_BOX][1], this[gl.SCISSOR_BOX][2], this[gl.SCISSOR_BOX][3]);

        if (this[gl.STENCIL_TEST]) {
            gl.enable(gl.STENCIL_TEST);
        } else {
            gl.disable(gl.STENCIL_TEST);
        }
        gl.clearStencil(this[gl.STENCIL_CLEAR_VALUE]);
        gl.stencilFuncSeparate(gl.FRONT, this[gl.STENCIL_FUNC], this[gl.STENCIL_REF], this[gl.STENCIL_VALUE_MASK]);
        gl.stencilFuncSeparate(gl.BACK, this[gl.STENCIL_BACK_FUNC], this[gl.STENCIL_BACK_REF], this[gl.STENCIL_BACK_VALUE_MASK]);
        gl.stencilOpSeparate(gl.FRONT, this[gl.STENCIL_FAIL], this[gl.STENCIL_PASS_DEPTH_FAIL], this[gl.STENCIL_PASS_DEPTH_PASS]);
        gl.stencilOpSeparate(gl.BACK, this[gl.STENCIL_BACK_FAIL], this[gl.STENCIL_BACK_PASS_DEPTH_FAIL], this[gl.STENCIL_BACK_PASS_DEPTH_PASS]);
        gl.stencilMaskSeparate(gl.FRONT, this[gl.STENCIL_WRITEMASK]);
        gl.stencilMaskSeparate(gl.BACK, this[gl.STENCIL_BACK_WRITEMASK]);

        gl.hint(gl.GENERATE_MIPMAP_HINT, this[gl.GENERATE_MIPMAP_HINT]);

        gl.pixelStorei(gl.PACK_ALIGNMENT, this[gl.PACK_ALIGNMENT]);
        gl.pixelStorei(gl.UNPACK_ALIGNMENT, this[gl.UNPACK_ALIGNMENT]);
        //gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, this[gl.UNPACK_COLORSPACE_CONVERSION_WEBGL]); ////////////////////// NOT YET SUPPORTED IN SOME BROWSERS
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, this[gl.UNPACK_FLIP_Y_WEBGL]);
        gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, this[gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL]);

        var program = getTargetValue(this[gl.CURRENT_PROGRAM]);
        // HACK: if not linked, try linking
        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            gl.linkProgram(program);
        }
        gl.useProgram(program);

        for (var n = 0; n < this.attribs.length; n++) {
            var values = this.attribs[n];
            if (values[gl.VERTEX_ATTRIB_ARRAY_ENABLED]) {
                gl.enableVertexAttribArray(n);
            } else {
                gl.disableVertexAttribArray(n);
            }
            if (values[gl.CURRENT_VERTEX_ATTRIB]) {
                gl.vertexAttrib4fv(n, values[gl.CURRENT_VERTEX_ATTRIB]);
            }
            var buffer = getTargetValue(values[gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING]);
            if (buffer) {
                gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
                gl.vertexAttribPointer(n, values[gl.VERTEX_ATTRIB_ARRAY_SIZE], values[gl.VERTEX_ATTRIB_ARRAY_TYPE], values[gl.VERTEX_ATTRIB_ARRAY_NORMALIZED], values[gl.VERTEX_ATTRIB_ARRAY_STRIDE], values[0]);
            }
        }

        gl.bindBuffer(gl.ARRAY_BUFFER, getTargetValue(this[gl.ARRAY_BUFFER_BINDING]));
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, getTargetValue(this[gl.ELEMENT_ARRAY_BUFFER_BINDING]));

        gl.activeTexture(this[gl.ACTIVE_TEXTURE]);
    };

    return StateSnapshot;
});
