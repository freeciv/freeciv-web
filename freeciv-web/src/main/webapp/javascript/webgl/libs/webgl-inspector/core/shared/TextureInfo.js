define([
        './GLConsts',
        './Utilities',
    ], function (
        glc,
        util
    ) {

    const texTargetInfo = {}
    texTargetInfo[glc.TEXTURE_2D]                  = { target: glc.TEXTURE_2D,       query: glc.TEXTURE_BINDING_2D, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP]            = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, };
    texTargetInfo[glc.TEXTURE_3D]                  = { target: glc.TEXTURE_3D,       query: glc.TEXTURE_BINDING_3D, };
    texTargetInfo[glc.TEXTURE_2D_ARRAY]            = { target: glc.TEXTURE_2D_ARRAY, query: glc.TEXTURE_BINDING_2D_ARRAY, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_POSITIVE_X] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_NEGATIVE_X] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_POSITIVE_Y] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_NEGATIVE_Y] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_POSITIVE_Z] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };
    texTargetInfo[glc.TEXTURE_CUBE_MAP_NEGATIVE_Z] = { target: glc.TEXTURE_CUBE_MAP, query: glc.TEXTURE_BINDING_CUBE_MAP, face: true, };

    const defaultTargetInfo = texTargetInfo[glc.TEXTURE_2D];

    // Given a target for gl.bindTexture or gl.texXXX return a texTargetInfo above
    function getTargetInfo(target) {
      return texTargetInfo[target] || defaultTargetInfo;
    }


    const formatTypeInfo = {};
    const textureInternalFormatInfo = {};
    {
        const t = textureInternalFormatInfo;
        // unsized formats                                                             can render to it        can filter
        t[glc.ALPHA]              = { colorType: "0-1",   format: glc.ALPHA,           colorRenderable: true,  textureFilterable: true,  bytesPerElement: [1, 2, 4],        type: [glc.UNSIGNED_BYTE, glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.LUMINANCE]          = { colorType: "0-1",   format: glc.LUMINANCE,       colorRenderable: true,  textureFilterable: true,  bytesPerElement: [1, 2, 4],        type: [glc.UNSIGNED_BYTE, glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.LUMINANCE_ALPHA]    = { colorType: "0-1",   format: glc.LUMINANCE_ALPHA, colorRenderable: true,  textureFilterable: true,  bytesPerElement: [2, 4, 8],        type: [glc.UNSIGNED_BYTE, glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RGB]                = { colorType: "0-1",   format: glc.RGB,             colorRenderable: true,  textureFilterable: true,  bytesPerElement: [3, 6, 12, 2],    type: [glc.UNSIGNED_BYTE, glc.HALF_FLOAT, glc.FLOAT, glc.UNSIGNED_SHORT_5_6_5], };
        t[glc.RGBA]               = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement: [4, 8, 16, 2, 2], type: [glc.UNSIGNED_BYTE, glc.HALF_FLOAT, glc.FLOAT, glc.UNSIGNED_SHORT_4_4_4_4, glc.UNSIGNED_SHORT_5_5_5_1], };

        // sized formats
        t[glc.R8]                 = { colorType: "0-1",   format: glc.RED,             colorRenderable: true,  textureFilterable: true,  bytesPerElement:  1,         type: glc.UNSIGNED_BYTE, };
        t[glc.R8_SNORM]           = { colorType: "norm",  format: glc.RED,             colorRenderable: false, textureFilterable: true,  bytesPerElement:  1,         type: glc.BYTE, };
        t[glc.R16F]               = { colorType: "float", format: glc.RED,             colorRenderable: false, textureFilterable: true,  bytesPerElement: [2, 4],     type: [glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.R32F]               = { colorType: "float", format: glc.RED,             colorRenderable: false, textureFilterable: false, bytesPerElement:  4,         type: glc.FLOAT, };
        t[glc.R8UI]               = { colorType: "uint",  format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  1,         type: glc.UNSIGNED_BYTE, };
        t[glc.R8I]                = { colorType: "int",   format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  1,         type: glc.BYTE, };
        t[glc.R16UI]              = { colorType: "uint",  format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  2,         type: glc.UNSIGNED_SHORT, };
        t[glc.R16I]               = { colorType: "int",   format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  2,         type: glc.SHORT, };
        t[glc.R32UI]              = { colorType: "uint",  format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_INT, };
        t[glc.R32I]               = { colorType: "int",   format: glc.RED_INTEGER,     colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.INT, };
        t[glc.RG8]                = { colorType: "0-1",   format: glc.RG,              colorRenderable: true,  textureFilterable: true,  bytesPerElement:  2,         type: glc.UNSIGNED_BYTE, };
        t[glc.RG8_SNORM]          = { colorType: "norm",  format: glc.RG,              colorRenderable: false, textureFilterable: true,  bytesPerElement:  2,         type: glc.BYTE, };
        t[glc.RG16F]              = { colorType: "float", format: glc.RG,              colorRenderable: false, textureFilterable: true,  bytesPerElement: [4, 8],     type: [glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RG32F]              = { colorType: "float", format: glc.RG,              colorRenderable: false, textureFilterable: false, bytesPerElement:  8,         type: glc.FLOAT, };
        t[glc.RG8UI]              = { colorType: "uint",  format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  2,         type: glc.UNSIGNED_BYTE, };
        t[glc.RG8I]               = { colorType: "int",   format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  2,         type: glc.BYTE, };
        t[glc.RG16UI]             = { colorType: "uint",  format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_SHORT, };
        t[glc.RG16I]              = { colorType: "int",   format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.SHORT, };
        t[glc.RG32UI]             = { colorType: "uint",  format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  8,         type: glc.UNSIGNED_INT, };
        t[glc.RG32I]              = { colorType: "int",   format: glc.RG_INTEGER,      colorRenderable: true,  textureFilterable: false, bytesPerElement:  8,         type: glc.INT, };
        t[glc.RGB8]               = { colorType: "0-1",   format: glc.RGB,             colorRenderable: true,  textureFilterable: true,  bytesPerElement:  3,         type: glc.UNSIGNED_BYTE, };
        t[glc.SRGB8]              = { colorType: "0-1",   format: glc.RGB,             colorRenderable: false, textureFilterable: true,  bytesPerElement:  3,         type: glc.UNSIGNED_BYTE, };
        t[glc.RGB565]             = { colorType: "0-1",   format: glc.RGB,             colorRenderable: true,  textureFilterable: true,  bytesPerElement: [3, 2],     type: [glc.UNSIGNED_BYTE, glc.UNSIGNED_SHORT_5_6_5], };
        t[glc.RGB8_SNORM]         = { colorType: "norm",  format: glc.RGB,             colorRenderable: false, textureFilterable: true,  bytesPerElement:  3,         type: glc.BYTE, };
        t[glc.R11F_G11F_B10F]     = { colorType: "float", format: glc.RGB,             colorRenderable: false, textureFilterable: true,  bytesPerElement: [4, 6, 12], type: [glc.UNSIGNED_INT_10F_11F_11F_REV, glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RGB9_E5]            = { colorType: "float", format: glc.RGB,             colorRenderable: false, textureFilterable: true,  bytesPerElement: [4, 6, 12], type: [glc.UNSIGNED_INT_5_9_9_9_REV, glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RGB16F]             = { colorType: "float", format: glc.RGB,             colorRenderable: false, textureFilterable: true,  bytesPerElement: [6, 12],    type: [glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RGB32F]             = { colorType: "float", format: glc.RGB,             colorRenderable: false, textureFilterable: false, bytesPerElement: 12,         type: glc.FLOAT, };
        t[glc.RGB8UI]             = { colorType: "uint",  format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement:  3,         type: glc.UNSIGNED_BYTE, };
        t[glc.RGB8I]              = { colorType: "int",   format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement:  3,         type: glc.BYTE, };
        t[glc.RGB16UI]            = { colorType: "uint",  format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement:  6,         type: glc.UNSIGNED_SHORT, };
        t[glc.RGB16I]             = { colorType: "int",   format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement:  6,         type: glc.SHORT, };
        t[glc.RGB32UI]            = { colorType: "uint",  format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement: 12,         type: glc.UNSIGNED_INT, };
        t[glc.RGB32I]             = { colorType: "int",   format: glc.RGB_INTEGER,     colorRenderable: false, textureFilterable: false, bytesPerElement: 12,         type: glc.INT, };
        t[glc.RGBA8]              = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement:  4,         type: glc.UNSIGNED_BYTE, };
        t[glc.SRGB8_ALPHA8]       = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement:  4,         type: glc.UNSIGNED_BYTE, };
        t[glc.RGBA8_SNORM]        = { colorType: "norm",  format: glc.RGBA,            colorRenderable: false, textureFilterable: true,  bytesPerElement:  4,         type: glc.BYTE, };
        t[glc.RGB5_A1]            = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement: [4, 2, 4],  type: [glc.UNSIGNED_BYTE, glc.UNSIGNED_SHORT_5_5_5_1, glc.UNSIGNED_INT_2_10_10_10_REV], };
        t[glc.RGBA4]              = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement: [4, 2],     type: [glc.UNSIGNED_BYTE, glc.UNSIGNED_SHORT_4_4_4_4], };
        t[glc.RGB10_A2]           = { colorType: "0-1",   format: glc.RGBA,            colorRenderable: true,  textureFilterable: true,  bytesPerElement:  4,         type: glc.UNSIGNED_INT_2_10_10_10_REV, };
        t[glc.RGBA16F]            = { colorType: "float", format: glc.RGBA,            colorRenderable: false, textureFilterable: true,  bytesPerElement: [8, 16],    type: [glc.HALF_FLOAT, glc.FLOAT], };
        t[glc.RGBA32F]            = { colorType: "float", format: glc.RGBA,            colorRenderable: false, textureFilterable: false, bytesPerElement: 16,         type: glc.FLOAT, };
        t[glc.RGBA8UI]            = { colorType: "uint",  format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_BYTE, };
        t[glc.RGBA8I]             = { colorType: "int",   format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.BYTE, };
        t[glc.RGB10_A2UI]         = { colorType: "uint",  format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_INT_2_10_10_10_REV, };
        t[glc.RGBA16UI]           = { colorType: "uint",  format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement:  8,         type: glc.UNSIGNED_SHORT, };
        t[glc.RGBA16I]            = { colorType: "int",   format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement:  8,         type: glc.SHORT, };
        t[glc.RGBA32I]            = { colorType: "int",   format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement: 16,         type: glc.INT, };
        t[glc.RGBA32UI]           = { colorType: "uint",  format: glc.RGBA_INTEGER,    colorRenderable: true,  textureFilterable: false, bytesPerElement: 16,         type: glc.UNSIGNED_INT, };
        // Sized Internal FormatFormat	Type	Depth Bits	Stencil Bits
        t[glc.DEPTH_COMPONENT16]  = { colorType: "0-1",   format: glc.DEPTH_COMPONENT, colorRenderable: true,  textureFilterable: false, bytesPerElement: [2, 4],     type: [glc.UNSIGNED_SHORT, glc.UNSIGNED_INT], };
        t[glc.DEPTH_COMPONENT24]  = { colorType: "0-1",   format: glc.DEPTH_COMPONENT, colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_INT, };
        t[glc.DEPTH_COMPONENT32F] = { colorType: "0-1",   format: glc.DEPTH_COMPONENT, colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.FLOAT, };
        t[glc.DEPTH24_STENCIL8]   = { colorType: "0-1",   format: glc.DEPTH_STENCIL,   colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.UNSIGNED_INT_24_8, };
        t[glc.DEPTH32F_STENCIL8]  = { colorType: "0-1",   format: glc.DEPTH_STENCIL,   colorRenderable: true,  textureFilterable: false, bytesPerElement:  4,         type: glc.FLOAT_32_UNSIGNED_INT_24_8_REV, };

        Object.keys(t).forEach(function(internalFormat) {
            const info = t[internalFormat];
            info.bytesPerElementMap = {};

            const formatToTypeMap = formatTypeInfo[info.format] || {};
            formatTypeInfo[info.format] = formatToTypeMap;

            if (Array.isArray(info.bytesPerElement)) {
                info.bytesPerElement.forEach(function(bytesPerElement, ndx) {
                    const type = info.type[ndx];
                    info.bytesPerElementMap[type] = bytesPerElement;
                    formatToTypeMap[type] = bytesPerElement;
                });
            } else {
                const type = info.type;
                info.bytesPerElementMap[type] = info.bytesPerElement;
                formatToTypeMap[type] = info.bytesPerElement;
            }
        });
    }

    function getInternalFormatInfo(internalFormat) {
       return textureInternalFormatInfo[internalFormat];
    }

    function calculateNumSourceBytes(width, height, depth, internalFormat, format, type) {
        const formatToTypeMap = formatTypeInfo[format];
        const bytesPerElement = formatToTypeMap[type];
        return width * height * depth * bytesPerElement;
    }

    return {
        calculateNumSourceBytes: calculateNumSourceBytes,
        getInternalFormatInfo: getInternalFormatInfo,
        getTargetInfo: getTargetInfo,
    };

});
