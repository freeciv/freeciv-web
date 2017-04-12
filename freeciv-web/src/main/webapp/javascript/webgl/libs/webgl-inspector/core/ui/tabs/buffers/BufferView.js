define([
        '../../Helpers',
        '../../shared/BufferPreview',
        '../../shared/SurfaceInspector',
        '../../shared/TraceLine',
    ], function (
        helpers,
        BufferPreview,
        SurfaceInspector,
        traceLine
    ) {

    function shouldShowPreview(gl, buffer, version) {
        return !!buffer && (buffer.type == gl.ARRAY_BUFFER) && !!version.structure && !!version.lastDrawState;
    }

    var BufferView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-right")[0],
            listing: elementRoot.getElementsByClassName("buffer-listing")[0]
        };

        this.inspectorElements = {
            "window-buffer-outer": elementRoot.getElementsByClassName("window-buffer-outer")[0],
            "window-buffer-inspector": elementRoot.getElementsByClassName("window-buffer-inspector")[0],
            "buffer-listing": elementRoot.getElementsByClassName("buffer-listing")[0]
        };
        this.inspector = new SurfaceInspector(this, w, elementRoot, {
            splitterKey: 'bufferSplitter',
            title: 'Buffer Preview',
            selectionName: null,
            selectionValues: null,
            disableSizing: true,
            transparentCanvas: true,
            autoFit: true
        });
        this.inspector.currentBuffer = null;
        this.inspector.currentVersion = null;
        this.inspector.querySize = function () {
            var gl = this.gl;
            if (!this.currentBuffer || !this.currentVersion) {
                return null;
            }
            return [256, 256]; // ?
        };
        this.inspector.setupPreview = function () {
            var self = this;
            if (this.previewer) {
                return;
            }

            this.previewer = new BufferPreview(this.canvas);
            this.gl = this.previewer.gl;

            this.canvas.width = 256;
            this.canvas.height = 256;

            this.previewer.setupDefaultInput();
        }
        this.inspector.updatePreview = function () {
            var gl = this.gl;

            this.previewer.draw();
        };
        this.inspector.setBuffer = function (buffer, version) {
            var gl = this.gl;

            var showPreview = shouldShowPreview(gl, buffer, version);
            if (showPreview) {
                // Setup UI
                this.canvas.width = 256;
                this.canvas.height = 256;
                this.canvas.style.display = "";
                this.updatePreview();
            } else {
                // Clear everything
                this.canvas.width = 1;
                this.canvas.height = 1;
                this.canvas.style.display = "none";
            }

            if (showPreview) {
                this.options.title = "Buffer Preview: " + buffer.getName();
            } else {
                this.options.title = "Buffer Preview: (none)";
            }

            this.currentBuffer = buffer;
            this.currentVersion = version;
            this.activeOption = 0;
            this.optionsList.selectedIndex = 0;

            this.reset();
            this.layout();

            if (showPreview) {
                this.previewer.setBuffer(buffer.previewOptions);
            }
        };

        this.currentBuffer = null;
    };

    BufferView.prototype.setInspectorWidth = function (newWidth) {
        //.window-buffer-outer margin-left: -800px !important; /* -2 * window-buffer-inspector.width */
        //.window-buffer margin-left: 400px !important; /* window-buffer-inspector.width */
        //.buffer-listing right: 400px; /* window-buffer-inspector */
        this.inspectorElements["window-buffer-outer"].style.marginLeft = (-2 * newWidth) + "px";
        this.inspectorElements["window-buffer-inspector"].style.width = newWidth + "px";
        this.inspectorElements["buffer-listing"].style.right = newWidth + "px";
    };

    BufferView.prototype.layout = function () {
        this.inspector.layout();
    };

    function appendHistoryLine(gl, el, buffer, call) {
        traceLine.appendHistoryLine(gl, el, call);

        // TODO: other custom stuff?
    }

    function generateBufferHistory(gl, el, buffer, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-secondary";
        titleDiv.textContent = "History";
        el.appendChild(titleDiv);

        var rootEl = document.createElement("div");
        rootEl.className = "buffer-history";
        el.appendChild(rootEl);

        for (var n = 0; n < version.calls.length; n++) {
            var call = version.calls[n];
            appendHistoryLine(gl, rootEl, buffer, call);
        }
    };

    function generateGenericArrayBufferContents(gl, el, buffer, version) {
        var data = buffer.constructVersion(gl, version);

        var table = document.createElement("table");
        table.className = "buffer-data";
        for (var n = 0, len = data.length; n < len; ++n) {
            var tr = document.createElement("tr");
            var tdkey = document.createElement("td");
            tdkey.className = "buffer-data-key";
            tdkey.textContent = n;
            tr.appendChild(tdkey);
            var tdvalue = document.createElement("td");
            tdvalue.className = "buffer-data-value";
            tdvalue.textContent = data[n];
            tr.appendChild(tdvalue);
            table.appendChild(tr);
        }
        el.appendChild(table);
    };

    function generateArrayBufferContents(gl, el, buffer, version) {
        if (!version.structure) {
            generateGenericArrayBufferContents(gl, el, buffer, version);
            return;
        }

        var data = buffer.constructVersion(gl, version);
        var datas = version.structure;
        var stride = datas[0].stride;
        if (stride == 0) {
            // Calculate stride from last byte
            for (var m = 0; m < datas.length; m++) {
                var byteAdvance = 0;
                switch (datas[m].type) {
                    case gl.BYTE:
                    case gl.UNSIGNED_BYTE:
                        byteAdvance = 1 * datas[m].size;
                        break;
                    case gl.SHORT:
                    case gl.UNSIGNED_SHORT:
                        byteAdvance = 2 * datas[m].size;
                        break;
                    default:
                    case gl.FLOAT:
                        byteAdvance = 4 * datas[m].size;
                        break;
                }
                stride = Math.max(stride, datas[m].offset + byteAdvance);
            }
        }

        var table = document.createElement("table");
        table.className = "buffer-data";
        var byteOffset = 0;
        var itemOffset = 0;
        while (byteOffset < data.byteLength) {
            var tr = document.createElement("tr");

            var tdkey = document.createElement("td");
            tdkey.className = "buffer-data-key";
            tdkey.textContent = itemOffset;
            tr.appendChild(tdkey);

            var innerOffset = byteOffset;
            for (var m = 0; m < datas.length; m++) {
                var byteAdvance = 0;
                var readView = null;
                var dataBuffer = data.buffer ? data.buffer : data;
                switch (datas[m].type) {
                    case gl.BYTE:
                        byteAdvance = 1 * datas[m].size;
                        readView = new Int8Array(dataBuffer, innerOffset, datas[m].size);
                        break;
                    case gl.UNSIGNED_BYTE:
                        byteAdvance = 1 * datas[m].size;
                        readView = new Uint8Array(dataBuffer, innerOffset, datas[m].size);
                        break;
                    case gl.SHORT:
                        byteAdvance = 2 * datas[m].size;
                        readView = new Int16Array(dataBuffer, innerOffset, datas[m].size);
                        break;
                    case gl.UNSIGNED_SHORT:
                        byteAdvance = 2 * datas[m].size;
                        readView = new Uint16Array(dataBuffer, innerOffset, datas[m].size);
                        break;
                    default:
                    case gl.FLOAT:
                        byteAdvance = 4 * datas[m].size;
                        readView = new Float32Array(dataBuffer, innerOffset, datas[m].size);
                        break;
                }
                innerOffset += byteAdvance;

                for (var i = 0; i < datas[m].size; i++) {
                    var td = document.createElement("td");
                    td.className = "buffer-data-value";
                    if ((m != datas.length - 1) && (i == datas[m].size - 1)) {
                        td.className += " buffer-data-value-end";
                    }
                    td.textContent = readView[i];
                    tr.appendChild(td);
                }
            }

            byteOffset += stride;
            itemOffset++;
            table.appendChild(tr);
        }
        el.appendChild(table);
    };

    function generateBufferDisplay(view, gl, el, buffer, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = buffer.getName();
        switch (buffer.type) {
            case gl.ARRAY_BUFFER:
                titleDiv.textContent += " / ARRAY_BUFFER";
                break;
            case gl.ELEMENT_ARRAY_BUFFER:
                titleDiv.textContent += " / ELEMENT_ARRAY_BUFFER";
                break;
        }
        el.appendChild(titleDiv);

        helpers.appendParameters(gl, el, buffer, ["BUFFER_SIZE", "BUFFER_USAGE"], [null, ["STREAM_DRAW", "STATIC_DRAW", "DYNAMIC_DRAW"]]);
        helpers.appendbr(el);

        function updatePreviewSettings () {
            var options = buffer.previewOptions;

            // Draw options
            options.mode = gl.POINTS + modeSelect.selectedIndex;
            options.positionIndex = attributeSelect.selectedIndex;
            options.position = version.structure[options.positionIndex];

            // Element array buffer options
            if (elementArraySelect.selectedIndex === 0) {
                // Unindexed
                options.elementArrayBuffer = null;
            } else {
                var option = elementArraySelect.options[elementArraySelect.selectedIndex];
                var elid = parseInt(option.value, 10);
                var elbuffer = gl.resources.getResourceById(elid);
                options.elementArrayBuffer = [elbuffer, elbuffer.currentVersion];
            }
            switch (sizeSelect.selectedIndex) {
                case 0:
                    options.elementArrayType = gl.UNSIGNED_BYTE;
                    break;
                case 1:
                    options.elementArrayType = gl.UNSIGNED_SHORT;
                    break;
            }

            // Range options
            if (options.elementArrayBuffer) {
                options.offset = parseInt(startInput.value, 10);
            } else {
                options.first = parseInt(startInput.value, 10);
            }
            options.count = parseInt(countInput.value, 10);

            try {
                view.inspector.setBuffer(buffer, version);
            } catch (e) {
                view.inspector.setBuffer(null, null);
                console.log("exception while setting buffer preview: " + e);
            }
        };

        var showPreview = shouldShowPreview(gl, buffer, version);
        if (showPreview) {
            helpers.appendSeparator(el);

            var previewDiv = document.createElement("div");
            previewDiv.className = "info-title-secondary";
            previewDiv.textContent = "Preview Options";
            el.appendChild(previewDiv);

            var previewContainer = document.createElement("div");

            // Tools for choosing preview options
            var previewOptions = document.createElement("table");
            previewOptions.className = "buffer-preview";

            // Draw settings
            var drawRow = document.createElement("tr");
            {
                var col0 = document.createElement("td");
                var span0 = document.createElement("span");
                span0.textContent = "Mode: ";
                col0.appendChild(span0);
                drawRow.appendChild(col0);
            }
            {
                var col1 = document.createElement("td");
                var modeSelect = document.createElement("select");
                var modeEnums = ["POINTS", "LINE_STRIP", "LINE_LOOP", "LINES", "TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN"];
                for (var n = 0; n < modeEnums.length; n++) {
                    var option = document.createElement("option");
                    option.textContent = modeEnums[n];
                    modeSelect.appendChild(option);
                }
                modeSelect.onchange = function () {
                    updatePreviewSettings();
                };
                col1.appendChild(modeSelect);
                drawRow.appendChild(col1);
            }
            {
                var col2 = document.createElement("td");
                var span1 = document.createElement("span");
                span1.textContent = "Position Attribute: ";
                col2.appendChild(span1);
                drawRow.appendChild(col2);
            }
            {
                var col3 = document.createElement("td");
                var attributeSelect = document.createElement("select");
                for (var n = 0; n < version.structure.length; n++) {
                    var attrInfo = version.structure[n];
                    var option = document.createElement("option");
                    var typeString;
                    switch (attrInfo.type) {
                        case gl.BYTE:
                            typeString = "BYTE";
                            break;
                        case gl.UNSIGNED_BYTE:
                            typeString = "UNSIGNED_BYTE";
                            break;
                        case gl.SHORT:
                            typeString = "SHORT";
                            break;
                        case gl.UNSIGNED_SHORT:
                            typeString = "UNSIGNED_SHORT";
                            break;
                        default:
                        case gl.FLOAT:
                            typeString = "FLOAT";
                            break;
                    }
                    option.textContent = "+" + attrInfo.offset + " / " + attrInfo.size + " * " + typeString;
                    attributeSelect.appendChild(option);
                }
                attributeSelect.onchange = function () {
                    updatePreviewSettings();
                };
                col3.appendChild(attributeSelect);
                drawRow.appendChild(col3);
            }
            previewOptions.appendChild(drawRow);

            // ELEMENT_ARRAY_BUFFER settings
            var elementArrayRow = document.createElement("tr");
            {
                var col0 = document.createElement("td");
                var span0 = document.createElement("span");
                span0.textContent = "Element Array: ";
                col0.appendChild(span0);
                elementArrayRow.appendChild(col0);
            }
            {
                var col1 = document.createElement("td");
                var elementArraySelect = document.createElement("select");
                var noneOption = document.createElement("option");
                noneOption.textContent = "[unindexed]";
                noneOption.value = null;
                elementArraySelect.appendChild(noneOption);
                var allBuffers = gl.resources.getBuffers();
                for (var n = 0; n < allBuffers.length; n++) {
                    var elBuffer = allBuffers[n];
                    if (elBuffer.type == gl.ELEMENT_ARRAY_BUFFER) {
                        var option = document.createElement("option");
                        option.textContent = elBuffer.getName();
                        option.value = elBuffer.id;
                        elementArraySelect.appendChild(option);
                    }
                }
                elementArraySelect.onchange = function () {
                    updatePreviewSettings();
                };
                col1.appendChild(elementArraySelect);
                elementArrayRow.appendChild(col1);
            }
            {
                var col2 = document.createElement("td");
                var span1 = document.createElement("span");
                span1.textContent = "Element Type: ";
                col2.appendChild(span1);
                elementArrayRow.appendChild(col2);
            }
            {
                var col3 = document.createElement("td");
                var sizeSelect = document.createElement("select");
                var sizeEnums = ["UNSIGNED_BYTE", "UNSIGNED_SHORT"];
                for (var n = 0; n < sizeEnums.length; n++) {
                    var option = document.createElement("option");
                    option.textContent = sizeEnums[n];
                    sizeSelect.appendChild(option);
                }
                sizeSelect.onchange = function () {
                    updatePreviewSettings();
                };
                col3.appendChild(sizeSelect);
                elementArrayRow.appendChild(col3);
            }
            previewOptions.appendChild(elementArrayRow);

            // Range settings
            var rangeRow = document.createElement("tr");
            {
                var col0 = document.createElement("td");
                var span0 = document.createElement("span");
                span0.textContent = "Start: ";
                col0.appendChild(span0);
                rangeRow.appendChild(col0);
            }
            {
                var col1 = document.createElement("td");
                var startInput = document.createElement("input");
                startInput.type = "text";
                startInput.value = "0";
                startInput.onchange = function () {
                    updatePreviewSettings();
                };
                col1.appendChild(startInput);
                rangeRow.appendChild(col1);
            }
            {
                var col2 = document.createElement("td");
                var span1 = document.createElement("span");
                span1.textContent = "Count: ";
                col2.appendChild(span1);
                rangeRow.appendChild(col2);
            }
            {
                var col3 = document.createElement("td");
                var countInput = document.createElement("input");
                countInput.type = "text";
                countInput.value = "0";
                countInput.onchange = function () {
                    updatePreviewSettings();
                };
                col3.appendChild(countInput);
                rangeRow.appendChild(col3);
            }
            previewOptions.appendChild(rangeRow);

            // Set all defaults based on draw state
            {
                var options = buffer.previewOptions;

                // Draw options
                modeSelect.selectedIndex = options.mode - gl.POINTS;
                attributeSelect.selectedIndex = options.positionIndex;

                // Element array buffer options
                if (options.elementArrayBuffer) {
                    // TODO: speed up lookup
                    for (var n = 0; n < elementArraySelect.options.length; n++) {
                        var option = elementArraySelect.options[n];
                        if (option.value == options.elementArrayBuffer[0].id) {
                            elementArraySelect.selectedIndex = n;
                            break;
                        }
                    }
                } else {
                    elementArraySelect.selectedIndex = 0; // unindexed
                }
                switch (options.elementArrayType) {
                    case gl.UNSIGNED_BYTE:
                        sizeSelect.selectedIndex = 0;
                        break;
                    case gl.UNSIGNED_SHORT:
                        sizeSelect.selectedIndex = 1;
                        break;
                }

                // Range options
                if (options.elementArrayBuffer) {
                    startInput.value = options.offset;
                } else {
                    startInput.value = options.first;
                }
                countInput.value = options.count;
            }

            previewContainer.appendChild(previewOptions);

            el.appendChild(previewContainer);
            helpers.appendbr(el);

            helpers.appendSeparator(el);
        }

        if (version.structure) {
            // TODO: some kind of fancy structure editor/overload?
            var attribs = version.structure;

            var structDiv = document.createElement("div");
            structDiv.className = "info-title-secondary";
            structDiv.textContent = "Structure (from last draw)";
            el.appendChild(structDiv);

            var table = document.createElement("table");
            table.className = "buffer-struct";

            var tr = document.createElement("tr");
            var td = document.createElement("th");
            td.textContent = "offset";
            tr.appendChild(td);
            td = document.createElement("th");
            td.textContent = "size";
            tr.appendChild(td);
            td = document.createElement("th");
            td.textContent = "type";
            tr.appendChild(td);
            td = document.createElement("th");
            td.textContent = "stride";
            tr.appendChild(td);
            td = document.createElement("th");
            td.textContent = "normalized";
            tr.appendChild(td);
            table.appendChild(tr);

            for (var n = 0; n < attribs.length; n++) {
                var attrib = attribs[n];

                var tr = document.createElement("tr");

                td = document.createElement("td");
                td.textContent = attrib.offset;
                tr.appendChild(td);
                td = document.createElement("td");
                td.textContent = attrib.size;
                tr.appendChild(td);
                td = document.createElement("td");
                switch (attrib.type) {
                    case gl.BYTE:
                        td.textContent = "BYTE";
                        break;
                    case gl.UNSIGNED_BYTE:
                        td.textContent = "UNSIGNED_BYTE";
                        break;
                    case gl.SHORT:
                        td.textContent = "SHORT";
                        break;
                    case gl.UNSIGNED_SHORT:
                        td.textContent = "UNSIGNED_SHORT";
                        break;
                    default:
                    case gl.FLOAT:
                        td.textContent = "FLOAT";
                        break;
                }
                tr.appendChild(td);
                td = document.createElement("td");
                td.textContent = attrib.stride;
                tr.appendChild(td);
                td = document.createElement("td");
                td.textContent = attrib.normalized;
                tr.appendChild(td);

                table.appendChild(tr);
            }

            el.appendChild(table);
            helpers.appendbr(el);
        }

        helpers.appendSeparator(el);

        generateBufferHistory(gl, el, buffer, version);
        helpers.appendbr(el);

        var frame = gl.ui.controller.currentFrame;
        if (frame) {
            helpers.appendSeparator(el);
            traceLine.generateUsageList(gl, el, frame, buffer);
            helpers.appendbr(el);
        }

        helpers.appendSeparator(el);

        var contentsDiv = document.createElement("div");
        contentsDiv.className = "info-title-secondary";
        contentsDiv.textContent = "Contents";
        el.appendChild(contentsDiv);

        var contentsContainer = document.createElement("div");

        function populateContents() {
            while (contentsContainer.hasChildNodes()) {
              contentsContainer.removeChild(contentsContainer.firstChild);
            }
            var frag = document.createDocumentFragment();
            switch (buffer.type) {
                case gl.ARRAY_BUFFER:
                    generateArrayBufferContents(gl, frag, buffer, version);
                    break;
                case gl.ELEMENT_ARRAY_BUFFER:
                    generateGenericArrayBufferContents(gl, frag, buffer, version);
                    break;
            }
            contentsContainer.appendChild(frag);
        };

        if (buffer.parameters[gl.BUFFER_SIZE] > 40000) {
            // Buffer is really big - delay populating
            var expandLink = document.createElement("span");
            expandLink.className = "buffer-data-collapsed";
            expandLink.textContent = "Show buffer contents";
            expandLink.onclick = function () {
                populateContents();
            };
            contentsContainer.appendChild(expandLink);
        } else {
            // Auto-expand
            populateContents();
        }

        el.appendChild(contentsContainer);

        helpers.appendbr(el);
    }

    BufferView.prototype.setBuffer = function (buffer) {
        this.currentBuffer = buffer;

        var node = this.elements.listing;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
        if (buffer) {
            var version;
            switch (this.window.activeVersion) {
                case null:
                    version = buffer.currentVersion;
                    break;
                case "current":
                    var frame = this.window.controller.currentFrame;
                    if (frame) {
                        version = frame.findResourceVersion(buffer);
                    }
                    version = version || buffer.currentVersion; // Fallback to live
                    break;
            }

            // Setup user preview options if not defined
            var lastDrawState = version.lastDrawState;
            if (!buffer.previewOptions && lastDrawState) {
                var elementArrayBufferArray = null;
                if (lastDrawState.elementArrayBuffer) {
                    elementArrayBufferArray = [lastDrawState.elementArrayBuffer, null];
                    // TODO: pick the right version of the ELEMENT_ARRAY_BUFFER
                    elementArrayBufferArray[1] = elementArrayBufferArray[0].currentVersion;
                }

                // TODO: pick the right position attribute
                var positionIndex = 0;
                var positionAttr = version.structure[positionIndex];

                var drawState = {
                    mode: lastDrawState.mode,
                    arrayBuffer: [buffer, version],
                    positionIndex: positionIndex,
                    position: positionAttr,
                    elementArrayBuffer: elementArrayBufferArray,
                    elementArrayType: lastDrawState.elementArrayBufferType,
                    first: lastDrawState.first,
                    offset: lastDrawState.offset,
                    count: lastDrawState.count
                };

                buffer.previewOptions = drawState;
            }

            try {
                this.inspector.setBuffer(buffer, version);
            } catch (e) {
                this.inspector.setBuffer(null, null);
                console.log("exception while setting up buffer preview: " + e);
            }

            generateBufferDisplay(this, this.window.context, this.elements.listing, buffer, version);
        } else {
            this.inspector.setBuffer(null, null);
        }

        this.elements.listing.scrollTop = 0;
    };

    return BufferView;
});
