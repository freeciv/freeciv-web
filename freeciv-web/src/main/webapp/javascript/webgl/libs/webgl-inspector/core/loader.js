var gliloader = {};

(function() {
    function insertHeaderNode(node) {
        var document = window.document;
        var targets = [document.body, document.head, document.documentElement];
        for (var n = 0; n < targets.length; n++) {
            var target = targets[n];
            if (target) {
                if (target.firstElementChild) {
                    target.insertBefore(node, target.firstElementChild);
                } else {
                    target.appendChild(node);
                }
                break;
            }
        }
    };

    function insertStylesheet(url) {
        var document = window.document;
        var link = document.createElement("link");
        link.rel = "stylesheet";
        link.href = url;
        insertHeaderNode(link);
        return link;
    };

    function insertScript(url, callback, attributes) {
        var document = window.document;
        var script = document.createElement("script");
        if (attributes) {
          Object.keys(attributes).forEach(function(key) {
              script.setAttribute(key, attributes[key]);
          });
        }
        script.type = "text/javascript";
        script.src = url;

        //insertHeaderNode(script);
        var doc = document;
        (doc.head || doc.body || doc.documentElement).appendChild(script);

        script.onreadystatechange = function () {
            if (("loaded" === script.readyState || "complete" === script.readyState) && !script.loadCalled) {
                this.loadCalled = true;
                callback();
            }
        };
        script.onload = function () {
            if (!script.loadCalled) {
                this.loadCalled = true;
                callback();
            }
        };
        return script;
    };

    function load(unused, callback) {
        var pathRoot = gliloader.pathRoot;

        insertScript(pathRoot + "gli-require-loader.js", function() {
            callback();
        });
    }

    gliloader.load = load;

}());

