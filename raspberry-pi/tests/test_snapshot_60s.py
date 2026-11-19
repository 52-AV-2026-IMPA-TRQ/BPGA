#!/usr/bin/env python3

import csv
import serial
import statistics
import time
from pathlib import Path

PORT = "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AL5SH6SC-if00-port0"
OUTPUT = Path.home() / "bpga" / "snapshot_60s.csv"

SLAVE = 1
FUNCTION = 4
ADDRESS = 0x0010
COUNT = 32

DURATION_S = 60.0
PERIOD_S = 0.2
SERIAL_TIMEOUT_S = 0.15


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


def build_request(slave, function, address, count):
    frame = bytes([
        slave,
        function,
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


def parse_registers(response):
    if len(response) != 69:
        raise ValueError(f"Longitud incorrecta: {len(response)}")

    if response[0] != SLAVE:
        raise ValueError(f"Esclavo incorrecto: {response[0]}")

    if response[1] != FUNCTION:
        raise ValueError(f"Función incorrecta: {response[1]}")

    if response[2] != 64:
        raise ValueError(f"Cantidad de datos incorrecta: {response[2]}")

    received_crc = int.from_bytes(response[-2:], "little")
    calculated_crc = crc16_modbus(response[:-2])

    if received_crc != calculated_crc:
        raise ValueError("CRC incorrecto")

    data = response[3:-2]

    return [
        int.from_bytes(data[index:index + 2], "big")
        for index in range(0, len(data), 2)
    ]


def u32(registers, index):
    return (registers[index] << 16) | registers[index + 1]


request = build_request(
    SLAVE,
    FUNCTION,
    ADDRESS,
    COUNT,
)

latencies_ms = []
logical_sequences = []

successful = 0
timeouts = 0
crc_or_frame_errors = 0
inconsistent_snapshots = 0
repeated_sequences = 0

previous_sequence = None

start_test = time.monotonic()
next_cycle = start_test

with OUTPUT.open("w", newline="", encoding="utf-8") as csv_file:
    writer = csv.writer(csv_file)

    writer.writerow([
        "elapsed_s",
        "latency_ms",
        "sequence_raw",
        "sequence",
        "system_state",
        "system_substate",
        "status_flags",
        "control_flags",
        "rpm",
        "voltage_mV",
        "current_mA",
        "power_W",
        "sample_age_ms",
        "snapshot_consistent",
    ])

    with serial.Serial(
        PORT,
        baudrate=19200,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_EVEN,
        stopbits=serial.STOPBITS_ONE,
        timeout=SERIAL_TIMEOUT_S,
        rtscts=False,
        dsrdtr=False,
    ) as port:

        while True:
            now = time.monotonic()

            if now - start_test >= DURATION_S:
                break

            if now < next_cycle:
                time.sleep(next_cycle - now)

            cycle_start = time.monotonic()

            port.reset_input_buffer()
            port.write(request)
            port.flush()

            response = port.read(69)

            latency_ms = (
                time.monotonic() - cycle_start
            ) * 1000.0

            elapsed_s = time.monotonic() - start_test

            if not response:
                timeouts += 1
                print(
                    f"\rTiempo: {elapsed_s:5.1f} s | "
                    f"OK: {successful:3d} | "
                    f"Timeouts: {timeouts:2d} | "
                    f"Errores: {crc_or_frame_errors:2d}",
                    end="",
                    flush=True,
                )

                next_cycle += PERIOD_S
                continue

            try:
                registers = parse_registers(response)
            except ValueError as error:
                crc_or_frame_errors += 1
                print(f"\nError de trama: {error}")
                next_cycle += PERIOD_S
                continue

            sequence_begin = registers[0]
            sequence_end = registers[31]
            snapshot_consistent = (
                sequence_begin == sequence_end
                and sequence_begin % 2 == 0
            )

            if not snapshot_consistent:
                inconsistent_snapshots += 1

            logical_sequence = sequence_begin >> 1

            if previous_sequence == logical_sequence:
                repeated_sequences += 1

            previous_sequence = logical_sequence

            system_state = registers[1]
            system_substate = registers[2]
            status_flags = registers[3]
            control_flags = registers[4]

            rpm = registers[17]
            voltage_mv = u32(registers, 18)
            current_ma = u32(registers, 20)
            power_w = u32(registers, 22)
            sample_age_ms = u32(registers, 28)

            successful += 1
            latencies_ms.append(latency_ms)
            logical_sequences.append(logical_sequence)

            writer.writerow([
                f"{elapsed_s:.3f}",
                f"{latency_ms:.3f}",
                sequence_begin,
                logical_sequence,
                system_state,
                system_substate,
                status_flags,
                control_flags,
                rpm,
                voltage_mv,
                current_ma,
                power_w,
                sample_age_ms,
                int(snapshot_consistent),
            ])

            print(
                f"\rTiempo: {elapsed_s:5.1f} s | "
                f"OK: {successful:3d} | "
                f"Timeouts: {timeouts:2d} | "
                f"Errores: {crc_or_frame_errors:2d} | "
                f"RPM: {rpm:4d} | "
                f"V: {voltage_mv / 1000:6.1f} | "
                f"A: {current_ma / 1000:5.2f} | "
                f"Latencia: {latency_ms:5.1f} ms",
                end="",
                flush=True,
            )

            next_cycle += PERIOD_S

            if next_cycle < time.monotonic() - PERIOD_S:
                next_cycle = time.monotonic()

print("\n\n=== RESUMEN DE LA PRUEBA ===")
print(f"Archivo CSV:              {OUTPUT}")
print(f"Lecturas correctas:       {successful}")
print(f"Timeouts:                 {timeouts}")
print(f"Errores CRC/trama:        {crc_or_frame_errors}")
print(f"Snapshots inconsistentes: {inconsistent_snapshots}")
print(f"Secuencias repetidas:     {repeated_sequences}")

if latencies_ms:
    print(f"Latencia mínima:          {min(latencies_ms):.2f} ms")
    print(f"Latencia media:           {statistics.mean(latencies_ms):.2f} ms")
    print(f"Latencia máxima:          {max(latencies_ms):.2f} ms")

    actual_duration = time.monotonic() - start_test
    frequency = successful / actual_duration

    print(f"Duración real:            {actual_duration:.2f} s")
    print(f"Frecuencia correcta:      {frequency:.2f} Hz")

if len(logical_sequences) >= 2:
    print(
        "Secuencia lógica:         "
        f"{logical_sequences[0]} -> {logical_sequences[-1]}"
    )
