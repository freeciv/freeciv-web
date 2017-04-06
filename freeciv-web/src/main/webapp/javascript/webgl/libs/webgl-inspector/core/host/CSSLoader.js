define([], function() {

    function injectCSS(filename, injectState) {
        var doc = injectState.window.document;
        var url = injectState.pathRoot + filename;
        if ((url.indexOf("http://") == 0) || (url.indexOf("file://") == 0) || (url.indexOf("chrome-extension://") == 0)) {
            var link = doc.createElement("link");
            link.rel = "stylesheet";
            link.href = url;
            (doc.head || doc.body || doc.documentElement).appendChild(link);
        } else {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        (doc.head || doc.body || doc.documentElement).appendChild(style);
                    }
                }
            };
            xhr.open("GET", url, true);
            xhr.send(null);
        }
    };

    function load(w) {

        if (window.gliCssRoot) {
            const injectState = {
                window: w,
                pathRoot: window.gliCssRoot,
            };

            injectCSS("./dependencies/reset-context.css", injectState);
            injectCSS("./dependencies/syntaxhighlighter_3.0.83/shCore.css", injectState);
            injectCSS("./dependencies/syntaxhighlighter_3.0.83/shThemeDefault.css", injectState);
            injectCSS("./ui/gli.css", injectState);
        } else if (window.gliCssUrl) {
            var targets = [w.document.body, w.document.head, w.document.documentElement];
            for (var n = 0; n < targets.length; n++) {
                var target = targets[n];
                if (target) {
                    var link = w.document.createElement("link");
                    link.rel = "stylesheet";
                    link.href = window["gliCssUrl"];
                    target.appendChild(link);
                    break;
                }
            }

        }
    }

    return {
        load: load,
    };

});


