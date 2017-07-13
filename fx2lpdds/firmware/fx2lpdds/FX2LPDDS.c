//-----------------------------------------------------------------------------
//   File:      bulkloop.c
//   Contents:  Hooks required to implement USB peripheral function.
//
// $Archive: /USB/Examples/FX2LP/bulkloop/bulkloop.c $
// $Date: 3/23/05 2:55p $
// $Revision: 4 $
//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//-----------------------------------------------------------------------------
#pragma NOIV               // Do not generate interrupt vectors

#include "fx2.h"
#include "fx2regs.h"
#include "syncdly.h"            // SYNCDELAY macro

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings


#define VR_NAKALL_ON    0xD0
#define VR_NAKALL_OFF   0xD1

#define VX_BB 0xBB // GPIF write
#define VX_BC 0xBC // GPIF read 

#define GPIFTRIGRD 4
#define GPIF_EP2 0
#define GPIF_EP4 1
#define GPIF_EP6 2
#define GPIF_EP8 3

BOOL enum_high_speed = FALSE;     // flag to let firmware know FX2 enumerated at high speed
static WORD xFIFOBC_IN = 0x0000;  // variable that contains EP6FIFOBCH/L value
static WORD xdata LED_Count = 0;
static BYTE xdata LED_Status = 0;

WORD addr, len, Tcount;
// ...debug LEDs: accessed via movx reads only ( through CPLD )
// it may be worth noting here that the default monitor loads at 0xC000
xdata volatile const BYTE LED0_ON  _at_ 0x8000;
xdata volatile const BYTE LED0_OFF _at_ 0x8100;
xdata volatile const BYTE LED1_ON  _at_ 0x9000;
xdata volatile const BYTE LED1_OFF _at_ 0x9100;
xdata volatile const BYTE LED2_ON  _at_ 0xA000;
xdata volatile const BYTE LED2_OFF _at_ 0xA100;
xdata volatile const BYTE LED3_ON  _at_ 0xB000;
xdata volatile const BYTE LED3_OFF _at_ 0xB100;
// use this global variable when (de)asserting debug LEDs...

void GpifInit();

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

unsigned char ifconfigCache;

void TD_Init(void)             // Called once at startup
{
	// set the CPU clock to 48MHz
	CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1) ;
	SYNCDELAY;
	// set the slave FIFO interface to 48MHz
	IFCONFIG |= 0x40;

	//change EP configuration
	EP2CFG = 0xA0;
	SYNCDELAY;                    
	EP4CFG = 0x00;
	SYNCDELAY;                    
	EP6CFG = 0xE0;
	SYNCDELAY;                    
	EP8CFG = 0x00;

	// out endpoints do not come up armed

	FIFORESET = 0x80;  // set NAKALL bit to NAK all transfers from host
	SYNCDELAY;
	FIFORESET = 0x02;  // reset EP2 FIFO
	SYNCDELAY;
	FIFORESET = 0x06;  // reset EP6 FIFO
	SYNCDELAY;
	FIFORESET = 0x00;  // clear NAKALL bit to resume normal operation
	SYNCDELAY;

	EP2FIFOCFG = 0x00; // allow core to see zero to one transition of auto out bit
	SYNCDELAY;
	EP2FIFOCFG = 0x10; // auto out mode, disable PKTEND zero length send, byte ops
	SYNCDELAY;
	EP6FIFOCFG = 0x08; // auto in mode, disable PKTEND zero length send, byte ops
	SYNCDELAY; 

	// enable dual autopointer feature
	AUTOPTRSETUP |= 0x01;

	GpifInit ();       // initialize GPIF registers  


	/* XXX: Wait states: */
	// Wave 2 
	/* LenBr */ // 0xC7,     0xC7,     0xC8,     0xC8,     0xC8,     0xAE,     0x80,     0x07,


	/* Set fifo flag to Empty */
	EP2GPIFFLGSEL &= ~3;
	EP2GPIFFLGSEL |= 1;


	PORTACFG = 0x00;
//	OEA |= 0xF3;
	OEA |= 0xFF;
	IOA &= 0xFC;

	IOA |= 2; // Enable DDS Clock

	Rwuen = TRUE;                 // Enable remote-wakeup

	//IFCONFIG = 0xbE;
	//IFCONFIG = 0x1E;
	/* KLABAM */
	GPIFTRIG = GPIF_EP2;    // launch GPIF FIFO WRITE Transaction from EP2 FIFO
	SYNCDELAY;

	IOA &= ~4; // Enable LED

	ifconfigCache = IFCONFIG;
}


void TD_Poll(void)              // Called repeatedly while the device is idle
{
}

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
	return(TRUE);
}

BOOL TD_Resume(void)          // Called after the device resumes
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------

BOOL DR_GetDescriptor(void)
{
	return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{	
	if( EZUSB_HIGHSPEED( ) )
	{ // FX2 enumerated at high speed
		SYNCDELAY;                  
		//    EP6AUTOINLENH = 0x02;       // set AUTOIN commit length to 512 bytes
		EP6AUTOINLENH = 0x01;       // set AUTOIN commit length to 510 bytes
		SYNCDELAY;                  
		//    EP6AUTOINLENL = 0x00;
		EP6AUTOINLENL = 0xfe;
		SYNCDELAY;                  
		enum_high_speed = TRUE;
	}

	else
	{ // FX2 enumerated at full speed
		SYNCDELAY;                   
		//EP6AUTOINLENH = 0x00;       // set AUTOIN commit length to 64 bytes
		EP6AUTOINLENH = 0x00;       // set AUTOIN commit length to 60 bytes
		SYNCDELAY;                   
		//EP6AUTOINLENL = 0x40;
		EP6AUTOINLENL = 0x3c;
		SYNCDELAY;                  
		enum_high_speed = FALSE;
	}


	Configuration = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
	EP0BUF[0] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
	AlternateSetting = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
	EP0BUF[0] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
	return(TRUE);
}

BOOL DR_ClearFeature(void)
{
	return(TRUE);
}

BOOL DR_SetFeature(void)
{
	return(TRUE);
}

unsigned long int ddsClockFreqHz = 124998388;
unsigned long int ifClockFreqHz = 48007317;


BOOL DR_VendorCmnd(void)
{
	BYTE tmp;
	volatile BYTE *pSTATE = &GPIF_WAVE_DATA;
	BYTE bmRequestType = SETUPDAT[0];
	BYTE bmRequest = SETUPDAT[1];
	WORD wIndex = ( (SETUPDAT[5] << 8) + SETUPDAT[4] );
	WORD wValue = ( (SETUPDAT[3] << 8) + SETUPDAT[2] );
	WORD wLength = ( (SETUPDAT[7] << 8) + SETUPDAT[6] );

	switch (bmRequest)
	{
		case VR_NAKALL_ON:
			tmp = FIFORESET;
			tmp |= bmNAKALL;      
			SYNCDELAY;                    
			FIFORESET = tmp;
			break;
		case VR_NAKALL_OFF:
			tmp = FIFORESET;
			tmp &= ~bmNAKALL;      
			SYNCDELAY;                    
			FIFORESET = tmp;
			break;

		case 0x42:
			GPIFTRIG = GPIF_EP2;    // launch GPIF FIFO WRITE Transaction from EP2 FIFO
			SYNCDELAY;
			break;
		case 0x43:
			GPIFABORT = 0xFF;;
			break;
		case 0x44:
			IOA |= 1; // DDS in reset state
//			IOA |= 4; // Disable LED
			break;
		case 0x45:
			IOA &= ~1; // DDS Out of reset
//			IOA &= ~4; // Enable LED
			break;

		case 0x50: // DDS reset
			IOA = wValue & 0xff;
			break;

		case 0x52: // IF Clock source
			/* Always set IFCLKPOL=1, ASYNC=1, GSTATE=1 and IFCFG[1:0]=10 */
			ifconfigCache = (wValue | 0x1e) & (~0xfe) ;
			IFCONFIG = ifconfigCache;
			break;

		case 0x53: // Get on-board DDS reference frequency
			while(EP0CS & bmEPBUSY);

			*(EP0BUF+0) = (ddsClockFreqHz >>  0) & 0xff;
			*(EP0BUF+1) = (ddsClockFreqHz >>  8) & 0xff;
			*(EP0BUF+2) = (ddsClockFreqHz >> 16) & 0xff;
			*(EP0BUF+3) = (ddsClockFreqHz >> 24) & 0xff;
			EP0BCH = 0;
			EP0BCL = (BYTE)4;
			break;

		case 0x54: // Get on-board IF reference frequency
			while(EP0CS & bmEPBUSY);

			*(EP0BUF+0) = (ifClockFreqHz >>  0) & 0xff;
			*(EP0BUF+1) = (ifClockFreqHz >>  8) & 0xff;
			*(EP0BUF+2) = (ifClockFreqHz >> 16) & 0xff;
			*(EP0BUF+3) = (ifClockFreqHz >> 24) & 0xff;
			EP0BCH = 0;
			EP0BCL = (BYTE)4;
			break;

		case 0x55:
			GPIFABORT = 0xFF;
			while (!( GPIFTRIG & 0x80 ));

			FIFORESET = 0x80;  // set NAKALL bit to NAK all transfers from host
			SYNCDELAY;
			FIFORESET = 0x02;  // reset EP2 FIFO
			SYNCDELAY;
			FIFORESET = 0x06;  // reset EP6 FIFO
			SYNCDELAY;
			FIFORESET = 0x00;  // clear NAKALL bit to resume normal operation
			SYNCDELAY;

			IFCONFIG |= 0x40;
			SYNCDELAY;
			break;

		case 0x56:
			pSTATE[0x45] = wValue & 0xff;
			SYNCDELAY;
			break;

		case 0x57:
			TD_Init();
//			while (!( GPIFTRIG & 0x80 ));
//			GPIFTRIG = GPIF_EP2;
//			SYNCDELAY;
//			IFCONFIG = ifconfigCache;
//			SYNCDELAY;
 
 			break;

		case 0x60:
			IFCONFIG |= 0x80;
			break;
		case 0x61:
			IFCONFIG &= ~0x80;
			break;

		case 0xc0:
 		    IFCONFIG = 0xFE;
			SYNCDELAY;
			GPIFABORT = 0xFF; 
			SYNCDELAY;

			USBCS |= 0x08; // Disconnect
			USBCS &= ~0x02; // Re-numerate
			USBCS &= ~0x08; // Reconnect

			for(;;);

			break;

		default:
			return(TRUE);
	}

	return(FALSE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
	GotSUD = TRUE;            // Set flag
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

void ISR_Sof(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSOF;            // Clear SOF IRQ
}

void ISR_Ures(void) interrupt 0
{
	// whenever we get a USB reset, we should revert to full speed mode
	pConfigDscr = pFullSpeedConfigDscr;
	((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
	pOtherConfigDscr = pHighSpeedConfigDscr;
	((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		pConfigDscr = pHighSpeedConfigDscr;
		((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
		pOtherConfigDscr = pFullSpeedConfigDscr;
		((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}
