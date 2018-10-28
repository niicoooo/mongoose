cmake_minimum_required(VERSION 3.0)
find_package(PythonInterp REQUIRED)
enable_language(C)


project (mgTelnet)


add_subdirectory(libtelnet)


add_library(libmg STATIC mongoose.c mongoose.h)
target_link_libraries(libmg PUBLIC libtelnet)
target_include_directories(libmg PUBLIC ${PROJECT_BINARY_DIR})
target_compile_options(libmg PUBLIC "-DMG_ENABLE_TELNET")
target_compile_options(libmg PRIVATE "-ggdb")
#target_compile_options(libmg PUBLIC "-DCS_ENABLE_DEBUG")


add_custom_command(OUTPUT 
                    mongoose.c
                    PRE_BUILD
                    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/amalgam.py 
                        --prefix=MG --public-header=mongoose.h
                        --strict --strict --norel
                        `cat ${PROJECT_SOURCE_DIR}/mongoose.c.manifest`
                        > ${PROJECT_BINARY_DIR}/mongoose.c
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    DEPENDS
                    mongoose.c.manifest
                    src/mg_telnet.c src/mg_telnet.h src/mg_telnet_debug.c
                )


add_custom_command(OUTPUT 
                    mongoose.h
                    PRE_BUILD
                    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/amalgam.py 
                        --prefix=MG
                        --strict --strict --norel
                        `cat ${PROJECT_SOURCE_DIR}/mongoose.h.manifest`
                        > ${PROJECT_BINARY_DIR}/mongoose.h
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    DEPENDS
                    mongoose.h.manifest
                    src/mg_telnet.h
                )


add_executable(simplestTelnetServer examples/simplest_telnet_server/simplest_telnet_server.c)
target_compile_options(simplestTelnetServer PUBLIC "-ggdb")
target_compile_options(simplestTelnetServer PUBLIC "-Wall")
target_compile_options(simplestTelnetServer PUBLIC "-Werror")
target_link_libraries(simplestTelnetServer PUBLIC libmg)
