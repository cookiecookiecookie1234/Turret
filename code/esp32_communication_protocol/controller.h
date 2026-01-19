#ifndef CONTROLLER_H
#define CONTROLLER_H

const char controllerHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: #8B7355;
            font-family: monospace;
            overflow: hidden;
            cursor: crosshair;
        }
        
        .eye-controller {
            text-align: center;
            color: white;
        }
        
        .canvas-container {
            position: relative;
            display: inline-block;
            margin: 20px;
        }
        
        #eyeCanvas {
            border: 3px solid #5C4033;
            border-radius: 15px;
            background: #D2B48C;
            box-shadow: 0 0 30px rgba(92, 64, 51, 0.4);
        }
        
        .info-panel {
            background: #A0826D;
            padding: 20px;
            border-radius: 10px;
            margin-top: 20px;
            display: inline-block;
            min-width: 300px;
        }
        
        .info-row {
            margin: 10px 0;
            font-size: 18px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .label {
            color: #3E2723;
            font-weight: bold;
        }
        
        .value {
            color: #3E2723;
            background: #D7CCC8;
            padding: 5px 15px;
            border-radius: 5px;
            min-width: 80px;
            text-align: center;
        }
        
        .instructions {
            margin-top: 20px;
            padding: 15px;
            background: #6F4E37;
            border-radius: 8px;
            font-size: 14px;
            color: #F5DEB3;
        }
        
        .crosshair {
            position: absolute;
            width: 20px;
            height: 20px;
            border: 2px solid #8B4513;
            border-radius: 50%;
            pointer-events: none;
            transform: translate(-50%, -50%);
            box-shadow: 0 0 10px rgba(139, 69, 19, 0.5);
        }
        
        .crosshair::before,
        .crosshair::after {
            content: '';
            position: absolute;
            background: #8B4513;
        }
        
        .crosshair::before {
            width: 2px;
            height: 12px;
            left: 50%;
            top: 50%;
            transform: translate(-50%, -50%);
        }
        
        .crosshair::after {
            width: 12px;
            height: 2px;
            left: 50%;
            top: 50%;
            transform: translate(-50%, -50%);
        }
        
        h1 {
            color: #3E2723;
            text-shadow: 0 0 10px rgba(62, 39, 35, 0.3);
        }
    </style>
</head>
<body>
    <div class="eye-controller">
        <h1>Taco!</h1>
        
        <div class="canvas-container">
            <canvas id="eyeCanvas" width="500" height="500"></canvas>
            <div class="crosshair" id="crosshair"></div>
        </div>
        
        <div class="info-panel">
            <div class="info-row">
                <span class="label">Eye X:</span>
                <span class="value" id="eyeX">0.00</span>
            </div>
            <div class="info-row">
                <span class="label">Eye Y:</span>
                <span class="value" id="eyeY">0.00</span>
            </div>
            <div class="info-row">
                <span class="label">Eyelid:</span>
                <span class="value" id="eyelid">1.00</span>
            </div>
            <div class="info-row">
                <span class="label">Status:</span>
                <span class="value" id="status">Active</span>
            </div>
        </div>
        
        <div class="instructions">
            <strong>Controls:</strong><br>
            Move mouse in canvas to control eye direction<br>
            Scroll wheel to control eyelid (0 = closed, 1 = open)<br>
            WASD to move Taco<br>
            Idle animation starts after 30 seconds
        </div>
    </div>
    
    <script>
        const robot = 'http://192.168.4.1'; // Change this to your ESP32 IP
        const canvas = document.getElementById('eyeCanvas');
        const ctx = canvas.getContext('2d');
        const crosshair = document.getElementById('crosshair');
        
        let eyeX = 0;
        let eyeY = 0;
        let eyelid = 1.0;
        let lastSendTime = 0;
        const sendInterval = 50; // Send updates every 50ms
        
        // WASD movement tracking
        let pressed = {};
        
        // Draw the eye visualization
        function drawEye() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            const centerX = canvas.width / 2;
            const centerY = canvas.height / 2;
            const eyeRadius = 150;
            const pupilRadius = 40;
            
            // Draw eye white
            ctx.fillStyle = '#ffffff';
            ctx.beginPath();
            ctx.arc(centerX, centerY, eyeRadius, 0, Math.PI * 2);
            ctx.fill();
            
            // Draw iris
            const irisX = centerX + eyeX * 60;
            const irisY = centerY + eyeY * 60;
            
            ctx.fillStyle = '#5a4002';
            ctx.beginPath();
            ctx.arc(irisX, irisY, pupilRadius + 20, 0, Math.PI * 2);
            ctx.fill();
            
            // Draw pupil
            ctx.fillStyle = '#000000';
            ctx.beginPath();
            ctx.arc(irisX, irisY, pupilRadius, 0, Math.PI * 2);
            ctx.fill();
            
            // Draw eyelid (partial circle based on eyelid value)
            if (eyelid < 1.0) {
                ctx.fillStyle = '#8B4513';
                ctx.beginPath();
                const eyelidHeight = eyeRadius * 2 * (1 - eyelid);
                ctx.arc(centerX, centerY - eyeRadius + eyelidHeight, eyeRadius, 0, Math.PI * 2);
                ctx.fill();
            }
            
            // Draw eye outline
            ctx.strokeStyle = '#5C4033';
            ctx.lineWidth = 3;
            ctx.beginPath();
            ctx.arc(centerX, centerY, eyeRadius, 0, Math.PI * 2);
            ctx.stroke();
        }
        
        // Update display values
        function updateDisplay() {
            document.getElementById('eyeX').textContent = eyeX.toFixed(2);
            document.getElementById('eyeY').textContent = eyeY.toFixed(2);
            document.getElementById('eyelid').textContent = eyelid.toFixed(2);
        }
        
        // Send data to ESP32
        function sendToRobot() {
            const now = Date.now();
            if (now - lastSendTime < sendInterval) return;
            lastSendTime = now;
            
            fetch(`${robot}/eye?x=${eyeX.toFixed(2)}&y=${eyeY.toFixed(2)}&l=${eyelid.toFixed(2)}`)
                .catch(err => console.error('Connection error:', err));
        }
        
        // ========== WASD Movement Functions ==========
        function sendSpeed(left, right) {
            fetch(`${robot}/move?l=${left}&r=${right}`)
                .catch(err => console.error('Connection error:', err));
        }
        
        function updateSpeed() {
            let left = 0, right = 0;
            
            if (pressed['w']) { // Forwards
                left = 1;
                right = 1;
            } else if (pressed['s']) { // Backward
                left = -1;
                right = -1;
            }
            
            // Default left/right
            if (pressed['a']) { // Left
                left = 0; 
                right = 1.0;
            } else if (pressed['d']) { // Right
                left = 0;
                right = -1.0;
            }
            
            sendSpeed(left, right);
        }
        
        // Keyboard event listeners
        document.addEventListener('keydown', (e) => {
            const key = e.key.toLowerCase();
            if (!pressed[key]) {
                pressed[key] = true;
                updateSpeed();
            }
        });
        
        document.addEventListener('keyup', (e) => {
            const key = e.key.toLowerCase();
            delete pressed[key];
            updateSpeed();
        });
        // =============================================
        
        // Mouse move handler
        canvas.addEventListener('mousemove', (e) => {
            const rect = canvas.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            
            // Convert to -1 to 1 range
            eyeX = ((x / canvas.width) * 2 - 1);
            eyeY = ((y / canvas.height) * 2 - 1);
            
            // Clamp values
            eyeX = Math.max(-1, Math.min(1, eyeX));
            eyeY = Math.max(-1, Math.min(1, eyeY));
            
            // Update crosshair position
            crosshair.style.left = x + 'px';
            crosshair.style.top = y + 'px';
            
            drawEye();
            updateDisplay();
            sendToRobot();
        });
        
        // Scroll wheel handler for eyelid
        canvas.addEventListener('wheel', (e) => {
            e.preventDefault();
            
            // Scroll down = close, scroll up = open
            const delta = e.deltaY > 0 ? -0.05 : 0.05;
            eyelid += 2.5 * delta;
            eyelid = Math.max(0, Math.min(1, eyelid));
            
            drawEye();
            updateDisplay();
            sendToRobot();
        });
        
        // Prevent default scroll on canvas
        canvas.addEventListener('wheel', (e) => e.preventDefault(), { passive: false });
        
        // Poll status
        setInterval(() => {
            fetch(`${robot}/status`)
                .then(res => res.json())
                .then(data => {
                    document.getElementById('status').textContent = data.idle ? 'Idle' : 'Active';
                })
                .catch(() => {
                    document.getElementById('status').textContent = 'Offline';
                });
        }, 2000);
        
        // Initial draw
        drawEye();
        updateDisplay();
    </script>
</body>
</html>
)rawliteral";

#endif
