define([
       './GLConsts',
    ], function(
        glc
    ) {

    /**
     * Returns the corresponding bind point for a given sampler type
     */
    function getBindPointForSamplerType(gl, type) {
        return typeMap[type].bindPoint;
    }

    // This kind of sucks! If you could compose functions as in `var fn = gl[name];`
    // this code could be a lot smaller but that is sadly really slow (T_T)

    function floatSetter(gl, location) {
        return function(v) {
            gl.uniform1f(location, v);
        };
    }

    function floatArraySetter(gl, location) {
        return function(v) {
            gl.uniform1fv(location, v);
        };
    }

    function floatVec2Setter(gl, location) {
        return function(v) {
            gl.uniform2fv(location, v);
        };
    }

    function floatVec3Setter(gl, location) {
        return function(v) {
            gl.uniform3fv(location, v);
        };
    }

    function floatVec4Setter(gl, location) {
        return function(v) {
          gl.uniform4fv(location, v);
        };
    }

    function intSetter(gl, location) {
        return function(v) {
            gl.uniform1i(location, v);
        };
    }

    function intArraySetter(gl, location) {
        return function(v) {
            gl.uniform1iv(location, v);
        };
    }

    function intVec2Setter(gl, location) {
        return function(v) {
            gl.uniform2iv(location, v);
        };
    }

    function intVec3Setter(gl, location) {
        return function(v) {
            gl.uniform3iv(location, v);
        };
    }

    function intVec4Setter(gl, location) {
        return function(v) {
            gl.uniform4iv(location, v);
        };
    }

    function uintSetter(gl, location) {
        return function(v) {
            gl.uniform1ui(location, v);
        };
    }

    function uintArraySetter(gl, location) {
        return function(v) {
            gl.uniform1uiv(location, v);
        };
    }

    function uintVec2Setter(gl, location) {
        return function(v) {
            gl.uniform2uiv(location, v);
        };
    }

    function uintVec3Setter(gl, location) {
        return function(v) {
            gl.uniform3uiv(location, v);
        };
    }

    function uintVec4Setter(gl, location) {
        return function(v) {
            gl.uniform4uiv(location, v);
        };
    }

    function floatMat2Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix2fv(location, false, v);
        };
    }

    function floatMat3Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix3fv(location, false, v);
        };
    }

    function floatMat4Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix4fv(location, false, v);
        };
    }

    function floatMat23Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix2x3fv(location, false, v);
        };
    }

    function floatMat32Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix3x2fv(location, false, v);
        };
    }

    function floatMat24Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix2x4fv(location, false, v);
        };
    }

    function floatMat42Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix4x2fv(location, false, v);
        };
    }

    function floatMat34Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix3x4fv(location, false, v);
        };
    }

    function floatMat43Setter(gl, location) {
        return function(v) {
            gl.uniformMatrix4x3fv(location, false, v);
        };
    }

    function samplerSetter(gl, type, unit, location) {
        var bindPoint = getBindPointForSamplerType(gl, type);
        return function(textureOrPair) {
            let texture;
            if (textureOrPair instanceof WebGLTexture) {
                texture = textureOrPair;
            } else {
                texture = textureOrPair.texture;
                gl.bindSampler(unit, textureOrPair.sampler);
            }
            gl.uniform1i(location, unit);
            gl.activeTexture(gl.TEXTURE0 + unit);
            gl.bindTexture(bindPoint, texture);
        };
    }

    function samplerArraySetter(gl, type, unit, location, size) {
        var bindPoint = getBindPointForSamplerType(gl, type);
        var units = new Int32Array(size);
        for (var ii = 0; ii < size; ++ii) {
            units[ii] = unit + ii;
        }

        return function(textures) {
            gl.uniform1iv(location, units);
            textures.forEach(function(textureOrPair, index) {
                gl.activeTexture(gl.TEXTURE0 + units[index]);
                let texture;
                if (textureOrPair instanceof WebGLTexture) {
                    texture = textureOrPair;
                } else {
                    texture = textureOrPair.texture;
                    gl.bindSampler(unit, textureOrPair.sampler);
                }
                gl.bindTexture(bindPoint, texture);
            });
        };
    }

    const typeMap = {};
    typeMap[glc.FLOAT]                         = { Type: Float32Array, size:  4, setter: floatSetter,      arraySetter: floatArraySetter, };
    typeMap[glc.FLOAT_VEC2]                    = { Type: Float32Array, size:  8, setter: floatVec2Setter,  };
    typeMap[glc.FLOAT_VEC3]                    = { Type: Float32Array, size: 12, setter: floatVec3Setter,  };
    typeMap[glc.FLOAT_VEC4]                    = { Type: Float32Array, size: 16, setter: floatVec4Setter,  };
    typeMap[glc.INT]                           = { Type: Int32Array,   size:  4, setter: intSetter,        arraySetter: intArraySetter, };
    typeMap[glc.INT_VEC2]                      = { Type: Int32Array,   size:  8, setter: intVec2Setter,    };
    typeMap[glc.INT_VEC3]                      = { Type: Int32Array,   size: 12, setter: intVec3Setter,    };
    typeMap[glc.INT_VEC4]                      = { Type: Int32Array,   size: 16, setter: intVec4Setter,    };
    typeMap[glc.UNSIGNED_INT]                  = { Type: Uint32Array,  size:  4, setter: uintSetter,       arraySetter: uintArraySetter, };
    typeMap[glc.UNSIGNED_INT_VEC2]             = { Type: Uint32Array,  size:  8, setter: uintVec2Setter,   };
    typeMap[glc.UNSIGNED_INT_VEC3]             = { Type: Uint32Array,  size: 12, setter: uintVec3Setter,   };
    typeMap[glc.UNSIGNED_INT_VEC4]             = { Type: Uint32Array,  size: 16, setter: uintVec4Setter,   };
    typeMap[glc.BOOL]                          = { Type: Uint32Array,  size:  4, setter: intSetter,        arraySetter: intArraySetter, };
    typeMap[glc.BOOL_VEC2]                     = { Type: Uint32Array,  size:  8, setter: intVec2Setter,    };
    typeMap[glc.BOOL_VEC3]                     = { Type: Uint32Array,  size: 12, setter: intVec3Setter,    };
    typeMap[glc.BOOL_VEC4]                     = { Type: Uint32Array,  size: 16, setter: intVec4Setter,    };
    typeMap[glc.FLOAT_MAT2]                    = { Type: Float32Array, size: 16, setter: floatMat2Setter,  };
    typeMap[glc.FLOAT_MAT3]                    = { Type: Float32Array, size: 36, setter: floatMat3Setter,  };
    typeMap[glc.FLOAT_MAT4]                    = { Type: Float32Array, size: 64, setter: floatMat4Setter,  };
    typeMap[glc.FLOAT_MAT2x3]                  = { Type: Float32Array, size: 24, setter: floatMat23Setter, };
    typeMap[glc.FLOAT_MAT2x4]                  = { Type: Float32Array, size: 32, setter: floatMat24Setter, };
    typeMap[glc.FLOAT_MAT3x2]                  = { Type: Float32Array, size: 24, setter: floatMat32Setter, };
    typeMap[glc.FLOAT_MAT3x4]                  = { Type: Float32Array, size: 48, setter: floatMat34Setter, };
    typeMap[glc.FLOAT_MAT4x2]                  = { Type: Float32Array, size: 32, setter: floatMat42Setter, };
    typeMap[glc.FLOAT_MAT4x3]                  = { Type: Float32Array, size: 48, setter: floatMat43Setter, };
    typeMap[glc.SAMPLER_2D]                    = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D,       };
    typeMap[glc.SAMPLER_CUBE]                  = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_CUBE_MAP, };
    typeMap[glc.SAMPLER_3D]                    = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_3D,       };
    typeMap[glc.SAMPLER_2D_SHADOW]             = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D,       };
    typeMap[glc.SAMPLER_2D_ARRAY]              = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D_ARRAY, };
    typeMap[glc.SAMPLER_2D_ARRAY_SHADOW]       = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D_ARRAY, };
    typeMap[glc.SAMPLER_CUBE_SHADOW]           = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_CUBE_MAP, };
    typeMap[glc.INT_SAMPLER_2D]                = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D,       };
    typeMap[glc.INT_SAMPLER_3D]                = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_3D,       };
    typeMap[glc.INT_SAMPLER_CUBE]              = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_CUBE_MAP, };
    typeMap[glc.INT_SAMPLER_2D_ARRAY]          = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D_ARRAY, };
    typeMap[glc.UNSIGNED_INT_SAMPLER_2D]       = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D,       };
    typeMap[glc.UNSIGNED_INT_SAMPLER_3D]       = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_3D,       };
    typeMap[glc.UNSIGNED_INT_SAMPLER_CUBE]     = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_CUBE_MAP, };
    typeMap[glc.UNSIGNED_INT_SAMPLER_2D_ARRAY] = { Type: null,         size:  0, setter: samplerSetter,    arraySetter: samplerArraySetter, bindPoint: glc.TEXTURE_2D_ARRAY, };

    function floatAttribSetter(gl, index) {
        return function(b) {
            gl.bindBuffer(gl.ARRAY_BUFFER, b.buffer);
            gl.enableVertexAttribArray(index);
            gl.vertexAttribPointer(
                index, b.numComponents || b.size, b.type || gl.FLOAT, b.normalize || false, b.stride || 0, b.offset || 0);
        };
    }

    function intAttribSetter(gl, index) {
        return function(b) {
            gl.bindBuffer(gl.ARRAY_BUFFER, b.buffer);
            gl.enableVertexAttribArray(index);
            gl.vertexAttribIPointer(
                index, b.numComponents || b.size, b.type || gl.INT, b.stride || 0, b.offset || 0);
        };
    }

    function matAttribSetter(gl, index, typeInfo) {
        var defaultSize = typeInfo.size;
        var count = typeInfo.count;

        return function(b) {
            gl.bindBuffer(gl.ARRAY_BUFFER, b.buffer);
            var numComponents = b.size || b.numComponents || defaultSize;
            var size = numComponents / count;
            var type = b.type || gl.FLOAT;
            var typeInfo = typeMap[type];
            var stride = typeInfo.size * numComponents;
            var normalize = b.normalize || false;
            var offset = b.offset || 0;
            var rowOffset = stride / count;
            for (var i = 0; i < count; ++i) {
                gl.enableVertexAttribArray(index + i);
                gl.vertexAttribPointer(
                    index + i, size, type, normalize, stride, offset + rowOffset * i);
            }
        };
    }

    function addLineNumbers(src, lineOffset) {
        lineOffset = lineOffset || 0;
        ++lineOffset;

        return src.split("\n").map(function(line, ndx) {
            return (ndx + lineOffset) + ": " + line;
        }).join("\n");
    }

    function createShader(gl, vSrc, vType) {
        const shader = gl.createShader(vType);
        gl.shaderSource(shader, vSrc);
        gl.compileShader(shader);
        const success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
        if (!success) {
            console.error(addLineNumbers(vSrc), "\n", gl.getShaderInfoLog(shader));
        }
        return shader;
    };

    const shaderTypes = [glc.VERTEX_SHADER, glc.FRAGMENT_SHADER];

    function createProgram(gl, sources, locations) {
        var program = gl.createProgram();
        var shaders = sources.map((src, ndx) => {
            const shader = createShader(gl, src, shaderTypes[ndx]);
            gl.attachShader(program, shader);
            gl.deleteShader(shader);
            return shader;
        });

        if (locations) {
            locations.forEach((name, ndx) => {
                gl.bindAttribLocation(program, ndx, name);
            });
        }

        gl.linkProgram(program);

        shaders.forEach(shader => gl.deleteShader(shader));
        var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
        if (!linked) {
            console.error("--vertexShader--\n",
                          addLineNumbers(source[0]),
                          "--fragmentShader--\n",
                          addLineNumbers(source[1]),
                          "--error--\n",
                          gl.getProgramInfoLog(program));
        }
        return program;
    }

    function createUniformSetters(gl, program) {
        var textureUnit = 0;

        function createUniformSetter(program, uniformInfo) {
            var location = gl.getUniformLocation(program, uniformInfo.name);
            var isArray = (uniformInfo.size > 1 && uniformInfo.name.substr(-3) === "[0]");
            var type = uniformInfo.type;
            var typeInfo = typeMap[type];
            if (!typeInfo) {
                throw ("unknown type: 0x" + type.toString(16)); // we should never get here.
            }
            if (typeInfo.bindPoint) {
                // it's a sampler
                var unit = textureUnit;
                textureUnit += uniformInfo.size;

                if (isArray) {
                    return typeInfo.arraySetter(gl, type, unit, location, uniformInfo.size);
                } else {
                    return typeInfo.setter(gl, type, unit, location, uniformInfo.size);
                }
            } else {
                if (typeInfo.arraySetter && isArray) {
                    return typeInfo.arraySetter(gl, location);
                } else {
                    return typeInfo.setter(gl, location);
                }
            }
        }

        var uniformSetters = { };
        var numUniforms = gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS);

        for (var ii = 0; ii < numUniforms; ++ii) {
            var uniformInfo = gl.getActiveUniform(program, ii);
            if (!uniformInfo) {
                break;
            }
            var name = uniformInfo.name;
            // remove the array suffix.
            if (name.substr(-3) === "[0]") {
                name = name.substr(0, name.length - 3);
            }
            var setter = createUniformSetter(program, uniformInfo);
            uniformSetters[name] = setter;
        }
        return uniformSetters;
    }

    function setUniforms(gl, setters, values) {  // eslint-disable-line
        var actualSetters = setters.uniformSetters || setters;
        var numArgs = arguments.length;
        for (var andx = 1; andx < numArgs; ++andx) {
            var vals = arguments[andx];
            if (Array.isArray(vals)) {
                var numValues = vals.length;
                for (var ii = 0; ii < numValues; ++ii) {
                    setUniforms(actualSetters, vals[ii]);
                }
            } else {
                for (var name in vals) {
                    var setter = actualSetters[name];
                    if (setter) {
                        setter(vals[name]);
                    }
                }
            }
        }
    }

    function createProgramInfo(gl, sources, attributes) {
        const program = createProgram(gl, sources, attributes);
        const uniformSetters = createUniformSetters(gl, program);
        const programInfo = {
            program: program,
            uniformSetters: uniformSetters,
        };

        return programInfo;
    }

    return {
        createShader: createShader,
        createProgram: createProgram,
        createProgramInfo: createProgramInfo,
        setUniforms: setUniforms,
    };

});

