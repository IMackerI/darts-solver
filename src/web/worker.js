/* global DartsModule */

let module = null;

let target = null;
let dist = null;
let game = null;
let solver = null;
let heatVis = null;

let cachedCov = null;
let cachedMode = null;
let cachedSolverType = null;
let cachedSamples = null;

function cleanupAll() {
    heatVis?.delete();
    heatVis = null;
    solver?.delete();
    solver = null;
    game?.delete();
    game = null;
    dist?.delete();
    dist = null;
    target?.delete();
    target = null;
    cachedCov = null;
    cachedMode = null;
    cachedSolverType = null;
    cachedSamples = null;
}

function ensureObjects(covFlat, gameMode, solverType, samples) {
    if (!target) {
        throw new Error('Target is not loaded in worker');
    }

    const covKey = covFlat.join(',');
    const needDist = covKey !== cachedCov;
    const needGame = needDist || gameMode !== cachedMode;
    const needSolver = needGame || solverType !== cachedSolverType || samples !== cachedSamples;

    if (needDist) {
        dist?.delete();
        dist = new module.NormalDistributionQuadrature(covFlat, { x: 0, y: 0 });
        cachedCov = covKey;
    }

    if (needGame) {
        heatVis?.delete();
        heatVis = null;
        solver?.delete();
        solver = null;
        game?.delete();

        const GameClass = gameMode === 'finishOnAny'
            ? module.GameFinishOnAny
            : module.GameFinishOnDouble;
        game = new GameClass(target, dist);
        cachedMode = gameMode;
    }

    if (needSolver) {
        heatVis?.delete();
        heatVis = null;
        solver?.delete();

        const SolverClass = solverType === 'maxPoints'
            ? module.MaxPointsSolver
            : solverType === 'minRounds'
                ? module.SolverMinRounds
                : module.SolverMinThrows;

        if (solverType === 'minRounds') {
            solver = new SolverClass(game, 3, samples);
        } else {
            solver = new SolverClass(game, samples);
        }

        cachedSolverType = solverType;
        cachedSamples = samples;
    }
}

function handleLoadTarget(payload) {
    const text = payload?.targetContent;
    if (typeof text !== 'string') {
        throw new Error('Invalid target content payload');
    }

    cleanupAll();
    target = new module.Target(text);
    return { ok: true };
}

function handleSolve(payload) {
    const {
        pointsRemaining,
        covFlat,
        gameMode,
        solverType,
        samples,
        minRoundsState,
    } = payload;

    ensureObjects(covFlat, gameMode, solverType, samples);
    const res = solverType === 'minRounds' && minRoundsState
        ? module.solverSolveMinRoundsRoundState(
            solver,
            minRoundsState.roundStartScore,
            minRoundsState.currentScore,
            minRoundsState.throwNumber,
        )
        : module.solverSolve(solver, pointsRemaining);

    return {
        expectedValue: res.expected_throws,
        optimalAim: {
            x: res.optimal_aim.x,
            y: res.optimal_aim.y,
        },
    };
}

function handleHeatmap(payload) {
    const {
        pointsRemaining,
        covFlat,
        gameMode,
        solverType,
        samples,
        resolution,
        minRoundsState,
    } = payload;

    ensureObjects(covFlat, gameMode, solverType, samples);
    const hm = solverType === 'minRounds' && minRoundsState
        ? module.solverHeatMapMinRoundsRoundState(
            solver,
            minRoundsState.roundStartScore,
            minRoundsState.currentScore,
            minRoundsState.throwNumber,
            resolution,
            resolution,
        )
        : (() => {
            if (!heatVis || heatVis._res !== resolution) {
                heatVis?.delete();
                heatVis = new module.HeatMapVisualizer(solver, resolution, resolution);
                heatVis._res = resolution;
            }
            return heatVis.heat_map(pointsRemaining);
        })();
    const rows = hm.size();
    const cols = rows > 0 ? hm.get(0).size() : 0;

    const grid = [];
    for (let r = 0; r < rows; r++) {
        const row = hm.get(r);
        const arr = new Array(cols);
        for (let c = 0; c < cols; c++) arr[c] = row.get(c);
        grid.push(arr);
    }

    const bounds = game.get_target_bounds();
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

const handlers = {
    loadTarget: handleLoadTarget,
    solve: handleSolve,
    heatmap: handleHeatmap,
};

function postError(id, err) {
    self.postMessage({
        id,
        error: {
            message: err?.message || String(err),
            stack: err?.stack || null,
        },
    });
}

function postResult(id, result) {
    self.postMessage({ id, result });
}

async function boot() {
    importScripts('darts_wasm.js');
    module = await DartsModule();
    self.postMessage({ type: 'ready' });
}

self.onmessage = async (event) => {
    const msg = event.data || {};
    const { id, type, payload } = msg;

    if (!handlers[type]) {
        postError(id, new Error(`Unknown worker request type: ${type}`));
        return;
    }

    try {
        const result = handlers[type](payload || {});
        postResult(id, result);
    } catch (err) {
        postError(id, err);
    }
};

boot().catch((err) => {
    self.postMessage({
        type: 'boot-error',
        error: {
            message: err?.message || String(err),
            stack: err?.stack || null,
        },
    });
});
