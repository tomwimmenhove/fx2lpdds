#ifndef MODUATORS_H
#define MODULATORS_H

void initModulators();
void makeStereoBuffer(double* audioBuffer, int nSamples, uint64_t totalSamples, double sampleRate, double reference, double carrier, double deviation, unsigned char* buffer);
void makeMonoBuffer(double* audioBuffer, int nSamples, double reference, double carrier, double deviation, unsigned char* buffer);

#endif /* MODULATORS_H */
