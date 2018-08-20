/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


/**
 * BMP Library for JavaScript
 *
 * Copyright 2008 Neil Fraser.
 * http://neil.fraser.name/software/bmp_lib/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Root object for BMP Library.
var bmp_lib = {};


/**
 * Replace the contents of an image or a table with a picture.
 * @param {string|Element} table The ID of an image or a table,
 *   or the actual image or table element.
 * @param {Array} grid The image data.
 * @param {Array} opt_palette Optional palette data.
 * @throws {string} If the element is invalid.
 */
bmp_lib.render = function(element, grid, opt_palette) {
  if (typeof element == 'string') {
    element = document.getElementById(element);
  }
  if (!element || !element.tagName) {
    throw('bmp_lib.render: Invalid element: ' + element);
  } else if (element.tagName == 'IMG') {
    element.src = this.imageSource(grid, opt_palette);
  } else if (element.tagName == 'TABLE') {
    var data = this.tableBody(grid, opt_palette);
    // IE throws "Unknown runtime error" if one sets table.innerHTML.
    // Using insertRow/insertCell works, but takes 10 times longer.
    // Instead, build a new table, then swap out the old one.
    if ('outerHTML' in element) {
      // IE, Safari, Opera.
      // Clear existing table.
      var rowCount = element.rows.length;
      for (var y = rowCount - 1; y >= 0; y--) {
        element.deleteRow(y);
      }
      // Fetch the opening table tag.
      var tableTag = element.outerHTML;
      tableTag = tableTag.substring(0, tableTag.indexOf('>') + 1);
      var tempDiv = document.createElement('DIV');
      tempDiv.innerHTML = tableTag + data + '</table>';
      element.parentElement.replaceChild(tempDiv.firstChild, element);
    } else {
      // Firefox.
      element.innerHTML = data;
    }
  } else {
    throw('bmp_lib.render: Invalid HTML tag: ' + element.tagName);
  }
};


/**
 * Create a BMP image and encode it so that it may be set directly to the
 * 'src' attribute of an HTML image tag.
 * @param {Array} grid The image data.
 * @param {Array} opt_palette Optional palette data.
 * @return {string} Base64-encoded image data with header.
 */
bmp_lib.imageSource = function(grid, opt_palette) {
  var a = this.normalize_(grid, opt_palette)
  var data = this.createBmp_(a[0], a[1]);
  return 'data:image/bmp;base64,' + this.encode64_(data);
};


/**
 * Create table contents with each cell depicting one pixel.
 * May be written directly between the <TABLE> and </TABLE> tags.
 * @param {Array} grid The image data.
 * @param {Array} opt_palette Optional palette data.
 * @return {string} HTML soup depicting the image as a table.
 */
bmp_lib.tableBody = function(grid, opt_palette) {
  // Note: run-length encoding was attempted but resulted in a slight slowdown.
  var a = this.normalize_(grid, opt_palette);
  grid = a[0];
  var palette = a[1];
  var height = grid.length;
  var width = height && grid[0].length;
  var rgb;
  var table = [];
  for (var y = 0; y < height; y++) {
    row = [];
    for (var x = 0; x < width; x++) {
      if (palette) {
        rgb = palette[grid[y].charCodeAt(x)];
      } else {
        rgb = grid[y][x];
      }
      var colour = this.dec2hex_(rgb[0]) + this.dec2hex_(rgb[1]) +
          this.dec2hex_(rgb[2]);
      row[x] = '<TD BGCOLOR=#' + colour + '><img width=1 height=1></TD>';
    }
    row.unshift('<TR>');
    row.push('</TR>');
    table[y] = row.join('');
  }
  return table.join('\n');
};


/**
 * Verifies that the grid and palette are one of the three known types.  If
 * the grid is a 2D array of numbers, convert it to an array of binary strings.
 * @param {Array} grid The image data.
 * @param {Array} opt_palette Optional palette data.
 * @return {Array.<Array>} A tuple containing the a normalized grid and palette.
 * @private
 */
bmp_lib.normalize_ = function(grid, opt_palette) {
  var palette;
  // Check what type of data was provided.
  if (grid.length == 0) {
    // 0x0 picture.
    palette = null;
  } else if (typeof grid[0] == 'string' && opt_palette) {
    // Array of strings, with palette.
    palette = opt_palette;
  } else if (typeof grid[0] == 'object' && typeof grid[0][0] == 'number' &&
      opt_palette) {
    // 2D array of numbers, with palette.  Convert to array of strings.
    grid = this.arrayArrayToArrayStr_(grid);
    palette = opt_palette;
  } else if (typeof grid[0] == 'object' && typeof grid[0][0] == 'object' &&
      grid[0][0].length >= 3) {
    // 2D array of [r, g, b] tuples, without palette.  True-colour mode.
    palette = null;
  } else {
    // WTF?
    throw('Invalid argument types.');
  }
  return [grid, palette];
};


/**
 * Assemble a BMP based on the image data and an optional palette.
 * If a palette is provided and contains 256 or fewer colours, the BMP is
 * in 8-bit paletted mode, otherwise it is in 24-bit true-colour mode.
 * @param {Array} grid The image data.
 * @param {Array?} palette Optional palette data.
 * @return {string} BMP as binary string.
 * @private
 */
bmp_lib.createBmp_ = function(grid, palette) {
  // xxxx and yyyy are placeholders for offsets (computed later).
  var bitmapFileHeader = 'BMxxxx\0\0\0\0yyyy';

  // Assemble the info header.
  var height = grid.length;
  var width = height && grid[0].length;
  var biHeight = this.multixbyteEncode_(height, 4);
  var biWidth = this.multixbyteEncode_(width, 4);
  var bfOffBits = this.multixbyteEncode_(40, 4);
  var bitCount;
  if (palette && palette.length <= 256) {
    bitCount = 8;
  } else {
    bitCount = 24;
    if (palette) {
      // Convert the oversized palette into inline-colours.
      grid = this.depalette_(grid, palette);
    }
    palette = null;
  }
  var biBitCount = this.multixbyteEncode_(bitCount, 2);
  var bitmapInfoHeader = bfOffBits + biWidth + biHeight + '\x01\0' +
      biBitCount + '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0';

  // Compute the palette.
  var rgbQuad;
  if (bitCount != 24) {
    var palette_str = String(palette);
    if (bmp_lib.createBmp_.palette_str_cache == palette_str) {
      // The previously computed palette was identical.  Use it.
      rgbQuad = bmp_lib.createBmp_.rgbQuad_cache;
    } else {
      rgbQuad = [];
      var r = 0;
      var g = 0;
      var b = 0;
      for (var x = 0; x < 256; x++) {
        if (x < palette.length) {
          r = palette[x][0];
          g = palette[x][1];
          b = palette[x][2];
        }
        rgbQuad[x] = String.fromCharCode(b, g, r, 0);
      }
      rgbQuad = rgbQuad.join('');
      // Cache this result in case the next call uses the same palette.
      bmp_lib.createBmp_.palette_str_cache = palette_str;
      bmp_lib.createBmp_.rgbQuad_cache = rgbQuad;
    }
  } else {
    rgbQuad = '';
  }

  var padding;
  if (width % 4 == 1) {
    padding = '\0\0\0';
  } else if (width % 4 == 2) {
    padding = '\0\0';
  } else if (width % 4 == 3) {
    padding = '\0';
  } else {
    padding = '';
  }
  if (bitCount == 24) {
    padding = padding + padding + padding;
  }

  var data = [];
  // BMPs are drawn from the bottom up.
  for (var y = 0; y < height; y++) {
    var row = grid[height - y - 1];
    if (bitCount == 8) {
      data[y] = row + padding;
    } else if (bitCount == 24) {
      for (var x = 0; x < width; x++) {
        data.push(String.fromCharCode(row[x][2], row[x][1], row[x][0]));
      }
      data.push(padding);
    }
  }
  data = data.join('');

  var bitmap = bitmapFileHeader + bitmapInfoHeader + rgbQuad + data;
  // Specify the offset from the beginning of the file to the bitmap data.
  bitmap = bitmap.replace(/yyyy/, this.multixbyteEncode_(
      bitmapFileHeader.length + bitmapInfoHeader.length + rgbQuad.length, 4));
  // Insert the size of the bitmap in xbytes.
  bitmap = bitmap.replace(/xxxx/, this.multixbyteEncode_(bitmap.length, 4));
  return bitmap;
};

// Cached palette to avoid recomputing identical palettes.
bmp_lib.createBmp_.palette_str_cache = '';
bmp_lib.createBmp_.rgbQuad_cache = '';


/**
 * Return a binary string of the specified xbyte length that encodes the
 * specified number.  LITTLE-ENDIAN!
 * @param {number} number The numeric value to be encoded.
 * @param {Array?} palette The number of xbytes to use.
 * @return {string} BMP as binary string.
 * @throws {string} If the number is too big to fit in the xbyte space.
 * @private
 */
bmp_lib.multixbyteEncode_ = function(number, xbytes) {
  if (number < 0 || xbytes < 0) {
    throw('Negative numbers not allowed.');
  }
  var oldbase = 1;
  var string = '';
  for (var x = 0; x < xbytes; x++) {
    var xbyte = 0;
    if (number != 0) {
      var base = oldbase * 256;
      xbyte = number % base;
      number = number - xbyte;
      xbyte = xbyte / oldbase;
      oldbase = base;
    }
    string += String.fromCharCode(xbyte);
  }
  if (number != 0) {
    throw('Overflow, number too big for string length');
  }
  return string;
};


/**
 * Converts a 2D array of numbers into an array of binary strings.
 * @param {Array.<Array.<number>>} grid Array of Arrays of numbers.
 * @return {Array.<string>} Array of strings.
 * @private
 */
bmp_lib.arrayArrayToArrayStr_ = function(arrayArray) {
  var arrayStr = Array(arrayArray.length);
  for (var y = 0; y < arrayArray.length; y++) {
    line = [];
    for (var x = 0; x < arrayArray[y].length; x++) {
      line[x] = String.fromCharCode(arrayArray[y][x]);
    }
    arrayStr[y] = line.join('');
  }
  return arrayStr;
};


/**
 * Convert a paletted image into a 24-bit true-colour image.
 * Used when a palette has more than 256 colours.
 * @param {Array.<string>} grid The image data.
 * @param {Array.<Array.<number>>} palette Palette data.
 * @return {Array.<Array.<Array.<number>>>} 2D array of RGB tuples.
 * @private
 */
bmp_lib.depalette_ = function(oldGrid, palette) {
  var newGrid = Array(oldGrid.length);
  for (var y = 0; y < oldGrid.length; y++) {
    newGrid[y] = [];
    for (var x = 0; x < oldGrid[y].length; x++) {
      newGrid[y][x] = palette[oldGrid[y].charCodeAt(x)];
    }
  }
  return newGrid;
};


/**
 * Converts decimal to hex.  Used for HTML colour codes.
 * @param {number} decimal 0-255.
 * @return {string} '00'-'FF'.
 * @private
 */
bmp_lib.dec2hex_ = function(decimal) {
  var a = decimal % 16;
  var b = (decimal - a) / 16;
  return bmp_lib.dec2hex_.hexChars.charAt(b) +
         bmp_lib.dec2hex_.hexChars.charAt(a);
};

bmp_lib.dec2hex_.hexChars = '0123456789ABCDEF';


// This code was written by Tyler Akins and has been placed in the
// public domain.  It would be nice if you left this header intact.
// Base64 code from Tyler Akins -- http://rumkin.com

/**
 * Encode a binary string as Base64.
 * @param {string} input Binary string.
 * @return {string} Base64-encoded string.
 * @private
 */
bmp_lib.encode64_ = function(input) {
  var output = '';
  var i = 0;

  do {
    var chr1 = input.charCodeAt(i++);
    var chr2 = input.charCodeAt(i++);
    var chr3 = input.charCodeAt(i++);

    var enc1 = chr1 >> 2;
    var enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
    var enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
    var enc4 = chr3 & 63;

    if (isNaN(chr2)) {
      enc3 = enc4 = 64;
    } else if (isNaN(chr3)) {
      enc4 = 64;
    }

    output = output + bmp_lib.encode64_.keyStr.charAt(enc1) +
        bmp_lib.encode64_.keyStr.charAt(enc2) + 
        bmp_lib.encode64_.keyStr.charAt(enc3) +
        bmp_lib.encode64_.keyStr.charAt(enc4);
  } while (i < input.length);

  return output;
};

bmp_lib.encode64_.keyStr =
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';

// Some browsers (Gecko, Webkit) have a native function for base64 encoding.
if ('btoa' in window && typeof window.btoa == 'function' &&
    window.btoa('hello') == 'aGVsbG8=') {
  // Overwrite previous function with native call.
  bmp_lib.encode64_ = function(input) {
    return window.btoa(input);
  }
}

