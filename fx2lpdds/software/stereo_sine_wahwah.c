#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#define SINETABLESIZE 4096
double sineTable[SINETABLESIZE];

void precalcSineTable()
{
	for (int i= 0; i < SINETABLESIZE; i++)
	{
		double w = (double) i * 2.0 * M_PI / SINETABLESIZE;
		sineTable[i] = sin(w);
	}
}

struct StereoSample
{
	double left;
	double right;
};

#define WAHWAH_FREQ 0.5
#define TONE1 1000.0
#define TONE2 1500.0
#define SAMPLE_RATE 192000.0

int main(int argc, char **argv)
{
	int nBuf = 3276;

	struct StereoSample audioBuf[nBuf];

	precalcSineTable();

	double wahwahMul = (double) SINETABLESIZE * WAHWAH_FREQ / SAMPLE_RATE;
	double tone1Mul = (double) SINETABLESIZE * TONE1 / SAMPLE_RATE;
	double tone2Mul = (double) SINETABLESIZE * TONE2 / SAMPLE_RATE;

	uint64_t totalSamples = 0;

	for (;;)
	{
		for (int i = 0; i < nBuf; i++)
		{
			double wahwahSample = sineTable[(uint32_t) ((double) totalSamples * wahwahMul) % SINETABLESIZE] + 1.0;
			double tone1Sample = sineTable[(uint32_t) ((double) totalSamples * tone1Mul) % SINETABLESIZE] * .5;
			double tone2Sample = sineTable[(uint32_t) ((double) totalSamples * tone2Mul) % SINETABLESIZE] * .5;

			audioBuf[i].left = wahwahSample * tone1Sample / 2.0;
			audioBuf[i].right = (2.0 - wahwahSample) * tone1Sample / 2.0;

			audioBuf[i].left += (2.0 - wahwahSample) * tone2Sample / 2.0;
			audioBuf[i].right += wahwahSample * tone2Sample / 2.0;

			totalSamples++;
		}

		int r = write(1, audioBuf, sizeof(audioBuf));

		if (r < 0)
		{
			fprintf(stderr, "write(): %s\n", strerror(errno));
			return -1;
		}

		if (r == 0)
		{
			return 0;
		}
	}

	return 0;
}

