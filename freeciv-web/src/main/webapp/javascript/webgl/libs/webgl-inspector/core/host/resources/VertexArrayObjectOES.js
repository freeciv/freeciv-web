define([
        '../../shared/Base',
        '../Resource',
    ], function (
        base,
        Resource) {

    var VertexArrayObjectOES = function (gl, frameNumber, stack, target) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 2;

        this.defaultName = "VAO " + this.id;

        this.parameters = {};

        this.currentVersion.setParameters(this.parameters);
    };

    VertexArrayObjectOES.prototype.refresh = function (gl) {
    };

    VertexArrayObjectOES.getTracked = function (gl, args) {
        var ext = gl.getExtension("OES_vertex_array_object");
        var glvao = gl.getParameter(ext.VERTEX_ARRAY_BINDING_OES);
        if (glvao == null) {
            // Going to fail
            return null;
        }
        return glvao.trackedObject;
    };

    VertexArrayObjectOES.setCaptures = function (gl) {
        var ext = gl.getExtension("OES_vertex_array_object");
        
        /*
        var original_renderbufferStorage = gl.renderbufferStorage;
        gl.renderbufferStorage = function () {
            var tracked = VertexArrayObjectOES.getTracked(gl, arguments);
            tracked.markDirty(true);
            tracked.currentVersion.pushCall("renderbufferStorage", arguments);

            var result = original_renderbufferStorage.apply(gl, arguments);

            // HACK: query the parameters now - easier than calculating all of them
            tracked.refresh(gl);
            tracked.currentVersion.setParameters(tracked.parameters);

            return result;
        };*/
    };

    VertexArrayObjectOES.prototype.createTarget = function (gl, version) {
        var ext = gl.getExtension("OES_vertex_array_object");
        
        var vao = ext.createVertexArrayOES();
        ext.bindVertexArrayOES(vao);

        this.replayCalls(gl, version, vao);

        return vao;
    };

    VertexArrayObjectOES.prototype.deleteTarget = function (gl, target) {
        var ext = gl.getExtension("OES_vertex_array_object");
        ext.deleteVertexArrayOES(target);
    };

    return VertexArrayObjectOES;

});
