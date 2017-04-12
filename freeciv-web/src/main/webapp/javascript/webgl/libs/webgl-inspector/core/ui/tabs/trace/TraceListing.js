define([
        '../../../shared/Base',
        '../../../shared/Info',
        '../../drawinfo/DrawInfo',
        '../../shared/PopupWindow',
        '../../shared/TraceLine',
    ], function (
        base,
        info,
        DrawInfo,
        PopupWindow,
        traceLine
    ) {

    var TraceListing = function (view, w, elementRoot) {
        var self = this;
        this.view = view;
        this.window = w;
        this.elements = {
            list: elementRoot.getElementsByClassName("trace-listing")[0]
        };

        this.calls = [];

        this.activeCall = null;
    };

    TraceListing.prototype.reset = function () {
        this.activeCall = null;
        this.calls.length = 0;

        // Swap out the element for faster clear
        var oldList = this.elements.list;
        var newList = document.createElement("div");
        newList.className = "trace-listing";
        newList.style.cssText = oldList.style.cssText;
        var parentNode = oldList.parentNode;
        parentNode.replaceChild(newList, oldList);
        this.elements.list = newList;
    };

    function addCall(listing, container, frame, call) {
        var document = listing.window.document;
        var gl = listing.window.context;

        // <div class="trace-call">
        //     <div class="trace-call-icon">
        //         &nbsp;
        //     </div>
        //     <div class="trace-call-line">
        //         hello world
        //     </div>
        //     <div class="trace-call-actions">
        //         ??
        //     </div>
        // </div>

        var el = document.createElement("div");
        el.className = "trace-call";

        var icon = document.createElement("div");
        icon.className = "trace-call-icon";
        el.appendChild(icon);

        var ordinal = document.createElement("div");
        ordinal.className = "trace-call-ordinal";
        ordinal.textContent = call.ordinal;
        el.appendChild(ordinal);

        // Actions must go before line for floating to work right
        var funcInfo = info.functions[call.name];
        if (funcInfo.type == info.FunctionType.DRAW) {
            var actions = document.createElement("div");
            actions.className = "trace-call-actions";

            var infoAction = document.createElement("div");
            infoAction.className = "trace-call-action trace-call-action-info";
            infoAction.title = "View draw information";
            actions.appendChild(infoAction);
            infoAction.onclick = function (e) {
                PopupWindow.show(listing.window.context, DrawInfo, "drawInfo", function (popup) {
                    popup.inspectDrawCall(frame, call);
                });
                e.preventDefault();
                e.stopPropagation();
            };

            var isolateAction = document.createElement("div");
            isolateAction.className = "trace-call-action trace-call-action-isolate";
            isolateAction.title = "Run draw call isolated";
            actions.appendChild(isolateAction);
            isolateAction.onclick = function (e) {
                listing.window.controller.runIsolatedDraw(frame, call);
                //listing.window.controller.runDepthDraw(frame, call);
                listing.view.minibar.refreshState(true);
                e.preventDefault();
                e.stopPropagation();
            };

            el.appendChild(actions);
        }

        var line = document.createElement("div");
        line.className = "trace-call-line";
        traceLine.populateCallLine(listing.window, call, line);
        el.appendChild(line);

        if (call.isRedundant) {
            el.className += " trace-call-redundant";
        }
        if (call.error) {
            el.className += " trace-call-error";

            var errorString = info.enumToString(call.error);
            var extraInfo = document.createElement("div");
            extraInfo.className = "trace-call-extra";
            var errorName = document.createElement("span");
            errorName.textContent = errorString;
            extraInfo.appendChild(errorName);
            el.appendChild(extraInfo);

            // If there is a stack, add to tooltip
            if (call.stack) {
                var line0 = call.stack[0];
                extraInfo.title = line0;
            }
        }

        container.appendChild(el);

        var index = listing.calls.length;
        el.onclick = function () {
            listing.view.minibar.stepUntil(index);
        };

        listing.calls.push({
            call: call,
            element: el,
            icon: icon
        });
    };

    TraceListing.prototype.setFrame = function (frame) {
        this.reset();

        var container = document.createDocumentFragment();

        for (var n = 0; n < frame.calls.length; n++) {
            var call = frame.calls[n];
            addCall(this, container, frame, call);
        }

        this.elements.list.appendChild(container);

        this.elements.list.scrollTop = 0;
    };

    TraceListing.prototype.setActiveCall = function (callIndex, ignoreScroll) {
        if (this.activeCall == callIndex) {
            return;
        }

        if (this.activeCall != null) {
            // Clean up previous changes
            var oldel = this.calls[this.activeCall].element;
            oldel.className = oldel.className.replace("trace-call-highlighted", "");
            var oldicon = this.calls[this.activeCall].icon;
            oldicon.className = oldicon.className.replace("trace-call-icon-active", "");
        }

        this.activeCall = callIndex;

        this.calls[callIndex].element.classList.add("trace-call-highlighted");
        this.calls[callIndex].icon.classList.add("trace-call-icon-active");

        if (!ignoreScroll) {
            this.scrollToCall(callIndex);
        }
    };

    TraceListing.prototype.scrollToCall = function (callIndex) {
        var el = this.calls[callIndex].icon;
        base.scrollIntoViewIfNeeded(el);
    };

    TraceListing.prototype.getScrollState = function () {
        return {
            list: this.elements.list.scrollTop
        };
    };

    TraceListing.prototype.setScrollState = function (state) {
        this.elements.list.scrollTop = state.list;
    };

    return TraceListing;

});
