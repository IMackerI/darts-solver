/**
 * @file wasm.js
 * @brief WASM interface — thin wrapper around the Emscripten bindings.
 *
 * Keeps WASM objects alive across calls and exposes promise-based helpers.
 * DartsModule is loaded via the non-module script tag in index.html,
 * so it lives on window.DartsModule.
 */

let module = null;
let worker = null;
let workerReadyPromise = null;
let requestSeq = 1;
const pendingRequests = new Map();

let workerPreferred = true;
let workerEnabled = false;
let workerHasTarget = false;

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

/** Enable or disable worker-mode preference. Best used before init(). */
export function setWorkerMode(enabled) {
    workerPreferred = !!enabled;
    if (!workerPreferred) {
        workerEnabled = false;
        _terminateWorker();
    }
}

/** Returns true when requests are currently executed inside the worker. */
export function isWorkerMode() {
    return workerEnabled;
}

function _supportsWorker() {
    return typeof Worker !== 'undefined';
}

function _terminateWorker() {
    if (worker) {
        worker.terminate();
        worker = null;
    }
    if (pendingRequests.size > 0) {
        const err = new Error('Worker terminated');
        for (const { reject } of pendingRequests.values()) reject(err);
        pendingRequests.clear();
    }
    workerReadyPromise = null;
    workerHasTarget = false;
}

function _onWorkerMessage(event) {
    const msg = event.data || {};
    if (msg.type === 'ready') return;

    const ticket = pendingRequests.get(msg.id);
    if (!ticket) return;
    pendingRequests.delete(msg.id);

    if (msg.error) {
        const err = new Error(msg.error.message || 'Worker request failed');
        err.stack = msg.error.stack || err.stack;
        ticket.reject(err);
        return;
    }

    ticket.resolve(msg.result);
}

function _onWorkerError(event) {
    const err = event?.error || new Error(event?.message || 'Worker crashed');
    workerEnabled = false;
    _terminateWorker();
    console.error(err);
}

async function _ensureWorker() {
    if (workerReadyPromise) {
        await workerReadyPromise;
        return;
    }

    workerReadyPromise = new Promise((resolve, reject) => {
        try {
            worker = new Worker(new URL('../worker.js', import.meta.url));
        } catch (err) {
            reject(err);
            return;
        }

        const cleanupReadyHandlers = () => {
            worker?.removeEventListener('message', onReadyMessage);
            worker?.removeEventListener('error', onReadyError);
        };

        const onReadyMessage = (event) => {
            const msg = event.data || {};
            if (msg.type === 'boot-error') {
                cleanupReadyHandlers();
                _terminateWorker();
                reject(new Error(msg.error?.message || 'Worker boot failed'));
                return;
            }
            if (msg.type !== 'ready') return;
            cleanupReadyHandlers();
            worker.addEventListener('message', _onWorkerMessage);
            worker.addEventListener('error', _onWorkerError);
            workerEnabled = true;
            resolve();
        };

        const onReadyError = (event) => {
            cleanupReadyHandlers();
            _terminateWorker();
            reject(event?.error || new Error(event?.message || 'Worker failed to initialize'));
        };

        worker.addEventListener('message', onReadyMessage);
        worker.addEventListener('error', onReadyError);
    });

    try {
        await workerReadyPromise;
    } catch (err) {
        workerReadyPromise = null;
        throw err;
    }
}

async function _workerRequest(type, payload) {
    await _ensureWorker();
    const id = requestSeq++;
    return new Promise((resolve, reject) => {
        pendingRequests.set(id, { resolve, reject });
        worker.postMessage({ id, type, payload });
    });
}

/* ---- init ---- */

/**
 * Initialise the Emscripten WASM module.  Must be awaited before any other export.
 * @returns {Promise<object>} The compiled module instance.
 */
export async function init() {
    if (workerPreferred && _supportsWorker()) {
        try {
            await _ensureWorker();
            return null;
        } catch (err) {
            console.warn('Worker init failed; falling back to main thread WASM.', err);
            workerEnabled = false;
        }
    }

    // DartsModule is the factory placed on window by darts_wasm.js.
    module = await DartsModule();
    return module;
}

export function isReady() { return module !== null || workerEnabled; }

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
    const changed = text !== _targetContent;
    _targetContent = text;

    if (workerEnabled) {
        if (!changed && workerHasTarget) return;
        await _workerRequest('loadTarget', { targetContent: text });
        workerHasTarget = true;
        return;
    }

    if (!changed && _target) return;

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
            : solverType === 'minRounds'
                ? module.SolverMinRounds
                : module.SolverMinThrows;
        if (solverType === 'minRounds') {
            _solver = new SolverClass(_game, 3, samples);
        } else {
            _solver = new SolverClass(_game, samples);
        }
        _cachedSolverType = solverType;
        _cachedSamples = samples;
    }
}

/* ---- public API ---- */

/**
 * Solve a single points-remaining value. Returns { expectedValue, optimalAim: {x,y} }.
 */
export async function solve(pointsRemaining, covFlat, gameMode, solverType, samples) {
    return _solveAsync(pointsRemaining, covFlat, gameMode, solverType, samples);
}

async function _solveAsync(pointsRemaining, covFlat, gameMode, solverType, samples) {
    if (workerEnabled) {
        return _workerRequest('solve', {
            pointsRemaining,
            covFlat,
            gameMode,
            solverType,
            samples,
        });
    }

    _ensureObjects(covFlat, gameMode, solverType, samples);
    const res = module.solverSolve(_solver, pointsRemaining);
    return {
        expectedValue: res.expected_throws,
        optimalAim: { x: res.optimal_aim.x, y: res.optimal_aim.y },
    };
}

/**
 * Generate heatmap grid. Returns { grid: Float64Array-like, rows, cols, bounds }.
 */
export async function heatmap(pointsRemaining, covFlat, gameMode, solverType, samples, resolution) {
    return _heatmapAsync(pointsRemaining, covFlat, gameMode, solverType, samples, resolution);
}

async function _heatmapAsync(pointsRemaining, covFlat, gameMode, solverType, samples, resolution) {
    if (workerEnabled) {
        return _workerRequest('heatmap', {
            pointsRemaining,
            covFlat,
            gameMode,
            solverType,
            samples,
            resolution,
        });
    }

    _ensureObjects(covFlat, gameMode, solverType, samples);
    if (!_heatVis || _heatVis._res !== resolution) {
        _heatVis?.delete();
        _heatVis = new module.HeatMapVisualizer(_solver, resolution, resolution);
        _heatVis._res = resolution;
    }
    const hm = _heatVis.heat_map(pointsRemaining);
    const rows = hm.size();
    const cols = rows > 0 ? hm.get(0).size() : 0;

    // Copy into plain JS 2D array.
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
    if (workerEnabled) {
        throw new Error('getTargetBounds is unavailable in worker mode');
    }
    const b = _game.get_target_bounds();
    return { min: { x: b.min.x, y: b.min.y }, max: { x: b.max.x, y: b.max.y } };
}

window.addEventListener('beforeunload', () => {
    _terminateWorker();
});
