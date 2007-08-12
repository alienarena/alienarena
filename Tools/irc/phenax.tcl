bind pub - !aaserver pub:invokeserver
bind pub - !aafov pub:invokefov
bind pub - !aastats pub:invokestats
bind pub - !alias pub:invokealias

proc pub:invokealias { nick host hand chan text } {
	set function [lindex [split $text] 0]
	set alias [lindex [split $text] 1]
	set ip [lindex [split $text] 2]

	set result [exec /bin/bash /home/alienarena/eggdrop/scripts/Phenax/alias/managealias.sh $function $alias $ip]
	
	foreach line [split $result \n] {
		putmsg $chan "$line"
	}
}

proc pub:invokeserver { nick host hand chan text } {
        set addr [lindex [split $text] 0]
        set result [exec /bin/bash /home/alienarena/eggdrop/scripts/Phenax/aaserver/aaserver.sh $addr]

        foreach line [split $result \n] {
                putmsg $chan "$line"
        }
}

proc pub:invokestats { nick host hand chan text } {
	set name [lindex [split $text] 0]
	set result [exec /bin/bash /home/alienarena/eggdrop/scripts/Phenax/aastats/aastats.sh $name]

	foreach line [split $result \n] {
		set line [string map [list COLOR \003] $line]
		putmsg $chan "$line"
	}
}

proc pub:invokefov { nick host hand chan text } {
	set res [lindex [split $text] 0]
        set result [exec /usr/bin/ruby /home/alienarena/eggdrop/scripts/Phenax/aafov/aafov.rb $res]

        foreach line [split $result \n] {
                putmsg $chan "$line"
        }
}

