# HP Deskjet 2050 J510 driver for macOS

A working macOS driver for the **HP Deskjet 2050 J510 series** (USB), a printer HP no longer supports on recent macOS releases such as Sequoia.

It is a port of `hpcups`, the open source CUPS filter from HP's own Linux driver ([HPLIP](https://developers.hp.com/hp-linux-imaging-and-printing) 3.26.6). It is trimmed down to the code this printer needs and built against the CUPS that ships with macOS. It has no external dependencies, needs no proprietary HP plugin and installs no kernel extensions.

> This project is not affiliated with or endorsed by HP. "HP" and "Deskjet" are used only to identify the supported printer.

## Compatibility

| | |
|---|---|
| Printer | HP Deskjet 2050 J510 series, connected over **USB** |
| USB ID | `03f0:8711` |
| macOS | Tested on macOS 15 Sequoia (15.7). It should also work on other recent releases |
| Mac | Universal binary (Intel and Apple Silicon). Tested on an Intel Mac |

Other printers are **not** supported. The installer looks specifically for this model.

## Features

- Color and grayscale printing (high-quality grayscale, or black-only)
- Print quality: Fast Draft, Normal, Best, High-Resolution Photo
- Plain and photo paper
- A4, Letter, A5, A6, B5, Legal, photo sizes (10×15 cm, 13×18 cm…), envelopes (DL, C5, C6, #10) and custom sizes
- Print-dialog options in English and Italian, following the system language

Scanning is not included. This is a printing driver only.

## Installation

### 1. Install the build tools

You need Apple's Command Line Tools. They are free and do not require the full Xcode. Open **Terminal** and run:

```sh
xcode-select --install
```

If they are already installed, you will see a message saying so. Continue to the next step.

### 2. Download the project

```sh
git clone https://github.com/gianmarcobelmonte/hp-deskjet-2050-macos.git
cd hp-deskjet-2050-macos
```

You can also use **Code → Download ZIP** on GitHub, unzip the file and `cd` into the folder.

### 3. Build

```sh
make
```

This creates `build/hpcups` (the driver) and `build/usbprobe` (a diagnostic tool). Compiler warnings about `Mode9.cpp` come from HP's original code and are harmless.

### 4. Connect the printer

1. Turn the printer on and **wait until it is completely idle and silent**.
2. Then connect the USB cable to the Mac. A USB‑C to USB‑A adapter is fine.

Connecting the cable while the printer is still starting up can leave its USB interface unresponsive (see [Troubleshooting](#troubleshooting)).

### 5. Install

```sh
sudo ./install.sh
```

Enter your Mac password when asked. The script:

- copies the driver to `/Library/Printers/hpcups-mac/Filters/hpcups`;
- installs the printer description file (PPD) in `/Library/Printers/PPDs/Contents/Resources/`;
- **deletes any existing queue named `HP_Deskjet_2050_J510_series`** and creates a new one that uses this driver. macOS often creates that queue on its own when the printer is plugged in, usually with a driver that does not work.

If you created the printer manually in **System Settings → Printers & Scanners** under a different name, remove it there to avoid confusion. Then use the queue named **HP Deskjet 2050 J510 series**.

### 6. Set A4 as the default paper size (optional)

The default paper size is Letter. Outside North America, set A4 for your user:

```sh
lpoptions -p HP_Deskjet_2050_J510_series -o PageSize=A4
```

### 7. Print a test page

```sh
lp -d HP_Deskjet_2050_J510_series /usr/share/cups/data/testprint
```

From now on, choose **HP Deskjet 2050 J510 series** in the Print dialog of any app.

## Updating

Pull the new version, rebuild and run the installer again:

```sh
git pull
make
sudo ./install.sh
```

## Uninstalling

```sh
sudo ./uninstall.sh
```

This removes the driver, the PPD and the print queue.

## Troubleshooting

### Nothing prints and the queue stops with "Unable to send data to printer"

CUPS could not write to the printer over USB, so it paused the queue. This is a USB connection problem, not a driver problem.

1. Unplug the USB cable.
2. Make sure the printer is on and idle.
3. Plug the cable back in.
4. Resume the queue:
   ```sh
   cupsenable HP_Deskjet_2050_J510_series
   ```

If that does not help, turn the printer off, unplug its power cord for 60 seconds, turn it back on, wait until it is idle, and only then connect the USB cable.

You can check whether the printer answers over USB with:

```sh
./build/usbprobe
```

A healthy printer returns a long `GET_DEVICE_ID` string (starting with `MFG:HP;MDL:Deskjet 2050 J510 series;CMD:...`) and `GET_PORT_STATUS ... no-error=1`. An error `0xe00002ed` means the printer is not responding on USB; reconnect it as described above. `Printer interface 03f0:8711 not found` means macOS sees no working printer interface; reconnect the cable.

### The printer starts, then stops; power and cartridge lights blink

This means the cartridge carriage is blocked (carriage stall), which is a mechanical problem.

1. Turn the printer off.
2. Open the cartridge access door and remove any paper scraps or objects from the carriage path.
3. Move the carriage by hand to check that it slides freely, and make sure both cartridges are firmly seated.
4. Reload the paper, close the door and turn the printer back on.

### Content near the edges is cut off

The printer cannot print all the way to the edge of the paper. The unprintable margins on A4 are about 3 mm on the left and right, 1.5 mm at the top and **14.5 mm at the bottom**. macOS clips anything outside the printable area, which is why you already see the cut in the print preview.

To avoid it:
- In the Print dialog, choose **Scale to Fit**, or reduce the scale to about 95%.
- Or give your document margins of at least 15 mm at the bottom and 4 mm on the sides.

### Faded prints or missing colors

The cartridges are probably low on ink. Cartridge lights that stay on mean low ink.

### Looking at CUPS logs

```sh
tail -50 /var/log/cups/error_log
```

For more detail, enable debug logging, print again, then turn it back off:

```sh
sudo cupsctl --debug-logging
sudo cupsctl --no-debug-logging
```

## How it works

```
App → macOS print system (CUPS) → PDF → raster image
    → hpcups (this driver) → PCL3GUI2 printer commands → USB → printer
```

- `src/hpcups/`: the `hpcups` filter from HPLIP 3.26.6, limited to the PCL3 family of printer languages (this printer uses `pcl3gui2`).
- `src/common/`: helper code from HPLIP.
- `ppd/hp-deskjet_2050_j510_series.ppd`: the official HPLIP printer description, with an added Italian translation.
- `tools/usbprobe.c`: a small IOKit tool that queries the printer directly over USB.
- `install.sh` / `uninstall.sh`: install and remove the driver and the queue.

### Changes from HPLIP

- `EncapsulatorFactory.cpp`: when `HPCUPS_MINIMAL` is defined, only the `pcl3`, `pcl3gui` and `pcl3gui2` languages are compiled. This removes the dependencies on libjpeg and on HP's Linux-only `libImageProcessor`.
- `common/utils.c` and `utils.h` are renamed to `hplip_utils.c` and `hplip_utils.h`. macOS uses a case-insensitive filesystem, where `utils.h` collides with `hpcups/Utils.h`. The includes in `HPCupsFilter.cpp`, `Halftoner.cpp`, `SystemServices.cpp` and `Utils.cpp` are updated to match.
- Source files used only by other printer families (LaserJet, PCLm, LIDIL, JPEG/JBIG) are removed.
- The PPD gained an Italian translation (`*cupsLanguages: "en it"`).
- The filter is installed in `/Library/Printers` because `/usr/libexec/cups/filter` is protected by System Integrity Protection. The PPD refers to it by absolute path.

Each modified source file carries a note describing the change.

## License

This project is distributed under the **GNU General Public License v2.0 or later** ([LICENSE](LICENSE)). It is required because it includes HPLIP code licensed under GPL v2 or later (`ErnieFilter.cpp`).

Most of the HPLIP code included here is © HP Development Company, L.P. under a BSD-style license, and some files are under the MIT license. Each file keeps its original copyright and license header. The full HPLIP license text is in [COPYING.HPLIP](COPYING.HPLIP).

This software comes with **no warranty**. Use it at your own risk.
