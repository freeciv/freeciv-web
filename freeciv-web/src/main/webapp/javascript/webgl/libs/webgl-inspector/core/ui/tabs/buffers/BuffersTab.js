define([
        '../../../shared/Base',
        '../../../host/Resource',
        '../../Tab',
        '../../shared/LeftListing',
        './BufferView',
    ], function (
        base,
        Resource,
        Tab,
        LeftListing,
        BufferView
    ) {

    var BuffersTab = function (w) {
        var outer = Tab.divClass("window-right-outer");
        var right = Tab.divClass("window-right");
        var inspector = Tab.divClass("window-inspector");
        inspector.classList.add("window-buffer-inspector");
        var buffer = Tab.divClass("window-buffer-outer");

        inspector.appendChild(Tab.divClass("surface-inspector-toolbar", "toolbar"));
        inspector.appendChild(Tab.divClass("surface-inspector-inner", "inspector"));
        inspector.appendChild(Tab.divClass("surface-inspector-statusbar"));
        buffer.appendChild(Tab.divClass("buffer-listing", "scrolling contents"));
        right.appendChild(inspector);
        right.appendChild(buffer);
        outer.appendChild(right);
        outer.appendChild(Tab.windowLeft({ listing: "frame list", toolbar: "buttons"}));

        this.el.appendChild(outer);

        this.listing = new LeftListing(w, this.el, "buffer", function (el, buffer) {
            var gl = w.context;

            if (buffer.status == Resource.DEAD) {
                el.className += " buffer-item-deleted";
            }

            switch (buffer.type) {
                case gl.ARRAY_BUFFER:
                    el.className += " buffer-item-array";
                    break;
                case gl.ELEMENT_ARRAY_BUFFER:
                    el.className += " buffer-item-element-array";
                    break;
            }

            var number = document.createElement("div");
            number.className = "buffer-item-number";
            number.textContent = buffer.getName();
            el.appendChild(number);

            buffer.modified.addListener(this, function (buffer) {
                // TODO: refresh view if selected
                //console.log("refresh buffer row");

                // Type may have changed - update it
                el.className = el.className.replace(" buffer-item-array", "").replace(" buffer-item-element-array", "");
                switch (buffer.type) {
                    case gl.ARRAY_BUFFER:
                        el.className += " buffer-item-array";
                        break;
                    case gl.ELEMENT_ARRAY_BUFFER:
                        el.className += " buffer-item-element-array";
                        break;
                }
            });
            buffer.deleted.addListener(this, function (buffer) {
                el.className += " buffer-item-deleted";
            });
        });
        this.bufferView = new BufferView(w, this.el);

        this.listing.valueSelected.addListener(this, function (buffer) {
            this.bufferView.setBuffer(buffer);
        });

        var scrollStates = {};
        this.lostFocus.addListener(this, function () {
            scrollStates.listing = this.listing.getScrollState();
        });
        this.gainedFocus.addListener(this, function () {
            this.listing.setScrollState(scrollStates.listing);
        });

        // Append buffers already present
        var context = w.context;
        var buffers = context.resources.getBuffers();
        for (var n = 0; n < buffers.length; n++) {
            var buffer = buffers[n];
            this.listing.appendValue(buffer);
        }

        // Listen for changes
        context.resources.resourceRegistered.addListener(this, function (resource) {
            if (base.typename(resource.target) == "WebGLBuffer") {
                this.listing.appendValue(resource);
            }
        });

        // When we lose focus, reselect the buffer - shouldn't mess with things too much, and also keeps the DOM small if the user had expanded things
        this.lostFocus.addListener(this, function () {
            if (this.listing.previousSelection) {
                this.listing.selectValue(this.listing.previousSelection.value);
            }
        });

        this.layout = function () {
            this.bufferView.layout();
        };

        this.refresh = function () {
            this.bufferView.setBuffer(this.bufferView.currentBuffer);
        };
    };

    return BuffersTab;
});
