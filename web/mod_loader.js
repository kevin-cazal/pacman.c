/* Pacman WASM mod lab — click mod panel OR game canvas to choose keyboard focus. */
(function() {
    "use strict";

    var DEFAULT_MOD = [
        "-- Default mod: empty hooks (vanilla C behavior)",
        "return {",
        '    name = "default",',
        "    pacman = {},",
        "    ghosts = {",
        "        blinky = {},",
        "        pinky = {},",
        "        inky = {},",
        "        clyde = {},",
        "    },",
        "}"
    ].join("\n");

    var focusTarget = null; /* "mod" | "game" */

    function el(id) { return document.getElementById(id); }

    function editor() {
        return window.pacmanModEditor || null;
    }

    function modSource() {
        var ed = editor();
        return ed ? ed.getValue() : "";
    }

    function hasModSource() {
        return modSource().trim().length > 0;
    }

    function showError(msg) {
        var node = el("mod-error");
        if (!node) return;
        if (msg) {
            node.textContent = msg;
            node.style.display = "block";
        } else {
            node.textContent = "";
            node.style.display = "none";
        }
    }

    function wasmReady() {
        return typeof Module !== "undefined" &&
            typeof Module._wasm_lua_mod_load === "function" &&
            typeof Module.cwrap === "function";
    }

    function bindModApi() {
        if (!wasmReady()) {
            return false;
        }
        if (!Module.loadMod) {
            Module.loadMod = Module.cwrap("wasm_lua_mod_load", "number", ["string"]);
        }
        if (!Module.setGameInput && typeof Module._wasm_set_game_input === "function") {
            Module.setGameInput = Module.cwrap("wasm_set_game_input", null, ["number"]);
        }
        return true;
    }

    function blurEditor() {
        var ed = editor();
        if (!ed) return;
        var node = ed.getContainerDomNode();
        if (node && node.blur) {
            node.blur();
        }
        if (document.activeElement && document.activeElement.blur) {
            document.activeElement.blur();
        }
    }

    function applyFocus(target) {
        focusTarget = target;
        var ed = editor();
        var canvas = el("canvas");
        document.body.classList.toggle("focus-mod", target === "mod");
        document.body.classList.toggle("focus-game", target === "game");

        if (target === "mod") {
            if (bindModApi() && Module.setGameInput) {
                Module.setGameInput(0);
            }
            if (ed) {
                ed.focus();
            }
        } else {
            blurEditor();
            if (bindModApi() && Module.setGameInput) {
                Module.setGameInput(1);
            }
            if (canvas) {
                canvas.focus();
            }
        }
    }

    function focusMod(ev) {
        if (ev) {
            ev.stopPropagation();
        }
        applyFocus("mod");
    }

    function focusGame(ev) {
        if (ev) {
            ev.stopPropagation();
        }
        applyFocus("game");
    }

    function syncLoadButton() {
        if (el("btn-load")) {
            el("btn-load").disabled = !hasModSource();
        }
    }

    function onLoadModClick(ev) {
        ev.stopPropagation();
        focusMod();
        var src = modSource();
        if (!src.trim()) {
            showError("Paste or edit Lua mod source first.");
            return;
        }
        if (!bindModApi()) {
            showError("WASM not ready — wait for load, then try again.");
            return;
        }
        showError("");
        if (!Module.loadMod(src)) {
            showError("Lua mod failed to load — see console.");
            return;
        }
        focusGame();
        syncLoadButton();
    }

    function wireShell() {
        var panel = el("mod-panel");
        var gamePane = el("game-pane");
        var canvas = el("canvas");

        if (panel) {
            panel.addEventListener("mousedown", focusMod);
        }
        if (gamePane) {
            gamePane.addEventListener("mousedown", focusGame);
        } else if (canvas) {
            canvas.addEventListener("mousedown", focusGame);
        }
        if (el("btn-load")) {
            el("btn-load").addEventListener("mousedown", function(ev) { ev.stopPropagation(); });
            el("btn-load").addEventListener("click", onLoadModClick);
        }
    }

    function wireEditor() {
        var ed = editor();
        if (!ed) {
            showError("Monaco editor failed to load — rebuild WASM (./build.sh wasm).");
            return;
        }

        if (!ed.getValue()) {
            ed.setValue(DEFAULT_MOD);
        }

        ed.onDidChangeModelContent(function() {
            showError("");
            syncLoadButton();
        });

        syncLoadButton();
        applyFocus("game");
    }

    function waitForWasm() {
        var tries = 0;
        var timer = setInterval(function() {
            tries++;
            if (bindModApi()) {
                clearInterval(timer);
                applyFocus(focusTarget || "game");
                syncLoadButton();
                return;
            }
            if (tries > 200) {
                clearInterval(timer);
                showError("WASM mod API timeout — rebuild pacman.html");
            }
        }, 50);
    }

    function boot() {
        wireShell();
        waitForWasm();
    }

    function onEditorReady() {
        wireEditor();
    }

    window.addEventListener("pacman-mod-editor-ready", onEditorReady);
    window.addEventListener("pacman-mod-editor-error", function(ev) {
        showError(ev.detail || "Monaco editor failed to load.");
    });

    if (window.pacmanModEditor) {
        onEditorReady();
    }

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", boot);
    } else {
        boot();
    }
})();
