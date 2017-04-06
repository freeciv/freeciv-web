define([
        '../../../shared/Base',
        '../../../host/Resource',
        '../../Tab',
        '../../shared/LeftListing',
        './VertexArrayView',
    ], function (
        base,
        Resource,
        Tab,
        LeftListing,
        VertexArrayView
    ) {

    var VertexArraysTab = function (w) {
        var outer = Tab.divClass("window-right-outer");
        var right = Tab.divClass("window-right");
        var inspector = Tab.divClass("window-inspector");
        inspector.classList.add("window-vertexarray-inspector");
        var vertexArray = Tab.divClass("window-vertexarray-outer");

        inspector.appendChild(Tab.divClass("surface-inspector-toolbar", "toolbar"));
        inspector.appendChild(Tab.divClass("surface-inspector-inner", "inspector"));
        inspector.appendChild(Tab.divClass("surface-inspector-statusbar"));
        vertexArray.appendChild(Tab.divClass("vertexarray-listing", "scrolling contents"));
        right.appendChild(inspector);
        right.appendChild(vertexArray);
        outer.appendChild(right);
        outer.appendChild(Tab.windowLeft({ listing: "frame list", toolbar: "buttons"}));

        this.el.appendChild(outer);

        this.listing = new LeftListing(w, this.el, "VertexArray", function (el, vertexArray) {
            var gl = w.context;

            if (vertexArray.status == Resource.DEAD) {
                el.className += " vertexarray-item-deleted";
            }

            var number = document.createElement("div");
            number.className = "vertexarray-item-number";
            number.textContent = vertexArray.getName();
            el.appendChild(number);

            vertexArray.modified.addListener(this, function (vertexArray) {
                // TODO: refresh view if selected
                //console.log("refresh vertexArray row");

                // Type may have changed - update it
                el.className = el.className.replace(" vertexarray-item-array", "").replace(" vertexarray-item-element-array", "");
            });
            vertexArray.deleted.addListener(this, function (vertexArray) {
                el.className += " vertexarray-item-deleted";
            });
        });
        this.vertexArrayView = new VertexArrayView(w, this.el);

        this.listing.valueSelected.addListener(this, function (vertexArray) {
            this.vertexArrayView.setVertexArray(vertexArray);
        });

        var scrollStates = {};
        this.lostFocus.addListener(this, function () {
            scrollStates.listing = this.listing.getScrollState();
        });
        this.gainedFocus.addListener(this, function () {
            this.listing.setScrollState(scrollStates.listing);
        });

        // Append vertexArrays already present
        var context = w.context;
        var vertexArrays = context.resources.getResources("WebGLVertexArrayObject");
        for (var n = 0; n < vertexArrays.length; n++) {
            var vertexArray = vertexArrays[n];
            this.listing.appendValue(vertexArray);
        }

        // Listen for changes
        context.resources.resourceRegistered.addListener(this, function (resource) {
            if (base.typename(resource.target) == "WebGLVertexArrayObject") {
                this.listing.appendValue(resource);
            }
        });

        // When we lose focus, reselect the vertexArray - shouldn't mess with things too much, and also keeps the DOM small if the user had expanded things
        this.lostFocus.addListener(this, function () {
            if (this.listing.previousSelection) {
                this.listing.selectValue(this.listing.previousSelection.value);
            }
        });

        this.layout = function () {
            this.vertexArrayView.layout();
        };

        this.refresh = function () {
            this.vertexArrayView.setVertexArray(this.vertexArrayView.currentVertexArray);
        };
    };

    return VertexArraysTab;
});
