//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Effect Wrapper for SoX
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

//SoX internal stuff
#include "sox_i.h"

//StdLib
#include <string.h>
#include <stdarg.h>

//Dynamic Audio Normalizer
#include <DynamicAudioNormalizer.h>

//Linkage
#ifdef MDYNAMICAUDIONORMALIZER_STATIC
	static const char *LINKAGE = "Static";
#else
	static const char *LINKAGE = "Shared";
#endif

// =============================================================================
// Private Data
// =============================================================================

typedef struct
{
	MDynamicAudioNormalizer_Handle *instance;
	double **temp;
	size_t tempSize;
}
priv_t;

// =============================================================================
// Internal Functions
// =============================================================================

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

// =============================================================================
// SoX Callback Functions
// =============================================================================

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
	lsx_report("--> flows=%u",              effp->flows);
	lsx_report("--> flow=%u",               effp->flow);
	lsx_report("--> in_signal.rate=%.2f",   effp->in_signal.rate);
	lsx_report("--> in_signal.channels=%u", effp->in_signal.channels);

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
	p->tempSize = (size_t) round(effp->in_signal.rate * 10.0);

	p->instance = MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance)
	(
		effp->in_signal.channels,
		(uint32_t) round(effp->in_signal.rate),
		500,
		31,
		0.95,
		10.0,
		0.0,
		0.0,
		1,
		0,
		0,
		NULL
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
	const size_t samples_per_channel = min(p->tempSize, min((*isamp), (*osamp)) / effp->in_signal.channels);

	dynaudnorm_deinterleave(p->temp, ibuf, samples_per_channel, effp->in_signal.channels);

	int64_t output_samples = p->tempSize;
	MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(p->instance, p->temp, samples_per_channel, &output_samples);

	if(output_samples > 0)
	{
		dynaudnorm_interleave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		*osamp = (size_t)(output_samples * effp->in_signal.channels);
	}
	else
	{
		*osamp = 0;
	}

	return SOX_SUCCESS;
}

static int dynaudnorm_drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
	lsx_debug("dynaudnorm_drain()");
	priv_t *const p = (priv_t *)effp->priv;
	const size_t samples_per_channel = min(p->tempSize, (*osamp) / effp->in_signal.channels);

	int64_t output_samples = p->tempSize;
	MDYNAMICAUDIONORMALIZER_FUNCTION(flushBuffer)(p->instance, p->temp, samples_per_channel, &output_samples);

	if(output_samples > 0)
	{
		dynaudnorm_interleave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		*osamp = (size_t)(output_samples * effp->in_signal.channels);
	}
	else
	{
		*osamp = 0;
	}

	return SOX_SUCCESS;
}

// =============================================================================
// SoX Public API
// =============================================================================

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
		"  -f --frame-len <value>   Frame length, in milliseconds",
		"  -g --gauss-size <value>  Gauss filter size, in frames",
		"  -p --peak <value>        Target peak magnitude, 0.1-1.0",
		"  -m --max-gain <value>    Maximum gain factor",
		"  -r --rms-mode <value>    Target RMS value",
		"  -n --no-coupling         Disable channel coupling",
		"  -c --correct-dc          Enable the DC bias correction",
		"  -b --alt-boundary        Use alternative boundary mode",
		"  -s --compress <value>    Compress the input data",
		"",
		"Diagnostics:",
		"  -v --verbose             Output additional diagnostic info",
		"  -l --log-file <file>     Create a log file",
		"",
		"",
		"Please refer to the manual for a detailed explanation!"
	};
	
	static char *usage;
	handler.usage = lsx_usage_lines(&usage, lines, array_length(lines));

	return &handler;
}
