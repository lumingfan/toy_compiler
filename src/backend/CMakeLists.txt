# define a library for the front end
add_library(backend)
target_sources(backend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/code_gen_ctx.h
        ${CMAKE_CURRENT_SOURCE_DIR}/code_gen_ctx.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/code_gen.h
        ${CMAKE_CURRENT_SOURCE_DIR}/code_gen.cpp
)
