#!/usr/bin/wish
#!/usr/bin/tclsh

#
# to use this:
#
# On silver:
#   netSlave -udp 19999
#
#

#
# Only display the patterns at a certain speed on the local screen
# Start video recording on raspberry pi
# Execute extractPattern on the raspberry pi
# copy the patterns from the raspberry pi to silver
# match the patterns on silver
#

source osc.tcl

# sur giga.local:
#./examples/netSlave -udp 19999
# sur picam999.local:
# ./examples/netSlave

puts "Connecting..."

#
# the connexion is named 'local'. It connects in tcp to localhost, port 19999
#

# reception des logs
::osc::initIn 29999 udp
# reception du feedback du processor
#::osc::initIn 21333 udp

# display of patterns
#set NETdisplay silver
#set NETdisplay picam999
#set NETcamera  picam999
#::osc::initOut picam999 192.168.6.77 19999 tcp
# processing (for now, local)
#set NETprocess local

::osc::initOut netDisplay liquid.local 19999 udp

#::osc::initIn 10000
#::osc::initOut mapper sugo3.local 12345
#::osc::initOut mapper localhost 12345
#::osc::debug 1

#::osc::send mapper /map/allo ii 1 2


proc redirectLog {net port} {
	# -> anything we put in log will go back to tcl controler
	# the queue is "log"
	# so we attach it to a pluginNet to our own log listener...
	::osc::send $net /@/addPlugin ss Netlog pluginNet
	::osc::send $net /@/addQueue s cmdNetlog
	::osc::send $net /@/connect sss cmdNetlog Netlog cmd
	# setup redirect to our favorite log port...
	::osc::send $net /cmdNetlog/out/udp sii liquid.local $port 0
	# hijack the log queue...
	::osc::send $net /@/connect sss log Netlog in
	::osc::send $net /@/start s Netlog
	# redirect the input of log plugin to unused queue
	# syslog will reconnect at the next message it receives
	::osc::send $net /@/addQueue s bidon
	::osc::send $net /@/connect sss bidon Syslog in
}

# pad msg with space to length len
proc makeLength {msg len} {
	return "$msg[string repeat " " [expr $len-[string length $msg]]]"
}

#
# from the log redirect...
#
proc addmsg {tag plugin msg} {
	.log.t insert end [makeLength $plugin 20] $tag
	.log.t insert end "$msg\n"
	.log.t see end
}
proc /info {plugin msg} { addmsg info $plugin $msg }
proc /warn {plugin msg} { addmsg warn $plugin $msg }
proc /err {plugin msg} { addmsg err $plugin $msg }

# compteur d'images, et nb d'images a envoyer
set imagenum 0
set imagenb  50
#set patternFiles "/home/faezeh/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png"
set patternFiles "/home/vision/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png"
set framesPerSecond 3

#
# configure reading patterns from files for display
#
proc configRead {net} {
	global imagenb patternFiles framesPerSecond
	#o::osc::send $net /@/addPlugin ss V pluginViewerGLES2
	::osc::send $net /@/addPlugin ss V pluginViewerGLFW
	::osc::send $net /@/addQueue s display
	::osc::send $net /@/connect sss display V in

	# raspberry pi display
	#::osc::send $net  /display/add-layer s es2-simple
        #::osc::send $net  /display/init-done
	# non raspberry pi
	::osc::send $net /display/
	# use xrandr to get list of monitors.
	# On silver, we get: DVI-I-2 (apple) et DVI-I-3 (samsung)
	::osc::send $net /display/new-monitor/main s "DVI-I-1"
        ::osc::send $net /display/new-quad/main/A fffffs 0.0 1.0 0.0 1.0 -10.0 "normal"

	::osc::send $net /@/start s V

	# (recycle)-->[Pattern]-->(drip) -> display

	::osc::send $net /@/addPlugin ss R pluginRead
	::osc::send $net /@/addQueue s R.in
	::osc::send $net /@/connect sss R.in R in
	::osc::send $net /@/addEmptyImages si R.in 10
	#::osc::send $net /R.in/view s tex
	::osc::send $net /R.in/view s /main/A/tex
	::osc::send $net /R.in/file s $patternFiles
	::osc::send $net /R.in/max i $imagenb
	#::osc::send $net /recycle/pause i 0
	::osc::send $net /R.in/init-done
	::osc::send $net /@/start s R

	## ajoute un drip pour l'affichage a une vitesse constante
	::osc::send $net /@/addPlugin ss D pluginDrip
	::osc::send $net /@/addQueue s D.in
	::osc::send $net /@/connect sss D.in D in
	::osc::send $net /@/connect sss display D out
	::osc::send $net /D.in/fps d $framesPerSecond
	# ask to display the first frame now instead of black
	::osc::send $net /D.in/predisplay-first
	::osc::send $net /D.in/debug T
	# go! starting displayign in 4 sec
	::osc::send $net /D.in/start i 6
	::osc::send $net /@/start s D


	# Read -> NetPat -> Viewer
	#::osc::send $net /@/connect sss inNetPat R out
	#::osc::send $net /@/connect sss display NetPat out
	# Read -> Viewer
	::osc::send $net /@/connect sss D.in R out
	
}

# create a directory for storing video/patterns for a damier/pi
proc dirName {damier pi} {
	return "scan_${damier}_${pi}"
}

###################

proc piRecord {} {
	set pis [getCurrentPis]
	set ids {}
	foreach pi $pis {
		puts "recording from $pi"
		lappend ids [exec ssh -q $pi raspivid -t 22000 -n -o scan.mp4 &]
	}
	waitForProcesses $ids
}

proc getVideoFromPi {damier} {
	set pis [getCurrentPis]
	set ids {}
	foreach pi $pis {
		set d [dirName $damier $pi]
		puts "getting scan.mp4 from $pi to $d/scan.mp4"
		#file delete -force $d
		file mkdir $d
		lappend ids [exec scp -q $pi:scan.mp4 $d/scan.mp4 &]
	}
	waitForProcesses $ids
}

proc extractPatternsFromVideo {damier} {
	set pis [getCurrentPis]
	foreach pi $pis {
		puts "*** extracting $damier $pi ***"
		set current [pwd]
		set d [dirName $damier $pi]
		cd $d
		catch {exec extractPattern -video scan.mp4 -n 50 -start 6}
		cd $current
	}
}

proc untest {} {
	::osc::send netDisplay /D.in/pausenow T
	::osc::send netDisplay /R.in/init-done
	::osc::send netDisplay /D.in/pausenow F
}

################

#
# pour verifier si un process existe encore
#
proc processExist {id} {
	set res [catch {exec ps -fp $id | grep $id}]
	return $res
}
proc waitForProcesses {ids} {
	puts "waiting for processes $ids"
	set sum 0
	while {$sum != [llength $ids]} {
		set sum 0
		foreach id $ids {
			incr sum [processExist $id]
		}
		puts "$sum/[llength $ids] process done"
		after 1000
	}
}

#################################################
################################################
###############################################

frame .current

## choix du damier
entry .current.en -textvariable currentdamier
set currentdamier damier0

## choix du pi
#menubutton .current.m -menu .current.m.pis -textvariable currentpi
#menu .current.m.pis -tearoff 0
#foreach p {picam1 picam2 picam3} {
#	.current.m.pis add command -label $p -command "set currentpi $p"
#}
#set currentpi picam1
listbox .current.m -selectmode multiple
foreach p {picam1 picam2 picam3} {
	.current.m insert end $p
}

proc getCurrentPis {} {
	set sel [.current.m curselection]
	set r {}
	foreach s $sel {
		lappend r [.current.m get $s]
	}
	return $r
}



button .a1 -text {Show Patterns} -command { configRead netDisplay }
button .a2 -text {Pi Record} -command { piRecord }
button .a3 -text {Get video from Pi} -command {getVideoFromPi $currentdamier}
button .a4 -text {Extract patterns from video} -command {extractPatternsFromVideo $currentdamier}
button .a5 -text {test} -command untest
#button .b1 -text next -command {::osc::send $NETdisplay /recycle/next}
button .b1 -text scan -command {::osc::send $NETcamera /recycleCam/snapshot}
button .q -text Quit -command {destroy .}

frame .log
text .log.t -yscrollcommand {.log.s set}
scrollbar .log.s -command {.log.t yview}
.log.t tag configure info -background {light green}
.log.t tag configure warn -background yellow
.log.t tag configure err -background salmon

pack .a1 .a2 .a3 .a4 .current .a5 .q -fill x -expand 1
pack .current.en .current.m -side left -fill x -expand 1

pack .log -fill both -expand 1
pack .log.t -fill x -expand 1 -side left
pack .log.s -fill y -expand 0 -side left

#.t insert end "bonjour tout le monde\n" info
#.t insert end "bonjour tout le monde\n" warning
#.t insert end "bonjour tout le monde\n"
#.t insert end "bonjour tout le monde\n" error


redirectLog netDisplay 29999






