/**
 * @file solver.js
 * @brief Solver tabs page logic.
 */
import { DartboardRenderer } from '../dartboard.js';
import * as State from '../state.js';
import * as Wasm from '../wasm.js';

const DEFAULT_COVARIANCE = [1600, 0, 0, 1600];
const MAX_SOLVE_CACHE = 50000;
const MAX_HEATMAP_CACHE = 1200;
const MAX_SOLVE_CACHE_HARD_CAP = 80000;
const MAX_HEATMAP_CACHE_HARD_CAP = 2500;
const instances = new Map();
let activeInstance = null;

export function mount(pageKey, parsedBeds, parsedBounds) {
    const section = document.getElementById(`page-${pageKey}`);
    if (!section) return;

    if (activeInstance && activeInstance.section !== section) {
        activeInstance.unmount();
    }

    let instance = instances.get(pageKey);
    if (!instance) {
        instance = new SolverTabController(section);
        instances.set(pageKey, instance);
    }

    activeInstance = instance;
    instance.mount(parsedBeds, parsedBounds);
}

export function unmount() {
    activeInstance?.unmount();
    activeInstance = null;
}

class SolverTabController {
    constructor(section) {
        this.section = section;
        this.solverType = section.dataset.solverType;
        this.autoSolve = section.dataset.autoSolve === 'true';

        this.renderer = null;
        this.beds = null;
        this.bounds = null;

        this.cachedHeatmap = null;
        this.cachedHeatmapBounds = null;
        this.currentHeatmapKey = null;
        this.solveCache = new Map();
        this.heatmapCache = new Map();
        this.solveCacheLimit = MAX_SOLVE_CACHE;
        this.heatmapCacheLimit = MAX_HEATMAP_CACHE;

        this.lastResult = null;
        this.mounted = false;
        this.abortController = null;
        this.autoSolveTimer = null;
        this.runningBatch = false;
        this.isSolving = false;
        this.cancelRequested = false;
    }

    q(role) {
        return this.section.querySelector(`[data-role="${role}"]`);
    }

    mount(parsedBeds, parsedBounds) {
        this.beds = parsedBeds;
        this.bounds = parsedBounds;

        const canvas = this.q('solver-canvas');
        if (!canvas) return;

        this._resizeCanvas(canvas);
        this.renderer = new DartboardRenderer(canvas);
        this.renderer.setBeds(this.beds, this.bounds);
        this.renderer.render();

        this._restoreFormFromState();
        this._toggleHeatmapResolutionVisibility();
        this._updateSolveUpToButtonLabel();
        this._setCancelButtonVisible(false);

        if (!this.mounted) {
            this._attachListeners();
            this.mounted = true;
        }

        this._updateCalibrationWarning();

        if (this.autoSolve) {
            this._scheduleAutoSolve();
        } else {
            this._renderBoard();
        }
    }

    unmount() {
        this.abortController?.abort();
        this.abortController = null;
        this.mounted = false;
        this._clearAutoSolveTimer();

        this.cachedHeatmap = null;
        this.cachedHeatmapBounds = null;
        this.currentHeatmapKey = null;
    }

    _attachListeners() {
        const ac = new AbortController();
        this.abortController = ac;
        const opts = { signal: ac.signal };

        this.q('show-heatmap')?.addEventListener('change', (e) => {
            e.target.blur();
            this._onHeatmapToggle();
        }, opts);

        this.q('heatmap-resolution')?.addEventListener('change', (e) => {
            e.target.blur();
            this._onResolutionChange();
        }, opts);

        this.q('btn-points-prev')?.addEventListener('click', (e) => {
            e.target.blur();
            this._stepPointsRemaining(-1);
        }, opts);

        this.q('btn-points-next')?.addEventListener('click', (e) => {
            e.target.blur();
            this._stepPointsRemaining(+1);
        }, opts);

        this.q('btn-throw-prev')?.addEventListener('click', (e) => {
            e.target.blur();
            this._stepThrowNumber(-1);
        }, opts);

        this.q('btn-throw-next')?.addEventListener('click', (e) => {
            e.target.blur();
            this._stepThrowNumber(+1);
        }, opts);

        this.q('btn-solve')?.addEventListener('click', (e) => {
            e.target.blur();
            this.solve({ loadingMessage: 'Solving...' });
        }, opts);

        this.q('btn-solve-up-to')?.addEventListener('click', (e) => {
            e.target.blur();
            this.solveUpToScore();
        }, opts);

        this.q('btn-cancel-solve')?.addEventListener('click', (e) => {
            e.target.blur();
            this._requestCancel();
        }, opts);

        const trackedRoles = ['game-mode', 'points-remaining', 'aim-samples', 'solve-up-to', 'round-start-score', 'precompute-all-round-starts'];
        for (const role of trackedRoles) {
            this.q(role)?.addEventListener('change', (e) => {
                e.target.blur();
                this._syncFormToState();
                if (role === 'solve-up-to') this._updateSolveUpToButtonLabel();
                if (this.autoSolve) this._scheduleAutoSolve();
            }, opts);
        }

        // Keep the solve-up button text in sync while the user types.
        this.q('solve-up-to')?.addEventListener('input', () => {
            this._updateSolveUpToButtonLabel();
        }, opts);

        document.addEventListener('keydown', (e) => this._onKeyDown(e), opts);

        const canvas = this.q('solver-canvas');
        canvas?.addEventListener('mousemove', (e) => this._onCanvasMouseMove(e), opts);
        canvas?.addEventListener('mouseleave', () => this._onCanvasMouseLeave(), opts);
    }

    _tabPath(path) {
        return `solverTabs.${this.solverType}.${path}`;
    }

    _getCovariance() {
        return State.get('calibration.covariance') || DEFAULT_COVARIANCE;
    }

    _restoreFormFromState() {
        const defaults = this._getDefaults();

        const gameMode = State.get(this._tabPath('gameMode')) ?? defaults.gameMode;
        const pointsRemaining = State.get(this._tabPath('pointsRemaining')) ?? defaults.pointsRemaining;
        const aimSamples = State.get(this._tabPath('aimSamples')) ?? defaults.aimSamples;
        const showHeatmap = State.get(this._tabPath('showHeatmap')) ?? defaults.showHeatmap;
        const heatmapResolution = State.get(this._tabPath('heatmapResolution')) ?? defaults.heatmapResolution;
        const solveUpTo = State.get(this._tabPath('solveUpTo')) ?? defaults.solveUpTo;
        const throwNumber = State.get(this._tabPath('throwNumber')) ?? defaults.throwNumber;
        const roundStartScore = State.get(this._tabPath('roundStartScore')) ?? defaults.roundStartScore;
        const precomputeAllRoundStarts = State.get(this._tabPath('precomputeAllRoundStarts')) ?? defaults.precomputeAllRoundStarts;

        if (this.q('game-mode')) this.q('game-mode').value = gameMode;
        if (this.q('points-remaining')) this.q('points-remaining').value = pointsRemaining;
        if (this.q('aim-samples')) this.q('aim-samples').value = aimSamples;
        if (this.q('show-heatmap')) this.q('show-heatmap').checked = showHeatmap;
        if (this.q('heatmap-resolution')) this.q('heatmap-resolution').value = String(heatmapResolution);
        if (this.q('solve-up-to')) this.q('solve-up-to').value = solveUpTo;
        if (this.q('throw-number')) this.q('throw-number').value = throwNumber;
        if (this.q('round-start-score')) this.q('round-start-score').value = roundStartScore;
        if (this.q('precompute-all-round-starts')) this.q('precompute-all-round-starts').checked = precomputeAllRoundStarts;

        this._syncMinRoundsRoundStateControls();

        this.lastResult = {
            expectedValue: State.get(this._tabPath('expectedValue')),
            optimalAim: State.get(this._tabPath('optimalAim')),
        };
    }

    _getDefaults() {
        const tabDefaults = {
            maxPoints: {
                pointsRemaining: 100000,
                gameMode: this.section.dataset.defaultGameMode || 'finishOnAny',
                aimSamples: parseInt(this.section.dataset.defaultSamples || '5000', 10),
                showHeatmap: this.section.dataset.defaultShowHeatmap === 'true',
                heatmapResolution: parseInt(this.section.dataset.defaultHeatmapResolution || '100', 10),
                solveUpTo: 170,
            },
            minThrows: {
                pointsRemaining: 501,
                gameMode: this.section.dataset.defaultGameMode || 'finishOnDouble',
                aimSamples: parseInt(this.section.dataset.defaultSamples || '5000', 10),
                showHeatmap: this.section.dataset.defaultShowHeatmap === 'true',
                heatmapResolution: parseInt(this.section.dataset.defaultHeatmapResolution || '50', 10),
                solveUpTo: 170,
            },
            minRounds: {
                pointsRemaining: 501,
                roundStartScore: 501,
                throwNumber: 1,
                precomputeAllRoundStarts: false,
                gameMode: this.section.dataset.defaultGameMode || 'finishOnDouble',
                aimSamples: parseInt(this.section.dataset.defaultSamples || '1000', 10),
                showHeatmap: this.section.dataset.defaultShowHeatmap === 'true',
                heatmapResolution: parseInt(this.section.dataset.defaultHeatmapResolution || '30', 10),
                solveUpTo: 170,
            },
        };
        return tabDefaults[this.solverType];
    }

    _syncFormToState() {
        const params = this._readParams();
        State.set(this._tabPath('gameMode'), params.gameMode);
        State.set(this._tabPath('pointsRemaining'), params.pointsRemaining);
        State.set(this._tabPath('aimSamples'), params.samples);
        State.set(this._tabPath('showHeatmap'), params.showHeatmap);
        State.set(this._tabPath('heatmapResolution'), params.heatmapResolution);
        if (this.solverType === 'minRounds') {
            State.set(this._tabPath('throwNumber'), params.minRoundsState.throwNumber);
            State.set(this._tabPath('roundStartScore'), params.minRoundsState.roundStartScore);
            State.set(this._tabPath('precomputeAllRoundStarts'), params.precomputeAllRoundStarts);
        }

        const solveUpToInput = this.q('solve-up-to');
        if (solveUpToInput) {
            State.set(this._tabPath('solveUpTo'), parseInt(solveUpToInput.value, 10) || 1);
        }

        this._dropStaleDisplayedHeatmap(params);
        State.saveToStorage();
        this._tryApplyCachedMinRoundsState(params);
    }

    _setCacheEntry(cache, key, value, maxSize) {
        if (cache.has(key)) {
            cache.delete(key);
        }
        cache.set(key, value);
        while (cache.size > maxSize) {
            const oldestKey = cache.keys().next().value;
            cache.delete(oldestKey);
        }
    }

    _roundStatePart(params) {
        if (params.solverType !== 'minRounds' || !params.minRoundsState) {
            return 'nostate';
        }
        const state = params.minRoundsState;
        return `${state.roundStartScore}:${params.pointsRemaining}:${state.throwNumber}`;
    }

    _solveCacheKey(params, cov) {
        const covKey = (cov || []).join(',');
        return [
            params.solverType,
            params.gameMode,
            params.samples,
            params.pointsRemaining,
            this._roundStatePart(params),
            covKey,
        ].join('|');
    }

    _heatmapCacheKey(params, cov) {
        const covKey = (cov || []).join(',');
        return [
            params.solverType,
            params.gameMode,
            params.samples,
            params.heatmapResolution,
            params.pointsRemaining,
            this._roundStatePart(params),
            covKey,
        ].join('|');
    }

    _dropStaleDisplayedHeatmap(params) {
        if (this.isSolving || this.runningBatch || !this.currentHeatmapKey) return;
        if (!params.showHeatmap) {
            this.cachedHeatmap = null;
            this.cachedHeatmapBounds = null;
            this.currentHeatmapKey = null;
            this._renderBoard();
            return;
        }

        const cov = this._getCovariance();
        const requestedKey = this._heatmapCacheKey(params, cov);
        if (requestedKey !== this.currentHeatmapKey) {
            this.cachedHeatmap = null;
            this.cachedHeatmapBounds = null;
            this.currentHeatmapKey = null;
            this._renderBoard();
        }
    }

    _tryApplyCachedMinRoundsState(params) {
        if (this.solverType !== 'minRounds') return false;
        if (this.isSolving || this.runningBatch) return false;

        const cov = this._getCovariance();
        const solveKey = this._solveCacheKey(params, cov);
        const cachedResult = this.solveCache.get(solveKey);
        if (!cachedResult) return false;

        let cachedHeatmap = null;
        let heatmapKey = null;
        if (params.showHeatmap) {
            heatmapKey = this._heatmapCacheKey(params, cov);
            cachedHeatmap = this.heatmapCache.get(heatmapKey);
        }

        this.lastResult = cachedResult;
        if (params.showHeatmap && cachedHeatmap) {
            this.cachedHeatmap = cachedHeatmap.grid;
            this.cachedHeatmapBounds = cachedHeatmap.bounds;
            this.currentHeatmapKey = heatmapKey;
        } else {
            // Solve result can still be applied instantly even if heatmap cache was evicted.
            this.cachedHeatmap = null;
            this.cachedHeatmapBounds = null;
            this.currentHeatmapKey = null;
        }

        State.set(this._tabPath('optimalAim'), cachedResult.optimalAim);
        State.set(this._tabPath('expectedValue'), cachedResult.expectedValue);
        State.saveToStorage();

        this._renderBoard();
        this._showResults(cachedResult);
        return true;
    }

    async _fetchSolveResult(params, cov) {
        const key = this._solveCacheKey(params, cov);
        const cached = this.solveCache.get(key);
        if (cached) return cached;

        const result = params.solverType === 'minRounds'
            ? await Wasm.solveWithRoundState(
                params.pointsRemaining,
                cov,
                params.gameMode,
                params.solverType,
                params.samples,
                params.minRoundsState,
            )
            : await Wasm.solve(params.pointsRemaining, cov, params.gameMode, params.solverType, params.samples);

        this._setCacheEntry(this.solveCache, key, result, this.solveCacheLimit);
        return result;
    }

    async _fetchHeatmap(params, cov) {
        const key = this._heatmapCacheKey(params, cov);
        const cached = this.heatmapCache.get(key);
        if (cached) return { key, hm: cached };

        const hm = params.solverType === 'minRounds'
            ? await Wasm.heatmapWithRoundState(
                params.pointsRemaining,
                cov,
                params.gameMode,
                params.solverType,
                params.samples,
                params.heatmapResolution,
                params.minRoundsState,
            )
            : await Wasm.heatmap(
                params.pointsRemaining,
                cov,
                params.gameMode,
                params.solverType,
                params.samples,
                params.heatmapResolution,
            );

        this._setCacheEntry(this.heatmapCache, key, hm, this.heatmapCacheLimit);
        return { key, hm };
    }

    _batchStatesForScore(base, score, limit) {
        const requestedThrow = base.minRoundsState?.throwNumber || 1;
        const preferredRoundStart = base.minRoundsState?.roundStartScore || score;
        const sharedRoundStart = Math.max(score, preferredRoundStart);

        const maxRoundStart = Math.max(score, limit);
        const startScores = base.precomputeAllRoundStarts
            ? (() => {
                const vals = [];
                for (let start = score; start <= maxRoundStart; start++) {
                    vals.push(start);
                }
                return vals;
            })()
            : [sharedRoundStart];

        const states = [{
            throwNumber: 1,
            roundStartScore: score,
        }];

        for (const roundStartScore of startScores) {
            states.push({
                throwNumber: 2,
                roundStartScore,
            });
            states.push({
                throwNumber: 3,
                roundStartScore,
            });
        }

        let displayIndex = states.findIndex((s) => s.throwNumber === requestedThrow && s.roundStartScore === preferredRoundStart);
        if (displayIndex < 0) displayIndex = states.findIndex((s) => s.throwNumber === requestedThrow);
        if (displayIndex < 0) displayIndex = 0;
        return { states, displayIndex };
    }

    _estimateBatchStateCount(limit, base) {
        if (!base.precomputeAllRoundStarts) {
            return limit * 3;
        }

        let total = 0;
        for (let score = 1; score <= limit; score++) {
            const startCount = limit - score + 1;
            total += 1 + 2 * startCount;
        }
        return total;
    }

    _updateCalibrationWarning() {
        const subtitle = this.q('solver-subtitle');
        if (!subtitle) return;

        if (!State.get('calibration.covariance')) {
            subtitle.textContent = 'No calibration data found. A default distribution is being used until you calibrate.';
            subtitle.style.color = 'var(--red)';
            return;
        }

        subtitle.style.color = '';
        if (this.solverType === 'maxPoints') {
            subtitle.textContent = 'Find the best place to aim for the highest expected points on your next throw.';
        } else if (this.solverType === 'minRounds') {
            subtitle.textContent = 'Use this higher-cost solver for round-level strategy and long batch calculations.';
        } else {
            subtitle.textContent = 'Configure points remaining and instantly compute the best aiming strategy.';
        }
    }

    _toggleHeatmapResolutionVisibility() {
        const field = this.q('heatmap-res-field');
        if (field) field.style.display = '';
    }

    _readParams() {
        const mode = this.q('game-mode')?.value || this.section.dataset.defaultGameMode || 'finishOnDouble';
        const samplesInput = parseInt(this.q('aim-samples')?.value || '1000', 10);
        const heatmapResolution = parseInt(this.q('heatmap-resolution')?.value || this.section.dataset.defaultHeatmapResolution || '50', 10);
        const showHeatmap = this.q('show-heatmap')?.checked ?? false;

        let pointsRemaining;
        if (this.solverType === 'maxPoints') {
            pointsRemaining = State.get(this._tabPath('pointsRemaining')) || 100000;
        } else {
            pointsRemaining = parseInt(this.q('points-remaining')?.value || '1', 10);
        }

        return {
            pointsRemaining,
            gameMode: mode,
            samples: Math.max(1, samplesInput || 1),
            solverType: this.solverType,
            showHeatmap,
            heatmapResolution: Math.max(1, heatmapResolution || 1),
            precomputeAllRoundStarts: this.solverType === 'minRounds'
                ? (this.q('precompute-all-round-starts')?.checked ?? false)
                : false,
            minRoundsState: this._readMinRoundsState(pointsRemaining),
        };
    }

    _readMinRoundsState(pointsRemaining) {
        if (this.solverType !== 'minRounds') return null;
        const throwInput = this.q('throw-number');
        const roundStartInput = this.q('round-start-score');
        const throwNumber = Math.max(1, Math.min(3, parseInt(throwInput?.value || '1', 10) || 1));
        let roundStartScore = parseInt(roundStartInput?.value || String(pointsRemaining), 10) || pointsRemaining;
        if (throwNumber === 1) {
            roundStartScore = pointsRemaining;
        }
        return {
            throwNumber,
            roundStartScore,
        };
    }

    _validateParams(params) {
        if (!Number.isFinite(params.pointsRemaining) || params.pointsRemaining < 1) {
            alert('Invalid points remaining value.');
            return false;
        }

        if ((params.solverType === 'minThrows' || params.solverType === 'minRounds') && params.pointsRemaining > 1000) {
            alert('Points remaining above 1000 are not supported for this solver.');
            return false;
        }

        if (params.solverType === 'minRounds') {
            const throwNumber = params.minRoundsState.throwNumber;
            const roundStartScore = params.minRoundsState.roundStartScore;
            if (!Number.isFinite(throwNumber) || throwNumber < 1 || throwNumber > 3) {
                alert('Throw in round must be between 1 and 3.');
                return false;
            }
            if (!Number.isFinite(roundStartScore) || roundStartScore < 1 || roundStartScore > 1000) {
                alert('Round start score must be between 1 and 1000.');
                return false;
            }
            if (roundStartScore < params.pointsRemaining) {
                alert('Round start score must be greater than or equal to points remaining.');
                return false;
            }
        }

        return true;
    }

    _clearAutoSolveTimer() {
        if (this.autoSolveTimer !== null) {
            clearTimeout(this.autoSolveTimer);
            this.autoSolveTimer = null;
        }
    }

    _scheduleAutoSolve() {
        if (!this.autoSolve || this.runningBatch) return;
        this._clearAutoSolveTimer();
        this.autoSolveTimer = setTimeout(() => {
            this.autoSolveTimer = null;
            this.solve({ loadingMessage: 'Solving...' });
        }, 120);
    }

    _stepPointsRemaining(delta) {
        const input = this.q('points-remaining');
        if (!input) return;

        const current = parseInt(input.value, 10) || 0;
        const max = parseInt(input.max || '1000', 10);
        const next = Math.max(1, Math.min(max, current + delta));
        input.value = String(next);
        this._syncMinRoundsRoundStateControls();

        this._syncFormToState();
        if (this.autoSolve) this._scheduleAutoSolve();
    }

    _stepThrowNumber(delta) {
        if (this.solverType !== 'minRounds') return;
        const input = this.q('throw-number');
        if (!input) return;

        const current = parseInt(input.value, 10) || 1;
        const next = Math.max(1, Math.min(3, current + delta));
        input.value = String(next);
        this._syncMinRoundsRoundStateControls();
        this._syncFormToState();
        if (this.autoSolve) this._scheduleAutoSolve();
    }

    _syncMinRoundsRoundStateControls() {
        if (this.solverType !== 'minRounds') return;
        const throwInput = this.q('throw-number');
        const pointsInput = this.q('points-remaining');
        const roundStartInput = this.q('round-start-score');
        if (!throwInput || !pointsInput || !roundStartInput) return;

        const throwNumber = Math.max(1, Math.min(3, parseInt(throwInput.value, 10) || 1));
        throwInput.value = String(throwNumber);

        if (throwNumber === 1) {
            roundStartInput.value = pointsInput.value;
            roundStartInput.setAttribute('disabled', 'disabled');
        } else {
            roundStartInput.removeAttribute('disabled');
        }
    }

    _onKeyDown(e) {
        if (!this.section.classList.contains('active')) return;
        if (!this.q('points-remaining')) return;

        const active = document.activeElement;
        const isTyping = active && (active.tagName === 'INPUT' || active.tagName === 'TEXTAREA');
        if (isTyping) return;

        if (e.key === 'ArrowLeft') {
            e.preventDefault();
            this._stepPointsRemaining(-1);
        } else if (e.key === 'ArrowRight') {
            e.preventDefault();
            this._stepPointsRemaining(+1);
        }
    }

    _onHeatmapToggle() {
        this._syncFormToState();

        if (this.autoSolve) {
            this._scheduleAutoSolve();
            return;
        }

        if (!this.q('show-heatmap')?.checked) {
            this.cachedHeatmap = null;
            this.cachedHeatmapBounds = null;
            this._renderBoard();
        }
    }

    _onResolutionChange() {
        this._syncFormToState();
        if (this.autoSolve) this._scheduleAutoSolve();
    }

    _tooltip() {
        return document.getElementById('heatmap-tooltip');
    }

    _hideTooltip() {
        this._tooltip().classList.add('hidden');
    }

    _requestCancel() {
        this.cancelRequested = true;
        const cancelBtn = this.q('btn-cancel-solve');
        if (cancelBtn) {
            cancelBtn.setAttribute('disabled', 'disabled');
            cancelBtn.textContent = 'Cancelling...';
        }
        const progressText = this.q('batch-progress-text');
        if (progressText) {
            progressText.textContent = 'Cancelling after current computation...';
        }
    }

    _updateSolveUpToButtonLabel() {
        const btn = this.q('btn-solve-up-to');
        const input = this.q('solve-up-to');
        if (!btn || !input) return;
        const value = parseInt(input.value || '0', 10);
        if (Number.isFinite(value) && value >= 1) {
            btn.textContent = `Solve up to ${value}`;
        } else {
            btn.textContent = 'Solve up to a score';
        }
    }

    _onCanvasMouseMove(e) {
        const tip = this._tooltip();
        if (!this.cachedHeatmap || !this.renderer) {
            tip.classList.add('hidden');
            return;
        }

        const rect = e.currentTarget.getBoundingClientRect();
        const px = e.clientX - rect.left;
        const py = e.clientY - rect.top;
        const val = this.renderer.heatmapValueAt(px, py);

        if (val === null) {
            tip.classList.add('hidden');
            return;
        }

        const label = this.solverType === 'maxPoints'
            ? `${val.toFixed(2)} pts`
            : this.solverType === 'minRounds'
                ? `${val.toFixed(2)} rounds`
                : `${val.toFixed(2)} throws`;

        tip.textContent = label;
        tip.style.left = `${e.clientX}px`;
        tip.style.top = `${e.clientY}px`;
        tip.classList.remove('hidden');
    }

    _onCanvasMouseLeave() {
        this._tooltip().classList.add('hidden');
    }

    async solve({ loadingMessage = 'Solving...', forceHeatmap = null, showOverlay = true } = {}) {
        if (this.runningBatch || this.isSolving) return false;

        const params = this._readParams();
        if (!this._validateParams(params)) return false;

        this.isSolving = true;
        this.cancelRequested = false;
        this._setNavigationDisabled(true);
        this._hideTooltip();
        this._setCancelButtonVisible(false);

        if (showOverlay) {
            showLoading(loadingMessage);
            await nextFrame();
            if (this.cancelRequested) return false;
        }

        try {
            const cov = this._getCovariance();
            const result = await this._fetchSolveResult(params, cov);
            if (this.cancelRequested) return false;
            this.lastResult = result;

            State.set(this._tabPath('optimalAim'), result.optimalAim);
            State.set(this._tabPath('expectedValue'), result.expectedValue);
            this._syncFormToState();

            const shouldComputeHeatmap = forceHeatmap ?? params.showHeatmap;
            if (shouldComputeHeatmap) {
                if (showOverlay) {
                    showLoading('Generating heatmap...');
                    await nextFrame();
                    if (this.cancelRequested) return false;
                }
                const { key, hm } = await this._fetchHeatmap(params, cov);
                if (this.cancelRequested) return false;
                this.cachedHeatmap = hm.grid;
                this.cachedHeatmapBounds = hm.bounds;
                this.currentHeatmapKey = key;
            } else {
                this.cachedHeatmap = null;
                this.cachedHeatmapBounds = null;
                this.currentHeatmapKey = null;
            }

            this._renderBoard();
            this._showResults(result);
            return true;
        } catch (err) {
            console.error(err);
            alert(`Solve failed: ${err.message}`);
            return false;
        } finally {
            this.isSolving = false;
            this.cancelRequested = false;
            this._setCancelButtonVisible(false);
            this._setNavigationDisabled(false);
            if (showOverlay) hideLoading();
        }
    }

    async solveUpToScore() {
        if (this.solverType !== 'minRounds' || this.runningBatch || this.isSolving) return;

        const limitInput = this.q('solve-up-to');
        const limit = parseInt(limitInput?.value || '1', 10);
        if (!Number.isFinite(limit) || limit < 1 || limit > 1000) {
            alert('Please choose a valid upper score between 1 and 1000.');
            return;
        }

        const pointsInput = this.q('points-remaining');
        if (!pointsInput) return;

        const progressWrap = this.q('batch-progress');
        const progressText = this.q('batch-progress-text');
        const progressBar = this.q('batch-progress-bar');

        this.runningBatch = true;
        this.isSolving = true;
        this.cancelRequested = false;
        this._setControlsDisabled(true);
        this._setCancelButtonVisible(true);
        this._setNavigationDisabled(true);
        this._hideTooltip();

        if (progressWrap) progressWrap.hidden = false;
        if (progressBar) {
            progressBar.max = limit;
            progressBar.value = 0;
        }

        try {
            const cov = this._getCovariance();
            const base = this._readParams();
            const totalStates = this._estimateBatchStateCount(limit, base);
            let processedStates = 0;

            // Keep as many newly precomputed states as practical so subsequent interactions
            // and reruns can reuse results instead of recomputing.
            this.solveCacheLimit = Math.min(
                Math.max(this.solveCacheLimit, totalStates + 256),
                MAX_SOLVE_CACHE_HARD_CAP,
            );
            this.heatmapCacheLimit = Math.min(
                Math.max(this.heatmapCacheLimit, Math.min(totalStates, MAX_HEATMAP_CACHE_HARD_CAP)),
                MAX_HEATMAP_CACHE_HARD_CAP,
            );

            if (progressBar) {
                progressBar.max = totalStates;
                progressBar.value = 0;
            }

            for (let score = 1; score <= limit; score++) {
                if (this.cancelRequested) {
                    if (progressText) {
                        progressText.textContent = `Cancelled at ${Math.max(1, score - 1)} / ${limit}.`;
                    }
                    break;
                }
                if (progressText) {
                    progressText.textContent = `Calculating points remaining ${score} / ${limit}`;
                }
                pointsInput.value = String(score);

                // Yield before each heavy phase so cancel clicks get processed quickly.
                await yieldToUi();
                if (this.cancelRequested) {
                    if (progressText) {
                        progressText.textContent = `Cancelled at ${Math.max(1, score - 1)} / ${limit}.`;
                    }
                    break;
                }

                const { states, displayIndex } = this._batchStatesForScore(base, score, limit);
                let selectedResult = null;
                let selectedHeatmap = null;
                let selectedHeatmapKey = null;

                for (let idx = 0; idx < states.length; idx++) {
                    const state = states[idx];
                    const params = {
                        ...base,
                        pointsRemaining: score,
                        showHeatmap: true,
                        minRoundsState: state,
                    };

                    if (progressText) {
                        progressText.textContent = `Calculating points ${score}/${limit}, throw ${state.throwNumber}/3, round start ${state.roundStartScore}`;
                    }

                    const result = await this._fetchSolveResult(params, cov);
                    await yieldToUi();
                    if (this.cancelRequested) break;

                    const { key, hm } = await this._fetchHeatmap(params, cov);
                    processedStates += 1;
                    if (progressBar) {
                        progressBar.value = processedStates;
                    }
                    await yieldToUi();
                    if (this.cancelRequested) break;

                    if (idx === displayIndex) {
                        selectedResult = result;
                        selectedHeatmap = hm;
                        selectedHeatmapKey = key;
                    }
                }

                if (this.cancelRequested) {
                    if (progressText) {
                        progressText.textContent = `Cancelled at ${Math.max(1, score - 1)} / ${limit}.`;
                    }
                    break;
                }

                if (selectedResult && selectedHeatmap) {
                    this.lastResult = selectedResult;
                    this.cachedHeatmap = selectedHeatmap.grid;
                    this.cachedHeatmapBounds = selectedHeatmap.bounds;
                    this.currentHeatmapKey = selectedHeatmapKey;
                }

                State.set(this._tabPath('pointsRemaining'), score);
                if (selectedResult) {
                    State.set(this._tabPath('optimalAim'), selectedResult.optimalAim);
                    State.set(this._tabPath('expectedValue'), selectedResult.expectedValue);
                }
                State.saveToStorage();

                this._renderBoard();
                this._showResults(this.lastResult);
                await yieldToUi();
            }

            if (progressText && !this.cancelRequested) {
                progressText.textContent = `Finished up to ${limit}.`;
            }
        } catch (err) {
            console.error(err);
            alert(`Batch solve failed: ${err.message}`);
        } finally {
            this.runningBatch = false;
            this.isSolving = false;
            this.cancelRequested = false;
            this._setControlsDisabled(false);
            this._syncMinRoundsRoundStateControls();
            this._setCancelButtonVisible(false);
            this._setNavigationDisabled(false);
            if (progressWrap) progressWrap.hidden = true;
        }
    }

    _setCancelButtonVisible(visible) {
        const btn = this.q('btn-cancel-solve');
        if (!btn) return;
        if (visible) {
            btn.textContent = 'Cancel solve';
            btn.removeAttribute('disabled');
        } else {
            btn.textContent = 'Cancel solve';
            btn.setAttribute('disabled', 'disabled');
        }
    }

    _setNavigationDisabled(disabled) {
        document.querySelectorAll('.nav-link').forEach((link) => {
            link.classList.toggle('nav-link-disabled', disabled);
            if (disabled) {
                link.setAttribute('aria-disabled', 'true');
                link.setAttribute('tabindex', '-1');
            } else {
                link.removeAttribute('aria-disabled');
                link.removeAttribute('tabindex');
            }
        });
    }

    _setControlsDisabled(disabled) {
        const controls = this.section.querySelectorAll('button, input, select');
        controls.forEach((el) => {
            if (el.dataset.role === 'batch-progress-bar') return;
            el.disabled = disabled;
        });
    }

    _renderBoard() {
        this._hideTooltip();
        const aim = this.lastResult?.optimalAim || State.get(this._tabPath('optimalAim'));
        if (!this.renderer) return;

        this.renderer.render({
            aimPoint: aim,
            heatmap: this.cachedHeatmap,
            heatmapBounds: this.cachedHeatmapBounds,
            invertColors: this.solverType === 'maxPoints',
        });
    }

    _showResults(result) {
        const panel = this.q('solve-results');
        if (!panel || !result) return;

        const valueLabel = this.solverType === 'maxPoints'
            ? 'Expected points'
            : this.solverType === 'minRounds'
                ? 'Expected rounds'
                : 'Expected throws';

        const currentParams = this._readParams();

        const minRoundsContext = this.solverType === 'minRounds'
            ? `<p class="result-line">State: <span class="value">round start ${currentParams.minRoundsState.roundStartScore}, throw ${currentParams.minRoundsState.throwNumber}/3</span></p>`
            : '';

        panel.innerHTML = `
            <p class="result-line">${valueLabel}: <span class="value">${result.expectedValue.toFixed(3)}</span></p>
            <p class="result-line">Optimal aim: <span class="value">(${result.optimalAim.x.toFixed(1)}, ${result.optimalAim.y.toFixed(1)})</span></p>
            ${minRoundsContext}
        `;
    }

    _resizeCanvas(canvas) {
        const wrap = canvas.parentElement;
        const maxW = wrap.clientWidth - 24;
        const maxH = wrap.clientHeight - 24;
        const size = Math.max(400, Math.min(maxW, maxH));
        canvas.width = size;
        canvas.height = size;
    }
}

function showLoading(msg) {
    document.getElementById('loading-message').textContent = msg;
    const overlay = document.getElementById('loading-overlay');
    overlay.classList.remove('hidden');
    overlay.style.animation = 'none';
    overlay.offsetHeight;
    overlay.style.animation = '';
}

function hideLoading() {
    document.getElementById('loading-overlay').classList.add('hidden');
}

function nextFrame() {
    return new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
}

function yieldToUi() {
    return new Promise((resolve) => setTimeout(resolve, 0));
}
