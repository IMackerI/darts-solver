/**
 * State manager — single global state object with subscriber notifications.
 */
const state = {
    calibration: {
        shots: [],          // [{x, y}, ...]  (dartboard mm coords)
        covariance: null,   // [a, b, c, d] flat 2×2
        mean: null,         // {x, y}
    },
    solver: {
        currentState: 501,
        gameMode: 'finishOnDouble',
        solverType: 'minThrows',
        aimSamples: 5000,
        optimalAim: null,
        expectedValue: null,
        heatmapData: null,
        showHeatmap: false,
        heatmapResolution: 50,
    },
};

const _listeners = {};  // path → Set<callback>

/** Subscribe to a top-level key change. callback(newValue) */
export function subscribe(path, callback) {
    if (!_listeners[path]) _listeners[path] = new Set();
    _listeners[path].add(callback);
    return () => _listeners[path].delete(callback);
}

function _notify(path) {
    const parts = path.split('.');
    const val = parts.reduce((o, k) => o?.[k], state);
    _listeners[path]?.forEach(cb => cb(val));
}

/** Get a nested value:  get('calibration.shots') */
export function get(path) {
    return path.split('.').reduce((o, k) => o?.[k], state);
}

/** Set a nested value and notify subscribers */
export function set(path, value) {
    const parts = path.split('.');
    let obj = state;
    for (let i = 0; i < parts.length - 1; i++) obj = obj[parts[i]];
    obj[parts[parts.length - 1]] = value;
    _notify(path);
}

/* ---------- localStorage persistence ---------- */
const STORAGE_KEY = 'darts-solver-state';

export function saveToStorage() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify({
            calibration: state.calibration,
            solver: {
                currentState: state.solver.currentState,
                gameMode: state.solver.gameMode,
                solverType: state.solver.solverType,
                aimSamples: state.solver.aimSamples,
            },
        }));
    } catch { /* quota exceeded — ignore */ }
}

export function loadFromStorage() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (!raw) return;
        const saved = JSON.parse(raw);
        if (saved.calibration) {
            state.calibration = { ...state.calibration, ...saved.calibration };
        }
        if (saved.solver) {
            Object.assign(state.solver, saved.solver);
        }
    } catch { /* corrupted — ignore */ }
}
