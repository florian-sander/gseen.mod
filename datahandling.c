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


static void write_seens()
{
  seenreq *r;
  seenreq_by *b;
  FILE *f;
  char s[125];

  Context;
  /* putlog(LOG_MISC, "*", "Saving seen data..."); */
  if (!gseenfile[0])
    return;
  sprintf(s, "%s~new", gseenfile);
  f = fopen(s, "w");
  chmod(s, 0600);
  if (f == NULL) {
    putlog(LOG_MISC, "*", "ERROR writing gseen file.");
    return;
  }
  fprintf(f, "# gseen data file v1.\n");
  write_seen_tree_target = f;
  btree_getall(&seentree, write_seen_tree);
  for (r = requests; r; r = r->next)
    for (b = r->by; b; b = b->next)
      /* @ nick by host chan when */
      fprintf(f, "@ %s %s %s %s %lu\n", r->nick, b->who, b->host, b->chan,
              b->when);
  fclose(f);
  unlink(gseenfile);
  movefile(s, gseenfile);
  /* putlog(LOG_MISC, "*", "Done."); */
  return;
}

static void read_seens()
{
  FILE *f;
  char buf[512], *s, *type, *nick, *host, *chan, *msg, *by;
  time_t when;
  int spent, iType, i;

  Context;
  f = fopen(gseenfile, "r");
  if (f == NULL) {
    putlog(LOG_MISC, "*", "Can't open gseen file, creating new database...");
    return;
  }
  while (!feof(f)) {
    buf[0] = 0;
    s = buf;
    fgets(s, 511, f);
    i = strlen(buf);
    if (!i)
      continue;
    if (buf[i - 1] == '\n')
      buf[i - 1] = 0;
    if ((buf[0] == 0) || (buf[0] == '#'))
      continue;
    type = newsplit(&s);
    if (!strcmp(type, "!")) {
      nick = newsplit(&s);
      host = newsplit(&s);
      chan = newsplit(&s);
      iType = atoi(newsplit(&s));
      when = (time_t) atoi(newsplit(&s));
      spent = atoi(newsplit(&s));
      msg = s;
      add_seen(iType, nick, host, chan, msg, when, spent);
    } else if (!strcmp(type, "@")) {
      nick = newsplit(&s);
      by = newsplit(&s);
      host = newsplit(&s);
      chan = newsplit(&s);
      when = (time_t) atoi(newsplit(&s));
      add_seenreq(nick, by, host, chan, when);
    }
  }
  fclose(f);
  Context;
  return;
}

static void purge_seens()
{
  seenreq *r, *rr;
  seenreq_by *b, *bb;

  Context;
  if (!expire_seens)
    return;
  btree_getall_expanded(&seentree, purge_seen_tree);
  debug0("purge done");
  r = requests;
  rr = NULL;
  while (r) {
    b = r->by;
    bb = NULL;
    while (b) {
      if ((now - b->when) > (expire_seens * 86400)) {
        debug2("request for %s from %s has expired.", r->nick, b->who);
        nfree(b->who);
        nfree(b->host);
        nfree(b->chan);
        if (bb) {
          bb->next = b->next;
          nfree(b);
          b = bb->next;
        } else {
          r->by = b->next;
          nfree(b);
          b = r->by;
        }
      } else {
        bb = b;
        b = b->next;
      }
    }
    if (!r->by) {
      debug1("no further seen requests for %s, deleting", r->nick);
      nfree(r->nick);
      if (rr) {
        rr->next = r->next;
        nfree(r);
        r = rr->next;
      } else {
        requests = r->next;
        nfree(r);
        r = requests;
      }
    } else {
      rr = r;
      r = r->next;
    }
  }
}
