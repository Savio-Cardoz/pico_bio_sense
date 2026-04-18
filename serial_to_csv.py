#!/usr/bin/env python3
"""Read serial CSV data from Pico and write to CSV file.

The data format is:
timestamp,accX,accY,accZ,gyroX,gyroY,gyroZ,tempMPU,IR,Red,BPM,DS1
"""

import argparse
import csv
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing pyserial. Install it with: pip install pyserial")
    sys.exit(1)


def find_serial_ports():
    return [port.device for port in list_ports.comports()]


def parse_arguments():
    parser = argparse.ArgumentParser(description="Read serial CSV data from Pico and save to CSV file.")
    parser.add_argument("port", nargs="?", help="Serial port name (e.g. COM3). If omitted, the script lists available ports.")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("-o", "--output", default="data.csv", help="Output CSV file (default: data.csv)")
    parser.add_argument("-t", "--timeout", type=float, default=1.0, help="Serial read timeout in seconds")
    return parser.parse_args()


def open_serial(port, baudrate, timeout):
    try:
        ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud")
        return ser
    except serial.SerialException as exc:
        print(f"Failed to open serial port {port}: {exc}")
        sys.exit(1)


def main():
    args = parse_arguments()

    if not args.port:
        ports = find_serial_ports()
        if not ports:
            print("No serial ports found.")
            sys.exit(1)
        print("Available serial ports:")
        for port in ports:
            print(f"  {port}")
        sys.exit(0)

    ser = open_serial(args.port, args.baud, args.timeout)

    header = ['timestamp', 'accX', 'accY', 'accZ', 'gyroX', 'gyroY', 'gyroZ', 'tempMPU', 'IR', 'Red', 'BPM', 'DS1']

    with open(args.output, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(header)

        print(f"Writing data to {args.output}. Press Ctrl+C to stop.")

        try:
            while True:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    # Split the CSV line
                    values = line.split(',')
                    if len(values) == len(header):
                        writer.writerow(values)
                        print(f"Logged: {line}")
                    else:
                        print(f"Invalid line: {line} (expected {len(header)} values, got {len(values)})")
        except KeyboardInterrupt:
            print("\nStopped logging.")
        finally:
            ser.close()


if __name__ == "__main__":
    main()