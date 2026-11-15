#!/usr/bin/env python3

import json
import serial
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

SERIAL_PORT = (
    "/dev/serial/by-id/"
    "usb-FTDI_FT232R_USB_UART_AL5SH6SC-if00-port0"
)

HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080
POLL_PERIOD_S = 0.2
SERIAL_TIMEOUT_S = 0.15

cache_lock = threading.Lock()

cache = {
    "online": False,
    "dataValid": False,
    "lastError": "Iniciando",
    "lastUpdate": None,
    "consecutiveErrors": 0,
    "measurement": {
        "sequence": 0,
        "systemState": 0,
        "systemSubstate": 0,
        "statusFlags": 0,
        "controlFlags": 0,
        "rpm": 0,
        "voltageMv": 0,
        "currentMa": 0,
        "powerW": 0,
        "sampleAgeMs": 0,
    },
}

HTML = r"""<!doctype html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BPGA Aeronáutico</title>
<style>
:root {
  color-scheme: dark;
  font-family: Arial, sans-serif;
}
* { box-sizing: border-box; }
html, body {
  width: 100%;
  height: 100%;
  margin: 0;
  overflow: hidden;
  background: #081521;
  color: #eef6fb;
}
body {
  display: grid;
  grid-template-rows: 16vh 1fr 13vh;
}
header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 2vh 3vw;
  background: #12344c;
  border-bottom: 3px solid #3d91c5;
}
.title h1 {
  margin: 0;
  font-size: clamp(22px, 2.2vw, 40px);
}
.title p {
  margin: .5vh 0 0;
  color: #b5d4e6;
  font-size: clamp(12px, 1vw, 18px);
}
.status {
  min-width: 220px;
  padding: 1.2vh 1.4vw;
  border-radius: 8px;
  text-align: center;
  font-size: clamp(15px, 1.2vw, 22px);
  font-weight: bold;
}
.online { background: #176b43; }
.offline { background: #9c2f2f; }
main {
  min-height: 0;
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 2vw;
  padding: 3vh 3vw;
}
.instrument {
  min-width: 0;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  background: #102b3e;
  border: 2px solid #386b88;
  border-radius: 14px;
}
.label {
  color: #a9c8d9;
  text-align: center;
  font-size: clamp(15px, 1.4vw, 25px);
}
.value {
  margin-top: 2vh;
  color: #f7fbfd;
  font-family: monospace;
  font-size: clamp(45px, 5vw, 95px);
  font-weight: bold;
}
.unit {
  margin-top: 1vh;
  color: #68c3f4;
  font-size: clamp(18px, 1.5vw, 28px);
}
footer {
  display: grid;
  grid-template-columns: 1.2fr 1.2fr 1fr 1.4fr;
  gap: 1vw;
  align-items: center;
  padding: 1.5vh 3vw;
  background: #102333;
  border-top: 2px solid #315d76;
}
.footer-item {
  min-width: 0;
}
.footer-label {
  color: #91afbf;
  font-size: clamp(11px, .9vw, 16px);
}
.footer-value {
  margin-top: .5vh;
  font-size: clamp(15px, 1.25vw, 23px);
  font-weight: bold;
}
.warning {
  color: #ffc766;
}
@media (max-width: 900px) {
  main {
    grid-template-columns: repeat(2, 1fr);
  }
  body {
    grid-template-rows: 17vh 1fr 16vh;
  }
}
</style>
</head>
<body>
<header>
  <div class="title">
    <h1>✈ Banco de Pruebas de Generadores Aeronáuticos</h1>
    <p>E.E.S.T. Nº7 «Taller Regional Quilmes» · IMPA</p>
  </div>
  <div id="connection" class="status offline">SIN COMUNICACIÓN</div>
</header>

<main>
  <section class="instrument">
    <div class="label">VELOCIDAD</div>
    <div id="rpm" class="value">0</div>
    <div class="unit">rpm</div>
  </section>

  <section class="instrument">
    <div class="label">TENSIÓN DE SALIDA</div>
    <div id="voltage" class="value">0.0</div>
    <div class="unit">V</div>
  </section>

  <section class="instrument">
    <div class="label">CORRIENTE DE SALIDA</div>
    <div id="current" class="value">0.00</div>
    <div class="unit">A</div>
  </section>

  <section class="instrument">
    <div class="label">POTENCIA</div>
    <div id="power" class="value">0</div>
    <div class="unit">W</div>
  </section>
</main>

<footer>
  <div class="footer-item">
    <div class="footer-label">ESTADO</div>
    <div id="state" class="footer-value">Inicializando</div>
  </div>

  <div class="footer-item">
    <div class="footer-label">FASE OPERATIVA</div>
    <div id="phase" class="footer-value">Detenido</div>
  </div>

  <div class="footer-item">
    <div class="footer-label">EDAD DE MUESTRA</div>
    <div id="age" class="footer-value">0 ms</div>
  </div>

  <div class="footer-item">
    <div class="footer-label">SEGURIDAD</div>
    <div class="footer-value warning">
      Motor serie siempre acoplado
    </div>
  </div>
</footer>

<script>
const STATES = {
  0: "Inicializando",
  1: "Parada segura",
  2: "Listo"
};

const PHASES = {
  0: "Detenido",
  1: "Acelerando",
  2: "Sin carga eléctrica",
  3: "Aplicando carga",
  4: "Estable",
  5: "Retirando carga",
  6: "Desacelerando"
};

function text(id, value) {
  document.getElementById(id).textContent = value;
}

async function update() {
  const indicator = document.getElementById("connection");

  try {
    const response = await fetch("/api/status", { cache: "no-store" });
    const data = await response.json();
    const measurement = data.measurement || {};

    indicator.textContent =
      data.online && data.dataValid
        ? "BANCO CONECTADO"
        : "DATOS INCOMPLETOS";

    indicator.className =
      data.online && data.dataValid
        ? "status online"
        : "status offline";

    text("rpm", measurement.rpm ?? 0);
    text("voltage", ((measurement.voltageMv ?? 0) / 1000).toFixed(1));
    text("current", ((measurement.currentMa ?? 0) / 1000).toFixed(2));
    text("power", measurement.powerW ?? 0);
    text("age", `${measurement.sampleAgeMs ?? 0} ms`);

    const state = measurement.systemState;
    const phase = measurement.systemSubstate;

    text("state", STATES[state] ?? `Desconocido (${state})`);
    text("phase", PHASES[phase] ?? `Desconocido (${phase})`);
  } catch (error) {
    indicator.textContent = "SIN COMUNICACIÓN";
    indicator.className = "status offline";
  } finally {
    setTimeout(update, 200);
  }
}

update();
</script>
</body>
</html>
"""


def crc16_modbus(data):
    crc = 0xFFFF

    for byte in data:
        crc ^= byte

        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1

    return crc


def build_request(address, count):
    frame = bytes([
        1,
        4,
        (address >> 8) & 0xFF,
        address & 0xFF,
        (count >> 8) & 0xFF,
        count & 0xFF,
    ])

    crc = crc16_modbus(frame)

    return frame + bytes([
        crc & 0xFF,
        (crc >> 8) & 0xFF,
    ])


def parse_snapshot(response):
    if len(response) != 69:
        raise ValueError(f"Longitud incorrecta: {len(response)}")

    if response[0:3] != bytes([1, 4, 64]):
        raise ValueError("Cabecera Modbus incorrecta")

    received_crc = int.from_bytes(response[-2:], "little")
    calculated_crc = crc16_modbus(response[:-2])

    if received_crc != calculated_crc:
        raise ValueError("CRC incorrecto")

    raw = response[3:-2]

    registers = [
        int.from_bytes(raw[index:index + 2], "big")
        for index in range(0, 64, 2)
    ]

    sequence_begin = registers[0]
    sequence_end = registers[31]

    if sequence_begin != sequence_end:
        raise ValueError("Snapshot inconsistente")

    if sequence_begin & 1:
        raise ValueError("Snapshot en actualización")

    def u32(index):
        return (
            (registers[index] << 16)
            | registers[index + 1]
        )

    return {
        "sequence": sequence_begin >> 1,
        "systemState": registers[1],
        "systemSubstate": registers[2],
        "statusFlags": registers[3],
        "controlFlags": registers[4],
        "rpm": registers[17],
        "voltageMv": u32(18),
        "currentMa": u32(20),
        "powerW": u32(22),
        "sampleAgeMs": u32(28),
    }


def modbus_worker():
    global cache

    request = build_request(0x0010, 32)

    while True:
        try:
            with serial.Serial(
                SERIAL_PORT,
                baudrate=19200,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_EVEN,
                stopbits=serial.STOPBITS_ONE,
                timeout=SERIAL_TIMEOUT_S,
                rtscts=False,
                dsrdtr=False,
            ) as port:
                next_cycle = time.monotonic()

                while True:
                    now = time.monotonic()

                    if now < next_cycle:
                        time.sleep(next_cycle - now)

                    started = time.monotonic()

                    port.reset_input_buffer()
                    port.write(request)
                    port.flush()

                    response = port.read(69)
                    measurement = parse_snapshot(response)

                    with cache_lock:
                        cache["online"] = True
                        cache["dataValid"] = True
                        cache["lastError"] = None
                        cache["lastUpdate"] = time.time()
                        cache["consecutiveErrors"] = 0
                        cache["measurement"] = measurement

                    next_cycle += POLL_PERIOD_S

                    if next_cycle < started:
                        next_cycle = time.monotonic()

        except Exception as error:
            with cache_lock:
                cache["online"] = False
                cache["dataValid"] = False
                cache["lastError"] = str(error)
                cache["consecutiveErrors"] += 1

            time.sleep(1.0)


class Handler(BaseHTTPRequestHandler):
    def send_bytes(self, content, content_type, status=200):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(content)

    def do_GET(self):
        if self.path.startswith("/api/status"):
            with cache_lock:
                payload = json.dumps(
                    cache,
                    ensure_ascii=False,
                ).encode("utf-8")

            self.send_bytes(
                payload,
                "application/json; charset=utf-8",
            )
            return

        if self.path == "/" or self.path.startswith("/index"):
            self.send_bytes(
                HTML.encode("utf-8"),
                "text/html; charset=utf-8",
            )
            return

        self.send_bytes(
            b"Not Found",
            "text/plain; charset=utf-8",
            status=404,
        )

    def log_message(self, format, *args):
        return


threading.Thread(
    target=modbus_worker,
    daemon=True,
).start()

server = ThreadingHTTPServer(
    (HTTP_HOST, HTTP_PORT),
    Handler,
)

print(f"BPGA disponible en http://{HTTP_HOST}:{HTTP_PORT}")
server.serve_forever()
