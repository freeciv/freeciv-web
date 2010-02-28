// Copyright 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Known Issues: (From VML version)
//
// * Patterns are not implemented.
// * Radial gradient are not implemented. The VML version of these look very
//   different from the canvas one.
// * Coordsize. The width and height attribute have higher priority than the
//   width and height style values which isn't correct.
// * Painting mode isn't implemented.
// * Canvas width/height should is using content-box by default. IE in
//   Quirks mode will draw the canvas using border-box. Either change your
//   doctype to HTML5
//   (http://www.whatwg.org/specs/web-apps/current-work/#the-doctype)
//   or use Box Sizing Behavior from WebFX
//   (http://webfx.eae.net/dhtml/boxsizing/boxsizing.html)
// * Optimize. There is always room for speed improvements.

//Known Issues: Silverlight version
//
// * Doing a transformation during a path (ie lineTo, transform, lineTo) will
//   not work corerctly because the transform is done to the whole path (ie
//   transform, lineTo, lineTo)
// * Patterns are not yet implemented.


// only add this code if we do not already have a canvas implementation
if (!window.CanvasRenderingContext2D) {

(function () {

  var xamlId;

  var G_vmlCanvasManager_ = {
    init: function (opt_doc) {
      var doc = opt_doc || document;
      // Create a dummy element so that IE will allow canvas elements to be
      // recognized.
      doc.createElement('canvas');
      if (/MSIE/.test(navigator.userAgent) && !window.opera) {
        var self = this;

        createXamlScriptTag();

        doc.attachEvent('onreadystatechange', function () {
          self.init_(doc);
        });
      }
    },

    init_: function (doc) {
      // setup default css
      var ss = doc.createStyleSheet();
      ss.cssText = 'canvas{display:inline-block;overflow:hidden;' +
          // default size is 300x150 in Gecko and Opera
          'text-align:left;width:300px;height:150px}' +
          'canvas *{width:100%;height:100%;border:0;' +
          'background:transparen;margin:0}' +
          'canvas div {position:relative}' +
          // Place a div on top of the plugin.
          'canvas div div{position:absolute;top:0;' +
          // needs to be "non transparent"
          'filter:alpha(opacity=0);background:red}';

      // find all canvas elements
      var els = doc.getElementsByTagName('canvas');
      for (var i = 0; i < els.length; i++) {
        if (!els[i].getContext) {
          this.initElement(els[i]);
        }
      }
    },


    /**
     * Public initializes a canvas element so that it can be used as canvas
     * element from now on. This is called automatically before the page is
     * loaded but if you are creating elements using createElement you need to
     * make sure this is called on the element.
     * @param {HTMLElement} el The canvas element to initialize.
     * @return {HTMLElement} the element that was created.
     */
    initElement: function (el) {
      el.getContext = function () {
        if (this.context_) {
          return this.context_;
        }
        return this.context_ = new CanvasRenderingContext2D_(this);
      };

      var attrs = el.attributes;
      if (attrs.width && attrs.width.specified) {
        // TODO: use runtimeStyle and coordsize
        // el.getContext().setWidth_(attrs.width.nodeValue);
        el.style.width = attrs.width.nodeValue + 'px';
      } else {
        el.width = el.clientWidth;
      }
      if (attrs.height && attrs.height.specified) {
        // TODO: use runtimeStyle and coordsize
        // el.getContext().setHeight_(attrs.height.nodeValue);
        el.style.height = attrs.height.nodeValue + 'px';
      } else {
        el.height = el.clientHeight;
      }

      // insert object tag
      el.innerHTML = getObjectHtml();

      // do not use inline function because that will leak memory
      el.attachEvent('onpropertychange', onPropertyChange);
      return el;
    }
  };

  function onPropertyChange(e) {
    var el = e.srcElement;

    switch (e.propertyName) {
      case 'width':
        el.style.width = el.attributes.width.nodeValue + 'px';
        el.getContext().clearRect();
        break;
      case 'height':
        el.style.height = el.attributes.height.nodeValue + 'px';
        el.getContext().clearRect();
        break;
    }
  }

  G_vmlCanvasManager_.init();

  function createXamlScriptTag() {
    // This script tag contains the boilerplate XAML.
    document.write('<script type=text/xaml>' +
        '<Canvas x:Name="root" ' +
        'xmlns="http://schemas.microsoft.com/client/2007" ' +
        'xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" ' +
        'Width="300" ' +
        'Height="150" ' +
        'Background="Transparent"> ' +
        '</Canvas>' +
        '</script>');
    // Find the id of the writtenscript file.
    var scripts = document.scripts;
    var script = scripts[scripts.length - 1];
    xamlId = script.uniqueID;
    script.id = xamlId;
  }

  function getObjectHtml(fn) {
    return '<div><object type="application/x-silverlight" >' +
        '<param name="windowless" value="true">' +
        '<param name="background" value="transparent">' +
        '<param name="source" value="#' + xamlId + '">' +
        '</object><div></div></div>';
  }

  function hasSilverlight() {
    try {
      new ActiveXObject('AgControl.AgControl');
      return true;
    } catch(_) {
      return false;
    }
  }

  // precompute "00" to "FF"
  var dec2hex = [];
  for (var i = 0; i < 16; i++) {
    for (var j = 0; j < 16; j++) {
      dec2hex[i * 16 + j] = i.toString(16) + j.toString(16);
    }
  }

  function createMatrixIdentity() {
    return [
      [1, 0, 0],
      [0, 1, 0],
      [0, 0, 1]
    ];
  }

  function matrixMultiply(m1, m2) {
    var result = createMatrixIdentity();

    for (var x = 0; x < 3; x++) {
      for (var y = 0; y < 3; y++) {
        var sum = 0;

        for (var z = 0; z < 3; z++) {
          sum += m1[x][z] * m2[z][y];
        }

        result[x][y] = sum;
      }
    }
    return result;
  }

  function doTransform(ctx) {
    transformObject(ctx, getRoot(ctx), ctx.m_);
  }

  function transformObject(ctx, obj, m) {
    var transform = obj.renderTransform;
    var matrix;
    if (!transform) {
      transform = create(ctx, '<MatrixTransform/>');
      matrix = create(ctx, '<Matrix/>');
      transform.matrix = matrix;
      obj.renderTransform = transform;
    } else {
      matrix = transform.matrix;
    }

    matrix.m11 = m[0][0];
    matrix.m12 = m[0][1];
    matrix.m21 = m[1][0];
    matrix.m22 = m[1][1];
    matrix.offsetX = m[2][0];
    matrix.offsetY = m[2][1];
  }

  function copyState(o1, o2) {
    o2.fillStyle     = o1.fillStyle;
    o2.lineCap       = o1.lineCap;
    o2.lineJoin      = o1.lineJoin;
    o2.lineWidth     = o1.lineWidth;
    o2.miterLimit    = o1.miterLimit;
    o2.shadowBlur    = o1.shadowBlur;
    o2.shadowColor   = o1.shadowColor;
    o2.shadowOffsetX = o1.shadowOffsetX;
    o2.shadowOffsetY = o1.shadowOffsetY;
    o2.strokeStyle   = o1.strokeStyle;
    o2.globalAlpha   = o1.globalAlpha;
    o2.arcScaleX_    = o1.arcScaleX_;
    o2.arcScaleY_    = o1.arcScaleY_;
  }

  // precompute "00" to "FF"
  var decToHex = [];
  for (var i = 0; i < 16; i++) {
    for (var j = 0; j < 16; j++) {
      decToHex[i * 16 + j] = i.toString(16) + j.toString(16);
    }
  }

  // Silverlight does not support spelling gray as grey.
  var colorData = {
    darkgrey: '#A9A9A9',
    darkslategrey: '#2F4F4F',
    dimgrey: '#696969',
    grey: '#808080',
    lightgrey: '#D3D3D3',
    lightslategrey: '#778899',
    slategrey: '#708090'
  };


  function getRgbHslContent(styleString) {
    var start = styleString.indexOf('(', 3);
    var end = styleString.indexOf(')', start + 1);
    var parts = styleString.substring(start + 1, end).split(',');
    // add alpha if needed
    if (parts.length == 4 && styleString.substr(3, 1) == 'a') {
      alpha = +parts[3];
    } else {
      parts[3] = 1;
    }
    return parts;
  }

  function percent(s) {
    return parseFloat(s) / 100;
  }

  function clamp(v, min, max) {
    return Math.min(max, Math.max(min, v));
  }

  function hslToRgb(parts){
    var r, g, b;
    h = parseFloat(parts[0]) / 360 % 360;
    if (h < 0)
      h++;
    s = clamp(percent(parts[1]), 0, 1);
    l = clamp(percent(parts[2]), 0, 1);
    if (s == 0) {
      r = g = b = l; // achromatic
    } else {
      var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
      var p = 2 * l - q;
      r = hueToRgb(p, q, h + 1 / 3);
      g = hueToRgb(p, q, h);
      b = hueToRgb(p, q, h - 1 / 3);
    }

    return decToHex[Math.floor(r * 255)] +
        decToHex[Math.floor(g * 255)] +
        decToHex[Math.floor(b * 255)];
  }

  function hueToRgb(m1, m2, h) {
    if (h < 0)
      h++;
    if (h > 1)
      h--;

    if (6 * h < 1)
      return m1 + (m2 - m1) * 6 * h;
    else if (2 * h < 1)
      return m2;
    else if (3 * h < 2)
      return m1 + (m2 - m1) * (2 / 3 - h) * 6;
    else
      return m1;
  }

  function translateColor(styleString) {
    var str, alpha = 1;

    styleString = String(styleString);
    if (styleString.charAt(0) == '#') {
      return styleString;
    } else if (/^rgb/.test(styleString)) {
      var parts = getRgbHslContent(styleString);
      var str = '', n;
      for (var i = 0; i < 3; i++) {
        if (parts[i].indexOf('%') != -1) {
          n = Math.floor(percent(parts[i]) * 255);
        } else {
          n = +parts[i];
        }
        str += decToHex[clamp(n, 0, 255)];
      }
      alpha = parts[3];
    } else if (/^hsl/.test(styleString)) {
      var parts = getRgbHslContent(styleString);
      str = hslToRgb(parts);
      alpha = parts[3];
    } else if (styleString in colorData) {
      return colorData[styleString];
    } else {
      return styleString;
    }
    return '#' + dec2hex[Math.floor(alpha * 255)] + str;
  }

  function processLineCap(lineCap) {
    switch (lineCap) {
      case 'butt':
        return 'flat';
      case 'round':
        return 'round';
      case 'square':
      default:
        return 'square';
    }
  }

  function getRoot(ctx) {
    return ctx.canvas.firstChild.firstChild.content.findName('root');
  }

  function create(ctx, s, opt_args) {
    if (opt_args) {
      s = s.replace(/\%(\d+)/g, function(match, index) {
        return opt_args[+index - 1];
      });
    }

    try {
      return ctx.canvas.firstChild.firstChild.content.createFromXaml(s);
    } catch (ex) {
      throw Error('Could not create XAML from: ' + s);
    }
  }

  function drawShape(ctx, s, opt_args) {
    var canvas = ctx.lastCanvas_ || create(ctx, '<Canvas/>');
    var shape = create(ctx, s, opt_args);
    canvas.children.add(shape);
    transformObject(ctx, canvas, ctx.m_);
    if (!ctx.lastCanvas_) {
      getRoot(ctx).children.add(canvas);
      ctx.lastCanvas_ = canvas;
    }
    return shape;
  }

  function createBrushObject(ctx, value) {
    if (value instanceof CanvasGradient_) {
      return value.createBrush_(ctx);
    } else if (value instanceof CanvasPattern_) {
      throw Error('Not implemented');
    } else {
      return create(ctx, '<SolidColorBrush Color="%1"/>',
                    [translateColor(value)]);
    }
  }

  /**
   * This class implements CanvasRenderingContext2D interface as described by
   * the WHATWG.
   * @param {HTMLElement} surfaceElement The element that the 2D context should
   *     be associated with
   */
   function CanvasRenderingContext2D_(surfaceElement) {
    this.m_ = createMatrixIdentity();
    this.lastCanvas_ = null;

    this.mStack_ = [];
    this.aStack_ = [];
    this.currentPath_ = [];

    // Canvas context properties
    this.strokeStyle = '#000';
    this.fillStyle = '#000';

    this.lineWidth = 1;
    this.lineJoin = 'miter';
    this.lineCap = 'butt';
    this.miterLimit = 10;
    this.globalAlpha = 1;
    this.canvas = surfaceElement;
  };


  var contextPrototype = CanvasRenderingContext2D_.prototype;

  contextPrototype.clearRect = function() {
    var root = getRoot(this);
    root.children.clear();

    // TODO: Implement
    this.currentPath_ = [];
    this.lastCanvas_ = null;

  };

  contextPrototype.beginPath = function() {
    // TODO: Branch current matrix so that save/restore has no effect
    //       as per safari docs.

    this.currentPath_ = [];
  };

  contextPrototype.moveTo = function(aX, aY) {
    this.currentPath_.push('M' + aX + ',' + aY);
  };

  contextPrototype.lineTo = function(aX, aY) {
    if (this.currentPath_.length == 0) return;
    this.currentPath_.push('L' + aX + ',' + aY);
  };

  contextPrototype.bezierCurveTo = function(aCP1x, aCP1y,
                                            aCP2x, aCP2y,
                                            aX, aY) {
    if (this.currentPath_.length == 0) return;
    this.currentPath_.push('C' + aCP1x + ',' + aCP1y + ' ' +
                           aCP2x + ',' + aCP2y + ' ' +
                           aX + ' ' + aY);
  };

  contextPrototype.quadraticCurveTo = function(aCPx, aCPy, aX, aY) {
    if (this.currentPath_.length == 0) return;
    this.currentPath_.push('Q' + aCPx + ',' + aCPy + ' ' +
                           aX + ',' + aY);
  };

  contextPrototype.arcTo = function(x1, y1, x2, y2, radius) {
    if (this.currentPath_.length == 0) return;
    // TODO: Implement
  };

  contextPrototype.arc = function(aX, aY, aRadius,
                                  aStartAngle, aEndAngle, aClockwise) {
    var deltaAngle = Math.abs(aStartAngle - aEndAngle);
    // If start and stop are the same WebKit and Moz does nothing
    if (aStartAngle == aEndAngle) {
      // different browsers behave differently here so we do the easiest thing
      return;
    }

    var endX = aX + aRadius * Math.cos(aEndAngle);
    var endY = aY + aRadius * Math.sin(aEndAngle);

    if (deltaAngle >= 2 * Math.PI) {
      // if larger than 2PI
      this.arc(aX, aY, aRadius, aStartAngle, aStartAngle + Math.PI, aClockwise);
      this.arc(aX, aY, aRadius, aStartAngle + Math.PI,
               aStartAngle + 2 * Math.PI, aClockwise);
      // now move to end point
      this.moveTo(endX, endY);
      return;
    }

    var startX = aX + aRadius * Math.cos(aStartAngle);
    var startY = aY + aRadius * Math.sin(aStartAngle);
    var rotationAngle = deltaAngle * 180 / Math.PI; // sign, abs?
    var sweepDirection = aClockwise ? 0 : 1;
    var isLargeArc = rotationAngle >= 180 == Boolean(aClockwise) ? 0 : 1;

    if (this.currentPath_.length != 0) {
      // add line to start point
      this.lineTo(startX, startY);
    } else {
      this.moveTo(startX, startY);
    }

    this.currentPath_.push('A' + aRadius + ',' + aRadius + ' ' +
                           rotationAngle + ' ' +
                           isLargeArc + ' ' +
                           sweepDirection + ' ' +
                           endX + ',' + endY);
  };

  contextPrototype.rect = function(aX, aY, aWidth, aHeight) {
    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
  };

  contextPrototype.strokeRect = function(aX, aY, aWidth, aHeight) {
    // Will destroy any existing path (same as FF behaviour)
    this.beginPath();
    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
    this.stroke();
    this.currentPath_ = [];
  };

  contextPrototype.fillRect = function(aX, aY, aWidth, aHeight) {
    // Will destroy any existing path (same as FF behaviour)
    this.beginPath();
    this.moveTo(aX, aY);
    this.lineTo(aX + aWidth, aY);
    this.lineTo(aX + aWidth, aY + aHeight);
    this.lineTo(aX, aY + aHeight);
    this.closePath();
    this.fill();
    this.currentPath_ = [];
  };

  contextPrototype.createLinearGradient = function(aX0, aY0, aX1, aY1) {
    return new LinearCanvasGradient_(aX0, aY0, aX1, aY1);
  };

  contextPrototype.createRadialGradient = function(x0, y0,
                                                   r0, x1,
                                                   y1, r1) {
    return new RadialCanvasGradient_(x0, y0, r0, x1, y1, r1);
  };

  contextPrototype.drawImage = function (image, var_args) {
    var dx, dy, dw, dh, sx, sy, sw, sh;

    // For Silverlight we don't need to get the size of the image since
    // Silverlight uses the image original dimension if not provided.

    if (arguments.length == 3) {
      dx = arguments[1];
      dy = arguments[2];
      // Keep sx, sy, sw, dw, sh and dh undefined
    } else if (arguments.length == 5) {
      dx = arguments[1];
      dy = arguments[2];
      dw = arguments[3];
      dh = arguments[4];
      // Keep sx, sy, sw and sh undefined
    } else if (arguments.length == 9) {
      sx = arguments[1];
      sy = arguments[2];
      sw = arguments[3];
      sh = arguments[4];
      dx = arguments[5];
      dy = arguments[6];
      dw = arguments[7];
      dh = arguments[8];
    } else {
      throw Error('Invalid number of arguments');
    }

    var slImage;

    // If we have a source rect we need to clip the image.
    if (arguments.length == 9) {
      slImage = drawShape(this, '<Image Source="%1"/>', [image.src]);

      var clipRect = create(this,
          '<RectangleGeometry Rect="%1,%2,%3,%4"/>', [sx, sy, sw, sh]);
      slImage.clip = clipRect;

      var m = createMatrixIdentity();

      // translate to 0,0
      m[2][0] = -sx;
      m[2][1] = -sy;

      // scale
      var m2 = createMatrixIdentity();
      m2[0][0] = dw / sw;
      m2[1][1] = dh / sh;

      m = matrixMultiply(m, m2);

      // translate to destination
      m[2][0] += dx;
      m[2][1] += dy;

      transformObject(this, slImage, m);

    } else {
      slImage = drawShape(this,
          '<Image Source="%1" Canvas.Left="%2" Canvas.Top="%3"/>',
          [image.src, dx, dy]);
      if (dw != undefined || dh != undefined) {
        slImage.width = dw;
        slImage.height = dh;
        slImage.stretch = 'fill';
      }
    }
  };

  contextPrototype.stroke = function() {
    if (this.currentPath_.length == 0) return;
    var path = drawShape(this, '<Path Data="%1"/>',
                         [this.currentPath_.join(' ')]);
    path.stroke = createBrushObject(this, this.strokeStyle);
    path.opacity = this.globalAlpha;
    path.strokeThickness = this.lineWidth;
    path.strokeMiterLimit = this.miterLimit;
    path.strokeLineJoin = this.lineJoin;
    // Canvas does not differentiate start from end
    path.strokeEndLineCap = path.strokeStartLineCap =
        processLineCap(this.lineCap);
  };

  contextPrototype.fill = function() {
    if (this.currentPath_.length == 0) return;
    var path = drawShape(this, '<Path Data="%1"/>',
                         [this.currentPath_.join(' ')]);
    // The spec says to use non zero but Silverlight uses EvenOdd by defaul
    path.data.fillRule = 'NonZero';
    path.fill = createBrushObject(this, this.fillStyle);
    path.fill.opacity = this.globalAlpha;
    // TODO: What about even-odd etc?
  };

  contextPrototype.closePath = function() {
    this.currentPath_.push('z');
  };

  /**
   * Sets the transformation matrix and marks things as dirty
   */
  function setM(self, m) {
    self.m_ = m;
    self.lastCanvas_ = null;
  };

  contextPrototype.save = function() {
    var o = {};
    copyState(this, o);
    this.aStack_.push(o);
    this.mStack_.push(this.m_);
    setM(this, matrixMultiply(createMatrixIdentity(), this.m_));
  };

  contextPrototype.restore = function() {
    copyState(this.aStack_.pop(), this);
    setM(this, this.mStack_.pop());
  };

  contextPrototype.translate = function(aX, aY) {
    var m1 = [
      [1,  0,  0],
      [0,  1,  0],
      [aX, aY, 1]
    ];

    setM(this, matrixMultiply(m1, this.m_));
  };

  contextPrototype.rotate = function(aRot) {
    var c = Math.cos(aRot);
    var s = Math.sin(aRot);

    var m1 = [
      [c,  s, 0],
      [-s, c, 0],
      [0,  0, 1]
    ];

    setM(this, matrixMultiply(m1, this.m_));
  };

  contextPrototype.scale = function(aX, aY) {
    var m1 = [
      [aX, 0,  0],
      [0,  aY, 0],
      [0,  0,  1]
    ];

    setM(this, matrixMultiply(m1, this.m_));
  };

  contextPrototype.transform = function(m11, m12, m21, m22, dx, dy) {
    var m1 = [
      [m11, m12, 0],
      [m21, m22, 0],
      [ dx,  dy, 1]
    ];

    setM(this, matrixMultiply(m1, this.m_));
  };

  contextPrototype.setTransform = function(m11, m12, m21, m22, dx, dy) {
    setM(this, [
      [m11, m12, 0],
      [m21, m22, 0],
      [ dx,  dy, 1],
    ]);
  };

  contextPrototype.clip = function() {
    if (this.currentPath_.length) {
      var clip = this.currentPath_.join(' ');
      var canvas = create(this, '<Canvas Width="%1" Height="%2" Clip="%3"/>',
                          [getRoot(this).width, getRoot(this).height, clip]);
      var parent = this.lastCanvas_ || getRoot(this);

      parent.children.add(canvas);
      this.lastCanvas_ = canvas;
    }
  };

  contextPrototype.createPattern = function() {
    return new CanvasPattern_;
  };

  // Gradient / Pattern Stubs
  function CanvasGradient_() {
    this.colors_ = [];
  }

  CanvasGradient_.prototype.addColorStop = function(aOffset, aColor) {
    aColor = translateColor(aColor);
    this.colors_.push({offset: aOffset, color: aColor});
  };

  CanvasGradient_.prototype.createStops_ = function(ctx, brushObj, colors) {
    var gradientStopCollection = brushObj.gradientStops;
    for (var i = 0, c; c = colors[i]; i++) {
      var color = translateColor(c.color);
      gradientStopCollection.add(create(ctx,
          '<GradientStop Color="%1" Offset="%2"/>', [color, c.offset]));
    }
  };

  function LinearCanvasGradient_(x0, y0, x1, y1) {
    CanvasGradient_.call(this);
    this.x0_ = x0;
    this.y0_ = y0;
    this.x1_ = x1;
    this.y1_ = y1;
  }
  LinearCanvasGradient_.prototype = new CanvasGradient_;

  LinearCanvasGradient_.prototype.createBrush_ = function(ctx) {
    var brushObj = create(ctx, '<LinearGradientBrush MappingMode="Absolute" ' +
                          'StartPoint="%1,%2" EndPoint="%3,%4"/>',
                          [this.x0_, this.y0_, this.x1_, this.y1_]);
    this.createStops_(ctx, brushObj, this.colors_);
    return brushObj;
  };

  function isNanOrInfinite(v) {
    return isNaN(v) || !isFinite(v);
  }

  function RadialCanvasGradient_(x0, y0, r0, x1, y1, r1) {
    if (r0 < 0 || r1 < 0 || isNanOrInfinite(x0) || isNanOrInfinite(y0) ||
        isNanOrInfinite(x1) || isNanOrInfinite(y1)) {
      // IE does not support DOMException so this is as close as we get.
      var error = Error('DOMException.INDEX_SIZE_ERR');
      error.code = 1;
      throw error;
    }

    CanvasGradient_.call(this);
    this.x0_ = x0;
    this.y0_ = y0;
    this.r0_ = r0;
    this.x1_ = x1;
    this.y1_ = y1;
    this.r1_ = r1;
  }
  RadialCanvasGradient_.prototype = new CanvasGradient_;

  CanvasGradient_.prototype.createBrush_ = function(ctx) {
    if (this.x0_ == this.x1_ && this.y0_ == this.y1_ && this.r0_ == this.r1_) {
      return null;
    }

    var radius = Math.max(this.r0_, this.r1_);
    var minRadius = Math.min(this.r0_, this.r1_);
    var brushObj = create(ctx, '<RadialGradientBrush MappingMode="Absolute" ' +
                          'GradientOrigin="%1,%2" Center="%3,%4" ' +
                          'RadiusX="%5" RadiusY="%5"/>',
                          [this.x0_, this.y0_, this.x1_, this.y1_, radius]);

    var colors = this.colors_.concat();

    if (this.r1_ < this.r0_) {
      // reverse color stop array
      colors.reverse();
      for (var i = 0, c; c = colors[i]; i++) {
        c.offset = 1 - c.offset;
      }
    }

    // sort the color stops
    colors.sort(function(c1, c2) {
      return c1.offset - c2.offset;
    });

    if (minRadius > 0) {
      // We need to adjust the color stops since SL always have the inner radius
      // at (0, 0) so we change the stops in case the min radius is not 0.
      for (var i = 0, c; c = colors[i]; i++) {
        c.offset = minRadius / radius + (radius - minRadius) / radius * c.offset;
      }
    }

    this.createStops_(ctx, brushObj, colors);
    return brushObj;
  };

  function CanvasPattern_() {}

  // set up externs
  G_vmlCanvasManager = G_vmlCanvasManager_;
  CanvasRenderingContext2D = CanvasRenderingContext2D_;
  CanvasGradient = CanvasGradient_;
  CanvasPattern = CanvasPattern_;

})();

} // if
