document.addEventListener('DOMContentLoaded', () => {
    const toggleSwitches = document.querySelectorAll('.toggle-switch');
    const brightnessSliders = document.querySelectorAll('.brightness-slider');
    const speedSliders = document.querySelectorAll('.speed-slider');
    const switchToMapBtn = document.getElementById('switchToMap');
    const tabContent = document.getElementById('roomTabContent');
    const map2DContainer = document.getElementById('2d-map-container');

    const modal = new bootstrap.Modal(document.getElementById('device-control-modal'));

    // Load saved states from localStorage
    const loadDeviceState = (deviceId) => {
        const savedState = localStorage.getItem(`${deviceId}-state`);
        const savedBrightness = localStorage.getItem(`${deviceId}-brightness`);
        const savedSpeed = localStorage.getItem(`${deviceId}-speed`);
        const savedStartTime = localStorage.getItem(`${deviceId}-startTime`);
        return {
            state: savedState || 'off',
            brightness: savedBrightness || 50,
            speed: savedSpeed || 50,
            startTime: savedStartTime ? parseInt(savedStartTime) : null
        };
    };

    const saveDeviceState = (deviceId, state, brightness, speed, startTime) => {
        localStorage.setItem(`${deviceId}-state`, state);
        if (brightness !== undefined) localStorage.setItem(`${deviceId}-brightness`, brightness);
        if (speed !== undefined) localStorage.setItem(`${deviceId}-speed`, speed);
        if (startTime !== undefined) localStorage.setItem(`${deviceId}-startTime`, startTime);
    };

    const updateStatusText = (deviceId, state, brightness, speed, timeOn) => {
        const deviceControl = document.querySelector(`[data-device="${deviceId}"]`).closest('.device-control');
        const statusText = deviceControl.querySelector('.status-text');
        const stateSpan = statusText.querySelector('.state');
        const brightnessSpan = statusText.querySelector('.brightness');
        const speedSpan = statusText.querySelector('.speed');
        const timeOnSpan = statusText.querySelector('.time-on');

        stateSpan.textContent = state === 'on' ? 'Bật' : 'Tắt';
        if (brightnessSpan) brightnessSpan.textContent = brightness;
        if (speedSpan) speedSpan.textContent = speed;
        timeOnSpan.textContent = timeOn;
    };

    const calculateTimeOn = (startTime) => {
        if (!startTime) return 0;
        const currentTime = Date.now();
        const minutes = Math.floor((currentTime - startTime) / 1000 / 60);
        return minutes;
    };

    // Initialize devices
    toggleSwitches.forEach(switchElement => {
        const deviceId = switchElement.getAttribute('data-device');
        const savedData = loadDeviceState(deviceId);

        switchElement.setAttribute('data-state', savedData.state);
        switchElement.classList.toggle('active', savedData.state === 'on');

        const brightnessSlider = document.querySelector(`.brightness-slider[data-device="${deviceId}"]`);
        const speedSlider = document.querySelector(`.speed-slider[data-device="${deviceId}"]`);

        if (brightnessSlider) brightnessSlider.value = savedData.brightness;
        if (speedSlider) speedSlider.value = savedData.speed;

        const timeOn = savedData.state === 'on' && savedData.startTime ? calculateTimeOn(savedData.startTime) : 0;
        updateStatusText(deviceId, savedData.state, savedData.brightness, savedData.speed, timeOn);
    });

    // Toggle Switch Logic
    toggleSwitches.forEach(switchElement => {
        switchElement.addEventListener('click', () => {
            const deviceId = switchElement.getAttribute('data-device');
            const currentState = switchElement.getAttribute('data-state');
            const newState = currentState === 'on' ? 'off' : 'on';
            const savedData = loadDeviceState(deviceId);

            switchElement.setAttribute('data-state', newState);
            switchElement.classList.toggle('active', newState === 'on');

            const brightness = savedData.brightness || 50;
            const speed = savedData.speed || 50;
            const startTime = newState === 'on' ? Date.now() : null;
            const timeOn = newState === 'on' ? 0 : calculateTimeOn(savedData.startTime);

            saveDeviceState(deviceId, newState, brightness, speed, startTime);
            updateStatusText(deviceId, newState, brightness, speed, timeOn);
        });
    });

    // Brightness and Speed Slider Logic
    brightnessSliders.forEach(slider => {
        slider.addEventListener('input', () => {
            const deviceId = slider.getAttribute('data-device');
            const brightness = slider.value;
            const savedData = loadDeviceState(deviceId);
            const timeOn = savedData.state === 'on' ? calculateTimeOn(savedData.startTime) : 0;

            saveDeviceState(deviceId, savedData.state, brightness, savedData.speed, savedData.startTime);
            updateStatusText(deviceId, savedData.state, brightness, savedData.speed, timeOn);
        });
    });

    speedSliders.forEach(slider => {
        slider.addEventListener('input', () => {
            const deviceId = slider.getAttribute('data-device');
            const speed = slider.value;
            const savedData = loadDeviceState(deviceId);
            const timeOn = savedData.state === 'on' ? calculateTimeOn(savedData.startTime) : 0;

            saveDeviceState(deviceId, savedData.state, savedData.brightness, speed, savedData.startTime);
            updateStatusText(deviceId, savedData.state, savedData.brightness, speed, timeOn);
        });
    });

    // Update time-on every minute
    setInterval(() => {
        toggleSwitches.forEach(switchElement => {
            const deviceId = switchElement.getAttribute('data-device');
            const savedData = loadDeviceState(deviceId);
            if (savedData.state === 'on' && savedData.startTime) {
                const timeOn = calculateTimeOn(savedData.startTime);
                updateStatusText(deviceId, savedData.state, savedData.brightness, savedData.speed, timeOn);
            }
        });
    }, 60000);

    // Switch to Map View
    let currentView = 'tab';
    switchToMapBtn.addEventListener('click', () => {
        if (currentView === 'tab') {
            tabContent.style.display = 'none';
            map2DContainer.style.display = 'block';
            initialize2DMap();
            currentView = '2d';
            switchToMapBtn.textContent = 'Chuyển sang Tab';
        } else {
            map2DContainer.style.display = 'none';
            tabContent.style.display = 'block';
            currentView = 'tab';
            switchToMapBtn.textContent = 'Chuyển sang bản đồ';
        }
    });

    // 2D Map Interaction
    const initialize2DMap = () => {
        const rooms = document.querySelectorAll('.room');
        rooms.forEach(room => {
            room.addEventListener('click', () => {
                const roomId = room.getAttribute('id');
                const devices = getDevicesForRoom(roomId);
                displayDeviceControls(roomId, devices);
            });
        });
    };

    const getDevicesForRoom = (roomId) => {
        const deviceMap = {
            'bedroom1': ['bedroom1-light'],
            'bedroom2': ['bedroom2-light'],
            'livingroom': ['livingroom-light'],
            'kitchen': ['kitchen-light'],
            'bathroom1': [],
            'bathroom2': []
        };
        return deviceMap[roomId] || [];
    };

    const displayDeviceControls = (roomId, devices) => {
        const modalTitle = document.getElementById('modal-title');
        const modalBody = document.getElementById('modal-body');
        modalTitle.textContent = `Điều khiển thiết bị - ${roomId.charAt(0).toUpperCase() + roomId.slice(1)}`;
        modalBody.innerHTML = devices.map(deviceId => {
            const savedData = loadDeviceState(deviceId);
            const iconClass = deviceId.includes('light') ? 'fa-lightbulb' : '';
            const colorClass = 'text-primary';
            let slider = '';
            if (deviceId.includes('light')) {
                slider = `<input type="range" class="form-range brightness-slider" min="0" max="100" value="${savedData.brightness}" data-device="${deviceId}" style="width: 150px;">`;
            }
            return `
                <div class="device-control mb-3">
                    <h5>${deviceId.replace(/-/g, ' ').replace(/\b\w/g, l => l.toUpperCase())}</h5>
                    <div class="d-flex align-items-center justify-content-center">
                        <i class="fas ${iconClass} fa-2x toggle-switch ${colorClass} me-3" data-device="${deviceId}" data-state="${savedData.state}"></i>
                        ${slider}
                    </div>
                    <p class="status-text mt-2">Trạng thái: <span class="state">${savedData.state === 'on' ? 'Bật' : 'Tắt'}</span> | 
                        ${savedData.brightness !== undefined ? `Độ sáng: <span class="brightness">${savedData.brightness}</span>% |` : ''} 
                        Thời gian bật: <span class="time-on">${savedData.state === 'on' && savedData.startTime ? calculateTimeOn(savedData.startTime) : 0}</span> phút</p>
                </div>
            `;
        }).join('');
        modal.show();
    };
});