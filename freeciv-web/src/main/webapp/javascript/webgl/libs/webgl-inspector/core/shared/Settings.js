define(function() {

    var Settings = function () {
        this.global = {
            captureOn: [],
            showHud: false,
            popupHud: false,
            enableTimeline: true
        };

        this.session = {
            showRedundantCalls: true,
            showDepthDiscarded: true,
            enableTimeline: false,
            hudVisible: false,
            hudHeight: 275,
            hudPopupWidth: 1200,
            hudPopupHeight: 500,
            traceSplitter: 400,
            textureSplitter: 240,
            counterToggles: {}
        };

        this.load();
    };

    Settings.prototype.setGlobals = function (globals) {
        for (var n in globals) {
            this.global[n] = globals[n];
        }
    };

    Settings.prototype.load = function () {
        var sessionString = localStorage["__gli"];
        if (sessionString) {
            var sessionObj = JSON.parse(sessionString);
            for (var n in sessionObj) {
                this.session[n] = sessionObj[n];
            }
        }
    };
    Settings.prototype.save = function () {
        localStorage["__gli"] = JSON.stringify(this.session);
    };

    return new Settings();
});
