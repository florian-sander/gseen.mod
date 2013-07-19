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

static int tcl_setchanseenlang STDVAR
{
  Context;
  BADARGS(3, 3, " channel language");
  chanlangs = slang_chanlang_add(chanlangs, argv[1], argv[2]);
  return TCL_OK;
}

static int tcl_loadseenslang STDVAR
{
//  int ret = 0;
  char *shortname, *longname, *filename;
  struct slang_header *slang;

  Context;
  BADARGS(4, 4, " language description langfile");
  shortname = argv[1];
  longname = argv[2];
  filename = argv[3];
  coreslangs = slang_create(coreslangs, shortname, longname);
  slang = slang_find(coreslangs, shortname);
  Assert(slang);
  if (!slang_load(slang, filename)) {
    Tcl_AppendResult(irp, "Couldn't open seenslang file!!!", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static tcl_cmds gseentcls[] =
{
  {"loadseenslang", tcl_loadseenslang},
  {"setchanseenlang", tcl_setchanseenlang},
  {0, 0}
};
