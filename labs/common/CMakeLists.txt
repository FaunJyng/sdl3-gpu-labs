add_library(common STATIC
	src/path.cpp
	src/timer.cpp
    src/gpu_shader_creator.cpp
	src/gpu_buffer_uploader.cpp
	src/gpu_graphics_pipeline_creator.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC SDL3::SDL3
    PRIVATE fmt::fmt
)