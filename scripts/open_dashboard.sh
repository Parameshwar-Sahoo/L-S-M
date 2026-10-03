#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HTML_PATH="$SCRIPT_DIR/../web/index.html"

echo "Opening SmartLMS Interactive Web Visualizer..."
if which xdg-open > /dev/null 2>&1; then
    xdg-open "$HTML_PATH"
elif which wslview > /dev/null 2>&1; then
    wslview "$HTML_PATH"
elif which explorer.exe > /dev/null 2>&1; then
    explorer.exe "$(wslpath -w "$HTML_PATH" 2>/dev/null || echo "$HTML_PATH")"
else
    echo "Please open the file in your browser: file://$HTML_PATH"
fi
