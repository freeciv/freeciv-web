define([
        '../../../shared/Base',
        '../../../shared/GLConsts',
        '../../../shared/Info',
        '../../../shared/Settings',
        '../../../shared/Utilities',
        '../../Helpers',
        '../../shared/BufferPreview',
        '../../shared/SurfaceInspector',
        '../../shared/TexturePreview',
        '../../shared/TraceLine',
    ], function (
        base,
        glc,
        info,
        settings,
        util,
        helpers,
        BufferPreview,
        SurfaceInspector,
        TexturePreviewGenerator,
        traceLine
    ) {

    var TextureView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-right")[0],
            listing: elementRoot.getElementsByClassName("texture-listing")[0]
        };

        this.inspectorElements = {
            "window-texture-outer": elementRoot.getElementsByClassName("window-texture-outer")[0],
            "window-texture-inspector": elementRoot.getElementsByClassName("window-texture-inspector")[0],
            "texture-listing": elementRoot.getElementsByClassName("texture-listing")[0]
        };
        this.inspector = new SurfaceInspector(this, w, elementRoot, {
            splitterKey: 'textureSplitter',
            title: 'Texture Preview',
            selectionName: 'Face',
            selectionValues: ["POSITIVE_X", "NEGATIVE_X", "POSITIVE_Y", "NEGATIVE_Y", "POSITIVE_Z", "NEGATIVE_Z"]
        });
        this.inspector.currentTexture = null;
        this.inspector.currentVersion = null;
        this.inspector.getTargetFace = function (gl) {
            var targetFace;
            switch (this.currentTexture.type) {
                case gl.TEXTURE_CUBE_MAP:
                    targetFace = gl.TEXTURE_CUBE_MAP_POSITIVE_X + this.activeOption;
                    break;
                default:
                    targetFace = null;
                    break;
            }
            return targetFace;
        };
        this.inspector.querySize = function () {
            var gl = this.gl;
            if (!this.currentTexture || !this.currentVersion) {
                return null;
            }
            var targetFace = this.getTargetFace(gl);
            return this.currentTexture.guessSize(gl, this.currentVersion, targetFace);
        };
        this.inspector.setupPreview = function () {
            if (this.previewer) {
                return;
            }
            this.previewer = new TexturePreviewGenerator(this.canvas, false);
            this.gl = this.previewer.gl;
        };
        this.inspector.updatePreview = function () {
            var gl = this.gl;

            var targetFace = this.getTargetFace(gl);
            var size = this.currentTexture.guessSize(gl, this.currentVersion, targetFace);
            var desiredWidth = 1;
            var desiredHeight = 1;
            if (size) {
                desiredWidth = size[0];
                desiredHeight = size[1];
                this.canvas.style.display = "";
            } else {
                this.canvas.style.display = "none";
            }

            this.previewer.draw(this.currentTexture, this.currentVersion, targetFace, desiredWidth, desiredHeight);
        };
        this.inspector.setTexture = function (texture, version) {
            var gl = this.window.context;

            if (texture) {
                this.options.title = "Texture Preview: " + texture.getName();
            } else {
                this.options.title = "Texture Preview: (none)";
            }

            this.currentTexture = texture;
            this.currentVersion = version;
            this.activeOption = 0;
            this.optionsList.selectedIndex = 0;

            if (texture) {
                // Setup UI
                switch (texture.type) {
                    case gl.TEXTURE_2D:
                    case gl.TEXTURE_3D:
                    case gl.TEXTURE_2D_ARRAY:
                        this.elements.faces.style.display = "none";
                        break;
                    case gl.TEXTURE_CUBE_MAP:
                        this.elements.faces.style.display = "";
                        break;
                }
                this.updatePreview();
            } else {
                // Clear everything
                this.elements.faces.style.display = "none";
                this.canvas.width = 1;
                this.canvas.height = 1;
                this.canvas.style.display = "none";
            }

            this.reset();
            this.layout();
        };

        this.currentTexture = null;
    };

    TextureView.prototype.setInspectorWidth = function (newWidth) {
        //.window-texture-outer margin-left: -800px !important; /* -2 * window-texture-inspector.width */
        //.window-texture margin-left: 400px !important; /* window-texture-inspector.width */
        //.texture-listing right: 400px; /* window-texture-inspector */
        this.inspectorElements["window-texture-outer"].style.marginLeft = (-2 * newWidth) + "px";
        this.inspectorElements["window-texture-inspector"].style.width = newWidth + "px";
        this.inspectorElements["texture-listing"].style.right = newWidth + "px";
    };

    TextureView.prototype.layout = function () {
        this.inspector.layout();
    };

    function oneValueOneChannelCopyRect(src, srcStart, srcStride, dst, dstStride, width, height, valueFn, formatConversionInfo, preMultAlpha) {
        const {numComponents, channelMult, channelNdx, channelOffset} = formatConversionInfo;
        for (let y = 0; y < height; ++y) {
            let sn = srcStart;
            let dn = y * dstStride;
            for (let x = 0; x < width; ++x, sn += numComponents, dn += 4) {
                dst[dn + 0] = channelMult[0] * valueFn(src[sn + channelNdx[0]]) + channelOffset[0] * 255;
                dst[dn + 1] = channelMult[1] * valueFn(src[sn + channelNdx[1]]) + channelOffset[1] * 255;
                dst[dn + 2] = channelMult[2] * valueFn(src[sn + channelNdx[2]]) + channelOffset[2] * 255;
                dst[dn + 3] = channelMult[3] * valueFn(src[sn + channelNdx[3]]) + channelOffset[3] * 255;
            }
            srcStart += srcStride;
        }
    }

    function oneValueAllChannelsCopyRect(src, srcStart, srcStride, dst, dstStride, width, height, valueFn) {
        for (let y = 0; y < height; ++y) {
            let sn = srcStart;
            let dn = y * dstStride;
            for (let x = 0; x < width; ++x, ++sn, dn += 4) {
                const s = valueFn(src[sn]);
                dst[dn + 0] = s.r;
                dst[dn + 1] = s.g;
                dst[dn + 2] = s.b;
                dst[dn + 3] = s.a;
            }
            srcStart += srcStride;
        }
    }

    function FP32() {
        var floatView = new Float32Array(1);
        var uint32View = new Uint32Array(floatView.buffer);

        return {
            u: uint32View,
            f: floatView,
        };
    }

    // from https://gist.github.com/rygorous/2156668
    const fromHalf = (function() {
        const shifted_exp = 0x7c00 << 13; // exponent mask after shift
        const magic = new FP32();
        magic.u[0] = 113 << 23;
        const o = new FP32();

        return function(v) {

            o.u[0] = (v & 0x7fff) << 13;       // exponent/mantissa bits
            const exp = shifted_exp & o.u[0];  // just the exponent
            o.u[0] += (127 - 15) << 23;        // exponent adjust

            // handle exponent special cases
            if (exp === shifted_exp) {         // Inf/NaN?
                o.u[0] += (128 - 16) << 23;    // extra exp adjust
            } else if (exp == 0) {             // Zero/Denormal?
                o.u[0] += 1 << 23;             // extra exp adjust
                o.f[0] -= magic.f[0];          // renormalize
            }

            o.u[0] |= (v & 0x8000) << 16;      // sign bit
            return o.f[0];
        };
    }());

    // From OpenGL ES 3.0 spec 2.1.3
    function from11f(v) {
        const e = v >> 6;
        const m = v & 0x2F;
        if (e === 0) {
            if (m === 0) {
                return 0;
            } else {
                return Math.pow(2, -14) * (m / 64);
            }
        } else {
            if (e < 31) {
                return Math.pow(2, e - 15) * (1 + m / 64);
            } else {
                if (m === 0) {
                    return 0;  // Inf
                } else {
                    return 0;  // Nan
                }
            }
        }
    }

    // From OpenGL ES 3.0 spec 2.1.4
    function from10f(v) {
        const e = v >> 5;
        const m = v & 0x1F;
        if (e === 0) {
            if (m === 0) {
                return 0;
            } else {
                return Math.pow(2, -14) * (m / 32);
            }
        } else {
            if (e < 31) {
                return Math.pow(2, e - 15) * (1 + m / 32);
            } else {
                if (m === 0) {
                    return 0;  // Inf
                } else {
                    return 0;  // Nan
                }
            }
        }
    }

    function rgba8From565(v) {
        return {
            r: ((v >> 11) & 0x1F) * 0xFF / 0x1F | 0,
            g: ((v >>  5) & 0x3F) * 0xFF / 0x3F | 0,
            b: ((v >>  0) & 0x1F) * 0xFF / 0x1F | 0,
            a: 255,
        };
    }

    function rgba8From4444(v) {
        return {
            r: ((v >> 12) & 0xF) * 0xFF / 0xF | 0,
            g: ((v >>  8) & 0xF) * 0xFF / 0xF | 0,
            b: ((v >>  4) & 0xF) * 0xFF / 0xF | 0,
            a: ((v >>  0) & 0xF) * 0xFF / 0xF | 0,
        };
    }

    function rgba8From5551(v) {
        return {
            r: ((v >> 11) & 0x1F) * 0xFF / 0x1F | 0,
            g: ((v >>  6) & 0x1F) * 0xFF / 0x1F | 0,
            b: ((v >>  1) & 0x1F) * 0xFF / 0x1F | 0,
            a: ((v >>  0) & 0x01) * 0xFF / 0x01 | 0,
        };
    }

    function rgba8From10F11F11Frev(v) {
        return {
            r: from11f((v >>  0) & 0x3FF) * 0xFF | 0,
            g: from11f((v >> 11) & 0x3FF) * 0xFF | 0,
            b: from10f((v >> 22) & 0x1FF) * 0xFF | 0,
            a: 255,
        };
    }

    function rgba8From5999rev(v) {
        // OpenGL ES 3.0 spec 3.8.3.2
        const n = 9;  // num bits
        const b = 15; // exponent bias
        const exp = v >> 27;
        const sharedExp = Math.pow(2, exp - b - n);
        return {
            r: ((v >>  0) & 0x1FF) * sharedExp * 0xFF | 0,
            g: ((v >>  9) & 0x1FF) * sharedExp * 0xFF | 0,
            b: ((v >> 18) & 0x1FF) * sharedExp * 0xFF | 0,
            a: 255,
        };
    }

    function rgba8From2101010rev(v) {
        return {
            r: ((v >>  0) & 0x2FF) * 0xFF / 0x2FF | 0,
            g: ((v >> 10) & 0x2FF) * 0xFF / 0x2FF | 0,
            b: ((v >> 20) & 0x2FF) * 0xFF / 0x2FF | 0,
            a: ((v >> 30) &   0x3) * 0xFF /   0x3 | 0,
        };
    }

    function rgba8From248(v) {
        return {
            r: ((v >> 8) & 0xFFFFFF) * 0xFF / 0xFFFFFF | 0,
            g: ((v >> 0) &     0xFF),
            b: 0,
            a: 0,
        };
    }

    function rgba8From248rev(v) {
        return {
            r: ((v >>  0) & 0xFFFFFF) * 0xFF / 0xFFFFFF | 0,
            g: ((v >> 24) &     0xFF),
            b: 0,
            a: 0,
        };
    }

    const formatConversionInfo = {};

    formatConversionInfo[glc.ALPHA]           = { numComponents: 1, channelMult: [0, 0, 0, 1], channelNdx: [0, 0, 0, 0], channelOffset: [0, 0, 0, 0], };
    formatConversionInfo[glc.LUMINANCE]       = { numComponents: 1, channelMult: [1, 1, 1, 0], channelNdx: [0, 0, 0, 0], channelOffset: [0, 0, 0, 1], };
    formatConversionInfo[glc.LUMINANCE_ALPHA] = { numComponents: 2, channelMult: [1, 1, 1, 1], channelNdx: [0, 0, 0, 1], channelOffset: [0, 0, 0, 0], };
    formatConversionInfo[glc.RED]             = { numComponents: 1, channelMult: [1, 0, 0, 0], channelNdx: [0, 0, 0, 0], channelOffset: [0, 0, 0, 1], }; // NOTE: My experience is these are actually alpha = 0 but then you can't see them
    formatConversionInfo[glc.RED_INTEGER]     = { numComponents: 1, channelMult: [1, 0, 0, 0], channelNdx: [0, 0, 0, 0], channelOffset: [0, 0, 0, 1], }; // NOTE: My experience is these are actually alpha = 0 but then you can't see them
    formatConversionInfo[glc.RG]              = { numComponents: 2, channelMult: [1, 1, 0, 0], channelNdx: [0, 1, 0, 0], channelOffset: [0, 0, 0, 1], }; // NOTE: My experience is these are actually alpha = 0 but then you can't see them
    formatConversionInfo[glc.RG_INTEGER]      = { numComponents: 2, channelMult: [1, 1, 0, 0], channelNdx: [0, 0, 0, 0], channelOffset: [0, 0, 0, 1], }; // NOTE: My experience is these are actually alpha = 0 but then you can't see them
    formatConversionInfo[glc.RGB]             = { numComponents: 3, channelMult: [1, 1, 1, 0], channelNdx: [0, 1, 2, 0], channelOffset: [0, 0, 0, 1], };
    formatConversionInfo[glc.RGB_INTEGER]     = { numComponents: 3, channelMult: [1, 1, 1, 0], channelNdx: [0, 1, 2, 0], channelOffset: [0, 0, 0, 1], };
    formatConversionInfo[glc.RGBA]            = { numComponents: 4, channelMult: [1, 1, 1, 1], channelNdx: [0, 1, 2, 3], channelOffset: [0, 0, 0, 0], };
    formatConversionInfo[glc.RGBA_INTEGER]    = { numComponents: 4, channelMult: [1, 1, 1, 1], channelNdx: [0, 1, 2, 3], channelOffset: [0, 0, 0, 0], };
    formatConversionInfo[glc.DEPTH_COMPONENT] = { numComponents: 1, channelMult: [1, 1, 0, 0], channelNdx: [1, 1, 1, 0], channelOffset: [0, 0, 0, 1], };
    formatConversionInfo[glc.DEPTH_STENCIL]   = { numComponents: 2, channelMult: [1, 1, 0, 0], channelNdx: [0, 1, 0, 0], channelOffset: [0, 0, 0, 1], };

    const typeFormatConversionInfo = {};

    typeFormatConversionInfo[glc.BYTE]                           = { typeSize: 1, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v + 128, };
    typeFormatConversionInfo[glc.UNSIGNED_BYTE]                  = { typeSize: 1, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v, };
    typeFormatConversionInfo[glc.FLOAT]                          = { typeSize: 4, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v * 255, };
    typeFormatConversionInfo[glc.HALF_FLOAT]                     = { typeSize: 2, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => fromHalf(v) * 255, };
    typeFormatConversionInfo[glc.SHORT]                          = { typeSize: 2, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v, };
    typeFormatConversionInfo[glc.UNSIGNED_SHORT]                 = { typeSize: 2, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v, };
    typeFormatConversionInfo[glc.INT]                            = { typeSize: 4, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v, };
    typeFormatConversionInfo[glc.UNSIGNED_INT]                   = { typeSize: 4, multiChannel: false, rectFn: oneValueOneChannelCopyRect,  valueFn: v => v, };
    typeFormatConversionInfo[glc.UNSIGNED_SHORT_5_6_5]           = { typeSize: 2, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From565, };
    typeFormatConversionInfo[glc.UNSIGNED_SHORT_4_4_4_4]         = { typeSize: 2, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From4444, };
    typeFormatConversionInfo[glc.UNSIGNED_SHORT_5_5_5_1]         = { typeSize: 2, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From5551, };
    typeFormatConversionInfo[glc.UNSIGNED_INT_10F_11F_11F_REV]   = { typeSize: 4, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From10F11F11Frev, };
    typeFormatConversionInfo[glc.UNSIGNED_INT_5_9_9_9_REV]       = { typeSize: 4, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From5999rev, };
    typeFormatConversionInfo[glc.UNSIGNED_INT_2_10_10_10_REV]    = { typeSize: 4, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From2101010rev, };
    typeFormatConversionInfo[glc.UNSIGNED_INT_24_8]              = { typeSize: 4, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From248, };
    typeFormatConversionInfo[glc.FLOAT_32_UNSIGNED_INT_24_8_REV] = { typeSize: 4, multiChannel: true,  rectFn: oneValueAllChannelsCopyRect, valueFn: rgba8From248rev, };


    //// WEBGL_compressed_texture_s3tc
    //glc.COMPRESSED_RGB_S3TC_DXT1_EXT
    //glc.COMPRESSED_RGBA_S3TC_DXT1_EXT
    //glc.COMPRESSED_RGBA_S3TC_DXT3_EXT
    //glc.COMPRESSED_RGBA_S3TC_DXT5_EXT
    //
    //glc.COMPRESSED_R11_EAC
    //glc.COMPRESSED_SIGNED_R11_EAC
    //glc.COMPRESSED_RG11_EAC
    //glc.COMPRESSED_SIGNED_RG11_EAC
    //glc.COMPRESSED_RGB8_ETC2
    //glc.COMPRESSED_SRGB8_ETC2
    //glc.COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
    //glc.COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
    //glc.COMPRESSED_RGBA8_ETC2_EAC
    //glc.COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
    //
    //// WEBGL_compressed_texture_atc
    //glc.COMPRESSED_RGB_ATC_WEBGL
    //glc.COMPRESSED_RGBA_ATC_EXPLICIT_ALPHA_WEBGL
    //glc.COMPRESSED_RGBA_ATC_INTERPOLATED_ALPHA_WEBGL
    //
    //// WEBGL_compressed_texture_pvrtc
    //glc.COMPRESSED_RGB_PVRTC_4BPPV1_IMG
    //glc.COMPRESSED_RGB_PVRTC_2BPPV1_IMG
    //glc.COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
    //glc.COMPRESSED_RGBA_PVRTC_2BPPV1_IMG

    function createImageDataFromPixels(gl, pixelStoreState, width, height, depth, format, type, source) {
        var canvas = document.createElement("canvas");
        canvas.className = "gli-reset";
        var ctx = canvas.getContext("2d");
        var imageData = ctx.createImageData(width, height);

        // TODO: support depth > 1

        // TODO: support all pixel store state
        //UNPACK_ALIGNMENT
        //UNPACK_COLORSPACE_CONVERSION_WEBGL
        //UNPACK_FLIP_Y_WEBGL
        //UNPACK_PREMULTIPLY_ALPHA_WEBGL
        //UNPACK_ROW_LENGTH
        //UNPACK_SKIP_ROWS
        //UNPACK_SKIP_PIXELS
        //UNPACK_SKIP_IMAGES
        //UNPACK_IMAGE_HEIGHT


        var unpackAlignment = pixelStoreState["UNPACK_ALIGNMENT"];
        if (unpackAlignment === undefined) {
            unpackAlignment = 4;
        }

        if (pixelStoreState["UNPACK _COLORSPACE_CONVERSION_WEBGL"] !== gl.BROWSER_DEFAULT_WEBGL) {
            console.log("unsupported: UNPACK_COLORSPACE_CONVERSION_WEBGL != BROWSER_DEFAULT_WEBGL");
        }
        if (pixelStoreState["UNPACK_PREMULTIPLY_ALPHA_WEBGL"]) {
            console.log("unsupported: UNPACK_PREMULTIPLY_ALPHA_WEBGL = true");
        }

        // TODO: implement all texture formats
        const typeFormatInfo = typeFormatConversionInfo[type];
        if (!typeFormatInfo) {
            console.log("unsupported texture type:", info.enumToString(type));
            return null;
        }

        const formatInfo = formatConversionInfo[format];
        if (!formatInfo) {
            console.log("unsupported texture format:", info.enumToString(format));
            return null;
        }

        var bytesPerElement = (typeFormatInfo.multiChannel ? 1 : formatInfo.numComponents) * typeFormatInfo.typeSize;

        const srcStride = (bytesPerElement * width + unpackAlignment - 1) & (0x10000000 - unpackAlignment) / source.BYTES_PER_ELEMENT;
        if (srcStride % 1) {
            console.log("unsupported source type");
            return null;
        }

        const dstStride = width * 4;
        const preMultAlpha = pixelStoreState["UNPACK_PREMULTIPLY_ALPHA_WEBGL"];
        const flipY = pixelStoreState["UNPACK_FLIP_Y_WEBGL"];
        const srcStart = flipY ? srcStride * (height - 1) : 0;

        typeFormatInfo.rectFn(source, srcStart, flipY ? -srcStide : srcStride,
                              imageData.data, dstStride,
                              width, height,
                              typeFormatInfo.valueFn, formatInfo, preMultAlpha);

        return imageData;
    };

    function appendHistoryLine(gl, el, texture, version, call) {
        if (call.name == "pixelStorei") {
            // Don't care about these for now - maybe they will be useful in the future
            return;
        }

        traceLine.appendHistoryLine(gl, el, call);

        if ((call.name == "texImage2D") || (call.name == "texSubImage2D") ||
            (call.name == "texImage3D") || (call.name == "texSubImage3D") ||
            (call.name == "compressedTexImage2D") || (call.name == "compressedTexSubImage2D")) {
            // Gather up pixel store state between this call and the previous one
            var pixelStoreState = {};
            for (var i = version.calls.indexOf(call) - 1; i >= 0; i--) {
                var prev = version.calls[i];
                if ((prev.name == "texImage2D") || (prev.name == "texSubImage2D") ||
                    (prev.name == "texImage3D") || (prev.name == "texSubImage3D") ||
                    (prev.name == "compressedTexImage2D") || (prev.name == "compressedTexSubImage2D")) {
                    break;
                }
                var pname = info.enumMap[prev.args[0]];
                pixelStoreState[pname] = prev.args[1];
            }

            // TODO: display src of last arg (either data, img, video, etc)
            var sourceArg = null;
            for (var n = 0; n < call.args.length; n++) {
                var arg = call.args[n];
                if (arg) {
                    if ((arg instanceof HTMLCanvasElement) ||
                        (arg instanceof HTMLImageElement) ||
                        (arg instanceof HTMLVideoElement)) {
                        sourceArg = util.clone(arg);
                    } else if (base.typename(arg) == "ImageData") {
                        sourceArg = arg;
                    } else if (arg.length) {
                        // Likely an array of some kind
                        sourceArg = arg;
                    }
                }
            }

            // Fixup arrays by converting to ImageData
            if (sourceArg && sourceArg.length) {
                let width;
                let height;
                let depth = 1;
                let format;
                let type;
                if (call.name == "texImage2D") {
                    width = call.args[3];
                    height = call.args[4];
                    format = call.args[6];
                    type = call.args[7];
                } else if (call.name == "texSubImage2D") {
                    width = call.args[4];
                    height = call.args[5];
                    format = call.args[6];
                    type = call.args[7];
                } else if (call.name == "texImage3D") {
                    width = call.args[3];
                    height = call.args[4];
                    depth = call.args[5];
                    format = call.args[7];
                    type = call.args[8];
                } else if (call.name == "texSubImage3D") {
                    width = call.args[4];
                    height = call.args[5];
                    depth = call.args[6];
                    format = call.args[8];
                    type = call.args[9];
                } else if (call.name == "compressedTexImage2D") {
                    width = call.args[3];
                    height = call.args[4];
                    format = call.args[2];
                    type = format;
                } else if (call.name == "compressedTexSubImage2D") {
                    width = call.args[4];
                    height = call.args[5];
                    format = call.args[6];
                    type = format;
                }
                sourceArg = createImageDataFromPixels(gl, pixelStoreState, width, height, depth, format, type, sourceArg);
            }

            // Fixup ImageData
            if (sourceArg && base.typename(sourceArg) == "ImageData") {
                // Draw into a canvas
                var canvas = document.createElement("canvas");
                canvas.className = "gli-reset";
                canvas.width = sourceArg.width;
                canvas.height = sourceArg.height;
                var ctx = canvas.getContext("2d");
                ctx.putImageData(sourceArg, 0, 0);
                sourceArg = canvas;
            }

            if (sourceArg) {
                var dupeEl = sourceArg;

                // Grab the size before we muck with the element
                var size = [dupeEl.width, dupeEl.height];

                dupeEl.style.width = "100%";
                dupeEl.style.height = "100%";

                if (dupeEl.src) {
                    var srcEl = document.createElement("div");
                    srcEl.className = "texture-history-src";
                    srcEl.textContent = "Source: ";
                    var srcLinkEl = document.createElement("span");
                    srcLinkEl.className = "texture-history-src-link";
                    srcLinkEl.target = "_blank";
                    srcLinkEl.href = dupeEl.src;
                    srcLinkEl.textContent = dupeEl.src;
                    srcEl.appendChild(srcLinkEl);
                    el.appendChild(srcEl);
                }

                var dupeRoot = document.createElement("div");
                dupeRoot.className = "texture-history-dupe";
                dupeRoot.appendChild(dupeEl);
                el.appendChild(dupeRoot);

                // Resize on click logic
                var parentWidth = 512; // TODO: pull from parent?
                var parentHeight = Math.min(size[1], 128);
                var parentar = parentHeight / parentWidth;
                var ar = size[1] / size[0];

                var width;
                var height;
                if (ar * parentWidth < parentHeight) {
                    width = parentWidth;
                    height = (ar * parentWidth);
                } else {
                    height = parentHeight;
                    width = (parentHeight / ar);
                }
                dupeRoot.style.width = width + "px";
                dupeRoot.style.height = height + "px";

                var sizedToFit = true;
                dupeRoot.onclick = function (e) {
                    sizedToFit = !sizedToFit;
                    if (sizedToFit) {
                        dupeRoot.style.width = width + "px";
                        dupeRoot.style.height = height + "px";
                    } else {
                        dupeRoot.style.width = size[0] + "px";
                        dupeRoot.style.height = size[1] + "px";
                    }
                    e.preventDefault();
                    e.stopPropagation();
                };
            }
        }
    };

    function generateTextureHistory(gl, el, texture, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-secondary";
        titleDiv.textContent = "History";
        el.appendChild(titleDiv);

        var rootEl = document.createElement("div");
        rootEl.className = "texture-history";
        el.appendChild(rootEl);

        for (var n = 0; n < version.calls.length; n++) {
            var call = version.calls[n];
            appendHistoryLine(gl, rootEl, texture, version, call);
        }
    };

    const notEnum = undefined;
    const repeatEnums = ["REPEAT", "CLAMP_TO_EDGE", "MIRROR_REPEAT"];
    const filterEnums = ["NEAREST", "LINEAR", "NEAREST_MIPMAP_NEAREST", "LINEAR_MIPMAP_NEAREST", "NEAREST_MIPMAP_LINEAR", "LINEAR_MIPMAP_LINEAR"];
    const compareModeEnums = ["NONE", "COMPARE_REF_TO_TEXTURE"];
    const compareFuncEnums = ["LEQUAL", "GEQUAL", "LESS", "GREATER", "EQUAL", "NOTEQUAL", "ALWAYS", "NEVER"];

    const webgl1TexturePropertyInfo = {
        properties: ["TEXTURE_WRAP_S", "TEXTURE_WRAP_T", "TEXTURE_MIN_FILTER", "TEXTURE_MAG_FILTER"],
        enums: [repeatEnums, repeatEnums, filterEnums, filterEnums],
    };
    const webgl2TexturePropertyInfo = {
        properties: [
            "TEXTURE_WRAP_S",
            "TEXTURE_WRAP_T",
            "TEXTURE_WRAP_R",
            "TEXTURE_MIN_FILTER",
            "TEXTURE_MAG_FILTER",
            "TEXTURE_MIN_LOD",
            "TEXTURE_MAX_LOD",
            "TEXTURE_BASE_LEVEL",
            "TEXTURE_MAX_LEVEL",
            "TEXTURE_COMPARE_FUNC",
            "TEXTURE_COMPARE_MODE",
        ],
        enums: [
            repeatEnums,
            repeatEnums,
            repeatEnums,
            filterEnums,
            filterEnums,
            notEnum,
            notEnum,
            notEnum,
            notEnum,
            compareFuncEnums,
            compareModeEnums,
        ],
    };

    function generateTextureDisplay(gl, el, texture, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = texture.getName();
        el.appendChild(titleDiv);

        const propInfo = util.isWebGL2(gl) ? webgl2TexturePropertyInfo : webgl1TexturePropertyInfo;

        helpers.appendParameters(gl, el, texture, propInfo.properties, propInfo.enums);
        helpers.appendbr(el);

        helpers.appendSeparator(el);

        generateTextureHistory(gl, el, texture, version);
        helpers.appendbr(el);

        var frame = gl.ui.controller.currentFrame;
        if (frame) {
            helpers.appendSeparator(el);
            traceLine.generateUsageList(gl, el, frame, texture);
            helpers.appendbr(el);
        }
    };

    TextureView.prototype.setTexture = function (texture) {
        this.currentTexture = texture;

        var version = null;
        if (texture) {
            switch (this.window.activeVersion) {
                case null:
                    version = texture.currentVersion;
                    break;
                case "current":
                    var frame = this.window.controller.currentFrame;
                    if (frame) {
                        version = frame.findResourceVersion(texture);
                    }
                    version = version || texture.currentVersion; // Fallback to live
                    break;
            }
        }

        var node = this.elements.listing;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
        if (texture) {
            generateTextureDisplay(this.window.context, this.elements.listing, texture, version);
        }

        this.inspector.setTexture(texture, version);

        this.elements.listing.scrollTop = 0;
    };

    return TextureView;
});
