define([
        '../../shared/Base',
        '../Resource',
    ], function (
        base,
        Resource) {

    var Shader = function (gl, frameNumber, stack, target, args) {
        base.subclass(Resource, this, [gl, frameNumber, stack, target]);
        this.creationOrder = 4;

        this.defaultName = "Shader " + this.id;

        this.type = args[0]; // VERTEX_SHADER, FRAGMENT_SHADER

        this.source = null;

        this.parameters = {};
        this.parameters[gl.SHADER_TYPE] = this.type;
        this.parameters[gl.DELETE_STATUS] = 0;
        this.parameters[gl.COMPILE_STATUS] = 0;
        this.infoLog = null;

        this.currentVersion.target = this.type;
        this.currentVersion.setParameters(this.parameters);
        this.currentVersion.setExtraParameters("extra", { infoLog: "" });
    };

    Shader.prototype.refresh = function (gl) {
        var paramEnums = [gl.SHADER_TYPE, gl.DELETE_STATUS, gl.COMPILE_STATUS];
        for (var n = 0; n < paramEnums.length; n++) {
            this.parameters[paramEnums[n]] = gl.getShaderParameter(this.target, paramEnums[n]);
        }
        this.infoLog = gl.getShaderInfoLog(this.target);
    };

    Shader.setCaptures = function (gl) {
        var original_shaderSource = gl.shaderSource;
        gl.shaderSource = function () {
            var tracked = arguments[0].trackedObject;
            tracked.source = arguments[1];
            tracked.markDirty(true);
            tracked.currentVersion.target = tracked.type;
            tracked.currentVersion.setParameters(tracked.parameters);
            tracked.currentVersion.pushCall("shaderSource", arguments);
            return original_shaderSource.apply(gl, arguments);
        };

        var original_compileShader = gl.compileShader;
        gl.compileShader = function () {
            var tracked = arguments[0].trackedObject;
            tracked.markDirty(false);
            var result = original_compileShader.apply(gl, arguments);
            tracked.refresh(gl);
            tracked.currentVersion.setParameters(tracked.parameters);
            tracked.currentVersion.setExtraParameters("extra", { infoLog: tracked.infoLog });
            tracked.currentVersion.pushCall("compileShader", arguments);
            return result;
        };
    };

    Shader.prototype.createTarget = function (gl, version, options) {
        var shader = gl.createShader(version.target);

        this.replayCalls(gl, version, shader, function (call, args) {
            if (options.fragmentShaderOverride) {
                if (call.name == "shaderSource") {
                    if (version.target == gl.FRAGMENT_SHADER) {
                        args[1] = options.fragmentShaderOverride;
                    }
                }
            }
            return true;
        });

        return shader;
    };

    Shader.prototype.deleteTarget = function (gl, target) {
        gl.deleteShader(target);
    };

    return Shader;

});
