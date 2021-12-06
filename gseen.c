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

#define MAKING_GSEEN
#define MODULE_NAME "gseen"
#define MODULE_VERSION "1.2.0"
#define MODULE_NUMVERSION 10200
#include "../module.h"
#include "../irc.mod/irc.h"
#include "../server.mod/server.h"
#include "../channels.mod/channels.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h> /* for time_t */

#undef global
static Function *global = NULL, *irc_funcs = NULL, *server_funcs = NULL, *channels_funcs = NULL;

#ifndef EGG_IS_MIN_VER
#define EGG_IS_MIN_VER(ver)             ((ver) <= 10400)
#endif

#ifndef EGG_IS_MAX_VER
#define EGG_IS_MAX_VER(ver)		((ver) >= 10400)
#endif

#ifndef Context
#define Context context
#endif

#ifndef findchan_by_dname
#define findchan_by_dname findchan
#endif

#include "gseen.h"
#include "seenlang.h"

static struct slang_header *coreslangs = NULL;
static gseenres *results = NULL;
static seenreq *requests = NULL;
static ignoredword *ignoredwords = NULL;
static char *bnsnick = NULL;
static char *bnschan = NULL;
static char *seen_reply = NULL;
static char *temp_wildmatch_host;
static int numresults = 0;
static double glob_presearch, glob_aftersearch;
int numseens, glob_total_queries;
double glob_total_searchtime;

static char gseenfile[121] = "gseen.dat";
static char no_pub[121];
static char quiet_seen[121];
static char quiet_ai_seen[121];
static char no_log[121];
static char ignore_words[1024];
static char default_slang[21] = "eng";
static int gseen_numversion = MODULE_NUMVERSION;
static int save_seens = 60;
static int save_seens_temp = 1;
static int expire_seens = 60;
static int maxseen_thr = 0;
static int maxseen_time = 0;
static int seenflood_thr = 0;
static time_t seenflood_time = 0;
static int use_handles = 0;
static int tell_seens = 1;
static int botnet_seen = 1;
int fuzzy_search = 1;          // search for the same user under a differnt nick
static int wildcard_search = 1;// allow wildcard seaching? ("*!*@*.isp.de")
static int max_matches = 500;  // break if there are more than X matches
static int hide_secret_chans = 1;	// #chan (+secret) => [secret]
static int seen_nick_len = 9;

#include "global_vars.c"
#define SLANG_NOTYPES 1
#define SLANG_NOFACTS 1
#define SLANG_NOGETALL 1
#define SLANG_NOVALIDATE 1
#include "slang.c"
#include "slang_gseen_commands.c"
#include "generic_binary_tree.c"
#include "seentree.c"
#include "datahandling.c"
#include "sensors.c"
#include "do_seen.c"
#include "gseencmds.c"
#include "ai.c"
#include "misc.c"
#include "tclcmds.c"

static int gseen_expmem()
{
  int size = 0;

  size += seentree_expmem();
  size += expmem_seenresults();
  size += expmem_seenreq();
  size += expmem_ignoredwords();
  size += slang_expmem(coreslangs);
  size += slang_glob_expmem();
  size += slang_chanlang_expmem(chanlangs);
  if (bnsnick)
    size += strlen(bnsnick) + 1;
  if (bnschan)
    size += strlen(bnschan) + 1;
  if (seen_reply) {
    size += strlen(seen_reply) + 1;
  }
  return size;
}

static void free_gseen()
{
  seentree_free();
  slang_free(coreslangs);
  slang_chanlang_free(chanlangs);
  if (seen_reply)
    nfree(seen_reply);
  return;
}

/* a report on the module status */
static void gseen_report(int idx, int details)
{
  int size = 0;

  Context;
  if (details) {
    size = gseen_expmem();
    dprintf(idx, "    using %d bytes\n", size);
  }
}

static void gseen_minutely ()
{
  if (save_seens_temp >= save_seens) {
    write_seens();
    save_seens_temp = 1;
  } else
    save_seens_temp++;
}

static void gseen_daily ()
{
  Context;
  purge_seens();
}

static tcl_strings my_tcl_strings[] =
{
  {"gseenfile", gseenfile, 121, 0},
  {"ai-seen-ignore", ignore_words, 1024, 0},
  {"no-pub-seens", no_pub, 121, 0},
  {"quiet-seens", quiet_seen, 121, 0},
  {"quiet-ai-seens", quiet_ai_seen, 121, 0},
  {"no-log", no_log, 121, 0},
  {"no-seendata", no_log, 121, 0},
  {"default-slang", default_slang, 20, 0},
  {0, 0, 0, 0}
};

static tcl_ints my_tcl_ints[] =
{
  {"save-seens", &save_seens, 0},
  {"expire-seens", &expire_seens, 0},
  {"use-handles", &use_handles, 0},
  {"tell-seens", &tell_seens, 0},
  {"botnet-seens", &botnet_seen, 0},
  {"max-matches", &max_matches, 0},
  {"fuzzy-search", &fuzzy_search, 0},
  {"wildcard-search", &wildcard_search, 0},
  {"hide-secret-chans", &hide_secret_chans, 0},
  {"seen-nick-len", &seen_nick_len, 0},
  {0, 0, 0}
};

static tcl_coups my_tcl_coups[] =
{
  {"max-seens", &maxseen_thr, &maxseen_time},
  {0, 0, 0},
};

static char *gseen_close()
{
  Context;
  write_seens();
  slang_glob_free();
  free_gseen();
  free_seenreq();
  free_seenresults();
  free_ignoredwords();
  if (bnsnick)
    nfree(bnsnick);
  if (bnschan)
    nfree(bnschan);
  rem_tcl_strings(my_tcl_strings);
  rem_tcl_ints(my_tcl_ints);
  rem_tcl_coups(my_tcl_coups);
  rem_tcl_commands(mytcls);
  rem_tcl_commands(gseentcls);
  rem_tcl_commands(seendebugtcls);
  rem_tcl_commands(gseentcls);
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_join, seen_join);
  rem_builtins(H_kick, seen_kick);
  rem_builtins(H_nick, seen_nick);
  rem_builtins(H_part, seen_part);
  rem_builtins(H_sign, seen_sign);
  rem_builtins(H_splt, seen_splt);
  rem_builtins(H_rejn, seen_rejn);
  rem_builtins(H_pub, seen_pub);
  rem_builtins(H_msg, seen_msg);
  rem_builtins(H_bot, seen_bot);
  del_hook(HOOK_MINUTELY, (Function) gseen_minutely);
  del_hook(HOOK_DAILY, (Function) gseen_daily);
  module_undepend(MODULE_NAME);
  return NULL;
}

char *gseen_start();

static Function gseen_table[] =
{
  (Function) gseen_start,
  (Function) gseen_close,
  (Function) gseen_expmem,
  (Function) gseen_report,
  /* 4 - 7 */
  (Function) findseens,
  (Function) free_seenresults,
  (Function) gseen_duration,
  (Function) & glob_seendat,
  (Function) & numresults,
  (Function) & fuzzy_search,
  (Function) & numseens,
  (Function) & glob_total_queries,
  (Function) & glob_total_searchtime,
  (Function) & gseen_numversion,
};

char *gseen_start(Function * global_funcs)
{
  global = global_funcs;
  Context;
  module_register(MODULE_NAME, gseen_table, 1, 2);
  if (!(irc_funcs = module_depend(MODULE_NAME, "irc", 1, 0)))
    return "You need the irc module to use the gseen module.";
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0)))
    return "You need the server module to use the gseen module.";
  if (!(channels_funcs = module_depend(MODULE_NAME, "channels", 1, 0)))
    return "You need the channels module to use the gseen module.";
  if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
    if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
      if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
        if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
          if (!module_depend(MODULE_NAME, "eggdrop", 105, 0)) {
            if (!module_depend(MODULE_NAME, "eggdrop", 104, 0)) {
              module_undepend(MODULE_NAME);
              return "This module requires eggdrop1.4.0 or later";
            }
          }
        }
      }
    }
  }
  chanlangs = NULL;
  coreslangs = NULL;
  slang_glob_init();

  results = NULL;
  requests = NULL;
  ignoredwords = NULL;
  bnsnick = NULL;
  bnschan = NULL;
  seen_reply = NULL;

  numresults = 0;
  numseens = 0;
  glob_total_queries = 0;
  glob_total_searchtime = 0.0;
  ignore_words[0] = 0;
  no_pub[0] = 0;
  quiet_seen[0] = 0;
  no_log[0] = 0;
  seentree_init();
  add_tcl_strings(my_tcl_strings);
  add_tcl_ints(my_tcl_ints);
  add_tcl_coups(my_tcl_coups);
  add_tcl_commands(mytcls);
  add_tcl_commands(seendebugtcls);
  add_tcl_commands(gseentcls);
  add_builtins(H_dcc, mydcc);
  add_builtins(H_join, seen_join);
  add_builtins(H_kick, seen_kick);
  add_builtins(H_nick, seen_nick);
  add_builtins(H_part, seen_part);
  add_builtins(H_sign, seen_sign);
  add_builtins(H_sign, seen_sign);
  add_builtins(H_splt, seen_splt);
  add_builtins(H_rejn, seen_rejn);
  add_builtins(H_pub, seen_pub);
  add_builtins(H_msg, seen_msg);
  add_builtins(H_bot, seen_bot);
  read_seens();
  add_hook(HOOK_MINUTELY, (Function) gseen_minutely);
  add_hook(HOOK_DAILY, (Function) gseen_daily);
#if EGG_IS_MIN_VER(10503)
  initudef(1, "noseendata", 1);
  initudef(1, "quietseens", 1);
  initudef(1, "quietaiseens", 1);
  initudef(1, "nopubseens", 1);
#endif
  glob_slang_cmd_list = slang_commands_list_add(glob_slang_cmd_list, slang_text_gseen_command_table);
  putlog(LOG_MISC, "*", "gseen.mod v%s loaded.", MODULE_VERSION);
  return NULL;
}
