/* $Id$ */

/** 
 * @projectDescription An cross-browser implementation of the HTML5 <canvas> text methods
 * @author Fabien Ménager
 * @version $Revision$
 * @license MIT License <http://www.opensource.org/licenses/mit-license.php>
 */

/**
 * Known issues:
 * - The 'light' font weight is not supported, neither is the 'oblique' font style.
 * - Optimize the different hacks (for Opera9)
 */

window.Canvas = window.Canvas || {};
window.Canvas.Text = {
  // http://mondaybynoon.com/2007/04/02/linux-font-equivalents-to-popular-web-typefaces/
  equivalentFaces: {
    // Web popular fonts
    'arial': ['liberation sans', 'nimbus sans l', 'freesans'],
    'times new roman': ['liberation serif', 'linux libertine', 'freeserif'],
    'courier new': ['dejavu sans mono', 'liberation mono', 'nimbus mono l', 'freemono'],
    'georgia': ['nimbus roman no9 l'],
    'helvetica': ['nimbus sans l', 'freesans'],
    'tahoma': ['dejavu sans', 'bitstream vera sans'],
    'verdana': ['dejavu sans', 'bitstream vera sans']
  },
  genericFaces: {
    'serif': ['times new roman', 'georgia', 'garamond', 'bodoni', 'minion web', 'itc stone serif', 'bitstream cyberbit'],
    'sans-serif': ['arial', 'verdana', 'trebuchet', 'tahoma', 'helvetica', 'itc avant garde gothic', 'univers', 'futura', 
                   'gill sans', 'akzidenz grotesk', 'attika', 'typiko new era', 'itc stone sans', 'monotype gill sans 571'],
    'monospace': ['courier', 'courier new', 'prestige', 'everson mono'],
    'cursive': ['caflisch script', 'adobe poetica', 'sanvito', 'ex ponto', 'snell roundhand', 'zapf-chancery'],
    'fantasy': ['alpha geometrique', 'critter', 'cottonwood', 'fb reactor', 'studz']
  },
  faces: {},
  scaling: 0.962,
  _styleCache: {}
};

/** The implementation of the text functions */
(function(){
  var isOpera9 = (window.opera && navigator.userAgent.match(/Opera\/9/)), // It seems to be faster when the hacked methods are used. But there are artifacts with Opera 10.
      proto = window.CanvasRenderingContext2D ? window.CanvasRenderingContext2D.prototype : document.createElement('canvas').getContext('2d').__proto__,
      ctxt = window.Canvas.Text;

  // Global options
  ctxt.options = {
    fallbackCharacter: ' ', // The character that will be drawn when not present in the font face file
    dontUseMoz: false, // Don't use the builtin Firefox 3.0 functions (mozDrawText, mozPathText and mozMeasureText)
    reimplement: false, // Don't use the builtin official functions present in Chrome 2, Safari 4, and Firefox 3.1+
    debug: false, // Debug mode, not used yet
    autoload: false // Specify the directory containing the face files or false
  };
  
  function initialize(){
    var libFileName = 'canvas.text.js',
        scripts = document.getElementsByTagName("script"), i, j;

    for (i = 0; i < scripts.length; i++) {
      var src = scripts[i].src;
      if (src.indexOf(libFileName) != -1) {
        var parts = src.split('?');
        ctxt.basePath = parts[0].replace(libFileName, '');
        if (parts[1]) {
          var options = parts[1].split('&');
          for (j = options.length-1; j >= 0; --j) {
            var pair = options[j].split('=');
            ctxt.options[pair[0]] = pair[1];
          }
        }
        break;
      }
    }
  }
  initialize();
  
  // What is the browser's implementation ?
  var moz = !ctxt.options.dontUseMoz && proto.mozDrawText && !proto.strokeText;

  // If the text functions are already here : nothing to do !
  if (proto.strokeText && !ctxt.options.reimplement) {
    // This property is needed, when including the font face files
    return window._typeface_js = {loadFace: function(){}};
  }
  
  function getCSSWeightEquivalent(weight){
    switch(String(weight)) {
      case 'bolder':
      case 'bold':
      case '900':
      case '800':
      case '700': return 'bold';
      case '600':
      case '500':
      case '400':
      default:
      case 'normal': return 'normal';
      //default: return 'light';
    }
  }
  
  function getElementStyle(e){
    if (document.defaultView && document.defaultView.getComputedStyle) {
      return document.defaultView.getComputedStyle(e, null);
    }
    return e.currentStyle || e.style;
  }
  
  function getXHR(){
    if (!ctxt.xhr) {
      var methods = [
        function(){return new XMLHttpRequest()},
        function(){return new ActiveXObject('Msxml2.XMLHTTP')},
        function(){return new ActiveXObject('Microsoft.XMLHTTP')}
      ];
      for (i = 0; i < methods.length; i++) {
        try {
          ctxt.xhr = methods[i](); 
          break;
        } 
        catch (e) {}
      }
    }
    return ctxt.xhr;
  }

  function arrayContains(a, v){
    var i, l = a.length;
    for (i = l-1; i >= 0; --i) if (a[i] === v) return true;
    return false;
  }
  
  ctxt.lookupFamily = function(family){
    var faces = this.faces, face, i, f, list,
        equiv = this.equivalentFaces,
        generic = this.genericFaces;
        
    if (faces[family]) return faces[family];
    
    if (generic[family]) {
      for (i = 0; i < generic[family].length; i++) {
        if (f = this.lookupFamily(generic[family][i])) return f;
      }
    }
    
    if (!(list = equiv[family])) return false;

    for (i = 0; i < list.length; i++)
      if (face = faces[list[i]]) return face;
    return false;
  }

  ctxt.getFace = function(family, weight, style){
    var face = this.lookupFamily(family);
    if (!face) return false;
    
    if (face && 
        face[weight] && 
        face[weight][style]) return face[weight][style];
    
    if (!this.options.autoload) return false;
    
    var faceName = (family.replace(/[ -]/g, '_')+'-'+weight+'-'+style),
        xhr = this.xhr,
        url = this.basePath+this.options.autoload+'/'+faceName+'.js';

    xhr = getXHR();
    xhr.open("get", url, false);
    xhr.send(null);
    if(xhr.status == 200) {
      eval(xhr.responseText);
      return this.faces[family][weight][style];
    }
    else throw 'Unable to load the font ['+family+' '+weight+' '+style+']';
    return false;
  };
  
  ctxt.loadFace = function(data){
    var family = data.familyName.toLowerCase();
    this.faces[family] = this.faces[family] || {};
    this.faces[family][data.cssFontWeight] = this.faces[family][data.cssFontWeight] || {};
    this.faces[family][data.cssFontWeight][data.cssFontStyle] = data;
    return data;
  };

  // To use the typeface.js face files
  window._typeface_js = {faces: ctxt.faces, loadFace: ctxt.loadFace};
  
  ctxt.getFaceFromStyle = function(style){
    var weight = getCSSWeightEquivalent(style.weight),
        families = style.family, i, face;
        
    for (i = 0; i < families.length; i++) {
      if (face = this.getFace(families[i].toLowerCase(), weight, style.style)) {
        return face;
      }
    }
    return false;
  };
  
  // Default values
  // Firefox 3.5 throws an error when redefining these properties
  try {
    proto.font = "10px sans-serif";
    proto.textAlign = "start";
    proto.textBaseline = "alphabetic";
  }
  catch(e){}
  
  proto.parseStyle = function(styleText){
    if (ctxt._styleCache[styleText]) return this.getComputedStyle(ctxt._styleCache[styleText]);
    
    var style = {}, computedStyle, families;
    
    if (!this._elt) {
      this._elt = document.createElement('span');
      this.canvas.appendChild(this._elt);
    }
    
    // Default style
    this.canvas.font = '10px sans-serif';
    this._elt.style.font = styleText;
    
    computedStyle = getElementStyle(this._elt);
    style.size = computedStyle.fontSize;
    style.weight = getCSSWeightEquivalent(computedStyle.fontWeight);
    style.style = computedStyle.fontStyle;
    
    families = computedStyle.fontFamily.split(',');
    for(i = 0; i < families.length; i++) {
      families[i] = families[i].replace(/^["'\s]*/, '').replace(/["'\s]*$/, '');
    }
    style.family = families;
    return this.getComputedStyle(ctxt._styleCache[styleText] = style);
  };
  
  proto.buildStyle = function (style){
    return style.style+' '+style.weight+' '+style.size+'px "'+style.family+'"';
  };

  proto.renderText = function(text, style){
    var face = ctxt.getFaceFromStyle(style),
        scale = (style.size / face.resolution) * 0.75,
        offset = 0, i, 
        chars = text.split(''), 
        length = chars.length;
    
    if (!isOpera9) {
      this.scale(scale, -scale);
      this.lineWidth /= scale;
    }
    
    for (i = 0; i < length; i++) {
      offset += this.renderGlyph(chars[i], face, scale, offset);
    }
  };

  if (isOpera9) {
    proto.renderGlyph = function(c, face, scale, offset){
      var i, cpx, cpy, outline, action, glyph = face.glyphs[c], length;
      
      if (!glyph) return;
  
      if (glyph.o) {
        outline = glyph._cachedOutline || (glyph._cachedOutline = glyph.o.split(' '));
        length = outline.length;
        for (i = 0; i < length; ) {
          action = outline[i++];
  
          switch(action) {
            case 'm':
              this.moveTo(outline[i++]*scale+offset, outline[i++]*-scale);
              break;
            case 'l':
              this.lineTo(outline[i++]*scale+offset, outline[i++]*-scale);
              break;
            case 'q':
              cpx = outline[i++]*scale+offset;
              cpy = outline[i++]*-scale;
              this.quadraticCurveTo(outline[i++]*scale+offset, outline[i++]*-scale, cpx, cpy);
              break;
          }
        }
      }
      return glyph.ha*scale;
    };
  }
  else {
    proto.renderGlyph = function(c, face){
      var i, cpx, cpy, outline, action, glyph = face.glyphs[c], length;
      
      if (!glyph) return;

      if (glyph.o) {
        outline = glyph._cachedOutline || (glyph._cachedOutline = glyph.o.split(' '));
        length = outline.length;
        for (i = 0; i < length; ) {
          action = outline[i++];
 
          switch(action) {
            case 'm':
              this.moveTo(outline[i++], outline[i++]);
              break;
            case 'l':
              this.lineTo(outline[i++], outline[i++]);
              break;
            case 'q':
              cpx = outline[i++];
              cpy = outline[i++];
              this.quadraticCurveTo(outline[i++], outline[i++], cpx, cpy);
              break;
          }
        }
      }
      if (glyph.ha) this.translate(glyph.ha, 0);
    };
  }
  
  proto.getTextExtents = function(text, style){
    var width = 0, height = 0, ha = 0, 
        face = ctxt.getFaceFromStyle(style),
        i, length = text.length, glyph;
    
    for (i = 0; i < length; i++) {
      glyph = face.glyphs[text.charAt(i)] || face.glyphs[ctxt.options.fallbackCharacter];
      width += Math.max(glyph.ha, glyph.x_max);
      ha += glyph.ha;
    }
    
    return {
      width: width,
      height: face.lineHeight,
      ha: ha
    };
  };
  
  proto.getComputedStyle = function(style){
    var p, canvasStyle = getElementStyle(this.canvas), 
        computedStyle = {},
        s = style.size,
        canvasFontSize = parseFloat(canvasStyle.fontSize),
        fontSize = parseFloat(s);
    
    for (p in style) {
      computedStyle[p] = style[p];
    }
    
    // Compute the size
    if (typeof s === 'number' || s.indexOf('px') != -1) 
      computedStyle.size = fontSize;
    else if (s.indexOf('em') != -1)
      computedStyle.size = canvasFontSize * fontSize;
    else if (s.indexOf('%') != -1)
      computedStyle.size = (canvasFontSize / 100) * fontSize;
    else if (s.indexOf('pt') != -1)
      computedStyle.size = fontSize / 0.75;
    else
      computedStyle.size = canvasFontSize;
    
    return computedStyle;
  };
  
  proto.getTextOffset = function(text, style, face){
    var canvasStyle = getElementStyle(this.canvas),
        metrics = this.measureText(text), 
        scale = (style.size / face.resolution) * 0.75,
        offset = {x: 0, y: 0, metrics: metrics, scale: scale};

    switch (this.textAlign) {
      default:
      case null:
      case 'left': break;
      case 'center': offset.x = -metrics.width/2; break;
      case 'right':  offset.x = -metrics.width; break;
      case 'start':  offset.x = (canvasStyle.direction == 'rtl') ? -metrics.width : 0; break;
      case 'end':    offset.x = (canvasStyle.direction == 'ltr') ? -metrics.width : 0; break;
    }
    
    switch (this.textBaseline) {
      case 'alphabetic': break;
      default:
      case null:
      case 'ideographic':
      case 'bottom': offset.y = face.descender; break;
      case 'hanging': 
      case 'top': offset.y = face.ascender; break;
      case 'middle': offset.y = (face.ascender + face.descender) / 2; break;
    }
    offset.y *= scale;
    return offset;
  };

  proto.drawText = function(text, x, y, maxWidth, stroke){
    var style = this.parseStyle(this.font),
        face = ctxt.getFaceFromStyle(style),
        offset = this.getTextOffset(text, style, face);
        
    this.save();
    this.translate(x + offset.x, y + offset.y);
    if (face.strokeFont && !stroke) {
      this.strokeStyle = this.fillStyle;
    }
    this.beginPath();

    if (moz) {
      this.mozTextStyle = this.buildStyle(style);
      this[stroke ? 'mozPathText' : 'mozDrawText'](text);
    }
    else {
      this.scale(ctxt.scaling, ctxt.scaling);
      this.renderText(text, style);
      if (face.strokeFont) {
        this.lineWidth = style.size * (style.weight == 'bold' ? 0.5 : 0.3);
      }
    }

    this[(stroke || (face.strokeFont && !moz)) ? 'stroke' : 'fill']();

    this.closePath();
    this.restore();
    
    if (ctxt.options.debug) {
      var left = Math.floor(offset.x + x) + 0.5,
          top = Math.floor(y)+0.5;
          
      this.save();
      this.strokeStyle = '#F00';
      this.lineWidth = 0.5;
      this.beginPath();
      
      // Text baseline
      this.moveTo(left + offset.metrics.width, top);
      this.lineTo(left, top);
      
      // Text align
      this.moveTo(left - offset.x, top + offset.y);
      this.lineTo(left - offset.x, top + offset.y - style.size);
      
      this.stroke();
      this.closePath();
      this.restore();
    }
  };
  
  proto.fillText = function(text, x, y, maxWidth){
    this.drawText(text, x, y, maxWidth, false);
  };
  
  proto.strokeText = function(text, x, y, maxWidth){
    this.drawText(text, x, y, maxWidth, true);
  };
  
  proto.measureText = function(text){
    var style = this.parseStyle(this.font), 
        dim = {width: 0};
    
    if (moz) {
      this.mozTextStyle = this.buildStyle(style);
      dim.width = this.mozMeasureText(text);
    }
    else {
      var face = ctxt.getFaceFromStyle(style),
          scale = (style.size / face.resolution) * 0.75;
          
      dim.width = this.getTextExtents(text, style).ha * scale * ctxt.scaling;
    }
    
    return dim;
  };
})();