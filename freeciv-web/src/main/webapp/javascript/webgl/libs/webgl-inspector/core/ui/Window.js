define([
        '../host/CaptureContext',
        '../host/HostUI',
        '../replay/Controller',
        './Tab',
        './tabs/trace/TraceTab',
        './tabs/timeline/TimelineTab',
        './tabs/state/StateTab',
        './tabs/textures/TexturesTab',
        './tabs/buffers/BuffersTab',
        './tabs/programs/ProgramsTab',
        './tabs/vertexarrays/VertexArraysTab',
    ], function (
        captureContext,
        HostUI,
        Controller,
        Tab,
        TraceTab,
        TimelineTab,
        StateTab,
        TexturesTab,
        BuffersTab,
        ProgramsTab,
        VertexArraysTab
    ) {

    var Toolbar = function (w) {
        var self = this;
        var document = w.document;

        this.window = w;
        this.elements = {
            bar: w.root.getElementsByClassName("window-toolbar")[0]
        };
        this.buttons = {};

        function appendRightRegion(title, buttons) {
            var regionDiv = document.createElement("div");
            regionDiv.className = "toolbar-right-region";

            var titleDiv = document.createElement("div");
            titleDiv.className = "toolbar-right-region-title";
            titleDiv.textContent = title;
            regionDiv.appendChild(titleDiv);

            var activeIndex = 0;
            var previousSelection = null;

            for (var n = 0; n < buttons.length; n++) {
                var button = buttons[n];

                var buttonSpan = document.createElement("span");
                if (button.name) {
                    buttonSpan.textContent = button.name;
                }
                if (button.className) {
                    buttonSpan.className = button.className;
                }
                buttonSpan.title = button.title ? button.title : button.name;
                regionDiv.appendChild(buttonSpan);
                button.el = buttonSpan;

                (function (n, button) {
                    buttonSpan.onclick = function () {
                        if (previousSelection) {
                            previousSelection.el.className = previousSelection.el.className.replace(" toolbar-right-region-active", "");
                        }
                        previousSelection = button;
                        button.el.className += " toolbar-right-region-active";

                        button.onclick.apply(self);
                    };
                })(n, button);

                if (n < buttons.length - 1) {
                    var sep = document.createElement("div");
                    sep.className = "toolbar-right-region-sep";
                    sep.textContent = " | ";
                    regionDiv.appendChild(sep);
                }
            }

            // Select first
            buttons[0].el.onclick();

            self.elements.bar.appendChild(regionDiv);
        };
        function appendRightButtons(buttons) {
            var regionDiv = document.createElement("div");
            regionDiv.className = "toolbar-right-buttons";

            for (var n = 0; n < buttons.length; n++) {
                var button = buttons[n];

                var buttonDiv = document.createElement("div");
                if (button.name) {
                    buttonDiv.textContent = button.name;
                }
                buttonDiv.className = "toolbar-right-button";
                if (button.className) {
                    buttonDiv.className += " " + button.className;
                }
                buttonDiv.title = button.title ? button.title : button.name;
                regionDiv.appendChild(buttonDiv);
                button.el = buttonDiv;

                (function (button) {
                    buttonDiv.onclick = function () {
                        button.onclick.apply(self);
                    };
                })(button);

                if (n < buttons.length - 1) {
                    var sep = document.createElement("div");
                    sep.className = "toolbar-right-buttons-sep";
                    sep.textContent = " ";
                    regionDiv.appendChild(sep);
                }
            }

            self.elements.bar.appendChild(regionDiv);
        };

        appendRightButtons([
            /*{
                title: "Options",
                className: "toolbar-right-button-options",
                onclick: function () {
                    alert("options");
                }
            },*/
            {
                title: "Hide inspector (F11)",
                className: "toolbar-right-button-close",
                onclick: function () {
                    HostUI.requestFullUI(w.context);
                }
            }
        ]);
        /*
        appendRightRegion("Version: ", [
            {
                name: "Live",
                onclick: function () {
                    w.setActiveVersion(null);
                }
            },
            {
                name: "Current",
                onclick: function () {
                    w.setActiveVersion("current");
                }
            }
        ]);
        */
        appendRightRegion("Frame Control: ", [
            {
                name: "Normal",
                onclick: function () {
                    captureContext.setFrameControl(0);
                }
            },
            {
                name: "Slowed",
                onclick: function () {
                    captureContext.setFrameControl(250);
                }
            },
            {
                name: "Paused",
                onclick: function () {
                    captureContext.setFrameControl(Infinity);
                }
            }
        ]);
    };
    Toolbar.prototype.addSelection = function (name, tip) {
        var self = this;

        var el = document.createElement("div");
        el.className = "toolbar-button toolbar-button-enabled toolbar-button-command-" + name;

        el.title = tip;
        el.textContent = tip;

        el.onclick = function () {
            self.window.selectTab(name);
        };

        this.elements.bar.appendChild(el);

        this.buttons[name] = el;
    };
    Toolbar.prototype.toggleSelection = function (name) {
        for (var n in this.buttons) {
            var el = this.buttons[n];
            el.className = el.className.replace("toolbar-button-selected", "toolbar-button-enabled");
        }
        var el = this.buttons[name];
        if (el) {
            el.className = el.className.replace("toolbar-button-disabled", "toolbar-button-selected");
            el.className = el.className.replace("toolbar-button-enabled", "toolbar-button-selected");
        }
    };

    function writeDocument(document, elementHost) {
        var root = document.createElement("div");
        root.className = "window";

        // Toolbar
        // <div class="window-toolbar">
        // ...
        var toolbar = document.createElement("div");
        toolbar.className = "window-toolbar";
        root.appendChild(toolbar);

        // Middle
        // <div class="window-middle">
        // ...
        var middle = document.createElement("div");
        middle.className = "window-middle";
        root.appendChild(middle);

        if (elementHost) {
            elementHost.appendChild(root);
        } else {
            document.body.appendChild(root);
        }

        root.elements = {
            toolbar: toolbar,
            middle: middle
        };

        return root;
    };

    var Window = function (context, document, elementHost) {
        var self = this;
        this.context = context;
        this.document = document;
        this.browserWindow = window;

        this.root = writeDocument(document, elementHost);

        this.controller = new Controller();

        this.toolbar = new Toolbar(this);
        this.tabs = {};
        this.currentTab = null;
        this.windows = {};

        this.activeVersion = "current"; // or null for live
        this.activeFilter = null;

        var middle = this.root.elements.middle;
        function addTab(name, tip, implType) {
            var tab = new Tab(self, middle, name);

            if (implType) {
                implType.apply(tab, [self]);
            }

            self.toolbar.addSelection(name, tip);

            self.tabs[name] = tab;
        };

        addTab("trace", "Trace", TraceTab);
        addTab("timeline", "Timeline", TimelineTab);
        addTab("state", "State", StateTab);
        addTab("textures", "Textures", TexturesTab);
        addTab("buffers", "Buffers", BuffersTab);
        addTab("programs", "Programs", ProgramsTab);
        addTab("vaos", "VAOs", VertexArraysTab);
        //addTab("performance", "Performance", PerformanceTab);

        this.selectTab("trace");

        window.addEventListener("beforeunload", function () {
            for (var n in self.windows) {
                var w = self.windows[n];
                if (w) {
                    w.close();
                }
            }
        }, false);

        captureContext.setTimeout(function () {
            self.selectTab("trace", true);
        }, 0);
    };

    Window.prototype.layout = function () {
        for (var n in this.tabs) {
            var tab = this.tabs[n];
            if (tab.layout) {
                tab.layout();
            }
        }
    };

    Window.prototype.selectTab = function (name, force) {
        if (name.name) {
            name = name.name;
        }
        if (this.currentTab && this.currentTab.name == name && !force) {
            return;
        }
        var tab = this.tabs[name];
        if (!tab) {
            return;
        }

        if (this.currentTab) {
            this.currentTab.loseFocus();
            this.currentTab = null;
        }

        this.currentTab = tab;
        this.currentTab.gainFocus();
        this.toolbar.toggleSelection(name);

        if (tab.layout) {
            tab.layout();
        }
        if (tab.refresh) {
            tab.refresh();
        }
    };

    Window.prototype.setActiveVersion = function (version) {
        if (this.activeVersion == version) {
            return;
        }
        this.activeVersion = version;
        if (this.currentTab.refresh) {
            this.currentTab.refresh();
        }
    };

    Window.prototype.setActiveFilter = function (filter) {
        if (this.activeFilter == filter) {
            return;
        }
        this.activeFilter = filter;
        console.log("would set active filter: " + filter);
    };

    Window.prototype.appendFrame = function (frame) {
        var tab = this.tabs["trace"];
        this.selectTab(tab);
        tab.listing.appendValue(frame);
        tab.listing.selectValue(frame);
    };

    Window.prototype.showTrace = function (frame, callOrdinal) {
        var tab = this.tabs["trace"];
        this.selectTab(tab);
        if (this.controller.currentFrame != frame) {
            tab.listing.selectValue(frame);
        }
        tab.traceView.stepUntil(callOrdinal);
    };

    Window.prototype.showResource = function (resourceTab, resource, switchToCurrent) {
        if (switchToCurrent) {
            // TODO: need to update UI to be able to do this
            //this.setActiveVersion("current");
        }
        var tab = this.tabs[resourceTab];
        this.selectTab(tab);
        tab.listing.selectValue(resource);
        this.browserWindow.focus();
    };

    Window.prototype.showTexture = function (texture, switchToCurrent) {
        this.showResource("textures", texture, switchToCurrent);
    };

    Window.prototype.showBuffer = function (buffer, switchToCurrent) {
        this.showResource("buffers", buffer, switchToCurrent);
    };

    Window.prototype.showVertexArray = function (vertexArray, switchToCurrent) {
        this.showResource("vaos", vertexArray, switchToCurrent);
    };

    Window.prototype.showProgram = function (program, switchToCurrent) {
        this.showResource("programs", program, switchToCurrent);
    };

    return Window;
});
