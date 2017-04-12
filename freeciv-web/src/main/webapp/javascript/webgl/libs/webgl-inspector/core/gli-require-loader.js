(function() {

    var pathRoot;
    var scripts = document.getElementsByTagName("script");
    for (var n = 0; n < scripts.length; n++) {
        var scriptTag = scripts[n];
        var src = scriptTag.src.toLowerCase();
        if (/\/gli-require-loader\.js$/.test(src)) {
            // Found ourself - strip our name and set the root
            var index = src.lastIndexOf("gli-require-loader.js");
            pathRoot = scriptTag.src.substring(0, index);
            break;
        }
    }

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

        insertHeaderNode(script);

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
        window.gliCssRoot = pathRoot;

        insertScript(pathRoot + "dependencies/require.js", function() {
           require.config({
               baseUrl: pathRoot,
           });
           require.nextTick = function(fn) { fn(); };
           require(['./gli'], function() {
               window.require = undefined;
               window.define = undefined;
               callback();
           });
        });
    }

    load([], function() {});

}());

