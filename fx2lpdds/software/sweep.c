#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		fprintf(stderr, "Usage: %s <sweeptime in samples> <mutiplier (like .5 for 50%% gain)> <offset (line -.25 for 50%% gain)\n", argv[0]);
		return 1;
	}

	int nSamples = atoi(argv[1]);
	double multiplier = atof(argv[2]);
	double offset = atof(argv[3]);

	int bufSize = 3276;

	double buffer[bufSize];

	int bufPos = 0;

	for (int i = 0;;)
	{
		double sample = ((double) i  * multiplier) / nSamples + offset;
		if (sample > 1.0)
		{
			fprintf(stderr, "OVER CLIP!\n");
			exit(1);
		}
		if (sample < -1.0)
		{
			fprintf(stderr, "UNDER CLIP!\n");
			exit(1);
		}
		buffer[bufPos++] = sample;

		if (bufPos >= bufSize)
		{
			if (write(1, buffer, sizeof(buffer)) == -1)
			{
				fprintf(stderr, "write(): %s\n", strerror(errno));
				return 1;
			}

			bufPos = 0;
		}

		i++;
		if (i >= nSamples)
		{
			i = 0;
		}
	}

	return 0;
}
