add_executable(vertex_buffer_triangle vertex_buffer_triangle.cpp)
target_link_libraries(vertex_buffer_triangle PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET vertex_buffer_triangle 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:vertex_buffer_triangle>/02/vertex_buffer_triangle/shaders"
)