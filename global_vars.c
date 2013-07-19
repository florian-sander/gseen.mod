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

static char *glob_query, *glob_laston, *glob_otherchan, *glob_othernick;
static char *glob_remotebot, *glob_nick;
static struct slang_header *glob_slang;
static seendat *glob_seendat;
static seenreq *glob_seenrequest;
static int glob_seenrequests, glob_totalnicks, glob_totalbytes;

static void reset_global_vars()
{
  glob_query = glob_laston = glob_otherchan = glob_othernick = NULL;
  glob_remotebot = glob_nick = NULL;
  glob_seendat = NULL;
  glob_slang = NULL;
  glob_seenrequest = NULL;
  glob_seenrequests = glob_totalnicks = glob_totalbytes = 0;
}
