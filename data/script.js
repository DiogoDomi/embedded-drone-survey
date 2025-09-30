document.addEventListener("DOMContentLoaded", function() {

    const webSocket = new WebSocket("ws://192.168.4.200/ws");

    const DRONE_STATE = {
        0: "DISARMED",
        1: "ARMED"
    };

    const joystickState = {"lx": 0, "ly": 0, "rx": 0, "ry": 0};
    const joysticks = {};

    webSocket.onopen = () => console.log("WS Connection opened.");

    webSocket.onclose = () => {
        console.log("WS Connection closed.");
        setTimeout(() => window.location.reload(), 2000);
    };

    webSocket.onerror = (error) => console.log("WS error: ", error);

    webSocket.onmessage = (event) => {
        console.log("Received data: " + event.data);
        const myObj = JSON.parse(event.data);

        if (myObj.state !== undefined) {
            document.getElementById("state").innerHTML = DRONE_STATE[myObj.state];
        }
        if (myObj.rssi !== undefined) {
            document.getElementById("rssi").innerHTML = myObj.rssi;
        }
        if (myObj.lat !== undefined) {
            document.getElementById("lat").innerHTML = myObj.lat;
            if (myObj.lat > 0) {
                document.getElementById("latdir").innerHTML = " N ";
            }
            else if (myObj.lat < 0) {
                document.getElementById("latdir").innerHTML = " S ";
            }
        }
        if (myObj.lon !== undefined) {
            document.getElementById("lon").innerHTML = myObj.lon;
            if (myObj.lon > 0) {
                document.getElementById("londir").innerHTML = " E ";
            }
            else if (myObj.lon < 0) {
                document.getElementById("londir").innerHTML = " W ";
            }
        }
        if (myObj.alt !== undefined) {
            document.getElementById("alt").innerHTML = myObj.alt;
        }

        let joyDataUpdated = false;
        if (myObj.lx !== undefined) { joystickState.lx = myObj.lx; joyDataUpdated = true; }
        if (myObj.ly !== undefined) { joystickState.ly = myObj.ly; joyDataUpdated = true; }
        if (myObj.rx !== undefined) { joystickState.rx = myObj.rx; joyDataUpdated = true; }
        if (myObj.ry !== undefined) { joystickState.ry = myObj.ry; joyDataUpdated = true; }

        if (joyDataUpdated) {
            joysticks.left.sync();
            joysticks.right.sync();
        }
    };

    function createJoystick(canvasId, side) {
        const canvas = document.getElementById(canvasId);
        const ctx = canvas.getContext("2d");
        const center = { x: canvas.width / 2, y: canvas.height / 2 };
        let stick = { x: center.x, y: center.y };
        const RADIUS = 80;
        const NORM_MUL_DIV = 0.8;

        function draw() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);

            ctx.beginPath();
            ctx.arc(center.x, center.y, RADIUS, 0, Math.PI * 2);
            ctx.strokeStyle = "#888";
            ctx.lineWidth = 4;
            ctx.stroke();

            ctx.beginPath();
            ctx.arc(stick.x, stick.y, 20, 0, Math.PI * 2);
            ctx.fillStyle = "#0f0";
            ctx.fill();
        }

        function updateStick(e) {
            const rect = canvas.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            const dx = x - center.x;
            const dy = y - center.y;

            const dist = Math.min(Math.sqrt(dx * dx + dy * dy), RADIUS);
            const angle = Math.atan2(dy, dx);
            stick.x = center.x + Math.cos(angle) * dist;
            stick.y = center.y + Math.sin(angle) * dist;

            const normX = Math.round((stick.x - center.x) / NORM_MUL_DIV);
            const normY = Math.round((stick.y - center.y) / NORM_MUL_DIV);

            if (side === "left") {
                joystickState.lx = normX;
                joystickState.ly = -normY;
            } else {
                joystickState.rx = normX;
                joystickState.ry = -normY;
            }
            draw();
        }

        function resetStick() {
            if (side === "left") {
                stick.x = center.x;
                joystickState.lx = 0;
            } else {
                stick.x = center.x;
                stick.y = center.y;
                joystickState.rx = 0;
                joystickState.ry = 0;
            }
            draw();
        }

        function sync() {
            let normX, normY;

            if (side === "left") {
                normX = joystickState.lx;
                normY = -joystickState.ly;
            } else {
                normX = joystickState.rx;
                normY = -joystickState.ry;
            }
            stick.x = center.x + normX * NORM_MUL_DIV;
            stick.y = center.y + normY * NORM_MUL_DIV;
            draw();
        }

        canvas.addEventListener("pointerdown", (e) => {
            canvas.setPointerCapture(e.pointerId);
            updateStick(e);
        });

        canvas.addEventListener("pointermove", (e) => {
            if (e.buttons > 0) {
                updateStick(e);
            }
        });

        canvas.addEventListener("pointerup", (e) => {
            canvas.releasePointerCapture(e.pointerId);
            resetStick();
        });

        canvas.addEventListener("pointerleave", (e) => {
            if (e.buttons === 0) {
                resetStick();
            }
        });
        draw();
        return { sync: sync };
    }

    function sendRequest(data) {
        if (webSocket.readyState === WebSocket.OPEN) {
            webSocket.send(JSON.stringify(data));
        }
    }

    const stateBox = document.getElementById("stateBox");
    stateBox.addEventListener("click", () => {
        const command = { ...joystickState, "state": true };
        sendRequest(command);
    });

    setInterval(() => sendRequest(joystickState), 50);

    joysticks.left = createJoystick("ljoy", "left");
    joysticks.right = createJoystick("rjoy", "right");
}
);