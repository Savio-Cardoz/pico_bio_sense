#!/usr/bin/env python3
"""Read Pico CSV output from a serial port and plot it continuously.

The Pico serial format is expected to be:
  timestamp,accX,accY,accZ,gyroX,gyroY,gyroZ,tempMPU,IR,Red,BPM,DS1

By default the script plots three data series from the schema.
"""

import argparse
import collections
import csv
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing pyserial. Install it with: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
except ImportError:
    print("Missing matplotlib. Install it with: pip install matplotlib")
    sys.exit(1)

FIELD_NAMES = [
    "timestamp",
    "accX",
    "accY",
    "accZ",
    "gyroX",
    "gyroY",
    "gyroZ",
    "tempMPU",
    "IR",
    "Red",
    "BPM",
    "DS1",
]


def find_serial_ports():
    return [port.device for port in list_ports.comports()]


def parse_columns(value):
    columns = [item.strip() for item in value.split(",") if item.strip()]
    invalid = [col for col in columns if col not in FIELD_NAMES]
    if invalid:
        raise argparse.ArgumentTypeError(f"Unknown column(s): {', '.join(invalid)}")
    if len(columns) != 3:
        raise argparse.ArgumentTypeError("Please specify exactly 3 columns to plot.")
    return columns


def parse_arguments():
    parser = argparse.ArgumentParser(description="Plot Pico CSV serial data in real time.")
    parser.add_argument("port", nargs="?", help="Serial port name (e.g. COM3). If omitted, the script lists available ports.")
    parser.add_argument("-b", "--baud", type=int, default=1000000, help="Baud rate (default: 1000000)")
    parser.add_argument("-l", "--lines", type=int, default=200, help="Number of points to keep in the moving window")
    parser.add_argument("-t", "--timeout", type=float, default=1.0, help="Serial read timeout in seconds")
    parser.add_argument(
        "-c",
        "--columns",
        type=parse_columns,
        default=["DS1", "accZ", "IR"],
        help="Comma-separated list of three fields to plot. Available fields: " + ", ".join(FIELD_NAMES),
    )
    return parser.parse_args()


def open_serial(port, baudrate, timeout):
    try:
        ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud")
        return ser
    except serial.SerialException as exc:
        print(f"Failed to open serial port {port}: {exc}")
        sys.exit(1)


def parse_csv_line(line):
    text = line.decode(errors="ignore").strip()
    if not text or text.startswith("Found") or text.startswith("could not"):
        return None

    parts = [part.strip() for part in text.split(",")]
    if len(parts) != len(FIELD_NAMES):
        return None

    try:
        return {name: float(value) for name, value in zip(FIELD_NAMES, parts)}
    except ValueError:
        return None


def main():
    args = parse_arguments()

    if args.port is None:
        ports = find_serial_ports()
        if not ports:
            print("No serial ports found.")
            sys.exit(1)
        print("Available serial ports:")
        for port in ports:
            print(f"  {port}")
        print("Run with the desired port, for example: python plot_serial.py COM3")
        sys.exit(0)

    ser = open_serial(args.port, args.baud, args.timeout)

    window = collections.deque(maxlen=args.lines)
    data_windows = {col: collections.deque(maxlen=args.lines) for col in args.columns}
    start_time = time.time()

    fig, ax = plt.subplots(figsize=(10, 6))
    colors = ["tab:blue", "tab:orange", "tab:green"]
    lines = [ax.plot([], [], label=col, color=colors[i])[0] for i, col in enumerate(args.columns)]

    info_text = ax.text(
        0.02,
        0.98,
        "",
        transform=ax.transAxes,
        va="top",
        ha="left",
        bbox={"boxstyle": "round", "facecolor": "white", "alpha": 0.8, "edgecolor": "gray"},
    )
    ax.set_title(f"Serial data from {args.port}")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Value")
    ax.legend(loc="upper left")
    ax.grid(True)

    def update(frame):
        try:
            raw = ser.readline()
        except serial.SerialException as exc:
            print(f"Serial error: {exc}")
            return (*lines, info_text)

        values = parse_csv_line(raw)
        if values is not None:
            elapsed = time.time() - start_time
            window.append(elapsed)
            for col in args.columns:
                data_windows[col].append(values[col])

            for line, col in zip(lines, args.columns):
                line.set_data(window, data_windows[col])

            current_text = "Current:\n" + "\n".join(
                f"  {col}={values[col]:.2f}" for col in args.columns
            )
            info_text.set_text(current_text)

            if window:
                ax.set_xlim(window[0], window[-1] + 0.1)
                all_values = []
                for col in args.columns:
                    all_values.extend(data_windows[col])
                if all_values:
                    amin = min(all_values)
                    amax = max(all_values)
                    padding = max(0.5, (amax - amin) * 0.1)
                    ax.set_ylim(amin - padding, amax + padding)

        return (*lines, info_text)

    ani = FuncAnimation(fig, update, interval=100, blit=True)

    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()


if __name__ == "__main__":
    main()
