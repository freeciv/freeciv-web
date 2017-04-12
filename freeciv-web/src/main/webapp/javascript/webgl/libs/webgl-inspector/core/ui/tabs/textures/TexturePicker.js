define([
        '../../../shared/Base',
        '../../shared/PopupWindow',
        '../../shared/TexturePreview',
    ], function (
        base,
        PopupWindow,
        TexturePreviewGenerator
    ) {

    var TexturePicker = function (context, name) {
        base.subclass(PopupWindow, this, [context, name, "Texture Browser", 610, 600]);
    };

    TexturePicker.prototype.setup = function () {
        var self = this;
        var context = this.context;
        var doc = this.browserWindow.document;
        var gl = context;

        this.previewer = new TexturePreviewGenerator();
        
        // Append textures already present
        var textures = context.resources.getTextures();
        for (var n = 0; n < textures.length; n++) {
            var texture = textures[n];
            var el = this.previewer.buildItem(this, doc, gl, texture, true, true);
            this.elements.innerDiv.appendChild(el);
        }

        // Listen for changes
        context.resources.resourceRegistered.addListener(this, this.resourceRegistered);
    };
    
    TexturePicker.prototype.dispose = function () {
        this.context.resources.resourceRegistered.removeListener(this);
    };
    
    TexturePicker.prototype.resourceRegistered = function (resource) {
        var doc = this.browserWindow.document;
        var gl = this.context;
        if (base.typename(resource.target) == "WebGLTexture") {
            var el = this.previewer.buildItem(this, doc, gl, resource, true);
            this.elements.innerDiv.appendChild(el);
        }
    };

    return TexturePicker;
});
