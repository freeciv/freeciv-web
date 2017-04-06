define([
        '../../shared/Base',
        '../Resource',
    ], function (
        base,
        Resource) {

    var Framebuffer = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 3;

        this.defaultName = "Framebuffer " + this.id;

        // Track the attachments a framebuffer has (watching framebufferRenderbuffer/etc calls)
        this.attachments = {};

        this.parameters = {};
        // Attachments: COLOR_ATTACHMENT0, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT
        // These parameters are per-attachment
        //this.parameters[gl.FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE] = 0;
        //this.parameters[gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME] = 0;
        //this.parameters[gl.FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL] = 0;
        //this.parameters[gl.FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE] = 0;

        this.currentVersion.setParameters(this.parameters);
        this.currentVersion.setExtraParameters("attachments", this.attachments);
    };

    Framebuffer.prototype.getDependentResources = function () {
        var resources = [];
        for (var n in this.attachments) {
            var attachment = this.attachments[n];
            if (resources.indexOf(attachment) == -1) {
                resources.push(attachment);
            }
        }
        return resources;
    };

    Framebuffer.prototype.refresh = function (gl) {
        // Attachments: COLOR_ATTACHMENT0, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT
        //var paramEnums = [gl.FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, gl.FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, gl.FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, gl.FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE];
        //for (var n = 0; n < paramEnums.length; n++) {
        //    this.parameters[paramEnums[n]] = gl.getFramebufferAttachmentParameter(gl.FRAMEBUFFER, attachment, paramEnums[n]);
        //}
    };

    Framebuffer.getTracked = function (gl, args) {
        // only FRAMEBUFFER
        var bindingEnum = gl.FRAMEBUFFER_BINDING;
        var glframebuffer = gl.getParameter(bindingEnum);
        if (glframebuffer == null) {
            // Going to fail
            return null;
        }
        return glframebuffer.trackedObject;
    };

    Framebuffer.setCaptures = function (gl) {
        var original_framebufferRenderbuffer = gl.framebufferRenderbuffer;
        gl.framebufferRenderbuffer = function () {
            var tracked = Framebuffer.getTracked(gl, arguments);
            tracked.markDirty(false);
            // TODO: remove existing calls for this attachment
            tracked.currentVersion.pushCall("framebufferRenderbuffer", arguments);

            // Save attachment
            tracked.attachments[arguments[1]] = arguments[3] ? arguments[3].trackedObject : null;
            tracked.currentVersion.setExtraParameters("attachments", tracked.attachments);

            var result = original_framebufferRenderbuffer.apply(gl, arguments);

            // HACK: query the parameters now - easier than calculating all of them
            tracked.refresh(gl);
            tracked.currentVersion.setParameters(tracked.parameters);

            return result;
        };

        var original_framebufferTexture2D = gl.framebufferTexture2D;
        gl.framebufferTexture2D = function () {
            var tracked = Framebuffer.getTracked(gl, arguments);
            tracked.markDirty(false);
            // TODO: remove existing calls for this attachment
            tracked.currentVersion.pushCall("framebufferTexture2D", arguments);

            // Save attachment
            tracked.attachments[arguments[1]] = arguments[3] ? arguments[3].trackedObject : null;
            tracked.currentVersion.setExtraParameters("attachments", tracked.attachments);

            var result = original_framebufferTexture2D.apply(gl, arguments);

            // HACK: query the parameters now - easier than calculating all of them
            tracked.refresh(gl);
            tracked.currentVersion.setParameters(tracked.parameters);

            return result;
        };
    };

    Framebuffer.prototype.createTarget = function (gl, version) {
        var framebuffer = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);

        this.replayCalls(gl, version, framebuffer);

        return framebuffer;
    };

    Framebuffer.prototype.deleteTarget = function (gl, target) {
        gl.deleteFramebuffer(target);
    };

    return Framebuffer;

});
