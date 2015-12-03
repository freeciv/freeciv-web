/**************************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************/

/**************************************************************************
  Create a new BitVector from an array containing its bytes.
**************************************************************************/
function BitVector(raw)
{
  this.raw = raw;

  /************************************************************************
    Returns true iff the specified bit is set to true.
  ************************************************************************/
  this.isSet = function(bitNumber) {
    return (this.raw[Math.floor(bitNumber / 8)]
            & (1 << (bitNumber % 8))) != 0;
  };

  /************************************************************************
    Set the specified bit to true.
  ************************************************************************/
  this.set = function(bitNumber) {
    var byteNumber = Math.floor(bitNumber / 8);

    this.raw[byteNumber] = (this.raw[byteNumber]
                            | (1 << (bitNumber % 8)));
  };

  /************************************************************************
    Set the specified bit to false.
  ************************************************************************/
  this.unset = function(bitNumber, value) {
    var byteNumber = Math.floor(bitNumber / 8);

    this.raw[byteNumber] = (this.raw[byteNumber]
                            & ~(1 << (bitNumber % 8)));
  };

  /************************************************************************
    Show 1 or zero for each bit.

    Note that the last 7 bits may be unused.
  ************************************************************************/
  this.toString = function() {
    var out = "";

    for (var i = 0; i < this.raw.length * 8; i++) {
      out += this.isSet(i) ? "1" : 0;
    }

    return out;
  };
}
