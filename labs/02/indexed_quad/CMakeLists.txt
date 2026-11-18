add_executable(indexed_quad indexed_quad.cpp)
target_link_libraries(indexed_quad PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET indexed_quad 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:indexed_quad>/02/indexed_quad/shaders"
)