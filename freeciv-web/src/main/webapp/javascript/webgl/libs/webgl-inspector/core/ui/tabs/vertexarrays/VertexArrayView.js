define([
        '../../Helpers',
        '../../shared/TraceLine',
    ], function (
        helpers,
        traceLine
    ) {

    var VertexArrayView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-right")[0],
            listing: elementRoot.getElementsByClassName("vertexarray-listing")[0]
        };

        this.currentVertexArray = null;
    };

    VertexArrayView.prototype.layout = function () {
    };

    function appendHistoryLine(gl, el, vertexArray, call) {
        traceLine.appendHistoryLine(gl, el, call);

        // TODO: other custom stuff?
    }

    function generateVertexArrayHistory(gl, el, vertexArray, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-secondary";
        titleDiv.textContent = "History";
        el.appendChild(titleDiv);

        var rootEl = document.createElement("div");
        rootEl.className = "vertexarray-history";
        el.appendChild(rootEl);

        for (var n = 0; n < version.calls.length; n++) {
            var call = version.calls[n];
            appendHistoryLine(gl, rootEl, vertexArray, call);
        }
    };

    function generateBufferValue(buffer) {
        if (buffer) {
            if (buffer.target) {
                return "[" + buffer.getName() + "]";
            }
            return "**unknown buffer";
        }
        return "null";
    }

    // Do I want the buffer at a certain state?
    function addBufferClickHandler(window, elem, buffer) {
      if (buffer && buffer.target) {
        elem.addEventListener('click', e => {
           window.showBuffer(buffer);
        });
        elem.className = elem.className + " vertexattrib-clickable";
      }
    }

    function generateVertexArrayDisplay(view, gl, el, vertexArray, version) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = vertexArray.getName();
        el.appendChild(titleDiv);

        {
            const table = document.createElement("table");
            table.className = "info-parameters";

            const tr = document.createElement("tr");
            tr.className = "info-parameter-row";
            helpers.appendElement(tr, "td", "ELEMENT_ARRAY_BUFFER");
            const bufElem = helpers.ui.appendElement(tr, "td", generateBufferValue(version.extras.elementBuffer.buffer), "vertexattrib-buffer");
            addBufferClickHandler(view.window, bufElem, version.extras.elementBuffer.buffer);

            table.appendChild(tr);
            el.appendChild(table);
        }

        helpers.ui.appendbr(el);

        {
            const table = document.createElement("table");
            table.className = "vertexarray-struct";

            let tr = document.createElement("tr");
            helpers.appendElement(tr, "th", "index");
            helpers.appendElement(tr, "th", "enabled");
            helpers.appendElement(tr, "th", "size");
            helpers.appendElement(tr, "th", "type");
            helpers.appendElement(tr, "th", "normalized");
            helpers.appendElement(tr, "th", "stride");
            helpers.appendElement(tr, "th", "offset");
            helpers.appendElement(tr, "th", "divisor");
            helpers.appendElement(tr, "th", "buffer");
            helpers.appendElement(tr, "th", "value");
            table.appendChild(tr);

            const defaultAttrib = {
                enabled: false,
                value: [0, 0, 0, 1],
                size: 4,
                type: gl.FLOAT,
                normalize: false,
                stride: 0,
                offset: 0,
                buffer: null,
                divisor: 0,
            };
            const attributes = version.extras.attributes;
            const maxAttributes = gl.getParameter(gl.MAX_VERTEX_ATTRIBS);
            for (let n = 0; n < maxAttributes; ++n) {
              const attribute = attributes[n] || defaultAttrib;
              let tr = document.createElement("tr");
              tr.className = attribute.enabled ? "vertexattrib-enabled" : "vertexattrib-disabled";

              helpers.appendElement(tr, "td", n, "vertexattrib-index");
              helpers.appendElement(tr, "td", attribute.enabled ? "●" : "", "vertexattrib-enabled");
              helpers.appendElement(tr, "td", attribute.size, "vertexattrib-size");
              helpers.appendElement(tr, "td", glc.enumToString(attribute.type), "vertexattrib-type");
              helpers.appendElement(tr, "td", attribute.normalized ? "true" : "false", "vertexattrib-normalized");
              helpers.appendElement(tr, "td", attribute.stride, "vertexattrib-stride");
              helpers.appendElement(tr, "td", attribute.offset, "vertexattrib-offset");
              helpers.appendElement(tr, "td", attribute.divisor || defaultAttrib.divisor, "vertexattrib-divisor");
              const bufElem = helpers.appendElement(tr, "td", generateBufferValue(attribute.buffer), "vertexattrib-buffer");
              addBufferClickHandler(view.window, bufElem, attribute.buffer);
              helpers.appendElement(tr, "td", attribute.value || defaultAttrib.value, "vertexattrib-value");

              table.appendChild(tr);
            }

            el.appendChild(table);
        }

        helpers.appendbr(el);
        helpers.appendSeparator(el);

        generateVertexArrayHistory(gl, el, vertexArray, version);
        helpers.appendbr(el);

        var frame = gl.ui.controller.currentFrame;
        if (frame) {
            helpers.appendSeparator(el);
            traceLine.generateUsageList(gl, el, frame, vertexArray);
            helpers.appendbr(el);
        }

        helpers.appendSeparator(el);
        helpers.appendbr(el);
    }

    VertexArrayView.prototype.setVertexArray = function (vertexArray) {
        this.currentVertexArray = vertexArray;

        var node = this.elements.listing;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
        if (vertexArray) {
            var version;
            switch (this.window.activeVersion) {
                case null:
                    version = vertexArray.currentVersion;
                    break;
                case "current":
                    var frame = this.window.controller.currentFrame;
                    if (frame) {
                        version = frame.findResourceVersion(vertexArray);
                    }
                    version = version || vertexArray.currentVersion; // Fallback to live
                    break;
            }

            // Setup user preview options if not defined
            var lastDrawState = version.lastDrawState;
            if (!vertexArray.previewOptions && lastDrawState) {
                var elementArrayBufferArray = null;
                if (lastDrawState.elementArrayBuffer) {
                    elementArrayBufferArray = [lastDrawState.elementArrayBuffer, null];
                    // TODO: pick the right version of the ELEMENT_ARRAY_BUFFER
                    elementArrayBufferArray[1] = elementArrayBufferArray[0].currentVersion;
                }
            }

            generateVertexArrayDisplay(this, this.window.context, this.elements.listing, vertexArray, version);
        }

        this.elements.listing.scrollTop = 0;
    };

    return VertexArrayView;
});
