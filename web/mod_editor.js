/* Monaco editor for pacman WASM mod lab (self-hosted under monaco/vs). */
(function() {
    "use strict";

    var VS = "monaco/vs";

    window.MonacoEnvironment = {
        getWorkerUrl: function(workerId, label) {
            if (label === "json") {
                return VS + "/language/json/json.worker.js";
            }
            if (label === "css" || label === "scss" || label === "less") {
                return VS + "/language/css/css.worker.js";
            }
            if (label === "html" || label === "handlebars" || label === "razor") {
                return VS + "/language/html/html.worker.js";
            }
            if (label === "typescript" || label === "javascript" || label === "typescriptreact" || label === "javascriptreact") {
                return VS + "/language/typescript/ts.worker.js";
            }
            return VS + "/editor/editor.worker.js";
        }
    };

    function fail(msg) {
        console.error(msg);
        window.dispatchEvent(new CustomEvent("pacman-mod-editor-error", { detail: msg }));
    }

    function init() {
        var host = document.getElementById("mod-editor");
        if (!host) {
            fail("mod-editor container not found");
            return;
        }
        if (typeof require === "undefined") {
            fail("Monaco loader.js not loaded");
            return;
        }

        require.config({ paths: { vs: VS } });
        require(["vs/editor/editor.main"], function() {
            var editor = monaco.editor.create(host, {
                language: "lua",
                theme: "vs-dark",
                automaticLayout: true,
                minimap: { enabled: false },
                fontSize: 12,
                lineNumbers: "on",
                wordWrap: "on",
                scrollBeyondLastLine: false,
                tabSize: 4,
                insertSpaces: true,
                renderWhitespace: "selection",
                folding: true
            });
            window.pacmanModEditor = editor;
            window.dispatchEvent(new Event("pacman-mod-editor-ready"));
        }, function(err) {
            fail("Monaco failed to load: " + err);
        });
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
