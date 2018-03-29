#!/usr/bin/wish
#!/usr/bin/tclsh

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
set NETdisplay picam999
set NETcamera  picam999
::osc::initOut picam999 picam999.local 19999 tcp
# processing (for now, local)
set NETprocess local
::osc::initOut local localhost 19999 udp

#::osc::initIn 10000
#::osc::initOut mapper sugo3.local 12345
#::osc::initOut mapper localhost 12345
#::osc::debug 1

#::osc::send mapper /map/allo ii 1 2


#
# configure la machine $net pour afficher des images
# recues du reseau...
# une image recue en tcp port 20000 sera a gauche
# une image recue en tcp port 20001 sera a droite
#
proc configProcess {net} {
	global imagenb
	# pluginNet (tcp/20000) -> display
	# pluginNet (tcp/20001) -> display
	::osc::send $net /@/addPlugin ss V pluginViewerGLFW
	::osc::send $net /@/addQueue s display
	::osc::send $net /@/connect sss display V in

	::osc::send $net /display/new-window/main iiiiTs 100 100 1920 480 "ABx"
        ::osc::send $net /display/new-quad/main/A fffffs 0.0 0.333 0.0 1.0 -10.0 "normal"
        ::osc::send $net /display/new-quad/main/B fffffs 0.333 0.666 0.0 1.0 -10.0 "normal"
        ::osc::send $net /display/new-quad/main/C fffffs 0.666 1.0 0.0 1.0 -10.0 "normal"

	# network feedback to main controller
	# assume giga.local port 21333
	::osc::send $net /@/addPlugin ss NetFeedback pluginNet
	::osc::send $net /@/addQueue s NetFeedbackin
	::osc::send $net /@/addQueue s NetFeedbackcmd
	::osc::send $net /@/connect sss NetFeedbackin NetFeedback in
	::osc::send $net /@/connect sss NetFeedbackcmd NetFeedback cmd
	::osc::send $net /NetFeedbackcmd/out/udp sii giga.local 21333 0
	::osc::send $net /@/start s NetFeedback
	

	# receive from network
	::osc::send $net /@/addPlugin ss NetA pluginNet
	::osc::send $net /@/addQueue s cmdNetA
	::osc::send $net /@/addQueue s recycleNetA
	::osc::send $net /@/addEmptyImages si recycleNetA 10
	::osc::send $net /@/connect sss cmdNetA NetA cmd
	::osc::send $net /@/connect sss recycleNetA NetA in
	::osc::send $net /cmdNetA/debug F
	::osc::send $net /cmdNetA/view s /main/A/tex
	# UDP ou TCP?
	::osc::send $net /cmdNetA/in/tcp i 20000
	#::osc::send $net /cmdNetA/in/udp i 20000
	::osc::send $net /@/start s NetA

	#::osc::send $net /@/addPlugin ss NetB pluginNet
	#::osc::send $net /@/addQueue s cmdNetB
	#::osc::send $net /@/addQueue s recycleNetB
	#::osc::send $net /@/addEmptyImages si recycleNetB 10
	#::osc::send $net /@/connect sss cmdNetB NetB cmd
	#::osc::send $net /@/connect sss recycleNetB NetB in
	#::osc::send $net /cmdNetB/debug F
	#::osc::send $net /cmdNetB/view s /main/B/tex
	#::osc::send $net /cmdNetB/in/udp i 20001
	#::osc::send $net /@/start s NetB

	::osc::send $net /@/start s V

	# reading images from local files instead of network...
	::osc::send $net /@/addPlugin ss R pluginRead
	::osc::send $net /@/addQueue s recycleRead
	::osc::send $net /@/addEmptyImages si recycleRead 3
	::osc::send $net /recycleRead/file s /home/roys/svn3d/imgv/build/leopard/leopard_1280_800_100_32_B_%03d.png
	::osc::send $net /recycleRead/max i 90
	#::osc::send $net /recycle/internal-buffer i 500000
	::osc::send $net /recycleRead/view s /main/B/tex
	::osc::send $net /recycleRead/init-done
	::osc::send $net /@/connect sss recycleRead R in
	::osc::send $net /@/start s R
	

	# processing leopard
	::osc::send $net /@/addPlugin ss L pluginLeopard
	::osc::send $net /@/addQueue s inCam
	::osc::send $net /@/addQueue s inProj
	::osc::send $net /@/connect sss inCam L incam
	::osc::send $net /@/connect sss inProj L inproj
	::osc::send $net /@/addQueue s recycleLeopard
	::osc::send $net /@/addEmptyImages si recycleLeopard 3

	::osc::send $net /@/connect sss display L out
	::osc::send $net /@/connect sss display L thrash
	::osc::send $net /@/connect sss recycleLeopard L recycle

	#::osc::send $net /@/connect sss display NetA out
	#::osc::send $net /@/connect sss display NetB out
	::osc::send $net /@/connect sss inCam NetA out

	# send feedback to master control about the image reception
	::osc::send $net /@/reservePort ss toNetFeedback NetA
	::osc::send $net /@/connect sss NetFeedbackin NetA toNetFeedback
	::osc::send $net /cmdNetA/defevent-ctrl sss "received" toNetFeedback /imageSent

	# si on veut recevoir de l'exterieur...
	#::osc::send $net /@/connect sss inProj NetB out
	# si on lit les patterns localement
	::osc::send $net /@/connect sss inProj R out

	::osc::send $net /inCam/set/view s /main/C/tex
	# start the matcher... should be done with /init-done
	::osc::send $net /inCam/set/count i $imagenb

	::osc::send $net /@/start s L
	
}


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
proc configRead {net processMachine processPort} {
	::osc::send $net /@/addPlugin ss V pluginViewerGLES2
	::osc::send $net /@/addQueue s display
	::osc::send $net /@/connect sss display V in

	# raspberry pi display
	::osc::send $net  /display/add-layer s es2-simple
        ::osc::send $net  /display/init-done
	::osc::send $net /@/start s V

	# (recycle)-->[Pattern]-->(display)

	::osc::send $net /@/addPlugin ss R pluginRead
	::osc::send $net /@/addQueue s recycle
	::osc::send $net /@/connect sss recycle R in

	::osc::send $net /@/addEmptyImages si recycle 5

	::osc::send $net /recycle/view s tex
	::osc::send $net /recycle/file s /home/pi/leopard/leopard_1280_800_100_32_B_%03d.png
	::osc::send $net /recycle/max i 89
	#::osc::send $net /recycle/internal-buffer i 500000
	::osc::send $net /recycle/pause i 0
	::osc::send $net /recycle/init-done

	::osc::send $net /@/start s R


	# Read -> NetPat -> Viewer
	#::osc::send $net /@/connect sss inNetPat R out
	#::osc::send $net /@/connect sss display NetPat out
	# Read -> Viewer
	::osc::send $net /@/connect sss display R out
	
}

#
# camera of a raspberry pi
#
proc configCamera {net processMachine processPort} {
	::osc::send $net /@/addPlugin ss C pluginRaspicam

	::osc::send $net /@/addQueue s recycleCam
	::osc::send $net /@/addEmptyImages si recycleCam 3

	::osc::send $net /@/connect sss recycleCam C in

	# setup network connexion to image processor
	::osc::send $net /@/addPlugin ss NetCam pluginNet
	::osc::send $net /@/addQueue s cmdNetCam
	::osc::send $net /@/addQueue s inNetCam
	::osc::send $net /@/connect sss cmdNetCam NetCam cmd
	::osc::send $net /@/connect sss inNetCam NetCam in
	::osc::send $net /cmdNetCam/debug F
	# setup redirect to our favorite log port...
	#::osc::send $net /cmdNetCam/defevent-ctrl sss "image-sending" log /imageSending
	#::osc::send $net /cmdNetCam/defevent-ctrl sss "sent" log /imageSent

	# UDP ou TCP?
	#::osc::send $net /cmdNetCam/out/udp sii $processMachine $processPort 1
	::osc::send $net /cmdNetCam/out/tcp si $processMachine $processPort

	::osc::send $net /@/start s NetCam

	#::osc::send $net /@/connect sss display C out
	::osc::send $net /@/connect sss inNetCam C out
	# affiche l'image apres l'avoir envoye...
	#::osc::send $net /@/connect sss display NetCam out

	::osc::send $net /recycleCam/view s tex
        ::osc::send $net /recycleCam/setWidth i [expr 1280/1]
        ::osc::send $net /recycleCam/setHeight i [expr 960/1]
	# format 3=bgr, 1=gray
	# gray format crash... todo check
        ::osc::send $net /recycleCam/setFormat i 1
        ::osc::send $net /recycleCam/setExposure i 1
        ::osc::send $net /recycleCam/setMetering i 1
	# disable to put in pause mode... use snapshot to get one image
        ::osc::send $net /recycleCam/disable
        ::osc::send $net /recycleCam/init-done

	::osc::send $net /@/start s C
}

button .a -text Config -command {
        configProcess $NETprocess
	configRead $NETdisplay giga.local 20001
	configCamera $NETcamera giga.local 20000
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






