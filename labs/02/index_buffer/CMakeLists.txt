add_executable(index_buffer index_buffer.cpp)
target_link_libraries(index_buffer PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET index_buffer 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:index_buffer>/02/index_buffer/shaders"
)