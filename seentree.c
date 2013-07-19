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

static struct generic_binary_tree seentree;

static void seentree_init();
static int seentree_expmem();
static void seentree_free();
static int compareseens(void *, void *);
static int expmemseen(void *);
static void add_seen(int, char *, char *, char *, char *,
                     time_t, int);
static void freeseen(void *);
static seendat *findseen(char *);
static void wildmatch_seens(char *, char *, int);
static void process_wildmatch_seens(void *);
static void write_seen_tree(void *);
static void purge_seen_tree(void *);
static int count_seens();
static void _count_seens(void *);


static void seentree_init()
{
  seentree.root = NULL;
  seentree.comparedata = compareseens;
  seentree.expmemdata = expmemseen;
  seentree.freedata = freeseen;
}

static int seentree_expmem()
{
  return btree_expmem(&seentree);
}

static void seentree_free()
{
  btree_freetree(&seentree);
  seentree.root = NULL;
}

static int compareseens(void *first, void *second)
{
  return rfc_casecmp(((seendat *) first)->nick, ((seendat *) second)->nick);
}

// add another entry to the tree
static void add_seen(int type, char *nick, char *host, char *chan, char *msg,
                     time_t when, int spent)
{
  seendat *newseen;

  newseen = nmalloc(sizeof(seendat));
  newseen->type = type;
  newseen->nick = nmalloc(strlen(nick) + 1);
  strcpy(newseen->nick, nick);
  newseen->host = nmalloc(strlen(host) + 1);
  strcpy(newseen->host, host);
  newseen->chan = nmalloc(strlen(chan) + 1);
  strcpy(newseen->chan, chan);
  newseen->msg = nmalloc(strlen(msg) + 1);
  strcpy(newseen->msg, msg);
  newseen->when = when;
  newseen->spent = spent;
  btree_add(&seentree, newseen);
}

static void freeseen(void *what)
{
  seendat *s = (seendat *) what;
  
  Assert(s);
  Assert(s->nick);
  Assert(s->host);
  Assert(s->chan);
  Assert(s->msg);

  nfree(s->nick);
  nfree(s->host);
  nfree(s->chan);
  nfree(s->msg);
  nfree(s);
}

static int expmemseen(void *what)
{
  int size = 0;
  seendat *d = (seendat *) what;
  
  size += sizeof(seendat);
  size += strlen(d->nick) + 1;
  size += strlen(d->host) + 1;
  size += strlen(d->chan) + 1;
  size += strlen(d->msg) + 1;
  return size;
}

// finds a seen entry in the tree
seendat findseen_temp;
static seendat *findseen(char *nick)
{
  findseen_temp.nick = nick;
  return btree_get(&seentree, &findseen_temp);
}

// function to find all nicks that match a host
// (calls btree_getall() which calls a target function for each item)
// host: user's hostmask (used if search query doesn't contain any wildcards)
// mask: search mask
// wild: defines if we want to use the mask, or host for the search
static char *wildmatch_host, *wildmatch_mask;
int wildmatch_wild;
static void wildmatch_seens(char *host, char *mask, int wild)
{
  wildmatch_host = host;
  wildmatch_mask = mask;
  wildmatch_wild = wild;
  btree_getall(&seentree, process_wildmatch_seens);
}

/* process_wildmatch_seens():
 * gets called from the binary tree for each existing item.
 */
static void process_wildmatch_seens(void *data)
{
  seendat *s = (seendat *) data;
  
  if ((numresults > max_matches) && (max_matches > 0)) // Don't return too many
    return;                                            // matches...
  if (!wildmatch_wild) {
    if (wild_match(wildmatch_host, s->host))
      add_seenresult(s);
  } else {
    temp_wildmatch_host = my_realloc(temp_wildmatch_host, strlen(s->nick) + 1 + strlen(s->host) + 1);
    strcpy(temp_wildmatch_host, s->nick);
    strcat(temp_wildmatch_host, "!");
    strcat(temp_wildmatch_host, s->host);
    if (wild_match(wildmatch_mask, s->nick) || wild_match(wildmatch_mask, temp_wildmatch_host))
      add_seenresult(s);
  }
}

// write seendata in the datafile
FILE *write_seen_tree_target;
static void write_seen_tree(void *data)
{
  seendat *node = (seendat *) data;
  
  /* format: "! nick host chan type when spent msg" */
  fprintf(write_seen_tree_target, "! %s %s %s %d %lu %d %s\n", node->nick,
          node->host, node->chan, node->type, node->when, node->spent,
          node->msg);
}

// recursive function to remove old data
// QUESTION: What happens if one of the nodes get moved by killseen()?
//           Possible bug/crash?
//           I think it should not be a problem. When killseen() is called the
//           first time, recursion already reached its end and no pointers
//           are accessed anymore. But I'm not sure... maybe I'm wrong.
static void purge_seen_tree(void *data)
{
  seendat *node = (seendat *) data;
  
  if ((now - node->when) > (expire_seens * 86400)) {
    debug1("seen data for %s has expired.", node->nick);
    btree_remove(&seentree, node);
  }
}

// counts the number of nicks in the database
static int count_seens_temp;
static int count_seens()
{
  count_seens_temp = 0;
  btree_getall(&seentree, _count_seens);
  return count_seens_temp;
}

static void _count_seens(void *node)
{
  count_seens_temp++;
}

static int tcl_killseen STDVAR
{
  Context;
  BADARGS(2, 2, " nick");
  findseen_temp.nick = argv[1];
  btree_remove(&seentree, &findseen_temp);
  return TCL_OK;
}

static tcl_cmds seendebugtcls[] =
{
  {"killseen", tcl_killseen},
  {0, 0}
};
