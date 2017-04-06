define([
        '../../host/CaptureContext',
        '../../shared/GLConsts',
        '../../shared/Info',
        '../../shared/Settings',
        '../../shared/ShaderUtils',
        '../../shared/TextureInfo',
        '../../shared/Utilities',
        '../../host/Resource',
    ], function (
        captureContext,
        glc,
        info,
        settings,
        shaderUtils,
        textureInfo,
        util,
        Resource
    ) {

    function isSampler3D(samplerType) {
        const is3D = samplerType === "sampler3D" || samplerType === "sampler2DArray";
        return is3D;
    }

    function getMinMaxFragmentShaderSnippet(gl, samplerType, vecType) {
        if (isSampler3D(samplerType)) {
            return `
                ivec3 size = textureSize(u_sampler0, 0);
                ${vecType} minColor = texelFetch(u_sampler0, ivec3(0), 0);
                ${vecType} maxColor = minColor;
                for (int z = 0; z < size.z; ++z) {
                  for (int y = 0; y < size.y; ++y) {
                    for (int x = 0; x < size.x; ++x) {
                      ${vecType} color = texelFetch(u_sampler0, ivec3(x, y, z), 0);
                      minColor = min(minColor, color);
                      maxColor = max(maxColor, color);
                    }
                  }
                }
            `;
        } else {
            return `
                ivec2 size = textureSize(u_sampler0, 0);
                ${vecType} minColor = texelFetch(u_sampler0, ivec2(0), 0);
                ${vecType} maxColor = minColor;
                for (int y = 0; y < size.y; ++y) {
                  for (int x = 0; x < size.x; ++x) {
                    ${vecType} color = texelFetch(u_sampler0, ivec2(x, y), 0);
                    minColor = min(minColor, color);
                    maxColor = max(maxColor, color);
                  }
                }
            `;
        }
    }

    function getInOutSnippets(gl, samplerType, vecType) {
        // is this hacky? should we use a table?
        const isFloatType = samplerType[0] === 's';
        if (isFloatType) {
            return {
                outputDefinition: `
                    layout(location = 0) out highp uvec4 outMinColor;
                    layout(location = 1) out highp uvec4 outMaxColor;
                `,
                output: `
                    outMinColor = floatBitsToUint(${vecType}(minColor.rgb, 0));
                    outMaxColor = floatBitsToUint(${vecType}(maxColor.rgb, 1));
                `,
                inputDefinition: `
                    uniform highp usampler2D u_min;
                    uniform highp usampler2D u_max;
                `,
                input: `
                    vec4 minColor = uintBitsToFloat(texelFetch(u_min, ivec2(0), 0));
                    vec4 maxColor = uintBitsToFloat(texelFetch(u_max, ivec2(0), 0));
                `,
            };
        } else {
            return {
                outputDefinition: `
                    layout(location = 0) out highp ${vecType} outMinColor;
                    layout(location = 1) out highp ${vecType} outMaxColor;
                `,
                output: `
                    outMinColor = ${vecType}(minColor.rgb, 0);
                    outMaxColor = ${vecType}(maxColor.rgb, 1);
                `,
                inputDefinition: `
                    uniform highp ${samplerType} u_min;
                    uniform highp ${samplerType} u_max;
                `,
                input: `
                    vec4 minColor = vec4(texelFetch(u_min, ivec2(0), 0));
                    vec4 maxColor = vec4(texelFetch(u_max, ivec2(0), 0));
                `,
            }
        }
    }

    // This function is currently only used in WebGL2
    function generateMinMaxFragmentShader(gl, samplerType, vecType) {
        const loopSnippet = getMinMaxFragmentShaderSnippet(gl, samplerType, vecType);
        const inOutSnippets = getInOutSnippets(gl, samplerType, vecType);
        return `#version 300 es
            precision mediump float;

            uniform mediump ${samplerType} u_sampler0;

            in vec2 v_uv;

            ${inOutSnippets.outputDefinition}

            void main() {
                ${loopSnippet}

                // TODO: decide what to do about unused channels. If the texture is like R16F
                // only red has valid values. That means GBA's zero value affect this answer
                // conversely normalizing each channel separately is also pretty horrible.

                // Also what to do about solid colors. If min/max are equal.

                minColor = ${vecType}(min(min(min(minColor.r, minColor.g), minColor.b), minColor.a));
                maxColor = ${vecType}(max(max(max(maxColor.r, maxColor.g), maxColor.b), maxColor.a));

                ${inOutSnippets.output}
            }
        `;
    }

    function generateColorFragmentShader(gl, samplerType, vecType) {
        if (!util.isWebGL2(gl)) {
            return `
                precision mediump float;
                uniform mediump sampler2D u_sampler0;
                uniform sampler2D u_min;
                uniform sampler2D u_max;

                varying vec2 v_uv;

                void main() {
                    vec4 minColor = texture2D(u_min, vec2(.5));
                    vec4 maxColor = texture2D(u_max, vec2(.5));
                    gl_FragColor = (vec4(texture2D(u_sampler0, v_uv)) - minColor) / (maxColor - minColor);
                }
            `;
        }

        const inOutSnippets = getInOutSnippets(gl, samplerType, vecType);
        if (samplerType.endsWith("sampler2D")) {
            return `#version 300 es
                precision mediump float;
                uniform mediump ${samplerType} u_sampler0;

                ${inOutSnippets.inputDefinition}

                in vec2 v_uv;
                out vec4 outColor;

                void main() {
                    ${inOutSnippets.input}

                    outColor = (vec4(texture(u_sampler0, v_uv)) - minColor) / (maxColor - minColor);
                    outColor.a = 1.;  // TODO: decide what to do about alpha
                }
            `;
        } else if (samplerType.endsWith("sampler2DArray")) {
            return `#version 300 es
                precision mediump float;
                uniform mediump ${samplerType} u_sampler0;

                ${inOutSnippets.inputDefinition}

                in vec2 v_uv;
                out vec4 outColor;

                void main() {
                    // show every slice squeezed into whatever size quad
                    // the preview is drawn to
                    vec3 size = vec3(textureSize(u_sampler0, 0));
                    float slicesAcross = floor(sqrt(size.z));
                    float sliceUnit = 1. / slicesAcross;
                    vec2 sliceST = floor(v_uv / sliceUnit);
                    float slice = sliceST.x + sliceST.y * slicesAcross;
                    vec3 uvw = vec3(mod(v_uv, sliceUnit) / sliceUnit, slice);

                    ${inOutSnippets.input}

                    outColor = (vec4(texture(u_sampler0, uvw)) - minColor) / (maxColor - minColor);
                    outColor.a = 1.;  // TODO: decide what to do about alpha
                }
            `;
        } else if (samplerType.endsWith("sampler3D")) {
            return `#version 300 es
                precision mediump float;
                uniform mediump ${samplerType} u_sampler0;

                ${inOutSnippets.inputDefinition}

                in vec2 v_uv;
                out vec4 outColor;

                void main() {
                    // show every slice squeezed into whatever size quad
                    // the preview is drawn to
                    vec3 size = vec3(textureSize(u_sampler0, 0));
                    float slicesAcross = floor(sqrt(size.z));
                    float sliceUnit = 1. / slicesAcross;
                    vec2 sliceST = floor(v_uv / sliceUnit);
                    float slice = (sliceST.x + sliceST.y * slicesAcross) / size.z;
                    vec3 uvw = vec3(mod(v_uv, sliceUnit) / sliceUnit, slice);

                    ${inOutSnippets.input}

                    outColor = (vec4(texture(u_sampler0, uvw)) - minColor) / (maxColor - minColor);
                    outColor.a = 1.;  // TODO: decide what to do about alpha
                }
            `;
        } else {
            return `#version 300 es
                precision mediump float;
                uniform mediump ${samplerType} u_sampler0;

                ${inOutSnippets.inputDefinition}

                in vec2 v_uv;
                out vec4 outColor;

                void main() {
                    ${inOutSnippets.input}

                    outColor = (vec4(texture(u_sampler0, v_uv)) - minColor) / (maxColor - minColor);
                    outColor.a = 1.;  // TODO: decide what to do about alpha
                }
            `;
        }
    }

    function generateUnitQuadVertexShader(gl, samplerType, vecType) {
        if (!util.isWebGL2(gl)) {
            return `
                attribute vec4 a_position;
                varying vec2 v_uv;
                void main() {
                    gl_Position = a_position;
                    v_uv = a_position.xy * vec2(1,-1) * .5 + .5;
                }
            `;
        } else {
            return `#version 300 es
                in vec4 a_position;
                out vec2 v_uv;
                void main() {
                    gl_Position = a_position;
                    v_uv = a_position.xy * vec2(1,-1) * .5 + .5;
                }
            `;
        }
    }

    const sharedAttributeNames = ["a_position"];

    function create1x1PixelTexture(gl, target, internalFormat, format, type, data) {
        const tex = gl.createTexture();
        gl.bindTexture(target, tex);
        if (target === gl.TEXTURE_2D_ARRAY || target === gl.TEXTURE_3D) {
            gl.texImage3D(target, 0, internalFormat, 1, 1, 1, 0, format, type, data);
            gl.texParameteri(target, gl.TEXTURE_WRAP_R, gl.CLAMP_TO_EDGE);
        } else {
            gl.texImage2D(target, 0, internalFormat, 1, 1, 0, format, type, data);
        }
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        return tex;
    }

    var TexturePreviewGenerator = function (canvas, useMirror) {
        this.useMirror = useMirror;
        if (canvas) {
            // Re-use the canvas passed in
        } else {
            // Create a canvas for previewing
            canvas = document.createElement("canvas");
            canvas.className = "gli-reset";

            // HACK: this gets things working in firefox
            var frag = document.createDocumentFragment();
            frag.appendChild(canvas);
        }
        this.canvas = canvas;

        var gl = this.gl = util.getWebGLContext(canvas);
        this.programInfos = {};
        this.normMinMaxTextures = {};

        // Initialize buffer
        var vertices = new Float32Array([
            -1, -1,
             1, -1,
            -1,  1,
            -1,  1,
             1, -1,
             1,  1,
        ]);
        var buffer = this.buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        this.zeroTex = create1x1PixelTexture(gl, gl.TEXTURE_2D, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([0, 0, 0, 0]));
        this.oneTex = create1x1PixelTexture(gl, gl.TEXTURE_2D, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([255, 255, 255, 255]));
        this.point5Tex = create1x1PixelTexture(gl, gl.TEXTURE_2D, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([127, 127, 127, 127]));
    };

    TexturePreviewGenerator.prototype.dispose = function() {
        var gl = this.gl;

        Object.keys(this.programInfos).forEach(programInfo => {
            gl.deleteProgram(programInfo.program);
        });
        this.programInfos = null;

        ["zeroTex", "oneTex", "point5Tex"].forEach(name => {
            if (this[name]) {
                gl.deleteTexture(this[name]);
                this[name] = null;
            }
        });

        Object.keys(this.normMinMaxTextures).forEach(name => {
            const normMinMax = this.normMinMaxTextures[name];
            gl.deleteFramebuffer(normMinMax.framebuffer);
            gl.deleteTexture(normMinMax.minTex);
            gl.deleteTexture(normMinMax.maxTex);
        });

        gl.deleteBuffer(this.buffer);
        this.buffer = null;

        this.gl = null;
        this.canvas = null;
    };

    TexturePreviewGenerator.prototype.getMinMaxTextures = function (samplerType, vecType) {
        const gl = this.gl;
        const minMaxName = `${samplerType}-${vecType}`;
        if (!this.normMinMaxTextures[minMaxName]) {
            let internalFormat;
            let format;
            let type;
            switch (samplerType) {
                case "sampler2D":
                case "sampler2DArray":
                    internalFormat = gl.RGBA32UI;
                    format = gl.RGBA_INTEGER;
                    type = gl.UNSIGNED_INT;
                    break;
                case "isampler2D":
                case "isampler2DArray":
                    internalFormat = gl.RGBA32I;
                    format = gl.RGBA_INTEGER;
                    type = gl.INT;
                    break;
                case "usampler2D":
                case "usampler2DArray":
                    internalFormat = gl.RGBA32UI;
                    format = gl.RGBA_INTEGER;
                    type = gl.UNSIGNED_INT;
                    break;
            }
            // These are created on demand because they will fail if not WebGL2
            const minTex = create1x1PixelTexture(gl, gl.TEXTURE_2D, internalFormat, format, type, null);
            const maxTex = create1x1PixelTexture(gl, gl.TEXTURE_2D, internalFormat, format, type, null);
            const minMaxFramebuffer = gl.createFramebuffer();
            gl.bindFramebuffer(gl.FRAMEBUFFER, minMaxFramebuffer);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, minTex, 0);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT1, gl.TEXTURE_2D, maxTex, 0);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            this.normMinMaxTextures[minMaxName] = {
                minTex: minTex,
                maxTex: maxTex,
                framebuffer: minMaxFramebuffer,
            };
        }
        return this.normMinMaxTextures[minMaxName];
    };

    function getSamplerTypeForTarget(target, prefix) {
        let samplerType;
        switch (target) {
        case glc.TEXTURE_2D:
            samplerType = `${prefix}sampler2D`;
            break;
        case glc.TEXTURE_CUBE_MAP:
            samplerType = `${prefix}samplerCube`;
            break;
        case glc.TEXTURE_3D:
            samplerType = `${prefix}sampler3D`;
            break;
        case glc.TEXTURE_2D_ARRAY:
            samplerType = `${prefix}sampler2DArray`;
            break;
        }
        return samplerType;
    }

    TexturePreviewGenerator.prototype.draw = function (texture, version, targetFace, desiredWidth, desiredHeight) {
        var gl = this.gl;

        if ((this.canvas.width != desiredWidth) || (this.canvas.height != desiredHeight)) {
            this.canvas.width = desiredWidth;
            this.canvas.height = desiredHeight;
        }

        if (texture && version) {
            let samplerType = "sampler2D";
            let vecType = "vec4";
            let normalize = false;
            let target = version.target;
            let normalizeTextures = [this.zeroTex, this.oneTex];
            const internalFormatInfo = textureInfo.getInternalFormatInfo(version.extras.format.internalFormat);
            if (internalFormatInfo) {
                switch (internalFormatInfo.colorType) {
                    case "0-1":
                        samplerType = getSamplerTypeForTarget(target, "");
                        vecType = "vec4";
                        normalize = false;
                        normalizeTextures = {
                            minTex: this.zeroTex,
                            maxTex: this.oneTex,
                        };
                        break;
                    case "norm":
                        samplerType = getSamplerTypeForTarget(target, "");
                        vecType = "vec4";
                        normalize = false;
                        normalizeTextures = {
                            minTex: this.point5Tex,
                            maxTex: this.point5Tex,
                        };
                        break;
                    case "float":
                        samplerType = getSamplerTypeForTarget(target, "");
                        vecType = "vec4";
                        normalize = true;
                        normalizeTextures = this.getMinMaxTextures(samplerType, vecType);
                        break;
                    case "int":
                        samplerType = getSamplerTypeForTarget(target, "i");
                        vecType = "ivec4";
                        normalize = true;
                        normalizeTextures = this.getMinMaxTextures(samplerType, vecType);
                        break;
                    case "uint":
                        samplerType = getSamplerTypeForTarget(target, "u");
                        vecType = "uvec4";
                        normalize = true;
                        normalizeTextures = this.getMinMaxTextures(samplerType, vecType);
                        break;
                }
            }

            const a_position = 0;

            gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);

            gl.enableVertexAttribArray(a_position);
            gl.vertexAttribPointer(a_position, 2, gl.FLOAT, false, 0, 0);

            var gltex;
            if (this.useMirror) {
                gltex = texture.mirror.target;
            } else {
                gltex = texture.createTarget(gl, version, null, targetFace);
            }

            gl.disable(gl.CULL_FACE);
            gl.disable(gl.DEPTH_TEST);

            if (normalize) {
                gl.bindFramebuffer(gl.FRAMEBUFFER, normalizeTextures.framebuffer);
                gl.viewport(0, 0, 1, 1);
                gl.disable(gl.BLEND);

                const normShaderName = `${samplerType}-${vecType}-norm`;
                if (!this.programInfos[normShaderName]) {
                    this.programInfos[normShaderName] = shaderUtils.createProgramInfo(
                        gl, [
                            generateUnitQuadVertexShader(gl, samplerType, vecType),
                            generateMinMaxFragmentShader(gl, samplerType, vecType),
                        ], sharedAttributeNames);
                }

                const normProgramInfo = this.programInfos[normShaderName];
                gl.useProgram(normProgramInfo.program);

                shaderUtils.setUniforms(gl, normProgramInfo, {
                    u_sampler0: gltex,
                });

                gl.drawBuffers([gl.COLOR_ATTACHMENT0, gl.COLOR_ATTACHMENT1]);
                gl.drawArrays(gl.TRIANGLES, 0, 6);
                gl.drawBuffers([]);
            }

            const shaderName = `${samplerType}-${vecType}`;
            if (!this.programInfos[shaderName]) {
                this.programInfos[shaderName] = shaderUtils.createProgramInfo(
                    gl, [
                        generateUnitQuadVertexShader(gl, samplerType, vecType),
                        generateColorFragmentShader(gl, samplerType, vecType),
                    ], sharedAttributeNames);
            }

            const programInfo = this.programInfos[shaderName];
            gl.useProgram(programInfo.program);

            shaderUtils.setUniforms(gl, programInfo, {
                u_sampler0: gltex,
                u_min: normalizeTextures.minTex,
                u_max: normalizeTextures.maxTex,
            });

            gl.bindFramebuffer(gl.FRAMEBUFFER, null);

            gl.viewport(0, 0, this.canvas.width, this.canvas.height);

            gl.colorMask(true, true, true, true);
            gl.clearColor(0.0, 0.0, 0.0, 0.0);
            gl.clear(gl.COLOR_BUFFER_BIT);
            gl.enable(gl.BLEND);
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE);

            gl.drawArrays(gl.TRIANGLES, 0, 6);

            if (!this.useMirror) {
                texture.deleteTarget(gl, gltex);
            }
        }
    };

    TexturePreviewGenerator.prototype.capture = function (doc) {
        var targetCanvas = doc.createElement("canvas");
        targetCanvas.className = "gli-reset";
        targetCanvas.width = this.canvas.width;
        targetCanvas.height = this.canvas.height;
        try {
            var ctx = targetCanvas.getContext("2d");
            if (doc == this.canvas.ownerDocument) {
                ctx.drawImage(this.canvas, 0, 0);
            } else {
                // Need to extract the data and copy manually, as doc->doc canvas
                // draws aren't supported for some stupid reason
                var srcctx = this.canvas.getContext("2d");
                if (srcctx) {
                    var srcdata = srcctx.getImageData(0, 0, this.canvas.width, this.canvas.height);
                    ctx.putImageData(srcdata, 0, 0);
                } else {
                    var dataurl = this.canvas.toDataURL();
                    var img = doc.createElement("img");
                    img.onload = function() {
                        ctx.drawImage(img, 0, 0);
                    };
                    img.src = dataurl;
                }
            }
        } catch (e) {
            window.console.log('unable to draw texture preview');
            window.console.log(e);
        }
        return targetCanvas;
    };

    TexturePreviewGenerator.prototype.buildItem = function (w, doc, gl, texture, closeOnClick, useCache) {
        var self = this;

        var el = doc.createElement("div");
        el.className = "texture-picker-item";
        if (texture.status == Resource.DEAD) {
            el.className += " texture-picker-item-deleted";
        }

        var previewContainer = doc.createElement("div");
        previewContainer.className = "texture-picker-item-container";
        el.appendChild(previewContainer);

        function updatePreview() {
            var preview = null;
            if (useCache && texture.cachedPreview) {
                // Preview exists - use it
                preview = texture.cachedPreview;
            }
            if (!preview) {
                // Preview does not exist - create it
                // TODO: pick the right version
                var version = texture.currentVersion;
                var targetFace;
                switch (texture.type) {
                    case gl.TEXTURE_CUBE_MAP:
                        targetFace = gl.TEXTURE_CUBE_MAP_POSITIVE_X; // pick a different face?
                        break;
                    default:
                        targetFace = null;
                        break;
                }
                var size = texture.guessSize(gl, version, targetFace);
                var desiredWidth = 128;
                var desiredHeight = 128;
                if (size) {
                    if (size[0] > size[1]) {
                        desiredWidth = 128;
                        desiredHeight = 128 / (size[0] / size[1]);
                    } else {
                        desiredHeight = 128;
                        desiredWidth = 128 / (size[1] / size[0]);
                    }
                }
                self.draw(texture, version, targetFace, desiredWidth, desiredHeight);
                preview = self.capture(doc);
                var x = (128 / 2) - (desiredWidth / 2);
                var y = (128 / 2) - (desiredHeight / 2);
                preview.style.marginLeft = x + "px";
                preview.style.marginTop = y + "px";
                if (useCache) {
                    texture.cachedPreview = preview;
                }
            }
            if (preview) {
                // TODO: setup
                preview.className = "";
                if (preview.parentNode) {
                    preview.parentNode.removeChild(preview);
                }
                while (previewContainer.hasChildNodes()) {
                    previewContainer.removeChild(previewContainer.firstChild);
                }
                previewContainer.appendChild(preview);
            }
        };

        updatePreview();

        var iconDiv = doc.createElement("div");
        iconDiv.className = "texture-picker-item-icon";
        switch (texture.type) {
            case gl.TEXTURE_2D:
                iconDiv.className += " texture-picker-item-icon-2d";
                break;
            case gl.TEXTURE_CUBE_MAP:
                iconDiv.className += " texture-picker-item-icon-cube";
                break;
        }
        el.appendChild(iconDiv);

        var titleDiv = doc.createElement("div");
        titleDiv.className = "texture-picker-item-title";
        titleDiv.textContent = texture.getName();
        el.appendChild(titleDiv);

        el.onclick = function (e) {
            w.context.ui.showTexture(texture);
            if (closeOnClick) {
                w.close(); // TODO: do this?
            }
            e.preventDefault();
            e.stopPropagation();
        };

        texture.modified.addListener(self, function (texture) {
            texture.cachedPreview = null;
            updatePreview();
        });
        texture.deleted.addListener(self, function (texture) {
            el.className += " texture-picker-item-deleted";
        });

        return el;
    };

    return TexturePreviewGenerator;
});
