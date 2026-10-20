#!/bin/sh
# Removes the filter, PPD and print queue installed by install.sh.
# Usage: sudo ./uninstall.sh
set -e

PREFIX=/Library/Printers/hpcups-mac
PPD_DST=/Library/Printers/PPDs/Contents/Resources/hp-deskjet_2050_j510_series-mac.ppd.gz
QUEUE=HP_Deskjet_2050_J510_series

if [ "$(id -u)" -ne 0 ]; then
    echo "Run with sudo: sudo $0" >&2
    exit 1
fi

lpadmin -x "$QUEUE" 2>/dev/null || true
rm -f "$PPD_DST"
rm -rf "$PREFIX"

echo "Uninstalled."
