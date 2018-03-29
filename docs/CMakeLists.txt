
add_custom_target(docs ALL ${DOXYGEN_EXECUTABLE}
                  WORKING_DIRECTORY ${IMGV_BINARY_DIR}/docs
                  COMMENT "Generating HTML documentation" VERBATIM
		)

message("..... ${IMGV_BINARY_DIR}/docs/html" )
install(DIRECTORY ${IMGV_BINARY_DIR}/docs/html DESTINATION ${IMGV_SHARE}/docs)


