add_executable(triangle_color triangle_color.cpp)
target_link_libraries(triangle_color PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET triangle_color 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:triangle_color>/01/triangle_color/shaders"
)