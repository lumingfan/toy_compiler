# call macro to add lexer and grammar to your build dependencies.
antlr_target(l24Grammar ${CMAKE_SOURCE_DIR}/grammar/l24.g4 LEXER PARSER VISITOR)

# include antlr generated files
include_directories(${ANTLR_l24Grammar_OUTPUT_DIR})

# define a library for the front end
add_library(frontend)
target_sources(frontend PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/ast_builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ast_builder.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/ast.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ast.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/front_end.h
        ${CMAKE_CURRENT_SOURCE_DIR}/front_end.cpp
        ${ANTLR_l24Grammar_CXX_OUTPUTS}
)
target_link_libraries(frontend PRIVATE antlr4_static)
