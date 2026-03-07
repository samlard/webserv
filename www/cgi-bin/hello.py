#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgitb
cgitb.enable()


print()

print("""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Horloge Digitale</title>
<style>
body {
    margin: 0;
    padding: 0;
    background: linear-gradient(135deg, #1e1e2f, #3a3a5c);
    height: 100vh;
    display: flex;
    justify-content: center;
    align-items: center;
    font-family: 'Orbitron', sans-serif;
}
#clock {
    color: #00ffcc;
    font-size: 100px;
    letter-spacing: 8px;
    text-shadow: 0 0 20px #00ffcc, 0 0 30px #00ffcc, 0 0 40px #00ffcc;
}
</style>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@700&display=swap" rel="stylesheet">
</head>
<body>
<div id="clock">00:00:00</div>

<script>
function updateClock() {
    const now = new Date();
    const hours = now.getHours().toString().padStart(2,'0');
    const minutes = now.getMinutes().toString().padStart(2,'0');
    const seconds = now.getSeconds().toString().padStart(2,'0');
    document.getElementById('clock').textContent = hours + ':' + minutes + ':' + seconds;
}
updateClock();
setInterval(updateClock, 1000);
</script>
</body>
</html>
""")