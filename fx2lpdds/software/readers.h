#ifndef READERS_H
#define READERS_H

int readDoubles(int fd, double* buffer, int nSamples, int* nSamplesRead);
int readFloats(int fd, double* buffer, int nSamples, int* nSamplesRead);
int readSigned16BitBE(int fd, double* buffer, int nSamples, int* nSamplesRead);
int readSigned16BitLE(int fd, double* buffer, int nSamples, int* nSamplesRead);

typedef int (*readFunction_t)(int fd, double* buffer, int nSamples, int* nSamplesRead);

readFunction_t getReadFunction(char* name);

#endif /* READERS_H */
