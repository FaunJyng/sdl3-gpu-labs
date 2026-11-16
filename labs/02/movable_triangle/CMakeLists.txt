add_executable(movable_triangle movable_triangle.cpp)
target_link_libraries(movable_triangle PRIVATE SDL3::SDL3 imgui)
add_custom_command(
	TARGET movable_triangle 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:movable_triangle>/02/movable_triangle/shaders"
)