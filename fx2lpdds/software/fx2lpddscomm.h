#ifndef FX2LPDDSCOMM
#define FX2LPDDSCOMM

#include <libusb-1.0/libusb.h>

#define CONTROL_FLAG_RESET 0x01
#define CONTROL_FLAG_DDS_CLOCK_EN 0x02
#define CONTROL_FLAG_LEDOFF 0x04

inline void makeDdsWord(unsigned char *word, uint32_t divider)
{
	*word++ = 0;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*((uint32_t*) word) = __builtin_bswap32(divider);
#else   
	*((uint32_t*) word) = divider;
#endif
}

libusb_device_handle* fx2lpdds_open();
void fx2lpdds_writeControl(libusb_device_handle* hndl, unsigned char control);
void fx2lpdds_internalIf(libusb_device_handle* hndl);
void fx2lpdds_externalIf(libusb_device_handle* hndl);
unsigned int fx2lpdds_getReference(libusb_device_handle* hndl);
unsigned int fx2lpdds_getIf(libusb_device_handle* hndl);
void fx2lpdds_stop(libusb_device_handle* hndl);
void fx2lpdds_start(libusb_device_handle* hndl, int useInternalDdsClock, int useInternalIfClock);

#endif /* FX2LPDDSCOMM */
