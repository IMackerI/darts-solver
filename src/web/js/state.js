/**
 * @file state.js
 * @brief State manager — single global state object with subscriber notifications.
 */
const state = {
    calibration: {
        shots: [],          // [{x, y}, ...]  (dartboard mm coords)
        covariance: null,   // [a, b, c, d] flat 2×2
        mean: null,         // {x, y}
    },
    solverTabs: {
        maxPoints: {
            pointsRemaining: 100000,
            gameMode: 'finishOnAny',
            aimSamples: 5000,
            showHeatmap: true,
            heatmapResolution: 100,
            optimalAim: null,
            expectedValue: null,
        },
        minThrows: {
            pointsRemaining: 501,
            gameMode: 'finishOnDouble',
            aimSamples: 5000,
            showHeatmap: true,
            heatmapResolution: 50,
            optimalAim: null,
            expectedValue: null,
        },
        minRounds: {
            pointsRemaining: 501,
            roundStartScore: 501,
            throwNumber: 1,
            precomputeAllRoundStarts: false,
            gameMode: 'finishOnDouble',
            aimSamples: 1000,
            showHeatmap: false,
            heatmapResolution: 30,
            solveUpTo: 170,
            optimalAim: null,
            expectedValue: null,
        },
    },
};

const _listeners = {};  // path → Set<callback>

/** Subscribe to a top-level key change. callback(newValue) */
export function subscribe(path, callback) {
    if (!_listeners[path]) _listeners[path] = new Set();
    _listeners[path].add(callback);
    return () => _listeners[path].delete(callback);
}

/** Notify all subscribers for the given dot-path with its current value.
 * @param {string} path
 */
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
    for (let i = 0; i < parts.length - 1; i++) {
        if (obj[parts[i]] == null || typeof obj[parts[i]] !== 'object') obj[parts[i]] = {};
        obj = obj[parts[i]];
    }
    obj[parts[parts.length - 1]] = value;
    _notify(path);
}

/* ---------- localStorage persistence ---------- */
const STORAGE_KEY = 'darts-solver-state';

/** Persist the serialisable parts of state to localStorage. */
export function saveToStorage() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify({
            calibration: state.calibration,
            solverTabs: state.solverTabs,
        }));
    } catch { /* quota exceeded — ignore */ }
}

/** Restore previously persisted state from localStorage. Silently ignores missing or corrupted data. */
export function loadFromStorage() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (!raw) return;
        const saved = JSON.parse(raw);
        if (saved.calibration) {
            state.calibration = { ...state.calibration, ...saved.calibration };
        }

        if (saved.solverTabs) {
            for (const key of Object.keys(state.solverTabs)) {
                if (saved.solverTabs[key]) {
                    state.solverTabs[key] = { ...state.solverTabs[key], ...saved.solverTabs[key] };
                }
            }
        }

        // Defensive normalization in case localStorage contained malformed tab data.
        if (!state.solverTabs || typeof state.solverTabs !== 'object') {
            state.solverTabs = {};
        }
        if (!state.solverTabs.maxPoints || typeof state.solverTabs.maxPoints !== 'object') {
            state.solverTabs.maxPoints = {
                pointsRemaining: 100000,
                gameMode: 'finishOnAny',
                aimSamples: 5000,
                showHeatmap: true,
                heatmapResolution: 100,
                optimalAim: null,
                expectedValue: null,
            };
        }
        if (!state.solverTabs.minThrows || typeof state.solverTabs.minThrows !== 'object') {
            state.solverTabs.minThrows = {
                pointsRemaining: 501,
                gameMode: 'finishOnDouble',
                aimSamples: 5000,
                showHeatmap: true,
                heatmapResolution: 50,
                optimalAim: null,
                expectedValue: null,
            };
        }
        if (!state.solverTabs.minRounds || typeof state.solverTabs.minRounds !== 'object') {
            state.solverTabs.minRounds = {
                pointsRemaining: 501,
                roundStartScore: 501,
                throwNumber: 1,
                precomputeAllRoundStarts: false,
                gameMode: 'finishOnDouble',
                aimSamples: 1000,
                showHeatmap: false,
                heatmapResolution: 30,
                solveUpTo: 170,
                optimalAim: null,
                expectedValue: null,
            };
        }
    } catch { /* corrupted — ignore */ }
}
