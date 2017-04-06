/**
 * SyntaxHighlighter
 * http://alexgorbatchev.com/SyntaxHighlighter
 *
 * SyntaxHighlighter is donationware. If you are using it, please donate.
 * http://alexgorbatchev.com/SyntaxHighlighter/donate.html
 *
 * @version
 * 3.0.83 (July 02 2010)
 * 
 * @copyright
 * Copyright (C) 2004-2010 Alex Gorbatchev.
 *
 * @license
 * Dual licensed under the MIT and GPL licenses.
 */
;(function()
{
	// CommonJS
	if (typeof(SyntaxHighlighter) == 'undefined' && typeof(require) != 'undefined') {
		SyntaxHighlighter = require('shCore').SyntaxHighlighter;
	}

	function Brush()
	{
		// Copyright 2006 Shin, YoungJin
	
		var datatypes =	'void float int bool vec2 vec3 vec4 bvec2 bvec3 bvec4 ivec2 ivec3 ivec4 mat2 mat3 mat4 sampler2D samplerCube ';

		var keywords =	'precision highp mediump lowp ' +
                        'in out inout ' +
                        'attribute const break continue do else for if discard return uniform varying struct void while ' +
                        'gl_Position gl_PointSize ' +
                        'gl_FragCoord gl_FrontFacing gl_FragColor gl_FragData gl_PointCoord ' +
                        'gl_DepthRange ' +
                        'gl_MaxVertexAttribs gl_MaxVertexUniformVectors gl_MaxVaryingVectors gl_MaxVertexTextureImageUnits gl_MaxCombinedTextureImageUnits gl_MaxTextureImageUnits gl_MaxFragmentUniformVectors gl_MaxDrawBuffers ';
					
		var functions =	'radians degrees sin cos tan asin acos atan ' +
                        'pow exp log exp2 log2 sqrt inversesqrt ' +
                        'abs sign floor ceil fract mod min max clamp mix step smoothstep ' +
                        'length distance dot cross normalize faceforward reflect refract ' +
                        'matrixCompMult ' +
                        'lessThan lessThanEqual greaterThan greaterThanEqual equal notEqual any all not ' +
                        'texture2D texture2DProj texture2DLod texture2DProjLod textureCube textureCubeLod ';

		this.regexList = [
			{ regex: SyntaxHighlighter.regexLib.singleLineCComments,	css: 'comments' },			// one line comments
			{ regex: SyntaxHighlighter.regexLib.multiLineCComments,		css: 'comments' },			// multiline comments
			{ regex: SyntaxHighlighter.regexLib.doubleQuotedString,		css: 'string' },			// strings
			{ regex: SyntaxHighlighter.regexLib.singleQuotedString,		css: 'string' },			// strings
			{ regex: /^ *#.*/gm,										css: 'preprocessor' },
			{ regex: new RegExp(this.getKeywords(datatypes), 'gm'),		css: 'color1 bold' },
			{ regex: new RegExp(this.getKeywords(functions), 'gm'),		css: 'functions bold' },
			{ regex: new RegExp(this.getKeywords(keywords), 'gm'),		css: 'keyword bold' }
			];
	};

	Brush.prototype	= new SyntaxHighlighter.Highlighter();
	Brush.aliases	= ['glsl', 'vs', 'fs'];

	SyntaxHighlighter.brushes.GLSL = Brush;

	// CommonJS
	typeof(exports) != 'undefined' ? exports.Brush = Brush : null;
})();
