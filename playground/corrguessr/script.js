document.addEventListener("DOMContentLoaded", () => {
    const ctx = document.getElementById("scatterPlot").getContext("2d");
    let chartInstance; // Chart.js Instance
    let trueCorr; // true correlation coefficient <-> target
    let round = 1;
    let totalError = 0;
    const maxRounds = 5;

    // 散布図のデータを生成
    function generateScatterData() {
        const target = Math.round((Math.random() * 2 - 1) * 100) / 100; // -1 から 1 の相関係数
        const x = Array.from({ length: 50 }, () => Math.random()); // x は一様分布のまま
        const e = Array.from({ length: 50 }, () => Math.random()); // e も一様分布
    
        // 平均と分散を調整して標準化
        const mean = arr => arr.reduce((sum, v) => sum + v, 0) / arr.length;
        const stdDev = arr => Math.sqrt(arr.reduce((sum, v) => sum + (v - mean(arr)) ** 2, 0) / arr.length);
        const normalize = arr => {
            const mu = mean(arr);
            const sigma = stdDev(arr);
            return arr.map(v => (v - mu) / sigma);
        };
    
        const xNorm = normalize(x);
        const eNorm = normalize(e);
    
        // e' の調整（x と無相関にする）
        const ep = (((x, e) => {
            const Sx = x.reduce((sum, xi) => sum + xi, 0);
            const Se = e.reduce((sum, ei) => sum + ei, 0);
            const Sxe = x.reduce((sum, xi, i) => sum + xi * e[i], 0);
            const Sxx = x.reduce((sum, xi) => sum + xi * xi, 0);
            const N = x.length;
            return x.map((xi, i) => e[i] - xi * (N * Sxe - Sx * Se) / (N * Sxx - Sx * Sx));
        })(xNorm, eNorm));
    
        // y を生成
        const y = xNorm.map((xi, i) => target * xi + Math.sqrt(1 - target ** 2) * ep[i]);
        trueCorr = correlationCoefficient(xNorm, y).toFixed(2);
        return { x: xNorm, y};
    }
    
    function correlationCoefficient(x, y) {
        const N = x.length;
        const Sx = x.reduce((sum, xi) => sum + xi, 0);
        const Sy = y.reduce((sum, yi) => sum + yi, 0);
        const Sxy = x.reduce((sum, xi, i) => sum + xi * y[i], 0);
        const Sxx = x.reduce((sum, xi) => sum + xi ** 2, 0);
        const Syy = y.reduce((sum, yi) => sum + yi ** 2, 0);
        return (N * Sxy - Sx * Sy) / Math.sqrt((N * Sxx - Sx * Sx) * (N * Syy - Sy * Sy));
    }
    

    //draw chart
    function drawScatterPlot(data) {
        if (chartInstance) chartInstance.destroy();
        chartInstance = new Chart(ctx, {
            type: "scatter",
            data: {
                datasets: [{
                    label: "Scatter Data",
                    data: data.x.map((xi, i) => ({ x: xi, y: data.y[i] })),
                    backgroundColor: "rgba(75, 192, 192, 0.6)"
                }]
            },
            options: {
                scales: {
                    x: { type: "linear", position: "bottom" },
                    y: { type: "linear" }
                }
            }
        });
    }

    //initialize
    function initializeGame() {
        round = 1;
        totalError = 0;
        document.getElementById("round-number").textContent = round;
        //document.getElementById("result").textContent = "";
        document.getElementById("final-score").style.display = "none";
        document.getElementById("replay").style.display = "none";
        document.getElementById("submit-btn").disabled = false;
        document.getElementById("next-btn").disabled = true;
        document.getElementById("guess").value = "";
        document.getElementById("results-list").innerHTML = `
            <thead>
                <tr>
                    <th scope="col">Round</th>
                    <th scope="col">相関係数</th>
                    <th scope="col">あなたの予想</th>
                    <th scope="col">誤差</th>
                </tr>
            </thead>
        `;
        startRound();
    }

    //start new round
    function startRound() {
        const data = generateScatterData();
        drawScatterPlot(data);
    }

    //check answer
    document.getElementById("guess-form").onsubmit = (e) => {
        e.preventDefault();
        const guess = parseFloat(document.getElementById("guess").value);
        if (isNaN(guess) || guess < -1 || guess > 1) {
            document.getElementById("result").textContent = "-1 以上 1 以下の値を入れてね";
            return;
        }
        const error = Math.abs(trueCorr - guess);
        totalError += error;
        const result = document.createElement("tr");
        result.innerHTML = `
            <td scope = "row">${round}</td>
            <td>${trueCorr}</td>
            <td>${guess}</td>
            <td>${error.toFixed(2)}</td>
        `;
        const resultlist = document.getElementById("results-list");
        resultlist.appendChild(result);
        if (round == maxRounds) {
            endGame();
        }
        else {
            document.getElementById("next-btn").disabled = false;
        }
        document.getElementById("submit-btn").disabled = true;
    };

    document.getElementById("next-btn").onclick = () => {
        if (round < maxRounds) {
            round++;
            document.getElementById("round-number").textContent = round;
            startRound();
        }
        document.getElementById("submit-btn").disabled = false;
        document.getElementById("next-btn").disabled = true;
        document.getElementById("guess").value = "";
    };

    //end game
    function endGame() {
        document.getElementById("final-score").style.display = "block";
        document.getElementById("score").textContent = totalError.toFixed(2);
        document.getElementById("replay").style.display = "inline";
        document.getElementById("submit-btn").disabled = true;
    }

    //replay
    document.getElementById("replay").onclick = initializeGame;

    //initialize
    initializeGame();
});