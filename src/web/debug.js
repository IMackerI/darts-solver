// Darts Solver WebAssembly Debug Script
let dartsModule = null;
let output = null;

// Logging functions
function log(message, type = 'info') {
    const colors = {
        'info': '#d4d4d4',
        'success': '#4ec9b0',
        'error': '#f48771',
        'warning': '#dcdcaa'
    };
    const line = document.createElement('div');
    line.className = 'test-result';
    line.style.color = colors[type];
    line.textContent = message;
    output.appendChild(line);
    output.scrollTop = output.scrollHeight;
    console.log(message);
}

function clearOutput() {
    output.innerHTML = '';
}

function testPassed(name) {
    log(`✓ ${name}`, 'success');
}

function testFailed(name, error) {
    log(`✗ ${name}: ${error}`, 'error');
}

// Test 1: Basic Types
function testBasicTypes() {
    log('\n=== Testing Basic Types ===', 'warning');
    
    try {
        // Test Vec2 - it's a value_object, so we can create it as a plain JS object
        const vec = {x: 10.5, y: -5.3};
        log(`Created Vec2: {x: ${vec.x}, y: ${vec.y}}`);
        testPassed('Vec2 value object');
        
        // Test createVec2 helper function if needed
        try {
            const vec2 = dartsModule.createVec2(20.0, 15.0);
            log(`Created Vec2 with helper: {x: ${vec2.x}, y: ${vec2.y}}`);
            testPassed('Vec2 helper function');
        } catch (e) {
            log('Note: createVec2 helper not available (optional)', 'warning');
        }
        
    } catch (error) {
        testFailed('Vec2 test', error.message);
        return false;
    }
    
    return true;
}

// Test 2: Distribution Classes
function testDistributions() {
    log('\n=== Testing Distribution Classes ===', 'warning');
    
    try {
        // Test NormalDistributionQuadrature
        // JavaScript arrays are passed directly to C++ wrapper
        const cov = [[1600, 0], [0, 1600]];
        const mean = {x: 0, y: 0};
        
        log('Creating NormalDistributionQuadrature...');
        log('Covariance matrix: [[1600, 0], [0, 1600]]');
        const dist = new dartsModule.NormalDistributionQuadrature(cov, mean);
        testPassed('NormalDistributionQuadrature constructor');
        
        // Test inherited sample() function
        log('Testing inherited sample() function...');
        const sample1 = dist.sample();
        log(`Sample 1: {x: ${sample1.x.toFixed(3)}, y: ${sample1.y.toFixed(3)}}`);
        
        const sample2 = dist.sample();
        log(`Sample 2: {x: ${sample2.x.toFixed(3)}, y: ${sample2.y.toFixed(3)}}`);
        
        const sample3 = dist.sample();
        log(`Sample 3: {x: ${sample3.x.toFixed(3)}, y: ${sample3.y.toFixed(3)}}`);
        
        testPassed('Distribution.sample() inherited method works');
        
        // Calculate average distance from mean
        let totalDist = 0;
        const numSamples = 100;
        for (let i = 0; i < numSamples; i++) {
            const s = dist.sample();
            totalDist += Math.sqrt(s.x * s.x + s.y * s.y);
        }
        const avgDist = totalDist / numSamples;
        log(`Average distance from mean (${numSamples} samples): ${avgDist.toFixed(2)}`);
        
        dist.delete();
        testPassed('Distribution memory cleanup');
        
    } catch (error) {
        testFailed('Distribution test', error.message);
        return false;
    }
    
    // Test NormalDistributionRandom
    try {
        log('\nTesting NormalDistributionRandom...');
        const cov = [[1600, 0], [0, 1600]];
        const mean = {x: 0, y: 0};
        const numSamples = 1000;
        
        const distRandom = new dartsModule.NormalDistributionRandom(cov, mean, numSamples);
        testPassed('NormalDistributionRandom constructor');
        
        const sample = distRandom.sample();
        log(`Random sample: {x: ${sample.x.toFixed(3)}, y: ${sample.y.toFixed(3)}}`);
        
        testPassed('NormalDistributionRandom.sample()');
        
        distRandom.delete();
        
    } catch (error) {
        testFailed('NormalDistributionRandom test', error.message);
        return false;
    }
    
    return true;
}

// Test 3: Game Classes
async function testGameClasses() {
    log('\n=== Testing Game Classes ===', 'warning');
    
    let target = null;
    let dist = null;
    let game = null;
    
    try {
        // Load target file content
        const targetFile = document.getElementById('targetFile').value;
        log(`Loading target file "${targetFile}"...`);
        
        const response = await fetch(targetFile);
        if (!response.ok) {
            throw new Error(`Failed to load ${targetFile}: ${response.statusText}`);
        }
        const targetContent = await response.text();
        log('Target file loaded successfully');
        
        // Create Target from string content
        log('Creating Target from content...');
        target = new dartsModule.Target(targetContent);
        testPassed('Target constructor from string');
        
        // Create Distribution
        const cov = [[1600, 0], [0, 1600]];
        const mean = {x: 0, y: 0};
        dist = new dartsModule.NormalDistributionQuadrature(cov, mean);
        testPassed('Distribution created for Game');
        
        // Create GameFinishOnDouble (passing classes as parameters!)
        log('Creating GameFinishOnDouble (passing Target and Distribution objects)...');
        game = new dartsModule.GameFinishOnDouble(target, dist);
        testPassed('GameFinishOnDouble constructor with class parameters');
        
        // Test inherited method get_target_bounds
        log('Testing get_target_bounds()...');
        const bounds = dartsModule.getTargetBounds(game);
        log(`Target bounds: min(${bounds.min.x.toFixed(2)}, ${bounds.min.y.toFixed(2)}) ` +
            `max(${bounds.max.x.toFixed(2)}, ${bounds.max.y.toFixed(2)})`);
        testPassed('getTargetBounds() wrapper function');
        
        // Test throw_at_sample
        const aimPoint = {x: 0, y: 0};
        const state = 501;
        log(`Testing throw_at_sample at (${aimPoint.x}, ${aimPoint.y}) from state ${state}...`);
        
        const results = [];
        for (let i = 0; i < 5; i++) {
            const newState = game.throw_at_sample(aimPoint, state);
            results.push(newState);
        }
        log(`5 sample throws: [${results.join(', ')}]`);
        testPassed('Game.throw_at_sample() inherited method');
        
        // Test GameFinishOnAny
        log('\nTesting GameFinishOnAny...');
        const gameAny = new dartsModule.GameFinishOnAny(target, dist);
        testPassed('GameFinishOnAny constructor');
        
        const resultAny = gameAny.throw_at_sample(aimPoint, 50);
        log(`GameFinishOnAny throw result: ${resultAny}`);
        
        gameAny.delete();
        
    } catch (error) {
        testFailed('Game test', error.message);
        return false;
    } finally {
        // Cleanup
        if (game) game.delete();
        if (dist) dist.delete();
        if (target) target.delete();
    }
    
    return true;
}

// Test 4: Solver Class
async function testSolver() {
    log('\n=== Testing Solver Class ===', 'warning');
    
    let target = null;
    let dist = null;
    let game = null;
    let solver = null;
    
    try {
        // Setup - load target
        const targetFile = document.getElementById('targetFile').value;
        const response = await fetch(targetFile);
        if (!response.ok) {
            throw new Error(`Failed to load ${targetFile}`);
        }
        const targetContent = await response.text();
        target = new dartsModule.Target(targetContent);
        
        const cov = [[1600, 0], [0, 1600]];
        const mean = {x: 0, y: 0};
        dist = new dartsModule.NormalDistributionQuadrature(cov, mean);
        
        game = new dartsModule.GameFinishOnDouble(target, dist);
        
        // Create Solver (passing Game object!)
        log('Creating Solver with 1000 samples (passing Game object)...');
        solver = new dartsModule.Solver(game, 1000);
        testPassed('Solver constructor with Game parameter');
        
        log('✓ Solver creation successful!');
        log('Note: solve() method binding will be tested when implemented');
        
    } catch (error) {
        testFailed('Solver test', error.message);
        return false;
    } finally {
        // Cleanup
        if (solver) solver.delete();
        if (game) game.delete();
        if (dist) dist.delete();
        if (target) target.delete();
    }
    
    return true;
}

// Run all tests
async function runAllTests() {
    clearOutput();
    log('Starting comprehensive binding tests...', 'warning');
    log('====================================\n');
    
    let allPassed = true;
    
    allPassed = testBasicTypes() && allPassed;
    allPassed = testDistributions() && allPassed;
    allPassed = (await testGameClasses()) && allPassed;
    allPassed = (await testSolver()) && allPassed;
    
    log('\n====================================');
    if (allPassed) {
        log('✓ ALL TESTS PASSED!', 'success');
    } else {
        log('✗ SOME TESTS FAILED', 'error');
    }
}

// Manual throw test
async function testManualThrow() {
    clearOutput();
    log('Testing manual throw...', 'warning');
    
    try {
        const targetFile = document.getElementById('targetFile').value;
        const state = parseInt(document.getElementById('throwState').value);
        
        // Load target file
        const response = await fetch(targetFile);
        if (!response.ok) {
            throw new Error(`Failed to load ${targetFile}`);
        }
        const targetContent = await response.text();
        
        const target = new dartsModule.Target(targetContent);
        const cov = [[1600, 0], [0, 1600]];
        const dist = new dartsModule.NormalDistributionQuadrature(cov, {x: 0, y: 0});
        const game = new dartsModule.GameFinishOnDouble(target, dist);
        
        log(`Throwing 10 darts at (0, 0) from state ${state}:`);
        for (let i = 0; i < 10; i++) {
            const result = game.throw_at_sample({x: 0, y: 0}, state);
            log(`  Throw ${i+1}: ${state} → ${result}`);
        }
        
        game.delete();
        dist.delete();
        target.delete();
        
        testPassed('Manual throw test');
        
    } catch (error) {
        testFailed('Manual throw', error.message);
    }
}

// Initialize module
async function initWasm() {
    try {
        log('Loading WebAssembly module...', 'info');
        dartsModule = await DartsModule();
        
        document.getElementById('status').textContent = '✓ WebAssembly module loaded and ready!';
        document.getElementById('status').className = 'ready';
        
        // Enable buttons
        document.getElementById('testAllBtn').disabled = false;
        document.getElementById('testBasicBtn').disabled = false;
        document.getElementById('testDistributionBtn').disabled = false;
        document.getElementById('testGameBtn').disabled = false;
        document.getElementById('testThrowBtn').disabled = false;
        
        log('✓ Module loaded successfully!', 'success');
        log('Available classes:', 'info');
        log('  - Vec2', 'info');
        log('  - Target', 'info');
        log('  - NormalDistributionQuadrature', 'info');
        log('  - NormalDistributionRandom', 'info');
        log('  - GameFinishOnAny', 'info');
        log('  - GameFinishOnDouble', 'info');
        log('  - Solver', 'info');
        log('\nClick "Run All Tests" to verify bindings!', 'warning');
        
    } catch (error) {
        document.getElementById('status').textContent = '✗ Error loading module: ' + error.message;
        document.getElementById('status').className = 'error';
        log('✗ Failed to load module: ' + error.message, 'error');
    }
}

// Setup event listeners
window.addEventListener('DOMContentLoaded', () => {
    output = document.getElementById('output');
    
    document.getElementById('testAllBtn').addEventListener('click', runAllTests);
    document.getElementById('testBasicBtn').addEventListener('click', () => {
        clearOutput();
        testBasicTypes();
    });
    document.getElementById('testDistributionBtn').addEventListener('click', () => {
        clearOutput();
        testDistributions();
    });
    document.getElementById('testGameBtn').addEventListener('click', async () => {
        clearOutput();
        await testGameClasses();
    });
    document.getElementById('testThrowBtn').addEventListener('click', testManualThrow);
    document.getElementById('clearBtn').addEventListener('click', clearOutput);
    
    initWasm();
});
