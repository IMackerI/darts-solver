/**
 * @file wasm.js
 * @brief WASM interface — thin wrapper around the Emscripten bindings.
 *
 * Keeps WASM objects alive across calls and exposes promise-based helpers.
 * DartsModule is loaded via the non-module script tag in index.html,
 * so it lives on window.DartsModule.
 */

let module = null;

// Cached WASM objects (recreated when distribution/target change)
let _target = null;
let _dist = null;
let _game = null;
let _solver = null;
let _heatVis = null;

let _targetContent = null;   // raw text, used to detect changes
let _cachedCov = null;       // serialised cov, detect changes
let _cachedMode = null;
let _cachedSolverType = null;
let _cachedSamples = null;

/* ---- init ---- */

/**
 * Initialise the Emscripten WASM module.  Must be awaited before any other export.
 * @returns {Promise<object>} The compiled module instance.
 */
export async function init() {
    // DartsModule is the factory placed on window by darts_wasm.js
    module = await DartsModule();
    return module;
}

/** Returns true if the WASM module has been initialised. @returns {boolean} */
export function isReady() { return module !== null; }

/* ---- target ---- */

/**
 * Fetch and load a target layout file into the WASM module.
 * Re-creates all cached WASM objects when the file content changes.
 * @param {string} [url='target.out'] URL of the target file to fetch.
 * @returns {Promise<void>}
 */
export async function loadTarget(url = 'target.out') {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`Failed to load target: ${res.statusText}`);
    const text = await res.text();
    if (text === _targetContent) return;
    _targetContent = text;
    _cleanup();
    _target = new module.Target(text);
}

/** Returns the raw text content of the last successfully loaded target file.
 * @returns {string|null}
 */
export function getTargetContent() { return _targetContent; }

/* ---- build / rebuild game + solver ---- */

/** Delete all cached WASM heap objects and reset change-detection keys. */
function _cleanup() {
    _heatVis?.delete();  _heatVis = null;
    _solver?.delete();   _solver = null;
    _game?.delete();     _game = null;
    _dist?.delete();     _dist = null;
    _target?.delete();   _target = null;
    _cachedCov = null;
    _cachedMode = null;
    _cachedSolverType = null;
    _cachedSamples = null;
}

/**
 * Ensure game + solver objects match the given params; rebuild if any changed.
 * @param {number[]} covFlat  [a,b,c,d]
 * @param {string}   gameMode 'finishOnDouble' | 'finishOnAny'
 * @param {string}   solverType 'minThrows' | 'maxPoints'
 * @param {number}   samples
 */
function _ensureObjects(covFlat, gameMode, solverType, samples) {
    const covKey = covFlat.join(',');
    const needDist = covKey !== _cachedCov;
    const needGame = needDist || gameMode !== _cachedMode;
    const needSolver = needGame || solverType !== _cachedSolverType || samples !== _cachedSamples;

    if (needDist) {
        _dist?.delete();
        _dist = new module.NormalDistributionQuadrature(covFlat, { x: 0, y: 0 });
        _cachedCov = covKey;
    }
    if (needGame) {
        _heatVis?.delete(); _heatVis = null;
        _solver?.delete();  _solver = null;
        _game?.delete();
        const GameClass = gameMode === 'finishOnAny'
            ? module.GameFinishOnAny
            : module.GameFinishOnDouble;
        _game = new GameClass(_target, _dist);
        _cachedMode = gameMode;
    }
    if (needSolver) {
        _heatVis?.delete(); _heatVis = null;
        _solver?.delete();
        const SolverClass = solverType === 'maxPoints'
            ? module.MaxPointsSolver
            : module.SolverMinThrows;
        _solver = new SolverClass(_game, samples);
        _cachedSolverType = solverType;
        _cachedSamples = samples;
    }
}

/* ---- public API ---- */

/**
 * Solve a single state. Returns { expectedValue, optimalAim: {x,y} }.
 */
export function solve(stateVal, covFlat, gameMode, solverType, samples) {
    _ensureObjects(covFlat, gameMode, solverType, samples);
    const res = module.solverSolve(_solver, stateVal);
    return {
        expectedValue: res.expected_throws,
        optimalAim: { x: res.optimal_aim.x, y: res.optimal_aim.y },
    };
}

/**
 * Generate heatmap grid. Returns { grid: Float64Array-like, rows, cols, bounds }.
 */
export function heatmap(stateVal, covFlat, gameMode, solverType, samples, resolution) {
    _ensureObjects(covFlat, gameMode, solverType, samples);
    if (!_heatVis || _heatVis._res !== resolution) {
        _heatVis?.delete();
        _heatVis = new module.HeatMapVisualizer(_solver, resolution, resolution);
        _heatVis._res = resolution;
    }
    const hm = _heatVis.heat_map(stateVal);
    const rows = hm.size();
    const cols = rows > 0 ? hm.get(0).size() : 0;

    // Copy into plain JS 2D array
    const grid = [];
    for (let r = 0; r < rows; r++) {
        const row = hm.get(r);
        const arr = new Array(cols);
        for (let c = 0; c < cols; c++) arr[c] = row.get(c);
        grid.push(arr);
    }

    const bounds = _game.get_target_bounds();
    return {
        grid,
        rows,
        cols,
        bounds: {
            min: { x: bounds.min.x, y: bounds.min.y },
            max: { x: bounds.max.x, y: bounds.max.y },
        },
    };
}

/**
 * Get target bounding box.
 */
export function getTargetBounds() {
    const b = _game.get_target_bounds();
    return { min: { x: b.min.x, y: b.min.y }, max: { x: b.max.x, y: b.max.y } };
}
