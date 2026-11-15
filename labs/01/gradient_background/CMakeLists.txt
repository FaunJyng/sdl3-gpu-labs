add_executable(gradient_background gradient_background.cpp)
target_link_libraries(gradient_background PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET gradient_background 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:gradient_background>/01/gradient_background/shaders"
)