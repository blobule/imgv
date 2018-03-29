
install(FILES README DESTINATION ${IMGV_SHARE})

file (GLOB shaders "shaders/*.glsl")
install (FILES ${shaders} DESTINATION ${IMGV_SHARE}/shaders)

file (GLOB images "images/*.jpg" "images/*.png")
install (FILES ${images} DESTINATION ${IMGV_SHARE}/images)

#message("shaders are ${shaders}")
#message("images are ${images}")

