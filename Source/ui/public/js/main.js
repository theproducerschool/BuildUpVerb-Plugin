let isDragging = false;
let currentValue = 0;
let startY = 0;
let startValue = 0;

const knobContainer = document.querySelector('.knob-container');
const indicator = document.getElementById('indicator');
const valueText = document.getElementById('value-text');
const progressArc = document.getElementById('progress-arc');

// Convert value (0-100) to angle (225 to -45 degrees)
function valueToAngle(value) {
    return 225 - (value * 2.7); // 270 degree sweep
}

// Convert angle to value
function angleToValue(angle) {
    let normalizedAngle = ((225 - angle) / 270) * 100;
    return Math.max(0, Math.min(100, normalizedAngle));
}

// Update knob visual
function updateKnobVisual(value) {
    const angle = valueToAngle(value);
    const radians = (angle * Math.PI) / 180;
    
    // Update indicator position
    const indicatorX = 100 + 70 * Math.cos(radians);
    const indicatorY = 100 - 70 * Math.sin(radians);
    indicator.setAttribute('cx', indicatorX);
    indicator.setAttribute('cy', indicatorY);
    
    // Update progress arc
    const startAngle = 225;
    const endAngle = angle;
    const largeArcFlag = Math.abs(endAngle - startAngle) > 180 ? 1 : 0;
    
    const startRadians = (startAngle * Math.PI) / 180;
    const endRadians = (endAngle * Math.PI) / 180;
    
    const startX = 100 + 80 * Math.cos(startRadians);
    const startY = 100 - 80 * Math.sin(startRadians);
    const endX = 100 + 80 * Math.cos(endRadians);
    const endY = 100 - 80 * Math.sin(endRadians);
    
    if (value > 0) {
        const arcPath = `M ${startX} ${startY} A 80 80 0 ${largeArcFlag} 0 ${endX} ${endY}`;
        progressArc.setAttribute('d', arcPath);
    } else {
        progressArc.setAttribute('d', '');
    }
    
    // Update value text
    valueText.textContent = Math.round(value) + '%';
    
    // Add pulse animation
    valueText.classList.add('value-changing');
    setTimeout(() => valueText.classList.remove('value-changing'), 200);
    
    // Update color based on value
    const hue = 240 - (value * 0.6); // Blue to purple
    const saturation = 50 + (value * 0.5); // Increase saturation
    progressArc.style.stroke = `hsl(${hue}, ${saturation}%, 60%)`;
}

// Update parameter in JUCE
function updateParameter(value) {
    if (typeof window.juce !== 'undefined' && window.juce.setParameter) {
        window.juce.setParameter('buildup', value);
    }
}

// Handle mouse down
knobContainer.addEventListener('mousedown', (e) => {
    isDragging = true;
    startY = e.clientY;
    startValue = currentValue;
    knobContainer.style.cursor = 'grabbing';
});

// Handle mouse move
document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    
    const deltaY = startY - e.clientY;
    const newValue = startValue + (deltaY * 0.5); // Sensitivity
    
    currentValue = Math.max(0, Math.min(100, newValue));
    updateKnobVisual(currentValue);
    updateParameter(currentValue);
});

// Handle mouse up
document.addEventListener('mouseup', () => {
    if (isDragging) {
        isDragging = false;
        knobContainer.style.cursor = 'pointer';
    }
});

// Handle mouse wheel
knobContainer.addEventListener('wheel', (e) => {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 1 : -1;
    currentValue = Math.max(0, Math.min(100, currentValue + delta));
    updateKnobVisual(currentValue);
    updateParameter(currentValue);
});

// Handle double click (reset to 0)
knobContainer.addEventListener('dblclick', () => {
    currentValue = 0;
    updateKnobVisual(currentValue);
    updateParameter(currentValue);
});

// Function called from JUCE to update knob
function updateKnobValue(value) {
    currentValue = value;
    updateKnobVisual(value);
}

// Initialize
updateKnobVisual(0);

// Expose function to global scope for JUCE
window.updateKnobValue = updateKnobValue;