

link_libraries (imgv)

##
## imgv
##
	message(STATUS "adding imgvx example")
	add_executable (imgvx imgvx.cpp)
	install(TARGETS imgvx RUNTIME DESTINATION ${IMGV_BIN})

##
## scanMask
##
	message(STATUS "adding scanMask")
	add_executable (scanMask scanMask.cpp)
	install(TARGETS scanMask RUNTIME DESTINATION ${IMGV_BIN})

##
## sendImage
##
if ( HAVE_PLUGIN_STREAM )
	message(STATUS "adding sendImage")
	add_executable (sendImage sendImage.cpp)
	install(TARGETS sendImage RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## netSlave
##
	message(STATUS "adding netSlave")
	add_executable (netSlave netSlave.cpp)
	install(TARGETS netSlave RUNTIME DESTINATION ${IMGV_BIN})

##
## netCamera
##
if ( HAVE_PLUGIN_STREAM )
	message(STATUS "adding netCamera")
	add_executable (netCamera netCamera.cpp)
	install(TARGETS netCamera RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## extractPattern
##
if ( HAVE_PLUGIN_FFMPEG AND NOT IMGV_RASPBERRY_PI)
	message(STATUS "adding extractPattern")
	add_executable (extractPattern extractPattern.cpp)
	install(TARGETS extractPattern RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## netReceive
##
if ( HAVE_PLUGIN_NET )
	message(STATUS "adding netReceive")
	add_executable (netReceive netReceive.cpp)
	install(TARGETS netReceive RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## detectTags
##
if ( HAVE_PLUGIN_PROSILICA)
#    message(STATUS "adding detectTags")
#    add_executable (detectTags detectTags.cpp)
#    install(TARGETS detectTags RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## scanImages
##
if ( HAVE_PLUGIN_PROSILICA)
    message(STATUS "adding scanImages")
    add_executable (scanImages scanImages.cpp)
    install(TARGETS scanImages RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## playPattern
##
if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_PATTERN
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding playPattern")
    add_executable (playPattern playPattern.cpp)
    install(TARGETS playPattern RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playMovie
##
if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding playMovie")
    add_executable (playMovie playMovie.cpp)
    install(TARGETS playMovie RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## play3Movies
##
if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding play3Movies")
    add_executable (play3Movies play3Movies.cpp)
    install(TARGETS play3Movies RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## saveMovie
##
if(NOT IMGV_RASPBERRY_PI)
    message(STATUS "adding saveMovie")
    add_executable (saveMovie saveMovie.cpp)
    install(TARGETS saveMovie RUNTIME DESTINATION ${IMGV_BIN})
endif()


##
## playImage
##
if( HAVE_PLUGIN_VIEWER_GLFW
 AND HAVE_PLUGIN_HOMOG )
	message(STATUS "adding playImage")
	add_executable (playImage playImage.cpp)
	install(TARGETS playImage RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playImagePi
##
if( HAVE_PLUGIN_VIEWER_GLES2
 AND HAVE_PLUGIN_HOMOG )
	message(STATUS "adding playImagePi")
	add_executable (playImagePi playImagePi.cpp)
	install(TARGETS playImagePi RUNTIME DESTINATION ${IMGV_BIN})
endif()


##
## playPi
##
if( HAVE_PLUGIN_VIEWER_GLES2 )
	message(STATUS "adding playPi")
	add_executable (playPi playPi.cpp)
	install(TARGETS playPi RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playRhinoPi
##
if( HAVE_PLUGIN_VIEWER_GLES2 )
	message(STATUS "adding playRhinoPi")
	add_executable (playRhinoPi playRhinoPi.cpp)
	install(TARGETS playRhinoPi RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playFaezeh
##
if( HAVE_PLUGIN_VIEWER_GLES2 )
	message(STATUS "adding playFaezeh")
	add_executable (playFaezeh playFaezeh.cpp)
	install(TARGETS playFaezeh RUNTIME DESTINATION ${IMGV_BIN})
endif()


##
## playLeopard
##
if( HAVE_PLUGIN_VIEWER_GLFW )
	message(STATUS "adding playLeopard")
	add_executable (playLeopard playLeopard.cpp)
	install(TARGETS playLeopard RUNTIME DESTINATION ${IMGV_BIN})
endif()


##
## decodePi
##
if( HAVE_PLUGIN_VIEWER_GLES2 )
	message(STATUS "adding decodePi")
	add_executable (decodePi decodePi.cpp)
	install(TARGETS decodePi RUNTIME DESTINATION ${IMGV_BIN})
endif()
##
## playDeform
##
if(  HAVE_PLUGIN_VIEWER_GLFW
 AND HAVE_PLUGIN_HOMOG )
	message(STATUS "adding playDeform")
	add_executable (playDeform playDeform.cpp)
	install(TARGETS playDeform RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playMapper
##
if(  HAVE_PLUGIN_VIEWER_GLFW
 AND HAVE_PLUGIN_HOMOG )
	message(STATUS "adding playMapper")
	add_executable (playMapper playMapper.cpp)
	install(TARGETS playMapper RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playSuperMapper
# AND IMGV_SEB_RESERVED
##
if(  HAVE_PLUGIN_VIEWER_GLFW
 AND HAVE_PLUGIN_HOMOG )
	message(STATUS "adding playSuperMapper")
	add_executable (playSuperMapper playSuperMapper.cpp)
	install(TARGETS playSuperMapper RUNTIME DESTINATION ${IMGV_BIN})
endif()


##
## omniPlayer
##
if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_VIEWER_GLFW )
#    message(STATUS "adding omniPlayer")
#    add_executable (omniPlayer omniPlayer.cpp)
#    install(TARGETS omniPlayer RUNTIME DESTINATION ${IMGV_BIN})
endif()

if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_VIEWER_GLFW )
#    message(STATUS "adding omniPlayerLive")
#    add_executable (omniPlayerLive omniPlayerLive.cpp)
#    target_link_libraries (omniPlayerLive ${OCULUS_LIBS})
#    install(TARGETS omniPlayerLive RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## playCamera
##
if ( HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding playCamera")
    add_executable (playCamera playCamera.cpp)
    install(TARGETS playCamera RUNTIME DESTINATION ${IMGV_BIN})
endif()

##
## calibCaptureLoop
## analyse the projection - capture loop timing
## 
##
if ( HAVE_PLUGIN_PROSILICA)
#    message(STATUS "adding playCameraLoop")
#    add_executable (calibCaptureLoop calibCaptureLoop.cpp)
#    install(TARGETS calibCaptureLoop RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## omniMontagne
##
if(  HAVE_PLUGIN_DRIP
 AND HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_VIEWER_GLFW )
    #message(STATUS "adding omniMontagne")
    #add_executable (omniMontagne omniMontagne.cpp)
    #install(TARGETS omniMontagne RUNTIME DESTINATION ${IMGV_BIN})
endif()


