add_executable(multiple_vertex_buffer multiple_vertex_buffer.cpp)
target_link_libraries(multiple_vertex_buffer PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET multiple_vertex_buffer 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:multiple_vertex_buffer>/02/multiple_vertex_buffer/shaders"
)