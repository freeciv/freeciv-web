define([
        '../shared/Base',
        '../shared/Info',
        '../shared/Utilities',
    ], function (
        base,
        info,
        util
    ) {
    function appendbr(el) {
        var br = document.createElement("br");
        el.appendChild(br);
    };
    function appendClear(el) {
        var clearDiv = document.createElement("div");
        clearDiv.style.clear = "both";
        el.appendChild(clearDiv);
    };
    function appendSeparator(el) {
        var div = document.createElement("div");
        div.className = "info-separator";
        el.appendChild(div);
        appendbr(el);
    };
    function appendParameters(gl, el, obj, parameters, parameterEnumValues) {
        var table = document.createElement("table");
        table.className = "info-parameters";

        for (var n = 0; n < parameters.length; n++) {
            var enumName = parameters[n];
            var value = obj.parameters[gl[enumName]];

            var tr = document.createElement("tr");
            tr.className = "info-parameter-row";

            var tdkey = document.createElement("td");
            tdkey.className = "info-parameter-key";
            tdkey.textContent = enumName;
            tr.appendChild(tdkey);

            var tdvalue = document.createElement("td");
            tdvalue.className = "info-parameter-value";
            if (parameterEnumValues && parameterEnumValues[n]) {
                var valueFound = false;
                for (var m = 0; m < parameterEnumValues[n].length; m++) {
                    if (value == gl[parameterEnumValues[n][m]]) {
                        tdvalue.textContent = parameterEnumValues[n][m];
                        valueFound = true;
                        break;
                    }
                }
                if (!valueFound) {
                    tdvalue.textContent = value + " (unknown)";
                }
            } else {
                tdvalue.textContent = value; // TODO: convert to something meaningful?
            }
            tr.appendChild(tdvalue);

            table.appendChild(tr);
        }

        el.appendChild(table);
    };
    function appendStateParameterRow(w, gl, table, state, param) {
        var tr = document.createElement("tr");
        tr.className = "info-parameter-row";

        var tdkey = document.createElement("td");
        tdkey.className = "info-parameter-key";
        tdkey.textContent = param.name;
        tr.appendChild(tdkey);

        var value;
        if (param.value) {
            value = state[param.value];
        } else {
            value = state[param.name];
        }

        // Grab tracked objects
        if (value && value.trackedObject) {
            value = value.trackedObject;
        }

        var tdvalue = document.createElement("td");
        tdvalue.className = "info-parameter-value";

        var text = "";
        var clickhandler = null;

        var UIType = info.UIType;
        var ui = param.ui;
        switch (ui.type) {
            case UIType.ENUM:
                var anyMatches = false;
                for (var i = 0; i < ui.values.length; i++) {
                    var enumName = ui.values[i];
                    if (value == gl[enumName]) {
                        anyMatches = true;
                        text = enumName;
                    }
                }
                if (anyMatches == false) {
                    if (value === undefined) {
                        text = "undefined";
                    } else {
                        text = "?? 0x" + value.toString(16) + " ??";
                    }
                }
                break;
            case UIType.ARRAY:
                text = "[" + value + "]";
                break;
            case UIType.BOOL:
                text = value ? "true" : "false";
                break;
            case UIType.LONG:
                text = value;
                break;
            case UIType.ULONG:
                text = value;
                break;
            case UIType.COLORMASK:
                text = value;
                break;
            case UIType.OBJECT:
                // TODO: custom object output based on type
                text = value ? value : "null";
                if (value && value.target && util.isWebGLResource(value.target)) {
                    var typename = base.typename(value.target);
                    switch (typename) {
                        case "WebGLBuffer":
                            clickhandler = function () {
                                w.showBuffer(value, true);
                            };
                            break;
                        case "WebGLFramebuffer":
                            break;
                        case "WebGLProgram":
                            clickhandler = function () {
                                w.showProgram(value, true);
                            };
                            break;
                        case "WebGLRenderbuffer":
                            break;
                        case "WebGLShader":
                            break;
                        case "WebGLTexture":
                            clickhandler = function () {
                                w.showTexture(value, true);
                            };
                            break;
                    }
                    text = "[" + value.getName() + "]";
                } else if (util.isTypedArray(value)) {
                    text = "[" + value + "]";
                } else if (value) {
                    var typename = base.typename(value);
                    switch (typename) {
                        case "WebGLUniformLocation":
                            text = '"' + value.sourceUniformName + '"';
                            break;
                    }
                }
                break;
            case UIType.WH:
                text = value[0] + " x " + value[1];
                break;
            case UIType.RECT:
                if (value) {
                    text = value[0] + ", " + value[1] + " " + value[2] + " x " + value[3];
                } else {
                    text = "null";
                }
                break;
            case UIType.STRING:
                text = '"' + value + '"';
                break;
            case UIType.COLOR:
                var rgba = "rgba(" + (value[0] * 255) + ", " + (value[1] * 255) + ", " + (value[2] * 255) + ", " + value[3] + ")";
                var div = document.createElement("div");
                div.classList.add("info-parameter-color");
                div.style.backgroundColor = rgba;
                tdvalue.appendChild(div);
                text = " " + rgba;
                // TODO: color tip
                break;
            case UIType.FLOAT:
                text = value;
                break;
            case UIType.BITMASK:
                text = "0x" + value.toString(16);
                // TODO: bitmask tip
                break;
            case UIType.RANGE:
                text = value[0] + " - " + value[1];
                break;
            case UIType.MATRIX:
                switch (value.length) {
                    default: // ?
                        text = "[matrix]";
                        break;
                    case 4: // 2x2
                        text = "[matrix 2x2]";
                        break;
                    case 9: // 3x3
                        text = "[matrix 3x3]";
                        break;
                    case 16: // 4x4
                        text = "[matrix 4x4]";
                        break;
                }
                // TODO: matrix tip
                text = "[" + value + "]";
                break;
        }

        // Some td's have more than just text, assigning to textContent clears.
        tdvalue.appendChild(document.createTextNode(text));
        if (clickhandler) {
            tdvalue.className += " trace-call-clickable";
            tdvalue.onclick = function (e) {
                clickhandler();
                e.preventDefault();
                e.stopPropagation();
            };
        }

        tr.appendChild(tdvalue);

        table.appendChild(tr);
    };
    function appendContextAttributeRow(w, gl, table, state, param) {
        appendStateParameterRow(w, gl, table, state, {name: param, ui: { type: info.UIType.BOOL }});
    }
    function appendMatrices(gl, el, type, size, value) {
        switch (type) {
            case gl.FLOAT_MAT2:
                for (var n = 0; n < size; n++) {
                    var offset = n * 4;
                    appendMatrix(el, value, offset, 2);
                }
                break;
            case gl.FLOAT_MAT3:
                for (var n = 0; n < size; n++) {
                    var offset = n * 9;
                    appendMatrix(el, value, offset, 3);
                }
                break;
            case gl.FLOAT_MAT4:
                for (var n = 0; n < size; n++) {
                    var offset = n * 16;
                    appendMatrix(el, value, offset, 4);
                }
                break;
        }
    };
    function appendMatrix(el, value, offset, size) {
        var div = document.createElement("div");

        var openSpan = document.createElement("span");
        openSpan.textContent = "[";
        div.appendChild(openSpan);

        for (var i = 0; i < size; i++) {
            for (var j = 0; j < size; j++) {
                var v = value[offset + i * size + j];
                div.appendChild(document.createTextNode(padFloat(v)));
                if (!((i == size - 1) && (j == size - 1))) {
                    var comma = document.createElement("span");
                    comma.textContent = ", ";
                    div.appendChild(comma);
                }
            }
            if (i < size - 1) {
                appendbr(div);
                var prefix = document.createElement("span");
                prefix.textContent = " ";
                div.appendChild(prefix);
            }
        }

        var closeSpan = document.createElement("span");
        closeSpan.textContent = " ]";
        div.appendChild(closeSpan);

        el.appendChild(div);
    };
    function appendArray(el, value) {
        var div = document.createElement("div");

        var openSpan = document.createElement("span");
        openSpan.textContent = "[";
        div.appendChild(openSpan);

        var s = "";
        var maxIndex = Math.min(64, value.length);
        var isFloat = base.typename(value).indexOf("Float") >= 0;
        for (var n = 0; n < maxIndex; n++) {
            if (isFloat) {
                s += padFloat(value[n]);
            } else {
                s += " " + padInt(value[n]);
            }
            if (n < value.length - 1) {
                s += ", ";
            }
        }
        if (maxIndex < value.length) {
            s += ",... (" + (value.length) + " total)";
        }
        var strSpan = document.createElement("span");
        strSpan.textContent = s;
        div.appendChild(strSpan);

        var closeSpan = document.createElement("span");
        closeSpan.textContent = " ]";
        div.appendChild(closeSpan);

        el.appendChild(div);
    };
    function padInt(v) {
        var s = String(v);
        if (s >= 0) {
            s = " " + s;
        }
        s = s.substr(0, 11);
        while (s.length < 11) {
            s = " " + s;
        }
        return s;
    };
    function padFloat(v) {
        var s = String(v);
        if (s >= 0.0) {
            s = " " + s;
        }
        if (s.indexOf(".") == -1) {
            s += ".";
        }
        s = s.substr(0, 12);
        while (s.length < 12) {
            s += "0";
        }
        return s;
    };

    return {
        appendbr: appendbr,
        appendClear: appendClear,
        appendSeparator: appendSeparator,
        appendParameters: appendParameters,
        appendStateParameterRow: appendStateParameterRow,
        appendContextAttributeRow: appendContextAttributeRow,
        appendMatrices: appendMatrices,
        appendMatrix: appendMatrix,
        appendArray: appendArray,
        padFloat: padFloat,
        padInt: padInt,
    };

});
