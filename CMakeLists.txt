

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(IMGV)

set(IMGV_VERSION_MAJOR "0")
set(IMGV_VERSION_MINOR "2")
set(IMGV_VERSION_PATCH "1")
set(IMGV_VERSION "${IMGV_VERSION_MAJOR}.${IMGV_VERSION_MINOR}")
set(IMGV_VERSION_FULL "${IMGV_VERSION_MAJOR}.${IMGV_VERSION_MINOR}.${IMGV_VERSION_PATCH}")
OPTION(IMGV_LIB_STATIC "Build static library" OFF)
OPTION(IMGV_EXAMPLES "Build examples" ON)
OPTION(IMGV_TESTS "Build tests" ON)
OPTION(IMGV_APPS "Build apps" ON)
OPTION(IMGV_DOCUMENTATION "Generation documentation" ON)
#OPTION(IMGV_DOCUMENT_INTERNALS "Include internal documentation" OFF)
OPTION(IMGV_USE_PROFILOMETRE "Build profilometre (performance meter)" ON)
IF(EXISTS "/opt/vc/include/bcm_host.h")
	OPTION(IMGV_RASPBERRY_PI "Build for Raspberry Pi" ON)
	OPTION(IMGV_RASPICAM "Build raspicam" ON)
ELSE()
	OPTION(IMGV_RASPBERRY_PI "Build for Raspberry Pi" OFF)
ENDIF()
#
# Pour le debugage
#
OPTION(IMGV_SEB_RESERVED "Pour Sebastien seulement" OFF)

set(IMGV_BIN bin)
set(IMGV_LIB lib)
set(IMGV_INCLUDE include/imgv)
set(IMGV_SHARE share/imgv)

message("IMGV_SOURCE_DIR is ${IMGV_SOURCE_DIR}")
message("IMGV_BINARY_DIR is ${IMGV_BINARY_DIR}")

# Ou sont les modules?
#
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Modules/")

# uninstall target
#
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


#
# ou sont les include? inutile ici???
#
include_directories( ${IMGV_SOURCE_DIR} )
include_directories( ${IMGV_SOURCE_DIR}/imgv )
# pour les fichiers generes (config.hpp)
include_directories( ${IMGV_BINARY_DIR}/imgv )

##
## trouve les dependances
##

#
#
#
find_package(V4L2)

#
# pthread (required!)
#
find_package(PTHREAD REQUIRED)
  set(IMGV_LIBS ${IMGV_LIBS} ${PTHREAD_LIBS})
  include_directories(${PTHREAD_INCLUDE_DIRS})
  link_directories(${PTHREAD_LIB_DIR})

find_package(PNG)
if(PNG_FOUND)
  add_definitions(-DHAVE_LIBPNG)
  set(IMGV_LIBS ${IMGV_LIBS} ${PNG_LIBRARIES})
  include_directories(${PNG_INCLUDE_DIR})
  #add_definitions(${PNG_DEFINITIONS})
  link_directories(${PNG_LIBRARY})
  
  #set(IMGV_LIBS ${IMGV_LIBS} ${ZLIB_LIBRARIES})
  #include_directories(${ZLIB_INCLUDE_DIR})
  #link_directories(${ZLIB_LIBRARY})
else()
  #message(FATAL_ERROR "Cannot build without PNG")
  message("Could not find libpng12-dev")
endif()


#
# codedlight (used in structuredlight matching)
#
#find_package(CodedLight)
#if(CodedLight_FOUND)
#  set(IMGV_LIBS ${IMGV_LIBS} ${CODEDLIGHT_LIBRARIES})
#
#  include_directories(${CODEDLIGHT_INCLUDE_DIRS})
#  #link_directories(${OpenCV_LIB_DIR})
#  message("found CodedLight (libs): ${CODEDLIGHT_LIBRARIES}")
#  #message("found OpenCV (link dirs): ${OpenCV_LIB_DIR}")
#  #message("found OpenCV (include dirs): ${OpenCV_INCLUDE_DIR}")
#  message(STATUS "Found CodedLight")
#endif()
FIND_PACKAGE(CodedLight)
if(CodedLight_FOUND)
	INCLUDE_DIRECTORIES(${CODEDLIGHT_INCLUDE_DIRS})
	set(IMGV_LIBS ${IMGV_LIBS} ${CODEDLIGHT_LIBRARIES})
	message("found CodedLight (libs): ${CODEDLIGHT_LIBRARIES}")
endif()


#
# OpenCv (required!)
#
find_package(OpenCV REQUIRED core imgproc highgui calib3d)
if(OpenCV_FOUND)
  #TARGET_LINK_LIBRARIES(imgv ${OpenCV_LIBS})
  set(IMGV_LIBS ${IMGV_LIBS} ${OpenCV_LIBS})
  include_directories(${OpenCV_INCLUDE_DIRS})
  link_directories(${OpenCV_LIB_DIR})
  #message("found OpenCV (libs): ${OpenCV_LIBS}")
  #message("found OpenCV (link dirs): ${OpenCV_LIB_DIR}")
  #message("found OpenCV (include dirs): ${OpenCV_INCLUDE_DIR}")
  message(STATUS "Found OpenCV")
endif()

#
# oscpack (required!)
#
find_package(oscpack REQUIRED)
if(oscpack_FOUND)
  set(IMGV_LIBS ${IMGV_LIBS} ${oscpack_LIBRARY})
  include_directories(${oscpack_INCLUDE_DIR})
  link_directories(${oscpack_LIBRARY_DIR})
  #message("Found oscpack")
  #message("oscpack_LIBRARY is ${oscpack_LIBRARY}")
  #message("oscpack_INCLUDE_DIR is ${oscpack_INCLUDE_DIR}")
  #message("oscpack_LIBRARY_DIR is ${oscpack_LIBRARY_DIR}")
else()
  message("oscpack not found. (need version >=1.1.0.) dep: liboscpack-dev (attention a la version)")
endif()

# pour le save-to-video (pluginSave)
#
find_program(AVCONV avconv)
if( ${AVCONV} MATCHES "AVCONV-NOTFOUND" )
	message("avconv NOT FOUND")
	set(HAVE_AVCONV 0)
else()
	message("-- Found avconv")
	set(HAVE_AVCONV 1)
endif()

# pour le pluginFfmpeg
#
find_package(FFMPEG)
if(FFMPEG_FOUND)
  add_definitions(-DHAVE_FFMPEG)
  set(IMGV_LIBS ${IMGV_LIBS} ${FFMPEG_LIBRARIES})
  include_directories(${FFMPEG_INCLUDE_DIR})
  link_directories(${FFMPEG_LIBRARY_DIR})
  #pour raspberry pi
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")
else()
  message(STATUS "Could not find ffmpeg: no video playback")
endif()

# pour la compression d'images en temps réel
#
find_package(LZ4)
if(LZ4_FOUND)
  add_definitions(-DHAVE_LZ4)
  set(IMGV_LIBS ${IMGV_LIBS} ${LZ4_LIBS})
  include_directories(${LZ4_INCLUDE_DIR})
  link_directories(${LZ4_LIBRARY_DIR})
else()
  message(STATUS "Could not find LZ4: no image compression")
endif()


# pour le pluginViewerGLES2 (raspberry pi)
#
if( IMGV_RASPBERRY_PI )
  add_definitions(-DHAVE_GLES2)
  add_definitions(-DSTANDALONE)
  add_definitions(-D__STDC_CONSTANT_MACROS)
  add_definitions(-D__STDC_LIMIT_MACROS)
  add_definitions(-DTARGET_POSIX)
  add_definitions(-D_LINUX)
  add_definitions(-fPIC)
  add_definitions(-DPIC)
  add_definitions(-D_REENTRANT)
  add_definitions(-D_LARGEFILE64_SOURCE)
  add_definitions(-D_FILE_OFFSET_BITS=64)
  add_definitions(-U_FORTIFY_SOURCE)
  add_definitions(-g)
  add_definitions(-DHAVE_LIBOPENMAX=2)
  add_definitions(-DOMX)
  add_definitions(-DOMX_SKIP64BIT)
  add_definitions(-ftree-vectorize)
  add_definitions(-pipe)
  add_definitions(-DUSE_EXTERNAL_OMX)
  add_definitions(-DHAVE_LIBBCM_HOST)
  add_definitions(-DUSE_EXTERNAL_LIBBCM_HOST)
  add_definitions(-DUSE_VCHIQ_ARM)
  add_definitions(-Wno-psabi)
  set(IMGV_LIBS ${IMGV_LIBS} /opt/vc/src/hello_pi/libs/ilclient/libilclient.a /opt/vc/lib/libopenmaxil.so /opt/vc/lib/libbcm_host.so /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so /opt/vc/lib/libmmal_core.so /opt/vc/lib/libmmal.so /opt/vc/lib/libmmal_util.so /opt/vc/lib/libmmal_vc_client.so)
set(raspicam_FOUND 0)
if( IMGV_RASPICAM )
  set(IMGV_LIBS ${IMGV_LIBS} libraspicam.so)
  set(raspicam_FOUND 1)
endif()
  include_directories( /opt/vc/include /opt/vc/include/interface/vcos/pthreads /opt/vc/include/interface/vmcs_host/linux  /opt/vc/src/hello_pi/libs/ilclient/ /opt/vc/include/interface/)
  set(gles2_FOUND 1)
  message(STATUS "Found gles2 (because of IMGV_RASPBERRY_PI)")
  message(STATUS "${IMGV_LIBS}")
endif()

# pour le pluginViewerGLFW
#
if(NOT IMGV_RASPBERRY_PI)
find_package(glfw)
find_package(GLEW)
if(glfw_FOUND)
  add_definitions(-DHAVE_GLFW)
  if(NOT GLEW_FOUND)
     message(STATUS "GLFW requires GLEW. (sudo apt-get install libglew-dev)")
  endif()
  set(IMGV_LIBS ${IMGV_LIBS} ${GLFW_LIBRARY} ${GLEW_LIBS})
  if (APPLE)
      find_package(OpenGL REQUIRED)
      include_directories(${OPENGL_INCLUDE_DIR})
      set (IMGV_LIBS ${IMGV_LIBS} /opt/local/lib/libXrandr.dylib /opt/local/lib/libXi.dylib ${OPENGL_gl_LIBRARY})
  else ()
      if( IMGV_RASPBERRY_PI )
  		link_directories(/opt/vc/lib)
      	set (IMGV_LIBS ${IMGV_LIBS} Xrandr Xi GLESv2)
      else()
	    if(WIN32)
  	  	  
        else() #standard linux
      	  set (IMGV_LIBS ${IMGV_LIBS} Xrandr Xi GL)
		endif()
      endif()
  endif (APPLE)
  include_directories(${GLFW_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
  link_directories(${GLFW_LIBRARY_DIR} ${GLEW_LIB_DIR})
  message(STATUS "Found glfw")
else()
  message(STATUS "Not found glfw (see www.glfw.org) (dep: xorg-dev libglu1-mesa-dev libglew-dev")
endif()
endif()

# Anciens drivers de prosilica (pour le pluginGige)
#
find_package(PROSILICA)
if(PROSILICA_FOUND)
  add_definitions(-DHAVE_PROSILICA)
  set(IMGV_LIBS ${IMGV_LIBS} ${PROSILICA_LIBS})
  include_directories(${PROSILICA_INCLUDE_DIR})
  link_directories(${PROSILICA_LIB_DIR})
else()
  #message(FATAL_ERROR "Cannot build without Prosilica")
  message("Could not find prosilica")
endif()

# driver Ptgrey Fly camera
#
find_package(PTGREY)
if(PTGREY_FOUND)
  add_definitions(-DHAVE_PTGREY)
  set(IMGV_LIBS ${IMGV_LIBS} ${PTGREY_LIBS})
  include_directories(${PTGREY_INCLUDE_DIR})
  link_directories(${PTGREY_LIB_DIR})
else()
  message("Could not find ptgrey")
endif()



# library OVR de Oculus Rift
#
find_package(OCULUS)
if(OCULUS_FOUND)
  add_definitions(-DHAVE_OCULUS)
  if (WIN32)
    set(IMGV_LIBS ${IMGV_LIBS} ${OCULUS_LIBS})
  else()
    set(IMGV_LIBS ${IMGV_LIBS} ${OCULUS_LIBS} X11 Xinerama udev pthread GL rt Xrandr)
  endif()
  include_directories(${OCULUS_INCLUDE_DIR})
  include_directories(${OCULUS_INCLUDE_DIR}/LibOVRKernel/Src/)
  include_directories(${OCULUS_INCLUDE_DIR}/oculus/Src/)
  link_directories(${OCULUS_LIB_DIR})
else()
  #message(FATAL_ERROR "Cannot build without Oculus")
  message("Could not find oculus")
endif()

##
## Documentation
##
# -> le find package ne fonctionne pas avec qtcreator
#FIND_PACKAGE(Doxygen)
# -> manual find of doxygen
find_program(DOXYGEN_EXECUTABLE doxygen)
if( ${DOXYGEN_EXECUTABLE} MATCHES "DOXYGEN_EXECUTABLE-NOTFOUND" )
	message("doxygen NOT FOUND")
	set(DOXYGEN_FOUND 0)
else()
	message("-- Found doxygen")
	set(DOXYGEN_FOUND 1)
endif()

#linking under MinGW
if (WIN32)
  set(IMGV_LIBS ${IMGV_LIBS} opengl32 glew32 avformat avcodec avdevice avutil avresample ws2_32 pthread zlib)
endif()

##
## Sources
##
#include_directories( ${IMGV_SOURCE_DIR} )
add_subdirectory(imgv)


##
## Fichiers auto-generes
##

# configure a header file to pass some of the CMake settings
# to the source code
message(STATUS "Generating config.hpp")
configure_file ( "${IMGV_SOURCE_DIR}/config.hpp.in" "${IMGV_BINARY_DIR}/imgv/config.hpp")
install(FILES ${IMGV_BINARY_DIR}/imgv/config.hpp DESTINATION ${IMGV_INCLUDE})


##
## Documentation
##
if (DOXYGEN_FOUND)
 if( IMGV_DOCUMENTATION )
    configure_file(${IMGV_SOURCE_DIR}/docs/Doxyfile.in
                   ${IMGV_BINARY_DIR}/docs/Doxyfile @ONLY)
#    if (IMGV_DOCUMENT_INTERNALS)
#        set(IMGV_INTERNAL_DOCS "${IMGV_SOURCE_DIR}/imgv/imgv.hpp ${IMGV_SOURCE_DIR}/imgv/plugin.hpp ${IMGV_SOURCE_DIR}/recyclable.hpp ${IMGV_SOURCE_DIR}/docs/internal.dox")
#    endif()
    add_subdirectory(docs)
 endif()
endif()

##
## Share folders (mostly shaders)
##
add_subdirectory(share)


##
## Examples
##
if (IMGV_EXAMPLES)
    add_subdirectory(examples)
endif()

##
## Tests
##
if (IMGV_TESTS)
    add_subdirectory(tests)
endif()

##
## Apps
##
if (IMGV_APPS)
    add_subdirectory(apps)
endif()

##
## plugin summary
##
message(STATUS "Summary of plugins:")
# plugin Webcam
#
message("   plugin V4L2:       ${HAVE_PLUGIN_V4L2}")
message("   plugin Net:        ${HAVE_PLUGIN_NET}")
message("   plugin Stream:     ${HAVE_PLUGIN_STREAM}")
message("   plugin Syslog:     ${HAVE_PLUGIN_SYSLOG}")
message("   plugin Webcam:     ${HAVE_PLUGIN_WEBCAM}")
message("   plugin Homog:      ${HAVE_PLUGIN_HOMOG}")
message("   plugin Save:       ${HAVE_PLUGIN_SAVE}")
message("   plugin Sync:       ${HAVE_PLUGIN_SYNC}")
message("   plugin Pattern:    ${HAVE_PLUGIN_PATTERN}")
message("   plugin Read:       ${HAVE_PLUGIN_READ}")
message("   plugin Transform:  ${HAVE_PLUGIN_TRANSFORM}")
message("   plugin Drip:       ${HAVE_PLUGIN_DRIP}")
message("   plugin DripMulti:  ${HAVE_PLUGIN_DRIPMULTI}")
message("   plugin Ffmpeg:     ${HAVE_PLUGIN_FFMPEG}")
message("   plugin ViewerGLFW: ${HAVE_PLUGIN_VIEWER_GLFW}")
message("   plugin ViewerGLES2: ${HAVE_PLUGIN_VIEWER_GLES2}")
message("   plugin RaspiCam:   ${HAVE_PLUGIN_RASPICAM}")
message("   plugin Prosilica:  ${HAVE_PLUGIN_PROSILICA}")
message("   plugin Ptgrey:     ${HAVE_PLUGIN_PTGREY}")
message("   plugin Leopard  :  ${HAVE_PLUGIN_LEOPARD}")
message("   profilometre:      ${HAVE_PROFILOMETRE}")




