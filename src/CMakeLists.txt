# Build the phase specific libraries
add_subdirectory(frontend)
# add_subdirectory(semantic)
# add_subdirectory(codegen)
# add_subdirectory(optimizer)

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

# include antlr generated files
include_directories(${ANTLR_l24Grammar_OUTPUT_DIR})


# compiler binary target
add_executable(l24 
    ${CMAKE_CURRENT_SOURCE_DIR}/driver.cpp
    ${ANTLR_l24Grammar_CXX_OUTPUTS}
)

target_link_libraries(l24 PRIVATE frontend
                                    #  semantic
                                    #  ircodegen
                                    #  asmcodegen
                                    #  optimizer
                                     antlr4_static
                                     ${llvm_libs}
)

# runtime
# add_subdirectory(runtime)