#!/usr/bin/tclsh

package require udp 1.0.6

package provide osc

#
# sending stuff:
#
# ::osc::initOut name localhost 12345 udp
# ::osc::initOut name localhost 12345 multicast
# ::osc::initOut name2 localhost 12345 tcp

# initIn 44444
# initIn 44444 udp
# initIn 44444 tcp
# initIn 44444 multicast 226.0.0.1

# ::osc::send name /allo isf 123 "test" 10.43
# ::osc::send name2 /allo isf 123 "test" 10.43
# ::osc::send {name name2} /allo isf 123 "test" 10.43
# ::osc::send * /allo isf 123 "test" 10.43
# ::osc::send name {/allo isf 123 "test" 10.43} {/second ii 11 22} {/trois TTF}
# ::osc::uninit *
#
# receiving stuff:
#
# ...
#

namespace eval ::osc {
	set version 0.1
	puts "package osc, version $version"
}


proc ::osc::debug {onoff} {
	set ::osc::debugflag $onoff
}

::osc::debug 0

proc ::osc::initOut {name host port {type "udp"}} {
	puts "osc initOut $host $port $type"
	global ::osc::f ::osc::stream
	if { [info exists ::osc::f($name)] } {
		puts "$name already initialized"
		return
	}
    if { $type=="udp" || $type=="multicast" } {
	    set ::osc::f($name) [udp_open]
	    set ::osc::stream($name) 0
	    fconfigure $::osc::f($name) -blocking 0 -buffering none -translation binary -remote [list $host $port]
	    if { $type=="multicast" } {
		    fconfigure $::osc::f($name) -mcastadd $host
			puts "using multicast out"
	    }
	    return
    }
    if { $type=="tcp" } {
	    puts "Opening tcp socket on $host:$port"
	    set ::osc::f($name) [socket $host $port]
	    set ::osc::stream($name) 1
	    fconfigure $::osc::f($name) -translation binary
	    return
    }
    puts "osc::initOut : bad type $type"
    return
}

proc ::osc::send {where args} {
	if { [llength [lindex $args 0]]==1 } {
		# single message
		global ::osc::f
		if { $where=={*} } { set where [array names ::osc::f] }
		foreach w $where {
			catch { ::osc::output $w [eval ::osc::buildMsg $args] }
		}
	} else {
		::osc::sendBundle $where $args
	}
}

proc ::osc::output {where data} {
	global ::osc::f ::osc::stream
	if { $::osc::stream($where) } {
		# length of data, as 4 byte int
		puts -nonewline $::osc::f($where) [binary format I1 [string length $data]]
		#puts "output preamble for lenght=[string length $data], size=[string length [binary format I1 [string length $data]]]"
	}
	puts -nonewline $::osc::f($where) $data
	#puts "output of lenght=[string length $data]"
	if { $::osc::stream($where) } {
		flush $::osc::f($where)
	}
}


proc ::osc::sendBundle {where messages} {
	puts "sending [llength $messages] msg to $where"
	#
	# send a bundle
	#
	set data "#bundle\x00[binary format W1 1]"

	#
	# ne verifie pas si la longueur totale est < MTU
	#

	foreach m $messages {
		if{ [catch {set content [::osc::buildMsg 0 {*}$m]}] } continue;
		append data [binary format I1 [string length $content]]
		append data $content
		puts "message is len [string length $content]"
	}
	global ::osc::f
	if { $where=={*} } { set where [array names ::osc::f] }
	foreach w $where {
		::osc::output $w $data
	}
}

proc ::osc::buildMsg {cmd {tag {}} args} {
	return "[padString $cmd][padString ",$tag"][::osc::buildArgs $tag {*}$args]"
}
	
proc ::osc::buildArgs {{tag {}} args} {
	#puts "building tag=$tag, args=$args"
	# sanity check 1...
	# tag should contain only sSichfdTFNI\[\]
	if { [regexp {^[sSichfdTFNI\[\]]*$} $tag]!=1 } {
		puts "******* Illegal tag : tag='$tag' args='$args'"
		throw oscerr "Illegal tag"
	}
	# sanity check 2...
	# the length is counting only sSichfd, and should match args
	if { [string length [string map {T "" F "" N "" I "" \[ "" \] ""} $tag]] != [llength $args] } {
		puts "******* tag mismatch : tag='$tag' args='$args'"
		throw oscerr "Tag mismatch"
	}

	# process!
	set data ""
	set j 0
	for {set i 0} {$i < [string length $tag]} {incr i} {
		set arg [lindex $args $j]
		switch [string index $tag $i] {
			"s" -
			"S" {
				# string and symbol
				append data [padString $arg]
				incr j
			}
			"i" {
				# 32 bit int
				append data [binary format I1 $arg]
				incr j
			}
			"c" {
				# char, sent as 32 bits
				append data [binary format I1 [scan $arg %c]]
				incr j
			}
			"h" {
				# 64 bit int
				append data [binary format W1 $arg]
				incr j
			}
			"f" {
				# 4 bytes float
				append data [binary format R1 $arg]
				incr j
			}
			"d" {
				# 8 bytes float (double)
				append data [binary format Q1 $arg]
				incr j
			}
			"T" -
			"F" -
			"N" - 
			"I" {
				# nothing to do
			}
			"\[" {
				# start an array
				# find the next ']', not counting indented [ ]
				set level 0
				for {set k $i} {$k < [string length $tag]} {incr k} {
					set c [string index $tag $k]
					if { $c=="\[" } {
						incr level;
					} else {
						if { $c=="\]" } {
							incr level -1;
							if { $level==0 } { break }
						}
					}
				}
				incr i
				append data [::osc::buildArgs [string range $tag $i [expr $k-1]] {*}$arg]
				# set to end of array.. so we skip the closing ]
				set i $k
				incr j
			}
		}
	}
	return $data
}


proc ::osc::padString {s} {
	set len [expr ([string length $s]/4+1)*4]
	set rep [expr $len-[string length $s]]
	return "$s[string repeat "\x00" $rep]"
}

proc ::osc::uninitOut {name} {
	global ::osc::f
	after 200
	if { $name=={*} } { set name [array names ::osc::f] }
	foreach n $name {
		close $f($n)
		unset f($n)
	}
}

###########################################################
###########################################################
###########################################################

#
# receiving stuff:
# initIn port
#

proc ::osc::oscDecode {s} {
	#puts "decoding string len [string length $s]"

	# si ca commence par #bundle, on decode un bundle...
	if { [string range $s 0 7]=="#bundle\x00" } {
		set len [string length $s]
		binary scan $s @8W1 timestamp
		# now size(4bytes) followed by [size] bites message
		set pos 16
		for {set pos 16} {$pos<$len} {} {
			binary scan $s @${pos}I1 size
			incr pos 4
			set data [string range $s $pos [expr $pos+$size-1]]
			::osc::oscDecode $data
			incr pos $size
		}
		return
	}
	#
	# un message simple
	#

	# debut: une string terminee par un 0, et suivie de 0 pour faire un multiple de 4
	set k [scan $s %\[^\x00\]%*\[\x00\]%\[^\x00\] cmd tag]
	if {$k==2} {
		#puts "cmd >$cmd<"
		#puts "tag >$tag<"
	} else {
		puts "bad osc format"
		return
	}

	set cmdlen [expr ([string length $cmd]/4+1)*4]
	set taglen [expr ([string length $tag]/4+1)*4]
	set pos [expr $cmdlen+$taglen]
	set params {}
	# pour les arrays
	set level 0

	for {set i 1} {$i<[string length $tag]} {incr i} {
		#puts "type at $i is [string index $tag $i]"
		switch [string index $tag $i] {
			"i" {
				binary scan $s @${pos}I1 val
				lappend params $val
				#puts "integer val=$val"
				incr pos 4
			}
			"c" {
				binary scan $s @[expr ${pos}+3]a1 val
				lappend params $val
				#puts "char $val"
				incr pos 4
			}
			"h" {
				binary scan $s @${pos}W1 val
				lappend params $val
				#puts "integer val=$val"
				incr pos 8
			}
			"f" {
				binary scan $s @${pos}R1 val
				lappend params $val
				#puts "float val=$val"
				incr pos 4
			}
			"d" {
				binary scan $s @${pos}Q1 val
				lappend params $val
				#puts "double val=$val"
				incr pos 8
			}
			"s" {
				scan [string range $s $pos end] %\[^\x00\] val
				lappend params $val
				#puts "string val=>>>$val<<<"
				incr pos [expr ([string length $val]/4+1)*4]
			}
			"T" {
				lappend params true
			}
			"F" {
				lappend params false
			}
			"N" {
				lappend params nil
			}
			"I" {
				lappend params infinitum
			}
			"\[" {
				# start an array... 
				puts "pushing $params at level $level"
				set pile($level) $params
				incr level
				set params {}
			}
			"\]" {
				# end an array... 
				set val $params
				incr level -1
				set params $pile($level)
				lappend params $val
			}
			default {
				puts "unkown tag : [string index $tag $i]"
			}
		}

	}
	if {[info commands "$cmd"]!={}} {
		#puts "jai trouve la commande $cmd"
		eval {$cmd} $params
	} else {
		puts "$cmd $params"
		#foreach i $params { puts "params is $i" }
	}
}


proc ::osc::event {sock} {
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    set me [fconfigure $sock -myport]
    if { $::osc::debugflag } { puts "Received [string length $pkt] from $peer and $me" }
    if {[string length $pkt]>0 } {
	::osc::oscDecode $pkt
    }
    return
}


#
# initIn 44444
# initIn 44444 udp
# initIn 44444 tcp
# initIn 44444 multicast 226.0.0.1
#
proc ::osc::initIn {port {type udp} {ip {}}} {
    global ::osc::sock ::osc::stream
    if { $type=="udp" || $type=="multicast" } {
	    set s [udp_open $port]
	    fconfigure $s -blocking 0 -buffering none -translation binary
	    # pour le multicast (on peut ecouter plusieurs groupes en meme temps....
	    if { $type=="multicast" } {
	    	fconfigure $s -mcastadd $ip -remote [list $ip $port]
	    }
	    # on peut faire du broadcast comme suit:
	    #fconfigure $s -broadcast 1 -romet [list 192.168.2.255 $port]
	    fileevent $s readable [list ::osc::event $s]
	    set ::osc::sock($port) $s
	    return
    }
    if { $type=="tcp" } {
	    socket -server ::osc::tcpServer $port
	    return
    }
    puts "::osc::initIn : bad type $type"
    return
}

proc ::osc::tcpServer {channel clientaddr clientport} {
   puts "::osc::tcp Connection from $clientaddr port $clientport registered"
   fconfigure $channel -translation binary

    for {set i 0} {$i<5} {incr i} {
	puts "waiting for a preamble..."
	set preamble [read $channel 4]
	if { [string length $preamble]==0 } { break }
	binary scan $preamble I1 size
	set data [read $channel $size]
	::osc::oscDecode $data
   }
   puts "::osc::tcp Connection from $clientaddr port $clientport closed"
   close $channel
}

proc ::osc::uninitIn {port} {
	global ::osc::sock
	close $::osc::sock($port)
}

#proc /abcd {a s b} { puts "ABCD!!! num=$a and $b, s is >$s<" }
#proc /efgh {a b} { puts "EFGH!!! a=$a b=$b" }

#set forever 0

#set s1 [Listen 12345]
#set s2 [Listen 23456]
#vwait ::forever
#close $s1



###########################################################
###########################################################
###########################################################


proc ::osc::test1 {} {
	::osc::initIn 7770

	#::osc::oscSendMsg {*}{/hello cifTFdsssS a 1234 3.14 6.281234 abc hello totoblub symbolic}

	#::osc::send local {
	#	{/hello cifTFdsssS a 1234 3.14 6.281234 abc hello totoblub symbolic}
	#	{/toto ss blub abc}
	#	{/yo i 1234}
	#	{/mouse ff 123.4 567.8}
	#	}

	#::osc::uninit local

	set forever 0
	vwait forever
}


proc ::osc::test2 {} {
	::osc::initOut local localhost 7770

	#::osc::send local /hello cifTFdsssS a 1234 3.14 6.281234 abc hello totoblub symbolic

	::osc::send local /hello {ii[i[iiss]i]ii} 1 2 {3 {4 5 abc def} 6} 7 8
	#::osc::send local /hello {ii[iiii]ii} 1 2 {3 4 5 6} 7 8
	#::osc::send local /hello {iiiiiiii} 1 2 3 4 5 6 7 8

	#::osc::send local {
	#	{/hello cifTFdsssS a 1234 3.14 6.281234 abc hello totoblub symbolic}
	#	{/toto ss blub abc}
	#	{/yo i 1234}
	#	{/mouse ff 123.4 567.8}
	#	}

	::osc::uninitOut local
}

proc ::osc::test3 {} {
	#::osc::initIn 10000
	#::osc::initIn 10000 udp
	::osc::initIn 10000 tcp
	#::osc::initIn 10000 multicast 226.0.0.1

	#::osc::initOut local localhost 10000
	#::osc::initOut local localhost 10000 udp
	#::osc::initOut local 226.0.0.1 10000 multicast
	::osc::initOut local localhost 10000 tcp

	::osc::send local /bonjour
	::osc::send local /bonjour/test si abc 1234
	::osc::uninitOut local

	set forever 0
	vwait forever
}


#::osc::test3

