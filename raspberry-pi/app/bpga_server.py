#!/usr/bin/env python3

import json
import serial
import threading
import time
from pathlib import Path
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

WEB_ROOT = (
    Path(__file__).resolve().parent.parent / "web"
)

INDEX_FILE = WEB_ROOT / "index.html"


def load_index_html():
    return INDEX_FILE.read_bytes()


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
            try:
                content = load_index_html()
            except OSError as error:
                self.send_bytes(
                    f"Interfaz no disponible: {error}".encode("utf-8"),
                    "text/plain; charset=utf-8",
                    status=503,
                )
                return

            self.send_bytes(
                content,
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
