define([
        '../../shared/Base',
        '../../shared/GLConsts',
        '../../shared/TextureInfo',
        '../../shared/Utilities',
        '../Resource',
    ], function (
        base,
        glc,
        textureInfo,
        util,
        Resource) {

    var Texture = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 1;

        this.defaultName = "Texture " + this.id;

        this.type = gl.TEXTURE_2D; // TEXTURE_2D, TEXTURE_CUBE_MAP, TEXTURE_3D, TEXTURE_2D_ARRAY

        this.parameters = {};
        this.parameters[gl.TEXTURE_MAG_FILTER] = gl.LINEAR;
        this.parameters[gl.TEXTURE_MIN_FILTER] = gl.NEAREST_MIPMAP_LINEAR;
        this.parameters[gl.TEXTURE_WRAP_S] = gl.REPEAT;
        this.parameters[gl.TEXTURE_WRAP_T] = gl.REPEAT;

        if (util.isWebGL2(gl)) {
            this.parameters[gl.TEXTURE_BASE_LEVEL] = 0;
            this.parameters[gl.TEXTURE_COMPARE_FUNC] = gl.LEQUAL;
            this.parameters[gl.TEXTURE_COMPARE_MODE] = gl.NONE;
            this.parameters[gl.TEXTURE_MIN_LOD] = -1000;
            this.parameters[gl.TEXTURE_MAX_LOD] = 1000;
            this.parameters[gl.TEXTURE_MAX_LEVEL] = 1000;
            this.parameters[gl.TEXTURE_WRAP_R] = gl.REPEAT;
        }

        // TODO: handle TEXTURE_MAX_ANISOTROPY_EXT

        this.currentVersion.target = this.type;
        this.currentVersion.setParameters(this.parameters);

        this.estimatedSize = 0;
    };

    Texture.prototype.guessSize = function (gl, version, face) {
        version = version || this.currentVersion;
        for (var n = version.calls.length - 1; n >= 0; n--) {
            var call = version.calls[n];
            if (call.name == "texImage2D") {
                // Ignore all but level 0
                if (call.args[1]) {
                    continue;
                }
                if (face) {
                    if (call.args[0] != face) {
                        continue;
                    }
                }
                if (call.args.length == 9) {
                    return [call.args[3], call.args[4], 1];
                } else {
                    var sourceObj = call.args[5];
                    if (sourceObj) {
                        return [sourceObj.width, sourceObj.height];
                    } else {
                        return null;
                    }
                }
            } else if (call.name == "compressedTexImage2D") {
                // Ignore all but level 0
                if (call.args[1]) {
                    continue;
                }
                if (face) {
                    if (call.args[0] != face) {
                        continue;
                    }
                }
                return [call.args[3], call.args[4], 1];
            } else if (call.name == "texImage3D") {
                // Ignore all but level 0
                if (call.args[1]) {
                    continue;
                }
                return [call.args[3], call.args[4], call.args[5]];
            }
        }
        return null;
    };

    const webgl1RefreshParamEnums = [
        glc.TEXTURE_MAG_FILTER,
        glc.TEXTURE_MIN_FILTER,
        glc.TEXTURE_WRAP_S,
        glc.TEXTURE_WRAP_T,
    ];

    const webgl2RefreshParamEnums = [
        ...webgl1RefreshParamEnums,
        glc.TEXTURE_BASE_LEVEL,
        glc.TEXTURE_COMPARE_FUNC,
        glc.TEXTURE_COMPARE_MODE,
        glc.TEXTURE_MIN_LOD,
        glc.TEXTURE_MAX_LOD,
        glc.TEXTURE_MAX_LEVEL,
        glc.TEXTURE_WRAP_R,
    ];

    Texture.prototype.refresh = function (gl) {
        var paramEnums = util.isWebGL2(gl) ? webgl2RefreshParamEnums : webgl1RefreshParamEnums;
        for (var n = 0; n < paramEnums.length; n++) {
            this.parameters[paramEnums[n]] = gl.getTexParameter(this.type, paramEnums[n]);
        }
    };

    Texture.getTracked = function (gl, args) {
        const bindingEnum = textureInfo.getTargetInfo(args[0]).query;
        var gltexture = gl.rawgl.getParameter(bindingEnum);
        if (gltexture == null) {
            // Going to fail
            return null;
        }
        return gltexture.trackedObject;
    };

    Texture.setCaptures = function (gl) {
        // TODO: copyTexImage2D
        // TODO: copyTexSubImage2D

        var original_texParameterf = gl.texParameterf;
        gl.texParameterf = function (target) {
            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                tracked.parameters[arguments[1]] = arguments[2];
                tracked.markDirty(false);
                tracked.currentVersion.target = tracked.type;
                tracked.currentVersion.setParameters(tracked.parameters);
            }

            return original_texParameterf.apply(gl, arguments);
        };
        var original_texParameteri = gl.texParameteri;
        gl.texParameteri = function (target) {
            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                tracked.parameters[arguments[1]] = arguments[2];
                tracked.markDirty(false);
                tracked.currentVersion.target = tracked.type;
                tracked.currentVersion.setParameters(tracked.parameters);
            }

            return original_texParameteri.apply(gl, arguments);
        };

        function pushPixelStoreState(gl, version) {
            var pixelStoreEnums = [gl.PACK_ALIGNMENT, gl.UNPACK_ALIGNMENT, gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.UNPACK_FLIP_Y_WEBGL, gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL];
            for (var n = 0; n < pixelStoreEnums.length; n++) {
                var pixelStoreEnum = pixelStoreEnums[n];
                if (pixelStoreEnum === undefined) {
                    continue;
                }
                var value = gl.getParameter(pixelStoreEnums[n]);
                version.pushCall("pixelStorei", [pixelStoreEnum, value]);
            }
        };

        var original_texImage2D = gl.texImage2D;
        gl.texImage2D = function (target, level, internalFormat) {
            // Track texture writes
            var totalBytes = 0;
            let width;
            let height;
            const depth = 1;
            let format;
            let type;
            if (arguments.length == 9) {
                width = arguments[3];
                height = arguments[4];
                format = arguments[6];
                type = arguments[7];
            } else {
                var sourceArg = arguments[5];
                width = sourceArg.width;
                height = sourceArg.height;
                format = arguments[3];
                type = arguments[4];
            }
            totalBytes = textureInfo.calculateNumSourceBytes(width, height, depth, internalFormat, format, type);
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                const targetInfo = textureInfo.getTargetInfo(target);
                tracked.type = targetInfo.target;

                // Track total texture bytes consumed
                gl.statistics.textureBytes.value -= tracked.estimatedSize;
                gl.statistics.textureBytes.value += totalBytes;
                tracked.estimatedSize = totalBytes;

                // If !face texture this is always a reset, otherwise it may be a single face of the cube
                if (!targetInfo.face) {
                    tracked.markDirty(true);
                    tracked.currentVersion.setParameters(tracked.parameters);
                } else {
                    // Cube face - always partial
                    tracked.markDirty(false);
                }
                tracked.currentVersion.target = tracked.type;
                if (level == 0) {
                    tracked.currentVersion.setExtraParameters("format", {
                       internalFormat: internalFormat,
                       width: width,
                       height: height,
                       depth: 1,
                    });
                }

                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("texImage2D", arguments);

                // If this is an upload from something with a URL and we haven't been named yet, auto name us
                if (arguments.length == 6) {
                    var sourceArg = arguments[5];
                    if (sourceArg && sourceArg.src) {
                        if (!tracked.target.displayName) {
                            var filename = sourceArg.src;
                            var lastSlash = filename.lastIndexOf("/");
                            if (lastSlash >= 0) {
                                filename = filename.substr(lastSlash + 1);
                            }
                            var lastDot = filename.lastIndexOf(".");
                            if (lastDot >= 0) {
                                filename = filename.substr(0, lastDot);
                            }
                            tracked.setName(filename, true);
                        }
                    }
                }
            }

            return original_texImage2D.apply(gl, arguments);
        };

        var original_texSubImage2D = gl.texSubImage2D;
        gl.texSubImage2D = function (target) {
            // Track texture writes
            var totalBytes = 0;
            if (arguments.length == 9) {
                totalBytes = textureInfo.calculateNumSourceBytes(arguments[4], arguments[5], 1, undefined, arguments[6], arguments[7]);
            } else {
                var sourceArg = arguments[6];
                var width = sourceArg.width;
                var height = sourceArg.height;
                totalBytes = textureInfo.calculateNumSourceBytes(width, height, 1, undefined, arguments[4], arguments[5]);
            }
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                tracked.markDirty(false);
                tracked.currentVersion.target = tracked.type;
                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("texSubImage2D", arguments);
            }

            return original_texSubImage2D.apply(gl, arguments);
        };

        var original_texImage3D = gl.texImage3D;
        gl.texImage3D = function (target, level, internalFormat, width, height, depth, border, format, type, source, offset) {
            // Track texture writes
            var totalBytes = 0;
            totalBytes = textureInfo.calculateNumSourceBytes(width, height, depth, internalFormat, format, type);
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                const targetInfo = textureInfo.getTargetInfo(target);
                tracked.type = targetInfo.target;

                // Track total texture bytes consumed
                gl.statistics.textureBytes.value -= tracked.estimatedSize;
                gl.statistics.textureBytes.value += totalBytes;
                tracked.estimatedSize = totalBytes;

                // If !face texture this is always a reset, otherwise it may be a single face of the cube
                if (!targetInfo.face) {
                    tracked.markDirty(true);
                    tracked.currentVersion.setParameters(tracked.parameters);
                } else {
                    // Cube face - always partial
                    tracked.markDirty(false);
                }
                tracked.currentVersion.target = tracked.type;
                if (level == 0) {
                    tracked.currentVersion.setExtraParameters("format", {
                       internalFormat: internalFormat,
                       width: width,
                       height: height,
                       depth: depth,
                    });
                }

                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("texImage3D", arguments);

                // If this is an upload from something with a URL and we haven't been named yet, auto name us
                if (source instanceof HTMLCanvasElement || source instanceof HTMLImageElement) {
                    if (!tracked.target.displayName) {
                        var filename = sourceArg.src;
                        var lastSlash = filename.lastIndexOf("/");
                        if (lastSlash >= 0) {
                            filename = filename.substr(lastSlash + 1);
                        }
                        var lastDot = filename.lastIndexOf(".");
                        if (lastDot >= 0) {
                            filename = filename.substr(0, lastDot);
                        }
                        tracked.setName(filename, true);
                    }
                }
            }

            return original_texImage3D.apply(gl, arguments);
        };

        var original_texSubImage3D = gl.texSubImage3D;
        gl.texSubImage3D = function (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, source, offset) {
            // Track texture writes
            var totalBytes = 0;
            totalBytes = textureInfo.calculateNumSourceBytes(width, height, depth, undefined, format, type);
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                tracked.markDirty(false);
                tracked.currentVersion.target = tracked.type;
                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("texSubImage3D", arguments);
            }

            return original_texSubImage3D.apply(gl, arguments);
        };

        var original_compressedTexImage2D = gl.compressedTexImage2D;
        gl.compressedTexImage2D = function (target, level, internalFormat, width, height) {
            // Track texture writes
            var totalBytes = 0;
            switch (internalFormat) {
                case glc.COMPRESSED_RGB_S3TC_DXT1_EXT:
                case glc.COMPRESSED_RGBA_S3TC_DXT1_EXT:
                    totalBytes = Math.floor((width + 3) / 4) * Math.floor((height + 3) / 4) * 8;
                    break;
                case glc.COMPRESSED_RGBA_S3TC_DXT3_EXT:
                case glc.COMPRESSED_RGBA_S3TC_DXT5_EXT:
                    totalBytes = Math.floor((width + 3) / 4) * Math.floor((height + 3) / 4) * 16;
                    break;
            }
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;

                // Track total texture bytes consumed
                gl.statistics.textureBytes.value -= tracked.estimatedSize;
                gl.statistics.textureBytes.value += totalBytes;
                tracked.estimatedSize = totalBytes;

                // If a 2D texture this is always a reset, otherwise it may be a single face of the cube
                // Note that we don't reset if we are adding extra levels.
                if (arguments[1] == 0 && arguments[0] == gl.TEXTURE_2D) {
                    tracked.markDirty(true);
                    tracked.currentVersion.setParameters(tracked.parameters);
                } else {
                    // Cube face - always partial
                    tracked.markDirty(false);
                }
                tracked.currentVersion.target = tracked.type;

                if (level == 0) {
                    tracked.currentVersion.setExtraParameters("format", {
                       internalFormat: internalFormat,
                       width: width,
                       height: height,
                       depth: 1,
                    });
                }

                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("compressedTexImage2D", arguments);
            }

            return original_compressedTexImage2D.apply(gl, arguments);
        };

        var original_compressedTexSubImage2D = gl.compressedTexSubImage2D;
        gl.compressedTexSubImage2D = function (target) {
            // Track texture writes
            var totalBytes = 0;
            switch (arguments[2]) {
                case glc.COMPRESSED_RGB_S3TC_DXT1_EXT:
                case glc.COMPRESSED_RGBA_S3TC_DXT1_EXT:
                    totalBytes = Math.floor((arguments[4] + 3) / 4) * Math.floor((arguments[5] + 3) / 4) * 8;
                    break;
                case glc.COMPRESSED_RGBA_S3TC_DXT3_EXT:
                case glc.COMPRESSED_RGBA_S3TC_DXT5_EXT:
                    totalBytes = Math.floor((arguments[4] + 3) / 4) * Math.floor((arguments[5] + 3) / 4) * 16;
                    break;
            }
            gl.statistics.textureWrites.value += totalBytes;

            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                tracked.markDirty(false);
                tracked.currentVersion.target = tracked.type;
                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("compressedTexSubImage2D", arguments);
            }

            return original_compressedTexSubImage2D.apply(gl, arguments);
        };

        var original_generateMipmap = gl.generateMipmap;
        gl.generateMipmap = function (target) {
            var tracked = Texture.getTracked(gl, arguments);
            if (tracked) {
                tracked.type = textureInfo.getTargetInfo(target).target;
                // TODO: figure out what to do with mipmaps
                pushPixelStoreState(gl.rawgl, tracked.currentVersion);
                tracked.currentVersion.pushCall("generateMipmap", arguments);
            }

            return original_generateMipmap.apply(gl, arguments);
        };

        var original_readPixels = gl.readPixels;
        gl.readPixels = function () {
            var result = original_readPixels.apply(gl, arguments);
            if (result) {
                // Track texture reads
                // NOTE: only RGBA is supported for reads
                var totalBytes = arguments[2] * arguments[3] * 4;
                gl.statistics.textureReads.value += totalBytes;
            }
            return result;
        };
    };

    // If a face is supplied the texture created will be a 2D texture containing only the given face
    Texture.prototype.createTarget = function (gl, version, options, face) {
        options = options || {};
        var target = version.target;
        if (face) {
            target = gl.TEXTURE_2D;
        }

        var texture = gl.createTexture();
        gl.bindTexture(target, texture);

        for (var n in version.parameters) {
            gl.texParameteri(target, parseInt(n), version.parameters[n]);
        }

        this.replayCalls(gl, version, texture, function (call, args) {
            // Filter uploads if requested
            if (options.ignoreTextureUploads) {
                if ((call.name === "texImage2D") ||
                    (call.name === "texImage3D") ||
                    (call.name === "texSubImage2D") ||
                    (call.name === "texSubImage3D") ||
                    (call.name === "compressedTexImage2D") ||
                    (call.name === "compressedTexSubImage2D") ||
                    (call.name === "generateMipmap")) {
                    return false;
                }
            }

            // Filter non-face calls and rewrite the target if this is a face-specific call
            if ((call.name == "texImage2D") ||
                (call.name == "texSubImage2D") ||
                (call.name == "compressedTexImage2D") ||
                (call.name == "compressedTexSubImage2D")) {
                if (face && (args.length > 0)) {
                    if (args[0] != face) {
                        return false;
                    }
                    args[0] = gl.TEXTURE_2D;
                }
            } else if (call.name == "generateMipmap") {
                args[0] = target;
            }

            return true;
        });

        return texture;
    };

    Texture.prototype.deleteTarget = function (gl, target) {
        gl.deleteTexture(target);
    };

    return Texture;

});
