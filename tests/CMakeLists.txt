

link_libraries (imgv)

add_executable (testnet testnet.cpp)
add_executable (testblob testblob.cpp)
add_executable (testmessage testmessage.cpp)
add_executable (testusleep testusleep.cpp)
add_executable (testdie testdie.cpp)
add_executable (testreply testreply.cpp)
add_executable (testremote testremote.cpp)

if( CodedLight_FOUND )
	add_executable (testcodedlight testcodedlight.cpp)
endif()

#install(TARGETS netCamera RUNTIME DESTINATION ${IMGV_BIN})



