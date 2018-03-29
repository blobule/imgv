#!/usr/bin/wish
#!/usr/bin/tclsh

#
# Only display the patterns at a certain speed on the raspberry pi
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

# reception des logs (du raspberry pi)
::osc::initIn 29999 udp
# reception du feedback du processor
::osc::initIn 21333 udp

# display of patterns
set NETdisplay local
#set NETdisplay picam999
set NETcamera  picam999
#::osc::initOut picam999 192.168.6.77 19999 tcp
# processing (for now, local)
set NETprocess local
::osc::initOut local localhost 19999 udp

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
	::osc::send $net /cmdNetlog/out/udp sii giga.local $port 0
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

# we get this when the image has been sent to the processor
proc /imageSent {} {
	global imagenum imagenb NETcamera NETdisplay
	puts "image $imagenum of $imagenb sent."
	incr imagenum
	if { $imagenum < $imagenb } {
		puts "Capturing next image"
		::osc::send $NETdisplay /recycle/next
		after 500 ::osc::send $NETcamera /recycleCam/snapshot
	} else {
		puts "FINI!"
	}
}

#
# configure reading patterns from files for display
# this is optimized for raspberry pi
#
proc configRead {net} {
	global imagenb
	#o::osc::send $net /@/addPlugin ss V pluginViewerGLES2
	::osc::send $net /@/addPlugin ss V pluginViewerGLFW
	::osc::send $net /@/addQueue s display
	::osc::send $net /@/connect sss display V in

	# raspberry pi display
	#::osc::send $net  /display/add-layer s es2-simple
        #::osc::send $net  /display/init-done
	# non raspberry pi
	::osc::send $net /display/
	::osc::send $net /display/new-monitor/main
        ::osc::send $net /display/new-quad/main/A fffffs 0.0 1.0 0.0 1.0 -10.0 "normal"

	::osc::send $net /@/start s V

	# (recycle)-->[Pattern]-->(drip) -> display

	::osc::send $net /@/addPlugin ss R pluginRead
	::osc::send $net /@/addQueue s R.in
	::osc::send $net /@/connect sss R.in R in

	::osc::send $net /@/addEmptyImages si R.in 10

	#::osc::send $net /R.in/view s tex
	::osc::send $net /R.in/view s /main/A/tex
	#::osc::send $net /R.in/file s /home/pi/leopard1/leopard_1280_1024_100_32_B_%03d.png
	::osc::send $net /R.in/file s /home/roys/Videos/leopard1/leopard_1280_1024_100_32_B_%03d.png
	::osc::send $net /R.in/max i $imagenb
	#::osc::send $net /recycle/internal-buffer i 500000
	#::osc::send $net /recycle/pause i 0
	::osc::send $net /R.in/init-done
	::osc::send $net /@/start s R

	## ajoute un drip pour l'affichage a une vitesse constante
	::osc::send $net /@/addPlugin ss D pluginDrip
	::osc::send $net /@/addQueue s D.in
	::osc::send $net /@/connect sss D.in D in
	::osc::send $net /@/connect sss display D out
	::osc::send $net /D.in/fps d 3
	# ask to display the first frame now instead of black
	::osc::send $net /D.in/predisplay-first
	::osc::send $net /D.in/debug T
	# go! starting displayign in 5 sec
	::osc::send $net /D.in/start i 4
	::osc::send $net /@/start s D


	# Read -> NetPat -> Viewer
	#::osc::send $net /@/connect sss inNetPat R out
	#::osc::send $net /@/connect sss display NetPat out
	# Read -> Viewer
	::osc::send $net /@/connect sss D.in R out
	
}


button .a -text Config -command {
	configRead $NETdisplay
 }
#button .b1 -text next -command {::osc::send $NETdisplay /recycle/next}
button .b1 -text scan -command {::osc::send $NETcamera /recycleCam/snapshot}
button .b2 -text stop -command { set imagenb 0 }
button .q -text Quit -command {destroy .}

frame .log
text .log.t -yscrollcommand {.log.s set}
scrollbar .log.s -command {.log.t yview}
.log.t tag configure info -background {light green}
.log.t tag configure warn -background yellow
.log.t tag configure err -background salmon

pack .a .b1 .b2 .q

pack .log -fill both -expand 1
pack .log.t -fill x -expand 1 -side left
pack .log.s -fill y -expand 0 -side left

#.t insert end "bonjour tout le monde\n" info
#.t insert end "bonjour tout le monde\n" warning
#.t insert end "bonjour tout le monde\n"
#.t insert end "bonjour tout le monde\n" error


redirectLog $NETdisplay 29999






