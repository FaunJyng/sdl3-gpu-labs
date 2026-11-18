add_executable(prim_topo_test prim_topo_test.cpp)
target_link_libraries(prim_topo_test PRIVATE SDL3::SDL3 imgui)
add_custom_command(
	TARGET prim_topo_test 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/build"
        "$<TARGET_FILE_DIR:prim_topo_test>/02/prim_topo_test/shaders"
)