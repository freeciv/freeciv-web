define([
        '../../shared/Base',
        '../../shared/EventSource',
    ], function (
        base,
        EventSource
    ) {

    var LeftListing = function (w, elementRoot, cssBase, itemGenerator) {
        var self = this;
        this.window = w;
        this.elements = {
            list: elementRoot.getElementsByClassName("window-left-listing")[0],
            toolbar: elementRoot.getElementsByClassName("window-left-toolbar")[0]
        };

        // Hide toolbar until the first button is added
        this.toolbarHeight = this.elements.toolbar.style.height;
        this.elements.toolbar.style.display = "none";
        this.elements.toolbar.style.height = "0px";
        this.elements.list.style.bottom = "0px";

        this.cssBase = cssBase;
        this.itemGenerator = itemGenerator;

        this.valueEntries = [];

        this.previousSelection = null;

        this.valueSelected = new EventSource("valueSelected");
    };

    LeftListing.prototype.addButton = function(name) {
        // Show the toolbar
        this.elements.toolbar.style.display = "";
        this.elements.toolbar.style.height = this.toolbarHeight;
        this.elements.list.style.bottom = this.toolbarHeight;

        var event = new EventSource("buttonClicked");

        var buttonEl = document.createElement("div");
        buttonEl.className = "mini-button";

        var leftEl = document.createElement("div");
        leftEl.className = "mini-button-left";
        buttonEl.appendChild(leftEl);

        var spanEl = document.createElement("div");
        spanEl.className = "mini-button-span";
        spanEl.textContent = name;
        buttonEl.appendChild(spanEl);

        var rightEl = document.createElement("div");
        rightEl.className = "mini-button-right";
        buttonEl.appendChild(rightEl);

        this.elements.toolbar.appendChild(buttonEl);

        buttonEl.onclick = function (e) {
            event.fire();
            e.preventDefault();
            e.stopPropagation();
        };

        return event;
    };

    LeftListing.prototype.appendValue = function (value) {
        var self = this;
        var document = this.window.document;

        // <div class="XXXX-item">
        //     ??
        // </div>

        var el = document.createElement("div");
        el.className = this.cssBase + "-item listing-item";

        this.itemGenerator(el, value);

        this.elements.list.appendChild(el);

        el.onclick = function () {
            self.selectValue(value);
        };

        this.valueEntries.push({
            value: value,
            element: el
        });
        value.uielement = el;
    };

    LeftListing.prototype.resort = function () {
        // TODO: restort
    };

    LeftListing.prototype.removeValue = function (value) {
    };

    LeftListing.prototype.selectValue = function (value) {
        if (this.previousSelection) {
            var el = this.previousSelection.element;
            el.className = el.className.replace(" " + this.cssBase + "-item-selected listing-item-selected", "");
            this.previousSelection = null;
        }

        var valueObj = null;
        for (var n = 0; n < this.valueEntries.length; n++) {
            if (this.valueEntries[n].value == value) {
                valueObj = this.valueEntries[n];
                break;
            }
        }
        this.previousSelection = valueObj;
        if (valueObj) {
            valueObj.element.className += " " + this.cssBase + "-item-selected listing-item-selected";
        }

        if (value) {
            base.scrollIntoViewIfNeeded(value.uielement);
        }

        this.valueSelected.fire(value);
    };

    LeftListing.prototype.getScrollState = function () {
        return {
            list: this.elements.list.scrollTop
        };
    };

    LeftListing.prototype.setScrollState = function (state) {
        if (!state) {
            return;
        }
        this.elements.list.scrollTop = state.list;
    };

    return LeftListing;

});
