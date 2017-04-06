define([
        'SyntaxHighlighterGLSL',
        '../../Helpers',
        '../../shared/SurfaceInspector',
        '../../shared/TraceLine',
    ], function (
        dummy,
        helpers,
        SurfaceInspector,
        traceLine
    ) {

    var ProgramView = function (w, elementRoot) {
        var self = this;
        this.window = w;
        this.elements = {
            view: elementRoot.getElementsByClassName("window-right-inner")[0]
        };

        this.currentProgram = null;
    };

    function prettyPrintSource(el, source, highlightLines) {
        var div = document.createElement("div");
        div.textContent = source;
        el.appendChild(div);

        var firstLine = 1;
        var firstChar = source.search(/./);
        if (firstChar > 0) {
            firstLine += firstChar;
        }

        SyntaxHighlighter.highlight({
            brush: 'glsl',
            'first-line': firstLine,
            highlight: highlightLines,
            toolbar: false
        }, div);
    };

    function generateShaderDisplay(gl, el, shader) {
        var shaderType = (shader.type == gl.VERTEX_SHADER) ? "Vertex" : "Fragment";

        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-secondary";
        titleDiv.textContent = shaderType + " " + shader.getName();
        el.appendChild(titleDiv);

        helpers.appendParameters(gl, el, shader, ["COMPILE_STATUS", "DELETE_STATUS"]);

        var highlightLines = [];
        if (shader.infoLog && shader.infoLog.length > 0) {
            var errorLines = shader.infoLog.match(/^ERROR: [0-9]+:[0-9]+: /gm);
            if (errorLines) {
                for (var n = 0; n < errorLines.length; n++) {
                    // expecting: 'ERROR: 0:LINE: '
                    var errorLine = errorLines[n];
                    errorLine = parseInt(errorLine.match(/ERROR: [0-9]+:([0-9]+): /)[1]);
                    highlightLines.push(errorLine);
                }
            }
        }

        var sourceDiv = document.createElement("div");
        sourceDiv.className = "shader-info-source";
        if (shader.source) {
            prettyPrintSource(sourceDiv, shader.source, highlightLines);
        } else {
            sourceDiv.textContent = "[no source uploaded]";
        }
        el.appendChild(sourceDiv);

        if (shader.infoLog && shader.infoLog.length > 0) {
            var infoDiv = document.createElement("div");
            infoDiv.className = "program-info-log";
            shader.infoLog.split("\n").forEach(function (line) {
              infoDiv.appendChild(document.createTextNode(line));
              infoDiv.appendChild(document.createElement("br"));
            });
            el.appendChild(infoDiv);
            helpers.appendbr(el);
        }
    };

    function appendTable(context, gl, el, program, name, tableData, valueCallback) {
        // [ordinal, name, size, type, optional value]
        var table = document.createElement("table");
        table.className = "program-attribs";

        var tr = document.createElement("tr");
        var td = document.createElement("th");
        td.textContent = "idx";
        tr.appendChild(td);
        td = document.createElement("th");
        td.className = "program-attribs-name";
        td.textContent = name + " name";
        tr.appendChild(td);
        td = document.createElement("th");
        td.textContent = "size";
        tr.appendChild(td);
        td = document.createElement("th");
        td.className = "program-attribs-type";
        td.textContent = "type";
        tr.appendChild(td);
        if (valueCallback) {
            td = document.createElement("th");
            td.className = "program-attribs-value";
            td.textContent = "value";
            tr.appendChild(td);
        }
        table.appendChild(tr);

        for (var n = 0; n < tableData.length; n++) {
            var row = tableData[n];

            var tr = document.createElement("tr");
            td = document.createElement("td");
            td.textContent = row[0];
            tr.appendChild(td);
            td = document.createElement("td");
            td.textContent = row[1];
            tr.appendChild(td);
            td = document.createElement("td");
            td.textContent = row[2];
            tr.appendChild(td);
            td = document.createElement("td");
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
                td = document.createElement("td");
                valueCallback(n, td);
                tr.appendChild(td);
            }

            table.appendChild(tr);
        }

        el.appendChild(table);
    };

    function appendUniformInfos(gl, el, program, isCurrent) {
        var tableData = [];
        var uniformInfos = program.getUniformInfos(gl, program.target);
        for (var n = 0; n < uniformInfos.length; n++) {
            var uniformInfo = uniformInfos[n];
            tableData.push([uniformInfo.index, uniformInfo.name, uniformInfo.size, uniformInfo.type]);
        }
        appendTable(gl, gl, el, program, "uniform", tableData, null);
    };

    function appendAttributeInfos(gl, el, program) {
        var tableData = [];
        var attribInfos = program.getAttribInfos(gl, program.target);
        for (var n = 0; n < attribInfos.length; n++) {
            var attribInfo = attribInfos[n];
            tableData.push([attribInfo.index, attribInfo.name, attribInfo.size, attribInfo.type]);
        }
        appendTable(gl, gl, el, program, "attribute", tableData, null);
    };

    function generateProgramDisplay(gl, el, program, version, isCurrent) {
        var titleDiv = document.createElement("div");
        titleDiv.className = "info-title-master";
        titleDiv.textContent = program.getName();
        el.appendChild(titleDiv);

        helpers.appendParameters(gl, el, program, ["LINK_STATUS", "VALIDATE_STATUS", "DELETE_STATUS", "ACTIVE_UNIFORMS", "ACTIVE_ATTRIBUTES"]);
        helpers.appendbr(el);

        if (program.parameters[gl.ACTIVE_UNIFORMS] > 0) {
            appendUniformInfos(gl, el, program, isCurrent);
            helpers.appendbr(el);
        }
        if (program.parameters[gl.ACTIVE_ATTRIBUTES] > 0) {
            appendAttributeInfos(gl, el, program);
            helpers.appendbr(el);
        }

        if (program.infoLog && program.infoLog.length > 0) {
            var infoDiv = document.createElement("div");
            infoDiv.className = "program-info-log";
            program.infoLog.split("\n").forEach(function (line) {
              infoDiv.appendChild(document.createTextNode(line));
              infoDiv.appendChild(document.createElement("br"));
            });
            el.appendChild(infoDiv);
            helpers.appendbr(el);
        }

        var frame = gl.ui.controller.currentFrame;
        if (frame) {
            helpers.appendSeparator(el);
            traceLine.generateUsageList(gl, el, frame, program);
            helpers.appendbr(el);
        }

        var vertexShader = program.getVertexShader(gl);
        var fragmentShader = program.getFragmentShader(gl);
        if (vertexShader) {
            var vertexShaderDiv = document.createElement("div");
            helpers.appendSeparator(el);
            generateShaderDisplay(gl, el, vertexShader);
        }
        if (fragmentShader) {
            var fragmentShaderDiv = document.createElement("div");
            helpers.appendSeparator(el);
            generateShaderDisplay(gl, el, fragmentShader);
        }
    };

    ProgramView.prototype.setProgram = function (program) {
        this.currentProgram = program;

        var node = this.elements.view;
        while (node.hasChildNodes()) {
          node.removeChild(node.firstChild);
        }
        if (program) {

            var version;
            var isCurrent = false;
            switch (this.window.activeVersion) {
                case null:
                    version = program.currentVersion;
                    break;
                case "current":
                    var frame = this.window.controller.currentFrame;
                    if (frame) {
                        version = frame.findResourceVersion(program);
                        isCurrent = true;
                    }
                    version = version || program.currentVersion; // Fallback to live
                    break;
            }

            generateProgramDisplay(this.window.context, this.elements.view, program, version, isCurrent);
        }

        this.elements.view.scrollTop = 0;
    };

    return ProgramView;
});
