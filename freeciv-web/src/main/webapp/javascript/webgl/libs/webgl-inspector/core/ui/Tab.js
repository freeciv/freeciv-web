define([
        '../shared/EventSource',
    ], function (
        EventSource
    ) {

    var Tab = function (w, container, name) {
        this.name = name;
        this.hasFocus = false;

        var el = this.el = document.createElement("div");
        el.className = "window-tab-root";
        container.appendChild(el);

        this.gainedFocus = new EventSource("gainedFocus");
        this.lostFocus = new EventSource("lostFocus");
    };
    Tab.prototype.gainFocus = function () {
        this.hasFocus = true;
        this.el.className += " window-tab-selected";
        this.gainedFocus.fire();
    };
    Tab.prototype.loseFocus = function () {
        this.lostFocus.fire();
        this.hasFocus = false;
        this.el.className = this.el.className.replace(" window-tab-selected", "");
    };

    // Variadic.
    Tab.eleClasses = function (eleType) {
      var ele = document.createElement(eleType);
      for (var i = 1, len = arguments.length; i < len; ++i) {
        ele.classList.add(arguments[i]);
      }
      return ele;
    };

    Tab.divClass = function (klass, comment) {
        var div = Tab.eleClasses("div", klass);
        if (comment) div.appendChild(document.createComment(" "+comment+" "));
        return div;
    };

    Tab.windowLeft = function (options) {
        var left = Tab.divClass("window-left");
        left.appendChild(Tab.divClass("window-left-listing", options.listing));
        left.appendChild(Tab.divClass("window-left-toolbar", options.toolbar));
        return left;
    };

    Tab.inspector = function () {
        var canvas = Tab.eleClasses("canvas", "gli-reset", "surface-inspector-pixel");
        var statusbar = Tab.divClass("surface-inspector-statusbar");
        var inspector = Tab.divClass("window-inspector");

        canvas.width = canvas.height = 1;

        statusbar.appendChild(canvas);
        statusbar.appendChild(Tab.eleClasses("span", "surface-inspector-location"));
        inspector.appendChild(Tab.divClass("surface-inspector-toolbar", "toolbar"));
        inspector.appendChild(Tab.divClass("surface-inspector-inner", "inspector"));
        inspector.appendChild(statusbar);

        return inspector;
    };

    Tab.genericLeftRightView = function () {
        var outer = Tab.divClass("window-right-outer");
        var right = Tab.divClass("window-right");

        right.appendChild(Tab.divClass("window-right-inner", "scrolling content"));
        outer.appendChild(right);
        outer.appendChild(Tab.windowLeft({ listing: "state list", toolbar: "toolbar" }));

        return outer;
    };

    return Tab;
});
