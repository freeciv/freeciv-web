module.exports = {
    context: __dirname,
    entry: "./gli.js",
    output: {
        filename: "gli.all.js",
        path: __dirname + "/lib",
    },
    module: {
        loaders: [
            {
                test: /\.js$/,
                loader: 'babel-loader',
                query: {
                    presets: ['stage-0', 'es2015'],
                },
            },
            {
                test: /dependencies\/stacktrace\.js/,
                loader: 'exports?printStackTrace',
            },
            {
                test: /dependencies\/syntaxhighlighter_3.0.83\/shBrushGLSL\.js/,
                loader: 'imports?SyntaxHighlighter',
            },
            {
                test: /dependencies\/syntaxhighlighter_3.0.83\/shCore\.js/,
                loader: 'exports?SyntaxHighlighter',
            },
        ],
        noParse: [
            // /dependencies\/syntaxhighlighter_3.0.83\/shBrushGLSL\.js/,
            /dependencies\/syntaxhighlighter_3.0.83\/shCore\.js/,
        ]
    },
    resolve: {
        alias: {
            StackTrace: __dirname + '/dependencies/stacktrace.js',
            SyntaxHighlighterGLSL: __dirname + '/dependencies/syntaxhighlighter_3.0.83/shBrushGLSL.js',
            SyntaxHighlighter: __dirname + '/dependencies/syntaxhighlighter_3.0.83/shCore.js',
            shCore: __dirname + '/dependencies/syntaxhighlighter_3.0.83/shCore.js',
        },
    },
};

