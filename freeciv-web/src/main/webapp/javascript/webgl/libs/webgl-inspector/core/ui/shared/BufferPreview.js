define([
        '../../shared/m4',
        '../../shared/ShaderUtils',
        '../../shared/Utilities',
    ], function (
        m4,
        shaderUtils,
        util
    ) {

    var BufferPreview = function (canvas) {
        this.document = canvas.ownerDocument;
        this.canvas = canvas;
        this.drawState = null;

        var expandLink = this.expandLink = document.createElement("span");
        expandLink.className = "surface-inspector-collapsed";
        expandLink.textContent = "Show preview";
        expandLink.style.visibility = "collapse";
        canvas.parentNode.appendChild(expandLink);

        var gl = this.gl = util.getWebGLContext(canvas);
        this.programInfo = shaderUtils.createProgramInfo(gl, [
            `
                uniform mat4 u_projMatrix;
                uniform mat4 u_modelViewMatrix;
                uniform mat4 u_modelViewInvMatrix;
                uniform bool u_enableLighting;
                attribute vec4 a_position;
                attribute vec4 a_normal;
                varying vec3 v_lighting;
                void main() {
                    gl_Position = u_projMatrix * u_modelViewMatrix * a_position;
                    if (u_enableLighting) {
                        vec3 lightDirection = vec3(0.0, 0.0, 1.0);
                        vec4 normalT = u_modelViewInvMatrix * a_normal;
                        float lighting = max(dot(normalT.xyz, lightDirection), 0.0);
                        v_lighting = vec3(0.2, 0.2, 0.2) + vec3(1.0, 1.0, 1.0) * lighting;
                    } else {
                        v_lighting = vec3(1.0, 1.0, 1.0);
                    }
                    gl_PointSize = 3.0;
                }
             `,
             `
                precision highp float;
                uniform bool u_wireframe;
                varying vec3 v_lighting;
                void main() {
                    vec4 color;
                    if (u_wireframe) {
                        color = vec4(1.0, 1.0, 1.0, 0.4);
                    } else {
                        color = vec4(1.0, 0.0, 0.0, 1.0);
                    }
                    gl_FragColor = vec4(color.rgb * v_lighting, color.a);
                }
              `],
              ['a_position', 'a_normal']);

        this.programInfo.a_position = 0;
        this.programInfo.a_normal = 1;

        // Default state
        gl.clearColor(0.0, 0.0, 0.0, 0.0);
        gl.depthFunc(gl.LEQUAL);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
        gl.disable(gl.CULL_FACE);

        this.camera = {
            defaultDistance: 5,
            distance: 5,
            rotx: 0,
            roty: 0
        };
    };

    BufferPreview.prototype.resetCamera = function () {
        this.camera.distance = this.camera.defaultDistance;
        this.camera.rotx = 0;
        this.camera.roty = 0;
        this.draw();
    };

    BufferPreview.prototype.dispose = function () {
        var gl = this.gl;

        this.setBuffer(null);

        gl.deleteProgram(this.programInfo.program);
        this.programInfo = null;

        this.gl = null;
        this.canvas = null;
    };

    BufferPreview.prototype.draw = function () {
        var gl = this.gl;

        gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        if (!this.drawState) {
            return;
        }
        var ds = this.drawState;

        gl.useProgram(this.programInfo.program);

        // Setup projection matrix
        var zn = 0.1;
        var zf = 1000.0; // TODO: normalize depth range based on buffer?
        var fovy = 45.0 * Math.PI / 180;
        var aspectRatio = (this.canvas.width / this.canvas.height);

        var projMatrix = m4.perspective(fovy, aspectRatio, zn, zf)

        // Build the view matrix
        /*this.camera = {
        distance: 5,
        rotx: 0,
        roty: 0
        };*/

        let modelViewMatrix = m4.identity();
        modelViewMatrix = m4.translate(modelViewMatrix, [0, 0, -this.camera.distance * 5]);
        modelViewMatrix = m4.rotateY(modelViewMatrix, this.camera.rotx);
        modelViewMatrix = m4.rotateX(modelViewMatrix, this.camera.roty);

        modelViewInvMatrix = m4.transpose(m4.inverse(modelViewMatrix));

        shaderUtils.setUniforms(gl, this.programInfo, {
            u_projMatrix: projMatrix,
            u_modelViewMatrix: modelViewMatrix,
            u_modelViewInvMatrix: modelViewInvMatrix,
        });

        gl.enable(gl.DEPTH_TEST);
        gl.disable(gl.BLEND);

        if (!this.triBuffer) {
            // No custom buffer, draw raw user stuff
            shaderUtils.setUniforms(gl, this.programInfo, {
                u_enableLighting: 0,
                u_wireframe: 0,
            });
            gl.enableVertexAttribArray(this.programInfo.a_position);
            gl.disableVertexAttribArray(this.programInfo.a_normal);
            gl.bindBuffer(gl.ARRAY_BUFFER, this.arrayBufferTarget);
            gl.vertexAttribPointer(this.programInfo.a_position, ds.position.size, ds.position.type, ds.position.normalized, ds.position.stride, ds.position.offset);
            gl.vertexAttribPointer(this.programInfo.a_normal, 3, gl.FLOAT, false, ds.position.stride, 0);
            if (this.elementArrayBufferTarget) {
                gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.elementArrayBufferTarget);
                gl.drawElements(ds.mode, ds.count, ds.elementArrayType, ds.offset);
            } else {
                gl.drawArrays(ds.mode, ds.first, ds.count);
            }
        } else {
            // Draw triangles
            if (this.triBuffer) {
                shaderUtils.setUniforms(gl, this.programInfo, {
                    u_enableLighting: 1,
                    u_wireframe: 0,
                });
                gl.enableVertexAttribArray(this.programInfo.a_position);
                gl.enableVertexAttribArray(this.programInfo.a_normal);
                gl.bindBuffer(gl.ARRAY_BUFFER, this.triBuffer);
                gl.vertexAttribPointer(this.programInfo.a_position, 3, gl.FLOAT, false, 24, 0);
                gl.vertexAttribPointer(this.programInfo.a_normal, 3, gl.FLOAT, false, 24, 12);
                gl.drawArrays(gl.TRIANGLES, 0, this.triBuffer.count);
            }

            // Draw wireframe
            if (this.lineBuffer) {
                gl.enable(gl.DEPTH_TEST);
                gl.enable(gl.BLEND);
                shaderUtils.setUniforms(gl, this.programInfo, {
                    u_enableLighting: 0,
                    u_wireframe: 1,
                });
                gl.enableVertexAttribArray(this.programInfo.a_position);
                gl.disableVertexAttribArray(this.programInfo.a_normal);
                gl.bindBuffer(gl.ARRAY_BUFFER, this.lineBuffer);
                gl.vertexAttribPointer(this.programInfo.a_position, 3, gl.FLOAT, false, 0, 0);
                gl.vertexAttribPointer(this.programInfo.a_normal, 3, gl.FLOAT, false, 0, 0);
                gl.drawArrays(gl.LINES, 0, this.lineBuffer.count);
            }
        }
    };

    function extractAttribute(gl, buffer, version, attrib) {
        var data = buffer.constructVersion(gl, version);
        if (!data) {
            return null;
        }
        var dataBuffer = data.buffer ? data.buffer : data;

        var result = [];

        var byteAdvance = 0;
        switch (attrib.type) {
            case gl.BYTE:
            case gl.UNSIGNED_BYTE:
                byteAdvance = 1 * attrib.size;
                break;
            case gl.SHORT:
            case gl.UNSIGNED_SHORT:
                byteAdvance = 2 * attrib.size;
                break;
            default:
            case gl.FLOAT:
                byteAdvance = 4 * attrib.size;
                break;
        }
        var stride = attrib.stride ? attrib.stride : byteAdvance;
        var byteOffset = 0;
        while (byteOffset < data.byteLength) {
            var readView = null;
            switch (attrib.type) {
                case gl.BYTE:
                    readView = new Int8Array(dataBuffer, byteOffset, attrib.size);
                    break;
                case gl.UNSIGNED_BYTE:
                    readView = new Uint8Array(dataBuffer, byteOffset, attrib.size);
                    break;
                case gl.SHORT:
                    readView = new Int16Array(dataBuffer, byteOffset, attrib.size);
                    break;
                case gl.UNSIGNED_SHORT:
                    readView = new Uint16Array(dataBuffer, byteOffset, attrib.size);
                    break;
                default:
                case gl.FLOAT:
                    readView = new Float32Array(dataBuffer, byteOffset, attrib.size);
                    break;
            }

            // HACK: this is completely and utterly stupidly slow
            // TODO: speed up extracting attributes
            switch (attrib.size) {
                case 1:
                    result.push([readView[0], 0, 0, 0]);
                    break;
                case 2:
                    result.push([readView[0], readView[1], 0, 0]);
                    break;
                case 3:
                    result.push([readView[0], readView[1], readView[2], 0]);
                    break;
                case 4:
                    result.push([readView[0], readView[1], readView[2], readView[3]]);
                    break;
            }

            byteOffset += stride;
        }

        return result;
    };

    function buildTriangles(gl, drawState, start, count, positionData, indices) {
        var triangles = [];

        var end = start + count;

        // Emit triangles
        switch (drawState.mode) {
            case gl.TRIANGLES:
                if (indices) {
                    for (var n = start; n < end; n += 3) {
                        triangles.push([indices[n], indices[n + 1], indices[n + 2]]);
                    }
                } else {
                    for (var n = start; n < end; n += 3) {
                        triangles.push([n, n + 1, n + 2]);
                    }
                }
                break;
            case gl.TRIANGLE_FAN:
                if (indices) {
                    triangles.push([indices[start], indices[start + 1], indices[start + 2]]);
                    for (var n = start + 2; n < end; n++) {
                        triangles.push([indices[start], indices[n], indices[n + 1]]);
                    }
                } else {
                    triangles.push([start, start + 1, start + 2]);
                    for (var n = start + 2; n < end; n++) {
                        triangles.push([start, n, n + 1]);
                    }
                }
                break;
            case gl.TRIANGLE_STRIP:
                if (indices) {
                    for (var n = start; n < end - 2; n++) {
                        if (indices[n] == indices[n + 1]) {
                            // Degenerate
                            continue;
                        }
                        if (n % 2 == 0) {
                            triangles.push([indices[n], indices[n + 1], indices[n + 2]]);
                        } else {
                            triangles.push([indices[n + 2], indices[n + 1], indices[n]]);
                        }
                    }
                } else {
                    for (var n = start; n < end - 2; n++) {
                        if (n % 2 == 0) {
                            triangles.push([n, n + 1, n + 2]);
                        } else {
                            triangles.push([n + 2, n + 1, n]);
                        }
                    }
                }
                break;
        }

        return triangles;
    };

    // from tdl
    function normalize(a) {
        var r = [];
        var n = 0.0;
        var aLength = a.length;
        for (var i = 0; i < aLength; i++) {
            n += a[i] * a[i];
        }
        n = Math.sqrt(n);
        if (n > 0.00001) {
            for (var i = 0; i < aLength; i++) {
                r[i] = a[i] / n;
            }
        } else {
            r = [0, 0, 0];
        }
        return r;
    };

    // drawState: {
    //     mode: enum
    //     arrayBuffer: [value, version]
    //     position: { size: enum, type: enum, normalized: bool, stride: n, offset: n }
    //     elementArrayBuffer: [value, version]/null
    //     elementArrayType: UNSIGNED_BYTE/UNSIGNED_SHORT/null
    //     first: n (if no elementArrayBuffer)
    //     offset: n bytes (if elementArrayBuffer)
    //     count: n
    // }
    BufferPreview.prototype.setBuffer = function (drawState, force) {
        var self = this;
        var gl = this.gl;
        if (this.arrayBufferTarget) {
            this.arrayBuffer.deleteTarget(gl, this.arrayBufferTarget);
            this.arrayBufferTarget = null;
            this.arrayBuffer = null;
        }
        if (this.elementArrayBufferTarget) {
            this.elementArrayBuffer.deleteTarget(gl, this.elementArrayBufferTarget);
            this.elementArrayBufferTarget = null;
            this.elementArrayBuffer = null;
        }

        var maxPreviewBytes = 40000;
        if (drawState && !force && drawState.arrayBuffer[1].parameters[gl.BUFFER_SIZE] > maxPreviewBytes) {
            // Buffer is really big - delay populating
            this.expandLink.style.visibility = "visible";
            this.expandLink.onclick = function () {
                self.setBuffer(drawState, true);
                self.expandLink.style.visibility = "collapse";
            };
            this.drawState = null;
            this.draw();
        } else if (drawState) {
            if (drawState.arrayBuffer) {
                this.arrayBuffer = drawState.arrayBuffer[0];
                var version = drawState.arrayBuffer[1];
                this.arrayBufferTarget = this.arrayBuffer.createTarget(gl, version);
            }
            if (drawState.elementArrayBuffer) {
                this.elementArrayBuffer = drawState.elementArrayBuffer[0];
                var version = drawState.elementArrayBuffer[1];
                this.elementArrayBufferTarget = this.elementArrayBuffer.createTarget(gl, version);
            }

            // Grab all position data as a list of vec4
            var positionData = extractAttribute(gl, drawState.arrayBuffer[0], drawState.arrayBuffer[1], drawState.position);

            // Pull out indices (or null if none)
            var indices = null;
            if (drawState.elementArrayBuffer) {
                indices = drawState.elementArrayBuffer[0].constructVersion(gl, drawState.elementArrayBuffer[1]);
            }

            // Get interested range
            var start;
            var count = drawState.count;
            if (drawState.elementArrayBuffer) {
                // Indexed
                start = drawState.offset;
                switch (drawState.elementArrayType) {
                    case gl.UNSIGNED_BYTE:
                        start /= 1;
                        break;
                    case gl.UNSIGNED_SHORT:
                        start /= 2;
                        break;
                }
            } else {
                // Unindexed
                start = drawState.first;
            }

            // Get all triangles as a list of 3-set [v1,v2,v3] vertex indices
            var areTriangles = false;
            switch (drawState.mode) {
                case gl.TRIANGLES:
                case gl.TRIANGLE_FAN:
                case gl.TRIANGLE_STRIP:
                    areTriangles = true;
                    break;
            }
            if (areTriangles) {
                this.triangles = buildTriangles(gl, drawState, start, count, positionData, indices);
                var i;

                // Generate interleaved position + normal data from triangles as a TRIANGLES list
                var triData = new Float32Array(this.triangles.length * 3 * 3 * 2);
                i = 0;
                for (var n = 0; n < this.triangles.length; n++) {
                    var tri = this.triangles[n];
                    var v1 = positionData[tri[0]];
                    var v2 = positionData[tri[1]];
                    var v3 = positionData[tri[2]];

                    // a = v2 - v1
                    var a = [v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]];
                    // b = v3 - v1
                    var b = [v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]];
                    // a x b
                    var normal = normalize([a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]);

                    triData[i++] = v1[0]; triData[i++] = v1[1]; triData[i++] = v1[2];
                    triData[i++] = normal[0]; triData[i++] = normal[1]; triData[i++] = normal[2];
                    triData[i++] = v2[0]; triData[i++] = v2[1]; triData[i++] = v2[2];
                    triData[i++] = normal[0]; triData[i++] = normal[1]; triData[i++] = normal[2];
                    triData[i++] = v3[0]; triData[i++] = v3[1]; triData[i++] = v3[2];
                    triData[i++] = normal[0]; triData[i++] = normal[1]; triData[i++] = normal[2];
                }
                this.triBuffer = gl.createBuffer();
                this.triBuffer.count = this.triangles.length * 3;
                gl.bindBuffer(gl.ARRAY_BUFFER, this.triBuffer);
                gl.bufferData(gl.ARRAY_BUFFER, triData, gl.STATIC_DRAW);

                // Generate LINES list for wireframe
                var lineData = new Float32Array(this.triangles.length * 3 * 2 * 3);
                i = 0;
                for (var n = 0; n < this.triangles.length; n++) {
                    var tri = this.triangles[n];
                    var v1 = positionData[tri[0]];
                    var v2 = positionData[tri[1]];
                    var v3 = positionData[tri[2]];
                    lineData[i++] = v1[0]; lineData[i++] = v1[1]; lineData[i++] = v1[2];
                    lineData[i++] = v2[0]; lineData[i++] = v2[1]; lineData[i++] = v2[2];
                    lineData[i++] = v2[0]; lineData[i++] = v2[1]; lineData[i++] = v2[2];
                    lineData[i++] = v3[0]; lineData[i++] = v3[1]; lineData[i++] = v3[2];
                    lineData[i++] = v3[0]; lineData[i++] = v3[1]; lineData[i++] = v3[2];
                    lineData[i++] = v1[0]; lineData[i++] = v1[1]; lineData[i++] = v1[2];
                }
                this.lineBuffer = gl.createBuffer();
                this.lineBuffer.count = this.triangles.length * 3 * 2;
                gl.bindBuffer(gl.ARRAY_BUFFER, this.lineBuffer);
                gl.bufferData(gl.ARRAY_BUFFER, lineData, gl.STATIC_DRAW);
            } else {
                this.triangles = null;
                this.triBuffer = null;
                this.lineBuffer = null;
            }

            // Determine the extents of the interesting region
            var minx = Number.MAX_VALUE; var miny = Number.MAX_VALUE; var minz = Number.MAX_VALUE;
            var maxx = Number.MIN_VALUE; var maxy = Number.MIN_VALUE; var maxz = Number.MIN_VALUE;
            if (indices) {
                for (var n = start; n < start + count; n++) {
                    var vec = positionData[indices[n]];
                    minx = Math.min(minx, vec[0]); maxx = Math.max(maxx, vec[0]);
                    miny = Math.min(miny, vec[1]); maxy = Math.max(maxy, vec[1]);
                    minz = Math.min(minz, vec[2]); maxz = Math.max(maxz, vec[2]);
                }
            } else {
                for (var n = start; n < start + count; n++) {
                    var vec = positionData[n];
                    minx = Math.min(minx, vec[0]); maxx = Math.max(maxx, vec[0]);
                    miny = Math.min(miny, vec[1]); maxy = Math.max(maxy, vec[1]);
                    minz = Math.min(minz, vec[2]); maxz = Math.max(maxz, vec[2]);
                }
            }
            var maxd = 0;
            var extents = [minx, miny, minz, maxx, maxy, maxz];
            for (var n = 0; n < extents.length; n++) {
                maxd = Math.max(maxd, Math.abs(extents[n]));
            }

            // Now have a bounding box for the mesh
            // TODO: set initial view based on bounding box
            this.camera.defaultDistance = maxd;
            this.resetCamera();

            this.drawState = drawState;
            this.draw();
        } else {
            this.drawState = null;
            this.draw();
        }
    };

    BufferPreview.prototype.setupDefaultInput = function () {
        var self = this;

        // Drag rotate
        var lastValueX = 0;
        var lastValueY = 0;
        function mouseMove(e) {
            var dx = e.screenX - lastValueX;
            var dy = e.screenY - lastValueY;
            lastValueX = e.screenX;
            lastValueY = e.screenY;

            var camera = self.camera;
            camera.rotx += dx * Math.PI / 180;
            camera.roty += dy * Math.PI / 180;
            self.draw();

            e.preventDefault();
            e.stopPropagation();
        };
        function mouseUp(e) {
            endDrag();
            e.preventDefault();
            e.stopPropagation();
        };
        function beginDrag() {
            self.document.addEventListener("mousemove", mouseMove, true);
            self.document.addEventListener("mouseup", mouseUp, true);
            self.canvas.style.cursor = "move";
            self.document.body.style.cursor = "move";
        };
        function endDrag() {
            self.document.removeEventListener("mousemove", mouseMove, true);
            self.document.removeEventListener("mouseup", mouseUp, true);
            self.canvas.style.cursor = "";
            self.document.body.style.cursor = "";
        };
        this.canvas.onmousedown = function (e) {
            beginDrag();
            lastValueX = e.screenX;
            lastValueY = e.screenY;
            e.preventDefault();
            e.stopPropagation();
        };

        // Zoom
        this.canvas.onmousewheel = function (e) {
            var delta = 0;
            if (e.wheelDelta) {
                delta = e.wheelDelta / 120;
            } else if (e.detail) {
                delta = -e.detail / 3;
            }
            if (delta) {
                var camera = self.camera;
                camera.distance -= delta * (camera.defaultDistance / 10.0);
                camera.distance = Math.max(camera.defaultDistance / 10.0, camera.distance);
                self.draw();
            }

            e.preventDefault();
            e.stopPropagation();
            e.returnValue = false;
        };
        this.canvas.addEventListener("DOMMouseScroll", this.canvas.onmousewheel, false);
    };

    return BufferPreview;
});
