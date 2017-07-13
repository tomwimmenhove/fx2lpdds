#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#include "fx2lpddscomm.h"

#define CONTROL_FLAG_RESET 0x01
#define CONTROL_FLAG_DDS_CLOCK_EN 0x02
#define CONTROL_FLAG_LEDOFF 0x04

libusb_device_handle* fx2lpdds_open()
{
	libusb_context* ctx;
	libusb_init(&ctx);

	libusb_device_handle* hndl = libusb_open_device_with_vid_pid(ctx,0x04b4,0x1004);
	if (hndl == NULL)
	{
		fprintf(stderr, "Could not open FX2LPDDS device!\n");
		exit(1);
	}
	libusb_claim_interface(hndl,0);
	libusb_set_interface_alt_setting(hndl,0,0);

	return hndl;
}

void fx2lpdds_writeControl(libusb_device_handle* hndl, unsigned char control)
{
	int ret = libusb_control_transfer (hndl, 0, 0x50, control, 0, NULL, 0, 1000);
	if (ret)
	{
		fprintf(stderr, "USB Transfer failed: %d\n", ret);
	}
}

void fx2lpdds_internalIf(libusb_device_handle* hndl)
{
	int ret = libusb_control_transfer (hndl, 0, 0x60, 0, 0, NULL, 0, 1000);
	if (ret)
	{
		fprintf(stderr, "USB Transfer failed: %d\n", ret);
	}
}

void fx2lpdds_externalIf(libusb_device_handle* hndl)
{
	int ret = libusb_control_transfer (hndl, 0, 0x61, 0, 0, NULL, 0, 1000);
	if (ret)
	{
		fprintf(stderr, "USB Transfer failed: %d\n", ret);
	}
}

unsigned int fx2lpdds_getReference(libusb_device_handle* hndl)
{
	unsigned int freq_hz = 0;

	libusb_control_transfer (hndl, 0x80, 0x53, 0, 0, (unsigned char*) &freq_hz, 4, 1000);

	return freq_hz;
}

unsigned int fx2lpdds_getIf(libusb_device_handle* hndl)
{
	unsigned int freq_hz = 0;

	libusb_control_transfer (hndl, 0x80, 0x54, 0, 0, (unsigned char*) &freq_hz, 4, 1000);

	return freq_hz;
}

void fx2lpdds_stop(libusb_device_handle* hndl)
{
	fx2lpdds_writeControl(hndl, CONTROL_FLAG_RESET | CONTROL_FLAG_LEDOFF);
}

void fx2lpdds_start(libusb_device_handle* hndl, int useInternalDdsClock, int useInternalIfClock)
{
	fx2lpdds_writeControl(hndl, CONTROL_FLAG_RESET | CONTROL_FLAG_LEDOFF);
#if 1
	char zeroBuf[5];
	int transferred;
	memset(zeroBuf, 0, sizeof(zeroBuf));

	int ret = libusb_bulk_transfer(hndl,
			0x02,
			(unsigned char*) zeroBuf,
			sizeof(zeroBuf),
			&transferred,
			10000);
	if(ret)
	{
		fprintf(stderr, "USB Bulk Transfer failed: %d\n", ret);
	}
#endif
	fx2lpdds_writeControl(hndl, useInternalDdsClock ? CONTROL_FLAG_DDS_CLOCK_EN : 0);
	if (useInternalIfClock)
	{
		fx2lpdds_internalIf(hndl);
	}
	else
	{
		fx2lpdds_externalIf(hndl);
	}
}

