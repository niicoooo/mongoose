cmake_minimum_required(VERSION 3.0)
find_package(PythonInterp REQUIRED)
enable_language(C)


project (myTelnet)


add_executable(myTelnet
    mongoose.c mongoose.h
    examples/simplest_telnet_server/simplest_telnet_server.c
    libtelnet/libtelnet.c libtelnet/libtelnet.h)


target_include_directories(myTelnet PUBLIC ${PROJECT_BINARY_DIR} libtelnet src . )


#target_compile_options(myTelnet PUBLIC "-DCS_ENABLE_DEBUG")
target_compile_options(myTelnet PUBLIC "-ggdb")
target_compile_options(myTelnet PUBLIC "-Wall")
#target_compile_options(myTelnet PUBLIC "-Werror")
target_compile_options(myTelnet PUBLIC "-DMG_ENABLE_TELNET")


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
                    src/mg_telnet.c src/mg_telnet.h
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
