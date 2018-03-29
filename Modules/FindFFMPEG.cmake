# Include the local modules directory

if( NOT FFMPEG_FOUND)

  # Locate LibFFMPEG files
  find_path( FFMPEG_AVCODEC_INCLUDE_DIR libavcodec/avcodec.h
    PATHS
    #    /usr/local/include
    /usr/include
    /opt/local/include
    )
  MARK_AS_ADVANCED( FFMPEG_AVCODEC_INCLUDE_DIR)

  find_path( FFMPEG_AVFORMAT_INCLUDE_DIR libavformat/avformat.h
    PATHS
    #/usr/local/include
    /usr/include
    /opt/local/include
    )
  MARK_AS_ADVANCED( FFMPEG_AVFORMAT_INCLUDE_DIR)

  find_path( FFMPEG_AVUTIL_INCLUDE_DIR libavutil/avutil.h
    PATHS
    #/usr/local/include
    /usr/include
    /opt/local/include
    )
  MARK_AS_ADVANCED( FFMPEG_AVUTIL_INCLUDE_DIR)

  find_path( FFMPEG_SWSCALE_INCLUDE_DIR libswscale/swscale.h
    PATHS
    #/usr/local/include
    /usr/include
    /opt/local/include
    )
  MARK_AS_ADVANCED( FFMPEG_SWSCALE_INCLUDE_DIR)

  #message(WARNING "ffmpeg : ${FFMPEG_AVCODEC_INCLUDE_DIR}")
  #message(WARNING "ffmpeg : ${FFMPEG_AVFORMAT_INCLUDE_DIR}")
  #message(WARNING "ffmpeg : ${FFMPEG_AVUTIL_INCLUDE_DIR}")
  #message(WARNING "ffmpeg : ${FFMPEG_SWSCALE_INCLUDE_DIR}")

  if( FFMPEG_AVCODEC_INCLUDE_DIR AND FFMPEG_AVFORMAT_INCLUDE_DIR AND FFMPEG_AVUTIL_INCLUDE_DIR AND FFMPEG_SWSCALE_INCLUDE_DIR)
     set (FFMPEG_INCLUDE_DIR	${FFMPEG_AVCODEC_INCLUDE_DIR}
				${FFMPEG_AVFORMAT_INCLUDE_DIR}
				${FFMPEG_AVUTIL_INCLUDE_DIR}
				${FFMPEG_SWSCALE_INCLUDE_DIR})
  endif()

  find_library( FFMPEG_AVCODEC_LIBRARY
    avcodec
    PATHS
    #/usr/local
    /usr
    /opt/local
    /usr/lib/arm-linux-gnueabihf
    PATH_SUFFIXES lib lib64 lib/i386-linux-gnu lib/x86_64-linux-gnu
    )
  MARK_AS_ADVANCED( FFMPEG_AVCODEC_LIBRARY)

  find_library( FFMPEG_AVUTIL_LIBRARY
    avutil
    PATHS
    #/usr/local
    /usr
    /opt/local
    /usr/lib/arm-linux-gnueabihf
    PATH_SUFFIXES lib lib64 lib/i386-linux-gnu lib/x86_64-linux-gnu
    )
  MARK_AS_ADVANCED( FFMPEG_AVUTIL_LIBRARY )

  find_library( FFMPEG_AVFORMAT_LIBRARY
    avformat
    PATHS
    #/usr/local
    /usr
    /opt/local
    /usr/lib/arm-linux-gnueabihf
    PATH_SUFFIXES lib lib64 lib/i386-linux-gnu lib/x86_64-linux-gnu
    )
  MARK_AS_ADVANCED( FFMPEG_AVFORMAT_LIBRARY )

  find_library( FFMPEG_SWSCALE_LIBRARY
    swscale
    PATHS
    #/usr/local
    /usr
    /opt/local
    /usr/lib/arm-linux-gnueabihf
    PATH_SUFFIXES lib lib64 lib/i386-linux-gnu lib/x86_64-linux-gnu
    )
  MARK_AS_ADVANCED( FFMPEG_SWSCALE_LIBRARY )

  if (FFMPEG_AVCODEC_LIBRARY AND FFMPEG_AVFORMAT_LIBRARY AND FFMPEG_AVUTIL_LIBRARY AND FFMPEG_SWSCALE_LIBRARY)
    #get_filename_component( FFMPEG_LIBRARY_DIR ${FFMPEG_AVFORMAT_LIBRARY} PATH )
#    set( FFMPEG_LIBRARY_DIR	${FFMPEG_AVCODEC_LIBRARY}
#				${FFMPEG_AVFORMAT_LIBRARY}
#				${FFMPEG_AVUTIL_LIBRARY}
#				${FFMPEG_SWSCALE_LIBRARY})
    set( FFMPEG_LIBRARIES ${FFMPEG_AVCODEC_LIBRARY} ${FFMPEG_AVUTIL_LIBRARY} ${FFMPEG_AVFORMAT_LIBRARY} ${FFMPEG_SWSCALE_LIBRARY})
    MARK_AS_ADVANCED( FFMPEG_LIBRARY_DIR)
  endif()
  
  if( FFMPEG_INCLUDE_DIR AND FFMPEG_LIBRARIES)
    set( FFMPEG_FOUND TRUE)
    #set( FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    #set( FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBRARY_DIR})
  endif()

  if( FFMPEG_FOUND)
    if( NOT FFMPEG_FIND_QUIETLY)
      message( STATUS "Found FFMPEG library: ${FFMPEG_LIBRARIES}")
      message( STATUS "Found FFMPEG library dirs: ${FFMPEG_LIBRARY_DIR}")
      message( STATUS "Found FFMPEG inc dirs: ${FFMPEG_INCLUDE_DIR}")
    endif( NOT FFMPEG_FIND_QUIETLY)
  else( FFMPEG_FOUND)
    if( FFMPEG_FIND_REQUIRED)
      message( FATAL_ERROR "Could not find FFMPEG")
    else( FFMPEG_FIND_REQUIRED)
      if( NOT FFMPEG_FIND_QUIETLY)
	message( STATUS "Could not find FFMPEG")
      endif( NOT FFMPEG_FIND_QUIETLY)
    endif( FFMPEG_FIND_REQUIRED)
  endif( FFMPEG_FOUND)
endif( NOT FFMPEG_FOUND)
