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
    '':                 'landing',
    'calibrate':        'calibration',
    'solve':            'solver-min-throws',
    'solve/max-points': 'solver-max-points',
    'solve/min-throws': 'solver-min-throws',
    'solve/min-rounds': 'solver-min-rounds',
};
/** Returns the current route name derived from the URL hash.
 * @returns {string} One of landing/calibration/solver-* page keys.
 */
function getRoute() {
    const raw = (location.hash || '').trim();
    const noQuery = raw.split('?')[0];
    const normalized = decodeURIComponent(noQuery)
        .replace(/^#\/?/, '')
        .replace(/\/+$/, '');
    return pages[normalized] ?? 'landing';
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
    if (currentPage?.startsWith('solver-')) SolverPage.unmount();

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
    if (page.startsWith('solver-') && targetBeds) SolverPage.mount(page, targetBeds, targetBounds);
}

/* ---------- init ---------- */

/**
 * Initialise the app: restore persisted state, start hash routing,
 * and load the WebAssembly module + target file.
 */
async function init() {
    State.loadFromStorage();

    // Ensure nav works even if hashchange is not dispatched by the environment.
    document.querySelectorAll('.nav-link[href^="#/"]').forEach((link) => {
        link.addEventListener('click', (e) => {
            const href = link.getAttribute('href');
            if (!href) return;
            e.preventDefault();
            if (location.hash !== href) {
                location.hash = href;
            }
            navigate();
        });
    });

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
        if (currentPage?.startsWith('solver-')) SolverPage.mount(currentPage, targetBeds, targetBounds);

    } catch (err) {
        console.error('WASM init failed:', err);
        statusEl.textContent = '✗ WASM Error';
        statusEl.className = 'status error';
    }
}

init();
