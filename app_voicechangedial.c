/*
 * Voice Changer for Asterisk 1.2
 * Version 0.4
 *
 * Copyright (C) 2005-2006 J.A. Roberts Tunney
 *
 * J.A. Roberts Tunney <jtunney@lobstertech.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License version 2.0.
 *
 * Keep it Open Source Pigs
 *
 * Special Thanks:
 *
 * Claude Patry <cpatry@gmail.com>
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/frame.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>
#include <asterisk/lock.h>
#include <asterisk/options.h>
#include <asterisk/app.h>

#include <soundtouch4c.h>

static char *app = "VoiceChangeDial";
static char *tdesc = "Voice Changer Dial Application";
static char *synopsis = "Voice Changer Dial Application";
static char *desc = "\n"
"Usage VoiceChangeDial(dialstring[|options])\n"
"\n"
"Usage:\n"
"  This app fuctions pretty much the same way as Dial().  You may\n"
"  also change the pitch of your voice during an active conversation\n"
"  by pressing * to go down, # to go up.\n"
"\n"
"Options:\n"
"  h    -- Hangup if the call was successful\n"
"  r    -- Indicate 'ringing' to the caller\n"
"  p    -- Apply effect to peer channel instead\n"
"  P(f) -- Voice pitch in semitones.  Negative is lower, positive\n"
"          is higher.  Default is -5.0\n"
"  T(n) -- Dial timeout in seconds.  If not set, waits 60 sec.\n"
"          for other side to pickup\n"
;

#define APP_VOICECHANGEDIAL_HANGUP     (1 << 0)
#define APP_VOICECHANGEDIAL_RINGING    (1 << 1)
#define APP_VOICECHANGEDIAL_PITCH      (1 << 2)
#define APP_VOICECHANGEDIAL_TIMEOUT    (1 << 3)
#define APP_VOICECHANGEDIAL_PEEREFFECT (1 << 4)

enum {
	OPT_ARG_VOICECHANGEDIAL_PITCH = 0,
	OPT_ARG_VOICECHANGEDIAL_TIMEOUT,
	/* note: this entry _MUST_ be the last one in the enum */
	OPT_ARG_VOICECHANGEDIAL_ARRAY_SIZE
} voicechangedial_option_args;

AST_APP_OPTIONS(voicechangedial_options, {
	AST_APP_OPTION('h', APP_VOICECHANGEDIAL_HANGUP),
	AST_APP_OPTION('r', APP_VOICECHANGEDIAL_RINGING),
	AST_APP_OPTION('p', APP_VOICECHANGEDIAL_PEEREFFECT),
	AST_APP_OPTION_ARG('P', APP_VOICECHANGEDIAL_PITCH, OPT_ARG_VOICECHANGEDIAL_PITCH),
	AST_APP_OPTION_ARG('T', APP_VOICECHANGEDIAL_TIMEOUT, OPT_ARG_VOICECHANGEDIAL_TIMEOUT),
});

struct voicechangedial_ops {
	struct ast_channel *chan;
	char *tech;
	char *dest;
	int options;
	float pitch;
	int timeout; /* in milliseconds */
};

STANDARD_LOCAL_USER;
LOCAL_USER_DECL;

static struct soundtouch *soundtouch_create(float newPitch)
{
	struct soundtouch *snd;

	ast_log(LOG_DEBUG, "Creating SoundTouch object...\n");
	snd = SoundTouch_construct();
	if (!snd) {
		ast_log(LOG_WARNING, "Failed to create SoundTouch object\n");
		return NULL;
	}

	SoundTouch_setChannels(snd, 1);
	SoundTouch_setSampleRate(snd, 8000);
	SoundTouch_setPitchSemiTonesFloat(snd, newPitch);
	SoundTouch_setSetting(snd, SETTING_USE_QUICKSEEK, 1);
	SoundTouch_setSetting(snd, SETTING_USE_AA_FILTER, 1);

	return snd;
}

static void soundtouch_free(struct soundtouch *st)
{
	ast_log(LOG_DEBUG, "Freeing SoundTouch object...\n");
	SoundTouch_destruct(st);
}

/*
 * Responsible for passing audio between channels
 */
static int bridge_audio(const struct voicechangedial_ops *ops, struct ast_channel *peer)
{
	int res = 0;
	struct ast_channel *chan = ops->chan;
	struct ast_channel *active, *inactive;
	struct ast_channel *channels[2] = { ops->chan, peer };
	struct ast_frame *f;
	int timeout = -1;
	struct soundtouch *st;
	float pitch = ops->pitch;
	int peereffect = (ops->options & APP_VOICECHANGEDIAL_PEEREFFECT) == APP_VOICECHANGEDIAL_PEEREFFECT;

	if (!(st = soundtouch_create(pitch)))
		return -1;

	while (!ast_check_hangup(channels[0]) && !ast_check_hangup(channels[1])) {
		if (!(active = ast_waitfor_n(channels, 2, &timeout)))
			continue;
		inactive = active == channels[0] ? channels[1] : channels[0];
		if (!(f = ast_read(active)))
			break;
		switch (f->frametype) {
		case AST_FRAME_DTMF:
			if (f->subclass == '*')
				pitch -= 1;
			else if (f->subclass == '#')
				pitch += 1;
			else {
				ast_write(inactive, f);
				break;
			}
			if (option_verbose > 3)
				ast_verbose(VERBOSE_PREFIX_4 "New pitch is %f semitones\n", pitch);
			SoundTouch_setPitchSemiTonesFloat(st, pitch);
			break;
		case AST_FRAME_VOICE:
			if (active == (peereffect ? peer : chan)) {
				SoundTouch_putSamples(st, f->data, f->samples);
				f->samples = SoundTouch_receiveSamplesEx(st, f->data, f->samples);
			}
			ast_write(inactive, f);
			break;
		default:
			ast_write(inactive, f);
		}
		channels[0] = inactive;
		channels[1] = active;
		ast_frfree(f);
	}

	soundtouch_free(st);

	return res;
}

/*
 * In this method, we wait for control frames on the peer channel to
 * let us know if the person we're calling picks up, is busy, etc.
 *
 * We're also going to be snooping on the calling channel to make sure
 * we can stop ringing the peer if they cancel by hanging up.
 *
 * Returns:
 *  -1: Error
 *   0: Fail to bridge
 *   1: Answered
 */
static int wait_for_answer(const struct voicechangedial_ops *ops, struct ast_channel *peer, int timeout, char *status, int len)
{
	int subclass;
	struct ast_frame *f;

	for (;;) {
		if (ast_check_hangup(ops->chan)) {
			snprintf(status, len, "CANCEL");
			ast_log(LOG_NOTICE, "Caller canceled call\n");
			return 0;
		}
		timeout = ast_waitfor(peer, timeout);
		if (timeout < 0) {
			ast_log(LOG_NOTICE, "ast_waitfor() error\n");
			return -1;
		}
		if (timeout == 0) {
			snprintf(status, len, "NOANSWER");
			ast_log(LOG_NOTICE, "Timeout waiting for answer\n");
			return 0;
		}
		if (!(f = ast_read(peer))) {
			ast_log(LOG_NOTICE, "ast_read() failed\n");
			return -1;
		}
		if (f->frametype != AST_FRAME_CONTROL) {
			ast_frfree(f);
			continue;
		}
		subclass = f->subclass;
		ast_frfree(f);

		switch (subclass) {
		case AST_CONTROL_RINGING:
			continue;
		case AST_CONTROL_BUSY:
			snprintf(status, len, "BUSY");
			return 0;
		case AST_CONTROL_CONGESTION:
			snprintf(status, len, "CONGESTION");
			return 0;
		case AST_CONTROL_ANSWER:
			snprintf(status, len, "ANSWERED");
			ast_log(LOG_NOTICE, "Call was answered!\n");
			return 1;
		}
	}
}

/*
 * Responsible for getting the callee and caller channels in the mood
 * to get down, make love.
 */
static int initiate_call(const struct voicechangedial_ops *ops, struct ast_channel *peer, char *status, int len)
{
	int res;
	struct ast_channel *chan = ops->chan;

	/* copy crap over from chan to peer, why isn't there a core function for this? */
	ast_channel_inherit_variables(chan, peer);
	ast_set_callerid(peer, chan->cid.cid_name, chan->cid.cid_num, chan->cid.cid_num);
	ast_copy_string(peer->language, chan->language, sizeof(peer->language));
	ast_copy_string(peer->accountcode, chan->accountcode, sizeof(peer->accountcode));
	peer->cdrflags = chan->cdrflags;
	if (ast_strlen_zero(peer->musicclass))
		ast_copy_string(peer->musicclass, chan->musicclass, sizeof(peer->musicclass));
	if (chan->cid.cid_rdnis)
		peer->cid.cid_rdnis = strdup(chan->cid.cid_rdnis);
	peer->cid.cid_pres = chan->cid.cid_pres;
	peer->cid.cid_ton = chan->cid.cid_ton;
	peer->cid.cid_tns = chan->cid.cid_tns;
	peer->adsicpe = chan->adsicpe;
	peer->transfercapability = chan->transfercapability;
	peer->appl = app;
	peer->data = ast_strdupa(chan->name);

	/* call the mofo */
	if ((ops->options & APP_VOICECHANGEDIAL_RINGING) == APP_VOICECHANGEDIAL_RINGING) {
		ast_indicate(chan, AST_CONTROL_RINGING);
	}
	if (chan->cdr)
		ast_cdr_setdestchan(chan->cdr, peer->name);
	if (ast_call(peer, ops->dest, 0) < 0) {
		ast_log(LOG_ERROR, "ast_call() failed\n");
		return -1;
	}
	res = wait_for_answer(ops, peer, ops->timeout, status, len);
	if (res <= 0)
		return res;
	if (ast_channel_make_compatible(chan, peer) != 0) {
		ast_log(LOG_ERROR, "failed to make remote_channel %s/%s Compatible\n", ops->tech, ops->dest);
		return -1;
	}

	ast_answer(chan);
	ast_indicate(chan, -1);

	if (ast_set_read_format(chan, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_read_format(peer, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_write_format(chan, AST_FORMAT_SLINEAR) < 0 ||
	    ast_set_write_format(peer, AST_FORMAT_SLINEAR) < 0) {
		ast_log(LOG_WARNING, "Unable to set channel i/o to slinear mode\n");
		return -1;
	}
	
	res = bridge_audio(ops, peer);
	ast_playtones_stop(chan);

	if ((ops->options & APP_VOICECHANGEDIAL_HANGUP) == APP_VOICECHANGEDIAL_HANGUP)
		return -1;
	else
		return res;
}

/*
 * Responsible for creating the peer 'callee' channel and managing
 * it's precense in the local user list.
 */
static int make_call(const struct voicechangedial_ops *ops)
{
	int rc;
	struct localuser *peer;
	char status[64] = "";
	int cause = 0;

	peer = malloc(sizeof(struct localuser));
	if (!peer) {
		ast_log(LOG_WARNING, "Out of memory\n");
		return -1;
	}
	memset(peer, 0, sizeof(struct localuser));
	if (!(peer->chan = ast_request(ops->tech, ops->chan->nativeformats, ops->dest, &cause))) {
		ast_log(LOG_ERROR, "Error creating channel %s/%s\n", ops->tech, ops->dest);
		ast_cdr_failed(ops->chan->cdr);
		return -1;
	}

	{
		struct ast_channel *chan = peer->chan;
		LOCAL_USER_ADD(peer);
	}

	rc = initiate_call(ops, peer->chan, status, sizeof(status));
	ast_indicate(ops->chan, -1);
	ast_hangup(peer->chan);

	if (status[0]) {
		if (strcmp(status, "BUSY") == 0)
			ast_cdr_busy(ops->chan->cdr);
		else if (strcmp(status, "ANSWERED") == 0)
			/* ast_answer() should do this */;
		else
			ast_cdr_failed(ops->chan->cdr);
		pbx_builtin_setvar_helper(ops->chan, "DIALSTATUS", status);
		ast_log(LOG_NOTICE, "Exiting with DIALSTATUS=%s.\n", status);
	} else
		ast_cdr_failed(ops->chan->cdr);

	LOCAL_USER_REMOVE(peer);

	return rc;
}

/*
 * Responsible for managing the presence of the calling channel in the
 * local users list.
 */
static int voicechangedial_exec(const struct voicechangedial_ops *ops)
{
	int rc;
	struct localuser *u;

	{
		struct ast_channel *chan = ops->chan;
		LOCAL_USER_ADD(u);
	}

	rc = make_call(ops);

	LOCAL_USER_REMOVE(u);

	return rc;
}

/*
 * The sole purpose of this function is to parse the argument data and
 * then call voicechangedial_exec()
 */
static int voicechangedial_app_exec(struct ast_channel *chan, void *data)
{
	int rc;
	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(dialstr);
		AST_APP_ARG(options);
	);
	char *parse;
	struct ast_flags opts = { 0, };
	char *opt_args[OPT_ARG_VOICECHANGEDIAL_ARRAY_SIZE];
	struct voicechangedial_ops ops;

	memset(&ops, 0, sizeof(ops));
	ops.chan = chan;

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "VoiceChangeDial() requires an argument\n");
		return -1;
	}
	parse = ast_strdupa(data);
	AST_STANDARD_APP_ARGS(args, parse);

	if (ast_strlen_zero(args.dialstr)) {
		ast_log(LOG_WARNING, "VoiceChangeDial() requires a dial string\n");
		return -1;
	}
	ops.tech = args.dialstr;
	if (!(ops.dest = strchr(args.dialstr, '/'))) {
		ast_log(LOG_ERROR, "Invalid dial string %s\n", args.dialstr);
		return -1;
	}
	*ops.dest++ = '\0';
	if (!ast_strlen_zero(args.options)) {
		if (ast_app_parse_options(voicechangedial_options, &opts, opt_args, args.options))
			return -1;
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_TIMEOUT) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_TIMEOUT])) {
		ops.options |= APP_VOICECHANGEDIAL_TIMEOUT;
		ops.timeout = strtol(opt_args[OPT_ARG_VOICECHANGEDIAL_TIMEOUT], NULL, 10);
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_PITCH) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_PITCH])) {
		ops.options |= APP_VOICECHANGEDIAL_PITCH;
		ops.pitch = strtof(opt_args[OPT_ARG_VOICECHANGEDIAL_PITCH], NULL);
	} else
		ops.pitch = -5.0;
	if (ops.timeout > 0)
		ops.timeout *= 1000;
	else
		ops.timeout = 60000;
	ops.options |= ast_test_flag(&opts, APP_VOICECHANGEDIAL_HANGUP) ? APP_VOICECHANGEDIAL_HANGUP : 0;
	ops.options |= ast_test_flag(&opts, APP_VOICECHANGEDIAL_RINGING) ? APP_VOICECHANGEDIAL_RINGING : 0;
	ops.options |= ast_test_flag(&opts, APP_VOICECHANGEDIAL_PEEREFFECT) ? APP_VOICECHANGEDIAL_PEEREFFECT : 0;

	rc = voicechangedial_exec(&ops);

	return rc;
}

int unload_module(void)
{
	STANDARD_HANGUP_LOCALUSERS;
	return ast_unregister_application(app);
}

int load_module(void)
{
	return ast_register_application(app, voicechangedial_app_exec, synopsis, desc);
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}

char *key()
{
	return ASTERISK_GPL_KEY;
}
