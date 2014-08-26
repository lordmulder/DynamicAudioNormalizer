/* ================================================================================== */
/* Dynamic Audio Normalizer - Effect Wrapper for SoX                                  */
/* Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.             */
/*                                                                                    */
/* This library is free software; you can redistribute it and/or                      */
/* modify it under the terms of the GNU Lesser General Public                         */
/* License as published by the Free Software Foundation; either                       */
/* version 2.1 of the License, or (at your option) any later version.                 */
/*                                                                                    */
/* This library is distributed in the hope that it will be useful,                    */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                  */
/* Lesser General Public License for more details.                                    */
/*                                                                                    */
/* You should have received a copy of the GNU Lesser General Public                   */
/* License along with this library; if not, write to the Free Software                */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA     */
/*                                                                                    */
/* http://www.gnu.org/licenses/lgpl-2.1.txt                                           */
/* ================================================================================== */

/*Shut up warnings*/
#define _CRT_SECURE_NO_WARNINGS

/*SoX internal stuff*/
#include "sox_i.h"
#include "unicode_support.h"

/*StdLib*/
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/*Linkage*/
#if defined(_MSC_VER) && defined(_MT)
#define MDYNAMICAUDIONORMALIZER_STATIC
static const char *LINKAGE = "Static";
#else
static const char *LINKAGE = "Shared";
#endif

/*Dynamic Audio Normalizer*/
#include <DynamicAudioNormalizer.h>

/* ================================================================================== */
/* Private Data                                                                       */
/* ================================================================================== */

typedef struct
{
	uint32_t frameLenMsec;
	uint32_t filterSize;
	double peakValue;
	double maxAmplification;
	double targetRms;
	double compressThresh;
	int channelsCoupled;
	int enableDCCorrection;
	int altBoundaryMode;
	FILE *logFile;
}
settings_t;

typedef struct
{
	settings_t settings;
	MDynamicAudioNormalizer_Handle *instance;
	double **temp;
	size_t tempSize;
}
priv_t;

/* ================================================================================== */
/* Internal Functions                                                                 */
/* ================================================================================== */

static int parseArgInt(const char *const str, uint32_t *parameter, const uint32_t min_val, const uint32_t max_val)
{
	uint32_t temp;
	if(sscanf(str, "%u", &temp) == 1)
	{
		*parameter = max(min_val, min(max_val, temp));
		return 1;
	}
	lsx_fail("Failed to parse integral value `%s'", str);
	return 0;
}

static int parseArgDbl(const char *const str, double *parameter, const double min_val, const double max_val)
{
	double temp;
	if(sscanf(str, "%lf", &temp) == 1)
	{
		*parameter = max(min_val, min(max_val, temp));
		return 1;
	}
	lsx_fail("Failed to parse floating point value `%s'", str);
	return 0;
}

#define TRY_PARSE(TYPE, PARAM, MIN, MAX) do \
{ \
	if(!parseArg##TYPE(optstate.arg, &(PARAM), (MIN), (MAX))) \
	{ \
		return 0; \
	} \
} \
while(0)

static void dynaudnorm_defaults(settings_t *settings)
{
	memset(settings, 0, sizeof(settings_t));

	settings->frameLenMsec       = 500;
	settings->filterSize         =  31;
	settings->peakValue          =   0.95;
	settings->maxAmplification   =  10.0;
	settings->targetRms          =   0.0;
	settings->compressThresh     =   0.0;
	settings->channelsCoupled    =   1;
	settings->enableDCCorrection =   0;
	settings->altBoundaryMode    =   0;
}

static int dynaudnorm_parse_args(settings_t *settings, int argc, char **argv)
{
	static const char *const opts = "+f:g:p:m:r:ncbs:l:";
	lsx_getopt_t optstate; char c;
	lsx_getopt_init(argc, argv, opts, NULL, lsx_getopt_flag_opterr, 1, &optstate);

	while((c = lsx_getopt(&optstate)) != -1)
	{
		switch(tolower(c))
		{
		case 'f':
			TRY_PARSE(Int, settings->frameLenMsec, 10, 8000);
			break;
		case 'g':
			TRY_PARSE(Int, settings->filterSize, 3, 301);
			settings->filterSize += ((settings->filterSize + 1) % 2);
			break;
		case 'p':
			TRY_PARSE(Dbl, settings->peakValue, 0.0, 1.0);
			break;
		case 'm':
			TRY_PARSE(Dbl, settings->maxAmplification, 1.0, 100.0);
			break;
		case 'r':
			TRY_PARSE(Dbl, settings->targetRms, 0.0, 1.0);
			break;
		case 'n':
			settings->channelsCoupled = 0;
			break;
		case 'c':
			settings->enableDCCorrection = 1;
			break;
		case 'b':
			settings->altBoundaryMode = 1;
			break;
		case 's':
			TRY_PARSE(Dbl, settings->compressThresh, 0.0, 1.0);
			break;
		case 'l':
			if(!settings->logFile)
			{
				settings->logFile = lsx_fopen(optstate.arg, "w");
				if(!settings->logFile)
				{
					lsx_warn("Failed to open logfile `%s'", optstate.arg);
				}
			}
			break;
		default:
			return 0;
		}
	}

	return 1;
}

static void dynaudnorm_deinterleave(double **temp, const sox_sample_t *const in, const size_t samples_per_channel, const size_t channels)
{
	size_t in_pos = 0;

	for(size_t i = 0; i < samples_per_channel; i++)
	{
		for(size_t c = 0; c < channels; c++)
		{
			temp[c][i] = ((double)in[in_pos++]) / ((double)SOX_INT32_MAX);
		}
	}
}

static void dynaudnorm_interleave(sox_sample_t *const out, const double *const *const temp, const size_t samples_per_channel, const size_t channels)
{
	size_t out_pos = 0;

	for(size_t i = 0; i < samples_per_channel; i++)
	{
		for(size_t c = 0; c < channels; c++)
		{
			out[out_pos++] = (sox_sample_t) round(min(1.0, max(-1.0, temp[c][i])) * ((double)SOX_INT32_MAX));
		}
	}
}

static void dynaudnorm_print(sox_effect_t *effp, const char *const fmt, ...)
{
	if(effp->global_info->global_info->output_message_handler)
	{
		va_list arg_list;
		va_start(arg_list, fmt);
		vfprintf(stderr, fmt, arg_list);
		va_end(arg_list);
	}
}

void dynaudnorm_log(const int logLevel, const char *const message)
{
	switch(logLevel)
	{
	case 0:
		lsx_report("%s", message);
		break;
	case 1:
		lsx_warn("%s", message);
		break;
	case 2:
		lsx_fail("%s", message);
		break;
	}
}

void dynaudnorm_update_buffsize(const sox_effect_t *const effp, priv_t *const p, const size_t input_samples)
{
	if(input_samples > p->tempSize)
	{
		lsx_warn("Increasing buffer size: %u -> %u", p->tempSize, input_samples);
		for(size_t c = 0; c < effp->in_signal.channels; c++)
		{
			p->temp[c] = lsx_realloc(p->temp[c], input_samples * sizeof(double));
		}
		p->tempSize = input_samples;
	}
}

/* ================================================================================== */
/* SoX Callback Functions                                                             */
/* ================================================================================== */

static int dynaudnorm_kill(sox_effect_t *effp)
{
	lsx_report("dynaudnorm_kill()");
	priv_t * p = (priv_t *)effp->priv;
	return SOX_SUCCESS;
}

static int dynaudnorm_create(sox_effect_t *effp, int argc, char **argv)
{
	lsx_report("dynaudnorm_create()");

	memset(effp->priv, 0, sizeof(priv_t));
	priv_t *const p = (priv_t *)effp->priv;
	dynaudnorm_defaults(&p->settings);

	if(!dynaudnorm_parse_args(&p->settings, argc, argv))
	{
		return lsx_usage(effp);
	}

	return SOX_SUCCESS;
}

static int dynaudnorm_stop(sox_effect_t *effp)
{
	lsx_report("dynaudnorm_stop()");
	priv_t *const p = (priv_t *)effp->priv;

	if(p->instance)
	{
		MDYNAMICAUDIONORMALIZER_FUNCTION(destroyInstance)(&p->instance);
		p->instance = NULL;
	}

	if(p->settings.logFile)
	{
		fclose(p->settings.logFile);
		p->settings.logFile = NULL;
	}

	if(p->temp)
	{
		for(size_t c = 0; c < effp->in_signal.channels; c++)
		{
			free(p->temp[c]);
			p->temp[c] = NULL;
		}
		free(p->temp);
		p->temp = NULL;
	}

	return SOX_SUCCESS;
}

static int dynaudnorm_start(sox_effect_t *effp)
{
	lsx_report("dynaudnorm_start()");
	lsx_report("flows=%u, flow=%u, in_signal.rate=%.2f, in_signal.channels=%u", effp->flows, effp->flow, effp->in_signal.rate, effp->in_signal.channels);

	if((effp->flow == 0) && (effp->global_info->global_info->verbosity > 1))
	{
		uint32_t versionMajor, versionMinor, versionPatch;
		MDYNAMICAUDIONORMALIZER_FUNCTION(getVersionInfo)(&versionMajor, &versionMinor, &versionPatch);

		const char *buildDate, *buildTime, *buildCompiler, *buildArch; int buildDebug;
		MDYNAMICAUDIONORMALIZER_FUNCTION(getBuildInfo)(&buildDate, &buildTime, &buildCompiler, &buildArch, &buildDebug);

		dynaudnorm_print(effp, "\n---------------------------------------------------------------------------\n");
		dynaudnorm_print(effp, "Dynamic Audio Normalizer (SoX Wrapper), Version %u.%02u-%u, %s\n", versionMajor, versionMinor, versionPatch, LINKAGE);
		dynaudnorm_print(effp, "Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n");
		dynaudnorm_print(effp, "Built on %s at %s with %s for %s.\n\n", buildDate, buildTime, buildCompiler, buildArch);
		dynaudnorm_print(effp, "This program is free software: you can redistribute it and/or modify\n");
		dynaudnorm_print(effp, "it under the terms of the GNU General Public License <http://www.gnu.org/>.\n");
		dynaudnorm_print(effp, "Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n");
		dynaudnorm_print(effp, "---------------------------------------------------------------------------\n\n");
	}

	priv_t *const p = (priv_t *)effp->priv;
	p->tempSize = (size_t) max(ceil(effp->in_signal.rate), 8192.0); /*initial buffer size is one second*/

	p->instance = MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance)
	(
		effp->in_signal.channels,
		(uint32_t) round(effp->in_signal.rate),
		p->settings.frameLenMsec,
		p->settings.filterSize,
		p->settings.peakValue,
		p->settings.maxAmplification,
		p->settings.targetRms,
		p->settings.compressThresh,
		p->settings.channelsCoupled,
		p->settings.enableDCCorrection,
		p->settings.altBoundaryMode,
		p->settings.logFile
	);

	if(p->instance)
	{
		p->temp = (double**) lsx_calloc(effp->in_signal.channels, sizeof(double*));
		for(size_t c = 0; c < effp->in_signal.channels; c++)
		{
			p->temp[c] = (double*) lsx_calloc(p->tempSize, sizeof(double));
		}
	}

	return (p->instance) ? SOX_SUCCESS : SOX_EINVAL;
}

static int dynaudnorm_flow(sox_effect_t *effp, const sox_sample_t *ibuf, sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
	lsx_debug("dynaudnorm_flow()");
	priv_t *const p = (priv_t *)effp->priv;

	const size_t input_samples = min((*isamp), (*osamp)) / effp->in_signal.channels; /*this is per channel!*/
	int64_t output_samples = 0;
	dynaudnorm_update_buffsize(effp, p, input_samples);

	if(input_samples > 0)
	{
		dynaudnorm_deinterleave(p->temp, ibuf, input_samples, effp->in_signal.channels);
		if(!MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(p->instance, p->temp, input_samples, &output_samples))
		{
			return SOX_EOF;
		}
		if(output_samples > 0)
		{
			dynaudnorm_interleave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		}
	}


	*isamp = (size_t)(input_samples  * effp->in_signal.channels);
	*osamp = (size_t)(output_samples * effp->in_signal.channels);

	return SOX_SUCCESS;
}

static int dynaudnorm_drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
	lsx_debug("dynaudnorm_drain()");
	priv_t *const p = (priv_t *)effp->priv;

	const size_t input_samples = (*osamp) / effp->in_signal.channels; /*this is per channel!*/
	int64_t output_samples = 0;
	dynaudnorm_update_buffsize(effp, p, input_samples);

	if(input_samples > 0)
	{
		if(!MDYNAMICAUDIONORMALIZER_FUNCTION(flushBuffer)(p->instance, p->temp, input_samples, &output_samples))
		{
			return SOX_EOF;
		}
		if(output_samples > 0)
		{
			dynaudnorm_interleave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		}
	}
	else
	{
		lsx_warn("drain() was called with zero-size output buffer!");
	}

	*osamp = (size_t)(output_samples * effp->in_signal.channels);
	return SOX_SUCCESS;
}

/* ================================================================================== */
/* SoX Public API                                                                     */
/* ================================================================================== */

sox_effect_handler_t const * lsx_dynaudnorm_effect_fn(void)
{
	static sox_effect_handler_t handler =
	{
		"dynaudnorm", NULL, SOX_EFF_MCHAN,
		dynaudnorm_create, dynaudnorm_start, dynaudnorm_flow, dynaudnorm_drain, dynaudnorm_stop, dynaudnorm_kill, sizeof(priv_t)
	};

	static char const * lines[] =
	{
		"[options]",
		"",
		"Algorithm Tweaks:",
		"  -f <value>  Frame length, in milliseconds",
		"  -g <value>  Gauss filter size, in frames",
		"  -p <value>  Target peak magnitude, 0.1-1.0",
		"  -m <value>  Maximum gain factor",
		"  -r <value>  Target RMS value",
		"  -n          Disable channel coupling",
		"  -c          Enable the DC bias correction",
		"  -b          Use alternative boundary mode",
		"  -s          Compress the input data",
		"",
		"Diagnostics:",
		"  -l <file>   Create a log file",
		"",
		"",
		"Please refer to the manual for a detailed explanation!"
	};
	
	static char *usage;
	handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));

	MDYNAMICAUDIONORMALIZER_FUNCTION(setLogFunction)(dynaudnorm_log);
	return &handler;
}
