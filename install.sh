#!/bin/sh
# Installs the hpcups filter and the HP Deskjet 2050 J510 PPD, then (re)creates the print queue.
# Usage: sudo ./install.sh
set -e

cd "$(dirname "$0")"

PREFIX=/Library/Printers/hpcups-mac
FILTER="$PREFIX/Filters/hpcups"
PPD_SRC=ppd/hp-deskjet_2050_j510_series.ppd
PPD_DST=/Library/Printers/PPDs/Contents/Resources/hp-deskjet_2050_j510_series-mac.ppd.gz
QUEUE=HP_Deskjet_2050_J510_series

if [ "$(id -u)" -ne 0 ]; then
    echo "Run with sudo: sudo $0" >&2
    exit 1
fi

if [ ! -x build/hpcups ]; then
    echo "build/hpcups not found: run 'make' first (without sudo)" >&2
    exit 1
fi

# Filter: must be owned by root and not writable by others, or CUPS refuses to run it.
# /usr/libexec/cups/filter is protected by SIP, so third-party filters live in /Library/Printers.
install -d -o root -g wheel -m 755 "$PREFIX/Filters"
install -o root -g wheel -m 755 build/hpcups "$FILTER"
# A binary from a downloaded release carries the quarantine attribute; CUPS can't run it with that.
xattr -c "$FILTER"

# PPD pointing at the filter by absolute path.
install -d -o root -g wheel -m 755 "$(dirname "$PPD_DST")"
sed -e "s|^\*cupsFilter: \"application/vnd.cups-raster 0 hpcups\"|*cupsFilter: \"application/vnd.cups-raster 0 $FILTER\"|" \
    -e 's|^\*NickName: "\(.*\)"|*NickName: "\1 (hpcups-mac)"|' \
    "$PPD_SRC" | gzip -9 > "$PPD_DST"
chown root:wheel "$PPD_DST"
chmod 644 "$PPD_DST"

# Find the printer on USB.
URI=$(lpinfo -v 2>/dev/null | awk '/usb:\/\/HP\/Deskjet%202050/ {print $2; exit}')
if [ -z "$URI" ]; then
    URI=$(lpstat -v "$QUEUE" 2>/dev/null | awk '{print $NF}')
fi
if [ -z "$URI" ]; then
    echo "Printer not found on USB: connect it, turn it on and try again." >&2
    exit 1
fi

lpadmin -x "$QUEUE" 2>/dev/null || true
lpadmin -p "$QUEUE" -E -v "$URI" -P "$PPD_DST" \
    -D "HP Deskjet 2050 J510 series" -o printer-is-shared=false
cupsenable "$QUEUE"
cupsaccept "$QUEUE"

echo "Installed. Queue: $QUEUE  URI: $URI"
echo "Test page: lp -d $QUEUE /usr/share/cups/data/testprint"
