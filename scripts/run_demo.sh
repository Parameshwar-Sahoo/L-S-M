#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."

echo "Building and running SmartLMS automated demonstration..."
cd "$ROOT_DIR"
make app
./bin/library_system --demo
