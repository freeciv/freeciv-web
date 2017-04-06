define([
        'StackTrace',
        '../shared/Base',
        '../shared/EventSource',
        '../shared/Utilities',
        './resources/Buffer',
        './resources/Framebuffer',
        './resources/Program',
        './resources/Renderbuffer',
        './resources/Shader',
        './resources/Texture',
        './resources/VertexArrayObject',
        './resources/VertexArrayObjectOES',
    ], function (
        printStackTrace,
        base,
        EventSource,
        util,
        Buffer,
        Framebuffer,
        Program,
        Renderbuffer,
        Shader,
        Texture,
        VertexArray,
        VertexArrayObjectOES
    ) {

    var resourceCtors = {
      Buffer: Buffer,
      Framebuffer: Framebuffer,
      Program: Program,
      Renderbuffer: Renderbuffer,
      Shader: Shader,
      Texture: Texture,
      VertexArray: VertexArray,
      VertexArrayObjectOES: VertexArrayObjectOES,
    };

    function setCaptures(cache, context) {
        var gl = context; //.rawgl;

        var generateStack;
        if (context.options.resourceStacks) {
            generateStack = function () {
                // Generate stack trace
                var stack = printStackTrace();
                // ignore garbage
                stack = stack.slice(4);
                // Fix up our type
                stack[0] = stack[0].replace("[object Object].", "gl.");
                return stack;
            };
        } else {
            generateStack = function () { return null; }
        }

        function captureCreateDelete(typeName) {
            var originalCreate = gl["create" + typeName];
            gl["create" + typeName] = function () {
                // Track object count
                gl.statistics[typeName.toLowerCase() + "Count"].value++;

                var result = originalCreate.apply(gl, arguments);
                var tracked = new resourceCtors[typeName](gl, context.frameNumber, generateStack(), result, arguments);
                if (tracked) {
                    cache.registerResource(tracked);
                }
                return result;
            };
            var originalDelete = gl["delete" + typeName];
            gl["delete" + typeName] = function () {
                // Track object count
                // FIXME: This object might already be deleted in which case
                // we should not decrement the count
                gl.statistics[typeName.toLowerCase() + "Count"].value--;

                var tracked = arguments[0] ? arguments[0].trackedObject : null;
                if (tracked) {
                    // Track total buffer and texture bytes consumed
                    if (typeName == "Buffer") {
                        gl.statistics.bufferBytes.value -= tracked.estimatedSize;
                    } else if (typeName == "Texture") {
                        gl.statistics.textureBytes.value -= tracked.estimatedSize;
                    }

                    tracked.markDeleted(generateStack());
                }
                originalDelete.apply(gl, arguments);
            };
        };

        captureCreateDelete("Buffer");
        captureCreateDelete("Framebuffer");
        captureCreateDelete("Program");
        captureCreateDelete("Renderbuffer");
        captureCreateDelete("Shader");
        captureCreateDelete("Texture");

        var glvao = gl.getExtension("OES_vertex_array_object");
        if (glvao) {
            (function() {
                var originalCreate = glvao.createVertexArrayOES;
                glvao.createVertexArrayOES = function () {
                    // Track object count
                    gl.statistics["vertexArrayObjectCount"].value++;

                    var result = originalCreate.apply(glvao, arguments);
                    var tracked = new VertexArrayObjectOES(gl, context.frameNumber, generateStack(), result, arguments);
                    if (tracked) {
                        cache.registerResource(tracked);
                    }
                    return result;
                };
                var originalDelete = glvao.deleteVertexArrayOES;
                glvao.deleteVertexArrayOES = function () {
                    // Track object count
                    // FIXME: This object might already be deleted in which case
                    // we should not decrement the count
                    gl.statistics["vertexArrayObjectCount"].value--;

                    var tracked = arguments[0] ? arguments[0].trackedObject : null;
                    if (tracked) {
                        tracked.markDeleted(generateStack());
                    }
                    originalDelete.apply(glvao, arguments);
                };
            })();
        }

        Buffer.setCaptures(gl);
        Framebuffer.setCaptures(gl);
        Program.setCaptures(gl);
        Renderbuffer.setCaptures(gl);
        Shader.setCaptures(gl);
        Texture.setCaptures(gl);
        VertexArrayObjectOES.setCaptures(gl);

        if (util.isWebGL2(gl)) {
//            captureCreateDelete("Query");
//            captureCreateDelete("Sampler");
            //captureCreateDelete("Sync");
//            captureCreateDelete("TransformFeedback");
            captureCreateDelete("VertexArray");

//            Query.setCaptures(gl);
//            Sampler.setCaptures(gl);
            //Sync.setCaptures(gl);
//            TransformFeedback.setCaptures(gl);
            VertexArray.setCaptures(gl);
        }


    };

    var ResourceCache = function (context) {
        this.context = context;

        this.resources = [];

        this.resourceRegistered = new EventSource("resourceRegistered");

        setCaptures(this, context);
    };

    ResourceCache.prototype.registerResource = function (resource) {
        this.resources.push(resource);
        this.resourceRegistered.fire(resource);
    };

    ResourceCache.prototype.captureVersions = function () {
        var allResources = [];
        for (var n = 0; n < this.resources.length; n++) {
            var resource = this.resources[n];
            allResources.push({
                resource: resource,
                value: resource.captureVersion()
            });
        }
        return allResources;
    };

    ResourceCache.prototype.getResources = function (name) {
        var selectedResources = [];
        for (var n = 0; n < this.resources.length; n++) {
            var resource = this.resources[n];
            var typename = base.typename(resource.target);
            if (typename == name) {
                selectedResources.push(resource);
            }
        }
        return selectedResources;
    };

    ResourceCache.prototype.getResourceById = function (id) {
        // TODO: fast lookup
        for (var n = 0; n < this.resources.length; n++) {
            var resource = this.resources[n];
            if (resource.id === id) {
                return resource;
            }
        }
        return null;
    };

    ResourceCache.prototype.getTextures = function () {
        return this.getResources("WebGLTexture");
    };

    ResourceCache.prototype.getBuffers = function () {
        return this.getResources("WebGLBuffer");
    };

    ResourceCache.prototype.getPrograms = function () {
        return this.getResources("WebGLProgram");
    };

    return ResourceCache;
});
