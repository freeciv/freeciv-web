define([
        '../../Tab',
        './TimelineView',
    ], function (
        Tab,
        TimelineView
    ) {

    var TimelineTab = function (w) {
        var outer = Tab.divClass('window-right-outer');
        var right = Tab.divClass('window-right');

        right.appendChild(Tab.eleClasses('canvas', 'gli-reset', 'timeline-canvas'));
        outer.appendChild(right);
        outer.appendChild(Tab.divClass('window-left'));

        this.el.appendChild(outer);

        this.timelineView = new TimelineView(w, this.el);

        this.lostFocus.addListener(this, function () {
            this.timelineView.suspendUpdating();
        });
        this.gainedFocus.addListener(this, function () {
            this.timelineView.resumeUpdating();
        });
    };

    return TimelineTab;
});
