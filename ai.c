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

static int quietaiseens(char *chan)
{
  char buf[121], *b;

  Context;
  strncpy(buf, quiet_ai_seen, 120);
  buf[120] = 0;
  b = buf;
  while (b[0])
    if (!strcasecmp(chan, newsplit(&b)))
      return 1;
#if EGG_IS_MIN_VER(10503)
  if (ngetudef("quietaiseens", chan))
    return 1;
#endif
  return 0;
}

static int tcl_pubmseen STDVAR
{
  char *nick, *uhost, *chan, *text;
  char buf[1024];
  char *words, *word;
  seendat *l;
  int i;

  Context;
  BADARGS(6, 6, " nick uhost hand chan text");
  nick = argv[1];
  uhost = argv[2];
  chan = argv[4];
  text = argv[5];
  reset_global_vars();
  glob_slang = slang_find(coreslangs, slang_chanlang_get(chanlangs, chan));
  glob_nick = nick;
  for (i = 0; i < strlen(text); i++)
    if (strchr("!?.,\"", text[i]))
      text[i] = ' ';
  strncpy(buf, ignore_words, 1023);
  buf[1023] = 0;
  words = buf;
  while (words[0])
    add_ignoredword(newsplit(&words));
  strncpy(buf, text, 1023);
  buf[1023] = 0;
  words = buf;
  while (words[0]) {
    word = newsplit(&words);
    if (word_is_ignored(word))
      continue;
    l = findseen(word);
    if (l) {
      if (quietaiseens(chan)) {
	set_prefix(SLNOTPREFIX);
        dprintf(DP_HELP, "NOTICE %s :%s%s\n", nick, reply_prefix,
        	do_seen(word, nick, uhost, chan, 0));
      } else {
	set_prefix(SLPUBPREFIX);
        dprintf(DP_HELP, "PRIVMSG %s :%s%s\n", chan, reply_prefix,
        	do_seen(word, nick, uhost, chan, 0));
      }
      add_seenreq(word, nick, uhost, chan, now);
      free_ignoredwords();
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  }
  free_ignoredwords();
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static tcl_cmds mytcls[] =
{
  {"*pubm:seen", tcl_pubmseen},
  {"*chjn:gseen", gseen_chjn},
  {"*chpt:gseen", gseen_chpt},
  {0, 0}
};

static void add_ignoredword(char *word)
{
  ignoredword *l, *nl;

  l = ignoredwords;
  while (l && l->next)
    l = l->next;
  nl = nmalloc(sizeof(ignoredword));
  nl->word = nmalloc(strlen(word) + 1);
  strcpy(nl->word, word);
  nl->next = NULL;
  if (ignoredwords)
    l->next = nl;
  else
    ignoredwords = nl;
}

static void free_ignoredwords()
{
  ignoredword *l, *ll;

  l = ignoredwords;
  while (l) {
    ll = l->next;
    nfree(l->word);
    nfree(l);
    l = ll;
  }
  ignoredwords = NULL;
}

static int expmem_ignoredwords()
{
  ignoredword *l;
  int size = 0;

  for (l = ignoredwords; l; l = l->next) {
    size += sizeof(ignoredword);
    size += strlen(l->word) + 1;
  }
  return size;
}

static int word_is_ignored(char *word)
{
  ignoredword *l;

  for (l = ignoredwords; l; l = l->next)
    if (!strcasecmp(l->word, word))
      return 1;
  return 0;
}
