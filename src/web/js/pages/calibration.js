/**
 * @file calibration.js
 * @brief Calibration page — record shots and estimate the 2D Gaussian throwing distribution.
 */
import { DartboardRenderer } from '../dartboard.js';
import * as State from '../state.js';

let renderer = null;
let beds = null;
let bounds = null;
let _mounted = false;
const _abortCtrl = { current: null };

/**
 * Mount the calibration page: initialise the renderer and attach event listeners.
 * @param {object[]} parsedBeds    Parsed bed array from target.js.
 * @param {{min:{x,y},max:{x,y}}} parsedBounds  Bounding box from target.js.
 */
export function mount(parsedBeds, parsedBounds) {
    beds = parsedBeds;
    bounds = parsedBounds;

    const canvas = document.getElementById('calibration-canvas');
    _resizeCanvas(canvas);
    renderer = new DartboardRenderer(canvas);
    renderer.setBeds(beds, bounds);
    _redraw();

    if (!_mounted) {
        _mounted = true;
        const ac = new AbortController();
        _abortCtrl.current = ac;
        const sig = { signal: ac.signal };

        canvas.addEventListener('click', _onCanvasClick, sig);
        document.getElementById('btn-undo').addEventListener('click', _undo, sig);
        document.getElementById('btn-clear').addEventListener('click', _clear, sig);
        document.getElementById('btn-estimate').addEventListener('click', _estimate, sig);
        document.getElementById('btn-export').addEventListener('click', _exportData, sig);
        document.getElementById('btn-import').addEventListener('click', () => {
            document.getElementById('import-file').click();
        }, sig);
        document.getElementById('import-file').addEventListener('change', _importData, sig);
    }

    _updateUI();
}

/** Unmount the calibration page and remove all event listeners via AbortController. */
export function unmount() {
    // AbortController removes all listeners at once
    _abortCtrl.current?.abort();
    _abortCtrl.current = null;
    _mounted = false;
}

/* --- event handlers --- */

/**
 * Handle a canvas click: convert pixel coords to dartboard mm and record a shot.
 * @param {MouseEvent} e
 */
function _onCanvasClick(e) {
    const rect = e.target.getBoundingClientRect();
    // Account for CSS scaling (canvas may be displayed smaller than its resolution)
    const scaleX = e.target.width  / rect.width;
    const scaleY = e.target.height / rect.height;
    const cx = (e.clientX - rect.left) * scaleX;
    const cy = (e.clientY - rect.top)  * scaleY;
    const pt = renderer.toBoard(cx, cy);

    const shots = State.get('calibration.shots');
    shots.push(pt);
    State.set('calibration.shots', shots);
    State.saveToStorage();

    _redraw();
    _updateUI();
}

/** Remove the most recently recorded shot. */
function _undo() {
    const shots = State.get('calibration.shots');
    if (shots.length === 0) return;
    shots.pop();
    State.set('calibration.shots', shots);
    State.saveToStorage();
    _redraw();
    _updateUI();
}

/** Clear all recorded shots after user confirmation. */
function _clear() {
    if (!confirm('Clear all shots?')) return;
    State.set('calibration.shots', []);
    State.set('calibration.covariance', null);
    State.set('calibration.mean', null);
    State.saveToStorage();
    _redraw();
    _updateUI();
}

/* --- distribution estimation (pure JS, no WASM needed) --- */

/**
 * Estimate a 2D Gaussian distribution from the recorded shots using
 * sample mean and covariance (Bessel-corrected).
 */
function _estimate() {
    const shots = State.get('calibration.shots');
    if (shots.length < 3) return;

    // Compute mean
    let mx = 0, my = 0;
    for (const s of shots) { mx += s.x; my += s.y; }
    mx /= shots.length;
    my /= shots.length;

    // Compute sample covariance
    let cxx = 0, cxy = 0, cyy = 0;
    for (const s of shots) {
        const dx = s.x - mx;
        const dy = s.y - my;
        cxx += dx * dx;
        cxy += dx * dy;
        cyy += dy * dy;
    }
    const n = shots.length - 1; // Bessel's correction
    cxx /= n; cxy /= n; cyy /= n;

    State.set('calibration.covariance', [cxx, cxy, cxy, cyy]);
    State.set('calibration.mean', { x: mx, y: my });
    State.saveToStorage();
    _updateUI();
}

/* --- export / import --- */

/** Export current calibration data as a timestamped JSON file download. */
function _exportData() {
    const data = {
        version: '1.0',
        shots: State.get('calibration.shots'),
        covariance: State.get('calibration.covariance'),
        mean: State.get('calibration.mean'),
        timestamp: new Date().toISOString(),
    };
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = `darts-calibration-${new Date().toISOString().slice(0, 10)}.json`;
    a.click();
    URL.revokeObjectURL(a.href);
}

/**
 * Import calibration data from a JSON file selected via file input.
 * @param {Event} e File input change event.
 */
function _importData(e) {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
        try {
            const data = JSON.parse(reader.result);
            if (!Array.isArray(data.shots)) throw new Error('Invalid format');
            const existing = State.get('calibration.shots') || [];
            State.set('calibration.shots', [...existing, ...data.shots]);
            // Covariance is now stale — clear it so the user re-estimates
            State.set('calibration.covariance', null);
            State.set('calibration.mean', null);
            State.saveToStorage();
            _redraw();
            _updateUI();
        } catch (err) {
            alert('Failed to import: ' + err.message);
        }
    };
    reader.readAsText(file);
    e.target.value = ''; // allow re-importing same file
}

/* --- rendering --- */

/** Redraw the calibration canvas with the current shot list. */
function _redraw() {
    const shots = State.get('calibration.shots');
    renderer.render({ shots });
    renderer.drawAimCenter();
}

/** Refresh all calibration UI elements (button states, shot count, distribution info). */
function _updateUI() {
    const shots = State.get('calibration.shots');
    const cov   = State.get('calibration.covariance');

    document.getElementById('shot-count').textContent = `${shots.length} shot${shots.length !== 1 ? 's' : ''} recorded`;
    document.getElementById('btn-undo').disabled    = shots.length === 0;
    document.getElementById('btn-clear').disabled   = shots.length === 0;
    document.getElementById('btn-export').disabled  = shots.length === 0;
    document.getElementById('btn-estimate').disabled = shots.length < 3;

    const info = document.getElementById('distribution-info');
    if (cov) {
        const mean = State.get('calibration.mean');
        const sx = Math.sqrt(cov[0]).toFixed(1);
        const sy = Math.sqrt(cov[3]).toFixed(1);
        info.innerHTML = `
            <p class="result-line">Mean: <span class="value">(${mean.x.toFixed(1)}, ${mean.y.toFixed(1)})</span></p>
            <p class="result-line">σ<sub>x</sub> = <span class="value">${sx} mm</span>, σ<sub>y</sub> = <span class="value">${sy} mm</span></p>
            <p class="result-line">Covariance: <span class="value">[${cov.map(v => v.toFixed(1)).join(', ')}]</span></p>
        `;
    } else if (shots.length >= 3) {
        info.innerHTML = '<p class="muted">Press "Estimate Distribution" to compute.</p>';
    } else {
        info.innerHTML = '<p class="muted">Record at least 3 shots to estimate your distribution.</p>';
    }
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
