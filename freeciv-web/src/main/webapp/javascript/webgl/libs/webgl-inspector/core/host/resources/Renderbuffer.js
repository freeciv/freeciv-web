define([
        '../../shared/Base',
        '../Resource',
    ], function (
        base,
        Resource) {

    var Renderbuffer = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 2;

        this.defaultName = "Renderbuffer " + this.id;

        this.parameters = {};
        this.parameters[gl.RENDERBUFFER_WIDTH] = 0;
        this.parameters[gl.RENDERBUFFER_HEIGHT] = 0;
        this.parameters[gl.RENDERBUFFER_INTERNAL_FORMAT] = gl.RGBA4;
        this.parameters[gl.RENDERBUFFER_RED_SIZE] = 0;
        this.parameters[gl.RENDERBUFFER_GREEN_SIZE] = 0;
        this.parameters[gl.RENDERBUFFER_BLUE_SIZE] = 0;
        this.parameters[gl.RENDERBUFFER_ALPHA_SIZE] = 0;
        this.parameters[gl.RENDERBUFFER_DEPTH_SIZE] = 0;
        this.parameters[gl.RENDERBUFFER_STENCIL_SIZE] = 0;

        this.currentVersion.setParameters(this.parameters);
    };

    Renderbuffer.prototype.refresh = function (gl) {
        var paramEnums = [gl.RENDERBUFFER_WIDTH, gl.RENDERBUFFER_HEIGHT, gl.RENDERBUFFER_INTERNAL_FORMAT, gl.RENDERBUFFER_RED_SIZE, gl.RENDERBUFFER_GREEN_SIZE, gl.RENDERBUFFER_BLUE_SIZE, gl.RENDERBUFFER_ALPHA_SIZE, gl.RENDERBUFFER_DEPTH_SIZE, gl.RENDERBUFFER_STENCIL_SIZE];
        for (var n = 0; n < paramEnums.length; n++) {
            this.parameters[paramEnums[n]] = gl.getRenderbufferParameter(gl.RENDERBUFFER, paramEnums[n]);
        }
    };

    Renderbuffer.getTracked = function (gl, args) {
        // only RENDERBUFFER
        var bindingEnum = gl.RENDERBUFFER_BINDING;
        var glrenderbuffer = gl.getParameter(bindingEnum);
        if (glrenderbuffer == null) {
            // Going to fail
            return null;
        }
        return glrenderbuffer.trackedObject;
    };

    Renderbuffer.setCaptures = function (gl) {
        var original_renderbufferStorage = gl.renderbufferStorage;
        gl.renderbufferStorage = function () {
            var tracked = Renderbuffer.getTracked(gl, arguments);
            tracked.markDirty(true);
            tracked.currentVersion.pushCall("renderbufferStorage", arguments);

            var result = original_renderbufferStorage.apply(gl, arguments);

            // HACK: query the parameters now - easier than calculating all of them
            tracked.refresh(gl);
            tracked.currentVersion.setParameters(tracked.parameters);

            return result;
        };
    };

    Renderbuffer.prototype.createTarget = function (gl, version) {
        var renderbuffer = gl.createRenderbuffer();
        gl.bindRenderbuffer(gl.RENDERBUFFER, renderbuffer);

        this.replayCalls(gl, version, renderbuffer);

        return renderbuffer;
    };

    Renderbuffer.prototype.deleteTarget = function (gl, target) {
        gl.deleteRenderbuffer(target);
    };

    return Renderbuffer;

});
