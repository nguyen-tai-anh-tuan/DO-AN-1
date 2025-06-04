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

document.addEventListener('DOMContentLoaded', () => {
    // Chart.js for History
    let temperatureChartData = { labels: [], datasets: [{ label: 'Nhiệt độ (°C)', data: [], borderColor: '#4dabf7', fill: false }] };
    let humidityChartData = { labels: [], datasets: [{ label: 'Độ ẩm (%)', data: [], borderColor: '#51cf66', fill: false }] };
    let gasChartData = { labels: [], datasets: [{ label: 'Khí gas (ppm)', data: [], borderColor: '#ff922b', fill: false }] };
    let lightChartData = { labels: [], datasets: [{ label: 'Cường độ ánh sáng (Lux)', data: [], borderColor: '#ffd43b', fill: false }] };

    const tempCtx = document.getElementById('temperatureChart')?.getContext('2d');
    const humidCtx = document.getElementById('humidityChart')?.getContext('2d');
    const gasCtx = document.getElementById('gasChart')?.getContext('2d');
    const lightCtx = document.getElementById('lightChart')?.getContext('2d');

    let temperatureChart, humidityChart, gasChart, lightChart;
    if (tempCtx) temperatureChart = new Chart(tempCtx, { type: 'line', data: temperatureChartData, options: { responsive: true, scales: { y: { beginAtZero: true } } } });
    if (humidCtx) humidityChart = new Chart(humidCtx, { type: 'line', data: humidityChartData, options: { responsive: true, scales: { y: { beginAtZero: true } } } });
    if (gasCtx) gasChart = new Chart(gasCtx, { type: 'line', data: gasChartData, options: { responsive: true, scales: { y: { beginAtZero: true } } } });
    if (lightCtx) lightChart = new Chart(lightCtx, { type: 'line', data: lightChartData, options: { responsive: true, scales: { y: { beginAtZero: true } } } });

    // Filter Elements
    const daySelect = document.getElementById('day');
    const monthSelect = document.getElementById('month');
    const yearSelect = document.getElementById('year');
    const trendTypeSelect = document.getElementById('trendType');
    const filterBtn = document.getElementById('filterBtn');
    const historyTable = document.getElementById('historyTable');
    const controlHistoryTable = document.getElementById('controlHistoryTable');
    const alertHistoryTable = document.getElementById('alertHistoryTable');
    const refreshBtn = document.getElementById('refreshBtn');
    const clearOldBtn = document.getElementById('clearOldBtn');

    // Format timestamp to Vietnam time (UTC+07)
    function formatTimestamp(timestamp) {
        const date = new Date(timestamp * 1000);
        return date.toLocaleString('vi-VN', {
            year: 'numeric',
            month: '2-digit',
            day: '2-digit',
            hour: '2-digit',
            minute: '2-digit'
        }).replace(',', '');
    }

    // Load sensor history from Firebase (giới hạn 5 bản ghi gần nhất)
    function loadSensorHistory() {
        database.ref('sensors/history').orderByKey().limitToLast(5).on('value', (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const rows = [];
                const timestamps = [];
                const temps = [];
                const humidities = [];
                const gases = [];
                const lights = [];
                let isFirst = true;
                Object.entries(data).forEach(([timestamp, entry]) => {
                    timestamps.push(parseInt(timestamp));
                    temps.push(entry.temperature.toFixed(1));
                    humidities.push(entry.humidity.toFixed(1));
                    gases.push(entry.gas >= 3000 ? 3000 : 300);
                    lights.push(entry.light);
                    const formattedTime = formatTimestamp(timestamp);
                    const gasStatus = entry.gas >= 3000 ? 'Warning' : 'Normal';
                    const rowClass = isFirst ? 'new-record' : ''; // Thêm class cho bản ghi mới nhất
                    rows.push(`
                        <tr data-timestamp="${timestamp}" class="${rowClass}">
                            <td>${formattedTime}</td>
                            <td>${entry.temperature.toFixed(1)}</td>
                            <td>${entry.humidity.toFixed(1)}</td>
                            <td>${gasStatus}</td>
                            <td>${entry.light}</td>
                        </tr>
                    `);
                    isFirst = false;
                });
                historyTable.innerHTML = rows.sort((a, b) => b.match(/data-timestamp="(\d+)"/)[1] - a.match(/data-timestamp="(\d+)"/)[1]).join('');
                updateCharts(timestamps.map(t => formatTimestamp(t).split(' ')[1]), temps, humidities, gases, lights);
                historyTable.parentElement.scrollTop = historyTable.parentElement.scrollHeight;
                applyFilter();
            } else {
                historyTable.innerHTML = '<tr><td colspan="5">Không có dữ liệu</td></tr>';
                updateCharts([], [], [], [], []);
            }
        });
    }

    // Load control history from Firebase (giới hạn 5 bản ghi gần nhất)
    function loadControlHistory() {
        database.ref('control/history').orderByKey().limitToLast(5).on('value', (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const rows = [];
                let isFirst = true;
                Object.entries(data).forEach(([timestamp, entry]) => {
                    const formattedTime = formatTimestamp(timestamp);
                    const rowClass = isFirst ? 'new-record' : ''; // Thêm class cho bản ghi mới nhất
                    rows.push(`
                        <tr data-timestamp="${timestamp}" class="${rowClass}">
                            <td>${formattedTime}</td>
                            <td>${entry.device.charAt(0).toUpperCase() + entry.device.slice(1)}</td>
                            <td>${entry.action === 'on' ? 'Bật' : 'Tắt'}</td>
                            <td>${entry.user}</td>
                        </tr>
                    `);
                    isFirst = false;
                });
                controlHistoryTable.innerHTML = rows.sort((a, b) => b.match(/data-timestamp="(\d+)"/)[1] - a.match(/data-timestamp="(\d+)"/)[1]).join('');
                controlHistoryTable.parentElement.scrollTop = controlHistoryTable.parentElement.scrollHeight;
            } else {
                controlHistoryTable.innerHTML = '<tr><td colspan="4">Không có dữ liệu</td></tr>';
            }
        });
    }

    // Load alert history from Firebase (giới hạn 5 bản ghi gần nhất)
    function loadAlertHistory() {
        database.ref('alerts/history').orderByKey().limitToLast(5).on('value', (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const rows = [];
                let isFirst = true;
                Object.entries(data).forEach(([timestamp, entry]) => {
                    const formattedTime = formatTimestamp(timestamp);
                    const rowClass = isFirst ? 'new-record' : ''; // Thêm class cho bản ghi mới nhất
                    rows.push(`
                        <tr data-timestamp="${timestamp}" class="${rowClass}">
                            <td>${formattedTime}</td>
                            <td>${entry.type}</td>
                            <td>${entry.severity}</td>
                            <td>${entry.action}</td>
                        </tr>
                    `);
                    isFirst = false;
                });
                alertHistoryTable.innerHTML = rows.sort((a, b) => b.match(/data-timestamp="(\d+)"/)[1] - a.match(/data-timestamp="(\d+)"/)[1]).join('');
                alertHistoryTable.parentElement.scrollTop = alertHistoryTable.parentElement.scrollHeight;
            } else {
                alertHistoryTable.innerHTML = '<tr><td colspan="4">Không có dữ liệu</td></tr>';
            }
        });
    }

    // Apply filter based on selections
    function applyFilter() {
        const day = daySelect.value;
        const month = monthSelect.value;
        const year = yearSelect.value;

        const rows = historyTable.getElementsByTagName('tr');
        for (let row of rows) {
            const timestamp = parseInt(row.getAttribute('data-timestamp'));
            const date = new Date(timestamp * 1000);
            const rowYear = date.getFullYear().toString();
            const rowMonth = (date.getMonth() + 1).toString().padStart(2, '0');
            const rowDay = date.getDate().toString().padStart(2, '0');

            if ((year === '' || year === rowYear) &&
                (month === '' || month === rowMonth) &&
                (day === '' || day === rowDay)) {
                row.style.display = '';
            } else {
                row.style.display = 'none';
            }
        }
        updateChartBasedOnTrend();
    }

    // Filter Logic
    if (filterBtn) {
        filterBtn.addEventListener('click', applyFilter);
    }

    // Trend Logic
    if (trendTypeSelect) {
        trendTypeSelect.addEventListener('change', () => {
            updateChartBasedOnTrend();
        });
    }

    function updateCharts(labels, temps, humidities, gases, lights) {
        const trendType = trendTypeSelect.value;
        let newLabels = [...labels];
        let newTemps = [...temps];
        let newHumidities = [...humidities];
        let newGases = [...gases];
        let newLights = [...lights];

        if (trendType === 'weekly') {
            const weeks = {};
            labels.forEach((label, index) => {
                const timestamp = parseInt(historyTable.rows[index]?.getAttribute('data-timestamp') || Date.now() / 1000);
                const date = new Date(timestamp * 1000);
                const week = getWeekNumber(date);
                if (!weeks[week]) weeks[week] = { temp: [], humidity: [], gas: [], light: [] };
                weeks[week].temp.push(parseFloat(temps[index]));
                weeks[week].humidity.push(parseFloat(humidities[index]));
                weeks[week].gas.push(parseFloat(gases[index]));
                weeks[week].light.push(parseFloat(lights[index]));
            });
            newLabels = Object.keys(weeks).map(week => `Tuần ${week}`);
            newTemps = Object.values(weeks).map(data => data.temp.reduce((a, b) => a + b, 0) / data.temp.length || 0);
            newHumidities = Object.values(weeks).map(data => data.humidity.reduce((a, b) => a + b, 0) / data.humidity.length || 0);
            newGases = Object.values(weeks).map(data => data.gas.reduce((a, b) => a + b, 0) / data.gas.length || 0);
            newLights = Object.values(weeks).map(data => data.light.reduce((a, b) => a + b, 0) / data.light.length || 0);
        } else if (trendType === 'monthly') {
            const months = {};
            labels.forEach((label, index) => {
                const timestamp = parseInt(historyTable.rows[index]?.getAttribute('data-timestamp') || Date.now() / 1000);
                const date = new Date(timestamp * 1000);
                const monthKey = `${date.getFullYear()}-${(date.getMonth() + 1).toString().padStart(2, '0')}`;
                if (!months[monthKey]) months[monthKey] = { temp: [], humidity: [], gas: [], light: [] };
                months[monthKey].temp.push(parseFloat(temps[index]));
                months[monthKey].humidity.push(parseFloat(humidities[index]));
                months[monthKey].gas.push(parseFloat(gases[index]));
                months[monthKey].light.push(parseFloat(lights[index]));
            });
            newLabels = Object.keys(months);
            newTemps = Object.values(months).map(data => data.temp.reduce((a, b) => a + b, 0) / data.temp.length || 0);
            newHumidities = Object.values(months).map(data => data.humidity.reduce((a, b) => a + b, 0) / data.humidity.length || 0);
            newGases = Object.values(months).map(data => data.gas.reduce((a, b) => a + b, 0) / data.gas.length || 0);
            newLights = Object.values(months).map(data => data.light.reduce((a, b) => a + b, 0) / data.light.length || 0);
        }

        if (temperatureChart) {
            temperatureChart.data.labels = newLabels;
            temperatureChart.data.datasets[0].data = newTemps;
            temperatureChart.update();
            document.querySelector('.chart-container').classList.add('updated'); // Thêm class để kích hoạt hiệu ứng
        }
        if (humidityChart) {
            humidityChart.data.labels = newLabels;
            humidityChart.data.datasets[0].data = newHumidities;
            humidityChart.update();
            document.querySelector('.chart-container').classList.add('updated');
        }
        if (gasChart) {
            gasChart.data.labels = newLabels;
            gasChart.data.datasets[0].data = newGases;
            gasChart.update();
            document.querySelector('.chart-container').classList.add('updated');
        }
        if (lightChart) {
            lightChart.data.labels = newLabels;
            lightChart.data.datasets[0].data = newLights;
            lightChart.update();
            document.querySelector('.chart-container').classList.add('updated');
        }
    }

    // Hàm lấy số tuần trong năm
    function getWeekNumber(date) {
        date.setHours(0, 0, 0, 0);
        date.setDate(date.getDate() + 4 - (date.getDay() || 7));
        const yearStart = new Date(date.getFullYear(), 0, 1);
        return Math.ceil((((date - yearStart) / 86400000) + 1) / 7);
    }

    // Refresh Data Logic
    if (refreshBtn) {
        refreshBtn.addEventListener('click', () => {
            database.ref('sensors/history').remove();
            database.ref('control/history').remove();
            database.ref('alerts/history').remove();
            loadSensorHistory();
            loadControlHistory();
            loadAlertHistory();
        });
    }

    // Clear Old Data Logic (xóa dữ liệu trước 7 ngày)
    if (clearOldBtn) {
        clearOldBtn.addEventListener('click', () => {
            const sevenDaysAgo = Math.floor(Date.now() / 1000) - 7 * 24 * 60 * 60;
            const promises = [];

            database.ref('sensors/history').once('value', (snapshot) => {
                const data = snapshot.val();
                if (data) {
                    Object.keys(data).forEach((timestamp) => {
                        if (parseInt(timestamp) < sevenDaysAgo) {
                            promises.push(database.ref(`sensors/history/${timestamp}`).remove());
                        }
                    });
                }
            });

            database.ref('control/history').once('value', (snapshot) => {
                const data = snapshot.val();
                if (data) {
                    Object.keys(data).forEach((timestamp) => {
                        if (parseInt(timestamp) < sevenDaysAgo) {
                            promises.push(database.ref(`control/history/${timestamp}`).remove());
                        }
                    });
                }
            });

            database.ref('alerts/history').once('value', (snapshot) => {
                const data = snapshot.val();
                if (data) {
                    Object.keys(data).forEach((timestamp) => {
                        if (parseInt(timestamp) < sevenDaysAgo) {
                            promises.push(database.ref(`alerts/history/${timestamp}`).remove());
                        }
                    });
                }
            });

            Promise.all(promises).then(() => {
                alert('Đã xóa dữ liệu cũ hơn 7 ngày!');
                loadSensorHistory();
                loadControlHistory();
                loadAlertHistory();
            }).catch((error) => {
                console.error('Lỗi khi xóa dữ liệu cũ:', error);
                alert('Có lỗi xảy ra khi xóa dữ liệu cũ!');
            });
        });
    }

    // Export to Excel Logic
    const exportBtn = document.getElementById('exportBtn');
    if (exportBtn) {
        exportBtn.addEventListener('click', () => {
            const exportSensor = document.getElementById('exportSensor').checked;
            const exportControl = document.getElementById('exportControl').checked;
            const exportAlert = document.getElementById('exportAlert').checked;

            let csv = '';
            if (exportSensor) {
                const sensorRows = historyTable.getElementsByTagName('tr');
                csv += 'Thời gian,Nhiệt độ (°C),Độ ẩm (%),Khí gas,Cường độ ánh sáng (Lux)\n';
                for (let row of sensorRows) {
                    if (row.style.display !== 'none') {
                        const cols = row.getElementsByTagName('td');
                        const rowData = [];
                        for (let col of cols) rowData.push(`"${col.textContent}"`);
                        csv += rowData.join(',') + '\n';
                    }
                }
                csv += '\n';
            }
            if (exportControl) {
                const controlRows = controlHistoryTable.getElementsByTagName('tr');
                csv += 'Thời gian,Thiết bị,Hành động,Người thực hiện\n';
                for (let row of controlRows) {
                    if (row.style.display !== 'none') {
                        const cols = row.getElementsByTagName('td');
                        const rowData = [];
                        for (let col of cols) rowData.push(`"${col.textContent}"`);
                        csv += rowData.join(',') + '\n';
                    }
                }
                csv += '\n';
            }
            if (exportAlert) {
                const alertRows = alertHistoryTable.getElementsByTagName('tr');
                csv += 'Thời gian,Loại cảnh báo,Mức độ,Hành động\n';
                for (let row of alertRows) {
                    if (row.style.display !== 'none') {
                        const cols = row.getElementsByTagName('td');
                        const rowData = [];
                        for (let col of cols) rowData.push(`"${col.textContent}"`);
                        csv += rowData.join(',') + '\n';
                    }
                }
            }

            const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
            const link = document.createElement('a');
            const url = URL.createObjectURL(blob);
            link.setAttribute('href', url);
            link.setAttribute('download', 'smart_home_history.csv');
            link.style.visibility = 'hidden';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        });
    }

    // Load initial data
    loadSensorHistory();
    loadControlHistory();
    loadAlertHistory();
});