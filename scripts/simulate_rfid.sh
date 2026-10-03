#!/bin/bash
# SmartLMS RFID Hardware Event Generator / Simulator
TAG=${1:-"RFID-LPI-001"}
TYPE=${2:-"2"} # 1: Kiosk, 2: DropBox, 3: Gate

echo "=========================================================="
echo "    SmartLMS RFID Hardware Event Injector                 "
echo "=========================================================="
echo "Injecting Tag: $TAG (Event Type: $TYPE)..."

if [ -c /dev/libsys_driver ]; then
    echo "Writing directly to /dev/libsys_driver character node..."
    # Writing via dd / python or C tool
    python3 -c "
import struct, time
# Format: uint32 event_id, uint64 ts, char tag[32], char card[32], uint8 type, uint8 gate, uint8 rssi, uint8 flags
tag_bytes = '$TAG'.encode('ascii')[:31].ljust(32, b'\x00')
card_bytes = b''.ljust(32, b'\x00')
payload = struct.pack('=IQ32s32sBBBB', 99, int(time.time()*1e9), tag_bytes, card_bytes, int('$TYPE'), 1, 95, 0)
try:
    with open('/dev/libsys_driver', 'wb') as f:
        f.write(payload)
    print('Event injected successfully into kernel driver.')
except Exception as e:
    print('Kernel write error:', e)
" 2>/dev/null || echo "Direct write requires root permissions or Python 3."
else
    echo "Driver node /dev/libsys_driver not present."
    echo "In mock mode, trigger events directly in the Terminal UI or Web Dashboard."
fi
