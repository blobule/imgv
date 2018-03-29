

link_libraries (imgv)

##
## playOmni
## (save 20 images from 3 fisheye cams)
##
if ( HAVE_PLUGIN_FFMPEG
 AND HAVE_PLUGIN_PROSILICA
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding playOmni")
    add_executable (playOmni playOmni.cpp)
    install(TARGETS playOmni RUNTIME DESTINATION ${IMGV_BIN})
endif ()

##
## scanBleuet
## grab images
##
if ( HAVE_PLUGIN_PROSILICA
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding scanBleuet")
    add_executable (scanBleuet scanBleuet.cpp)
    install(TARGETS scanBleuet RUNTIME DESTINATION ${IMGV_BIN})
endif ()


##
## hydroquebec
##
if ( HAVE_PLUGIN_PROSILICA
 AND HAVE_PLUGIN_VIEWER_GLFW )
    message(STATUS "adding hydroquebec")
    add_executable (hydroquebec hydroquebec.cpp)
    install(TARGETS hydroquebec RUNTIME DESTINATION ${IMGV_BIN})
endif ()


##
## kittistats
##
message(STATUS "adding kittistats")
add_executable (kittistats kittistats.cpp)
install(TARGETS kittistats RUNTIME DESTINATION ${IMGV_BIN})
