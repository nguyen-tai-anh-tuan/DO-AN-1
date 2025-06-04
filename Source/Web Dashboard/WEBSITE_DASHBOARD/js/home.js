// Fetch weather data using OpenWeatherMap API
async function fetchWeather() {
    const apiKey = "f2efe50e9cf609d464074bb67140aab2"; // Replace with your OpenWeatherMap API key
    const city = "Ho Chi Minh"; // Default city, can be changed based on user location
    const url = `https://api.openweathermap.org/data/2.5/weather?q=${city}&appid=${apiKey}&units=metric&lang=vi`;

    try {
        const response = await fetch(url);
        const data = await response.json();
        document.getElementById("temperature").textContent = `${data.main.temp} °C`;
        document.getElementById("humidity").textContent = `${data.main.humidity} %`;
    } catch (error) {
        document.getElementById("temperature").textContent = "Không tải được";
        document.getElementById("humidity").textContent = "Không tải được";
        console.error("Error fetching weather:", error);
    }
}

// Quick demo functionality
function setupQuickDemo() {
    const lightToggle = document.getElementById("lightToggle");
    const lightStatus = document.getElementById("lightStatus");
    const fanToggle = document.getElementById("fanToggle");
    const fanStatus = document.getElementById("fanStatus");

    let lightOn = false;
    let fanOn = false;

    lightToggle.addEventListener("click", () => {
        lightOn = !lightOn;
        lightToggle.textContent = lightOn ? "Tắt" : "Bật";
        lightStatus.textContent = lightOn ? "Bật" : "Tắt";
        lightStatus.className = lightOn ? "on" : "off";
    });

    fanToggle.addEventListener("click", () => {
        fanOn = !fanOn;
        fanToggle.textContent = fanOn ? "Tắt" : "Bật";
        fanStatus.textContent = fanOn ? "Bật" : "Tắt";
        fanStatus.className = fanOn ? "on" : "off";
    });
}

// Initialize on page load
document.addEventListener("DOMContentLoaded", () => {
    fetchWeather();
    setupQuickDemo();
});