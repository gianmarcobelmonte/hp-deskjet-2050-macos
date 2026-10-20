/*
 * usbprobe: talks directly to the USB printer-class interface (class 7) via IOKit.
 * Reads the IEEE 1284 Device ID and port status, and optionally sends a file.
 *
 * Usage: usbprobe [-r] [file]
 *   -r    send a printer-class SOFT_RESET first
 *   file  send the file to the bulk OUT pipe, with timeouts
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#define HP_VID 0x03f0
#define DJ2050_PID 0x8711

typedef IOUSBInterfaceInterface300 **Intf;

static Intf open_printer_interface(void)
{
    CFMutableDictionaryRef match = IOServiceMatching(kIOUSBInterfaceClassName);
    io_iterator_t it;
    io_service_t svc;
    Intf found = NULL;

    if (IOServiceGetMatchingServices(kIOMainPortDefault, match, &it) != KERN_SUCCESS)
        return NULL;

    while (!found && (svc = IOIteratorNext(it))) {
        IOCFPlugInInterface **plug = NULL;
        SInt32 score;
        Intf intf = NULL;

        if (IOCreatePlugInInterfaceForService(svc, kIOUSBInterfaceUserClientTypeID,
                                              kIOCFPlugInInterfaceID, &plug, &score) == KERN_SUCCESS) {
            (*plug)->QueryInterface(plug, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
                                    (LPVOID *)&intf);
            (*plug)->Release(plug);
        }
        IOObjectRelease(svc);
        if (!intf)
            continue;

        UInt16 vid = 0, pid = 0;
        UInt8 cls = 0;
        (*intf)->GetDeviceVendor(intf, &vid);
        (*intf)->GetDeviceProduct(intf, &pid);
        (*intf)->GetInterfaceClass(intf, &cls);
        if (vid == HP_VID && pid == DJ2050_PID && cls == kUSBPrintingClass)
            found = intf;
        else
            (*intf)->Release(intf);
    }
    IOObjectRelease(it);
    return found;
}

static IOReturn class_request(Intf intf, UInt8 dir, UInt8 req, UInt16 value, UInt16 index,
                              void *buf, UInt16 len, UInt32 *done)
{
    IOUSBDevRequestTO r = {0};
    r.bmRequestType = USBmakebmRequestType(dir, kUSBClass, kUSBInterface);
    r.bRequest = req;
    r.wValue = value;
    r.wIndex = index;
    r.wLength = len;
    r.pData = buf;
    r.noDataTimeout = 5000;
    r.completionTimeout = 5000;
    IOReturn kr = (*intf)->ControlRequestTO(intf, 0, &r);
    if (done)
        *done = r.wLenDone;
    return kr;
}

int main(int argc, char **argv)
{
    int do_reset = 0;
    const char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r"))
            do_reset = 1;
        else
            file = argv[i];
    }

    Intf intf = open_printer_interface();
    if (!intf) {
        fprintf(stderr, "Printer interface 03f0:8711 not found\n");
        return 1;
    }

    UInt8 num = 0, alt = 0;
    (*intf)->GetInterfaceNumber(intf, &num);
    (*intf)->GetAlternateSetting(intf, &alt);

    IOReturn kr = (*intf)->USBInterfaceOpen(intf);
    printf("USBInterfaceOpen(if=%d alt=%d): 0x%08x\n", num, alt, kr);
    if (kr != kIOReturnSuccess)
        return 1;

    UInt16 index = (UInt16)((num << 8) | alt);

    if (do_reset) {
        /* SOFT_RESET: request 2, OUT, wIndex = interface number */
        kr = class_request(intf, kUSBOut, 2, 0, num, NULL, 0, NULL);
        printf("SOFT_RESET: 0x%08x\n", kr);
    }

    unsigned char id[1024] = {0};
    UInt32 got = 0;
    kr = class_request(intf, kUSBIn, 0 /* GET_DEVICE_ID */, 0, index, id, sizeof(id) - 1, &got);
    if (kr == kIOReturnSuccess && got >= 2) {
        int len = (id[0] << 8) | id[1];
        if (len > (int)got)
            len = got;
        printf("GET_DEVICE_ID (%u bytes): %.*s\n", (unsigned)got, len - 2, id + 2);
    } else {
        printf("GET_DEVICE_ID: 0x%08x (%u bytes)\n", kr, (unsigned)got);
    }

    unsigned char st = 0;
    kr = class_request(intf, kUSBIn, 1 /* GET_PORT_STATUS */, 0, num, &st, 1, &got);
    printf("GET_PORT_STATUS: 0x%08x value=0x%02x (paper-empty=%d selected=%d no-error=%d)\n",
           kr, st, !!(st & 0x20), !!(st & 0x10), !!(st & 0x08));

    UInt8 neps = 0, outPipe = 0, inPipe = 0;
    (*intf)->GetNumEndpoints(intf, &neps);
    for (UInt8 p = 1; p <= neps; p++) {
        UInt8 dir, number, type, interval;
        UInt16 maxp;
        (*intf)->GetPipeProperties(intf, p, &dir, &number, &type, &maxp, &interval);
        printf("pipe %d: ep=%d dir=%s type=%d maxpacket=%d\n", p, number,
               dir == kUSBIn ? "IN" : "OUT", type, maxp);
        if (type == kUSBBulk && dir == kUSBOut && !outPipe)
            outPipe = p;
        if (type == kUSBBulk && dir == kUSBIn && !inPipe)
            inPipe = p;
    }

    if (file && outPipe) {
        FILE *f = fopen(file, "rb");
        if (!f) {
            perror(file);
            return 1;
        }
        unsigned char buf[16384];
        size_t n, total = 0;
        (*intf)->ClearPipeStallBothEnds(intf, outPipe);
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            kr = (*intf)->WritePipeTO(intf, outPipe, buf, (UInt32)n, 10000, 20000);
            if (kr != kIOReturnSuccess) {
                printf("WritePipe failed after %zu bytes: 0x%08x\n", total, kr);
                break;
            }
            total += n;
        }
        fclose(f);
        printf("Sent %zu bytes\n", total);
    }

    (*intf)->USBInterfaceClose(intf);
    (*intf)->Release(intf);
    return 0;
}
