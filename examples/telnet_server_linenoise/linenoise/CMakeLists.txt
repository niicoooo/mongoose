cmake_minimum_required(VERSION 3.0)
enable_language(C)


project (linenoise)


add_library(linenoise STATIC linenoise.c linenoise.h)
target_include_directories(linenoise PUBLIC .)
target_compile_options(linenoise PRIVATE "-ggdb")
target_compile_options(linenoise PUBLIC "-Wall")
target_compile_options(linenoise PUBLIC "-Werror")


add_executable(linenoiseExample example.c)
target_link_libraries(linenoiseExample PUBLIC linenoise)
