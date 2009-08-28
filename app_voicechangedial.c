/*
 * Voice Changer for Asterisk 1.2 and 1.4
 * Version 0.6
 *
 * Copyright (C) 2005-2007 J.A Roberts Tunney
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

#define AST_MODULE "app_voicechangedial"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <soundtouch4c.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/frame.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>
#include <asterisk/lock.h>
#include <asterisk/options.h>
#include <asterisk/app.h>
#ifndef _ASTERISK_1_2_
#include <asterisk/stringfields.h>
#endif

#ifdef _ASTERISK_1_2_
#define ast_module_user localuser
#define ast_module_user_remove LOCAL_USER_REMOVE
#define ast_module_user_hangup_all() STANDARD_HANGUP_LOCALUSERS
STANDARD_LOCAL_USER;
LOCAL_USER_DECL;
#endif

static char *app = "VoiceChangeDial";
static char *synopsis = "Voice Changer Dial Application";
static char *desc = "\n"
"Usage VoiceChangeDial(dialstring[|options])\n"
"\n"
"Usage:\n"
"  This app fuctions pretty much the same way as Dial() only you can do\n"
"  cool stuff like change the pitch of your voice.  Please note that\n"
"  pitch changes are not very sophisticated, this will not make a man\n"
"  sound like a woman or vice versa.  It will however do a great job of\n"
"  masking your voice by making you sound either like a chimpmunk or\n"
"  Jabba the Hut.\n"
"\n"
"  Because this application needs to mangle voice data, it will need to\n"
"  pass through Asterisk back-to-back.  This means that if you are\n"
"  switching SIP, the RTP will not pass from phone to phone, but will\n"
"  be proxied through Asterisk.  Therefore, VoiceChangeDial() will use\n"
"  considerably more CPU and bandwidth than a normal Dial() operation.\n"
"\n"
"Options:\n"
"  p    -- Apply effect to peer channel instead\n"
"  P(f) -- Voice pitch in semitones.  Negative is lower, positive\n"
"          is higher.  Default is -5.0\n"
"  T(n) -- Dial timeout in seconds.  If not set, waits 60 sec.\n"
"          for other side to pickup\n"
"  D(down:up[:delta]) -- Allow pitch changes during conversation by\n"
"          pressing DTMF keys 'up' and 'down' with a semitones change\n"
"          of 'delta'.  Examples:  D(*:#:5.0), D(1:2).  The default\n"
"          delta is 5.0 semitones.\n"
"  H(x) -- Allow caller to hangup the peer by pressing '*'.  This\n"
"          is different from the behavior of 'h' in previous\n"
"          releases of the voice changer.  This option trumps 'd'\n"
"  S(x) -- Allow peer to hangup themself by pressing DTMF digit 'x'.\n"
"          Mnemonic is 'suicide'.\n"
"  d    -- **DEPRECATED** Allow pitch changes during conversation with\n"
"          '*' and '#'\n"
"  h    -- **DEPRECATED** Allow caller to hangup the peer by pressing\n"
"          '*'.  This is different from the behavior of 'h' in previous\n"
"          releases of the voice changer.  This option trumps 'd'\n"
"  s    -- **DEPRECATED** Allow peer to hangup themself by pressing\n"
"          '*'.  Mnemonic is 'suicide'.  This option trumps 'd'\n"
"\n"
"Recommended Invocation:\n"
"\n"
"  VoiceChangeDial(SIP/bandwidth/+12036660420|T(30)P(-5.0)D(8:9)H(*))\n"
"\n"
"    This will wait 30 seconds for the called party to pickup, make\n"
"    voice sound lower, and allow you to change your voice down and up\n"
"    with digits '8' and '9'.  You may also hang up the call with '*'.\n"
;

#define APP_VOICECHANGEDIAL_PITCH      (1 << 0)
#define APP_VOICECHANGEDIAL_TIMEOUT    (1 << 1)
#define APP_VOICECHANGEDIAL_PEEREFFECT (1 << 2)
#define APP_VOICECHANGEDIAL_DYNAMIC    (1 << 3)
#define APP_VOICECHANGEDIAL_HANGUP     (1 << 4)
#define APP_VOICECHANGEDIAL_SUICIDE    (1 << 5)
#define APP_VOICECHANGEDIAL_DYNAMIC_DEPRECATED    (1 << 6)
#define APP_VOICECHANGEDIAL_HANGUP_DEPRECATED     (1 << 7)
#define APP_VOICECHANGEDIAL_SUICIDE_DEPRECATED    (1 << 8)

enum {
	OPT_ARG_VOICECHANGEDIAL_PITCH = 0,
	OPT_ARG_VOICECHANGEDIAL_TIMEOUT,
	OPT_ARG_VOICECHANGEDIAL_DYNAMIC,
	OPT_ARG_VOICECHANGEDIAL_HANGUP,
	OPT_ARG_VOICECHANGEDIAL_SUICIDE,
	/* note: this entry _MUST_ be the last one in the enum */
	OPT_ARG_VOICECHANGEDIAL_ARRAY_SIZE
} voicechangedial_option_args;

AST_APP_OPTIONS(voicechangedial_options, {
	AST_APP_OPTION('p', APP_VOICECHANGEDIAL_PEEREFFECT),
	AST_APP_OPTION('d', APP_VOICECHANGEDIAL_DYNAMIC_DEPRECATED),
	AST_APP_OPTION('h', APP_VOICECHANGEDIAL_HANGUP_DEPRECATED),
	AST_APP_OPTION('s', APP_VOICECHANGEDIAL_SUICIDE_DEPRECATED),
	AST_APP_OPTION_ARG('P', APP_VOICECHANGEDIAL_PITCH, OPT_ARG_VOICECHANGEDIAL_PITCH),
	AST_APP_OPTION_ARG('T', APP_VOICECHANGEDIAL_TIMEOUT, OPT_ARG_VOICECHANGEDIAL_TIMEOUT),
	AST_APP_OPTION_ARG('D', APP_VOICECHANGEDIAL_DYNAMIC, OPT_ARG_VOICECHANGEDIAL_DYNAMIC),
	AST_APP_OPTION_ARG('H', APP_VOICECHANGEDIAL_HANGUP, OPT_ARG_VOICECHANGEDIAL_HANGUP),
	AST_APP_OPTION_ARG('S', APP_VOICECHANGEDIAL_SUICIDE, OPT_ARG_VOICECHANGEDIAL_SUICIDE)
});

enum dtmfaction {
	DTMFACTION_NULL = 0,
	DTMFACTION_RAISEPITCH,
	DTMFACTION_LOWERPITCH,
	DTMFACTION_HANGUP
};

#define DTMFRANGE ('9' - '#' + 1)
#define ASCII2DTMF(c) (c - '#')
#define ISDTMFDIGIT(c) ((c >= '0' && c <= '9') || c == '*' || c == '#')
struct voicechangedial_ops {
	struct ast_channel *chan;
	char *tech;
	char *dest;
	int options;
	float pitch;
	float pitchdelta;
	int timeout; /* in milliseconds */
	enum dtmfaction chan_dtmfactions[DTMFRANGE];
	enum dtmfaction peer_dtmfactions[DTMFRANGE];
};

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
	float stout[256], stin[256];
	int n;

	float pitch = ops->pitch;
	int op_peereffect = (ops->options & APP_VOICECHANGEDIAL_PEEREFFECT) == APP_VOICECHANGEDIAL_PEEREFFECT;

	if (!(st = soundtouch_create(pitch)))
		return -1;

	while (!ast_check_hangup(channels[0]) && !ast_check_hangup(channels[1])) {
		if (!(active = ast_waitfor_n(channels, 2, &timeout)))
			continue;
		inactive = active == channels[0] ? channels[1] : channels[0];
		if (!(f = ast_read(active)))
			break;
		switch (f->frametype) {
		case AST_FRAME_DTMF: {
			enum dtmfaction action = DTMFACTION_NULL;
			if (active == chan)
				action = ops->chan_dtmfactions[ASCII2DTMF(f->subclass)];
			else
				action = ops->peer_dtmfactions[ASCII2DTMF(f->subclass)];
			switch (action) {
			case DTMFACTION_NULL:
				break;
			case DTMFACTION_RAISEPITCH:
				pitch += ops->pitchdelta;
				SoundTouch_setPitchSemiTonesFloat(st, pitch);
				if (option_verbose > 3)
					ast_verbose(VERBOSE_PREFIX_4 "New pitch is %f semitones\n", pitch);
				break;
			case DTMFACTION_LOWERPITCH:
				pitch -= ops->pitchdelta;
				SoundTouch_setPitchSemiTonesFloat(st, pitch);
				if (option_verbose > 3)
					ast_verbose(VERBOSE_PREFIX_4 "New pitch is %f semitones\n", pitch);
				break;
			case DTMFACTION_HANGUP:
				goto BREAKBREAK;
			}

			ast_write(inactive, f);
			break;
		}
		case AST_FRAME_VOICE:
			if (pitch != 0.0 && active == (op_peereffect ? peer : chan)) {
				SoundTouch_putSamples(st, f->data, f->samples);
				memset(f->data, 0, f->datalen);
				SoundTouch_receiveSamplesEx(st, f->data, f->samples);
			}
			ast_write(inactive, f);
			break;
		default:
			break;
		}
		channels[0] = inactive;
		channels[1] = active;
		ast_frfree(f);
		continue;
	BREAKBREAK:
		break;
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
	struct ast_frame *f;

	for (;;) {
		if (ast_check_hangup(ops->chan)) {
			snprintf(status, len, "CANCEL");
			ast_log(LOG_NOTICE, "Caller canceled call\n");
			return 0;
		}
		if (ops->chan->sched)
			ast_sched_runq(ops->chan->sched);
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
		switch (f->frametype) {
		case AST_FRAME_VOICE:
			if (ast_write(ops->chan, f))
				ast_log(LOG_NOTICE, "Unable to forward frame\n");
			break;
		case AST_FRAME_CONTROL:
			switch (f->subclass) {
			case AST_CONTROL_RINGING:
				ast_indicate(ops->chan, AST_CONTROL_RINGING);
				break;
			case AST_CONTROL_BUSY:
				snprintf(status, len, "BUSY");
				ast_frfree(f);
				return 0;
			case AST_CONTROL_CONGESTION:
				snprintf(status, len, "CONGESTION");
				ast_frfree(f);
				return 0;
			case AST_CONTROL_ANSWER:
				snprintf(status, len, "ANSWERED");
				if (option_verbose > 3)
					ast_verbose(VERBOSE_PREFIX_4 "Call was answered!\n");
				ast_frfree(f);
				return 1;
			}
			break;
		}
		ast_frfree(f);
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
	int oldread, oldwrite;

	oldread = chan->readformat;
	oldwrite = chan->writeformat;

	/* copy crap over from chan to peer, why isn't there a core function for this? */
	ast_channel_inherit_variables(chan, peer);
	ast_set_callerid(peer, chan->cid.cid_name, chan->cid.cid_num, chan->cid.cid_num);
#ifdef _ASTERISK_1_2_
	ast_copy_string(peer->language, chan->language, sizeof(peer->language));
	ast_copy_string(peer->accountcode, chan->accountcode, sizeof(peer->accountcode));
	if (ast_strlen_zero(peer->musicclass))
		ast_copy_string(peer->musicclass, chan->musicclass, sizeof(peer->musicclass));
#else
	ast_string_field_set(peer, language, chan->language);
	ast_string_field_set(peer, accountcode, chan->accountcode);
	if (ast_strlen_zero(peer->musicclass))
		ast_string_field_set(peer, musicclass, chan->musicclass);
#endif
	peer->cdrflags = chan->cdrflags;
	if (chan->cid.cid_rdnis)
		peer->cid.cid_rdnis = strdup(chan->cid.cid_rdnis);
	peer->cid.cid_pres = chan->cid.cid_pres;
	peer->cid.cid_ton = chan->cid.cid_ton;
	peer->cid.cid_tns = chan->cid.cid_tns;
	peer->adsicpe = chan->adsicpe;
	peer->transfercapability = chan->transfercapability;
	peer->appl = app;
	peer->data = ast_strdupa(chan->name);
	if (chan->cid.cid_num) {
		peer->cid.cid_num = strdup(chan->cid.cid_num);
		if (!peer->cid.cid_num)
			ast_log(LOG_WARNING, "Out of memory\n");
	}
	if (chan->cid.cid_name) {
		peer->cid.cid_name = strdup(chan->cid.cid_name);
		if (!peer->cid.cid_name)
			ast_log(LOG_WARNING, "Out of memory\n");
	}
	peer->cdrflags = chan->cdrflags;

	if (ast_set_read_format(chan, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_read_format(peer, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_write_format(chan, AST_FORMAT_SLINEAR) < 0 ||
	    ast_set_write_format(peer, AST_FORMAT_SLINEAR) < 0) {
		ast_log(LOG_WARNING, "Unable to set channel i/o to slinear mode\n");
		return -1;
	}

	/* call the mofo */
	ast_indicate(chan, AST_CONTROL_RINGING);
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

	/* just in case */
	if (ast_set_read_format(chan, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_read_format(peer, AST_FORMAT_SLINEAR)  < 0 ||
	    ast_set_write_format(chan, AST_FORMAT_SLINEAR) < 0 ||
	    ast_set_write_format(peer, AST_FORMAT_SLINEAR) < 0) {
		ast_log(LOG_WARNING, "Unable to set channel i/o to slinear mode\n");
		return -1;
	}
	
	res = bridge_audio(ops, peer);

	ast_playtones_stop(chan);
	ast_set_read_format(chan, oldread);
	ast_set_write_format(chan, oldwrite);

	return res;
}

/*
 * Responsible for creating the peer 'callee' channel and managing
 * it's precense in the local user list.
 */
static int make_call(const struct voicechangedial_ops *ops)
{
	int rc;
	struct ast_module_user *peer;
	struct ast_channel *peerchan;
	char status[64] = "";
	int cause = 0;

	if (!(peerchan = ast_request(ops->tech, ops->chan->nativeformats, ops->dest, &cause))) {
		ast_log(LOG_ERROR, "Error creating channel %s/%s\n", ops->tech, ops->dest);
		ast_cdr_failed(ops->chan->cdr);
		return -1;
	}

#ifdef _ASTERISK_1_2_
	if (!(peer = malloc(sizeof(peer)))) {
		ast_log(LOG_WARNING, "Out of memory\n");
		return -1;
	}
	memset(peer, 0, sizeof(peer));
	{
		struct ast_channel *chan = peerchan;
		LOCAL_USER_ADD(peer);
	}
#else
	peer = ast_module_user_add(peerchan);
#endif

	rc = initiate_call(ops, peerchan, status, sizeof(status));
	ast_indicate(ops->chan, -1);
	ast_hangup(peerchan);

	if (status[0]) {
		if (strcmp(status, "BUSY") == 0)
			ast_cdr_busy(ops->chan->cdr);
		else if (strcmp(status, "ANSWERED") == 0)
			/* ast_answer() should do this */;
		else
			ast_cdr_failed(ops->chan->cdr);
		pbx_builtin_setvar_helper(ops->chan, "DIALSTATUS", status);
		if (option_verbose > 3)
			ast_verbose(VERBOSE_PREFIX_4 "Exiting with DIALSTATUS=%s.\n", status);
	} else
		ast_cdr_failed(ops->chan->cdr);

	ast_module_user_remove(peer);

	return rc;
}

/*
 * Responsible for managing the presence of the calling channel in the
 * local users list.
 */
static int voicechangedial_exec(const struct voicechangedial_ops *ops)
{
	int rc;
	struct ast_module_user *u;
	struct ast_channel *chan = ops->chan;

#ifdef _ASTERISK_1_2_
	LOCAL_USER_ADD(u);
#else
	u = ast_module_user_add(chan);
#endif

	rc = make_call(ops);
	ast_module_user_remove(u);
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
	ops.pitchdelta = 5.0;
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

	ops.options |= ast_test_flag(&opts, APP_VOICECHANGEDIAL_PEEREFFECT) ? APP_VOICECHANGEDIAL_PEEREFFECT : 0;
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_DYNAMIC_DEPRECATED)) {
		ops.options |= APP_VOICECHANGEDIAL_DYNAMIC_DEPRECATED;
		if ((ops.options & APP_VOICECHANGEDIAL_PEEREFFECT) == APP_VOICECHANGEDIAL_PEEREFFECT) {
			ops.peer_dtmfactions[ASCII2DTMF('*')] = DTMFACTION_LOWERPITCH;
			ops.peer_dtmfactions[ASCII2DTMF('#')] = DTMFACTION_RAISEPITCH;
		} else {
			ops.chan_dtmfactions[ASCII2DTMF('*')] = DTMFACTION_LOWERPITCH;
			ops.chan_dtmfactions[ASCII2DTMF('#')] = DTMFACTION_RAISEPITCH;
		}
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_HANGUP_DEPRECATED)) {
		ops.options |= APP_VOICECHANGEDIAL_HANGUP_DEPRECATED;
		ops.chan_dtmfactions[ASCII2DTMF('*')] = DTMFACTION_HANGUP;
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_SUICIDE_DEPRECATED)) {
		ops.options |= APP_VOICECHANGEDIAL_SUICIDE_DEPRECATED;
		ops.peer_dtmfactions[ASCII2DTMF('*')] = DTMFACTION_HANGUP;
	}

	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_PITCH) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_PITCH])) {
		ops.options |= APP_VOICECHANGEDIAL_PITCH;
		ops.pitch = strtof(opt_args[OPT_ARG_VOICECHANGEDIAL_PITCH], NULL);
	} else
		ops.pitch = -5.0;
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_TIMEOUT) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_TIMEOUT])) {
		ops.options |= APP_VOICECHANGEDIAL_TIMEOUT;
		ops.timeout = strtol(opt_args[OPT_ARG_VOICECHANGEDIAL_TIMEOUT], NULL, 10);
	}
	if (ops.timeout > 0)
		ops.timeout *= 1000;
	else
		ops.timeout = 60000;
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_DYNAMIC) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_DYNAMIC])) {
		char *down, *up, *delta;
		ops.options |= APP_VOICECHANGEDIAL_DYNAMIC;
		down = ast_strdupa(opt_args[OPT_ARG_VOICECHANGEDIAL_DYNAMIC]);
		if (!(up = strchr(down, ':'))) {
			ast_log(LOG_ERROR, "Invalid D() argument format\n");
			return -1;
		}
		*up++ = '\0';
		if ((delta = strchr(up, ':'))) {
			*delta++ = '\0';
			ops.pitchdelta = strtof(delta, NULL);
			ast_verbose(VERBOSE_PREFIX_4 "Setting pitch delta to %f semitones\n",
				    ops.pitchdelta);
		}
		if (strlen(down) != 1 || !ISDTMFDIGIT(down[0])) {
			ast_log(LOG_ERROR, "Invalid DTMF digit for D(down)\n");
			return -1;
		}
		if (strlen(up) != 1 || !ISDTMFDIGIT(up[0])) {
			ast_log(LOG_ERROR, "Invalid DTMF digit for D(up)\n");
			return -1;
		}
		if ((ops.options & APP_VOICECHANGEDIAL_PEEREFFECT) == APP_VOICECHANGEDIAL_PEEREFFECT) {
			ops.peer_dtmfactions[ASCII2DTMF(down[0])] = DTMFACTION_LOWERPITCH;
			ops.peer_dtmfactions[ASCII2DTMF(up[0])]   = DTMFACTION_RAISEPITCH;
		} else {
			ops.chan_dtmfactions[ASCII2DTMF(down[0])] = DTMFACTION_LOWERPITCH;
			ops.chan_dtmfactions[ASCII2DTMF(up[0])]   = DTMFACTION_RAISEPITCH;
		}
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_HANGUP) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_HANGUP])) {
		const char *s;
		ops.options |= APP_VOICECHANGEDIAL_HANGUP;
		s = opt_args[OPT_ARG_VOICECHANGEDIAL_HANGUP];
		if (strlen(s) != 1 || !ISDTMFDIGIT(s[0])) {
			ast_log(LOG_ERROR, "Invalid DTMF digit for H()\n");
			return -1;
		}
		ops.chan_dtmfactions[ASCII2DTMF(s[0])] = DTMFACTION_HANGUP;
	}
	if (ast_test_flag(&opts, APP_VOICECHANGEDIAL_SUICIDE) && !ast_strlen_zero(opt_args[OPT_ARG_VOICECHANGEDIAL_SUICIDE])) {
		const char *s;
		ops.options |= APP_VOICECHANGEDIAL_SUICIDE;
		s = opt_args[OPT_ARG_VOICECHANGEDIAL_SUICIDE];
		if (strlen(s) != 1 || !ISDTMFDIGIT(s[0])) {
			ast_log(LOG_ERROR, "Invalid DTMF digit for H()\n");
			return -1;
		}
		ops.peer_dtmfactions[ASCII2DTMF(s[0])] = DTMFACTION_HANGUP;
	}

	rc = voicechangedial_exec(&ops);

	return rc;
}

int load_module(void)
{
	return ast_register_application(app, voicechangedial_app_exec, synopsis, desc);
}

int unload_module(void)
{
	ast_module_user_hangup_all();
	return ast_unregister_application(app);
}

#ifdef _ASTERISK_1_2_

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

char *description(void)
{
	return "Voice Changer Dial Application";
}

#else /* #ifdef _ASTERISK_1_2_ */

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Voice Changer Dial Application");

#endif /* #ifdef _ASTERISK_1_2_ */
