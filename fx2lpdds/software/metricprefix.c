#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "metricprefix.h"

double parseMetricPrefix(char* s)
{
	int len = strlen(s);

	if (len == 0)
	{
		return NAN;
	}

	unsigned char lastChar = s[len - 1];
	double mul = 1.0;

	if (isalpha(lastChar))
	{
		s[len - 1] = 0;
		len --;

		switch(lastChar)
		{
			case 'f': // femto
				mul = 1e-15f;
				break;
			case 'p': // pico
				mul = 1e-12f;
				break;
			case 'n': // nano
				mul = 1e-9f;
				break;
			case 'u': // micro
				mul = 1e-6f;
				break;
			case 'm': // mili
				mul = 1e-3f;
				break;
			case 'k': // kilo
				mul = 1e3f;
				break;
			case 'M': // mega
				mul = 1e6f;
				break;
			case 'G': // giga
				mul = 1e9f;
				break;
			case 'T': // tera
				mul = 1e12f;
				break;
			case 'P': // peta
				mul = 1e15f;
				break;
			default:
				fprintf(stderr, "Unknown metric prefix character: '%c'\n", lastChar);
				return NAN;
		}
	}

	char *endptr;
	double x = strtod(s, &endptr);

	if (endptr != &s[len])
	{
		fprintf(stderr, "Invalid numerical value: \"%s\"\n", s);
		return NAN;
	}

	return x * mul;
}

char* toMetricPrefixString(char* buffer, int bufSize, double x, int align, int prec)
{
	double div = 1.0;
	char* prefix = (char*) "";

	align--;

	if (x >= 1e15f) // peta
	{
		prefix = (char*) "P";
		div = 1e15f;
	}
	else if (x >= 1e12f) // tera
	{
		prefix = (char*) "T";
		div = 1e12f;
	}
	else if (x >= 1e9f) // giga
	{
		prefix = (char*) "G";
		div = 1e9f;
	}
	else if (x >= 1e6f) // mega
	{
		prefix = (char*) "M";
		div = 1e6f;
	}
	else if (x >= 1e3f) // kilo
	{
		prefix = (char*) "k";
		div = 1e3f;
	}
	/* ... ONE ... */
	else if (x < 1e-12f) // femto
	{
		prefix = (char*) "f";
		div = 1e-15f;
	}
	else if (x < 1e-9f) // pico
	{
		prefix = (char*) "u";
		div = 1e-12f;
	}
	else if (x < 1e-6f) // nano
	{
		prefix = (char*) "n";
		div = 1e-9f;
	}
	else if (x < 1e-3f) // micro
	{
		prefix = (char*) "u";
		div = 1e-6f;
	}
	else if (x < 1.0f) // mili
	{
		prefix = (char*) "m";
		div = 1e-3f;
	}
	else
	{
		/* No prefix */
		align++;
	}

	if (align < 0)
	{
		align = 0;
	}

	char format[16];
	snprintf(format, sizeof(format), "%%%d.%df%s", align, prec, prefix);
	snprintf(buffer, bufSize, format, x / div);

	return buffer;
}

