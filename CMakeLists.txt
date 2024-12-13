###############################################################################
# Project setup
###############################################################################

cmake_minimum_required(VERSION 3.5)
project(Liberty2JSON C CXX)
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG
"${CMAKE_CXX_FLAGS_DEBUG} -Wall -g"
)
set(CMAKE_CXX_FLAGS_RELEASE
"${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -g"
)

###############################################################################
# Fetch files
###############################################################################

file(GLOB LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/syntform.c)
file(GLOB LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

###############################################################################
# Flex/Bison targets
###############################################################################

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
bison_target(liberty_parser ${CMAKE_CURRENT_SOURCE_DIR}/src/liberty_parser.y ${CMAKE_CURRENT_SOURCE_DIR}/src/liberty_parser.c)
bison_target(syntax_parser ${CMAKE_CURRENT_SOURCE_DIR}/src/syntax_parser.y ${CMAKE_CURRENT_SOURCE_DIR}/src/syntax_parser.c)
flex_target(token ${CMAKE_CURRENT_SOURCE_DIR}/src/token.l ${CMAKE_CURRENT_SOURCE_DIR}/src/token.c)
flex_target(synttok ${CMAKE_CURRENT_SOURCE_DIR}/src/synttok.l ${CMAKE_CURRENT_SOURCE_DIR}/src/synttok.c)
add_flex_bison_dependency(token liberty_parser)
add_flex_bison_dependency(synttok syntax_parser)
set(FLEX_BISON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/liberty_parser.c ${CMAKE_CURRENT_SOURCE_DIR}/src/syntax_parser.c ${CMAKE_CURRENT_SOURCE_DIR}/src/synttok.c ${CMAKE_CURRENT_SOURCE_DIR}/src/token.c)

###############################################################################
# Subsystem
###############################################################################

add_subdirectory(third_party/backward-cpp)

###############################################################################
# Executable target for Liberty2JSON
###############################################################################

add_executable(liberty2json ${CMAKE_CURRENT_SOURCE_DIR}/liberty2json.cpp 
                            ${PROJECT_SOURCE_DIR}/third_party/backward-cpp/backward.cpp 
                            ${LIB_SOURCES} ${LIB_HEADERS} ${FLEX_BISON_SOURCES})
target_include_directories(liberty2json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                               ${PROJECT_SOURCE_DIR}/third_party)  
target_link_libraries(liberty2json PUBLIC Backward::Interface)

###############################################################################
# Executable target for syntform
###############################################################################
add_executable(syntform ${CMAKE_CURRENT_SOURCE_DIR}/src/syntform.c ${LIB_SOURCES} ${LIB_HEADERS} ${FLEX_BISON_SOURCES})
target_include_directories(syntform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
