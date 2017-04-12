require.config({
    // app entry point
    //deps: ["main"],
    paths: {
        StackTrace: './dependencies/stacktrace',
        SyntaxHighlighterGLSL: './dependencies/syntaxhighlighter_3.0.83/shBrushGLSL',
        SyntaxHighlighterCore: './dependencies/syntaxhighlighter_3.0.83/shCore',
    },
    shim: {
        StackTrace: {
            "exports": 'printStackTrace',
        },
        SyntaxHighlighterCore: {
            "exports": 'SyntaxHighlighter',
        },
        SyntaxHighlighterGLSL: {
            "deps": ["SyntaxHighlighterCore"],
        },
    }
});

define([
    './host/CaptureContext',
    './host/HostUI',
], function(
    captureContext,
    HostUI) {

    window.gli = window.gli || {};
    window.gli.host = window.gli.host || {};
    window.gli.host.inspectContext = captureContext.inspectContext.bind(captureContext);
    window.gli.host.HostUI = HostUI;
});
