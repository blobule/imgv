
set(IMGV_SRC blob.cpp 
        recyclable.cpp
        oscutils.cpp
        qpmanager.cpp
	blob.cpp
	plugin.hpp
	message.hpp
   )

##
## selection of plugins
##

# plugin V4L2
#
if(V4L2_FOUND)
  set(HAVE_PLUGIN_V4L2 "YES" PARENT_SCOPE)
  set(IMGV_SRC ${IMGV_SRC} pluginV4L2.cpp)
  install(FILES pluginV4L2.hpp DESTINATION ${IMGV_INCLUDE})
endif()

# plugin Leopard (simple matcher de pattern leopard)
#
set(HAVE_PLUGIN_LEOPARD "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginLeopard.cpp)
install(FILES pluginLeopard.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Stream (transfer images par reseau)
#
set(HAVE_PLUGIN_STREAM "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginStream.cpp tcpcast.c tcpcast.h udpcast.c udpcast.h )
install(FILES pluginStream.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Net (transfer message/images par reseau)
#
set(HAVE_PLUGIN_NET "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginNet.cpp tcpcast.c tcpcast.h udpcast.c udpcast.h )
install(FILES pluginNet.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Syslog (affichage des messages des plugins)
#
set(HAVE_PLUGIN_SYSLOG "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginSyslog.cpp)
install(FILES pluginSyslog.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Webcam
#
set(HAVE_PLUGIN_WEBCAM "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginWebcam.cpp)
install(FILES pluginWebcam.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Homog
#
set(HAVE_PLUGIN_HOMOG "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginHomog.cpp)
install(FILES pluginHomog.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Savec
#
set(HAVE_PLUGIN_SAVE "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginSave.cpp)
install(FILES pluginSave.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Sync
#
set(HAVE_PLUGIN_SYNC "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginSync.cpp)
install(FILES pluginSync.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Pattern
#
set(HAVE_PLUGIN_PATTERN "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginPattern.cpp)
install(FILES pluginPattern.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Read
#
set(HAVE_PLUGIN_READ "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginRead.cpp)
install(FILES pluginRead.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Transform
#
set(HAVE_PLUGIN_TRANSFORM "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginTransform.cpp)
install(FILES pluginTransform.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Drip
#
set(HAVE_PLUGIN_DRIP "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginDrip.cpp)
install(FILES pluginDrip.hpp DESTINATION ${IMGV_INCLUDE})

# plugin DripMulti
#
set(HAVE_PLUGIN_DRIPMULTI "YES" PARENT_SCOPE)
set(IMGV_SRC ${IMGV_SRC} pluginDripMulti.cpp)
install(FILES pluginDripMulti.hpp DESTINATION ${IMGV_INCLUDE})

# plugin Ffmpeg
#
if(FFMPEG_FOUND)
    set(HAVE_PLUGIN_FFMPEG "YES" PARENT_SCOPE)
    set(IMGV_SRC ${IMGV_SRC} pluginFfmpeg.cpp)
    install(FILES pluginFfmpeg.hpp DESTINATION ${IMGV_INCLUDE})
endif()

# plugin Viewer
#
if(glfw_FOUND)
    set(HAVE_PLUGIN_VIEWER_GLFW "YES" PARENT_SCOPE)
    set(IMGV_SRC ${IMGV_SRC} pluginViewerGLFW.cpp parseShader.cpp viewerGLFW.cpp)
    install(FILES pluginViewerGLFW.hpp viewerGLFW.hpp parseShader.hpp DESTINATION ${IMGV_INCLUDE})
endif()

# plugin Viewer GLES2
#
if(gles2_FOUND)
    set(HAVE_PLUGIN_VIEWER_GLES2 "YES" PARENT_SCOPE)
    set(IMGV_SRC ${IMGV_SRC} pluginViewerGLES2.cpp pj.c video.c video_egl.c graphics.c videoDecode.c videoPlayer.c)
    install(FILES pluginViewerGLES2.hpp DESTINATION ${IMGV_INCLUDE})
endif()

#plugin Raspicam
#
if(raspicam_FOUND)
    set(HAVE_PLUGIN_RASPICAM "YES" PARENT_SCOPE)
    set(IMGV_SRC ${IMGV_SRC} pluginRaspicam.cpp picam.cpp)
    install(FILES pluginRaspicam.hpp DESTINATION ${IMGV_INCLUDE})
endif()

# plugin Prosilica
#
if(PROSILICA_FOUND)	
	# plugin Transform
	#
	set(HAVE_PLUGIN_TRANSFORM "YES" PARENT_SCOPE)
	set(IMGV_SRC ${IMGV_SRC} pluginTransform.cpp)
	install(FILES pluginTransform.hpp DESTINATION ${IMGV_INCLUDE})

	set(HAVE_PLUGIN_PROSILICA "YES" PARENT_SCOPE)
	set(IMGV_SRC ${IMGV_SRC} pluginProsilica.cpp)
	install(FILES pluginProsilica.hpp DESTINATION ${IMGV_INCLUDE})
endif()

# plugin ptgrey
#
if(PTGREY_FOUND)	
	set(HAVE_PLUGIN_PTGREY "YES" PARENT_SCOPE)
	set(IMGV_SRC ${IMGV_SRC} pluginPtGrey.cpp)
	install(FILES pluginPtGrey.hpp DESTINATION ${IMGV_INCLUDE})
endif()

##
## profilometre
##
if(IMGV_USE_PROFILOMETRE)
  set(HAVE_PROFILOMETRE "YES" PARENT_SCOPE)
  set(IMGV_SRC ${IMGV_SRC} profilometre.c)
  install(FILES profilometre.h DESTINATION ${IMGV_INCLUDE})
endif()

##
## library, shared and static
##


add_library(imgv SHARED ${IMGV_SRC})
target_link_libraries (imgv ${IMGV_LIBS})


if (IMGV_LIB_STATIC)
  add_library(imgv-static STATIC ${IMGV_SRC})
  target_link_libraries (imgv-static ${IMGV_LIBS})
  set_target_properties(imgv-static PROPERTIES OUTPUT_NAME "imgv")
  set_target_properties (imgv-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif()

##
## install
##

## shared imgu
#install(TARGETS imgv LIBRARY DESTINATION ${IMGV_LIB})
install(TARGETS imgv DESTINATION ${IMGV_LIB})
install(FILES imgv.hpp blob.hpp message.hpp util.hpp recyclable.hpp rqueue.hpp imqueue.hpp qpmanager.hpp oscutils.hpp plugin.hpp tcpcast.h udpcast.h DESTINATION ${IMGV_INCLUDE})


if (IMGV_LIB_STATIC)
  install(TARGETS imgv-static ARCHIVE DESTINATION ${IMGV_LIB})
  target_link_libraries (imgv-static ${IMGV_LIBS})
endif()


