define([
        '../../Tab',
        './StateView',
    ], function (
        Tab,
        StateView
    ) {

    var divClass = Tab.divClass;

    var StateTab = function (w) {
        var outer = divClass("window-whole-outer");
        var whole = divClass("window-whole");
        whole.appendChild(divClass("window-whole-inner", "scrolling contents"));
        outer.appendChild(whole);
        this.el.appendChild(outer);

        this.stateView = new StateView(w, this.el);

        this.stateView.setState();

        this.refresh = function () {
            this.stateView.setState();
        };
    };

    return StateTab;
});
