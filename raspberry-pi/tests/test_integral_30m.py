#!/usr/bin/env python3

import csv
import json
import statistics
import subprocess
import time
from datetime import datetime
from pathlib import Path
from urllib.request import urlopen

DURATION_S = 30 * 60
INTERVAL_S = 1.0
HEALTH_INTERVAL_S = 10.0

BASE_URL = "http" + "://127.0.0.1:8080"
STAMP = datetime.now().strftime("%Y%m%d_%H%M%S")

OUTPUT_DIR = Path.home() / "bpga" / "test-results"
CSV_PATH = OUTPUT_DIR / f"integral_30m_{STAMP}.csv"
SUMMARY_PATH = OUTPUT_DIR / f"integral_30m_{STAMP}.txt"

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def command(*args):
    result = subprocess.run(
        args,
        text=True,
        capture_output=True,
        timeout=5,
        check=False,
    )
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def process_present(name):
    code, output, _ = command("pgrep", "-f", name)
    return code == 0 and bool(output)


def read_throttled():
    code, output, error = command("vcgencmd", "get_throttled")

    if code != 0 or "=" not in output:
        raise RuntimeError(error or output or "get_throttled sin respuesta")

    value = int(output.split("=", 1)[1], 16)

    return value, value & 0xF


def read_temperature():
    code, output, error = command("vcgencmd", "measure_temp")

    if code != 0 or "=" not in output:
        raise RuntimeError(error or output or "measure_temp sin respuesta")

    value = output.split("=", 1)[1]
    value = value.replace("'C", "")

    return float(value)


def read_api():
    started = time.monotonic()

    with urlopen(BASE_URL + "/api/status", timeout=3) as response:
        payload = json.load(response)
        status = response.status

    latency_ms = (time.monotonic() - started) * 1000.0

    return status, latency_ms, payload


def read_web():
    started = time.monotonic()

    with urlopen(BASE_URL + "/", timeout=3) as response:
        content = response.read()
        status = response.status

    latency_ms = (time.monotonic() - started) * 1000.0

    valid = (
        status == 200
        and b'data-screen="monitor"' in content
        and b'data-screen="diagnostics"' in content
        and b'data-screen="architecture"' in content
        and b'data-screen="results"' in content
    )

    return status, latency_ms, valid


service_failures = 0
api_failures = 0
web_failures = 0
invalid_data = 0
sequence_repeats = 0
sequence_regressions = 0
sequence_wraps = 0
active_power_events = 0
xorg_failures = 0
openbox_failures = 0
chromium_failures = 0

api_latencies = []
web_latencies = []
temperatures = []
sample_ages = []

previous_sequence = None
first_sequence = None
last_sequence = None
last_health_check = 0.0

baseline_mask = None
final_mask = None

rows = 0
start_monotonic = time.monotonic()
next_sample = start_monotonic

fieldnames = [
    "elapsed_s",
    "timestamp",
    "service_active",
    "api_http",
    "api_latency_ms",
    "online",
    "data_valid",
    "consecutive_errors",
    "last_error",
    "sequence",
    "sequence_delta",
    "sample_age_ms",
    "rpm",
    "voltage_mV",
    "current_mA",
    "power_W",
    "web_http",
    "web_latency_ms",
    "web_valid",
    "temperature_C",
    "throttled_mask",
    "active_flags",
    "xorg",
    "openbox",
    "chromium",
]

print("=== BPGA: PRUEBA INTEGRAL DE 30 MINUTOS ===")
print(f"CSV:     {CSV_PATH}")
print(f"Resumen: {SUMMARY_PATH}")
print("No cerrar esta terminal.")
print()

with CSV_PATH.open("w", newline="", encoding="utf-8") as csv_file:
    writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
    writer.writeheader()

    while True:
        now = time.monotonic()
        elapsed = now - start_monotonic

        if elapsed >= DURATION_S:
            break

        if now < next_sample:
            time.sleep(next_sample - now)

        cycle_started = time.monotonic()
        elapsed = cycle_started - start_monotonic

        service_active = False
        api_http = None
        api_latency_ms = None
        online = False
        data_valid = False
        consecutive_errors = None
        last_error = None
        sequence = None
        sequence_delta = None
        sample_age_ms = None
        rpm = None
        voltage_mv = None
        current_ma = None
        power_w = None
        web_http = None
        web_latency_ms = None
        web_valid = False
        temperature_c = None
        throttled_mask = None
        active_flags = None
        xorg = None
        openbox = None
        chromium = None

        code, output, _ = command(
            "systemctl",
            "is-active",
            "bpga.service",
        )
        service_active = code == 0 and output == "active"

        if not service_active:
            service_failures += 1

        try:
            api_http, api_latency_ms, payload = read_api()
            api_latencies.append(api_latency_ms)

            online = bool(payload.get("online"))
            data_valid = bool(payload.get("dataValid"))
            consecutive_errors = payload.get("consecutiveErrors")
            last_error = payload.get("lastError")

            measurement = payload.get("measurement") or {}

            sequence = measurement.get("sequence")
            sample_age_ms = measurement.get("sampleAgeMs")
            rpm = measurement.get("rpm")
            voltage_mv = measurement.get("voltageMv")
            current_ma = measurement.get("currentMa")
            power_w = measurement.get("powerW")

            if not online or not data_valid:
                invalid_data += 1

            if sample_age_ms is not None:
                sample_ages.append(sample_age_ms)

            if sequence is not None:
                if first_sequence is None:
                    first_sequence = sequence

                if previous_sequence is not None:
                    sequence_delta = sequence - previous_sequence

                    if sequence == previous_sequence:
                        sequence_repeats += 1
                    elif sequence < previous_sequence:
                        if (
                            previous_sequence >= 30000
                            and sequence <= 3000
                        ):
                            sequence_wraps += 1
                            sequence_delta = (
                                32768
                                - previous_sequence
                                + sequence
                            )
                        else:
                            sequence_regressions += 1

                previous_sequence = sequence
                last_sequence = sequence

        except Exception as error:
            api_failures += 1
            last_error = f"API: {error}"

        try:
            web_http, web_latency_ms, web_valid = read_web()
            web_latencies.append(web_latency_ms)

            if not web_valid:
                web_failures += 1

        except Exception:
            web_failures += 1

        if elapsed - last_health_check >= HEALTH_INTERVAL_S or rows == 0:
            last_health_check = elapsed

            try:
                temperature_c = read_temperature()
                temperatures.append(temperature_c)
            except Exception:
                temperature_c = None

            try:
                throttled_mask, active_flags = read_throttled()

                if baseline_mask is None:
                    baseline_mask = throttled_mask

                final_mask = throttled_mask

                if active_flags != 0:
                    active_power_events += 1

            except Exception:
                throttled_mask = None
                active_flags = None

            xorg = process_present("Xorg")
            openbox = process_present("openbox")
            chromium = process_present("chromium")

            if not xorg:
                xorg_failures += 1

            if not openbox:
                openbox_failures += 1

            if not chromium:
                chromium_failures += 1

        writer.writerow({
            "elapsed_s": f"{elapsed:.3f}",
            "timestamp": datetime.now().isoformat(timespec="seconds"),
            "service_active": int(service_active),
            "api_http": api_http,
            "api_latency_ms": (
                f"{api_latency_ms:.3f}"
                if api_latency_ms is not None
                else ""
            ),
            "online": int(online),
            "data_valid": int(data_valid),
            "consecutive_errors": consecutive_errors,
            "last_error": last_error,
            "sequence": sequence,
            "sequence_delta": sequence_delta,
            "sample_age_ms": sample_age_ms,
            "rpm": rpm,
            "voltage_mV": voltage_mv,
            "current_mA": current_ma,
            "power_W": power_w,
            "web_http": web_http,
            "web_latency_ms": (
                f"{web_latency_ms:.3f}"
                if web_latency_ms is not None
                else ""
            ),
            "web_valid": int(web_valid),
            "temperature_C": temperature_c,
            "throttled_mask": (
                f"0x{throttled_mask:X}"
                if throttled_mask is not None
                else ""
            ),
            "active_flags": (
                f"0x{active_flags:X}"
                if active_flags is not None
                else ""
            ),
            "xorg": "" if xorg is None else int(xorg),
            "openbox": "" if openbox is None else int(openbox),
            "chromium": "" if chromium is None else int(chromium),
        })

        csv_file.flush()
        rows += 1

        print(
            f"\rTiempo {elapsed / 60:5.1f}/30.0 min"
            f" | API err {api_failures}"
            f" | inválidos {invalid_data}"
            f" | web err {web_failures}"
            f" | energía activa {active_power_events}",
            end="",
            flush=True,
        )

        next_sample += INTERVAL_S

        if next_sample < time.monotonic() - INTERVAL_S:
            next_sample = time.monotonic()

duration_real = time.monotonic() - start_monotonic


def metric(values, operation):
    if not values:
        return "sin datos"

    if operation == "min":
        return f"{min(values):.2f}"
    if operation == "mean":
        return f"{statistics.mean(values):.2f}"
    if operation == "max":
        return f"{max(values):.2f}"

    raise ValueError(operation)


passed = all([
    service_failures == 0,
    api_failures == 0,
    web_failures == 0,
    invalid_data == 0,
    sequence_regressions == 0,
    active_power_events == 0,
    xorg_failures == 0,
    openbox_failures == 0,
    chromium_failures == 0,
])

summary = f"""
BPGA - RESULTADO PRUEBA INTEGRAL DE 30 MINUTOS
================================================

Fecha:                     {datetime.now().isoformat(timespec="seconds")}
Duración real:             {duration_real:.2f} s
Registros CSV:             {rows}

Servicio inactivo:         {service_failures}
Errores API:               {api_failures}
Datos inválidos:           {invalid_data}
Errores web:               {web_failures}

Secuencias repetidas:      {sequence_repeats}
Wraps normales:            {sequence_wraps}
Regresiones de secuencia:  {sequence_regressions}
Primera secuencia:         {first_sequence}
Última secuencia:          {last_sequence}

API latencia mínima:       {metric(api_latencies, "min")} ms
API latencia media:        {metric(api_latencies, "mean")} ms
API latencia máxima:       {metric(api_latencies, "max")} ms

Web latencia mínima:       {metric(web_latencies, "min")} ms
Web latencia media:        {metric(web_latencies, "mean")} ms
Web latencia máxima:       {metric(web_latencies, "max")} ms

Edad muestra mínima:       {metric(sample_ages, "min")} ms
Edad muestra media:        {metric(sample_ages, "mean")} ms
Edad muestra máxima:       {metric(sample_ages, "max")} ms

Temperatura mínima:        {metric(temperatures, "min")} °C
Temperatura media:         {metric(temperatures, "mean")} °C
Temperatura máxima:        {metric(temperatures, "max")} °C

Máscara inicial:           {"sin datos" if baseline_mask is None else f"0x{baseline_mask:X}"}
Máscara final:             {"sin datos" if final_mask is None else f"0x{final_mask:X}"}
Eventos eléctricos activos:{active_power_events}

Fallos Xorg:               {xorg_failures}
Fallos Openbox:            {openbox_failures}
Fallos Chromium:           {chromium_failures}

RESULTADO:                 {"APROBADA" if passed else "REVISAR"}
""".strip()

SUMMARY_PATH.write_text(summary + "\n", encoding="utf-8")

print("\n")
print(summary)
print()
print(f"CSV: {CSV_PATH}")
print(f"Resumen: {SUMMARY_PATH}")
