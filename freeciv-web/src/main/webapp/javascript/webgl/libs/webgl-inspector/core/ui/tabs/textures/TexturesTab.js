define([
        '../../../shared/Base',
        '../../../host/Resource',
        '../../Tab',
        '../../shared/LeftListing',
        '../../shared/PopupWindow',
        './TexturePicker',
        './TextureView',
    ], function (
        base,
        Resource,
        Tab,
        LeftListing,
        PopupWindow,
        TexturePicker,
        TextureView
    ) {

    const textureItemCSSClassRE = /texture-item-\w+$/;

    var TexturesTab = function (w) {
        var outer = Tab.divClass('window-right-outer');
        var right = Tab.divClass('window-right');
        var inspector = Tab.divClass('window-inspector');
        inspector.classList.add('window-texture-inspector');
        var texture = Tab.divClass('window-texture-outer');

        inspector.appendChild(Tab.divClass('surface-inspector-toolbar', 'toolbar'));
        inspector.appendChild(Tab.divClass('surface-inspector-inner', 'inspector'));
        inspector.appendChild(Tab.inspector());
        texture.appendChild(Tab.divClass('texture-listing', 'call trace'));
        right.appendChild(inspector);
        right.appendChild(texture);
        outer.appendChild(right);
        outer.appendChild(Tab.windowLeft({ listing: 'frame list', toolbar: 'buttons'}));

        this.el.appendChild(outer);

        this.listing = new LeftListing(w, this.el, "texture", function (el, texture) {
            var gl = w.context;

            if (texture.status == Resource.DEAD) {
                el.className += " texture-item-deleted";
            }

            switch (texture.type) {
                case gl.TEXTURE_2D:
                    el.className += " texture-item-2d";
                    break;
                case gl.TEXTURE_CUBE_MAP:
                    el.className += " texture-item-cube";
                    break;
                case gl.TEXTURE_3D:
                    el.className += " texture-item-3d";
                    break;
                case gl.TEXTURE_2D_ARRAY:
                    el.className += " texture-item-3d";
                    break;
            }

            var number = document.createElement("div");
            number.className = "texture-item-number";
            number.textContent = texture.getName();
            el.appendChild(number);

            var row = document.createElement("div");
            row.className = "texture-item-row";

            function updateSize() {
                switch (texture.type) {
                    case gl.TEXTURE_2D:
                        el.className = el.className.replace(textureItemCSSClassRE, 'texture-item-2d');
                        break;
                    case gl.TEXTURE_CUBE_MAP:
                        el.className = el.className.replace(textureItemCSSClassRE, 'texture-item-cube');
                        break;
                    case gl.TEXTURE_3D:
                        el.className = el.className.replace(textureItemCSSClassRE, 'texture-item-3d');
                        break;
                    case gl.TEXTURE_2D_ARRAY:
                        el.className = el.className.replace(textureItemCSSClassRE, 'texture-item-2d-array');
                        break;
                }
                var guessedSize = texture.guessSize(gl);
                if (guessedSize) {
                    row.textContent = guessedSize[0] + " x " + guessedSize[1];
                } else {
                    row.textContent = "? x ?";
                }
            };
            updateSize();

            if (!row.hasChildNodes()) {
                el.appendChild(row);
            }

            texture.modified.addListener(this, function (texture) {
                number.textContent = texture.getName();
                updateSize();
                // TODO: refresh view if selected
            });
            texture.deleted.addListener(this, function (texture) {
                el.className += " texture-item-deleted";
            });
        });

        this.listing.addButton("Browse All").addListener(this, function () {
            PopupWindow.show(w.context, TexturePicker, "texturePicker", function (popup) {
            });
        });

        this.textureView = new TextureView(w, this.el);

        this.listing.valueSelected.addListener(this, function (texture) {
            this.textureView.setTexture(texture);
        });

        var scrollStates = {};
        this.lostFocus.addListener(this, function () {
            scrollStates.listing = this.listing.getScrollState();
        });
        this.gainedFocus.addListener(this, function () {
            this.listing.setScrollState(scrollStates.listing);
        });

        // Append textures already present
        var context = w.context;
        var textures = context.resources.getTextures();
        for (var n = 0; n < textures.length; n++) {
            var texture = textures[n];
            this.listing.appendValue(texture);
        }

        // Listen for changes
        context.resources.resourceRegistered.addListener(this, function (resource) {
            if (base.typename(resource.target) == "WebGLTexture") {
                this.listing.appendValue(resource);
            }
        });

        this.layout = function () {
            this.textureView.layout();
        };

        this.refresh = function () {
            this.textureView.setTexture(this.textureView.currentTexture);
        };
    };

    return TexturesTab;
});
