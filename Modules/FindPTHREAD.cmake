
FIND_PATH(PTHREAD_INCLUDE_DIR pthread.h)
FIND_LIBRARY(PTHREAD_LIBRARY pthread)

IF(PTHREAD_LIBRARY AND PTHREAD_INCLUDE_DIR)
    SET(PTHREAD_LIBS ${PTHREAD_LIBRARY})
    SET(PTHREAD_FOUND "YES")
ELSE()
    SET(PTHREAD_FOUND "NO")
ENDIF()

IF(PTHREAD_FOUND)
   IF(NOT PTHREAD_FIND_QUIETLY)
      MESSAGE(STATUS "Found pthread: ${PTHREAD_LIBS}")
   ENDIF()
ELSE()
   IF(PTHREAD_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find PTHREAD library")
   ENDIF()
ENDIF()


MARK_AS_ADVANCED(
  PTHREAD_LIBRARY
  PTHREAD_INCLUDE_DIR
  )
