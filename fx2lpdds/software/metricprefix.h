#ifndef METRICPREFIX_H
#define METRICPREFIX_H

double parseMetricPrefix(char* s);
char* toMetricPrefixString(char* buffer, int bufSize, double x, int align, int prec);

#endif /* METRICPREFIX_H */
