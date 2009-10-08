/*
 * Asterisk Voice Changer
 *
 * Copyright (C) 2005-2009 Lobstertech, Inc.
 *
 * J.A. Roberts Tunney <jtunney@lobstertech.com>
 * Clod Patry <cpatry@gmail.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License version 2.0.
 *
 * Keep it Open Source Pigs
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <asterisk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/audiohook.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>
#include <asterisk/lock.h>
#include <asterisk/cli.h>
#include <asterisk/options.h>
#include <asterisk/app.h>
#include <asterisk/linkedlists.h>
#include <asterisk/utils.h>
#include <soundtouch4c.h>

static const char *app = "VoiceChanger";
static const char *synopsis = "Adjusts the pitch of your voice";
static const char *desc = ""
"  VoiceChanger(<pitch>)\n\n"
"Specify a pitch in semitones.  Like -5 for deeper and 5 for higher.\n"
"";

static const char *stop_app = "StopVoiceChanger";
static const char *stop_synopsis = "Removes voice changer";
static const char *stop_desc = ""
"  StopVoiceChanger()\n\n"
"Removes the voice change effect from a channel, if there is any.\n"
"";

static soundtouch4c_t soundtouch_create(float pitch)
{
	soundtouch4c_t snd;

	ast_log(LOG_DEBUG, "Creating SoundTouch object...\n");
	snd = SoundTouch_construct();
	if (!snd) {
		ast_log(LOG_WARNING, "Failed to create SoundTouch object\n");
		return NULL;
	}

	SoundTouch_setChannels(snd, 1);
	SoundTouch_setSampleRate(snd, 8000);
	SoundTouch_setPitchSemiTonesFloat(snd, pitch);
	SoundTouch_setSetting(snd, SETTING_USE_QUICKSEEK, 1);
	SoundTouch_setSetting(snd, SETTING_USE_AA_FILTER, 1);

	return snd;
}

struct voice_changer {
	soundtouch4c_t st;
	struct ast_audiohook *ah;
};

static void voicechanger_free(void *data)
{
	struct voice_changer *vc;
	if (!data)
		return;
	vc = (struct voice_changer *)data;
	if (vc->ah) {
		ast_log(LOG_DEBUG, "Freeing AudioHook object...\n");
		ast_free(vc->ah);
	}
	if (vc->st) {
		ast_log(LOG_DEBUG, "Freeing SoundTouch object...\n");
		SoundTouch_destruct(vc->st);
	}
	ast_free(vc);
	ast_log(LOG_DEBUG, "Done!\n");
}

static const struct ast_datastore_info voice_change_ds = {
	.type    = "VoiceChanger",
	.destroy = voicechanger_free,
};

static int audio_callback(struct ast_audiohook *audiohook, struct ast_channel *chan,
			  struct ast_frame *frame, enum ast_audiohook_direction direction)
{
	struct ast_datastore *ds;
	struct voice_changer *vc;

	if (chan && frame && direction == AST_AUDIOHOOK_DIRECTION_READ) {
		ds = ast_channel_datastore_find(chan, &voice_change_ds, "VoiceChanger");
		if (!ds) {
			ast_log(LOG_WARNING, "Could not fetch voice changer datastore!\n");
			return -1;
		}
		vc = (struct voice_changer *)ds->data;
		if (!ds) {
			ast_log(LOG_WARNING, "SoundTouch object missing from datastore!\n");
			return -1;
		}
		SoundTouch_putSamples(vc->st, frame->data, frame->samples);
		memset(frame->data, 0, frame->datalen);
		SoundTouch_receiveSamplesEx(vc->st, frame->data, frame->samples);
	}

	return 0;
}

static int attach_audiohook(struct ast_channel *chan, struct ast_audiohook *audiohook) 
{
	struct ast_channel *peer;
	int rc;

	rc = ast_audiohook_attach(chan, audiohook);
	if (!rc && ast_test_flag(chan, AST_FLAG_NBRIDGE) && (peer = ast_bridged_channel(chan)))
		ast_softhangup(peer, AST_SOFTHANGUP_UNBRIDGE);

	return rc;
}

static int install_vc(struct ast_channel *chan, float pitch)
{
	struct ast_datastore *ds;
	struct voice_changer *vc;

	if (pitch < 0.1 && pitch > -0.1)
		return 0;

	/* create soundtouch object */
	vc = ast_calloc(1, sizeof(struct voice_changer));
	if (!(vc->st = soundtouch_create(pitch))) {
		voicechanger_free(vc);
		return -1;
	}

	/* create audiohook */
	ast_log(LOG_DEBUG, "Creating AudioHook object...\n");
	vc->ah = ast_calloc(1, sizeof(struct ast_audiohook));
	if (ast_audiohook_init(vc->ah, AST_AUDIOHOOK_TYPE_MANIPULATE, "VoiceChanger")) {
		voicechanger_free(vc);
		return -1;
	}
	vc->ah->manipulate_callback = audio_callback;
	if (attach_audiohook(chan, vc->ah) != 0) {
		ast_log(LOG_WARNING, "Unable to add '%s' audiohook to channel '%s'\n", "VoiceChanger", chan->name);
		voicechanger_free(vc);
		return -1;
	}

	/* attach these renegade pointers to the channel */
	ds = ast_channel_datastore_alloc(&voice_change_ds, "VoiceChanger");
	ds->data = vc;
	ast_channel_datastore_add(chan, ds);

	return 0;
}

static int voicechanger_exec(struct ast_channel *chan, void *data)
{
	int rc;
	struct ast_module_user *u;
	float pitch;

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "Voicechanger requires an argument (pitch)\n");
		return -1;
	}
	pitch = strtof(data, NULL);

	u = ast_module_user_add(chan);
	rc = install_vc(chan, pitch);
	ast_module_user_remove(u);

	return rc;
}

static int uninstall_vc(struct ast_channel *chan)
{
	struct ast_datastore *ds;
	struct voice_changer *vc;

	ds = ast_channel_datastore_find(chan, &voice_change_ds, "VoiceChanger");
	if (!ds) 
		return 0;
	vc = (struct voice_changer *)ds->data;

	ast_log(LOG_DEBUG, "Detaching Voice Changer from channel...\n");

	if (ast_audiohook_detach_source(chan, "VoiceChanger") != 0)
		ast_log(LOG_WARNING, "could not find audiohook :(\n");

	/* change uid of datastore so it doesn't conflict with voice changer instances */
	free(ds->uid);
	ds->uid = ast_malloc(32);
	snprintf(ds->uid, 32, "VOICHG_%d", (int)ast_random());

	return 0;
}

static int stop_voicechanger_exec(struct ast_channel *chan, void *data)
{
	int rc;
	struct ast_module_user *u;

	u = ast_module_user_add(chan);
	rc = uninstall_vc(chan);
	ast_module_user_remove(u);

	return rc;
}

static int unload_module(void)
{
	int res;
	res  = ast_unregister_application(app);
	res |= ast_unregister_application(stop_app);
	ast_module_user_hangup_all();
	return res;
}

static int load_module(void)
{
	int res;
	res  = ast_register_application(app, voicechanger_exec, synopsis, desc);
	res |= ast_register_application(stop_app, stop_voicechanger_exec, stop_synopsis, stop_desc);
	return res;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Hooked Voice Changer");
