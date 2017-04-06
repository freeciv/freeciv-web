define([
        '../shared/Info',
        '../shared/Settings',
        '../shared/SplitterBar',
        '../ui/Window',
        './CSSLoader',
        './CaptureContext',
    ], function (
        info,
        settings,
        SplitterBar,
        Window,
        cssLoader,
        captureContext
    ) {

    function requestCapture(context) {
        context.requestCapture(function (context, frame) {
            for (var n = 0, len = frame.calls.length; n < len; ++n) {
                var call = frame.calls[n];
                call.info = info.functions[call.name];
            }
            context.frames.push(frame);
            if (context.ui) {
                context.ui.appendFrame(frame);
            }
        });
    };

    var InlineWindow = function (context) {
        var self = this;
        this.context = context;

        var w = this.element = document.createElement("div");
        w.className = "yui3-cssreset inline-window-host";

        // TODO: validate height better?
        var hudHeight = settings.session.hudHeight;
        hudHeight = Math.max(112, Math.min(hudHeight, window.innerHeight - 42));
        w.style.height = hudHeight + "px";

        document.body.appendChild(w);

        this.splitter = new SplitterBar(w, "horizontal", 112, 42, null, function (newHeight) {
            context.ui.layout();
            settings.session.hudHeight = newHeight;
            settings.save();
        });

        cssLoader.load(window);

        context.ui = new Window(context, window.document, w);

        this.opened = true;
        settings.session.hudVisible = true;
        settings.save();
    };
    InlineWindow.prototype.focus = function () {
    };
    InlineWindow.prototype.close = function () {
        if (this.element) {
            document.body.removeChild(this.element);

            this.context.ui = null;
            this.context.window = null;

            this.element = null;
            this.context = null;
            this.splitter = null;
            this.opened = false;
            settings.session.hudVisible = false;
            settings.save();
        }
    };
    InlineWindow.prototype.isOpened = function () {
        return this.opened;
    };
    InlineWindow.prototype.toggle = function () {
        if (this.opened) {
            this.element.style.display = "none";
        } else {
            this.element.style.display = "";
        }
        this.opened = !this.opened;
        settings.session.hudVisible = this.opened;
        settings.save();

        var self = this;
        captureContext.setTimeout(function () {
            self.context.ui.layout();
        }, 0);
    };

    var PopupWindow = function (context) {
        var self = this;
        this.context = context;

        settings.session.hudVisible = true;
        settings.save();

        var startupWidth = settings.session.hudPopupWidth ? settings.session.hudPopupWidth : 1000;
        var startupHeight = settings.session.hudPopupHeight ? settings.session.hudPopupHeight : 500;
        var w = this.browserWindow = window.open("about:blank", "_blank", "location=no,menubar=no,scrollbars=no,status=no,toolbar=no,innerWidth=" + startupWidth + ",innerHeight=" + startupHeight);
        w.document.writeln("<html><head><title>WebGL Inspector</title></head><body class='yui3-cssreset' style='margin: 0px; padding: 0px;'></body></html>");

        window.addEventListener("beforeunload", function () {
            w.close();
        }, false);

        w.addEventListener("unload", function () {
            context.window.browserWindow.opener.focus();
            context.window = null;
        }, false);

        // Key handler to listen for state changes
        w.document.addEventListener("keydown", function (event) {
            var handled = false;
            switch (event.keyCode) {
                case 122: // F11
                    w.opener.focus();
                    handled = true;
                    break;
                case 123: // F12
                    requestCapture(context);
                    handled = true;
                    break;
            };

            if (handled) {
                event.preventDefault();
                event.stopPropagation();
            }
        }, false);

        w.addEventListener("resize", function () {
            context.ui.layout();
            settings.session.hudPopupWidth = w.innerWidth;
            settings.session.hudPopupHeight = w.innerHeight;
            settings.save()
        }, false);

        cssLoader.load(w);

        // I don't think this is needed
        // w.gli = window.gli;

        captureContext.setTimeout(function () {
            context.ui = new Window(context, w.document);
        }, 0);
    };
    PopupWindow.prototype.focus = function () {
        this.browserWindow.focus();
    };
    PopupWindow.prototype.close = function () {
        this.browserWindow.close();
        this.browserWindow = null;
        this.context.window = null;
        settings.session.hudVisible = false;
        settings.save();
    };
    PopupWindow.prototype.isOpened = function () {
        return this.browserWindow && !this.browserWindow.closed;
    };

    function requestFullUI(context, hiddenByDefault) {
        if (settings.global.popupHud) {
            if (context.window) {
                if (context.window.isOpened()) {
                    context.window.focus();
                } else {
                    context.window.close();
                }
            }

            if (!context.window) {
                if (!hiddenByDefault) {
                    context.window = new PopupWindow(context);
                }
            }
        } else {
            if (!context.window) {
                context.window = new InlineWindow(context);
                if (hiddenByDefault) {
                    context.window.toggle();
                }
            } else {
                context.window.toggle();
            }
        }
    };

    function injectUI(ui) {
        var context = ui.context;

        var button1 = document.createElement("div");
        button1.style.zIndex = "99999";
        button1.style.position = "absolute";
        button1.style.right = "38px";
        button1.style.top = "5px";
        button1.style.cursor = "pointer";
        button1.style.backgroundColor = "rgba(50,10,10,0.8)";
        button1.style.color = "red";
        button1.style.fontSize = "8pt";
        button1.style.fontFamily = "Monaco, 'Andale Mono', 'Monotype.com', monospace";
        button1.style.fontWeight = "bold";
        button1.style.padding = "5px";
        button1.style.border = "1px solid red";
        button1.style.webkitUserSelect = "none";
        button1.style.mozUserSelect = "none";
        button1.title = "Capture frame (F12)";
        button1.textContent = "Capture";
        document.body.appendChild(button1);

        button1.addEventListener("click", function () {
            requestCapture(context);
        });

        var button2 = document.createElement("div");
        button2.style.zIndex = "99999";
        button2.style.position = "absolute";
        button2.style.right = "5px";
        button2.style.top = "5px";
        button2.style.cursor = "pointer";
        button2.style.backgroundColor = "rgba(10,50,10,0.8)";
        button2.style.color = "rgb(0,255,0)";
        button2.style.fontSize = "8pt";
        button2.style.fontFamily = "Monaco, 'Andale Mono', 'Monotype.com', monospace";
        button2.style.fontWeight = "bold";
        button2.style.padding = "5px";
        button2.style.border = "1px solid rgb(0,255,0)";
        button2.style.webkitUserSelect = "none";
        button2.style.mozUserSelect = "none";
        button2.title = "Show full inspector (F11)";
        button2.textContent = "UI";
        document.body.appendChild(button2);

        button2.addEventListener("click", function () {
            requestFullUI(context);
        }, false);
    };

    function injectHandlers(ui) {
        var context = ui.context;

        // Key handler to listen for capture requests
        document.addEventListener("keydown", function (event) {
            var handled = false;
            switch (event.keyCode) {
                case 122: // F11
                    requestFullUI(context);
                    handled = true;
                    break;
                case 123: // F12
                    requestCapture(context);
                    handled = true;
                    break;
            };

            if (handled) {
                event.preventDefault();
                event.stopPropagation();
            }
        }, false);
    };

    var HostUI = function (context) {
        this.context = context;

        injectUI(this);
        injectHandlers(this);

        this.context.frames = [];

        var hudVisible = settings.session.hudVisible || settings.global.showHud;
        requestFullUI(context, !hudVisible);

    };

    HostUI.requestFullUI = requestFullUI;
    return HostUI;
});
