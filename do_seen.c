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

/* do_seen(): Checks if someone matches the mask, and returns the reply
 * mask : first paramater (e.g. "G`Quann", "G`Quann", "*!*@*.isp.de", ...)
 * nick : nick of the one, who triggered the command
 * uhost: user@host of nick
 * chan : chan, where the command was triggered
 * bns  :
 *        1 : do a botnet-seen if no matches are found
 *        0 : don't do a botnet-seen
 *       -1 : return NULL instead of text, if no matches were found
 *            (necessary for botnet seen)
 */
static char *do_seen(char *mask, char *nick, char *uhost, char *chan, int bns)
{
  char hostbuf[UHOSTLEN + 1], *host, *newhost, *tmp, *dur;
  seendat *l;
  gseenres *r;
  int wild, nr;
  char bnquery[256];
  struct userrec *u;
  struct laston_info *li;
  struct chanset_t *ch;

  Context;
  start_seentime_calc();
  if (seen_reply) {
    nfree(seen_reply);
    seen_reply = NULL;
  }
  l = NULL;
  li = NULL;
  host = hostbuf;
  newhost = NULL;
  mask = newsplit(&mask);
  glob_query = mask;
  while (mask[0] == ' ')
    mask++;
  if (!mask[0]) {
    return SLNOPARAM;
  }
  if (strchr(mask, '?') || strchr(mask, '*')) {
    // if wildcard-searches ares not allowed, then either return
    // NULL (for botnet-seen), or a appropriate warning
    if (!wildcard_search) {
      if (bns == -1)
        return NULL;
      else
        return SLNOWILDCARDS;
    } else
      wild = 1;
  } else {
    if (strlen(mask) > seen_nick_len) // don't process if requested nick is too long
      return SLTOOLONGNICK;      // (e.g. stop stupid jokes)
    if (!strcasecmp(mask, nick)) {
      return SLMIRROR;
    }
    // check if the nick is on the current channel
    if (onchan(mask, chan))
      return SLONCHAN;
    if ((glob_othernick = handonchan(mask, chan)))
      return SLHANDONCHAN;
    // check if it is on any other channel
    if ((ch = onanychan(mask))) {
#if EGG_IS_MIN_VER(10500)
      if (!secretchan(ch->dname)) {
	glob_otherchan = ch->dname;
        return SLONOTHERCHAN;
      }
#else
      if (!secretchan(ch->name)) {
	glob_otherchan = ch->name;
        return SLONOTHERCHAN;
      }
#endif
    }
    // check if the user who uses this handle is on the channel under
    // a different nick
    if ((ch = handonanychan(mask))) {
#if EGG_IS_MIN_VER(10500)
      if (!secretchan(ch->dname)) {
        glob_otherchan = ch->dname;
        return SLONOTHERCHAN;
      }
#else
      if (!secretchan(ch->name)) {
        glob_otherchan = ch->name;
        return SLONOTHERCHAN;
      }
#endif
    }
    add_seenreq(mask, nick, uhost, chan, now);
    wild = 0;
    l = findseen(mask);
    // if there's a result, and if we don't want to search for the same user
    // under a different nick, just make a do_seennick on the result
    if (l && !fuzzy_search) {
      tmp = do_seennick(l);
      end_seentime_calc();
      return tmp;
    }
    if (!l) {
      u = get_user_by_handle(userlist, mask);
      if (u) {
        li = get_user(&USERENTRY_LASTON, u);
      }
      if (!u || !li) {
        if (bns == -1) {       // if bns is 0, then do_seen() was triggered by
          end_seentime_calc(); // a botnet seen function, which needs a clear
          return NULL;         // NULL to detect if there was a result or not
        }
        tmp = SLNOTSEEN;
        if (bns && ((strlen(mask) + strlen(nick) + strlen(uhost)
            + strlen(chan) + 20) < 255)) {
          debug0("trying botnet seen");
          if (bnsnick)
            nfree(bnsnick);
          if (bnschan)
            nfree(bnschan);
          bnsnick = nmalloc(strlen(nick) + 1);
          strcpy(bnsnick, nick);
          bnschan = nmalloc(strlen(chan) + 1);
          strcpy(bnschan, chan);
          sprintf(bnquery, "gseen_req %s %s %s %s", mask, nick, uhost, chan);
          botnet_send_zapf_broad(-1, botnetnick, NULL, bnquery);
        }
      } else {
        // we have a matching handle, no seen-entry, but a laston entry
        // in the userbase, so let's just return that one.
        dur = gseen_duration(now - li->laston);
        glob_laston = dur;
        tmp = SLPOORSEEN;
        seen_reply = nmalloc(strlen(tmp) + 1);
        strcpy(seen_reply, tmp);
        end_seentime_calc();
        return seen_reply;
      }
      end_seentime_calc();
      return tmp;
    }
    // now prepare the host for fuzzy-search
    if (strlen(l->host) < UHOSTLEN) {
      maskstricthost(l->host, host);
      host = strchr(host, '!') + 1; // strip nick from host for faster search
    } else {
      end_seentime_calc();
      return "error, too long host";
    }
  }
  if (l && (l->type == SEEN_CHPT)) {
    tmp = do_seennick(l);
    end_seentime_calc();
    return tmp;
  }
  numresults = 0;
  // wildmatch_seens uses a global var to store hosts in it
  // (to prevent massive nmalloc/nfree-usage), so don't forget
  // to initialize and free it
  temp_wildmatch_host = my_malloc(1);
  wildmatch_seens(host, mask, wild);
  my_free(temp_wildmatch_host);
  temp_wildmatch_host = NULL;
  if (!results) {
    end_seentime_calc();
    if (bns == -1)
      return NULL; // let the botnet seen function know, that seen failed
    return SLNOMATCH;
  }
  if (numresults >= max_matches) {
    end_seentime_calc();
    free_seenresults();
    return SLTOOMANYMATCHES;
  }
  sortresults();
  if (strcasecmp(results->seen->nick, mask)) {
    // if the user's latest nick is not the nick for which we were searching,
    // say that there were multiple matches and display the latest one
    if (numresults == 1)
      tmp = SLONEMATCH;
    else if (numresults <= 5)
      tmp = SLLITTLEMATCHES;
    else
      tmp = SLMANYMATCHES;
    seen_reply = nmalloc(strlen(tmp) + 1);
    strcpy(seen_reply, tmp);
    nr = 0;
    for (r = results; (r && (nr < 5)); r = r->next) {
      nr++;
      if (nr > 1) {
        seen_reply = nrealloc(seen_reply, 1 + strlen(seen_reply) + 1 + strlen(r->seen->nick) + 1);
        strcat(seen_reply, ", ");
      } else {
	seen_reply = nrealloc(seen_reply, 1 + strlen(seen_reply) + strlen(r->seen->nick) + 1);
        strcat(seen_reply, " ");
      }
      strcat(seen_reply, r->seen->nick);
    }
    tmp = do_seennick(results->seen);
    seen_reply = nrealloc(seen_reply, 2 + strlen(seen_reply) + strlen(tmp) + 1);
    sprintf(seen_reply, "%s. %s", seen_reply, tmp);
  } else { // first result is the nick which we were searching for
    // just return the info for this nick and don't care about other results
    tmp = do_seennick(results->seen);
    seen_reply = nmalloc(strlen(tmp) + 1);
    strcpy(seen_reply, tmp);
  }
  free_seenresults();
  end_seentime_calc();
  return seen_reply;
}

/* do_seennick():
 * takes a seen-dataset and produces the corresponding reply basically
 * by referencing to the lang entry with the same number as the seen-type.
 */
static char *do_seennick(seendat *l)
{
//  char buf[256], *msg;
  int stype;

  Context;
  if (!l) {
    debug0("ERROR! Tryed to do a seennick on a NULL pointer!");
    return "ERROR! seendat == NULL!!!";
  }
  glob_seendat = l;
  // l->type is the basic language-entry-number
  stype = l->type + 100;
  // in some cases, we might need a special reply, so modify the
  // number if neccessary
  switch (l->type) {
    case SEEN_JOIN:
      if (!onchan(l->nick, l->chan))
        stype += 20;
      break;
    case SEEN_PART:
      /* nothing to do here */
      break;
    case SEEN_SIGN:
      /* nothing again */
      break;
    case SEEN_NICK:
      if (!onchan(l->msg, l->chan))
        stype += 20;
      break;
    case SEEN_NCKF:
      if (!onchan(l->nick, l->chan))
        stype += 20;
      break;
    case SEEN_KICK:
/*      msg = buf;
      strncpy(buf, l->msg, 255);
      msg[255] = 0;
      sglobpunisher = newsplit(&msg);
      sglobreason = msg; */
      break;
    case SEEN_SPLT:
      /* nothing to do here */
      break;
    case SEEN_REJN:
      if (!onchan(l->nick, l->chan))
        stype += 20;
      break;
    case SEEN_CHJN:
    case SEEN_CHPT:
      if (!strcmp(l->chan, "0"))
        stype += 20;
      break;
    default:
      stype = 140;
  }
  return getslang(stype);
}

/* findseens():
 * interface for webseen.mod
 * find all results for a query and return a pointer to this list
 * (basically the core of do_seen())
 */
static gseenres *findseens(char *mask, int *ret, int fuzzy)
{
  char hostbuf[UHOSTLEN + 1], *host, *newhost;
  seendat *l;
  int wild;

  Context;
  start_seentime_calc();
  *ret = WS_OK;
  l = NULL;
  host = hostbuf;
  newhost = NULL;
  mask = newsplit(&mask);
  while (mask[0] == ' ')
    mask++;
  if (!mask[0]) {
    *ret = WS_NOPARAM;
    return NULL;
  }
  if (strchr(mask, '?') || strchr(mask, '*')) {
    // if wildcard-searches ares not allowed, then either return
    // NULL (for botnet-seen), or a appropriate warning
    if (!wildcard_search) {
      *ret = WS_NOWILDCARDS;
      return NULL;
    }
    wild = 1;
  } else {
    if (strlen(mask) > seen_nick_len) { // don't process if requested nick is too long
      *ret = WS_TOOLONGNICK;       // (e.g. stop stupid jokes)
      return NULL;
    }
    add_seenreq(mask, "www-user", "unknown_host", "webinterface", now);
    wild = 0;
    l = findseen(mask);
    // if there's a result, and if we don't want to search for the same user
    // under a different nick, just return this result
    if (l && (!fuzzy_search || !fuzzy)) {
      numresults = 1;
      add_seenresult(l);
      end_seentime_calc();
      return results;
    }
    if (!l) {
      // no matching user was found :(
      *ret = WS_NORESULT;
      end_seentime_calc();
      return NULL;
    }
    // now prepare the host for fuzzy-search
    if (strlen(l->host) < UHOSTLEN) {
      maskstricthost(l->host, host);
      host = strchr(host, '!') + 1; // strip nick from host for faster search
    } else {
      *ret = WS_TOOLONGHOST;
      end_seentime_calc();
      return NULL;
    }
  }
  if (l && (l->type == SEEN_CHPT)) {
    numresults = 1;
    add_seenresult(l);
    end_seentime_calc();
    return results;
  }
  numresults = 0;
  // wildmatch_seens uses a global var to store hosts in it
  // (to prevent massive nmalloc/nfree-usage), so don't forget
  // to initialize and free it
  temp_wildmatch_host = my_malloc(1);
  wildmatch_seens(host, mask, wild);
  my_free(temp_wildmatch_host);
  temp_wildmatch_host = NULL;
  if (!results) {
    // no match :(
    *ret = WS_NORESULT;
    end_seentime_calc();
    return NULL;
  }
  if (numresults >= max_matches) {
    free_seenresults();
    *ret = WS_TOOMANYMATCHES;
    end_seentime_calc();
    return NULL;
  }
  sortresults();
  *ret = 0;
  end_seentime_calc();
  return results;
}


char seenstats_reply[512];
static char *do_seenstats()
{
  glob_totalnicks = count_seens();
  glob_totalbytes = gseen_expmem();
  sprintf(seenstats_reply, "%s", SLSEENSTATS);
  return seenstats_reply;
}

// add an seen result (to the top of the list)
static void add_seenresult(seendat *seen)
{
  gseenres *nl;

  numresults++;
  if (numresults > max_matches)
    return;
  nl = nmalloc(sizeof(gseenres));
  nl->seen = seen;
  nl->next = results;
  results = nl;
}

static int expmem_seenresults()
{
  int bytes = 0;
  gseenres *l;

  for (l = results; l; l = l->next)
    bytes += sizeof(gseenres);
  return bytes;
}

static void free_seenresults()
{
  gseenres *l, *ll;

  l = results;
  while (l) {
    ll = l->next;
    nfree(l);
    l = ll;
  }
  results = NULL;
}

static void sortresults()
{
  int again = 1;
  gseenres *last, *p, *c, *n;
  int a, b;

  Context;
  again = 1;
  last = NULL;
  while ((results != last) && (again)) {
    p = NULL;
    c = results;
    n = c->next;
    again = 0;
    while (n != last) {
      if (!c || !n)
        a = b = 0;
      else
        a = c->seen->when;
        b = n->seen->when;
      if (a < b) {
  again = 1;
  c->next = n->next;
  n->next = c;
  if (p == NULL)
    results = n;
  else
    p->next = n;
      }
      p = c;
      c = n;
      n = n->next;
    }
    last = c;
  }
  Context;
  return;
}

static void sortrequests(seenreq *l)
{
  int again = 1;
  seenreq_by *last, *p, *c, *n;
  int a, b;

  Context;
  again = 1;
  last = NULL;
  while ((l->by != last) && (again)) {
    p = NULL;
    c = l->by;
    n = c->next;
    again = 0;
    while (n != last) {
      if (!c || !n)
        a = b = 0;
      else
        a = c->when;
        b = n->when;
      if (a < b) {
  again = 1;
  c->next = n->next;
  n->next = c;
  if (p == NULL)
    l->by = n;
  else
    p->next = n;
      }
      p = c;
      c = n;
      n = n->next;
    }
    last = c;
  }
  Context;
  return;
}

/* stolen from tcl_duration in tclmisc.c */
char gs_duration_temp[256];
static char *gseen_duration(int seconds)
{
  char s[256];
  time_t sec;

  sec = seconds;
  s[0] = 0;
  if (sec < 1) {
    snprintf(gs_duration_temp, sizeof(gs_duration_temp), "%s", SLSOMETIME);
    return gs_duration_temp;
  }
  if (sec < 60) {
    sprintf(gs_duration_temp, "%d %s", (int) (sec / 1),
            ((int) (sec / 1) > 1) ? SLSECONDS : SLSECOND);
    return gs_duration_temp;
  }
  if (sec >= 31536000) {
    sprintf(s, "%d %s ", (int) (sec / 31536000),
            ((int) (sec / 31536000) > 1) ? SLYEARS : SLYEAR);
    sec -= (((int) (sec / 31536000)) * 31536000);
  }
  if (sec >= 604800) {
    sprintf(&s[strlen(s)], "%d %s ", (int) (sec / 604800),
            ((int) (sec / 604800) > 1) ? SLWEEKS : SLWEEK);
    sec -= (((int) (sec / 604800)) * 604800);
  }
  if (sec >= 86400) {
    sprintf(&s[strlen(s)], "%d %s ", (int) (sec / 86400),
            ((int) (sec / 86400) > 1) ? SLDAYS : SLDAY);
    sec -= (((int) (sec / 86400)) * 86400);
  }
  if (sec >= 3600) {
    sprintf(&s[strlen(s)], "%d %s ", (int) (sec / 3600),
            ((int) (sec / 3600) > 1) ? SLHOURS : SLHOUR);
    sec -= (((int) (sec / 3600)) * 3600);
  }
  if (sec >= 60) {
    sprintf(&s[strlen(s)], "%d %s ", (int) (sec / 60),
            ((int) (sec / 60) > 1) ? SLMINUTES : SLMINUTE);
    sec -= (((int) (sec / 60)) * 60);
  }
  strcpy(gs_duration_temp, s);
  if (gs_duration_temp[strlen(gs_duration_temp) - 1] == ' ')
    gs_duration_temp[strlen(gs_duration_temp) - 1] = 0;
  return gs_duration_temp;
}

static int onchan(char *nick, char *chan)
{
  struct chanset_t *ch;
  memberlist *m;

  ch = findchan_by_dname(chan);
  if (!ch)
    return 0;
  m = ismember(ch, nick);
  if (!m)
    return 0;
  else if (chan_issplit(m))
    return 0;
  else
    return 1;
}

/* handonchan():
 * checks if the given user is on the channel and returns its nick
 */
static char *handonchan(char *hand, char *chan)
{
  struct chanset_t *ch;
  memberlist *m;

  ch = findchan_by_dname(chan);
  if (!ch)
    return 0;
  if (ch->channel.members > 0) {
    for (m = ch->channel.member; m; m = m->next) {
      if (m->user) {
        if (m->user->handle && !rfc_casecmp(m->user->handle, hand))
          return m->nick;
      }
    }
  }
  return NULL;
}

/* onanychan():
 * checks if the given nickname is on any of the bot's chans.
 */
static struct chanset_t *onanychan(char *nick)
{
  struct chanset_t *ch;
  memberlist *m;

  for (ch = chanset; ch; ch = ch->next) {
    m = ismember(ch, nick);
    if (m && !chan_issplit(m))
      return ch;
  }
  return NULL;
}

/* handonanychan():
 * checks if the given user is on any channel (no matter under which nick)
 */
static struct chanset_t *handonanychan(char *hand)
{
  struct chanset_t *ch;
  memberlist *m;

  for (ch = chanset; ch; ch = ch->next) {
    if (ch->channel.members > 0) {
      for (m = ch->channel.member; m; m = m->next) {
        if (m->user) {
          if (m->user->handle && !rfc_casecmp(m->user->handle, hand))
            return ch;
        }
      }
    }
  }
  return NULL;
}

static void add_seenreq(char *nick, char *from, char *host, char *chan,
			time_t when)
{
  seenreq *l, *nl;
  seenreq_by *b, *nb;
  char buf[10] = "[secret]";

  Context;
  if (!tell_seens)
    return;
  if (strcmp(chan, "[partyline]") && secretchan(chan))
    chan = buf;
  for (l = requests; l; l = l->next) {
    if (!strcasecmp(nick, l->nick)) {
      for (b = l->by; b; b = b->next) {
	if (!strcasecmp(from, b->who)) {
	  nfree(b->chan);
	  b->chan = nmalloc(strlen(chan) + 1);
	  strcpy(b->chan, chan);
	  b->when = when;
	  return;
	}
      }
      b = l->by;
      while (b && b->next)
        b = b->next;
      nb = nmalloc(sizeof(seenreq_by));
      nb->who = nmalloc(strlen(from) + 1);
      strcpy(nb->who, from);
      nb->host = nmalloc(strlen(host) + 1);
      strcpy(nb->host, host);
      nb->chan = nmalloc(strlen(chan) + 1);
      strcpy(nb->chan, chan);
      nb->when = when;
      nb->next = NULL;
      if (l->by)
        b->next = nb;
      else
        l->by = nb;
      return;
    }
  }
  nb = nmalloc(sizeof(seenreq_by));
  nb->who = nmalloc(strlen(from) + 1);
  strcpy(nb->who, from);
  nb->host = nmalloc(strlen(host) + 1);
  strcpy(nb->host, host);
  nb->chan = nmalloc(strlen(chan) + 1);
  strcpy(nb->chan, chan);
  nb->when = when;
  nb->next = NULL;
  l = requests;
  while (l && l->next)
    l = l->next;
  nl = nmalloc(sizeof(seenreq));
  nl->nick = nmalloc(strlen(nick) + 1);
  strcpy(nl->nick, nick);
  nl->by = nb;
  nl->next = NULL;
  if (requests)
    l->next = nl;
  else
    requests = nl;
}

static int expmem_seenreq()
{
  seenreq *l;
  seenreq_by *b;
  int size;

  size = 0;
  for (l = requests; l; l = l->next) {
    size += sizeof(seenreq);
    size += strlen(l->nick) + 1;
    for (b = l->by; b; b = b->next) {
      size += sizeof(seenreq_by);
      size += strlen(b->who) + 1;
      size += strlen(b->host) + 1;
      size += strlen(b->chan) + 1;
    }
  }
  return size;
}

static int count_seenreq(seenreq_by *b)
{
  seenreq_by *l;
  int nr;

  nr = 0;
  for (l = b; l; l = l->next)
    nr++;
  return nr;
}

static void free_seenreq()
{
  seenreq *l, *ll;
  seenreq_by *b, *bb;

  Context;
  l = requests;
  while (l) {
    b = l->by;
    while (b) {
      bb = b->next;
      nfree(b->who);
      nfree(b->host);
      nfree(b->chan);
      nfree(b);
      b = bb;
    }
    ll = l->next;
    nfree(l->nick);
    nfree(l);
    l = ll;
  }
  requests = NULL;
}

static void report_seenreq(char *channel, char *nick)
{
  seenreq *l, *ll;
  seenreq_by *b, *bb;
  char *reply, *tmp;
  int nr;

  if (!tell_seens)
    return;
  ll = NULL;
  l = requests;
  reply = NULL;
  while (l) {
    if (!strcasecmp(l->nick, nick)) {
      reset_global_vars();
      glob_slang = slang_find(coreslangs, slang_chanlang_get(chanlangs, channel));
      glob_nick = nick;
      nr = count_seenreq(l->by);
      if (nr == 1) {
        glob_seenrequest = l;
        dprintf(DP_HELP, "NOTICE %s :%s\n", l->nick, SLONELOOK);
      } else {
        sortrequests(l);
        glob_seenrequest = l;
        glob_seenrequests = nr;
        tmp = SLMORELOOKS;
        reply = nmalloc(strlen(tmp) + 1);
	    strcpy(reply, tmp);
	    nr = 0;
        for (b = l->by; b; b = b->next) {
          nr++;
          reply = nrealloc(reply, strlen(reply) + ((nr == 1) ? 1 : 2) + strlen(b->who) + 1);
          sprintf(reply, "%s%s%s", reply, (nr == 1) ? " " : ", ", b->who);
	    }
        tmp = SLLASTLOOK;
        reply = nrealloc(reply, strlen(reply) + 2 + strlen(tmp) + 1);
        sprintf(reply, "%s. %s", reply, tmp);
	    dprintf(DP_HELP, "NOTICE %s :%s\n", l->nick, reply);
        nfree(reply);
      }
      b = l->by;
      while (b) {
        bb = b->next;
        nfree(b->who);
        nfree(b->host);
        nfree(b->chan);
        nfree(b);
        b = bb;
      }
      nfree(l->nick);
      if (ll)
        ll->next = l->next;
      else
        requests = l->next;
      nfree(l);
      if (ll)
        l = ll->next;
      else
        l = requests;
    } else {
      ll = l;
      l = l->next;
    }
  }
}

static void start_seentime_calc()
{
  struct timeval t;

  gettimeofday(&t, NULL);
  glob_presearch = (float) t.tv_sec + (((float) t.tv_usec) / 1000000);
}

static void end_seentime_calc()
{
  struct timeval t;

  gettimeofday(&t, NULL);
  glob_aftersearch = (float) t.tv_sec + (((float) t.tv_usec) / 1000000);
  glob_total_searchtime += glob_aftersearch - glob_presearch;
  glob_total_queries++;
}
