// ==================== MAIN KNOB ====================
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
    const arcStartY = 100 - 80 * Math.sin(startRadians);
    const endX = 100 + 80 * Math.cos(endRadians);
    const endY = 100 - 80 * Math.sin(endRadians);
    
    if (value > 0) {
        const arcPath = `M ${startX} ${arcStartY} A 80 80 0 ${largeArcFlag} 0 ${endX} ${endY}`;
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
    const saturation = 50 + (value * 0.5);
    progressArc.style.stroke = `hsl(${hue}, ${saturation}%, 60%)`;
}

// Update parameter in JUCE
function updateParameter(paramId, value) {
    if (typeof window.juce !== 'undefined' && window.juce.setParameter) {
        window.juce.setParameter(paramId, value);
    }
}

// Handle mouse down on main knob
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
    const newValue = startValue + (deltaY * 0.5);
    
    currentValue = Math.max(0, Math.min(100, newValue));
    updateKnobVisual(currentValue);
    updateParameter('buildup', currentValue);
});

// Handle mouse up
document.addEventListener('mouseup', () => {
    if (isDragging) {
        isDragging = false;
        knobContainer.style.cursor = 'pointer';
    }
    if (isDraggingDrive) {
        isDraggingDrive = false;
        driveKnobContainer.style.cursor = 'pointer';
    }
});

// Handle mouse wheel
knobContainer.addEventListener('wheel', (e) => {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 1 : -1;
    currentValue = Math.max(0, Math.min(100, currentValue + delta));
    updateKnobVisual(currentValue);
    updateParameter('buildup', currentValue);
});

// Handle double click (reset to 0)
knobContainer.addEventListener('dblclick', () => {
    currentValue = 0;
    updateKnobVisual(currentValue);
    updateParameter('buildup', currentValue);
});

// Function called from JUCE to update knob
function updateKnobValue(value) {
    currentValue = value;
    updateKnobVisual(value);
}

// ==================== POPUP MENU ====================
const filterSection = document.getElementById('filter-section');
const filterSettingsBtn = document.getElementById('filter-settings-btn');
const popupOverlay = document.getElementById('popup-overlay');
const closePopupBtn = document.getElementById('close-popup');

function openPopup() {
    popupOverlay.classList.add('active');
}

function closePopup() {
    popupOverlay.classList.remove('active');
}

// Open popup when clicking filter section or settings button
filterSection.addEventListener('click', (e) => {
    openPopup();
});

filterSettingsBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    openPopup();
});

// Close popup
closePopupBtn.addEventListener('click', closePopup);

// Close popup when clicking overlay background
popupOverlay.addEventListener('click', (e) => {
    if (e.target === popupOverlay) {
        closePopup();
    }
});

// Close popup with Escape key
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        closePopup();
    }
});

// ==================== DRIVE KNOB ====================
let isDraggingDrive = false;
let driveValue = 0;
let driveStartY = 0;
let driveStartValue = 0;

const driveKnobContainer = document.getElementById('drive-knob-container');
const driveIndicator = document.getElementById('drive-indicator');
const driveArc = document.getElementById('drive-arc');
const driveValueText = document.getElementById('drive-value');

function updateDriveKnobVisual(value) {
    const angle = valueToAngle(value);
    const radians = (angle * Math.PI) / 180;
    
    // Update indicator position (scaled for mini knob)
    const indicatorX = 40 + 28 * Math.cos(radians);
    const indicatorY = 40 - 28 * Math.sin(radians);
    driveIndicator.setAttribute('cx', indicatorX);
    driveIndicator.setAttribute('cy', indicatorY);
    
    // Update progress arc
    const startAngle = 225;
    const endAngle = angle;
    const largeArcFlag = Math.abs(endAngle - startAngle) > 180 ? 1 : 0;
    
    const startRadians = (startAngle * Math.PI) / 180;
    const endRadians = (endAngle * Math.PI) / 180;
    
    const startX = 40 + 32 * Math.cos(startRadians);
    const arcStartY = 40 - 32 * Math.sin(startRadians);
    const endX = 40 + 32 * Math.cos(endRadians);
    const endY = 40 - 32 * Math.sin(endRadians);
    
    if (value > 0) {
        const arcPath = `M ${startX} ${arcStartY} A 32 32 0 ${largeArcFlag} 0 ${endX} ${endY}`;
        driveArc.setAttribute('d', arcPath);
    } else {
        driveArc.setAttribute('d', '');
    }
    
    driveValueText.textContent = Math.round(value) + '%';
}

// Drive knob mouse interactions
driveKnobContainer.addEventListener('mousedown', (e) => {
    e.stopPropagation();
    isDraggingDrive = true;
    driveStartY = e.clientY;
    driveStartValue = driveValue;
    driveKnobContainer.style.cursor = 'grabbing';
});

document.addEventListener('mousemove', (e) => {
    if (!isDraggingDrive) return;
    
    const deltaY = driveStartY - e.clientY;
    const newValue = driveStartValue + (deltaY * 0.5);
    
    driveValue = Math.max(0, Math.min(100, newValue));
    updateDriveKnobVisual(driveValue);
    updateParameter('filterDrive', driveValue);
});

driveKnobContainer.addEventListener('wheel', (e) => {
    e.preventDefault();
    e.stopPropagation();
    const delta = e.deltaY < 0 ? 2 : -2;
    driveValue = Math.max(0, Math.min(100, driveValue + delta));
    updateDriveKnobVisual(driveValue);
    updateParameter('filterDrive', driveValue);
});

driveKnobContainer.addEventListener('dblclick', (e) => {
    e.stopPropagation();
    driveValue = 0;
    updateDriveKnobVisual(driveValue);
    updateParameter('filterDrive', driveValue);
});

// Function called from JUCE to update drive value
function updateDriveValue(value) {
    driveValue = value;
    updateDriveKnobVisual(value);
}

// ==================== SLOPE SELECTOR ====================
let currentSlope = 1; // Default to 12dB
const slopeButtons = document.querySelectorAll('.slope-btn');

slopeButtons.forEach(btn => {
    btn.addEventListener('click', (e) => {
        e.stopPropagation();
        
        // Update active state
        slopeButtons.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        
        // Get slope value and update
        currentSlope = parseInt(btn.dataset.slope);
        updateParameter('filterSlope', currentSlope);
    });
});

// Function called from JUCE to update slope
function updateSlopeValue(slopeIndex) {
    currentSlope = slopeIndex;
    slopeButtons.forEach(btn => {
        btn.classList.toggle('active', parseInt(btn.dataset.slope) === slopeIndex);
    });
}

// ==================== INITIALIZE ====================
updateKnobVisual(0);
updateDriveKnobVisual(0);

// Expose functions to global scope for JUCE
window.updateKnobValue = updateKnobValue;
window.updateDriveValue = updateDriveValue;
window.updateSlopeValue = updateSlopeValue;
