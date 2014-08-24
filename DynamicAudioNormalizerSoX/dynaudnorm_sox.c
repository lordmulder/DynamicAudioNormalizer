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

//Dynamic Audio Normalizer
#include <DynamicAudioNormalizer.h>

#define PRINT(X, ...) fprintf(stderr, X, __VA_ARGS__)

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

static void deinterlave(double **temp, const sox_sample_t *const in, const size_t samples_per_channel, const size_t channels)
{
	size_t in_pos = 0;

	for(size_t c = 0; c < channels; c++)
	{
		for(size_t i = 0; i < samples_per_channel; i++)
		{
			temp[c][i] = ((double)in[in_pos++]) / ((double)SOX_INT32_MAX);
		}
	}
}

static void interlave(sox_sample_t *const out, const double *const *const temp, const size_t samples_per_channel, const size_t channels)
{
	size_t out_pos = 0;

	for(size_t c = 0; c < channels; c++)
	{
		for(size_t i = 0; i < samples_per_channel; i++)
		{
			out[out_pos++] = (sox_sample_t) round(temp[c][i] * ((double)SOX_INT32_MAX));
		}
	}
}

// =============================================================================
// SoX Filter API
// =============================================================================

static int lsx_kill(sox_effect_t *effp)
{
	PRINT("[DYNAUDNORM] lsx_kill()\n");
	priv_t * p = (priv_t *)effp->priv;
	return SOX_SUCCESS;
}

static int create(sox_effect_t *effp, int argc, char **argv)
{
	PRINT("[DYNAUDNORM] create(%u)\n", effp->flow);
	memset(effp->priv, 0, sizeof(priv_t));
	return SOX_SUCCESS;
}

static int stop(sox_effect_t *effp)
{
	PRINT("[DYNAUDNORM] stop(%u)\n", effp->flow);
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

static int start(sox_effect_t *effp)
{
	PRINT("[DYNAUDNORM] start(%u)\n", effp->flow);
	PRINT("[DYNAUDNORM] --> Flows: %u\n", effp->flows);
	PRINT("[DYNAUDNORM] --> Rate:  %.2f\n", effp->in_signal.rate);
	PRINT("[DYNAUDNORM] --> Chans: %u\n", effp->in_signal.channels);

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

static int flow(sox_effect_t *effp, const sox_sample_t *ibuf, sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
	priv_t *const p = (priv_t *)effp->priv;
	const size_t samples_per_channel = min(p->tempSize, min((*isamp), (*osamp)) / effp->in_signal.channels);

	deinterlave(p->temp, ibuf, samples_per_channel, effp->in_signal.channels);

	int64_t output_samples = p->tempSize;
	MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(p->instance, p->temp, samples_per_channel, &output_samples);
	
	if(output_samples > 0)
	{
		interlave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		*osamp = (size_t)(output_samples * effp->in_signal.channels);
	}
	else
	{
		*osamp = 0;
	}

	return SOX_SUCCESS;
}

static int drain(sox_effect_t * effp, sox_sample_t * obuf, size_t * osamp)
{
	PRINT("[DYNAUDNORM] drain(%u)\n", effp->flow);

	priv_t *const p = (priv_t *)effp->priv;
	const size_t samples_per_channel = min(p->tempSize, (*osamp) / effp->in_signal.channels);

	int64_t output_samples = p->tempSize;
	MDYNAMICAUDIONORMALIZER_FUNCTION(flushBuffer)(p->instance, p->temp, samples_per_channel, &output_samples);

	if(output_samples > 0)
	{
		interlave(obuf, p->temp, ((size_t)output_samples), effp->in_signal.channels);
		*osamp = (size_t)(output_samples * effp->in_signal.channels);
	}
	else
	{
		*osamp = 0;
	}

	return SOX_SUCCESS;
}

sox_effect_handler_t const * lsx_dynaudnorm_effect_fn(void)
{
	static sox_effect_handler_t handler =
	{
		"dynaudnorm", "{length}", SOX_EFF_MCHAN/*| SOX_EFF_MODIFY*/,
		create, start, flow, drain, stop, lsx_kill, sizeof(priv_t)
	};
	return &handler;
}
