#!/usr/bin/wish

source osc.tcl

# feedback
#::osc::initIn 10000 multicast 226.0.0.1
#::osc::initIn 10000 tcp
#::osc::initIn 10000 udp

#::osc::initOut pi 226.0.0.1 10000 multicast

#::osc::initOut pi localhost 10000 tcp
::osc::initOut pi1 bleuet1.local 4444 tcp
#::osc::initOut pi2 bleuet2.local 4444 tcp
#::osc::initOut pi2 192.168.2.118 10000 tcp

button .config -text config -command {config}
button .configNoDisplayNet -text "Config (no display - Net)"  -command {configNoDisplayNet}
button .configNoDisplaySave -text "Config (no display - Save)"  -command {
	set when [expr [clock seconds]+10]
	configNoDisplaySave pi1 $when
	#configNoDisplaySave pi2 $when
}
button .configNoDisplaySaveFinish -text "Finish (no display - Save)"  -command {
	configNoDisplaySaveFinish pi1
	#configNoDisplaySaveFinish pi2
}
button .shutter -text "set shutter 20000" -command {
	::osc::send pi1 /recycle/setShutterSpeed i 30000
}
checkbutton .mode -text Automatic -variable automatic -command { osc::send pi /recycle/automatic [expr $automatic?"T":"F"]}
button .snap -text Snap! -command {osc::send pi /recycle/snapshot}

ttk::scale .s1 -orient horizontal -command {::osc::send pi /pos f } -from 0 -to 100
button .q -text Exit -command { destroy . }



pack .config .configNoDisplayNet .shutter .configNoDisplaySave .configNoDisplaySaveFinish .mode .snap .s1 .q -padx 10 -pady 10


proc config {} {
	::osc::send pi /@/addPlugin ss V pluginViewerGLES2
	::osc::send pi /@/addPlugin ss C pluginRaspicam
	::osc::send pi /@/addPlugin ss D pluginDrip

	::osc::send pi /@/addQueue s recycle
	::osc::send pi /@/addQueue s display
	::osc::send pi /@/addQueue s recycle-drip
	::osc::send pi /@/addQueue s net
	::osc::send pi /@/addQueue s netcmd

	::osc::send pi /@/addEmptyImages si recycle 100
	::osc::send pi /@/addEmptyImages si recycle-drip 3

	::osc::send pi /@/connect sss recycle C in
	::osc::send pi /@/connect sss net C out
	::osc::send pi /@/connect sss display V in

	::osc::send pi /@/connect sss recycle-drip D in
	::osc::send pi /@/connect sss recycle-drip D out
	::osc::send pi /@/reservePort ss sync D
	::osc::send pi /@/connect sss recycle D sync

	::osc::send pi  /display/add-layer s es2-simple
	::osc::send pi  /display/init-done

	::osc::send pi  /recycle/view s tex
	::osc::send pi  /recycle/setWidth i [expr 1280/4]
	::osc::send pi  /recycle/setHeight i [expr 960/4]
	::osc::send pi  /recycle/setFormat i 3
	::osc::send pi  /recycle/setExposure i 1
	::osc::send pi  /recycle/setMetering i 1
	::osc::send pi  /recycle/automatic F
	::osc::send pi  /recycle/init-done

	#::osc::send pi  /recycle-drip/defevent-ctrl ssss frame log /info /snapshot
	::osc::send pi  /recycle-drip/defevent-ctrl sss frame sync /snapshot
	::osc::send pi  /recycle-drip/fps d   10.0
	::osc::send pi  /recycle-drip/count i 100
	::osc::send pi  /recycle-drip/start i [expr [clock seconds]+10]

	# send images to network
	::osc::send pi /@/addPlugin ss N pluginNet
	## delay=0
	::osc::send pi /netcmd/out/udp sii giga.local 10000 0
	::osc::send pi /netcmd/real-time T

	::osc::send pi /@/connect sss display N out
	::osc::send pi /@/connect sss net N in
	::osc::send pi /@/connect sss netcmd N cmd

	::osc::send pi  /@/start s V
	::osc::send pi  /@/start s C
	::osc::send pi  /@/start s D
	::osc::send pi  /@/start s N

#
# drip : on va connecter un recycle-drip en in et en ou... l'image va tourner en rond...
# /fps 10.0(double)
# /count 20
# /start starttime(int32) -> fin de l'initialisation
# reserver un port sync -> la queue de la camera
# 
# /defevent-ctrl << "frame" << "sync" << "/snapshot"
#

}

proc configNoDisplayNet {} {
	# plugins
	::osc::send pi /@/addPlugin ss C pluginRaspicam
	::osc::send pi /@/addPlugin ss D pluginDrip
	::osc::send pi /@/addPlugin ss N pluginNet

	# queues
	::osc::send pi /@/addQueue s recycle
	::osc::send pi /@/addQueue s recycle-drip
	::osc::send pi /@/addQueue s net
	::osc::send pi /@/addQueue s netcmd

	::osc::send pi /@/addEmptyImages si recycle 10
	::osc::send pi /@/addEmptyImages si recycle-drip 3

	# connect
	::osc::send pi /@/connect sss recycle C in
	::osc::send pi /@/connect sss net C out

	::osc::send pi /@/connect sss net N in
	::osc::send pi /@/connect sss netcmd N cmd

	::osc::send pi /@/connect sss recycle-drip D in
	::osc::send pi /@/connect sss recycle-drip D out
	::osc::send pi /@/reservePort ss sync D
	::osc::send pi /@/connect sss recycle D sync

	# configure C
	::osc::send pi  /recycle/view s tex
	::osc::send pi  /recycle/setWidth i [expr 1280/4]
	::osc::send pi  /recycle/setHeight i [expr 960/4]
	::osc::send pi  /recycle/setFormat i 3
	//::osc::send pi  /recycle/setExposure i 1
	::osc::send pi  /recycle/setMetering i 1
	::osc::send pi  /recycle/automatic F
	::osc::send pi  /recycle/init-done

	# configure D
	#::osc::send pi  /recycle-drip/defevent-ctrl ssss frame log /info /snapshot
	::osc::send pi  /recycle-drip/defevent-ctrl sss frame sync /snapshot
	::osc::send pi  /recycle-drip/fps d   1.0
	::osc::send pi  /recycle-drip/count i -1
	::osc::send pi  /recycle-drip/start i 2

	# configure N
	# send images to network, port 10000
	::osc::send pi /netcmd/out/udp sii giga.local 10000 0  # delay is 0
	::osc::send pi /netcmd/real-time F # delay does not work for now

	#::osc::send pi  /@/start s V
	::osc::send pi  /@/start s C
	::osc::send pi  /@/start s D
	::osc::send pi  /@/start s N

}


proc configNoDisplaySave {out when} {
	set nb 100
	# plugins
	::osc::send $out /@/addPlugin ss C pluginRaspicam
	::osc::send $out /@/addPlugin ss D pluginDrip
	::osc::send $out /@/addPlugin ss S pluginSave

	# queues
	::osc::send $out /@/addQueue s recycle
	::osc::send $out /@/addQueue s recycle-drip
	::osc::send $out /@/addQueue s save

	::osc::send $out /@/addEmptyImages si recycle $nb
	::osc::send $out /@/addEmptyImages si recycle-drip 3

	# connect
	::osc::send $out /@/connect sss recycle C in
	::osc::send $out /@/connect sss save C out

	::osc::send $out /@/connect sss save S in

	::osc::send $out /@/connect sss recycle-drip D in
	::osc::send $out /@/connect sss recycle-drip D out
	::osc::send $out /@/reservePort ss sync D
	::osc::send $out /@/connect sss recycle D sync

	# configure C
	::osc::send $out  /recycle/view s tex
#	::osc::send $out  /recycle/setWidth i [expr 1280/2]
#	::osc::send $out  /recycle/setHeight i [expr 960/2]
	# format: 0=YUV420, 1=GRAY, 2=BGR, 3=RGB
#	::osc::send $out  /recycle/setFormat i 2
	#::osc::send $out  /recycle/setExposure i 1
	#::osc::send pi1 /recycle/setShutterSpeed i 30000
#	::osc::send $out  /recycle/setMetering i 1
	::osc::send $out  /recycle/disable
	::osc::send $out  /recycle/init-done

	# configure D
	#::osc::send pi  /recycle-drip/defevent-ctrl ssss frame log /info /snapshot
	::osc::send $out  /recycle-drip/defevent-ctrl sss frame sync /enable
	::osc::send $out  /recycle-drip/fps d 1.0
	::osc::send $out  /recycle-drip/count i 1
	::osc::send $out  /recycle-drip/start i $when

	# configure S
	# we should attach a message when saving is done... this would be sent out so we know its done.
	::osc::send $out  /save/mode s internal
	::osc::send $out  /save/filename s /home/pi/Images/out%03d.png
	::osc::send $out  /save/count i $nb
	::osc::send $out  /save/init-done
	
	# start
	::osc::send $out  /@/start s C
	::osc::send $out  /@/start s D
	# do not start S, so there is nothing to interfere with the capture
	#::osc::send $out  /@/start s S

}

proc configNoDisplaySaveFinish {out} {
	::osc::send $out  /@/start s S
}


#first


#first
#second
#pi
#puts [ttk::style theme names]
#puts [ttk::style theme use]
#wm withdraw .




