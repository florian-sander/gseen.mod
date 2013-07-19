#####################################################################
#
# gseen.selectlang v1.0.0
#
# This is a simple script which selects a language based on the 
# user's host.
#
# It only works for /msg commands.
#
# If the user is in a channel which has a language defined, gseen's
# internal functions will override this selection and use the language
# of the channel instead.
#
#####################################################################


# Here you can define which language to use for which host.
# The first part is the mask for the host, and the second part
# is the language which should be used for this host.

set tld-langs {
  {"*.de"		"de"}
  {"*.at"		"de"}
  {"*.ch"		"de"}
  {"*.t-dialin.net"	"de"}
  {"*.t-ipconnect.net"	"de"}
  {"*.pl"		"pl"}
  {"*.jp"		"ja"}
}

#################################################


proc selectlang:getlang {uhost} {
  global tld-langs
  
  foreach tld ${tld-langs} {
    if {[string match [lindex $tld 0] $uhost]} {
      return [lindex $tld 1]
    }
  }
  return ""
}

proc sl:rebind {oldtarget newtarget} {
  foreach binding [binds msg] {
    if {[lindex $binding 4] == $oldtarget} {
      unbind [lindex $binding 0] [lindex $binding 1] [lindex $binding 2] [lindex $binding 4]
      bind [lindex $binding 0] [lindex $binding 1] [lindex $binding 2] $newtarget
    }
  }
}

proc sl:msg:trigger {nick uhost hand rest target} {
  global default-slang
  
  set lang [selectlang:getlang $uhost]
  set old-slang ${default-slang}
  if {$lang != ""} {
    set default-slang $lang
    putlog "using '$lang'..."
  }
  $target $nick $uhost $hand $rest
  set default-slang ${old-slang}
}

sl:rebind *msg:seen sl:msg:seen
proc sl:msg:seen {nick uhost hand rest} {
  sl:msg:trigger $nick $uhost $hand $rest *msg:seen
}

sl:rebind *msg:seenstats sl:msg:seenstats
proc sl:msg:seenstats {nick uhost hand rest} {
  sl:msg:trigger $nick $uhost $hand $rest *msg:seenstats
}

sl:rebind *msg:seennick sl:msg:seennick
proc sl:msg:seennick {nick uhost hand rest} {
  sl:msg:trigger $nick $uhost $hand $rest *msg:seennick
}