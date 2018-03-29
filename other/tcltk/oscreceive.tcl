#!/usr/bin/tclsh

package require udp 1.0.6


proc oscDecode {s} {
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
			oscDecode $data
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
			default {
				puts "unkown tag!"
			}
		}

	}
	if {[info commands $cmd]!={}} {
		eval $cmd $params
	} else {
		puts "$cmd $params"
	}
}


proc Event {sock} {
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    set me [fconfigure $sock -myport]
    #puts "Received [string length $pkt] from $peer and $me"
	if {[string length $pkt]>0 } {
		oscDecode $pkt
	}
    return
}

proc Listen {port} {
    set s [udp_open $port]
    fconfigure $s -blocking 0 -buffering none -translation binary
    fileevent $s readable [list Event $s]
    return $s
}

#proc /abcd {a s b} { puts "ABCD!!! num=$a and $b, s is >$s<" }
#proc /efgh {a b} { puts "EFGH!!! a=$a b=$b" }

set forever 0

set s1 [Listen 12345]
set s2 [Listen 23456]
vwait ::forever
#close $s1


