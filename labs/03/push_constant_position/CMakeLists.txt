add_executable(push_constant_position push_constant_position.cpp)

target_link_libraries(push_constant_position PRIVATE SDL3::SDL3 common)

target_include_directories(push_constant_position PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_custom_command(
	TARGET push_constant_position
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:push_constant_position>/03/push_constant_position/shaders"
)
