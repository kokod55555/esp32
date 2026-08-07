// 📌 URL ของ Google Apps Script
const SCRIPT_URL = 'https://script.google.com/macros/s/AKfycbwkI2SxiLmQyqqVcyPMRp5FkvHZJFoV4nnjQc6NSY2vw2O2cSWoNoKOrqvJMODyKFc/exec';

let distanceChart;

// เริ่มต้นสร้าง Line Chart แบบ Dynamic Max Value
function initChart() {
    const ctx = document.getElementById('distanceChart').getContext('2d');
    distanceChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Distance (cm)',
                data: [],
                borderColor: '#38bdf8',
                backgroundColor: 'rgba(56, 189, 248, 0.1)',
                borderWidth: 3,
                fill: true,
                tension: 0.3,
                pointRadius: 4,
                pointBackgroundColor: '#38bdf8'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    max: null,
                    title: { display: true, text: 'Distance (cm)' }
                },
                x: {
                    title: { display: true, text: 'Time' }
                }
            },
            plugins: {
                legend: { display: false }
            }
        }
    });
}

// อัปเดตเข็ม Gauge
function updateGauge(distance) {
    const needle = document.getElementById('gauge-needle');
    let clampedDist = Math.max(0, Math.min(100, distance));
    let angle = 90 - (clampedDist / 100) * 180;
    needle.style.transform = `rotate(${angle}deg)`;
}

// อัปเดตสถานะ Alert
function updateAlertStatus(distance) {
    const alertCard = document.getElementById('alert-card');
    const alertBanner = document.getElementById('alert-banner');
    const alertText = document.getElementById('alert-text');
    const alertIcon = document.getElementById('alert-icon');

    if (distance < 20) {
        alertCard.style.background = '#fee2e2';
        alertBanner.style.background = 'var(--color-red)';
        alertText.innerText = 'STATUS: ALERT';
        alertIcon.innerText = '✖';
    } else if (distance >= 20 && distance < 50) {
        alertCard.style.background = '#fef9c3';
        alertBanner.style.background = 'var(--color-yellow)';
        alertText.innerText = 'STATUS: WARNING';
        alertIcon.innerText = '⚠️';
    } else {
        alertCard.style.background = '#dcfce7';
        alertBanner.style.background = 'var(--color-green)';
        alertText.innerText = 'STATUS: NORMAL';
        alertIcon.innerText = '✔';
    }
}

// ดึงข้อมูลจาก Google Sheets API
async function fetchSensorData() {
    try {
        const response = await fetch(SCRIPT_URL);
        const data = await response.json();

        if (data && data.length > 0) {
            const latest = data[data.length - 1];
            const distanceVal = parseFloat(latest.distance) || 0;

            document.getElementById('current-distance').innerText = `${distanceVal.toFixed(1)} cm`;
            document.getElementById('last-updated').innerText = `Last Updated: ${latest.timestamp || 'Just now'}`;

            updateGauge(distanceVal);
            updateAlertStatus(distanceVal);

            const recentData = data.slice(-10);
            distanceChart.data.labels = recentData.map(item => {
                return item.timestamp ? item.timestamp.split(' ')[1] || item.timestamp : '';
            });
            distanceChart.data.datasets[0].data = recentData.map(item => parseFloat(item.distance) || 0);
            distanceChart.update();
        }
    } catch (error) {
        console.error('Fetch Error:', error);
        document.getElementById('last-updated').innerText = 'Connection Error!';
    }
}

// เรียกทำงานเมื่อโหลดหน้าเว็บ
window.onload = () => {
    initChart();
    fetchSensorData();
    setInterval(fetchSensorData, 5000);
};
