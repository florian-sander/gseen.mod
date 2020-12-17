/*
 * Copyright (C) 2000,2001  Florian Sander
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

static int get_spent(char *nick, char *chan)
{
  struct chanset_t *ch = NULL;
  memberlist *m = NULL;

  int spent;
  ch = findchan_by_dname(chan);
  if (ch)
    m = ismember(ch, nick);
  if (m && m->joined)
    spent = now - m->joined;
  else
    spent = -1;
  return spent;
}

static int secretchan(char *chan)
{
  struct chanset_t *ch;

  ch = findchan_by_dname(chan);
  if (!ch)
    return 0;
  if (ch->status & CHAN_SECRET)
    return 1;
  return 0;
}

static int nolog(char *chan)
{
  char buf[121], *b;

  Context;
  strncpy(buf, no_log, 120);
  buf[120] = 0;
  b = buf;
  while (b[0])
    if (!strcasecmp(chan, newsplit(&b)))
      return 1;
#if EGG_IS_MIN_VER(10503)
  if (ngetudef("noseendata", chan))
    return 1;
#endif
  return 0;
}

static int gseen_join(char *nick, char *uhost, char *hand, char *chan)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_JOIN, nick, uhost, chan, "", now, get_spent(nick, chan));
  report_seenreq(chan, nick);
  if ((hand[0] == '*') && strcasecmp(nick, hand))
    report_seenreq(chan, hand);
  return 0;
}

static int gseen_kick(char *nick, char *uhost, char *hand, char *chan,
		       char *victim, char *reason)
{
  struct chanset_t *ch = NULL;
  memberlist *m = NULL;
  char msg[1024], *s;
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  ch = findchan_by_dname(chan);
  if (!ch) {
    debug2("Unable to seen %s getting kicked from %s", victim, chan);
    return 0;
  }
  if (secretchan(chan))
    chan = buf;
  s = msg;
  s[0] = 0;
  m = ismember(ch, victim);
  if (!m) {
    debug2("Unable to seen %s getting kicked from %s", victim, chan);
    return 0;
  }
  if ((strlen(nick) + strlen(reason) + 2) < 1024)
    sprintf(s, "%s %s", nick, reason);
  add_seen(SEEN_KICK, victim, m->userhost, chan, s, now,
  	   get_spent(victim, chan));
  return 0;
}

static int gseen_nick(char *nick, char *uhost, char *hand, char *chan,
		       char *newnick)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_NICK, nick, uhost, chan, newnick, now, get_spent(nick, chan));
  if (!(use_handles && (hand[0] != '*')))
    add_seen(SEEN_NCKF, newnick, uhost, chan, nick, now, get_spent(nick, chan));
  report_seenreq(chan, newnick);
  if ((hand[0] != '*') && strcasecmp(newnick, hand))
    report_seenreq(chan, hand);
  return 0;
}

#if EGG_IS_MIN_VER(10502)
static int gseen_part(char *nick, char *uhost, char *hand, char *chan,
		       char *reason)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_PART, nick, uhost, chan, reason, now, get_spent(nick, chan));
  return 0;
}
#else
static int gseen_part(char *nick, char *uhost, char *hand, char *chan)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_PART, nick, uhost, chan, "", now, get_spent(nick, chan));
  return 0;
}
#endif

static int gseen_sign(char *nick, char *uhost, char *hand, char *chan,
		       char *reason)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_SIGN, nick, uhost, chan, reason, now, get_spent(nick, chan));
  return 0;
}

static int gseen_splt(char *nick, char *uhost, char *hand, char *chan)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_SPLT, nick, uhost, chan, "", now, get_spent(nick, chan));
  return 0;
}

static int gseen_rejn(char *nick, char *uhost, char *hand, char *chan)
{
  char buf[10] = "[secret]";

  Context;
  if (nolog(chan))
    return 0;
  if (use_handles && (hand[0] != '*'))
    nick = hand;
  if (secretchan(chan))
    chan = buf;
  add_seen(SEEN_REJN, nick, uhost, chan, "", now, get_spent(nick, chan));
  return 0;
}

static int gseen_chjn STDVAR
{
  Context;
  BADARGS(7, 7, " bot hand chan flag idx host");
  add_seen(SEEN_CHJN, argv[2], argv[6], argv[3], argv[1], now, -1);
  return 0;
}

static int gseen_chpt STDVAR
{
  Context;
  BADARGS(5, 5, " bot hand idx chan");
  add_seen(SEEN_CHPT, argv[2], "unknown", argv[4], argv[1], now, -1);
  return 0;
}

static cmd_t seen_kick[] =
{
  {"*", "", (IntFunc) gseen_kick, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_nick[] =
{
  {"*", "", (IntFunc) gseen_nick, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_join[] =
{
  {"*", "", (IntFunc) gseen_join, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_part[] =
{
  {"*", "", (IntFunc) gseen_part, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_sign[] =
{
  {"*", "", (IntFunc) gseen_sign, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_splt[] =
{
  {"*", "", (IntFunc) gseen_splt, "gseen"},
  {0, 0, 0, 0}
};

static cmd_t seen_rejn[] =
{
  {"*", "", (IntFunc) gseen_rejn, "gseen"},
  {0, 0, 0, 0}
};
