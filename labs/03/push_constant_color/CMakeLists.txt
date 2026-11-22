add_executable(push_constant_color push_constant_color.cpp)

target_link_libraries(push_constant_color PRIVATE SDL3::SDL3 common)

target_include_directories(push_constant_color PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_custom_command(
	TARGET push_constant_color
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:push_constant_color>/02/push_constant_color/shaders"
)
