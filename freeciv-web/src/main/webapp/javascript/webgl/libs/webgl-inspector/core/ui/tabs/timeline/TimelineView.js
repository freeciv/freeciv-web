define([
        '../../../shared/Settings',
    ], function (
        settings
    ) {

    var TimelineView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-right-outer")[0],
            left: elementRoot.getElementsByClassName("window-left")[0],
            right: elementRoot.getElementsByClassName("window-right")[0]
        };

        var statistics = this.window.context.statistics;

        this.displayCanvas = elementRoot.getElementsByClassName("timeline-canvas")[0];

        function appendKeyRow(keyRoot, counter) {
            var row = document.createElement("div");
            row.className = "timeline-key-row";
            if (counter.enabled) {
                row.className += " timeline-key-row-enabled";
            }

            var colorEl = document.createElement("div");
            colorEl.className = "timeline-key-color";
            colorEl.style.backgroundColor = counter.color;
            row.appendChild(colorEl);

            var nameEl = document.createElement("div");
            nameEl.className = "timeline-key-name";
            nameEl.textContent = counter.description;
            row.appendChild(nameEl);

            keyRoot.appendChild(row);

            row.onclick = function () {
                counter.enabled = !counter.enabled;
                if (!counter.enabled) {
                    row.className = row.className.replace(" timeline-key-row-enabled", "");
                } else {
                    row.className += " timeline-key-row-enabled";
                }

                settings.session.counterToggles[counter.name] = counter.enabled;
                settings.save();
            };
        };

        if (settings.session.enableTimeline) {
            this.displayCanvas.width = 800;
            this.displayCanvas.height = 200;

            this.elements.left.style.overflow = "auto";

            this.canvases = [];
            for (var n = 0; n < 2; n++) {
                var canvas = document.createElement("canvas");
                canvas.className = "gli-reset";
                canvas.width = 800;
                canvas.height = 200;
                this.canvases.push(canvas);
            }
            this.activeCanvasIndex = 0;

            // Load enabled status
            var counterToggles = settings.session.counterToggles;
            if (counterToggles) {
                for (var n = 0; n < statistics.counters.length; n++) {
                    var counter = statistics.counters[n];
                    var toggle = counterToggles[counter.name];
                    if (toggle === true) {
                        counter.enabled = true;
                    } else if (toggle === false) {
                        counter.enabled = false;
                    } else {
                        // Default
                    }
                }
            }

            var keyRoot = document.createElement("div");
            keyRoot.className = "timeline-key";
            for (var n = 0; n < statistics.counters.length; n++) {
                var counter = statistics.counters[n];
                appendKeyRow(keyRoot, counter);
            }
            this.elements.left.appendChild(keyRoot);

            // Install a frame watcher
            this.updating = false;
            var updateCount = 0;
            this.window.context.frameCompleted.addListener(this, function () {
                // TODO: hold updates for a bit? Could affect perf to do this every frame
                updateCount++;
                if (updateCount % 2 == 0) {
                    // Only update every other frame
                    self.appendFrame();
                }
            });
        } else {
            // Hide canvas
            this.displayCanvas.style.display = "none";
        }

        // Show help message
        var enableMessage = document.createElement("a");
        enableMessage.className = "timeline-enable-link";
        if (settings.session.enableTimeline) {
            enableMessage.textContent = "Timeline enabled - click to disable";
        } else {
            enableMessage.textContent = "Timeline disabled - click to enable";
        }
        enableMessage.onclick = function (e) {
            settings.session.enableTimeline = !settings.session.enableTimeline;
            settings.save();
            window.location.reload();
            e.preventDefault();
            e.stopPropagation();
        };
        this.elements.right.insertBefore(enableMessage, this.elements.right.firstChild);
    };

    TimelineView.prototype.suspendUpdating = function () {
        this.updating = false;
    };

    TimelineView.prototype.resumeUpdating = function () {
        this.updating = true;
    };

    TimelineView.prototype.appendFrame = function () {
        var statistics = this.window.context.statistics;

        var canvas = this.canvases[this.activeCanvasIndex];
        this.activeCanvasIndex = (this.activeCanvasIndex + 1) % this.canvases.length;
        var oldCanvas = this.canvases[this.activeCanvasIndex];

        var ctx = canvas.getContext("2d");

        // Draw old
        ctx.drawImage(oldCanvas, -1, 0);

        // Clear newly revealed line
        var x = canvas.width - 1;
        ctx.fillStyle = "rgb(255,255,255)";
        ctx.fillRect(x - 1, 0, 2, canvas.height);

        // Draw counter values
        for (var n = 0; n < statistics.counters.length; n++) {
            var counter = statistics.counters[n];
            if (!counter.enabled) {
                continue;
            }
            var v = Math.round(counter.value);
            var pv = Math.round(counter.previousValue);
            var y = canvas.height - v;
            var py = canvas.height - pv;
            ctx.beginPath();
            ctx.moveTo(x - 1, py + 0.5);
            ctx.lineTo(x, y + 0.5);
            ctx.strokeStyle = counter.color;
            ctx.stroke();
        }

        // Only do the final composite if we have focus
        if (this.updating) {
            var displayCtx = this.displayCanvas.getContext("2d");
            displayCtx.drawImage(canvas, 0, 0);
        }
    };

    TimelineView.prototype.refresh = function () {
        // 
    };

    return TimelineView;
});
