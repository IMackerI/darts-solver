/**
 * @file main.js
 * @brief App entry point — handles routing, WASM initialisation, and page lifecycle.
 */
import * as State from './state.js';
import * as Wasm  from './wasm.js';
import { parseTarget, computeBounds } from './target.js';
import * as CalibrationPage from './pages/calibration.js';
import * as SolverPage from './pages/solver.js';

let targetBeds = null;
let targetBounds = null;

/* ---------- routing ---------- */

const pages = {
    '':          'landing',
    'calibrate': 'calibration',
    'solve':     'solver',
};
/** Returns the current route name derived from the URL hash.
 * @returns {string} One of 'landing', 'calibration', 'solver'.
 */
function getRoute() {
    const hash = location.hash.replace(/^#\/?/, '');
    return pages[hash] ?? 'landing';
}

let currentPage = null;

/**
 * Navigate to the route matching the current URL hash,
 * unmounting the previous page and mounting the new one.
 */
function navigate() {
    const page = getRoute();
    if (page === currentPage) return;

    // unmount previous page
    if (currentPage === 'calibration') CalibrationPage.unmount();
    if (currentPage === 'solver')      SolverPage.unmount();

    // toggle visibility
    document.querySelectorAll('.page').forEach(el => el.classList.remove('active'));
    document.getElementById(`page-${page}`)?.classList.add('active');

    // update nav active state
    document.querySelectorAll('.nav-link').forEach(a => {
        a.classList.toggle('active', a.dataset.page === page);
    });

    currentPage = page;

    // mount new page
    if (page === 'calibration' && targetBeds) CalibrationPage.mount(targetBeds, targetBounds);
    if (page === 'solver'      && targetBeds) SolverPage.mount(targetBeds, targetBounds);
}

/* ---------- init ---------- */

/**
 * Initialise the app: restore persisted state, start hash routing,
 * and load the WebAssembly module + target file.
 */
async function init() {
    State.loadFromStorage();

    // Navigate immediately (before WASM loads) so the landing page shows
    window.addEventListener('hashchange', navigate);
    navigate();

    // Load WASM
    const statusEl = document.getElementById('wasm-status');
    try {
        await Wasm.init();
        await Wasm.loadTarget('target.out');

        // Parse target geometry for JS rendering
        const content = Wasm.getTargetContent();
        targetBeds = parseTarget(content);
        targetBounds = computeBounds(targetBeds);

        statusEl.textContent = '✓ WASM Ready';
        statusEl.className = 'status ready';

        // Re-mount current page now that data is available
        if (currentPage === 'calibration') CalibrationPage.mount(targetBeds, targetBounds);
        if (currentPage === 'solver')      SolverPage.mount(targetBeds, targetBounds);

    } catch (err) {
        console.error('WASM init failed:', err);
        statusEl.textContent = '✗ WASM Error';
        statusEl.className = 'status error';
    }
}

init();
