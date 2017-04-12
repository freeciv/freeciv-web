define([
        '../../Tab',
        '../../shared/LeftListing',
        './TraceView',
    ], function (
        Tab,
        LeftListing,
        TraceView
    ) {

    var TraceTab = function (w) {
        var outer = Tab.divClass("window-right-outer");
        var right = Tab.divClass("window-right");
        var inspector = Tab.inspector();
        inspector.classList.add("window-trace-inspector");
        var traceOuter = Tab.divClass("window-trace-outer");
        var trace = Tab.divClass("window-trace");
        var left = Tab.windowLeft({ listing: "frame list", toolbar: "toolbar" });

        trace.appendChild(Tab.divClass("trace-minibar", "minibar"));
        trace.appendChild(Tab.divClass("trace-listing", "call trace"));
        traceOuter.appendChild(trace);
        right.appendChild(inspector);
        right.appendChild(traceOuter);
        outer.appendChild(right);
        outer.appendChild(left);

        this.el.appendChild(outer);

        this.listing = new LeftListing(w, this.el, "frame", function (el, frame) {
            var canvas = document.createElement("canvas");
            canvas.className = "gli-reset frame-item-preview";
            canvas.style.cursor = "pointer";
            canvas.width = 80;
            canvas.height = frame.screenshot.height / frame.screenshot.width * 80;

            // Draw the data - hacky, but easiest?
            var ctx2d = canvas.getContext("2d");
            ctx2d.drawImage(frame.screenshot, 0, 0, canvas.width, canvas.height);

            el.appendChild(canvas);

            var number = document.createElement("div");
            number.className = "frame-item-number";
            number.textContent = frame.frameNumber;
            el.appendChild(number);
        });
        this.traceView = new TraceView(w, this.el);

        this.listing.valueSelected.addListener(this, function (frame) {
            this.traceView.setFrame(frame);
        });

        var scrollStates = {
            listing: null,
            traceView: null,
        };
        this.lostFocus.addListener(this, function () {
            scrollStates.listing = this.listing.getScrollState();
            scrollStates.traceView = this.traceView.getScrollState();
        });
        this.gainedFocus.addListener(this, function () {
            this.listing.setScrollState(scrollStates.listing);
            this.traceView.setScrollState(scrollStates.traceView);
        });

        var context = w.context;
        for (var n = 0; n < context.frames.length; n++) {
            var frame = context.frames[n];
            this.listing.appendValue(frame);
        }
        if (context.frames.length > 0) {
            this.listing.selectValue(context.frames[context.frames.length - 1]);
        }

        this.layout = function () {
            if (this.traceView.layout) this.traceView.layout();
        };

    };

    return TraceTab;
});
