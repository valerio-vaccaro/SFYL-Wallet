#!/usr/bin/env bash
# Flash an SFYL Wallet firmware artifact onto a TTGO T-Watch (ESP32).
# Usage: ./flash.sh [serial-port]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-}"

usage() {
  echo "Usage: $(basename "$0") [serial-port]" >&2
  echo "Example: $(basename "$0") /dev/ttyUSB0" >&2
}

if [[ "${PORT}" == "-h" || "${PORT}" == "--help" ]]; then
  usage
  exit 0
fi

if [[ $# -gt 1 ]]; then
  usage
  exit 1
fi

for image in bootloader.bin partitions.bin boot_app0.bin firmware.bin; do
  if [[ ! -f "${SCRIPT_DIR}/${image}" ]]; then
    echo "Missing ${image}; run this script from the extracted firmware artifact." >&2
    exit 1
  fi
done

if [[ -z "${PORT}" ]]; then
  shopt -s nullglob
  ports=(/dev/ttyUSB* /dev/ttyACM*)
  shopt -u nullglob

  if [[ ${#ports[@]} -eq 1 ]]; then
    PORT="${ports[0]}"
    echo "Using detected serial port: ${PORT}"
  elif [[ ${#ports[@]} -eq 0 ]]; then
    echo "No serial port found. Connect the board and pass its port explicitly." >&2
    usage
    exit 1
  else
    echo "More than one serial port found; pass the board port explicitly." >&2
    printf 'Detected ports: %s\n' "${ports[*]}" >&2
    usage
    exit 1
  fi
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required. Install Python and esptool, then try again." >&2
  exit 1
fi

if ! python3 -m esptool version >/dev/null 2>&1; then
  echo "esptool is required. Install it with: python3 -m pip install --user esptool" >&2
  exit 1
fi

echo "Flashing ${PORT}. Do not disconnect the board until flashing completes."
python3 -m esptool \
  --chip esp32 \
  --port "${PORT}" \
  --baud 460800 \
  write_flash -z \
  0x1000 "${SCRIPT_DIR}/bootloader.bin" \
  0x8000 "${SCRIPT_DIR}/partitions.bin" \
  0xe000 "${SCRIPT_DIR}/boot_app0.bin" \
  0x10000 "${SCRIPT_DIR}/firmware.bin"

echo "Firmware flashed successfully. The board will restart."
