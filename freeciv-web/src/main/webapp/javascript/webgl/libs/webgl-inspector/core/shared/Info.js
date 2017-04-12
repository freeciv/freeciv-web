define([
        './GLConsts',
        './Utilities',
    ], function (
        glc,
        util) {

    const info = {};

    var UIType = {
        ENUM: 0, // a specific enum
        ARRAY: 1, // array of values (tightly packed)
        BOOL: 2,
        LONG: 3,
        ULONG: 4,
        COLORMASK: 5, // 4 bools
        OBJECT: 6, // some WebGL object (texture/program/etc)
        WH: 7, // width x height (array with 2 values)
        RECT: 8, // x, y, w, h (array with 4 values)
        STRING: 9, // some arbitrary string
        COLOR: 10, // 4 floats
        FLOAT: 11,
        BITMASK: 12, // 32bit boolean mask
        RANGE: 13, // 2 floats
        MATRIX: 14 // 2x2, 3x3, or 4x4 matrix
    };

    var UIInfo = function (type, values) {
        this.type = type;
        this.values = values;
    };

    var FunctionType = {
        GENERIC: 0,
        DRAW: 1
    };

    var FunctionInfo = function (staticgl, name, returnType, args, type) {
        this.name = name;
        this.returnType = returnType;
        this.args = args;
        this.type = type;
    };
    FunctionInfo.prototype.getArgs = function (call) {
        return this.args;
    };

    var FunctionParam = function (staticgl, name, ui) {
        this.name = name;
        this.ui = ui;
    };

    const textureTypes = new UIInfo(UIType.ENUM, [
        "BYTE",
        "FLOAT",
        "FLOAT_32_UNSIGNED_INT_24_8_REV",
        "HALF_FLOAT",
        "INT",
        "SHORT",
        "UNSIGNED_BYTE",
        "UNSIGNED_INT",
        "UNSIGNED_INT_10F_11F_11F_REV",
        "UNSIGNED_INT_24_8",
        "UNSIGNED_INT_2_10_10_10_REV",
        "UNSIGNED_INT_5_9_9_9_REV",
        "UNSIGNED_SHORT",
        "UNSIGNED_INT",
        "UNSIGNED_SHORT_4_4_4_4",
        "UNSIGNED_SHORT_5_5_5_1",
        "UNSIGNED_SHORT_5_6_5",
    ]);

    const unsizedColorTextureFormats = [
        "RGB",
        "RGBA",
        "LUMINANCE_ALPHA",
        "LUMINANCE",
        "ALPHA",
    ];

    const sizedColorTextureFormats = [
        "RED",
        "RED_INTEGER",
        "RG",
        "RG_INTEGER",
        "RGB",
        "RGB_INTEGER",
        "RGBA",
        "RGBA_INTEGER",
    ];


    const sizedDepthTextureFormats = [
        "DEPTH_COMPONENT",
        "DEPTH_STENCIL",
    ];

    const textureFormats = [
        ...unsizedColorTextureFormats,
        ...sizedColorTextureFormats,
        ...sizedDepthTextureFormats,
    ];

    const drawModes = [
        "POINTS",
        "LINE_STRIP",
        "LINE_LOOP",
        "LINES",
        "TRIANGLES",
        "TRIANGLE_STRIP",
        "TRIANGLE_FAN",
    ];
    const elementTypes = [
        "UNSIGNED_BYTE",
        "UNSIGNED_SHORT",
        "UNSIGNED_INT",
    ];
    const texParamNames = [
        "TEXTURE_BASE_LEVEL",
        "TEXTURE_COMPARE_FUNC",
        "TEXTURE_COMPARE_MODE",
        "TEXTURE_MIN_FILTER",
        "TEXTURE_MAG_FILTER",
        "TEXTURE_MIN_LOD",
        "TEXTURE_MAX_LOD",
        "TEXTURE_MAX_LEVEL",
        "TEXTURE_WRAP_S",
        "TEXTURE_WRAP_T",
        "TEXTURE_WRAP_R",
        "TEXTURE_MAX_ANISOTROPY_EXT",
    ];

    const samplerParamNames = [
        "TEXTURE_WRAP_S",
        "TEXTURE_WRAP_T",
        "TEXTURE_WRAP_R",
        "TEXTURE_MIN_FILTER",
        "TEXTURE_MAG_FILTER",
        "TEXTURE_MIN_LOD",
        "TEXTURE_MAX_LOD",
        "TEXTURE_COMPARE_MODE",
        "TEXTURE_COMPARE_FUNC",
    ];
    const bufferTargets = [
        "ARRAY_BUFFER",
        "COPY_READ_BUFFER",
        "COPY_WRITE_BUFFER",
        "ELEMENT_ARRAY_BUFFER",
        "PIXEL_PACK_BUFFER",
        "PIXEL_UNPACK_BUFFER",
        "TRANSFORM_FEEDBACK_BUFFER",
        "UNIFORM_BUFFER",
    ];
    const framebufferTargets = [
        "DRAW_FRAMEBUFFER",
        "READ_FRAMEBUFFER",
        "FRAMEBUFFER",
    ];
    const texture2DTargets = [
        "TEXTURE_2D",
        "TEXTURE_CUBE_MAP",
    ];
    const texture3DTargets = [
        "TEXTURE_3D",
        "TEXTURE_2D_ARRAY",
    ];
    const bindTextureTargets = [
        ...texture2DTargets,
        ...texture3DTargets,
    ];
    const faceTextureTargets = [
        "TEXTURE_2D",
        "TEXTURE_CUBE_MAP_POSITIVE_X",
        "TEXTURE_CUBE_MAP_NEGATIVE_X",
        "TEXTURE_CUBE_MAP_POSITIVE_Y",
        "TEXTURE_CUBE_MAP_NEGATIVE_Y",
        "TEXTURE_CUBE_MAP_POSITIVE_Z",
        "TEXTURE_CUBE_MAP_NEGATIVE_Z",
    ];
    const colorAttachments = [
        "COLOR_ATTACHMENT0",
        "COLOR_ATTACHMENT1",
        "COLOR_ATTACHMENT2",
        "COLOR_ATTACHMENT3",
        "COLOR_ATTACHMENT4",
        "COLOR_ATTACHMENT5",
        "COLOR_ATTACHMENT6",
        "COLOR_ATTACHMENT7",
        "COLOR_ATTACHMENT8",
        "COLOR_ATTACHMENT9",
        "COLOR_ATTACHMENT10",
        "COLOR_ATTACHMENT11",
        "COLOR_ATTACHMENT12",
        "COLOR_ATTACHMENT13",
        "COLOR_ATTACHMENT14",
        "COLOR_ATTACHMENT15",
    ];
    const attachments = [
        ...colorAttachments,
        "DEPTH_ATTACHMENT",
        "STENCIL_ATTACHMENT",
        "DEPTH_STENCIL_ATTACHMENT",
    ];
    const colorRenderableFormats = [
        "R8",
        "R8UI",
        "R8I",
        "R16UI",
        "R16I",
        "R32UI",
        "R32I",
        "RG8",
        "RG8UI",
        "RG8I",
        "RG16UI",
        "RG16I",
        "RG32UI",
        "RG32I",
        "RGB8",
        "RGB565",
        "RGBA8",
        "SRGB8_ALPHA8",
        "RGB5_A1",
        "RGBA4",
        "RGB10_A2",
        "RGBA8UI",
        "RGBA8I",
        "RGB10_A2UI",
        "RGBA16UI",
        "RGBA16I",
        "RGBA32I",
        "RGBA32UI",
    ];
    const depthRenderableFormats = [
        "DEPTH_COMPONENT16",
        "DEPTH_COMPONENT24",
        "DEPTH_COMPONENT32F",
    ];
    const stencilRenderableFormats = [
        "DEPTH24_STENCIL8",
        "DEPTH32F_STENCIL8",
        "STENCIL_INDEX8",
    ];
    const renderableFormats = [
        ...colorRenderableFormats,
        ...depthRenderableFormats,
        ...stencilRenderableFormats,
    ];

    const unsizedColorTextureInternalFormats = [
        "RGB",
        "RGBA",
        "LUMINANCE_ALPHA",
        "LUMINANCE",
        "ALPHA",
    ];

    const sizedColorTextureInternalFormats = [
        "R8",
        "R8_SNORM",
        "R16F",
        "R32F",
        "R8UI",
        "R8I",
        "R16UI",
        "R16I",
        "R32UI",
        "R32I",
        "RG8",
        "RG8_SNORM",
        "RG16F",
        "RG32F",
        "RG8UI",
        "RG8I",
        "RG16UI",
        "RG16I",
        "RG32UI",
        "RG32I",
        "RGB8",
        "SRGB8",
        "RGB565",
        "RGB8_SNORM",
        "R11F_G11F_B10F",
        "RGB9_E5",
        "RGB16F",
        "RGB32F",
        "RGB8UI",
        "RGB8I",
        "RGB16UI",
        "RGB16I",
        "RGB32UI",
        "RGB32I",
        "RGBA8",
        "SRGB8_ALPHA8",
        "RGBA8_SNORM",
        "RGB5_A1",
        "RGBA4",
        "RGB10_A2",
        "RGBA16F",
        "RGBA32F",
        "RGBA8UI",
        "RGBA8I",
        "RGB10_A2UI",
        "RGBA16UI",
        "RGBA16I",
        "RGBA32I",
        "RGBA32UI",
    ];
    const depthTextureInternalFormats = [
        "DEPTH_COMPONENT16",
        "DEPTH_COMPONENT24",
        "DEPTH_COMPONENT32F",
        "DEPTH24_STENCIL8",
        "DEPTH32F_STENCIL8",
    ];
    const compressedTextureInternalFormats = [
        "COMPRESSED_R11_EAC",
        "COMPRESSED_SIGNED_R11_EAC",
        "COMPRESSED_RG11_EAC",
        "COMPRESSED_SIGNED_RG11_EAC",
        "COMPRESSED_RGB8_ETC2",
        "COMPRESSED_SRGB8_ETC2",
        "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2",
        "COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2",
        "COMPRESSED_RGBA8_ETC2_EAC",
        "COMPRESSED_SRGB8_ALPHA8_ETC2_EAC",
    ];
    const sizedTextureInternalFormats = [
        ...sizedColorTextureInternalFormats,
        ...depthTextureInternalFormats,
        ...compressedTextureInternalFormats,
    ];
    const allUncompressedTextureInternalFormats = [
        ...unsizedColorTextureInternalFormats,
        ...sizedColorTextureInternalFormats,
        ...depthTextureInternalFormats,
    ];
    const allTextureInternalFormats = [
        ...unsizedColorTextureInternalFormats,
        ...sizedColorTextureInternalFormats,
        ...depthTextureInternalFormats,
    ];

    const queryTargets = [
        "ANY_SAMPLES_PASSED",
        "ANY_SAMPLES_PASSED_CONSERVATIVE",
        "TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN",
    ];

    const readBufferEnums = [
        "BACK",
        "NONE",
        ...colorAttachments,
    ];

    const textureUnits = [
        "TEXTURE0",
        "TEXTURE1",
        "TEXTURE2",
        "TEXTURE3",
        "TEXTURE4",
        "TEXTURE5",
        "TEXTURE6",
        "TEXTURE7",
        "TEXTURE8",
        "TEXTURE9",
        "TEXTURE10",
        "TEXTURE11",
        "TEXTURE12",
        "TEXTURE13",
        "TEXTURE14",
        "TEXTURE15",
        "TEXTURE16",
        "TEXTURE17",
        "TEXTURE18",
        "TEXTURE19",
        "TEXTURE20",
        "TEXTURE21",
        "TEXTURE22",
        "TEXTURE23",
        "TEXTURE24",
        "TEXTURE25",
        "TEXTURE26",
        "TEXTURE27",
        "TEXTURE28",
        "TEXTURE29",
        "TEXTURE30",
        "TEXTURE31",
    ];

    const blendEquations = [
        "FUNC_ADD",
        "FUNC_SUBTRACT",
        "FUNC_REVERSE_SUBTRACT",
        "MIN",
        "MAX",
    ];

    const capabilities = [
        "BLEND",
        "CULL_FACE",
        "DEPTH_TEST",
        "DITHER",
        "POLYGON_OFFSET_FILL",
        "PRIMITIVE_RESTART_FIXED_INDEX",
        "RASTERIZER_DISCARD",
        "SAMPLE_ALPHA_TO_COVERAGE",
        "SAMPLE_COVERAGE",
        "SCISSOR_TEST",
        "STENCIL_TEST",
    ];

    function setupFunctionInfos(gl) {
        if (info.functions) {
            return;
        }

        var functionInfos = [
            new FunctionInfo(gl, "activeTexture", null, [
                new FunctionParam(gl, "texture", new UIInfo(UIType.ENUM, textureUnits))
            ]),
            new FunctionInfo(gl, "attachShader", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "bindAttribLocation", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "name", new UIInfo(UIType.STRING))
            ]),
            new FunctionInfo(gl, "bindBuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bufferTargets)),
                new FunctionParam(gl, "buffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "bindFramebuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "framebuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "bindRenderbuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "renderbuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "bindTexture", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bindTextureTargets)),
                new FunctionParam(gl, "texture", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "blendColor", null, new UIInfo(UIType.COLOR)),
            new FunctionInfo(gl, "blendEquation", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, blendEquations)),
            ]),
            new FunctionInfo(gl, "blendEquationSeparate", null, [
                new FunctionParam(gl, "modeRGB", new UIInfo(UIType.ENUM, blendEquations)),
                new FunctionParam(gl, "modeAlpha", new UIInfo(UIType.ENUM, blendEquations)),
            ]),
            new FunctionInfo(gl, "blendFunc", null, [
                new FunctionParam(gl, "sfactor", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE"])),
                new FunctionParam(gl, "dfactor", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA. GL_CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA"]))
            ]),
            new FunctionInfo(gl, "blendFuncSeparate", null, [
                new FunctionParam(gl, "srcRGB", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE"])),
                new FunctionParam(gl, "dstRGB", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA. GL_CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA"])),
                new FunctionParam(gl, "srcAlpha", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE"])),
                new FunctionParam(gl, "dstAlpha", new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA. GL_CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA"]))
            ]),
            new FunctionInfo(gl, "bufferData", null, null), // handled specially below
            new FunctionInfo(gl, "bufferSubData", null, null), // handled specially below
            new FunctionInfo(gl, "checkFramebufferStatus", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets))
            ]),
            new FunctionInfo(gl, "clear", null, [
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK, ["COLOR_BUFFER_BIT", "DEPTH_BUFFER_BIT", "STENCIL_BUFFER_BIT"]))
            ]),
            new FunctionInfo(gl, "clearColor", null, new UIInfo(UIType.COLOR)),
            new FunctionInfo(gl, "clearDepth", null, [
                new FunctionParam(gl, "depth", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "clearStencil", null, [
                new FunctionParam(gl, "s", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "colorMask", null, new UIInfo(UIType.COLORMASK)),
            new FunctionInfo(gl, "compileShader", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "copyBufferSubData", null, [
                new FunctionParam(gl, "readTarget", new UIInfo(UIType.ENUM, bufferTargets)),
                new FunctionParam(gl, "writeTarget", new UIInfo(UIType.ENUM, bufferTargets)),
                new FunctionParam(gl, "readOffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "writeOffset",new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "size", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "copyTexImage2D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, faceTextureTargets)),
                new FunctionParam(gl, "level", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, ["ALPHA", "LUMINANCE", "LUMINANCE_ALPHA", "RGB", "RGBA", "R8", "RG8", "RGB565", "RGB8", "RGBA4", "RGB5_A1", "RGBA8", "RGB10_A2", "SRGB8", "SRGB8_ALPHA8", "R8I", "R8UI", "R16I", "R16UI", "R32I", "R32UI", "RG8I", "RG8UI", "RG16I", "RG16UI", "RG32I", "RG32UI", "RGBA8I", "RGBA8UI", "RGB10_A2UI", "RGBA16I", "RGBA16UI", "RGBA32I", "RGBA32UI"])),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "border", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "copyTexSubImage2D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, faceTextureTargets)),
                new FunctionParam(gl, "level", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "createBuffer", null, [
            ]),
            new FunctionInfo(gl, "createFramebuffer", null, [
            ]),
            new FunctionInfo(gl, "createProgram", null, [
            ]),
            new FunctionInfo(gl, "createRenderbuffer", null, [
            ]),
            new FunctionInfo(gl, "createShader", null, [
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, ["VERTEX_SHADER", "FRAGMENT_SHADER"]))
            ]),
            new FunctionInfo(gl, "createTexture", null, [
            ]),
            new FunctionInfo(gl, "cullFace", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, ["FRONT", "BACK", "FRONT_AND_BACK"]))
            ]),
            new FunctionInfo(gl, "deleteBuffer", null, [
                new FunctionParam(gl, "buffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "deleteFramebuffer", null, [
                new FunctionParam(gl, "framebuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "deleteProgram", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "deleteRenderbuffer", null, [
                new FunctionParam(gl, "renderbuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "deleteShader", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "deleteTexture", null, [
                new FunctionParam(gl, "texture", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "depthFunc", null, [
                new FunctionParam(gl, "func", new UIInfo(UIType.ENUM, ["NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS"]))
            ]),
            new FunctionInfo(gl, "depthMask", null, [
                new FunctionParam(gl, "flag", new UIInfo(UIType.BOOL))
            ]),
            new FunctionInfo(gl, "depthRange", null, [
                new FunctionParam(gl, "zNear", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "zFar", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "detachShader", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "disable", null, [
                new FunctionParam(gl, "cap", new UIInfo(UIType.ENUM, capabilities)),
            ]),
            new FunctionInfo(gl, "disableVertexAttribArray", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "drawArrays", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, drawModes)),
                new FunctionParam(gl, "first", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "count", new UIInfo(UIType.LONG))
            ], FunctionType.DRAW),
            new FunctionInfo(gl, "drawElements", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, drawModes)),
                new FunctionParam(gl, "count", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, elementTypes)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG))
            ], FunctionType.DRAW),
            new FunctionInfo(gl, "enable", null, [
                new FunctionParam(gl, "cap", new UIInfo(UIType.ENUM, capabilities)),
            ]),
            new FunctionInfo(gl, "enableVertexAttribArray", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "finish", null, [
            ]),
            new FunctionInfo(gl, "flush", null, [
            ]),
            new FunctionInfo(gl, "framebufferRenderbuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachment", new UIInfo(UIType.ENUM, attachments)),
                new FunctionParam(gl, "renderbuffertarget", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "renderbuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "framebufferTexture2D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachment", new UIInfo(UIType.ENUM, attachments)),
                new FunctionParam(gl, "textarget", new UIInfo(UIType.ENUM, faceTextureTargets)),
                new FunctionParam(gl, "texture", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "level", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "frontFace", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, ["CW", "CCW"]))
            ]),
            new FunctionInfo(gl, "generateMipmap", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bindTextureTargets))
            ]),
            new FunctionInfo(gl, "getActiveAttrib", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "getActiveUniform", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "getAttachedShaders", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "getAttribLocation", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "name", new UIInfo(UIType.STRING))
            ]),
            new FunctionInfo(gl, "getParameter", null, [
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, [
                    "ACTIVE_TEXTURE",
                    "ALIASED_LINE_WIDTH_RANGE",
                    "ALIASED_POINT_SIZE_RANGE",
                    "ALPHA_BITS",
                    "ARRAY_BUFFER_BINDING",
                    "BLEND",
                    "BLEND_COLOR",
                    "BLEND_DST_ALPHA",
                    "BLEND_DST_RGB",
                    "BLEND_EQUATION_ALPHA",
                    "BLEND_EQUATION_RGB",
                    "BLEND_SRC_ALPHA",
                    "BLEND_SRC_RGB",
                    "BLUE_BITS",
                    "COLOR_CLEAR_VALUE",
                    "COLOR_WRITEMASK",
                    "COMPRESSED_TEXTURE_FORMATS",
                    "COPY_READ_BUFFER_BINDING",
                    "COPY_WRITE_BUFFER_BINDING",
                    "CULL_FACE",
                    "CULL_FACE_MODE",
                    "CURRENT_PROGRAM",
                    "DEPTH_BITS",
                    "DEPTH_CLEAR_VALUE",
                    "DEPTH_FUNC",
                    "DEPTH_RANGE",
                    "DEPTH_TEST",
                    "DEPTH_WRITEMASK",
                    "DITHER",
                    "DRAW_BUFFER0",
                    "DRAW_BUFFER1",
                    "DRAW_BUFFER2",
                    "DRAW_BUFFER3",
                    "DRAW_BUFFER4",
                    "DRAW_BUFFER5",
                    "DRAW_BUFFER6",
                    "DRAW_BUFFER7",
                    "DRAW_BUFFER8",
                    "DRAW_BUFFER9",
                    "DRAW_BUFFER10",
                    "DRAW_BUFFER11",
                    "DRAW_BUFFER12",
                    "DRAW_BUFFER13",
                    "DRAW_BUFFER14",
                    "DRAW_BUFFER15",
                    "DRAW_FRAMEBUFFER_BINDING",
                    "ELEMENT_ARRAY_BUFFER_BINDING",
                    "FRAGMENT_SHADER_DERIVATIVE_HINT",
                    "FRONT_FACE",
                    "GENERATE_MIPMAP_HINT",
                    "GREEN_BITS",
                    "IMPLEMENTATION_COLOR_READ_FORMAT",
                    "IMPLEMENTATION_COLOR_READ_TYPE",
                    "LINE_WIDTH",
                    "MAJOR_VERSION",
                    "MAX_3D_TEXTURE_SIZE",
                    "MAX_ARRAY_TEXTURE_LAYERS",
                    "MAX_COLOR_ATTACHMENTS",
                    "MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS",
                    "MAX_COMBINED_TEXTURE_IMAGE_UNITS",
                    "MAX_COMBINED_UNIFORM_BLOCKS",
                    "MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS",
                    "MAX_CUBE_MAP_TEXTURE_SIZE",
                    "MAX_DRAW_BUFFERS",
                    "MAX_ELEMENT_INDEX",
                    "MAX_ELEMENTS_INDICES",
                    "MAX_ELEMENTS_VERTICES",
                    "MAX_FRAGMENT_INPUT_COMPONENTS",
                    "MAX_FRAGMENT_UNIFORM_BLOCKS",
                    "MAX_FRAGMENT_UNIFORM_COMPONENTS",
                    "MAX_FRAGMENT_UNIFORM_VECTORS",
                    "MAX_PROGRAM_TEXEL_OFFSET",
                    "MAX_RENDERBUFFER_SIZE",
                    "MAX_SAMPLES",
                    "MAX_SERVER_WAIT_TIMEOUT",
                    "MAX_TEXTURE_IMAGE_UNITS",
                    "MAX_TEXTURE_LOD_BIAS",
                    "MAX_TEXTURE_SIZE",
                    "MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS",
                    "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS",
                    "MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS",
                    "MAX_UNIFORM_BLOCK_SIZE",
                    "MAX_UNIFORM_BUFFER_BINDINGS",
                    "MAX_VARYING_COMPONENTS",
                    "MAX_VARYING_VECTORS",
                    "MAX_VERTEX_ATTRIBS",
                    "MAX_VERTEX_TEXTURE_IMAGE_UNITS",
                    "MAX_VERTEX_OUTPUT_COMPONENTS",
                    "MAX_VERTEX_UNIFORM_BLOCKS",
                    "MAX_VERTEX_UNIFORM_COMPONENTS",
                    "MAX_VERTEX_UNIFORM_VECTORS",
                    "MAX_VIEWPORT_DIMS",
                    "MIN_PROGRAM_TEXEL_OFFSET",
                    "MINOR_VERSION",
                    "NUM_COMPRESSED_TEXTURE_FORMATS",
                    "NUM_EXTENSIONS",
                    "NUM_PROGRAM_BINARY_FORMATS",
                    "NUM_SHADER_BINARY_FORMATS",
                    "PACK_ALIGNMENT",
                    "PACK_ROW_LENGTH",
                    "PACK_SKIP_PIXELS",
                    "PACK_SKIP_ROWS",
                    "PIXEL_PACK_BUFFER_BINDING",
                    "PIXEL_UNPACK_BUFFER_BINDING",
                    "POLYGON_OFFSET_FACTOR",
                    "POLYGON_OFFSET_FILL",
                    "POLYGON_OFFSET_UNITS",
                    "PRIMITIVE_RESTART_FIXED_INDEX",
                    "PROGRAM_BINARY_FORMATS",
                    "RASTERIZER_DISCARD",
                    "READ_BUFFER",
                    "READ_FRAMEBUFFER_BINDING",
                    "RED_BITS",
                    "RENDERBUFFER_BINDING",
                    "SAMPLE_ALPHA_TO_COVERAGE",
                    "SAMPLE_BUFFERS",
                    "SAMPLE_COVERAGE",
                    "SAMPLE_COVERAGE_INVERT",
                    "SAMPLE_COVERAGE_VALUE",
                    "SAMPLER_BINDING",
                    "SAMPLES",
                    "SCISSOR_BOX",
                    "SCISSOR_TEST",
                    "SHADER_BINARY_FORMATS",
                    "SHADER_COMPILER",
                    "STENCIL_BACK_FAIL",
                    "STENCIL_BACK_FUNC",
                    "STENCIL_BACK_PASS_DEPTH_FAIL",
                    "STENCIL_BACK_PASS_DEPTH_PASS",
                    "STENCIL_BACK_REF",
                    "STENCIL_BACK_VALUE_MASK",
                    "STENCIL_BACK_WRITEMASK",
                    "STENCIL_BITS",
                    "STENCIL_CLEAR_VALUE",
                    "STENCIL_FAIL",
                    "STENCIL_FUNC",
                    "STENCIL_PASS_DEPTH_FAIL",
                    "STENCIL_PASS_DEPTH_PASS",
                    "STENCIL_REF",
                    "STENCIL_TEST",
                    "STENCIL_VALUE_MASK",
                    "STENCIL_WRITEMASK",
                    "SUBPIXEL_BITS",
                    "TEXTURE_BINDING_2D",
                    "TEXTURE_BINDING_2D_ARRAY",
                    "TEXTURE_BINDING_3D",
                    "TEXTURE_BINDING_CUBE_MAP",
                    "TRANSFORM_FEEDBACK_BINDING",
                    "TRANSFORM_FEEDBACK_ACTIVE",
                    "TRANSFORM_FEEDBACK_BUFFER_BINDING",
                    "TRANSFORM_FEEDBACK_PAUSED",
                    "UNIFORM_BUFFER_BINDING",
                    "UNIFORM_BUFFER_START",
                    "UNPACK_ROW_LENGTH",
                    "UNPACK_SKIP_ROWS",
                    "VERTEX_ARRAY_BINDING",
                ])),
            ]),
            new FunctionInfo(gl, "getBufferParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bufferTargets)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["BUFFER_ACCESS_FLAGS", "BUFFER_MAPPED", "BUFFER_MAP_LENGTH", "BUFFER_MAP_OFFSET", "BUFFER_SIZE", "BUFFER_USAGE"])),
            ]),
            new FunctionInfo(gl, "getBufferSubData", null, null), // handled specially below
            new FunctionInfo(gl, "getError", null, [
            ]),
            new FunctionInfo(gl, "getSupportedExtensions", null, [
            ]),
            new FunctionInfo(gl, "getExtension", null, [
                new FunctionParam(gl, "name", new UIInfo(UIType.STRING))
            ]),
            new FunctionInfo(gl, "getFramebufferAttachmentParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachment", new UIInfo(UIType.ENUM, attachments)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE", "FRAMEBUFFER_ATTACHMENT_OBJECT_NAME", "FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL", "FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE"]))
            ]),
            new FunctionInfo(gl, "getProgramParameter", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["ACTIVE_ATTRIBUTES", "ACTIVE_ATTRIBUTE_MAX_LENGTH", "ACTIVE_UNIFORMS", "ACTIVE_UNIFORM_BLOCKS", "ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH", "ACTIVE_UNIFORM_MAX_LENGTH", "ATTACHED_SHADERS", "DELETE_STATUS", "INFO_LOG_LENGTH", "LINK_STATUS", "PROGRAM_BINARY_RETRIEVABLE_HINT", "TRANSFORM_FEEDBACK_BUFFER_MODE", "TRANSFORM_FEEDBACK_VARYINGS", "TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH", "VALIDATE_STATUS"]))
            ]),
            new FunctionInfo(gl, "getProgramInfoLog", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "getRenderbufferParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["RENDERBUFFER_WIDTH", "RENDERBUFFER_HEIGHT", "RENDERBUFFER_INTERNAL_FORMAT", "RENDERBUFFER_RED_SIZE", "RENDERBUFFER_GREEN_SIZE", "RENDERBUFFER_BLUE_SIZE", "RENDERBUFFER_ALPHA_SIZE", "RENDERBUFFER_DEPTH_SIZE", "RENDERBUFFER_STENCIL_SIZE"]))
            ]),
            new FunctionInfo(gl, "getShaderParameter", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["SHADER_TYPE", "DELETE_STATUS", "COMPILE_STATUS", "INFO_LOG_LENGTH", "SHADER_SOURCE_LENGTH"]))
            ]),
            new FunctionInfo(gl, "getShaderInfoLog", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "getShaderSource", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "getTexParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bindTextureTargets)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, texParamNames))
            ]),
            new FunctionInfo(gl, "getUniform", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)) // TODO: find a way to treat this as an integer? browsers don't like this...
            ]),
            new FunctionInfo(gl, "getUniformLocation", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "name", new UIInfo(UIType.STRING))
            ]),
            new FunctionInfo(gl, "getVertexAttrib", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["VERTEX_ATTRIB_ARRAY_BUFFER_BINDING", "VERTEX_ATTRIB_ARRAY_ENABLED", "VERTEX_ATTRIB_ARRAY_SIZE", "VERTEX_ATTRIB_ARRAY_STRIDE", "VERTEX_ATTRIB_ARRAY_TYPE", "VERTEX_ATTRIB_ARRAY_NORMALIZED", "CURRENT_VERTEX_ATTRIB"]))
            ]),
            new FunctionInfo(gl, "getVertexAttribOffset", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["VERTEX_ATTRIB_ARRAY_POINTER"]))
            ]),
            new FunctionInfo(gl, "hint", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["GENERATE_MIPMAP_HINT", "FRAGMENT_SHADER_DERIVATIVE_HINT_OES"])),
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, ["FASTEST", "NICEST", "DONT_CARE"]))
            ]),
            new FunctionInfo(gl, "isBuffer", null, [
                new FunctionParam(gl, "buffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "isEnabled", null, [
                new FunctionParam(gl, "cap", new UIInfo(UIType.ENUM, capabilities)),
            ]),
            new FunctionInfo(gl, "isFramebuffer", null, [
                new FunctionParam(gl, "framebuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "isProgram", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "isRenderbuffer", null, [
                new FunctionParam(gl, "renderbuffer", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "isShader", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "isTexture", null, [
                new FunctionParam(gl, "texture", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "lineWidth", null, [
                new FunctionParam(gl, "width", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "linkProgram", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "pixelStorei", null, [
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["PACK_ALIGNMENT", "UNPACK_ALIGNMENT", "UNPACK_COLORSPACE_CONVERSION_WEBGL", "UNPACK_FLIP_Y_WEBGL", "UNPACK_PREMULTIPLY_ALPHA_WEBGL", "PACK_ROW_LENGTH", "PACK_SKIP_PIXELS", "PACK_SKIP_ROWS", "UNPACK_IMAGE_HEIGHT", "UNPACK_ROW_LENGTH", "UNPACK_SKIP_IMAGES", "UNPACK_SKIP_PIXELS", "UNPACK_SKIP_ROWS"])),
                new FunctionParam(gl, "param", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "polygonOffset", null, [
                new FunctionParam(gl, "factor", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "units", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "readPixels", null, null), // handled specially below[
            new FunctionInfo(gl, "renderbufferStorage", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, renderableFormats)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "sampleCoverage", null, [
                new FunctionParam(gl, "value", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "invert", new UIInfo(UIType.BOOL))
            ]),
            new FunctionInfo(gl, "scissor", null, [
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "shaderSource", null, [
                new FunctionParam(gl, "shader", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "source", new UIInfo(UIType.STRING))
            ]),
            new FunctionInfo(gl, "stencilFunc", null, [
                new FunctionParam(gl, "func", new UIInfo(UIType.ENUM, ["NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS"])),
                new FunctionParam(gl, "ref", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK))
            ]),
            new FunctionInfo(gl, "stencilFuncSeparate", null, [
                new FunctionParam(gl, "face", new UIInfo(UIType.ENUM, ["FRONT", "BACK", "FRONT_AND_BACK"])),
                new FunctionParam(gl, "func", new UIInfo(UIType.ENUM, ["NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS"])),
                new FunctionParam(gl, "ref", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK))
            ]),
            new FunctionInfo(gl, "stencilMask", null, [
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK))
            ]),
            new FunctionInfo(gl, "stencilMaskSeparate", null, [
                new FunctionParam(gl, "face", new UIInfo(UIType.ENUM, ["FRONT", "BACK", "FRONT_AND_BACK"])),
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK))
            ]),
            new FunctionInfo(gl, "stencilOp", null, [
                new FunctionParam(gl, "fail", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
                new FunctionParam(gl, "zfail", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
                new FunctionParam(gl, "zpass", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"]))
            ]),
            new FunctionInfo(gl, "stencilOpSeparate", null, [
                new FunctionParam(gl, "face", new UIInfo(UIType.ENUM, ["FRONT", "BACK", "FRONT_AND_BACK"])),
                new FunctionParam(gl, "fail", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
                new FunctionParam(gl, "zfail", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
                new FunctionParam(gl, "zpass", new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"]))
            ]),
            new FunctionInfo(gl, "texImage2D", null, null), // handled specially below
            new FunctionInfo(gl, "texParameterf", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bindTextureTargets)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, texParamNames)),
                new FunctionParam(gl, "param", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "texParameteri", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bindTextureTargets)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, texParamNames)),
                new FunctionParam(gl, "param", new UIInfo(UIType.ENUM, ["NEAREST", "LINEAR", "NEAREST_MIPMAP_NEAREST", "LINEAR_MIPMAP_NEAREST", "NEAREST_MIPMAP_LINEAR", "LINEAR_MIPMAP_LINEAR", "CLAMP_TO_EDGE", "MIRRORED_REPEAT", "REPEAT", "COMPARE_REF_TO_TEXTURE", "LEQUAL", "GEQUAL", "LESS", "GREATER", "EQUAL", "NOTEQUAL", "ALWAYS", "NEVER", "RED", "GREEN", "BLUE", "ALPHA", "ZERO", "ONE"])),
            ]),
            new FunctionInfo(gl, "texSubImage2D", null, null), // handled specially below
            new FunctionInfo(gl, "compressedTexImage2D", null, null), // handled specially below
            new FunctionInfo(gl, "compressedTexSubImage2D", null, null), // handled specially below
            new FunctionInfo(gl, "uniform1f", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "uniform1fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform1i", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "uniform1iv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform2f", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "uniform2fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform2i", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "uniform2iv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform3f", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "z", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "uniform3fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform3i", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "z", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "uniform3iv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform4f", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "z", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "w", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "uniform4fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform4i", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "z", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "w", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "uniform4iv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix2fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix3fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix4fv", null, null), // handled specially below
            new FunctionInfo(gl, "useProgram", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "validateProgram", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT))
            ]),
            new FunctionInfo(gl, "vertexAttrib1f", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "vertexAttrib1fv", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY))
            ]),
            new FunctionInfo(gl, "vertexAttrib2f", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "vertexAttrib2fv", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY))
            ]),
            new FunctionInfo(gl, "vertexAttrib3f", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "z", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "vertexAttrib3fv", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY))
            ]),
            new FunctionInfo(gl, "vertexAttrib4f", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "y", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "z", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "w", new UIInfo(UIType.FLOAT))
            ]),
            new FunctionInfo(gl, "vertexAttrib4fv", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY))
            ]),
            new FunctionInfo(gl, "vertexAttribPointer", null, [
                new FunctionParam(gl, "indx", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "size", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, ["BYTE", "UNSIGNED_BYTE", "SHORT", "UNSIGNED_SHORT", "FIXED", "FLOAT"])),
                new FunctionParam(gl, "normalized", new UIInfo(UIType.BOOL)),
                new FunctionParam(gl, "stride", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "viewport", null, [
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG))
            ]),
            new FunctionInfo(gl, "blitFramebuffer", null, [
                new FunctionParam(gl, "srcX0", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "srcY0", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "srcX1", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "srcY1", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "dstX0", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "dstY0", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "dstX1", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "dstY1", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "mask", new UIInfo(UIType.BITMASK, ["COLOR_BUFFER_BIT", "DEPTH_BUFFER_BIT", "STENCIL_BUFFER_BIT"])),
                new FunctionParam(gl, "filter", new UIInfo(UIType.ENUM, ["NEAREST", "LINEAR"])),
            ]),
            new FunctionInfo(gl, "framebufferTextureLayer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachment", new UIInfo(UIType.ENUM, attachments)),
                new FunctionParam(gl, "texture", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "level", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "layer", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "invalidateFramebuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachments", new UIInfo(UIType.ARRAY)),
            ]),
            new FunctionInfo(gl, "invalidateSubFramebuffer", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, framebufferTargets)),
                new FunctionParam(gl, "attachments", new UIInfo(UIType.ARRAY)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "readBuffer", null, [
                new FunctionParam(gl, "src", new UIInfo(UIType.ENUM, readBufferEnums)),
            ]),
            /* Renderbuffer objects */
            new FunctionInfo(gl, "getInternalformatParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, renderableFormats)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["NUM_SAMPLE_COUNTS", "SAMPLES"])),
            ]),
            new FunctionInfo(gl, "renderbufferStorageMultisample", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["RENDERBUFFER"])),
                new FunctionParam(gl, "samples", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, renderableFormats)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
            ]),

            /* Texture objects */
            new FunctionInfo(gl, "texStorage2D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture2DTargets)),
                new FunctionParam(gl, "levels", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, sizedTextureInternalFormats)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "texStorage3D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)),
                new FunctionParam(gl, "levels", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, sizedTextureInternalFormats)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "depth", new UIInfo(UIType.LONG)),
            ]),

            new FunctionInfo(gl, "copyTexSubImage3D", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)),
                new FunctionParam(gl, "level", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "zoffset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "width", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "height", new UIInfo(UIType.LONG)),
            ]),

            /* Programs and shaders */
            new FunctionInfo(gl, "GLint getFragDataLocation", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "name", new UIInfo(UIType.STRING)),
            ]),

            new FunctionInfo(gl, "uniform1ui", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "v0", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "uniform2ui", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "v0", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v1", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "uniform3ui", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "v0", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v1", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v2", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "uniform4ui", null, [
                new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "v0", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v1", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v2", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "v3", new UIInfo(UIType.ULONG)),
            ]),

            /* Vertex attribs */
            new FunctionInfo(gl, "vertexAttribI4i", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "z", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "w", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "vertexAttribI4iv", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY)),
            ]),
            new FunctionInfo(gl, "vertexAttribI4ui", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "x", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "y", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "z", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "w", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "vertexAttribI4uiv", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY)),
            ]),
            new FunctionInfo(gl, "vertexAttribIPointer", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "size", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, ["BYTE", "UNSIGNED_BYTE", "SHORT", "UNSIGNED_SHORT", "INT", "UNSIGNED_INT"])),
                new FunctionParam(gl, "stride", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)),
            ]),

            /* Writing to the drawing buffer */
            new FunctionInfo(gl, "vertexAttribDivisor", null, [
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "divisor", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "drawArraysInstanced", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, drawModes)),
                new FunctionParam(gl, "first", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "count", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "instanceCount", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "drawElementsInstanced", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, drawModes)),
                new FunctionParam(gl, "count", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, elementTypes)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "instanceCount", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "drawRangeElements", null, [
                new FunctionParam(gl, "mode", new UIInfo(UIType.ENUM, drawModes)),
                new FunctionParam(gl, "start", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "end", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "count", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, elementTypes)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)),
            ]),

            /* Multiple Render Targets */
            new FunctionInfo(gl, "drawBuffers", null, [
                new FunctionParam(gl, "buffers", new UIInfo(UIType.ARRAY)),
            ]),

            new FunctionInfo(gl, "clearBufferfi", null, [
                new FunctionParam(gl, "buffer", new UIInfo(UIType.ENUM, ["DEPTH_STENCIL"])),
                new FunctionParam(gl, "drawbuffer", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "depth", new UIInfo(UIType.FLOAT)),
                new FunctionParam(gl, "stencil", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "clearBufferfv", null, null),  // handled specially below
            new FunctionInfo(gl, "clearBufferiv", null,  null),  // handled specially below
            new FunctionInfo(gl, "clearBufferuiv", null,  null),  // handled specially below
            /* Query Objects */
            new FunctionInfo(gl, "createQuery", null, [,
            ]),
            new FunctionInfo(gl, "deleteQuery", null, [
                new FunctionParam(gl, "query", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "GLboolean isQuery", null, [
                new FunctionParam(gl, "query", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "beginQuery", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, queryTargets)),
                new FunctionParam(gl, "query", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "endQuery", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, queryTargets)),
            ]),
            new FunctionInfo(gl, "getQuery", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, queryTargets)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["CURRENT_QUERY"])),
            ]),
            new FunctionInfo(gl, "getQueryParameter", null, [
                new FunctionParam(gl, "query", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["QUERY_RESULT", "QUERY_RESULT_AVAILABLE"])),
            ]),

            /* Sampler Objects */
            new FunctionInfo(gl, "createSampler", null, [,
            ]),
            new FunctionInfo(gl, "deleteSampler", null, [
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "GLboolean isSampler", null, [
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "bindSampler", null, [
                new FunctionParam(gl, "unit", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "samplerParameteri", null, [
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, samplerParamNames)),
                new FunctionParam(gl, "param", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "samplerParameterf", null, [
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, samplerParamNames)),
                new FunctionParam(gl, "param", new UIInfo(UIType.FLOAT)),
            ]),
            new FunctionInfo(gl, "getSamplerParameter", null, [
                new FunctionParam(gl, "sampler", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, samplerParamNames)),
            ]),

            /* Sync objects */
            new FunctionInfo(gl, "fenceSync", null, [
                new FunctionParam(gl, "condition", new UIInfo(UIType.ENUM, ["SYNC_GPU_COMMANDS_COMPLETE"])),
                new FunctionParam(gl, "flags", new UIInfo(UIType.BITMASK, [])),
            ]),
            new FunctionInfo(gl, "GLboolean isSync", null, [
                new FunctionParam(gl, "sync", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "deleteSync", null, [
                new FunctionParam(gl, "sync", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "clientWaitSync", null, [
                new FunctionParam(gl, "sync", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "flags", new UIInfo(UIType.BITMASK, ["SYNC_FLUSH_COMMANDS_BIT"])),
                new FunctionParam(gl, "timeout", new  UIInfo(UIType.ULONG)),  // Uint64!
            ]),
            new FunctionInfo(gl, "waitSync", null, [
                new FunctionParam(gl, "sync", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "flags", new UIInfo(UIType.BITMASK, [])),
                new FunctionParam(gl, "timeout", new  UIInfo(UIType.ULONG)),  // Uint64!
            ]),
            new FunctionInfo(gl, "getSyncParameter", null, [
                new FunctionParam(gl, "sync", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["OBJECT_TYPE", "SYNC_STATUS", "SYNC_CONDITION", "SYNC_FLAGS"])),
            ]),

            /* Transform Feedback */
            new FunctionInfo(gl, "createTransformFeedback", null, [,
            ]),
            new FunctionInfo(gl, "deleteTransformFeedback", null, [
                new FunctionParam(gl, "tf", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "isTransformFeedback", null, [
                new FunctionParam(gl, "tf", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "bindTransformFeedback ", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["TRANSFORM_FEEDBACK"])),
                new FunctionParam(gl, "tf", new UIInfo(UIType.Object)),
            ]),
            new FunctionInfo(gl, "beginTransformFeedback", null, [
                new FunctionParam(gl, "primitiveMode", new UIInfo(UIType.ENUM, ["POINTS", "LINES", "TRIANGLES"])),
            ]),
            new FunctionInfo(gl, "endTransformFeedback", null, [,
            ]),
            new FunctionInfo(gl, "transformFeedbackVaryings", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "varyings", new UIInfo(UIType.ARRAY)),
                new FunctionParam(gl, "bufferMode", new UIInfo(UIType.ENUM, ["INTERLEAVED_ATTRIBS", "SEPARATE_ATTRIBS"])),
            ]),
            new FunctionInfo(gl, "getTransformFeedbackVarying", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "pauseTransformFeedback", null, [,
            ]),
            new FunctionInfo(gl, "resumeTransformFeedback", null, [,
            ]),

            /* Uniform Buffer Objects and Transform Feedback Buffers */
            new FunctionInfo(gl, "bindBufferBase", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["TRANSFORM_FEEDBACK_BUFFER", "UNIFORM_BUFFER"])),
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "buffer", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "bindBufferRange", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["TRANSFORM_FEEDBACK_BUFFER", "UNIFORM_BUFFER"])),
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "buffer", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)),
                new FunctionParam(gl, "size", new UIInfo(UIType.LONG)),
            ]),
            new FunctionInfo(gl, "getIndexedParameter", null, [
                new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, ["TRANSFORM_FEEDBACK_BUFFER_BINDING", "TRANSFORM_FEEDBACK_BUFFER_SIZE", "TRANSFORM_FEEDBACK_BUFFER_START", "UNIFORM_BUFFER_BINDING", "UNIFORM_BUFFER_SIZE", "UNIFORM_BUFFER_START"])),
                new FunctionParam(gl, "index", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "getUniformIndices", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformNames", new UIInfo(UIType.ARRAY)),
            ]),
            new FunctionInfo(gl, "getActiveUniforms", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformIndices", new UIInfo(UIType.ARRAY)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["UNIFORM_TYPE", "UNIFORM_SIZE", "UNIFORM_NAME_LENGTH", "UNIFORM_BLOCK_INDEX", "UNIFORM_ARRAY_STRIDE", "UNIFORM_MATRIX_STRIDE", "UNIFORM_IS_ROW_MAJOR"])),
            ]),
            new FunctionInfo(gl, "getUniformBlockIndex", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformBlockName", new UIInfo(UIType.STRING)),
            ]),
            new FunctionInfo(gl, "getActiveUniformBlockParameter", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformBlockIndex", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "pname", new UIInfo(UIType.ENUM, ["UNIFORM_BLOCK_BINDING", "UNIFORM_BLOCK_DATA_SIZE", "UNIFORM_BLOCK_NAME_LENGTH", "UNIFORM_BLOCK_ACTIVE_UNIFORMS", "UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES", "UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER", "UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER",])),
            ]),
            new FunctionInfo(gl, "getActiveUniformBlockName", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformBlockIndex", new UIInfo(UIType.ULONG)),
            ]),
            new FunctionInfo(gl, "uniformBlockBinding", null, [
                new FunctionParam(gl, "program", new UIInfo(UIType.OBJECT)),
                new FunctionParam(gl, "uniformBlockIndex", new UIInfo(UIType.ULONG)),
                new FunctionParam(gl, "uniformBlockBinding", new UIInfo(UIType.ULONG)),
            ]),

            /* Vertex Array Objects */
            new FunctionInfo(gl, "createVertexArray", null, [,
            ]),
            new FunctionInfo(gl, "deleteVertexArray", null, [
                new FunctionParam(gl, "vertexArray", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "GLboolean isVertexArray", null, [
                new FunctionParam(gl, "vertexArray", new UIInfo(UIType.OBJECT)),
            ]),
            new FunctionInfo(gl, "bindVertexArray", null, [
                new FunctionParam(gl, "vertexArray", new UIInfo(UIType.OBJECT)),
            ]),


            new FunctionInfo(gl, "texImage3D", null, null), // handled specially below
            new FunctionInfo(gl, "texSubImage3D", null, null), // handled specially below
            new FunctionInfo(gl, "compressedTexImage3D", null, null), // handled specially below
            new FunctionInfo(gl, "compressedTexSubImage3D", null, null), // handled specially below
            new FunctionInfo(gl, "uniform1uiv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform2uiv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform3uiv", null, null), // handled specially below
            new FunctionInfo(gl, "uniform4uiv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix3x2fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix4x2fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix2x3fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix4x3fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix2x4fv", null, null), // handled specially below
            new FunctionInfo(gl, "uniformMatrix3x4fv", null, null), // handled specially below
            new FunctionInfo(gl, "readPixels", null, null), // handled specially below
            new FunctionInfo(gl, "getBufferSubData", null, null), // handled specially below
            new FunctionInfo(gl, "texImage2D", null, null), // handled specially below
            new FunctionInfo(gl, "texSubImage2D", null, null), // handled specially below

        ];

        // Build lookup
        for (var n = 0; n < functionInfos.length; n++) {
            functionInfos[functionInfos[n].name] = functionInfos[n];
        }

        functionInfos["clearBufferfv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "buffer", new UIInfo(UIType.ENUM, [ ])));
            args.push(new FunctionParam(gl, "drawbuffer", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY)));

            if (call.args.length >= 4) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["clearBufferiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "buffer", new UIInfo(UIType.ENUM, [ ])));
            args.push(new FunctionParam(gl, "drawbuffer", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["clearBufferuiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "buffer", new UIInfo(UIType.ENUM, [ ])));
            args.push(new FunctionParam(gl, "drawbuffer", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "values", new UIInfo(UIType.ARRAY)));

            if (call.args.length >= 4) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            return args;
        };

        functionInfos["texImage3D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, allUncompressedTextureInternalFormats)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "depth", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "border", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
            args.push(new FunctionParam(gl, "type", textureTypes));
            if (typeof(call.args[9]) === "number") {
              args.push(new FunctionParam(gl, "pboOffset", new UIInfo(UIType.LONG)));
            } else if (util.isTypedArray(call.args[9])) {
              args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.ARRAY)));
              if (call.args.length >= 11) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
              }
            } else {
              args.push(new FunctionParam(gl, "source", new UIInfo(UIType.ARRAY)));
            }
            return args;
        };
        functionInfos["texSubImage3D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "zoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "depth", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
            args.push(new FunctionParam(gl, "type", textureTypes));
            if (typeof(call.args[10]) === "number") {
                args.push(new FunctionParam(gl, "pboOffset", new UIInfo(UIType.LONG)));
            } else if (util.isTypedArray(call.args[10])) {
                args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.ARRAY)));
                if (call.args.length >= 12) {
                  args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
                }
            } else {
                args.push(new FunctionParam(gl, "source", new UIInfo(UIType.OBJECT)));
            }
            return args;
        };
        functionInfos["compressedTexImage2D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, faceTextureTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, compressedTextureInternalFormats)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "border", new UIInfo(UIType.LONG)));
            if (typeof(call.args[6]) === "number") {
                args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.ARRAY)));
              if (call.args.length >= 7) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
              }
              if (call.args.length >= 8) {
                args.push(new FunctionParam(gl, "srcLengthOverride", new UIInfo(UIType.ULONG)))
              }
            }
            return args;
        };
        functionInfos["compressedTexImage3D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, compressedTextureInternalFormats)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "depth", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "border", new UIInfo(UIType.LONG)));
            if (typeof(call.args[7]) === "number") {
              args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.OBJECT)));
              if (call.args.length >= 8) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
              }
              if (call.args.length >= 9) {
                args.push(new FunctionParam(gl, "srcLengthOverride", new UIInfo(UIType.ULONG)));
              }
            }
            return args;
        };
        functionInfos["compressedTexSubImage2D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture2DTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, compressedTextureInternalFormats)));
            if (typeof(call.args[7]) === "number") {
              args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.OBJECT)));
              if (call.args.length >= 8) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
              }
              if (call.args.length >= 9) {
                args.push(new FunctionParam(gl, "srcLengthOverride", new UIInfo(UIType.ULONG)));
              }
            }
            return args;
        };
        functionInfos["compressedTexSubImage3D"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, texture3DTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "zoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "depth", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, compressedTextureInternalFormats)));
            if (typeof(call.args[9]) === "number") {
              args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.OBJECT)));
              if (call.args.length >= 10) {
                args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
              }
              if (call.args.length >= 11) {
                args.push(new FunctionParam(gl, "srcLengthOverride", new UIInfo(UIType.ULONG)));
              }
            }
            return args;
        };
        functionInfos["uniform1fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform2fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform3fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform4fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform1iv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform2iv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform3iv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform4iv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform1uiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform2uiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform3uiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniform4uiv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 3) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix2fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix3x2fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix4x2fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix2x3fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix3fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix4x3fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix2x4fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix3x4fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        functionInfos["uniformMatrix4fv"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "location", new UIInfo(UIType.OBJECT)));
            args.push(new FunctionParam(gl, "transpose", new UIInfo(UIType.BOOL)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.ARRAY)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)))
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "srcLength", new UIInfo(UIType.ULONG)))
            }
            return args;
        };
        /* Reading back pixels */
        // WebGL1:
        functionInfos["readPixels"].getArgs = function(call) {
            var args = [ ];
            args.push(new FunctionParam(gl, "x", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "y", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, ["RED", "RED_INTEGER", "RG", "RG_INTEGER", "RGB", "RGB_INTEGER", "RGBA", "RGBA_INTEGER", "LUMINANCE_ALPHA", "LUMINANCE", "ALPHA"])));
            args.push(new FunctionParam(gl, "type", new UIInfo(UIType.ENUM, ["UNSIGNED_BYTE", "BYTE", "HALF_FLOAT", "FLOAT", "UNSIGNED_SHORT_5_6_5", "UNSIGNED_SHORT_4_4_4_4", "UNSIGNED_SHORT_5_5_5_1", "UNSIGNED_INT_2_10_10_10_REV", "UNSIGNED_INT_10F_11F_11F_REV", "UNSIGNED_INT_5_9_9_9_REV"])));
            if (typeof(call.args[6]) === "number") {
              args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "dstData", new UIInfo(UIType.OBJECT)));
            }
            if (call.args.length === 7) {
              args.push(new FunctionParam(gl, "dstOffset", new UIInfo(UIType.ULONG)));
            }
            return args;
        };

        functionInfos["bufferData"].getArgs = function (call) {
            var args = [];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bufferTargets)));
            if (typeof(call.args[1]) === "number") {
              args.push(new FunctionParam(gl, "size", new UIInfo(UIType.LONG)));
            } else {
              args.push(new FunctionParam(gl, "data", new UIInfo(UIType.OBJECT)));
            }
            args.push(new FunctionParam(gl, "usage", new UIInfo(UIType.ENUM, ["STREAM_DRAW", "STREAM_READ", "STREAM_COPY", "STATIC_DRAW", "STATIC_READ", "STATIC_COPY", "DYNAMIC_DRAW", "DYNAMIC_READ", "DYNAMIC_COPY"])));
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "length", new UIInfo(UIType.LONG)));
            }
            return args;
        };
        functionInfos["bufferSubData"].getArgs = function (call) {
            var args = [];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bufferTargets)));
            args.push(new FunctionParam(gl, "offset", new UIInfo(UIType.ULONG)));
            args.push(new FunctionParam(gl, "data", new UIInfo(UIType.OBJECT)));
            if (call.args.length === 4) {
              args.push(new FunctionParam(gl, "length", new UIInfo(UIType.LONG)));
            }
            return args;
        };
        functionInfos["getBufferSubData"].getArgs = function (call) {
            var args = [];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, bufferTargets)));
            args.push(new FunctionParam(gl, "srcByteOffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "dstBuffer", new UIInfo(UIType.OBJECT)));
            if (call.args.length >= 4) {
              args.push(new FunctionParam(gl, "dstOffset", new UIInfo(UIType.LONG)));
            }
            if (call.args.length === 5) {
              args.push(new FunctionParam(gl, "length", new UIInfo(UIType.LONG)));
            }
            return args;
        };

        functionInfos["texImage2D"].getArgs = function (call) {
            var args = [];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, faceTextureTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "internalformat", new UIInfo(UIType.ENUM, allUncompressedTextureInternalFormats)));
            if (call.args.length >= 9) {
                args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
                args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
                args.push(new FunctionParam(gl, "border", new UIInfo(UIType.LONG)));
                args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
                args.push(new FunctionParam(gl, "type", textureTypes));
                if (util.isTypedArray(call.args[9])) {
                  args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.ARRAY)));
                } else {
                  args.push(new FunctionParam(gl, "source", new UIInfo(UIType.OBJECT)));
                }
                if (call.args.length >= 10) {
                  args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
                }
            } else {
                args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
                args.push(new FunctionParam(gl, "type", textureTypes));
                args.push(new FunctionParam(gl, "value", new UIInfo(UIType.OBJECT)));
            }
            return args;
        };
        functionInfos["texSubImage2D"].getArgs = function (call) {
            var args = [];
            args.push(new FunctionParam(gl, "target", new UIInfo(UIType.ENUM, faceTextureTargets)));
            args.push(new FunctionParam(gl, "level", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "xoffset", new UIInfo(UIType.LONG)));
            args.push(new FunctionParam(gl, "yoffset", new UIInfo(UIType.LONG)));
            if (call.args.length == 9) {
                args.push(new FunctionParam(gl, "width", new UIInfo(UIType.LONG)));
                args.push(new FunctionParam(gl, "height", new UIInfo(UIType.LONG)));
                args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
                args.push(new FunctionParam(gl, "type", textureTypes));
                if (typeof(call.args[8]) === "number") {
                    args.push(new FunctionParam(gl, "pboOffset", new UIInfo(UIType.LONG)));
                } else if (util.isTypedArray(call.args[8])) {
                    args.push(new FunctionParam(gl, "srcData", new UIInfo(UIType.ARRAY)));
                    if (call.args.length >= 10) {
                        args.push(new FunctionParam(gl, "srcOffset", new UIInfo(UIType.ULONG)));
                    }
                } else {
                  args.push(new FunctionParam(gl, "source", new UIInfo(UIType.OBJECT)));
                }
            } else {
                args.push(new FunctionParam(gl, "format", new UIInfo(UIType.ENUM, textureFormats)));
                args.push(new FunctionParam(gl, "type", textureTypes));
                args.push(new FunctionParam(gl, "value", new UIInfo(UIType.OBJECT)));
            }
            return args;
        };

        info.functions = functionInfos;
    };

    var StateParameter = function (staticgl, name, readOnly, ui) {
        this.value = staticgl[name];
        this.name = name;
        this.readOnly = readOnly;
        this.ui = ui;

        this.getter = function (gl) {
            try {
                return gl.getParameter(gl[this.name]);
            } catch (e) {
                console.log("unable to get state parameter " + this.name);
                return null;
            }
        };
    };

    function setupStateParameters(gl) {
        const isWebGL2 = util.isWebGL2(gl)

        if (info.stateParameters) {
            return;
        }

        var drawBuffers = [
          "NONE",
          "BACK",
          "DRAW_BUFFER0",
          "DRAW_BUFFER1",
          "DRAW_BUFFER2",
          "DRAW_BUFFER3",
          "DRAW_BUFFER4",
          "DRAW_BUFFER5",
          "DRAW_BUFFER6",
          "DRAW_BUFFER7",
          "DRAW_BUFFER8",
          "DRAW_BUFFER9",
          "DRAW_BUFFER10",
          "DRAW_BUFFER11",
          "DRAW_BUFFER12",
          "DRAW_BUFFER13",
          "DRAW_BUFFER14",
          "DRAW_BUFFER15",
        ];

        var maxTextureUnits = gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS);

        var hintValues = ["FASTEST", "NICEST", "DONT_CARE"];
        var stateParameters = [
            new StateParameter(gl, "ACTIVE_TEXTURE", false, new UIInfo(UIType.ENUM, ["TEXTURE0", "TEXTURE1", "TEXTURE2", "TEXTURE3", "TEXTURE4", "TEXTURE5", "TEXTURE6", "TEXTURE7", "TEXTURE8", "TEXTURE9", "TEXTURE10", "TEXTURE11", "TEXTURE12", "TEXTURE13", "TEXTURE14", "TEXTURE15", "TEXTURE16", "TEXTURE17", "TEXTURE18", "TEXTURE19", "TEXTURE20", "TEXTURE21", "TEXTURE22", "TEXTURE23", "TEXTURE24", "TEXTURE25", "TEXTURE26", "TEXTURE27", "TEXTURE28", "TEXTURE29", "TEXTURE30", "TEXTURE31"])),
            new StateParameter(gl, "ALIASED_LINE_WIDTH_RANGE", true, new UIInfo(UIType.RANGE)),
            new StateParameter(gl, "ALIASED_POINT_SIZE_RANGE", true, new UIInfo(UIType.RANGE)),
            new StateParameter(gl, "ALPHA_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "ARRAY_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
            new StateParameter(gl, "BLEND", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "BLEND_COLOR", false, new UIInfo(UIType.COLOR)),
            new StateParameter(gl, "BLEND_DST_ALPHA", false, new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA. GL_CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA"])),
            new StateParameter(gl, "BLEND_DST_RGB", false, new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA. GL_CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA"])),
            new StateParameter(gl, "BLEND_EQUATION_ALPHA", false, new UIInfo(UIType.ENUM, ["FUNC_ADD", "FUNC_SUBTRACT", "FUNC_REVERSE_SUBTRACT"])),
            new StateParameter(gl, "BLEND_EQUATION_RGB", false, new UIInfo(UIType.ENUM, ["FUNC_ADD", "FUNC_SUBTRACT", "FUNC_REVERSE_SUBTRACT"])),
            new StateParameter(gl, "BLEND_SRC_ALPHA", false, new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE"])),
            new StateParameter(gl, "BLEND_SRC_RGB", false, new UIInfo(UIType.ENUM, ["ZERO", "ONE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE"])),
            new StateParameter(gl, "BLUE_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "COLOR_CLEAR_VALUE", false, new UIInfo(UIType.COLOR)),
            new StateParameter(gl, "COLOR_WRITEMASK", false, new UIInfo(UIType.COLORMASK)),
            new StateParameter(gl, "CULL_FACE", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "CULL_FACE_MODE", false, new UIInfo(UIType.ENUM, ["FRONT", "BACK", "FRONT_AND_BACK"])),
            new StateParameter(gl, "CURRENT_PROGRAM", false, new UIInfo(UIType.OBJECT)),
            new StateParameter(gl, "DEPTH_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "DEPTH_CLEAR_VALUE", false, new UIInfo(UIType.FLOAT)),
            new StateParameter(gl, "DEPTH_FUNC", false, new UIInfo(UIType.ENUM, ["NEVER", "LESS", "EQUAL", "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL", "ALWAYS"])),
            new StateParameter(gl, "DEPTH_RANGE", false, new UIInfo(UIType.RANGE)),
            new StateParameter(gl, "DEPTH_TEST", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "DEPTH_WRITEMASK", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "DITHER", true, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "ELEMENT_ARRAY_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
            new StateParameter(gl, "FRAGMENT_SHADER_DERIVATIVE_HINT_OES", false, new UIInfo(UIType.ENUM, hintValues)),
            new StateParameter(gl, "FRAMEBUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
            new StateParameter(gl, "FRONT_FACE", false, new UIInfo(UIType.ENUM, ["CW", "CCW"])),
            new StateParameter(gl, "GENERATE_MIPMAP_HINT", false, new UIInfo(UIType.ENUM, hintValues)),
            new StateParameter(gl, "GREEN_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "LINE_WIDTH", false, new UIInfo(UIType.FLOAT)),
            new StateParameter(gl, "MAX_COMBINED_TEXTURE_IMAGE_UNITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_CUBE_MAP_TEXTURE_SIZE", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_FRAGMENT_UNIFORM_VECTORS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_RENDERBUFFER_SIZE", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_TEXTURE_IMAGE_UNITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_TEXTURE_MAX_ANISOTROPY_EXT", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_TEXTURE_SIZE", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_VARYING_VECTORS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_VERTEX_ATTRIBS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_VERTEX_TEXTURE_IMAGE_UNITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_VERTEX_UNIFORM_VECTORS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "MAX_VIEWPORT_DIMS", true, new UIInfo(UIType.WH)),
            new StateParameter(gl, "PACK_ALIGNMENT", false, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "POLYGON_OFFSET_FACTOR", false, new UIInfo(UIType.FLOAT)),
            new StateParameter(gl, "POLYGON_OFFSET_FILL", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "POLYGON_OFFSET_UNITS", false, new UIInfo(UIType.FLOAT)),
            new StateParameter(gl, "RED_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "RENDERBUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
            new StateParameter(gl, "RENDERER", true, new UIInfo(UIType.STRING)),
            new StateParameter(gl, "SAMPLE_BUFFERS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "SAMPLE_COVERAGE_INVERT", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "SAMPLE_COVERAGE_VALUE", false, new UIInfo(UIType.FLOAT)),
            new StateParameter(gl, "SAMPLES", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "SCISSOR_BOX", false, new UIInfo(UIType.RECT)),
            new StateParameter(gl, "SCISSOR_TEST", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "SHADING_LANGUAGE_VERSION", true, new UIInfo(UIType.STRING)),
            new StateParameter(gl, "STENCIL_BACK_FAIL", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_BACK_FUNC", false, new UIInfo(UIType.ENUM, ["NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS"])),
            new StateParameter(gl, "STENCIL_BACK_PASS_DEPTH_FAIL", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_BACK_PASS_DEPTH_PASS", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_BACK_REF", false, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "STENCIL_BACK_VALUE_MASK", false, new UIInfo(UIType.BITMASK)),
            new StateParameter(gl, "STENCIL_BACK_WRITEMASK", false, new UIInfo(UIType.BITMASK)),
            new StateParameter(gl, "STENCIL_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "STENCIL_CLEAR_VALUE", false, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "STENCIL_FAIL", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_FUNC", false, new UIInfo(UIType.ENUM, ["NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS"])),
            new StateParameter(gl, "STENCIL_PASS_DEPTH_FAIL", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_PASS_DEPTH_PASS", false, new UIInfo(UIType.ENUM, ["KEEP", "ZERO", "REPLACE", "INCR", "INCR_WRAP", "DECR", "DECR_WRAP", "INVERT"])),
            new StateParameter(gl, "STENCIL_REF", false, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "STENCIL_TEST", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "STENCIL_VALUE_MASK", false, new UIInfo(UIType.BITMASK)),
            new StateParameter(gl, "STENCIL_WRITEMASK", false, new UIInfo(UIType.BITMASK)),
            new StateParameter(gl, "SUBPIXEL_BITS", true, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "UNPACK_ALIGNMENT", false, new UIInfo(UIType.LONG)),
            new StateParameter(gl, "UNPACK_COLORSPACE_CONVERSION_WEBGL", false, new UIInfo(UIType.ENUM, ["NONE", "BROWSER_DEFAULT_WEBGL"])),
            new StateParameter(gl, "UNPACK_FLIP_Y_WEBGL", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "UNPACK_PREMULTIPLY_ALPHA_WEBGL", false, new UIInfo(UIType.BOOL)),
            new StateParameter(gl, "VENDOR", true, new UIInfo(UIType.STRING)),
            new StateParameter(gl, "VERSION", true, new UIInfo(UIType.STRING)),
            new StateParameter(gl, "VIEWPORT", false, new UIInfo(UIType.RECT)),
        ];

        if (isWebGL2) {
            stateParameters.splice(stateParameters.length, 0, ...[
                new StateParameter(gl, "COPY_READ_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "COPY_WRITE_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "DRAW_FRAMEBUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "FRAGMENT_SHADER_DERIVATIVE_HINT", false, new UIInfo(UIType.ENUM, hintValues)),
                new StateParameter(gl, "MAX_3D_TEXTURE_SIZE", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_ARRAY_TEXTURE_LAYERS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_CLIENT_WAIT_TIMEOUT_WEBGL", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_COLOR_ATTACHMENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_COMBINED_UNIFORM_BLOCKS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_DRAW_BUFFERS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_ELEMENT_INDEX", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_ELEMENTS_INDICES", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_ELEMENTS_VERTICES", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_FRAGMENT_INPUT_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_FRAGMENT_UNIFORM_BLOCKS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_FRAGMENT_UNIFORM_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_PROGRAM_TEXEL_OFFSET", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_SAMPLES", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_SERVER_WAIT_TIMEOUT", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_TEXTURE_LOD_BIAS", true, new UIInfo(UIType.FLOAT)),
                new StateParameter(gl, "MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_UNIFORM_BLOCK_SIZE", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_UNIFORM_BUFFER_BINDINGS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_VARYING_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_VERTEX_OUTPUT_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_VERTEX_UNIFORM_BLOCKS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MAX_VERTEX_UNIFORM_COMPONENTS", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "MIN_PROGRAM_TEXEL_OFFSET", true, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "PACK_ROW_LENGTH", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "PACK_SKIP_PIXELS", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "PACK_SKIP_ROWS", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "PIXEL_PACK_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "PIXEL_UNPACK_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "RASTERIZER_DISCARD", false, new UIInfo(UIType.BOOL)),
                new StateParameter(gl, "READ_BUFFER", false, new UIInfo(UIType.ENUM, readBufferEnums)),
                new StateParameter(gl, "READ_FRAMEBUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "SAMPLE_ALPHA_TO_COVERAGE", false, new UIInfo(UIType.BOOL)),
                new StateParameter(gl, "SAMPLE_COVERAGE", false, new UIInfo(UIType.BOOL)),
                new StateParameter(gl, "SAMPLER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "TEXTURE_BINDING_2D_ARRAY", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "TEXTURE_BINDING_3D", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "TRANSFORM_FEEDBACK_ACTIVE", false, new UIInfo(UIType.BOOL)),
                new StateParameter(gl, "TRANSFORM_FEEDBACK_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "TRANSFORM_FEEDBACK_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "TRANSFORM_FEEDBACK_PAUSED", false, new UIInfo(UIType.BOOL)),
                new StateParameter(gl, "UNIFORM_BUFFER_BINDING", false, new UIInfo(UIType.OBJECT)),
                new StateParameter(gl, "UNIFORM_BUFFER_OFFSET_ALIGNMENT", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "UNPACK_IMAGE_HEIGHT", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "UNPACK_ROW_LENGTH", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "UNPACK_SKIP_IMAGES", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "UNPACK_SKIP_PIXELS", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "UNPACK_SKIP_ROWS", false, new UIInfo(UIType.LONG)),
                new StateParameter(gl, "VERTEX_ARRAY_BINDING", false, new UIInfo(UIType.OBJECT)),
            ]);
        }

        function makeTextureStateParametersForBinding(binding) {
          for (let n = 0; n < maxTextureUnits; ++n) {
            var param = new StateParameter(gl, binding + "_" + n, false, new UIInfo(UIType.OBJECT));
            param.getter = (function (n) {
                return function (gl) {
                    var existingBinding = gl.getParameter(gl.ACTIVE_TEXTURE);
                    gl.activeTexture(gl.TEXTURE0 + n);
                    var result = gl.getParameter(gl[binding]);
                    gl.activeTexture(existingBinding);
                    return result;
                };
            })(n);
            stateParameters.push(param);
          }
        }

        makeTextureStateParametersForBinding("TEXTURE_BINDING_2D");
        makeTextureStateParametersForBinding("TEXTURE_BINDING_CUBE_MAP");

        if (isWebGL2) {
            makeTextureStateParametersForBinding("TEXTURE_BINDING_2D_ARRAY");
            makeTextureStateParametersForBinding("TEXTURE_BINDING_3D");

            // fix: on WebGL1 need if WEBGL_draw_buffers is enabled?
            var maxDrawBuffers = gl.getParameter(gl.MAX_DRAW_BUFFERS);
            for (let n = 0; n < maxDrawBuffers; ++n) {
              stateParameters.push(new StateParameter(gl, "DRAW_BUFFER" + n, false, new UIInfo(UIType.ENUM, drawBuffers)));
            }
        }

        // Build lookup
        for (let n = 0; n < stateParameters.length; n++) {
            stateParameters[stateParameters[n].name] = stateParameters[n];
        }

        info.stateParameters = stateParameters;
    };

    function setupEnumMap(gl) {
        var enumMap = {};
        for (var n in gl) {
            enumMap[gl[n]] = n;
        }

        info.enumMap = enumMap;
    };
    setupEnumMap(glc);

    info.UIType = UIType;
    info.FunctionType = FunctionType;
    //info.functions - deferred
    //info.stateParameters - deferred

    info.enumToString = function (n) {
        var string = info.enumMap[n];
        if (string !== undefined) {
            return string;
        }
        return "0x" + n.toString(16);
    };

    info.initialize = function (gl) {
        setupFunctionInfos(gl);
        setupStateParameters(gl);
    };

    return info;
});
