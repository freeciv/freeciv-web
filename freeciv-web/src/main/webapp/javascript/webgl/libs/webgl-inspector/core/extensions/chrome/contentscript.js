//var debugMode = true;
if (!window["debugMode"]) {
    window["debugMode"] = false;
}
var hasInjected = false;
var sessionKey = "WebGLInspectorEnabled" + (debugMode ? "Debug" : "");

function getExtensionURL() {
    if (window["chrome"]) {
        return chrome.extension.getURL("");
    } else if (window["safari"]) {
        return safari.extension.baseURI;
    }
};

function notifyPresent() {
    if (window["chrome"]) {
        chrome.extension.sendRequest({ present: true }, function (response) { });
    } else if (window["safari"]) {
        safari.self.tab.dispatchMessage("notifyPresent", { present: true });
    }
};

function notifyEnabled(present) {
    if (window["chrome"]) {
        chrome.extension.sendRequest({ present: present }, function (response) { });
    } else if (window["safari"]) {
        safari.self.tab.dispatchMessage("message", { present: present });
    }
};

function listenForMessage(callback) {
    if (window["chrome"]) {
        chrome.extension.onRequest.addListener(callback);
    } else if (window["safari"]) {
        safari.self.addEventListener("message", callback);
    }
};

function insertHeaderNode(node) {
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
    var link = document.createElement("link");
    link.rel = "stylesheet";
    link.href = url;
    insertHeaderNode(link);
    return link;
};

function insertScript(url) {
    var script = document.createElement("script");
    script.type = "text/javascript";

    // Place the entire inspector into a <script> tag instead of referencing
    // the URL - this enables synchronous loading and inspection of code
    // creating contexts before dom ready.
    // It's nasty, though, as dev tools shows it as another script group on
    // the main page.
    // script.src = url;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, false);
    xhr.send('');
    script.text = xhr.responseText;

    insertHeaderNode(script);
    return script;
};


// If we're reloading after enabling the inspector, load immediately
if (sessionStorage[sessionKey] == "yes") {
    hasInjected = true;

    var pathRoot = getExtensionURL();

    if (debugMode) {
        // We have the loader.js file ready to help out
        gliloader.pathRoot = pathRoot;
        gliloader.load(["loader", "host", "replay", "ui"], function () {
            // ?
        });
    } else {
        var jsurl = pathRoot + "gli.all.js";
        var cssurl = pathRoot + "gli.all.css";

        insertStylesheet(cssurl);
        insertScript(jsurl);
    }

    notifyPresent();
}

// Once the DOM is ready, bind to the content event
document.addEventListener("DOMContentLoaded", function () {
    listenForMessage(function (msg) {
        if (sessionStorage[sessionKey] == "yes") {
            sessionStorage[sessionKey] = "no";
        } else {
            sessionStorage[sessionKey] = "yes"
        }
        window.location.reload();
    });

    document.addEventListener("WebGLEnabledEvent", function () {
        notifyEnabled(true);
    }, false);

    // Setup message passing for path root interchange
    if (sessionStorage[sessionKey] == "yes") {
        if (!document.getElementById("__webglpathroot")) {
            var pathElement = document.createElement("div");
            pathElement.id = "__webglpathroot";
            pathElement.style.display = "none";
            document.body.appendChild(pathElement);
        }

        setTimeout(function () {
            var readyEvent = document.createEvent("Event");
            readyEvent.initEvent("WebGLInspectorReadyEvent", true, true);
            var pathElement = document.getElementById("__webglpathroot");
            pathElement.innerText = getExtensionURL();
            document.body.dispatchEvent(readyEvent);
        }, 0);
    }
}, false);

// Always start absent
notifyEnabled(false);

// --------- NOTE: THIS FUNCTION IS INJECTED INTO THE PAGE DIRECTLY ---------
// This relies on us being executed before the dom is ready so that we can overwrite any calls
// to canvas.getContext. When a call is made, we fire off an event that is handled in our extension
// above (as chrome.extension.* is not available from the page).
function main() {
    // Create enabled event
    function fireEnabledEvent() {
        // If gli exists, then we are already present and shouldn't do anything
        if (!window.gli) {
            setTimeout(function () {
                var enabledEvent = document.createEvent("Event");
                enabledEvent.initEvent("WebGLEnabledEvent", true, true);
                document.dispatchEvent(enabledEvent);
            }, 0);
        } else {
            //console.log("WebGL Inspector already embedded on the page - disabling extension");
        }
    };

    // Grab the path root from the extension
    document.addEventListener("WebGLInspectorReadyEvent", function (e) {
        var pathElement = document.getElementById("__webglpathroot");
        if (window["gliloader"]) {
            gliloader.pathRoot = pathElement.innerText;
        } else {
            // TODO: more?
            window.gliCssUrl = pathElement.innerText + "gli.all.css";
        }
    }, false);

    // Rewrite getContext to snoop for webgl
    var originalGetContext = HTMLCanvasElement.prototype.getContext;
    if (!HTMLCanvasElement.prototype.getContextRaw) {
        HTMLCanvasElement.prototype.getContextRaw = originalGetContext;
    }
    HTMLCanvasElement.prototype.getContext = function () {
        var ignoreCanvas = this.internalInspectorSurface;
        if (ignoreCanvas) {
            return originalGetContext.apply(this, arguments);
        }

        var result = originalGetContext.apply(this, arguments);
        if (result == null) {
            return null;
        }

        var contextNames = ["webgl", "webgl2"];
        var requestingWebGL = contextNames.indexOf(arguments[0]) != -1;
        if (requestingWebGL) {
            // Page is requesting a WebGL context!
            fireEnabledEvent(this);

            // If we are injected, inspect this context
            if (window.gli) {
                if (gli.host.inspectContext) {
                    // TODO: pull options from extension
                    result = gli.host.inspectContext(this, result);
                    // NOTE: execute in a timeout so that if the dom is not yet
                    // loaded this won't error out.
                    window.setTimeout(function() {
                        var hostUI = new gli.host.HostUI(result);
                        result.hostUI = hostUI; // just so we can access it later for debugging
                    }, 0);
                }
            }
        }

        return result;
    };
}

// TODO: better content type checks (if possible?)
var likelyHTML = true;
if (document.xmlVersion) {
    likelyHTML = false;
}

if (likelyHTML) {
    var script = document.createElement('script');
    script.appendChild(document.createTextNode('(' + main + ')();'));
    if (document.body || document.head || document.documentElement) {
        // NOTE: only valid html documents get body/head, so this is a nice way to not inject on them
        (document.body || document.head || document.documentElement).appendChild(script);
    }
}
