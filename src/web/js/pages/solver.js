/**
 * @file solver.js
 * @brief Solver tabs page logic.
 */
import { DartboardRenderer } from '../dartboard.js';
import * as State from '../state.js';
import * as Wasm from '../wasm.js';

const DEFAULT_COVARIANCE = [1600, 0, 0, 1600];
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

        const trackedRoles = ['game-mode', 'points-remaining', 'aim-samples', 'solve-up-to'];
        for (const role of trackedRoles) {
            this.q(role)?.addEventListener('change', (e) => {
                e.target.blur();
                this._syncFormToState();
                if (role === 'solve-up-to') this._updateSolveUpToButtonLabel();
                if (this.autoSolve) this._scheduleAutoSolve();
            }, opts);
        }

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

        if (this.q('game-mode')) this.q('game-mode').value = gameMode;
        if (this.q('points-remaining')) this.q('points-remaining').value = pointsRemaining;
        if (this.q('aim-samples')) this.q('aim-samples').value = aimSamples;
        if (this.q('show-heatmap')) this.q('show-heatmap').checked = showHeatmap;
        if (this.q('heatmap-resolution')) this.q('heatmap-resolution').value = String(heatmapResolution);
        if (this.q('solve-up-to')) this.q('solve-up-to').value = solveUpTo;

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

        const solveUpToInput = this.q('solve-up-to');
        if (solveUpToInput) {
            State.set(this._tabPath('solveUpTo'), parseInt(solveUpToInput.value, 10) || 1);
        }

        State.saveToStorage();
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

        this._syncFormToState();
        if (this.autoSolve) this._scheduleAutoSolve();
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
            const result = Wasm.solve(params.pointsRemaining, cov, params.gameMode, params.solverType, params.samples);
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
                const hm = Wasm.heatmap(
                    params.pointsRemaining,
                    cov,
                    params.gameMode,
                    params.solverType,
                    params.samples,
                    params.heatmapResolution,
                );
                this.cachedHeatmap = hm.grid;
                this.cachedHeatmapBounds = hm.bounds;
            } else {
                this.cachedHeatmap = null;
                this.cachedHeatmapBounds = null;
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
                if (progressBar) {
                    progressBar.value = score;
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

                const params = {
                    ...base,
                    pointsRemaining: score,
                    showHeatmap: true,
                };

                const result = Wasm.solve(params.pointsRemaining, cov, params.gameMode, params.solverType, params.samples);
                await yieldToUi();
                if (this.cancelRequested) {
                    if (progressText) {
                        progressText.textContent = `Cancelled at ${Math.max(1, score - 1)} / ${limit}.`;
                    }
                    break;
                }

                await yieldToUi();
                if (this.cancelRequested) {
                    if (progressText) {
                        progressText.textContent = `Cancelled at ${Math.max(1, score - 1)} / ${limit}.`;
                    }
                    break;
                }
                const hm = Wasm.heatmap(
                    params.pointsRemaining,
                    cov,
                    params.gameMode,
                    params.solverType,
                    params.samples,
                    params.heatmapResolution,
                );

                this.lastResult = result;
                this.cachedHeatmap = hm.grid;
                this.cachedHeatmapBounds = hm.bounds;

                State.set(this._tabPath('pointsRemaining'), score);
                State.set(this._tabPath('optimalAim'), result.optimalAim);
                State.set(this._tabPath('expectedValue'), result.expectedValue);
                State.saveToStorage();

                this._renderBoard();
                this._showResults(result);
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

        panel.innerHTML = `
            <p class="result-line">${valueLabel}: <span class="value">${result.expectedValue.toFixed(3)}</span></p>
            <p class="result-line">Optimal aim: <span class="value">(${result.optimalAim.x.toFixed(1)}, ${result.optimalAim.y.toFixed(1)})</span></p>
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
