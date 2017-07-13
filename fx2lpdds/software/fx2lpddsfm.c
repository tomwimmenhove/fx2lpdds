#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libusb-1.0/libusb.h>

#include "readers.h"
#include "metricprefix.h"
#include "fx2lpddscomm.h"
#include "modulators.h"

static volatile int quit = 0;

void intHandler(int sig)
{
	quit = 1;
}

void print_syntax(char* progName)
{
	fprintf(stderr, "FX2LPDDS FM driver 2017 by Tom Wimmenhove\n\n");
	fprintf(stderr, "Usage %s <options> [filename]\n", progName);
	fprintf(stderr, "\tfilename is a raw file, used for modulation. If no filename is given, stdin is used. The sample rate is fixed at 192kHz.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t-h --help                            : This\n");
	fprintf(stderr, "\t-n --renum                           : Re-numerate for firmware upload\n");
	fprintf(stderr, "\t-u --firmware <filename>             : Upload firmware using cycfx2prog\n");
	fprintf(stderr, "\t-v --verbose                         : Increase verbosity\n");
	fprintf(stderr, "\t-k --ddsclock <freqyency>            : Set external DDS clock frequency (defaults to 74kHz)\n");
	fprintf(stderr, "\t-f --carrier  <freqyency>            : Set carrier frequency\n");
	fprintf(stderr, "\t-c --channels <nChannels>            : Set number of channels\n");
	fprintf(stderr, "\t-d --deviation <frequency deviation> : Set maximum deviation\n");
	fprintf(stderr, "\t-t --format <input format>           : Set the input format\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Input formats:\n");
	fprintf(stderr, "\t\"s16\"    : 16 bit signed integer in native endianness.\n");
	fprintf(stderr, "\t\"s16le\"  : 16 bit signed integer, little endian.\n");
	fprintf(stderr, "\t\"s16be\"  : 16 bit signed integer, big endian.\n");
	fprintf(stderr, "\t\"float\"  : Single precision, 32 bit floaing point.\n");
	fprintf(stderr, "\t\"double\" : Double precision, 64 bit floating point. This is the default.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "\tsox sample.wav --encoding float -c 2 -r 192000 -t raw /dev/stdout lowpass -2 15000 | ./fx2lpddsfm --carrier 45M --format float --channels 2 -vv\n");
	fprintf(stderr, "This example will play the file \"sample.wav\" with the default frequency deviation, carrier at 45MHz, stereo modulation and verbose output.\n");
}

double getFundamental(double fo, double fc)
{
	double nyquist = fc / 2.0;

	double fundamental = fmod(fo , fc);
	if (fundamental > nyquist)
	{
		fundamental = fc - fundamental;
	}
	return fundamental;
}

double getsinxxdb(double fo, double fc)
{
	double x = M_PI * fo / fc;
	return  log10(fabs(sin(x) / x)) * 20.0;
}

void printImages(double fundamental, double fc, double carrier)
{
	char s[32];
	printf("Images:\n");
	for (int i = 0; i < 40; i++)
	{
		double image = (double) ((i + 2) / 2) * fc +
				(((i % 2) == 0) ? (-fundamental) : (fundamental));
		int isCarrier = fabs(carrier - image) < 1.0;
		printf("%2d: %s%sHz @ %5.1fdb%s", i + 1,
				isCarrier ? "\E[1m\E[4m" : "",
				toMetricPrefixString(s, sizeof(s), image, 12, 6), getsinxxdb(image, fc),
				isCarrier ? "\E[0m" : ""
				);
		if ((i % 4) == 3)
		{
			printf("\n");
		}
		else
		{
			printf("   ");
		}
	}
}

int main(int argc, char **argv)
{
	libusb_device_handle* hndl;

	double deviation = 75000.0;
	double reference = 0.0;
	double carrier = 0.0;
	double sampleRate = 0.0;
	double ifClock = 0.0;
	uint64_t totalSamples = 0;
	int channels = 1;

	int nBuf = 3276;
	int bufferSize = 5 * nBuf;

	unsigned char buffer[bufferSize];
	int transferred;

	int renum = 0;
	char *firmware = NULL;

	int verbose = 0;
	int fail = 0;

	char *format = "double";

	char s[32];

	int c;
	readFunction_t readFunction;

	while (1)
	{
		static struct option long_options[] =
		{
			{"help",      no_argument,       0, 'h'},
			{"verbose",   no_argument,       0, 'v'},
			{"renum",     no_argument,       0, 'n'},
			{"firmware",  required_argument, 0, 'u'},
			{"carrier",   required_argument, 0, 'f'},
			{"ddsclock",  required_argument, 0, 'k'},
			{"channels",  required_argument, 0, 'c'},
			{"deviation", required_argument, 0, 'd'},
			{"format",    required_argument, 0, 't'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hvnf:r:c:d:u:t:",
				long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
			case 'h':
				print_syntax(argv[0]);
				return 0;

			case 'v':
				verbose++;
				break;

			case 'k':
				reference = parseMetricPrefix(optarg);
				if (isnan(reference))
				{
					return 1;
				}
				break;

			case 'n':
				renum = 1;
				break;

			case 'u':
				firmware = strdup(optarg);
				break;

			case 'f':
				carrier = parseMetricPrefix(optarg);
				if (isnan(carrier))
				{
					return 1;
				}
				break;

			case 'c':
				channels = atoi(optarg);
				break;

			case 'd':
				deviation = parseMetricPrefix(optarg);
				if (isnan(deviation))
				{
					return 1;
				}
				break;
				
			case 't':
				format = optarg;
				break;

			default:
				print_syntax(argv[0]);
				return 1;
		}
	}

	int fd = 0;
	if (optind < argc)
	{
		char* filename = argv[optind];
		fd = open(filename, O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr, "%s: %s\n", filename, strerror(errno));
			return -1;
		}
	}

	readFunction = getReadFunction(format);
	if (readFunction == NULL)
	{
		fprintf(stderr, "Unknown input format: \"%s\"\n", format);
		return 1;
	}

	if (carrier == 0.0 && (!renum) && (firmware == NULL))
	{
		fprintf(stderr, "Carrier frequency must be set\n");
		return 1;
	}

	if (channels > 2 || channels < 1)
	{
		fprintf(stderr, "Channels can have a value of 1 or 2\n");
		return 1;
	}

	if (renum)
	{
		hndl = fx2lpdds_open();

		printf("Initiating re-numeration\n");
		int ret = libusb_control_transfer (hndl, 0, 0xc0, 0, 0, NULL, 0, 1000);

		if (firmware == NULL)
		{
			return 0;
		}
	}
	if (firmware != NULL)
	{
		if (renum)
		{
			printf("Waiting 750ms for renumeration to finish before upload\n");
			usleep(750000);
		}

		char s[4096];
		snprintf(s, sizeof(s), "cycfx2prog \"prg:%s\" run", firmware);
		puts(s);
		return system(s);
	}

	hndl = fx2lpdds_open();

	fx2lpdds_start(hndl, reference == 0.0, ifClock == 0.0);

//	signal(SIGINT, intHandler);
	struct sigaction action;
	sigemptyset (&action.sa_mask);
	action.sa_handler = intHandler;
	action.sa_flags = 0;
	sigaction (SIGINT, &action, NULL);

	if (reference == 0.0)
	{
		reference = (double) fx2lpdds_getReference(hndl);
	}
	ifClock = fx2lpdds_getIf(hndl);

	sampleRate = ifClock / 250.0;

	double fundamental = getFundamental(carrier, reference);

	if (verbose >= 1)
	{
		printf("sample rate  : %sHz\n", toMetricPrefixString(s, sizeof(s), sampleRate, 0, 6));
		printf("reference   :  %sHz\n", toMetricPrefixString(s, sizeof(s), reference, 0, 6));
		printf("deviation    : %sHz\n", toMetricPrefixString(s, sizeof(s), deviation, 0, 6));
		printf("carrier      : %sHz @ %0.1fdb\n", toMetricPrefixString(s, sizeof(s), carrier, 0, 6), getsinxxdb(carrier, reference));
		printf("fundamental  : %sHz @ %0.1fdb\n", toMetricPrefixString(s, sizeof(s), fundamental, 0, 6), getsinxxdb(fundamental, reference));
		printf("channels     : %d (%s)\n", channels, channels == 1 ? "mono" : "stereo");
		printf("input format : %s\n", format);
	}
	if (verbose >= 2)
	{
		printImages(fundamental, reference, carrier); 
	}

	double doubleBuffer[nBuf * channels];

	initModulators();

	int count = 0;
	while (!quit)
	{
		int nSamples;
		if (readFunction(fd, doubleBuffer, nBuf * channels, &nSamples) == -1)
		{
			fail = 1;
			quit = 1;
		}

		if (nSamples == 0)
		{
			quit = 1;
			break;
		}

		if (channels == 1)
		{
			makeMonoBuffer(doubleBuffer, nSamples, reference, carrier, deviation, buffer);
		}
		else
		{
			makeStereoBuffer(doubleBuffer, nSamples / 2, totalSamples, sampleRate, reference, carrier, deviation, buffer);
		}

		totalSamples += nBuf;

		if (verbose >= 1)
		{
			printf("\rt=%0.3f (buffer %d)", (double) totalSamples / sampleRate, count++);
		}
		fflush(stdout);
		int rv=libusb_bulk_transfer(hndl,
				0x02,
				(unsigned char*) buffer,
				sizeof(buffer),
				&transferred,
				10000);
		if(rv)
		{
			printf ( "OUT2 Transfer failed: %d\n", rv );
			quit=1;
			return -1;
		}
	}

	if (verbose >= 1)
	{
		printf("done\n");
	}

	fx2lpdds_stop(hndl);

	return fail;
}

