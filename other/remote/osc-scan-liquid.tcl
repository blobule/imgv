#!/usr/bin/wish8.6

#
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

#
# raspivid parameters
#
set raspiXres 1920
set raspiYres 1080
set raspiFps 30
# to get larger FOV
set raspiXres 1296
set raspiYres 972


::osc::initOut netDisplay liquid.local 19999 udp

#::osc::initIn 10000
#::osc::initOut mapper sugo3.local 12345
#::osc::initOut mapper localhost 12345
#::osc::debug 1

#::osc::send mapper /map/allo ii 1 2

#
# start and stop the netSlave
#
set slavepid 0
proc startSlave {} {
	global slavepid
	stopSlave
	set slavepid [exec netSlave -udp 19999 &]
	puts "process id is $slavepid"
	after 500 redirectLog netDisplay liquid.local 29999
}
proc stopSlave {} {
	global slavepid
	if {$slavepid==0} return
	puts "killing process id $slavepid"
	set success [catch {exec kill $slavepid}]
	puts "success is $success"
	set slavepid 0
}

#
# verify the time difference of this machine and the pi's
# use q for query, and b to reset
#
#
proc checkTime {command} {
	set pis [getCurrentPis]
	set r ""
	puts "checking time for this machine..."
	catch {set r [exec sudo /usr/sbin/ntpdate -${command}u vaioz.local]}
	puts "this machine: $r"
	foreach p $pis {
		set r ""
		puts "checking time for $p ..."
		catch {set r [exec ssh -q $p sudo /usr/sbin/ntpdate -${command}u vaioz.local]}
		puts "$p: $r"
	}
}


proc redirectLog {net to port} {
	# -> anything we put in log will go back to tcl controler
	# the queue is "log"
	# so we attach it to a pluginNet to our own log listener...
	::osc::send $net /@/addPlugin ss Netlog pluginNet
	::osc::send $net /@/addQueue s cmdNetlog
	::osc::send $net /@/connect sss cmdNetlog Netlog cmd
	# setup redirect to our favorite log port...
	::osc::send $net /cmdNetlog/out/udp sii $to $port 0
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
#set imagenum 0
set imagenb  100
set patternFiles20 "/home/vision/Videos/leopard20/leopard_2560_1080_32B_%03d.png"
set patternName20 "leopard20"
set patternFiles21 "/home/vision/Videos/leopard21/leopard_2560_1080_32B_%03d.png"
set patternName21 "leopard21"
set framesPerSecond 4

#
# where to put the files?
#
# SCAN/damier0_picam1/... (le scan avec damier0 vu par picam1)
#
# MATCH/picam1/... match-damier0-leopard10.xm (tous les matchs pour les damiers/patterns pour picam1)
#
# Quand on fait un scan, tout va dans SCAN/damier0_picam1
# et le resultat du match va dans MATCH/picam1
#
set scanDirectory $env(HOME)/SCAN
set matchDirectory $env(HOME)/MATCH
file mkdir $scanDirectory
file mkdir $matchDirectory

#
# configure reading patterns from files for display
#
proc configReadBase {net} {
	global framesPerSecond
	# display
	::osc::send $net /@/addPlugin ss V pluginViewerGLFW
	::osc::send $net /@/addQueue s display
	::osc::send $net /@/connect sss display V in
	::osc::send $net /display/manual T
	::osc::send $net /@/start s V

	# le drip
	::osc::send $net /@/addPlugin ss MD pluginDripMulti
	::osc::send $net /@/addQueue s MD.in
	::osc::send $net /@/connect sss MD.in MD in
	::osc::send $net /MD.in/fps d $framesPerSecond
	::osc::send $net /@/start s MD
}
proc configReadGo {net} {
	::osc::send $net /display/manual F
	::osc::send $net /MD.in/start i 5
}

# base: A, B, C, etc... un par ecran
proc configRead {net base ecran patternFiles offx offy} {
	global imagenb
	#o::osc::send $net /@/addPlugin ss V pluginViewerGLES2

	# raspberry pi display
	#::osc::send $net  /display/add-layer s es2-simple
        #::osc::send $net  /display/init-done
	# non raspberry pi
	#::osc::send $net /display/
	# use xrandr to get list of monitors.
	# On silver, we get: DVI-I-2 (apple) et DVI-I-3 (samsung)

	::osc::send $net /display/new-monitor/$base s $ecran
	#::osc::send $net /display/new-window/$base iiiiFs $offx $offy 1920 1080 $base
        ::osc::send $net /display/new-quad/$base/A fffffs 0.0 1.0 0.0 1.0 -10.0 "normal"

	# (recycle)-->[Pattern]-->(drip) -> display

	::osc::send $net /@/addPlugin ss R$base pluginRead
	::osc::send $net /@/addQueue s R$base.in
	::osc::send $net /@/connect sss R$base.in R$base in
	::osc::send $net /@/addEmptyImages si R$base.in 10
	::osc::send $net /R$base.in/view s /$base/A/tex
	::osc::send $net /R$base.in/file s $patternFiles
	::osc::send $net /R$base.in/max i $imagenb
	::osc::send $net /R$base.in/init-done
	::osc::send $net /@/reservePort ss videoout MD
	::osc::send $net /@/connect sss display MD videoout
	::osc::send $net /@/start s R$base

	## super plugin multi-drip
	::osc::send $net /@/addQueue s MD.videoin$base
	::osc::send $net /@/reservePort ss videoin$base MD
	::osc::send $net /@/connect sss MD.videoin$base MD videoin$base
	::osc::send $net /MD.in/in ss videoin$base video$base
	::osc::send $net /MD.in/out sss video$base videoout /$base/A/tex

	# Read -> NetPat -> Viewer
	#::osc::send $net /@/connect sss inNetPat R out
	#::osc::send $net /@/connect sss display NetPat out
	# Read -> Viewer
	#::osc::send $net /@/connect sss D.in R out
	::osc::send $net /@/connect sss MD.videoin$base R$base out

	# send first image right now
	::osc::send $net /@/loadImage sss [exec printf $patternFiles 0] display /$base/A/tex
	::osc::send $net /display/render
	::osc::send $net /display/render
}


# create a directory for storing video/patterns for a damier/pi
proc dirName {damier pi} {
	global scanDirectory
	return "$scanDirectory/${damier}_${pi}"
}

###################

proc piRecord {{short 0}} {
	global raspiXres raspiYres raspiFps
	set duration 35000
	if { $short } {
		set duration 2000
	}
	set pis [getCurrentPis]
	set ids {}
	foreach pi $pis {
		puts "recording from $pi"
		lappend ids [exec ssh -q $pi raspivid -t $duration -ss 10000 -awb off -awbg 1.5,1.2  -w $raspiXres -h $raspiYres -fps $raspiFps -b 2000000 -o scan.mp4 &]
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
		catch {exec extractPattern -video scan.mp4 -n 100 -start 6}
		cd $current
	}
}

proc extractSingleImageFromVideo {damier} {
	set pis [getCurrentPis]
	foreach pi $pis {
		puts "*** extracting $damier $pi ***"
		set current [pwd]
		set d [dirName $damier $pi]
		cd $d
		catch {exec extractPattern -video scan.mp4 -n 1 -start 30}
		cd $current
	}
}


proc matchLeopard {damier} {
	global imagenb patternFiles20 patternFiles21 patternName20 patternName21 matchDirectory
	set pis [getCurrentPis]
	foreach pi $pis {
		foreach {patfiles patname} [list $patternFiles20 $patternName20 $patternFiles21 $patternName21] {
			puts "*** matching leopard $damier $pi with $patfiles ($patname) ***"
			set current [pwd]
			set d [dirName $damier $pi]
			cd $d
			catch {exec playLeopard -cam cam%03d.png -proj $patfiles -number $imagenb -iter 20 -iterstep 5 -display -screen 2560 1080 38.03}
			file rename lutCam20.png lutCam20-${damier}-${patname}.png
			file rename lutProj20.png lutProj20-${damier}-${patname}.png
			set name match-${damier}-${patname}.xml
			file mkdir $matchDirectory/$pi
			set matchname $matchDirectory/$pi/$name
			file rename match.xml $name
			file copy $name $matchname
			cd $current
		}
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
foreach p {picam1 picam2 picam3 picam4 picam5 picam6 picam7 picam8 picam9 picam10 picam11 picam12 picam13 picam14 picam15 picam16} {
	.current.m insert end $p
}
# picam1 is selcted by default
.current.m selection set 0

proc getCurrentPis {} {
	set sel [.current.m curselection]
	set r {}
	foreach s $sel {
		lappend r [.current.m get $s]
	}
	return $r
}

#
# affiche tout!
#
proc showPatterns {} {
	global patternFiles20 patternFiles21
	configReadBase netDisplay
	#configRead netDisplay left HDMI-0 $patternFiles20 0 0
	configRead netDisplay left DVI-D-0 $patternFiles20 2560 0
	configRead netDisplay right HDMI-0 $patternFiles21 0 0
	configReadGo netDisplay
}

#
# call showPattern, piRecord, ....
#
proc doAll {} {
	global currentdamier
	# show patterns
	showPatterns
	# pi record
	after 2000
	piRecord
	getVideoFromPi $currentdamier
	extractPatternsFromVideo $currentdamier
	matchLeopard $currentdamier
}

#
# call showPattern, piRecord, ....
# this is to get a video panorama!
# for now we extract a single image from the video
#
proc doAllPanorama {} {
	global currentdamier
	# show patterns
	#showPatterns
	# pi record
	after 2000
	# we want short recording!
	piRecord 1
	getVideoFromPi $currentdamier
	extractSingleImageFromVideo $currentdamier
	#matchLeopard $currentdamier
}


button .a0 -text {Re-start Slave} -command { startSlave }

frame .fchecktime
button .fchecktime.check -text {Check time} -command { checkTime q}
button .fchecktime.reset -text {Reset time} -command { checkTime b}

button .a1 -text {Show Patterns} -command { showPatterns }
button .a2 -text {Pi Record} -command { piRecord }
button .a3 -text {Get video from Pi} -command {getVideoFromPi $currentdamier}
button .a4 -text {Extract patterns from video} -command {extractPatternsFromVideo $currentdamier}
button .a5 -text {Match Leopard} -command {matchLeopard $currentdamier}
button .a998 -text {Do all} -bg yellow -command doAll
button .a999 -text {Do all Panorama} -bg yellow -command doAllPanorama
#button .b1 -text next -command {::osc::send $NETdisplay /recycle/next}
button .b1 -text scan -command {::osc::send $NETcamera /recycleCam/snapshot}
button .q -text Quit -command {stopSlave;destroy .}


frame .log
text .log.t -yscrollcommand {.log.s set}
scrollbar .log.s -command {.log.t yview}
.log.t tag configure info -background {light green}
.log.t tag configure warn -background yellow
.log.t tag configure err -background salmon

pack .a0 .fchecktime .a1 .a2 .a3 .a4 .a5 .current .a998 .a999 .q -fill x -expand 1
pack .current.en .current.m -side left -fill x -expand 1

pack .fchecktime.check .fchecktime.reset -side left -fill x -expand 1

pack .log -fill both -expand 1
pack .log.t -fill x -expand 1 -side left
pack .log.s -fill y -expand 0 -side left

#.t insert end "bonjour tout le monde\n" info
#.t insert end "bonjour tout le monde\n" warning
#.t insert end "bonjour tout le monde\n"
#.t insert end "bonjour tout le monde\n" error



# demarre le netslave
puts "starting slave!"
startSlave

puts "GO!"




