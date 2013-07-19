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

/* #define USE_MEMDEBUG 1 */

#define SEEN_JOIN 1
#define SEEN_PART 2
#define SEEN_SIGN 3
#define SEEN_NICK 4
#define SEEN_NCKF 5
#define SEEN_KICK 6
#define SEEN_SPLT 7
#define SEEN_REJN 8
#define SEEN_CHPT 9
#define SEEN_CHJN 10

typedef struct gseen_data {
  int type;
  char *nick;
  char *host;
  char *chan;
  char *msg;
  time_t when;
  int spent;
} seendat;

typedef struct gseen_result {
  struct gseen_result *next;
  seendat *seen;
} gseenres;

typedef struct gseen_requests {
  struct gseen_requests *next;
  char *who;
  char *host;
  char *chan;
  time_t when;
} seenreq_by;

typedef struct gseen_request {
  struct gseen_request *next;
  char *nick;
  struct gseen_requests *by;
} seenreq;

typedef struct gseen_ignorewords {
  struct gseen_ignorewords *next;
  char *word;
} ignoredword;

#ifdef MAKING_GSEEN
static int gseen_expmem();
static void free_gseen();
static int get_spent(char *, char *);
static void write_seens();
static void read_seens();
static char *do_seen(char *, char *, char *, char *, int);
static void add_seenresult(seendat *);
static int expmem_seenresults();
static void free_seenresults();
static void sortresults();
static char *do_seennick(seendat *);
static int onchan(char *, char *);
static char *handonchan(char *, char *);
static struct chanset_t *onanychan(char *);
static struct chanset_t *handonanychan(char *);
static char *do_seenstats();
static void add_seenreq(char *, char *, char *, char *, time_t);
static int expmem_seenreq();
static void free_seenreq();
static void sortrequests(seenreq *);
static void report_seenreq(char *, char *);
static int count_seenreq(seenreq_by *b);
static int expmem_ignoredwords();
static void free_ignoredwords();
static void add_ignoredword(char *word);
static int word_is_ignored(char *word);
static void purge_seens();
static int seenflood();
static int secretchan(char *);
static int nopub(char *);
static int quietseen(char *);
static int quietaiseens(char *);
static int nolog(char *);
static void start_seentime_calc();
static void end_seentime_calc();
#endif


#ifdef MAKING_GSEEN

// tree stuff
static void maskstricthost(const char *, char *);
#endif

// interface for webseen
#define WS_OK 0
#define WS_NORESULT 1
#define WS_NOPARAM 2
#define WS_NOWILDCARDS 3
#define WS_TOOLONGNICK 4
#define WS_TOOMANYMATCHES 5
#define WS_TOOLONGHOST 6

#ifndef MAKING_GSEEN
#define findseens ((gseenres *(*)(char *, int *, int))gseen_funcs[4])
#define free_seenresults ((void (*)())gseen_funcs[5])
#define gseen_duration ((char *(*)(int))gseen_funcs[6])
#define numresults (*(int *)(gseen_funcs[12]))
#define fuzzy_search (*(int *)(gseen_funcs[13]))
#define numseens (*(int *)(gseen_funcs[15]))
#define glob_total_queries (*(int *)(gseen_funcs[16]))
#define glob_total_searchtime (*(double *)(gseen_funcs[17]))
#define gseen_numversion (*(int *)(gseen_funcs[19]))
#else
static gseenres *findseens(char *, int *, int);
static char *gseen_duration(int);
#endif

#ifdef MAKING_GSEEN

#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif
#ifdef realloc
#undef realloc
#endif

#ifdef USE_MEMDEBUG
#define my_malloc nmalloc
#define my_free nfree
#define my_realloc nrealloc
#else
#define my_malloc malloc
#define my_free free
#define my_realloc realloc
#endif

#endif
