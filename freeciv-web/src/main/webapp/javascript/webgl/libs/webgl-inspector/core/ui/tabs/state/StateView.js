define([
        '../../../shared/Info',
        '../../../host/StateSnapshot',
        '../../Helpers',
    ], function (
        info,
        StateSnapshot,
        helpers
    ) {

    var StateView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-whole-inner")[0]
        };
    };

    function generateStateDisplay(w, el, state) {
        var gl = w.context;

        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = "State Snapshot";
        el.appendChild(titleDiv);

        var table = document.createElement("table");
        table.className = "info-parameters";

        var stateParameters = info.stateParameters;
        for (var n = 0; n < stateParameters.length; n++) {
            var param = stateParameters[n];
            helpers.appendStateParameterRow(w, gl, table, state, param);
        }

        el.appendChild(table);

        titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = "Canvas Attributes";
        el.appendChild(titleDiv);

        table = document.createElement("table");
        table.className = "info-parameters";
        var attribs = gl.getContextAttributes();
        Object.keys(attribs).forEach(function(key) {
            helpers.appendContextAttributeRow(w, gl, table, attribs, key);
        });

        el.appendChild(table);
    };

    StateView.prototype.setState = function () {
        var rawgl = this.window.context.rawgl;
        var state = null;
        switch (this.window.activeVersion) {
            case null:
                state = new StateSnapshot(rawgl);
                break;
            case "current":
                state = this.window.controller.getCurrentState();
                break;
        }

        var node = this.elements.view;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }

        if (state) {
            generateStateDisplay(this.window, this.elements.view, state);
        }

        this.elements.view.scrollTop = 0;
    };

    return StateView;
});
