/*
 * Voice Changer for Asterisk 1.6+
 * Copyright (C) 2005-2010 J.A. Roberts Tunney
 *
 * J.A. Roberts Tunney <jtunney@lobstertech.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License version 2.0.
 *
 */

#include "voicechanger.h"

#include <soundtouch/SoundTouch.h>
using soundtouch::SoundTouch;

void *vc_soundtouch_create(int rate, float pitch)
{
	SoundTouch *st;
	st = new SoundTouch();
	if (st) {
		st->setChannels(1);
		st->setSampleRate(rate);
		st->setPitchSemiTones(pitch);
		st->setSetting(SETTING_USE_QUICKSEEK, 1);
		st->setSetting(SETTING_USE_AA_FILTER, 1);
	}
	return st;
}

void vc_soundtouch_free(void *st)
{
	if (st != NULL) {
		delete (SoundTouch *)st;
	}
}

/* convert array of pcm shorts (-32k - 32k) to floats (-1.0 - 1.0) */
static inline void slin_to_flin(float flin[],
				const int16_t __restrict slin[],
				int samps)
{
	int n;
	for (n = 0; n < samps; n++) {
		flin[n] = (float)slin[n] / 32767.0;
	}
}

/* convert array of pcm floats (-1.0 - 1.0) to shorts (-32k - 32k) */
static inline void flin_to_slin(int16_t __restrict slin[],
				const float flin[],
				int samps)
{
	int n;
	for (n = 0; n < samps; n++) {
		slin[n] = (int16_t)(flin[n] * 32767.0);
	}
}

void vc_voice_change(void *st_, float *fbuf, int16_t *data,
		     int samples, int datalen)
{
	SoundTouch *st = (SoundTouch *)st_;

#if defined(INTEGER_SAMPLES)

	st->putSamples(data, samples);
	if (st->numSamples() >= samples) {
		st->receiveSamplesEx(data, samples);
	} else {
		memset(data, 0, datalen);
	}

#elif defined(FLOAT_SAMPLES)

	slin_to_flin(fbuf, data, samples);
	st->putSamples(fbuf, samples);
	if ((int)st->numSamples() >= samples) {
		st->receiveSamples(fbuf, samples);
		flin_to_slin(data, fbuf, samples);
	} else {
		memset(data, 0, datalen);
	}

#else
#  error "unknown soundtouch sample type"
#endif
}

/* For Emacs:
 * Local Variables:
 * indent-tabs-mode:t
 * tab-width:8
 * c-basic-offset:8
 * c-file-style: nil
 * End:
 * For VIM:
 * vim:set softtabstop=8 shiftwidth=8 tabstop=8:
 */
