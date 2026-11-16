add_executable(first_triangle first_triangle.cpp)
target_link_libraries(first_triangle PRIVATE SDL3::SDL3)
add_custom_command(
	TARGET first_triangle 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:first_triangle>/01/first_triangle/shaders"
)