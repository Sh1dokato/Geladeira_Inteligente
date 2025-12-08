const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Geladeira Inteligente</title>

<style>
body {
  font-family: Arial, sans-serif;
  background: #f0f0f0;
  padding: 20px;
}
.card {
  background: white;
  padding: 15px;
  border-radius: 10px;
  margin-bottom: 15px;
  box-shadow: 0 0 10px #ccc;
}
button {
  width: 100%;
  padding: 12px;
  margin-top: 10px;
  font-size: 18px;
  border: none;
  border-radius: 8px;
  cursor: pointer;
}
.btn-green { background: #4CAF50; color: white; }
.btn-red   { background: #E53935; color: white; }
.btn-blue  { background: #1E88E5; color: white; }
.text {
  font-size: 20px;
  font-weight: bold;
}
</style>

<script>
function desligarBuzzer() {
  fetch("/desligaBuzzer")
}

function trancar() {
  fetch("/trancar")
}

function destrancar() {
  fetch("/destrancar")
}

function atualizarStatus() {
  fetch("/status")
  .then(r => r.json())
  .then(data => {
    document.getElementById("temp").innerHTML = data.temp + "°C";
    document.getElementById("umid").innerHTML = data.umid + "%";
  })
}

setInterval(atualizarStatus, 2000); // atualiza a cada 2s

</script>

</head>
<body>

<h2>Geladeira Inteligente</h2>

<div class="card">
  <div class="text">Temperatura: <span id="temp">--</span></div>
  <div class="text">Umidade: <span id="umid">--</span></div>
</div>

<div class="card">
  <button class="btn-red" onclick="trancar()">Trancar Geladeira</button>
  <button class="btn-green" onclick="destrancar()">Destrancar Geladeira</button>
</div>

<div class="card">
  <button class="btn-blue" onclick="desligarBuzzer()">Desligar Buzzer</button>
</div>

</body>
</html>
)=====";
