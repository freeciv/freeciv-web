define([
        '../../shared/Base',
        '../../shared/Info',
        '../../shared/Utilities',
        '../../host/StateSnapshot',
        '../shared/BufferPreview',
        '../shared/PopupWindow',
        '../shared/TexturePreview',
        '../shared/TraceLine',
        '../Helpers',
    ], function (
        base,
        info,
        util,
        StateSnapshot,
        BufferPreview,
        PopupWindow,
        TexturePreviewGenerator,
        traceLine,
        helpers
    ) {

    function padValue(v, l) {
        v = String(v);
        var n = v.length;
        while (n < l) {
            v = " " + v;
            n++;
        }
        return v;
    };

    var DrawInfo = function (context, name) {
        base.subclass(PopupWindow, this, [context, name, "Draw Info", 863, 600]);
    };

    DrawInfo.prototype.setup = function () {
        var self = this;
        var context = this.context;

        // TODO: toolbar buttons/etc
    };

    DrawInfo.prototype.dispose = function () {
        this.bufferCanvas = null;
        this.bufferCanvasOuter = null;
        if (this.bufferPreviewer) {
            this.bufferPreviewer.dispose();
            this.bufferPreviewer = null;
        }
        if (this.texturePreviewer) {
            this.texturePreviewer.dispose();
            this.texturePreviewer = null;
        }

        this.canvas = null;
        this.gl = null;
    };

    DrawInfo.prototype.demandSetup = function () {
        // This happens around here to work around some Chromium issues with
        // creating WebGL canvases in differing documents

        if (this.gl) {
            return;
        }

        var doc = this.browserWindow.document;

        // TODO: move to shared code
        function prepareCanvas(canvas) {
            var frag = document.createDocumentFragment();
            frag.appendChild(canvas);
            var gl = util.getWebGLContext(canvas);
            return gl;
        };
        this.canvas = document.createElement("canvas");
        this.gl = prepareCanvas(this.canvas);

        this.texturePreviewer = new TexturePreviewGenerator();

        var bufferCanvas = this.bufferCanvas = doc.createElement("canvas");
        bufferCanvas.className = "gli-reset drawinfo-canvas";
        bufferCanvas.width = 256;
        bufferCanvas.height = 256;
        var bufferCanvasOuter = this.bufferCanvasOuter = doc.createElement("div");
        bufferCanvasOuter.style.position = "relative";
        bufferCanvasOuter.appendChild(bufferCanvas);

        this.bufferPreviewer = new BufferPreview(bufferCanvas);
        this.bufferPreviewer.setupDefaultInput();
    };

    DrawInfo.prototype.clear = function () {
        var doc = this.browserWindow.document;
        doc.title = "Draw Info";
        var node = this.elements.innerDiv;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
    };

    DrawInfo.prototype.addCallInfo = function (frame, call, drawInfo) {
        var self = this;
        var doc = this.browserWindow.document;
        var gl = this.gl;
        var innerDiv = this.elements.innerDiv;

        var panel = this.buildPanel();

        // Call line
        var callLine = doc.createElement("div");
        callLine.className = "drawinfo-call";
        traceLine.appendCallLine(this.context, callLine, frame, call);
        panel.appendChild(callLine);

        // ELEMENT_ARRAY_BUFFER (if an indexed call)
        if (call.name == "drawElements") {
            var elementArrayLine = doc.createElement("div");
            elementArrayLine.className = "drawinfo-elementarray trace-call-line";
            elementArrayLine.style.paddingLeft = "42px";
            elementArrayLine.textContent = "ELEMENT_ARRAY_BUFFER: "
            traceLine.appendObjectRef(this.context, elementArrayLine, drawInfo.args.elementArrayBuffer);
            panel.appendChild(elementArrayLine);
            helpers.appendClear(panel);
        }

        helpers.appendClear(innerDiv);
        helpers.appendbr(innerDiv);

        // Guess the position attribute
        var positionIndex = (function guessPositionIndex(attribInfos) {
            // Look for any attributes that sound like a position ('pos'/'position'/etc)
            // and have the right type (vec2/vec3/vec4)
            for (var n = 0; n < drawInfo.attribInfos.length; n++) {
                var attrib = drawInfo.attribInfos[n];
                if (attrib.name.toLowerCase().indexOf("pos") != -1) {
                    switch (attrib.type) {
                    case gl.INT_VEC2:
                    case gl.INT_VEC3:
                    case gl.INT_VEC4:
                    case gl.FLOAT_VEC2:
                    case gl.FLOAT_VEC3:
                    case gl.FLOAT_VEC4:
                        return n;
                    }
                }
            }

            // Look for the first vec3 attribute
            for (var n = 0; n < drawInfo.attribInfos.length; n++) {
                var attrib = drawInfo.attribInfos[n];
                if (attrib.type == gl.FLOAT_VEC3) {
                    return n;
                }
            }

            return -1;
        })(drawInfo.attribInfos);

        // Setup default preview options
        var previewOptions = null;
        if (positionIndex >= 0) {
            var positionBuffer = drawInfo.attribInfos[positionIndex].state.buffer;
            var indexBuffer = drawInfo.args.elementArrayBuffer;
            previewOptions = {
                mode: drawInfo.args.mode,
                arrayBuffer: [positionBuffer, positionBuffer.mirror.version],
                positionIndex: positionIndex,
                position: drawInfo.attribInfos[positionIndex].state,
                elementArrayBuffer: indexBuffer ? [indexBuffer, indexBuffer.mirror.version] : null,
                elementArrayType: drawInfo.args.elementArrayType,
                offset: drawInfo.args.offset,
                first: drawInfo.args.first,
                count: drawInfo.args.count
            };
        }

        // Buffer preview item
        var bufferDiv = doc.createElement("div");
        bufferDiv.className = "drawinfo-canvas-outer";
        bufferDiv.appendChild(this.bufferCanvasOuter);
        innerDiv.appendChild(bufferDiv);
        this.bufferPreviewer.setBuffer(previewOptions);
        this.bufferPreviewer.draw();

        // Frame preview item
        var frameDiv = doc.createElement("div");
        frameDiv.className = "drawinfo-canvas-outer";
        var cc = doc.createElement("canvas");
        cc.className = "gli-reset drawinfo-canvas drawinfo-canvas-trans";
        cc.width = 256;
        cc.height = 256;
        frameDiv.appendChild(cc);
        innerDiv.appendChild(frameDiv);

        // Isolated preview item
        var frameDiv = doc.createElement("div");
        frameDiv.className = "drawinfo-canvas-outer";
        var cc = doc.createElement("canvas");
        cc.className = "gli-reset drawinfo-canvas drawinfo-canvas-trans";
        cc.width = 256;
        cc.height = 256;
        frameDiv.appendChild(cc);
        innerDiv.appendChild(frameDiv);

        helpers.appendClear(innerDiv);
        helpers.appendbr(innerDiv);

        var optionsDiv = doc.createElement("div");
        optionsDiv.className = "drawinfo-options";

        var attributeSelect = doc.createElement("select");
        var maxAttribNameLength = 0;
        var maxBufferNameLength = 0;
        for (var n = 0; n < drawInfo.attribInfos.length; n++) {
            maxAttribNameLength = Math.max(maxAttribNameLength, drawInfo.attribInfos[n].name.length);
            var buffer = drawInfo.attribInfos[n].state.buffer;
            if (buffer) {
                maxBufferNameLength = Math.max(maxBufferNameLength, buffer.getName().length);
            }
        }
        for (var n = 0; n < drawInfo.attribInfos.length; n++) {
            var attrib = drawInfo.attribInfos[n];
            var option = doc.createElement("option");
            var typeString;
            switch (attrib.state.type) {
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
            option.textContent = padValue(attrib.name, maxAttribNameLength) + ": ";
            if (attrib.state.buffer) {
                option.textContent += padValue("[" + attrib.state.buffer.getName() + "]", maxBufferNameLength) + " " + padValue("+" + attrib.state.pointer, 4) + " / " + attrib.state.size + " * " + typeString;
            } else {
                option.textContent += util.typedArrayToString(attrib.state.value);
            }
            attributeSelect.appendChild(option);
        }
        attributeSelect.selectedIndex = positionIndex;
        attributeSelect.onchange = function () {
            frame.switchMirrors("drawinfo");
            previewOptions.positionIndex = attributeSelect.selectedIndex;
            previewOptions.position = drawInfo.attribInfos[previewOptions.positionIndex].state;
            var positionBuffer = drawInfo.attribInfos[previewOptions.positionIndex].state.buffer;
            previewOptions.arrayBuffer = [positionBuffer, positionBuffer.mirror.version];
            try {
                self.bufferPreviewer.setBuffer(previewOptions);
            } catch (e) {
                console.log("error trying to preview buffer: " + e);
            }
            self.bufferPreviewer.draw();
            frame.switchMirrors();
        };
        optionsDiv.appendChild(attributeSelect);

        innerDiv.appendChild(optionsDiv);

        helpers.appendClear(innerDiv);
        helpers.appendbr(innerDiv);
    };

    DrawInfo.prototype.appendTable = function (el, drawInfo, name, tableData, valueCallback) {
        var doc = this.browserWindow.document;
        var gl = this.gl;

        // [ordinal, name, size, type, optional value]
        var table = doc.createElement("table");
        table.className = "program-attribs";

        var tr = doc.createElement("tr");
        var td = doc.createElement("th");
        td.textContent = "idx";
        tr.appendChild(td);
        td = doc.createElement("th");
        td.className = "program-attribs-name";
        td.textContent = name + " name";
        tr.appendChild(td);
        td = doc.createElement("th");
        td.textContent = "size";
        tr.appendChild(td);
        td = doc.createElement("th");
        td.className = "program-attribs-type";
        td.textContent = "type";
        tr.appendChild(td);
        if (valueCallback) {
            td = doc.createElement("th");
            td.className = "program-attribs-value";
            td.textContent = "value";
            tr.appendChild(td);
        }
        table.appendChild(tr);

        for (var n = 0; n < tableData.length; n++) {
            var row = tableData[n];

            var tr = doc.createElement("tr");
            td = doc.createElement("td");
            td.textContent = row[0];
            tr.appendChild(td);
            td = doc.createElement("td");
            td.textContent = row[1];
            tr.appendChild(td);
            td = doc.createElement("td");
            td.textContent = row[2];
            tr.appendChild(td);
            td = doc.createElement("td");
            switch (row[3]) {
                case gl.FLOAT:
                    td.textContent = "FLOAT";
                    break;
                case gl.FLOAT_VEC2:
                    td.textContent = "FLOAT_VEC2";
                    break;
                case gl.FLOAT_VEC3:
                    td.textContent = "FLOAT_VEC3";
                    break;
                case gl.FLOAT_VEC4:
                    td.textContent = "FLOAT_VEC4";
                    break;
                case gl.INT:
                    td.textContent = "INT";
                    break;
                case gl.INT_VEC2:
                    td.textContent = "INT_VEC2";
                    break;
                case gl.INT_VEC3:
                    td.textContent = "INT_VEC3";
                    break;
                case gl.INT_VEC4:
                    td.textContent = "INT_VEC4";
                    break;
                case gl.BOOL:
                    td.textContent = "BOOL";
                    break;
                case gl.BOOL_VEC2:
                    td.textContent = "BOOL_VEC2";
                    break;
                case gl.BOOL_VEC3:
                    td.textContent = "BOOL_VEC3";
                    break;
                case gl.BOOL_VEC4:
                    td.textContent = "BOOL_VEC4";
                    break;
                case gl.FLOAT_MAT2:
                    td.textContent = "FLOAT_MAT2";
                    break;
                case gl.FLOAT_MAT3:
                    td.textContent = "FLOAT_MAT3";
                    break;
                case gl.FLOAT_MAT4:
                    td.textContent = "FLOAT_MAT4";
                    break;
                case gl.SAMPLER_2D:
                    td.textContent = "SAMPLER_2D";
                    break;
                case gl.SAMPLER_CUBE:
                    td.textContent = "SAMPLER_CUBE";
                    break;
            }
            tr.appendChild(td);

            if (valueCallback) {
                td = doc.createElement("td");
                valueCallback(n, td);
                tr.appendChild(td);
            }

            table.appendChild(tr);
        }

        el.appendChild(table);
    };

    DrawInfo.prototype.appendUniformInfos = function (el, drawInfo) {
        var self = this;
        var doc = this.browserWindow.document;
        var gl = this.gl;

        var uniformInfos = drawInfo.uniformInfos;
        var tableData = [];
        for (var n = 0; n < uniformInfos.length; n++) {
            var uniformInfo = uniformInfos[n];
            tableData.push([uniformInfo.index, uniformInfo.name, uniformInfo.size, uniformInfo.type]);
        }
        this.appendTable(el, drawInfo, "uniform", tableData, function (n, el) {
            var uniformInfo = uniformInfos[n];
            if (uniformInfo.textureValue) {
                // Texture value
                var texture = uniformInfo.textureValue;

                var samplerDiv = doc.createElement("div");
                samplerDiv.className = "drawinfo-sampler-value";
                samplerDiv.textContent = "Sampler: " + uniformInfo.value;
                el.appendChild(samplerDiv);
                el.appendChild(document.createTextNode(" "));
                traceLine.appendObjectRef(self.context, el, uniformInfo.textureValue);

                if (texture) {
                    var item = self.texturePreviewer.buildItem(self, doc, gl, texture, false, false);
                    item.className += " drawinfo-sampler-thumb";
                    el.appendChild(item);
                }
            } else {
                // Normal value
                switch (uniformInfo.type) {
                    case gl.FLOAT_MAT2:
                    case gl.FLOAT_MAT3:
                    case gl.FLOAT_MAT4:
                        helpers.appendMatrices(gl, el, uniformInfo.type, uniformInfo.size, uniformInfo.value);
                        break;
                    case gl.FLOAT:
                        el.textContent = " " + helpers.padFloat(uniformInfo.value);
                        break;
                    case gl.INT:
                    case gl.BOOL:
                        el.textContent = " " + helpers.padInt(uniformInfo.value);
                        break;
                    default:
                        if (uniformInfo.value.hasOwnProperty("length")) {
                            helpers.appendArray(el, uniformInfo.value);
                        } else {
                            // TODO: prettier display
                            el.textContent = uniformInfo.value;
                        }
                        break;
                }
            }
        });
    };

    DrawInfo.prototype.appendAttribInfos = function (el, drawInfo) {
        var self = this;
        var doc = this.browserWindow.document;
        var gl = this.gl;

        var attribInfos = drawInfo.attribInfos;
        var tableData = [];
        for (var n = 0; n < attribInfos.length; n++) {
            var attribInfo = attribInfos[n];
            tableData.push([attribInfo.index, attribInfo.name, attribInfo.size, attribInfo.type]);
        }
        this.appendTable(el, drawInfo, "attribute", tableData, function (n, el) {
            var attribInfo = attribInfos[n];
            if (attribInfo.state.buffer) {
                el.textContent = "Buffer: ";
                traceLine.appendObjectRef(self.context, el, attribInfo.state.buffer);
                var typeString;
                switch (attribInfo.state.type) {
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
                var specifierSpan = doc.createElement("span");
                specifierSpan.textContent = " " + padValue("+" + attribInfo.state.pointer, 4) + " / " + attribInfo.state.size + " * " + typeString + (attribInfo.state.normalized ? " N" : "");
                el.appendChild(specifierSpan);
            } else {
                el.textContent = "Constant: ";
                // TODO: pretty print
                el.textContent += attribInfo.state.value;
            }
        });
    };

    DrawInfo.prototype.addProgramInfo = function (frame, call, drawInfo) {
        var doc = this.browserWindow.document;
        var gl = this.gl;
        var innerDiv = this.elements.innerDiv;

        var panel = this.buildPanel();

        // Name
        var programLine = doc.createElement("div");
        programLine.className = "drawinfo-program trace-call-line";
        var frag = document.createDocumentFragment();
        var b = document.createElement("b");
        b.textContent = "Program";
        frag.appendChild(b);
        frag.appendChild(document.createTextNode(": "));
        programLine.appendChild(frag);

        traceLine.appendObjectRef(this.context, programLine, drawInfo.program);
        panel.appendChild(programLine);
        helpers.appendClear(panel);
        helpers.appendClear(innerDiv);
        helpers.appendbr(innerDiv);

        // Uniforms
        this.appendUniformInfos(innerDiv, drawInfo);
        helpers.appendbr(innerDiv);

        // Vertex attribs
        this.appendAttribInfos(innerDiv, drawInfo);
        helpers.appendbr(innerDiv);
    };

    DrawInfo.prototype.addStateInfo = function (frame, call, drawInfo) {
        var doc = this.browserWindow.document;
        var gl = this.gl;
        var innerDiv = this.elements.innerDiv;

        var panel = this.buildPanel();

        var programLine = doc.createElement("div");
        programLine.className = "drawinfo-program trace-call-line";
        var b = document.createElement("b");
        b.textContent = "State";
        programLine.appendChild(b);

        // TODO: link to state object
        panel.appendChild(programLine);
        helpers.appendClear(panel);
        helpers.appendClear(innerDiv);

        var vertexState = [
            "CULL_FACE",
            "CULL_FACE_MODE",
            "FRONT_FACE",
            "LINE_WIDTH"
        ];

        var fragmentState = [
            "BLEND",
            "BLEND_EQUATION_RGB",
            "BLEND_EQUATION_ALPHA",
            "BLEND_SRC_RGB",
            "BLEND_SRC_ALPHA",
            "BLEND_DST_RGB",
            "BLEND_DST_ALPHA",
            "BLEND_COLOR"
        ];

        var depthStencilState = [
            "DEPTH_TEST",
            "DEPTH_FUNC",
            "DEPTH_RANGE",
            "POLYGON_OFFSET_FILL",
            "POLYGON_OFFSET_FACTOR",
            "POLYGON_OFFSET_UNITS",
            "STENCIL_TEST",
            "STENCIL_FUNC",
            "STENCIL_REF",
            "STENCIL_VALUE_MASK",
            "STENCIL_FAIL",
            "STENCIL_PASS_DEPTH_FAIL",
            "STENCIL_PASS_DEPTH_PASS",
            "STENCIL_BACK_FUNC",
            "STENCIL_BACK_REF",
            "STENCIL_BACK_VALUE_MASK",
            "STENCIL_BACK_FAIL",
            "STENCIL_BACK_PASS_DEPTH_FAIL",
            "STENCIL_BACK_PASS_DEPTH_PASS"
        ];

        var outputState = [
            "VIEWPORT",
            "SCISSOR_TEST",
            "SCISSOR_BOX",
            "COLOR_WRITEMASK",
            "DEPTH_WRITEMASK",
            "STENCIL_WRITEMASK",
            "FRAMEBUFFER_BINDING"
        // TODO: RTT / renderbuffers/etc
        ];

        function generateStateTable(el, name, state, enumNames) {
            var titleDiv = doc.createElement("div");
            titleDiv.className = "info-title-master";
            titleDiv.textContent = name;
            el.appendChild(titleDiv);

            var table = doc.createElement("table");
            table.className = "info-parameters";

            var stateParameters = info.stateParameters;
            for (var n = 0; n < enumNames.length; n++) {
                var enumName = enumNames[n];
                var param = stateParameters[enumName];
                helpers.appendStateParameterRow(this.window, gl, table, state, param);
            }

            el.appendChild(table);
        };

        generateStateTable(innerDiv, "Vertex State", drawInfo.state, vertexState);
        generateStateTable(innerDiv, "Fragment State", drawInfo.state, fragmentState);
        generateStateTable(innerDiv, "Depth/Stencil State", drawInfo.state, depthStencilState);
        generateStateTable(innerDiv, "Output State", drawInfo.state, outputState);
    };

    DrawInfo.prototype.captureDrawInfo = function (frame, call) {
        var gl = this.gl;

        var drawInfo = {
            args: {
                mode: 0,
                elementArrayBuffer: null,
                elementArrayType: 0,
                first: 0,
                offset: 0,
                count: 0
            },
            program: null,
            uniformInfos: [],
            attribInfos: [],
            state: null
        };

        // Args
        switch (call.name) {
            case "drawArrays":
                drawInfo.args.mode = call.args[0];
                drawInfo.args.first = call.args[1];
                drawInfo.args.count = call.args[2];
                break;
            case "drawElements":
                drawInfo.args.mode = call.args[0];
                drawInfo.args.count = call.args[1];
                drawInfo.args.elementArrayType = call.args[2];
                drawInfo.args.offset = call.args[3];
                var glelementArrayBuffer = gl.getParameter(gl.ELEMENT_ARRAY_BUFFER_BINDING);
                drawInfo.args.elementArrayBuffer = glelementArrayBuffer ? glelementArrayBuffer.trackedObject : null;
                break;
        }

        // Program
        var glprogram = gl.getParameter(gl.CURRENT_PROGRAM);
        drawInfo.program = glprogram ? glprogram.trackedObject : null;
        if (glprogram) {
            drawInfo.uniformInfos = drawInfo.program.getUniformInfos(gl, glprogram);
            drawInfo.attribInfos = drawInfo.program.getAttribInfos(gl, glprogram);
        }

        // Capture entire state for blend mode/etc
        drawInfo.state = new StateSnapshot(gl);

        return drawInfo;
    };

    DrawInfo.prototype.inspectDrawCall = function (frame, drawCall) {
        var doc = this.browserWindow.document;
        doc.title = "Draw Info: #" + drawCall.ordinal + " " + drawCall.name;

        var innerDiv = this.elements.innerDiv;
        while (innerDiv.hasChildNodes()) {
          innerDiv.removeChild(innerDiv.firstChild);
        }

        this.demandSetup();

        // Prep canvas
        var width = frame.canvasInfo.width;
        var height = frame.canvasInfo.height;
        this.canvas.width = width;
        this.canvas.height = height;
        var gl = this.gl;

        // Prepare canvas
        frame.switchMirrors("drawinfo");
        frame.makeActive(gl, true, {
            ignoreTextureUploads: true
        });

        // Issue all calls (minus the draws we don't care about) and stop at our draw
        for (var n = 0; n < frame.calls.length; n++) {
            var call = frame.calls[n];

            if (call == drawCall) {
                // Call we want
            } else {
                // Skip other draws/etc
                switch (call.name) {
                    case "drawArrays":
                    case "drawElements":
                        continue;
                }
            }

            call.emit(gl);

            if (call == drawCall) {
                break;
            }
        }

        // Capture interesting draw info
        var drawInfo = this.captureDrawInfo(frame, drawCall);

        this.addCallInfo(frame, drawCall, drawInfo);
        this.addProgramInfo(frame, drawCall, drawInfo);
        this.addStateInfo(frame, drawCall, drawInfo);

        helpers.appendbr(innerDiv);

        // Restore all resource mirrors
        frame.switchMirrors(null);
    };

    return DrawInfo;
});
