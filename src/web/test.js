// Load and use the WebAssembly module

// Initialize the module when it's ready
let dartsModule = null;

async function initWasm() {
    try {
        // Load the WebAssembly module
        dartsModule = await DartsModule();
        console.log('WebAssembly module loaded successfully!');
        
        // Test the functions
        testFunctions();
    } catch (error) {
        console.error('Error loading WebAssembly module:', error);
    }
}

function testFunctions() {
    if (!dartsModule) {
        console.error('Module not loaded yet');
        return;
    }
    
    // Test add function
    const sum = dartsModule.add(5, 3);
    console.log('5 + 3 =', sum);
    
    // Test getCircleInfo function
    const info = dartsModule.getCircleInfo(10.5);
    console.log('Circle info:', info);
    
    // Update the result in the HTML
    const resultElement = document.getElementById('result');
    if (resultElement) {
        resultElement.textContent = `Sum: ${sum}, ${info}`;
    }
}

// Set up button click handler
window.addEventListener('DOMContentLoaded', () => {
    const startButton = document.getElementById('startButton');
    if (startButton) {
        startButton.addEventListener('click', () => {
            if (dartsModule) {
                testFunctions();
            } else {
                console.log('Module not loaded yet, initializing...');
                initWasm();
            }
        });
    }
    
    // Auto-load on page load
    initWasm();
});