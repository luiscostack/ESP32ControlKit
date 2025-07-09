let websocket;
let chart;
const statusDiv = document.getElementById('status');
const MAX_DATA_POINTS = 100;

// Função para inicializar o gráfico
function initChart() {
    const ctx = document.getElementById('pidChart').getContext('2d');
    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Setpoint (V)',
                borderColor: 'rgb(255, 99, 132)',
                backgroundColor: 'rgba(255, 99, 132, 0.5)',
                data: [],
                borderWidth: 2,
            }, {
                label: 'Saída da Planta (V)',
                borderColor: 'rgb(54, 162, 235)',
                backgroundColor: 'rgba(54, 162, 235, 0.5)',
                data: [],
                borderWidth: 2,
            }]
        },
        options: {
            animation: false,
            scales: {
                x: {
                    type: 'linear',
                    title: { display: true, text: 'Tempo (s)' }
                },
                y: {
                    title: { display: true, text: 'Tensão (V)' },
                    suggestedMin: 0,
                    suggestedMax: 3.3
                }
            },
            maintainAspectRatio: false
        }
    });
}

// Função para adicionar dados ao gráfico
function addDataToChart(time, sp, y) {
    if (!chart) return;

    // Remove o ponto mais antigo se o limite for atingido
    if (chart.data.labels.length > MAX_DATA_POINTS) {
        chart.data.labels.shift();
        chart.data.datasets.forEach((dataset) => {
            dataset.data.shift();
        });
    }

    // Adiciona os novos pontos
    chart.data.labels.push(time / 1000); // Converte ms para s
    chart.data.datasets[0].data.push(sp);
    chart.data.datasets[1].data.push(y);

    chart.update();
}


function connectWebSocket() {
    const ip = document.getElementById('esp32_ip').value;
    if (!ip) {
        alert("Por favor, insira o endereço IP do ESP32.");
        return;
    }
    const wsUri = `ws://${ip}/ws`;
    
    // Fecha conexão anterior se existir
    if (websocket) {
        websocket.close();
    }

    websocket = new WebSocket(wsUri);

    websocket.onopen = (event) => {
        statusDiv.textContent = "Conectado";
        statusDiv.className = "connected";
    };

    websocket.onclose = (event) => {
        statusDiv.textContent = "Desconectado";
        statusDiv.className = "disconnected";
    };

    // Manipulador para receber mensagens do ESP32
    websocket.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            // Adiciona os dados recebidos ao gráfico
            if (data.time !== undefined && data.sp_v !== undefined && data.y_v !== undefined) {
                addDataToChart(data.time, data.sp_v, data.y_v);
            }
        } catch (e) {
            console.error("Erro ao interpretar JSON recebido:", e);
        }
    };

    websocket.onerror = (event) => {
        console.error("Erro no WebSocket: ", event);
        statusDiv.textContent = "Erro na conexão";
        statusDiv.className = "disconnected";
    };
}

function sendData() {
    if (!websocket || websocket.readyState !== WebSocket.OPEN) {
        alert("WebSocket não está conectado. Por favor, conecte-se primeiro.");
        return;
    }

    const data = {
        kp: parseFloat(document.getElementById('kp').value),
        ki: parseFloat(document.getElementById('ki').value),
        kd: parseFloat(document.getElementById('kd').value),
        setpoint: parseInt(document.getElementById('setpoint').value, 10),
        planta: parseInt(document.querySelector('input[name="planta"]:checked').value, 10),
        combinacao: parseInt(document.getElementById('combinacao').value, 10)
    };
    
    const jsonString = JSON.stringify(data);
    websocket.send(jsonString);
}

// Inicializa o gráfico quando a página carrega
window.onload = initChart;