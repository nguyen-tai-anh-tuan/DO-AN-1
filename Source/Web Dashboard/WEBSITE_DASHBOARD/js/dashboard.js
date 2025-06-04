// Firebase configuration
const firebaseConfig = {
    apiKey: "AIzaSyAwNLS7RHasQrILtUGDW6BiNZqB5u9U5WY",
    authDomain: "smart-home-d6e89.firebaseapp.com",
    databaseURL: "https://smart-home-d6e89-default-rtdb.firebaseio.com",
    projectId: "smart-home-d6e89",
    storageBucket: "smart-home-d6e89.appspot.com",
    messagingSenderId: "YOUR_SENDER_ID",
    appId: "YOUR_APP_ID"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const database = firebase.database();

// Chart.js setup for each sensor
let tempChart, humidityChart, gasChart, lightChart;

// Temperature Chart
const tempCtx = document.getElementById('tempChart')?.getContext('2d');
if (tempCtx) {
    tempChart = new Chart(tempCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Nhiệt độ (°C)',
                data: [],
                borderColor: '#4dabf7',
                fill: false
            }]
        },
        options: {
            responsive: true,
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}

// Humidity Chart
const humidityCtx = document.getElementById('humidityChart')?.getContext('2d');
if (humidityCtx) {
    humidityChart = new Chart(humidityCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Độ ẩm (%)',
                data: [],
                borderColor: '#51cf66',
                fill: false
            }]
        },
        options: {
            responsive: true,
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}

// Gas Chart
const gasCtx = document.getElementById('gasChart')?.getContext('2d');
if (gasCtx) {
    gasChart = new Chart(gasCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Khí gas (ppm)',
                data: [],
                borderColor: '#ff922b',
                fill: false
            }]
        },
        options: {
            responsive: true,
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}

// Light Chart
const lightCtx = document.getElementById('lightChart')?.getContext('2d');
if (lightCtx) {
    lightChart = new Chart(lightCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Ánh sáng (Lux)',
                data: [],
                borderColor: '#ffd43b',
                fill: false
            }]
        },
        options: {
            responsive: true,
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
}

// Arrays to store sensor data for the charts (limit to last 10 entries)
const chartData = {
    labels: [],
    temperature: [],
    humidity: [],
    gas: [],
    light: []
};

// Function to format timestamp to HH:mm with +07 offset
function formatTime(timestamp) {
    // Convert timestamp to Date object (assuming UTC), then adjust for +07
    const date = new Date((timestamp * 1000) + (7 * 60 * 60 * 1000)); // Add 7 hours
    return date.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
}

// Function to update sensor data and charts
function updateSensorData() {
    database.ref('sensors/current').on('value', (snapshot) => {
        const data = snapshot.val();
        if (data) {
            // Log raw data for debugging
            console.log('Raw sensor data from Firebase:', data);

            // Ensure all expected fields exist and are numbers
            const temperature = typeof data.temperature === 'number' ? data.temperature : 0;
            const humidity = typeof data.humidity === 'number' ? data.humidity : 0;
            const gas = typeof data.gas === 'number' ? data.gas : 0;
            const light = typeof data.light === 'number' ? data.light : 0;
            const timestamp = data.timestamp || Math.floor(Date.now() / 1000); // Use current time if no timestamp

            // Update sensor card values
            document.getElementById('temperature').textContent = `${temperature.toFixed(1)} °C`;
            document.getElementById('humidity').textContent = `${humidity.toFixed(1)} %`;
            document.getElementById('gas').innerHTML = `${gas} ppm <span style="color: ${gas >= 3000 ? '#ff6b6b' : '#00bcd4'}">${gas >= 3000 ? 'Warning' : 'Normal'}</span>`;
            document.getElementById('light').textContent = `${light} Lux`;

            // Update chart data
            const timeLabel = formatTime(timestamp);
            chartData.labels.push(timeLabel);
            chartData.temperature.push(temperature);
            chartData.humidity.push(humidity);
            chartData.gas.push(gas);
            chartData.light.push(light);

            // Limit to last 10 entries
            if (chartData.labels.length > 10) {
                chartData.labels.shift();
                chartData.temperature.shift();
                chartData.humidity.shift();
                chartData.gas.shift();
                chartData.light.shift();
            }

            // Update each chart
            if (tempChart) {
                tempChart.data.labels = chartData.labels;
                tempChart.data.datasets[0].data = chartData.temperature;
                tempChart.update();
            }
            if (humidityChart) {
                humidityChart.data.labels = chartData.labels;
                humidityChart.data.datasets[0].data = chartData.humidity;
                humidityChart.update();
            }
            if (gasChart) {
                gasChart.data.labels = chartData.labels;
                gasChart.data.datasets[0].data = chartData.gas;
                gasChart.update();
            }
            if (lightChart) {
                lightChart.data.labels = chartData.labels;
                lightChart.data.datasets[0].data = chartData.light;
                lightChart.update();
            }
        } else {
            console.error('No sensor data found in Firebase.');
        }
    }, (error) => {
        console.error('Error fetching sensor data:', error);
    });
}

// Function to update device control states
function updateDeviceStates() {
    database.ref('RL').on('value', (snapshot) => {
        const data = snapshot.val();
        if (data) {
            const roomMap = {
                'bedroom': 'RL1',
                'kitchen': 'RL2',
                'livingroom': 'RL3',
                'garden': 'RL4'
            };
            
            Object.keys(roomMap).forEach((room) => {
                const relay = roomMap[room];
                const state = data[relay] === 1 ? 'on' : 'off';
                const switchElement = document.querySelector(`.toggle-switch[data-room="${room}"]`);
                switchElement.setAttribute('data-state', state);
                switchElement.classList.toggle('active', state === 'on');
            });
        }
    }, (error) => {
        console.error('Error fetching relay states:', error);
    });
}

// Function to toggle device state
// Function to toggle device state
function toggleDevice(room, action) {
    const relayMap = {
        'bedroom': 'RL1',
        'kitchen': 'RL2',
        'livingroom': 'RL3',
        'garden': 'RL4'
    };
    const relay = relayMap[room];
    const newValue = action === 'on' ? 1 : 0;

    // Cập nhật trạng thái relay
    database.ref(`RL/${relay}`).set(newValue, (error) => {
        if (error) {
            console.error(`Error updating ${relay}:`, error);
        } else {
            // Lưu lịch sử điều khiển
            const timestamp = Math.floor(Date.now() / 1000);
            const controlData = {
                timestamp: timestamp,
                device: room,
                action: action,
                user: "Nguyen T.A.Tuan" // Có thể thay đổi nếu có hệ thống đăng nhập
            };
            database.ref(`control/history/${timestamp}`).set(controlData, (err) => {
                if (err) {
                    console.error('Error saving control history:', err);
                } else {
                    console.log(`Control history saved: ${room} - ${action}`);
                }
            });
        }
    });
}

// Device control logic
document.addEventListener('DOMContentLoaded', () => {
    const toggleSwitches = document.querySelectorAll('.toggle-switch');
    const controlButtons = document.querySelectorAll('.control-btn');

    // Handle button clicks
    controlButtons.forEach(button => {
        button.addEventListener('click', () => {
            const room = button.getAttribute('data-room');
            const action = button.getAttribute('data-action');
            toggleDevice(room, action);
        });
    });

    // Optional: Keep toggle switch clickable
    toggleSwitches.forEach(switchElement => {
        switchElement.addEventListener('click', () => {
            const room = switchElement.getAttribute('data-room');
            const currentState = switchElement.getAttribute('data-state');
            const newAction = currentState === 'on' ? 'off' : 'on';
            toggleDevice(room, newAction);
        });
    });

    // Initial data load
    updateSensorData();
    updateDeviceStates();
});