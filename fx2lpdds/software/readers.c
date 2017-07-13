#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "readers.h"

int readn(int fd, void* buffer, size_t len, size_t* nRead)
{
	int ret = 0;
	*nRead = 0;
	while (len)
	{
		int r = read(fd, buffer + (*nRead), len);
		if (r < 0)
		{
//			fprintf(stderr, "Read error: %s\n", strerror(errno));
			ret = -1; // Error while reading;;
			break;
		}

		if (r == 0)
		{
			ret = 0;
			break;
		}

		*nRead += r;
		len -= r;
	}

	return ret;
}

int readDoubles(int fd, double* buffer, int nSamples, int* nSamplesRead)
{
	double rBuf[nSamples];
	size_t nRead;

	int r = readn(fd, rBuf, sizeof(rBuf), &nRead);
	*nSamplesRead = nRead / sizeof(double);
	if (r == -1)
	{
		return -1;
	}

	for (int i = 0; i < *nSamplesRead; i++)
	{
		buffer[i] = (double) rBuf[i];
	}

	return 0;
}

int readFloats(int fd, double* buffer, int nSamples, int* nSamplesRead)
{
	float rBuf[nSamples];
	size_t nRead;

	int r = readn(fd, rBuf, sizeof(rBuf), &nRead);
	*nSamplesRead = nRead / sizeof(float);
	if (r == -1)
	{
		return -1;
	}

	for (int i = 0; i < *nSamplesRead; i++)
	{
		buffer[i] = (double) rBuf[i];
	}

	return 0;
}

int readSigned16BitBE(int fd, double* buffer, int nSamples, int* nSamplesRead)
{
	int16_t rBuf[nSamples];
	size_t nRead;

	int r = readn(fd, rBuf, sizeof(rBuf), &nRead);
	*nSamplesRead = nRead / sizeof(int16_t);
	if (r == -1)
	{
		return -1;
	}

	for (int i = 0; i < *nSamplesRead; i++)
	{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		buffer[i] = (double) __builtin_bswap16(rBuf[i]) / 32768.0f;
#else
		buffer[i] = (double) rBuf[i] / 32768.0f;
#endif
	}

	return 0;
}

int readSigned16BitLE(int fd, double* buffer, int nSamples, int* nSamplesRead)
{
	int16_t rBuf[nSamples];
	size_t nRead;

	int r = readn(fd, rBuf, sizeof(rBuf), &nRead);
	*nSamplesRead = nRead / sizeof(int16_t);
	if (r == -1)
	{
		return -1;
	}

	for (int i = 0; i < *nSamplesRead; i++)
	{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		buffer[i] = (double) rBuf[i] / 32768.0f;
#else
		buffer[i] = (double) __builtin_bswap16(rBuf[i]) / 32768.0f;
#endif
	}

	return 0;
}

readFunction_t getReadFunction(char* name)
{
	if (strcmp(name, "s16") == 0)
	{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		return readSigned16BitLE;
#else
		return readSigned16BitBE;
#endif
	}
	else if (strcmp(name, "s16le") == 0)
	{
		return readSigned16BitLE;
	}
	else if (strcmp(name, "s16be") == 0)
	{
		return readSigned16BitBE;
	}
	else if (strcmp(name, "float") == 0)
	{
		return readFloats;
	}
	else if (strcmp(name, "double") == 0)
	{
		return readDoubles;
	}

	return NULL;
}

