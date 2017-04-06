define([
        '../../host/CaptureContext',
        '../../shared/Settings',
        '../../shared/SplitterBar',
        '../../shared/Utilities',
    ], function (
        captureContext,
        settings,
        SplitterBar,
        util
    ) {

    // options: {
    //     splitterKey: 'traceSplitter' / etc
    //     title: 'Texture'
    //     selectionName: 'Face' / etc
    //     selectionValues: ['sel 1', 'sel 2', ...]
    //     disableSizing: true/false
    //     transparentCanvas: true/false
    // }

    var SurfaceInspector = function (view, w, elementRoot, options) {
        var self = this;
        var context = w.context;
        this.window = w;
        this.elements = {
            toolbar: elementRoot.getElementsByClassName("surface-inspector-toolbar")[0],
            statusbar: elementRoot.getElementsByClassName("surface-inspector-statusbar")[0],
            view: elementRoot.getElementsByClassName("surface-inspector-inner")[0]
        };
        this.options = options;

        var defaultWidth = 240;
        var width = settings.session[options.splitterKey];
        if (width) {
            width = Math.max(240, Math.min(width, window.innerWidth - 400));
        } else {
            width = defaultWidth;
        }
        this.elements.view.style.width = width + "px";
        this.splitter = new SplitterBar(this.elements.view, "vertical", 240, 800, "splitter-inspector", function (newWidth) {
            view.setInspectorWidth(newWidth);
            self.layout();

            if (self.elements.statusbar) {
                self.elements.statusbar.style.width = newWidth + "px";
            }

            settings.session[options.splitterKey] = newWidth;
            settings.save();
        });
        view.setInspectorWidth(width);

        // Add view options
        var optionsDiv = document.createElement("div");
        optionsDiv.className = "surface-inspector-options";
        optionsDiv.style.display = "none";
        var optionsSpan = document.createElement("span");
        optionsSpan.textContent = options.selectionName + ": ";
        optionsDiv.appendChild(optionsSpan);
        var optionsList = document.createElement("select");
        optionsList.className = "";
        optionsDiv.appendChild(optionsList);
        this.setSelectionValues = function (selectionValues) {
            while (optionsList.hasChildNodes()) {
              optionsList.removeChild(optionsList.firstChild);
            }
            if (selectionValues) {
                for (var n = 0; n < selectionValues.length; n++) {
                    var selectionOption = document.createElement("option");
                    selectionOption.textContent = selectionValues[n];
                    optionsList.appendChild(selectionOption);
                }
            }
        };
        this.setSelectionValues(options.selectionValues);
        this.elements.toolbar.appendChild(optionsDiv);
        this.elements.faces = optionsDiv;
        this.optionsList = optionsList;
        optionsList.onchange = function () {
            if (self.activeOption != optionsList.selectedIndex) {
                self.activeOption = optionsList.selectedIndex;
                self.updatePreview();
            }
        };

        // Add sizing options
        var sizingDiv = document.createElement("div");
        sizingDiv.className = "surface-inspector-sizing";
        if (this.options.disableSizing) {
            sizingDiv.style.display = "none";
        }
        var nativeSize = document.createElement("span");
        nativeSize.title = "Native resolution (100%)";
        nativeSize.textContent = "100%";
        nativeSize.onclick = function () {
            self.sizingMode = "native";
            self.layout();
        };
        sizingDiv.appendChild(nativeSize);
        var sepSize = document.createElement("div");
        sepSize.className = "surface-inspector-sizing-sep";
        sepSize.textContent = " | ";
        sizingDiv.appendChild(sepSize);
        var fitSize = document.createElement("span");
        fitSize.title = "Fit to inspector window";
        fitSize.textContent = "Fit";
        fitSize.onclick = function () {
            self.sizingMode = "fit";
            self.layout();
        };
        sizingDiv.appendChild(fitSize);
        this.elements.toolbar.appendChild(sizingDiv);
        this.elements.sizingDiv = sizingDiv;

        function getLocationString(x, y) {
            var width = self.canvas.width;
            var height = self.canvas.height;
            var tx = String(Math.round(x / width * 1000) / 1000);
            var ty = String(Math.round(y / height * 1000) / 1000);
            if (tx.length == 1) {
                tx += ".000";
            }
            while (tx.length < 5) {
                tx += "0";
            }
            if (ty.length == 1) {
                ty += ".000";
            }
            while (ty.length < 5) {
                ty += "0";
            }
            return x + ", " + y + " (" + tx + ", " + ty + ")";
        };

        // Statusbar (may not be present)
        var updatePixelPreview = null;
        var pixelDisplayMode = "location";
        var statusbar = this.elements.statusbar;
        var pixelCanvas = statusbar && statusbar.getElementsByClassName("surface-inspector-pixel")[0];
        var locationSpan = statusbar && statusbar.getElementsByClassName("surface-inspector-location")[0];
        if (statusbar) {
            statusbar.style.width = width + "px";
        }
        if (statusbar && pixelCanvas && locationSpan) {
            var lastX = 0;
            var lastY = 0;
            updatePixelPreview = function (x, y) {
                pixelCanvas.style.display = "none";

                if ((x === null) || (y === null)) {
                    while (locationSpan.hasChildNodes()) {
                      locationSpan.removeChild(locationSpan.firstChild);
                    }
                    return;
                }

                lastX = x;
                lastY = y;

                var gl = util.getWebGLContext(self.canvas);
                var pixel = new Uint8Array(4);
                gl.readPixels(x, self.canvas.height - y - 1, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
                var r = pixel[0];
                var g = pixel[1];
                var b = pixel[2];
                var a = pixel[3];
                var pixelStyle = "rgba(" + r + ", " + g + ", " + b + ", " + a + ")";

                // Draw preview in the pixel canvas
                pixelCanvas.style.display = "";
                var pctx = pixelCanvas.getContext("2d");
                pctx.clearRect(0, 0, 1, 1);
                pctx.fillStyle = pixelStyle;
                pctx.fillRect(0, 0, 1, 1);

                switch (pixelDisplayMode) {
                    case "location":
                        locationSpan.textContent = getLocationString(x, y);
                        break;
                    case "color":
                        locationSpan.textContent = pixelStyle;
                        break;
                }
            };
            statusbar.addEventListener("click", function () {
                if (pixelDisplayMode == "location") {
                    pixelDisplayMode = "color";
                } else {
                    pixelDisplayMode = "location";
                }
                updatePixelPreview(lastX, lastY);
            }, false);

            this.clearPixel = function () {
                updatePixelPreview(null, null);
            };
        } else {
            this.clearPixel = function () { };
        }

        // Display canvas
        var canvas = this.canvas = document.createElement("canvas");
        canvas.className = "gli-reset";
        if (options.transparentCanvas) {
            canvas.className += " surface-inspector-canvas-transparent";
        } else {
            canvas.className += " surface-inspector-canvas";
        }
        canvas.style.display = "none";
        canvas.width = 1;
        canvas.height = 1;
        this.elements.view.appendChild(canvas);

        function getPixelPosition(e) {
            var x = e.offsetX || e.layerX;
            var y = e.offsetY || e.layerY;
            switch (self.sizingMode) {
                case "fit":
                    var scale = parseFloat(self.canvas.style.width) / self.canvas.width;
                    x /= scale;
                    y /= scale;
                    break;
                case "native":
                    break;
            }
            return [Math.floor(x), Math.floor(y)];
        };

        canvas.addEventListener("click", function (e) {
            var pos = getPixelPosition(e);
            self.inspectPixel(pos[0], pos[1], getLocationString(pos[0], pos[1]));
        }, false);

        if (updatePixelPreview) {
            canvas.addEventListener("mousemove", function (e) {
                var pos = getPixelPosition(e);
                updatePixelPreview(pos[0], pos[1]);
            }, false);
        }

        this.sizingMode = "fit";
        this.resizeHACK = false;
        this.elements.view.style.overflow = "";

        this.activeOption = 0;

        captureContext.setTimeout(function () {
            self.setupPreview();
            self.layout();
        }, 0);
    };

    SurfaceInspector.prototype.inspectPixel = function (x, y, locationString) {
    };

    SurfaceInspector.prototype.setupPreview = function () {
        this.activeOption = 0;
    };

    SurfaceInspector.prototype.updatePreview = function () {
    };

    SurfaceInspector.prototype.layout = function () {
        var self = this;
        this.clearPixel();

        var size = this.querySize();
        if (!size) {
            return;
        }

        if (this.options.autoFit) {
            this.canvas.style.left = "";
            this.canvas.style.top = "";
            this.canvas.style.width = "";
            this.canvas.style.height = "";
            var parentWidth = this.elements.view.clientWidth;
            var parentHeight = this.elements.view.clientHeight;
            this.canvas.width = parentWidth;
            this.canvas.height = parentHeight;
            self.updatePreview();
        } else {
            switch (this.sizingMode) {
                case "native":
                    this.elements.view.style.overflow = "auto";
                    this.canvas.style.left = "";
                    this.canvas.style.top = "";
                    this.canvas.style.width = "";
                    this.canvas.style.height = "";
                    break;
                case "fit":
                    this.elements.view.style.overflow = "";

                    var parentWidth = this.elements.view.clientWidth;
                    var parentHeight = this.elements.view.clientHeight;
                    var parentar = parentHeight / parentWidth;
                    var ar = size[1] / size[0];

                    var width;
                    var height;
                    if (ar * parentWidth < parentHeight) {
                        width = parentWidth;
                        height = (ar * parentWidth);
                    } else {
                        height = parentHeight;
                        width = (parentHeight / ar);
                    }
                    if (width && height) {
                        this.canvas.style.width = width + "px";
                        this.canvas.style.height = height + "px";
                    }

                    this.canvas.style.left = ((parentWidth / 2) - (width / 2)) + "px";
                    this.canvas.style.top = ((parentHeight / 2) - (height / 2)) + "px";

                    // HACK: force another layout because we may have changed scrollbar status
                    if (this.resizeHACK) {
                        this.resizeHACK = false;
                    } else {
                        this.resizeHACK = true;
                        captureContext.setTimeout(function () {
                            self.layout();
                        }, 0);
                    }
                    break;
            }
        }
    };

    SurfaceInspector.prototype.reset = function () {
        this.elements.view.scrollLeft = 0;
        this.elements.view.scrollTop = 0;
    };

    return SurfaceInspector;
});
