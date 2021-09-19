add_subdirectory(Lexer)

add_executable(Compiler "Compiler.cxx")
target_link_libraries(Compiler PRIVATE Lexer)
