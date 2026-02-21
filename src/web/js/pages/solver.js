/**
 * @file solver.js
 * @brief Solver page — solve for optimal aim point and display the strategy heatmap.
 */
import { DartboardRenderer } from '../dartboard.js';
import * as State from '../state.js';
import * as Wasm  from '../wasm.js';

let renderer = null;
let beds = null;
let bounds = null;
let _mounted = false;
const _abortCtrl = { current: null };

// Cached results so we can re-render without recomputing
let cachedHeatmap = null;
let cachedHeatmapBounds = null;

/**
 * Mount the solver page: initialise the renderer, restore form state, and attach listeners.
 * @param {object[]} parsedBeds    Parsed bed array from target.js.
 * @param {{min:{x,y},max:{x,y}}} parsedBounds  Bounding box from target.js.
 */
export function mount(parsedBeds, parsedBounds) {
    beds = parsedBeds;
    bounds = parsedBounds;

    const canvas = document.getElementById('solver-canvas');
    _resizeCanvas(canvas);
    renderer = new DartboardRenderer(canvas);
    renderer.setBeds(beds, bounds);
    renderer.render();

    // Restore state into form fields
    document.getElementById('game-mode').value      = State.get('solver.gameMode');
    document.getElementById('game-state').value      = State.get('solver.currentState');
    document.getElementById('solver-type').value     = State.get('solver.solverType');
    document.getElementById('aim-samples').value     = State.get('solver.aimSamples');
    document.getElementById('show-heatmap').checked  = State.get('solver.showHeatmap');
    document.getElementById('heatmap-resolution').value = State.get('solver.heatmapResolution');
    _toggleHeatmapRes();

    if (!_mounted) {
        _mounted = true;
        const ac = new AbortController();
        _abortCtrl.current = ac;
        const sig = { signal: ac.signal };

        document.getElementById('btn-solve').addEventListener('click', (e) => { e.target.blur(); _onSolve(); }, sig);
        document.getElementById('show-heatmap').addEventListener('change', (e) => { e.target.blur(); _onHeatmapToggle(); }, sig);
        document.getElementById('heatmap-resolution').addEventListener('change', (e) => { e.target.blur(); _onResChange(); }, sig);
        document.getElementById('btn-state-prev').addEventListener('click', (e) => { e.target.blur(); _stepState(-1); }, sig);
        document.getElementById('btn-state-next').addEventListener('click', (e) => { e.target.blur(); _stepState(+1); }, sig);
        document.addEventListener('keydown', _onKeyDown, sig);

        // Heatmap value tooltip on hover
        const solverCanvas = document.getElementById('solver-canvas');
        solverCanvas.addEventListener('mousemove', _onCanvasMouseMove, sig);
        solverCanvas.addEventListener('mouseleave', _onCanvasMouseLeave, sig);

        for (const id of ['game-mode', 'game-state', 'solver-type', 'aim-samples']) {
            document.getElementById(id).addEventListener('change', (e) => { e.target.blur(); _syncFormToState(); }, sig);
        }
    }

    _updateCalibrationWarning();
}

/** Unmount the solver page, remove listeners, and clear cached heatmap data. */
export function unmount() {
    _abortCtrl.current?.abort();
    _abortCtrl.current = null;
    _mounted = false;
    cachedHeatmap = null;
    cachedHeatmapBounds = null;
}

/* --- helpers --- */

/**
 * Return the calibration covariance matrix from state.
 * @returns {number[]|null} Flat [a,b,c,d] 2×2 matrix, or null if not calibrated.
 */
function _getCovariance() {
    const cov = State.get('calibration.covariance');
    if (!cov) return null;
    return cov;
}

/** Show a warning in the subtitle if no calibration data is available. */
function _updateCalibrationWarning() {
    const cov = _getCovariance();
    const sub = document.getElementById('solver-subtitle');
    if (!cov) {
        sub.textContent = '⚠ No calibration data. Go to Calibration first, or a default will be used.';
        sub.style.color = 'var(--red)';
    } else {
        sub.textContent = 'Configure and solve for optimal dart strategy.';
        sub.style.color = '';
    }
}

/** Read all solver form fields and sync their values into the app state. */
function _syncFormToState() {
    State.set('solver.gameMode',    document.getElementById('game-mode').value);
    State.set('solver.currentState', parseInt(document.getElementById('game-state').value, 10));
    State.set('solver.solverType',  document.getElementById('solver-type').value);
    State.set('solver.aimSamples',  parseInt(document.getElementById('aim-samples').value, 10));
    State.saveToStorage();
}

/** Show or hide the heatmap resolution field based on the current toggle state. */
function _toggleHeatmapRes() {
    const show = document.getElementById('show-heatmap').checked;
    document.getElementById('heatmap-res-field').style.display = show ? '' : 'none';
}

/* --- state navigation --- */

/**
 * Increment or decrement the current game state by delta and re-solve.
 * @param {number} delta Positive or negative integer step.
 */
function _stepState(delta) {
    const input = document.getElementById('game-state');
    const cur = parseInt(input.value, 10) || 0;
    const next = Math.max(1, cur + delta);
    input.value = next;
    _syncFormToState();
    _onSolve();
}

/**
 * Handle arrow key presses to step the game state when not focused on an input.
 * @param {KeyboardEvent} e
 */
function _onKeyDown(e) {
    // Only block arrows when typing in a text input
    const active = document.activeElement;
    const isTyping = active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA');
    if (isTyping) return;

    if (e.key === 'ArrowLeft') {
        e.preventDefault();
        _stepState(-1);
    } else if (e.key === 'ArrowRight') {
        e.preventDefault();
        _stepState(+1);
    }
}

/* --- heatmap tooltip --- */

const _tooltip = () => document.getElementById('heatmap-tooltip');

function _onCanvasMouseMove(e) {
    const tip = _tooltip();
    if (!cachedHeatmap) { tip.classList.add('hidden'); return; }

    const rect = e.currentTarget.getBoundingClientRect();
    const px = e.clientX - rect.left;
    const py = e.clientY - rect.top;
    const val = renderer.heatmapValueAt(px, py);

    if (val === null) {
        tip.classList.add('hidden');
        return;
    }

    const solverType = document.getElementById('solver-type').value;
    const label = solverType === 'maxPoints'
        ? `${val.toFixed(2)} pts`
        : `${val.toFixed(2)} throws`;

    tip.textContent = label;
    tip.style.left = e.clientX + 'px';
    tip.style.top  = e.clientY + 'px';
    tip.classList.remove('hidden');
}

function _onCanvasMouseLeave() {
    _tooltip().classList.add('hidden');
}

/* --- solve --- */

/**
 * Run the solver with current form values, optionally compute a heatmap,
 * and display results on the board and in the results panel.
 */
async function _onSolve() {
    const cov = _getCovariance() || [1600, 0, 0, 1600]; // fallback default
    const gameMode   = document.getElementById('game-mode').value;
    const stateVal   = parseInt(document.getElementById('game-state').value, 10);
    const solverType = document.getElementById('solver-type').value;
    const samples    = parseInt(document.getElementById('aim-samples').value, 10);

    if (isNaN(stateVal) || stateVal < 1) {
        alert('Invalid state value');
        return;
    }
    // DP solver (minThrows) is O(state) and can hang/crash for large values
    if (solverType === 'minThrows' && stateVal > 1000) {
        alert('State values above 1000 are not supported for the DP solver (Minimize Throws).\n'
            + 'Use Max Points solver for large state values, or pick a value ≤ 1000.');
        return;
    }

    _showLoading('Solving...');

    // Run in a timeout so the loading overlay actually appears
    await _nextFrame();

    try {
        const result = Wasm.solve(stateVal, cov, gameMode, solverType, samples);

        State.set('solver.optimalAim', result.optimalAim);
        State.set('solver.expectedValue', result.expectedValue);
        _syncFormToState();

        // Compute heatmap if toggle is on
        if (document.getElementById('show-heatmap').checked) {
            await _computeHeatmap(stateVal, cov, gameMode, solverType, samples);
        } else {
            cachedHeatmap = null;
        }

        _renderBoard();
        _showResults(result, solverType);
    } catch (err) {
        console.error(err);
        alert('Solve failed: ' + err.message);
    } finally {
        _hideLoading();
    }
}

/**
 * Request a heatmap from the WASM solver and cache the result.
 * @param {number} stateVal   Current game state.
 * @param {number[]} cov      Flat 2×2 covariance.
 * @param {string} gameMode
 * @param {string} solverType
 * @param {number} samples
 */
async function _computeHeatmap(stateVal, cov, gameMode, solverType, samples) {
    _showLoading('Generating heatmap...');
    await _nextFrame();
    const res = parseInt(document.getElementById('heatmap-resolution').value, 10);
    const hm = Wasm.heatmap(stateVal, cov, gameMode, solverType, samples, res);
    cachedHeatmap = hm.grid;
    cachedHeatmapBounds = hm.bounds;
}

/** Re-render the solver canvas with the current aim point and cached heatmap. */
function _renderBoard() {
    const aim = State.get('solver.optimalAim');
    const solverType = document.getElementById('solver-type').value;
    const invertColors = solverType === 'maxPoints';
    renderer.render({
        aimPoint: aim,
        heatmap: cachedHeatmap,
        heatmapBounds: cachedHeatmapBounds,
        invertColors,
    });
}

/**
 * Populate the results panel with expected value and optimal aim coordinates.
 * @param {{expectedValue:number, optimalAim:{x:number,y:number}}} result
 * @param {string} solverType 'minThrows' | 'maxPoints'
 */
function _showResults(result, solverType) {
    const info = document.getElementById('solve-results');
    const label = solverType === 'maxPoints' ? 'Expected points' : 'Expected throws';
    info.innerHTML = `
        <p class="result-line">${label}: <span class="value">${result.expectedValue.toFixed(3)}</span></p>
        <p class="result-line">Optimal aim: <span class="value">(${result.optimalAim.x.toFixed(1)}, ${result.optimalAim.y.toFixed(1)})</span></p>
    `;
}

/* --- heatmap toggle --- */

/** Handle the heatmap toggle: compute and show, or clear, the heatmap overlay. */
async function _onHeatmapToggle() {
    const on = document.getElementById('show-heatmap').checked;
    State.set('solver.showHeatmap', on);
    _toggleHeatmapRes();

    if (on) {
        // If no solve has been done yet, run a full solve (which includes heatmap)
        if (!State.get('solver.optimalAim')) {
            await _onSolve();
            return;
        }
        // Otherwise just recompute heatmap with current settings
        const cov = _getCovariance() || [1600, 0, 0, 1600];
        const gameMode   = document.getElementById('game-mode').value;
        const stateVal   = parseInt(document.getElementById('game-state').value, 10);
        const solverType = document.getElementById('solver-type').value;
        const samples    = parseInt(document.getElementById('aim-samples').value, 10);
        try {
            await _computeHeatmap(stateVal, cov, gameMode, solverType, samples);
            _renderBoard();
        } catch (err) {
            console.error(err);
        } finally {
            _hideLoading();
        }
    } else {
        cachedHeatmap = null;
        cachedHeatmapBounds = null;
        _renderBoard();
    }
}

/** Handle a change to the heatmap resolution selector: persist and refresh the heatmap. */
async function _onResChange() {
    State.set('solver.heatmapResolution', parseInt(document.getElementById('heatmap-resolution').value, 10));

    // Refresh heatmap immediately if it's currently shown
    if (document.getElementById('show-heatmap').checked && State.get('solver.optimalAim')) {
        const cov = _getCovariance() || [1600, 0, 0, 1600];
        const gameMode   = document.getElementById('game-mode').value;
        const stateVal   = parseInt(document.getElementById('game-state').value, 10);
        const solverType = document.getElementById('solver-type').value;
        const samples    = parseInt(document.getElementById('aim-samples').value, 10);
        try {
            await _computeHeatmap(stateVal, cov, gameMode, solverType, samples);
            _renderBoard();
        } catch (err) {
            console.error(err);
        } finally {
            _hideLoading();
        }
    }
}

/* --- loading overlay (CSS handles 500ms fade-in delay) --- */

/**
 * Show the loading overlay with a status message.
 * @param {string} msg Message to display.
 */
function _showLoading(msg) {
    document.getElementById('loading-message').textContent = msg;
    const overlay = document.getElementById('loading-overlay');
    overlay.classList.remove('hidden');
    // Reset animation so the delay re-triggers each time
    overlay.style.animation = 'none';
    overlay.offsetHeight; // force reflow
    overlay.style.animation = '';
}
/** Hide the loading overlay. */
function _hideLoading() {
    document.getElementById('loading-overlay').classList.add('hidden');
}
/** Returns a promise that resolves after two animation frames, allowing the DOM to repaint.
 * @returns {Promise<void>}
 */
function _nextFrame() {
    return new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r)));
}

/** Size the canvas internal resolution to match its CSS display size. */
function _resizeCanvas(canvas) {
    const wrap = canvas.parentElement;
    const maxW = wrap.clientWidth - 24;
    const maxH = wrap.clientHeight - 24;
    const size = Math.max(400, Math.min(maxW, maxH));
    canvas.width = size;
    canvas.height = size;
}
