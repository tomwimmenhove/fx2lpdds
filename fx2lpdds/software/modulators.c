#include <stdint.h>
#include <math.h>

#include "fx2lpddscomm.h"

struct StereoSample
{
	double left;
	double right;
};

#define FM_LEFTPLUSRIGHT_GAIN 0.45
#define FM_PILOT_GAIN 0.1
#define FM_LEFTMINUSRIGHT_GAIN 0.45
#define FM_PILOT_FREQ_HZ 19000.0
#define FM_SUBCARRIER_FREQ_HZ 38000.0

#define SINETABLESIZE 4096
double sineTable[SINETABLESIZE];

void initModulators()
{
	for (int i= 0; i < SINETABLESIZE; i++)
	{
		double w = (double) i * 2.0 * M_PI / SINETABLESIZE;
		sineTable[i] = sin(w);
	}
}

void makeStereoBuffer(double* audioBuffer, int nSamples, uint64_t totalSamples, double sampleRate, double reference, double carrier, double deviation, unsigned char* buffer)
{
	struct StereoSample* stereoBuffer = (struct StereoSample*) audioBuffer;

	double referenceMul = (double) 4294967296.0 / reference;

	double pilotMul = (double) SINETABLESIZE * FM_PILOT_FREQ_HZ / sampleRate;
	double subCarrierMul = (double) SINETABLESIZE * FM_SUBCARRIER_FREQ_HZ / sampleRate;

	for (int i = 0; i < nSamples; i++)
	{
		double left  = stereoBuffer[i].left;
		double right = stereoBuffer[i].right;

		/* Mix left and right */
		/* XXX: Something seems wrong. Halving the leftPlusRight gain and NOT halving leftMinusRight seems to give best separation. */
		double leftPlusRight = (left + right) * FM_LEFTMINUSRIGHT_GAIN * 0.5;
		double leftMinusRight = (left - right) * FM_LEFTPLUSRIGHT_GAIN;// * 0.5;

		/* Pilot tone */
		double pilotSample = sineTable[(uint32_t) ((double) totalSamples * pilotMul) % SINETABLESIZE] * FM_PILOT_GAIN;

		/* (left - right) modulated channel */
		double subCarrier = sineTable[(uint32_t) ((double) totalSamples * subCarrierMul) % SINETABLESIZE];
		double sideBand = subCarrier * FM_LEFTMINUSRIGHT_GAIN * leftMinusRight;

		/* Sum up the left and right channel, the pilot and the (left - right) modulated channel */
		double sum = leftPlusRight + pilotSample + sideBand;

		/* Calculate frequency */
		double f = carrier + sum * deviation;

		makeDdsWord(buffer, (uint32_t) (f * referenceMul + 0.5));
		//makeDdsWord(buffer, (uint32_t) (f * referenceMul));

		buffer +=5;

		totalSamples++;
	}
}

void makeMonoBuffer(double* audioBuffer, int nSamples, double reference, double carrier, double deviation, unsigned char* buffer)
{
	int bufferPos = 0;

	double referenceMul = (double) 4294967296.0 / reference;

	for (int i = 0; i < nSamples; i++)
	{
		/* Calculate frequency */
		double f = carrier + audioBuffer[i] * deviation;

		makeDdsWord(buffer + bufferPos, (uint32_t) (f * referenceMul + 0.5));
		//makeDdsWord(buffer + bufferPos, (uint32_t) f * referenceMul);
		bufferPos +=5;
	}
}

