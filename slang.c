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

static struct slang_header *slang_find(struct slang_header *, char *);

#include "slang_text.c"
#include "slang_multitext.c"
#include "slang_ids.c"
#ifndef SLANG_NOTYPES
#include "slang_types.c"
#endif
#include "slang_duration.c"
#ifndef SLANG_NOFACTS
#include "slang_facts_places.c"
#include "slang_facts.c"
#endif
#include "slang_chanlang.c"


struct slang_header {
  struct slang_header *next;
  char *lang;
  char *desc;
  struct slang_id *ids;
#ifndef SLANG_NOTYPES
  struct slang_type *types;
#endif
  struct slang_duration *durations;
};

static void slang_glob_init()
{
  glob_slang_cmd_list = NULL;
}

static int slang_glob_expmem()
{
  return slang_commands_list_expmem(glob_slang_cmd_list);
}

static void slang_glob_free()
{
  slang_commands_list_free(glob_slang_cmd_list);
  glob_slang_cmd_list = NULL;
}

static struct slang_header *slang_create(struct slang_header *list, char *lang, char *desc)
{
  struct slang_header *nslang, *l;

  Assert(lang);
  debug2("Creating language '%s' starting by %" PRIdPTR, lang, list);
  for (nslang = list; nslang; nslang = nslang->next)
    if (!strcasecmp(nslang->lang, lang))
      return list;
  nslang = nmalloc(sizeof(struct slang_header));
  nslang->next = NULL;
  nslang->desc = NULL;
  nslang->lang = nmalloc(strlen(lang) + 1);
  strcpy(nslang->lang, lang);
  nslang->desc = nmalloc(strlen(desc) + 1);
  strcpy(nslang->desc, desc);
  nslang->ids = NULL;
#ifndef SLANG_NOTYPES
  nslang->types = NULL;
#endif
  nslang->durations = NULL;
  for (l = list; l && l->next; l = l->next);
  if (l)
    l->next = nslang;
  else {
    Assert(!list);
    list = nslang;
  }
  return list;
}

static int slang_expmem(struct slang_header *what)
{
  int size = 0;

  while (what) {
    size += sizeof(struct slang_header);
    size += strlen(what->lang) + 1;
    size += strlen(what->desc) + 1;
    size += slang_id_expmem(what->ids);
#ifndef SLANG_NOTYPES
    size += slang_type_expmem(what->types);
#endif
    size += slang_duration_expmem(what->durations);
    what = what->next;
  }
  return size;
}

static void slang_free(struct slang_header *what)
{
  struct slang_header *next;

  while (what) {
    next = what->next;
    slang_id_free(what->ids);
#ifndef SLANG_NOTYPES
    slang_type_free(what->types);
#endif
    slang_duration_free(what->durations);
    nfree(what->lang);
    nfree(what->desc);
    nfree(what);
    what = next;
  }
}

static int slang_load(struct slang_header *slang, char *filename)
{
  FILE *f;
  char *buffer, *s;
  char *cmd, *sid, *strtol_ret;
#ifndef SLANG_NOTYPES
  char *type;
#endif
  int line, id;

  Assert(slang);
  putlog(LOG_MISC, "*", "Loading language \"%s\" from %s...", slang->lang, filename);
  f = fopen(filename, "r");
  if (!f) {
    putlog(LOG_MISC, "*", "Couldn't open slangfile \"%s\"!", filename);
    return 0;
  }
  buffer = nmalloc(2000);
  line = 0;
  while (!feof(f)) {
    s = buffer;
    if (fgets(s, 2000, f)) {
      line++;
      // at first, kill those stupid line feeds and carriage returns...
      if (s[strlen(s) - 1] == '\n')
        s[strlen(s) - 1] = 0;
      if (s[strlen(s) - 1] == '\r')
        s[strlen(s) - 1] = 0;
      if (!s[0])
        continue;
      cmd = newsplit(&s);

      if (!strcasecmp(cmd, "T")) {
#ifndef SLANG_NOTYPES
        type = newsplit(&s);
        slang->types = slang_type_add(slang->types, type, s);
#endif
      } else if (!strcasecmp(cmd, "D")) {
        sid = newsplit(&s);
        id = strtol(sid, &strtol_ret, 10);
        if (strtol_ret == sid) {
          putlog(LOG_MISC, "*", "ERROR in slangfile \"%s\", line %d: %s is not a valid "
                 "duration index!", filename, line, sid);
          continue;
        }
        slang->durations = slang_duration_add(slang->durations, id, s);
      } else {
        id = strtol(cmd, &strtol_ret, 10);
        if (strtol_ret == cmd)
          continue;
        slang->ids = slang_id_add(slang->ids, id, s);
      }
    }
  }
  fclose(f);
  nfree(buffer);
  return 1;
}

static struct slang_header *slang_find(struct slang_header *where, char *language)
{
  struct slang_header *slang = NULL;

  // at first, search for the specified language
  for (slang = where; slang; slang = slang->next)
    if (!strcasecmp(slang->lang, language))
      return slang;
  // oops... language seems to be invalid. Let's find the default.
  Assert(default_slang != NULL);
  for (slang = where; slang; slang = slang->next)
    if (!strcasecmp(slang->lang, default_slang))
      return slang;
  // default_slang wasn't found either? *sigh*
  // Let's return the first known language then.
  return where;
}

#ifndef SLANG_NOVALIDATE
/* slang_valid():
 * check if the given language is a valid one
 */
static int slang_valid(struct slang_header *where, char *language)
{
  struct slang_header *slang = NULL;

  for (slang = where; slang; slang = slang->next)
    if (!strcasecmp(slang->lang, language))
      return 1;
  return 0;
}
#endif

static char getslang_error[17];
static char *getslang(int id)
{
  char *text;

  if (!glob_slang) {
    putlog(LOG_MISC, "*", "WARNING! No language selected! (getslang())");
    return "NOLANG";
  }
  text = slang_id_get(glob_slang->ids, id);
  if (!text) {
    snprintf(getslang_error, sizeof(getslang_error), "SLANG%d", id);
    return getslang_error;
  }
  return text;
}

static char *getdur(int idx)
{
  char *text;

  Assert((idx >= 0) && (idx < DURATIONS));
  if (!glob_slang) {
    putlog(LOG_MISC, "*", "WARNING! No language selected! (getdur())");
    return "NOLANG";
  }
  text = slang_duration_get(glob_slang->durations, idx);
  if (!text) {
    snprintf(getslang_error, sizeof(getslang_error), "DUR%d", idx);
    return getslang_error;
  }
  return text;
}

#ifndef SLANG_NOTYPES
static char *getslangtype(char *type)
{
  char *stype;

  if (!glob_slang) {
    putlog(LOG_MISC, "*", "WARNING! No language selected! (getslangtype())");
    return "NOLANG";
  }
  stype = slang_type_get(glob_slang->types, type);
  if (stype)
    return stype;
  else
    return type;
}

static int slangtypetoi(char *slangtype)
{
  char *type;

  if (!glob_slang) {
    putlog(LOG_MISC, "*", "WARNING! No language selected! (slangtypetoi())");
    return T_ERROR;
  }
  type = slang_type_slang2type(glob_slang->types, slangtype);
  if (type) {
    debug1("type: %s", type);
    return typetoi(type);
  } else
    return typetoi(slangtype);
}
#endif

#ifndef SLANG_NOGETALL
static char *getslang_first(int id)
{
  char *text;

  if (!glob_slang) {
    putlog(LOG_MISC, "*", "WARNING! No language selected! (getslang())");
    return "NOLANG";
  }
  text = slang_id_get_first(glob_slang->ids, id);
  if (!text) {
    snprintf(getslang_error, sizeof(getslang_error), "SLANG%d", id);
    return getslang_error;
  }
  return text;
}

static char *getslang_next()
{
  return slang_id_get_next();
}
#endif
