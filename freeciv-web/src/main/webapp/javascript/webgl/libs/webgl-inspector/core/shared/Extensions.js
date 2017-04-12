define([
        './EventSource',
    ], function (
        EventSource
    ) {

    function installFrameTerminatorExtension(gl) {
        var ext = {};

        ext.frameEvent = new EventSource("frameEvent");

        ext.frameTerminator = function () {
            ext.frameEvent.fire();
        };

        return {
            name: "GLI_frame_terminator",
            object: ext
        };
    };

    function installExtensions(gl) {
        var extensionStrings = [];
        var extensionObjects = {};

        // Setup extensions
        var frameTerminatorExt = installFrameTerminatorExtension(gl);
        extensionStrings.push(frameTerminatorExt.name);
        extensionObjects[frameTerminatorExt.name] = frameTerminatorExt.object;

        // Patch in new extensions
        var original_getSupportedExtensions = gl.getSupportedExtensions;
        gl.getSupportedExtensions = function () {
            var supportedExtensions = original_getSupportedExtensions.apply(gl);
            for (var n = 0; n < extensionStrings.length; n++) {
                supportedExtensions.push(extensionStrings[n]);
            }
            return supportedExtensions;
        };
        var original_getExtension = gl.getExtension;
        gl.getExtension = function (name) {
            var ext = extensionObjects[name];
            if (ext) {
                return ext;
            } else {
                return original_getExtension.apply(gl, arguments);
            }
        };
    };

    function enableAllExtensions(gl) {
        if (!gl.getSupportedExtensions) {
            return;
        }

        gl.getSupportedExtensions().forEach(function (ext) {
          if (ext.substr(0, 3) !== "MOZ") gl.getExtension(ext);
        });
    };

    return {
        enableAllExtensions: enableAllExtensions,
        installExtensions: installExtensions,
    };

});
