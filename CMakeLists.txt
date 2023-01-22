cmake_minimum_required(VERSION 3.13)
project(vcxproj VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # disables warning about DLL export 
  # this is annoying when using C++ STL
  add_compile_options(/wd4251)  
endif()

set(VCXPROJ_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

list(APPEND CMAKE_PREFIX_PATH "C:/usr")

##################################################################
###### coverage build
##################################################################

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

##################################################################
###### dependencies
##################################################################

find_package(tinyxml2)

##################################################################
###### vcxproj library
##################################################################

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)

add_library(vcxproj STATIC ${SRC_FILES} ${HDR_FILES})
target_include_directories(vcxproj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(vcxproj tinyxml2)

set_target_properties(vcxproj PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(vcxproj PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message("GCC compiler: linking to stdc++fs")
    target_link_libraries(vcxproj stdc++fs)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

  if (CLANG_VERSION_STRING VERSION_GREATER_EQUAL 9.0)
    message("Clang version greater than 9.0, no need to link to c++fs")
  else()
    message("Clang compiler: linking to c++fs")
    target_link_libraries(vcxproj c++fs)
  endif()
    
  message("See https://releases.llvm.org/10.0.0/projects/libcxx/docs/UsingLibcxx.html#using-filesystem for information about using #include<filesystem> with clang")

endif()

foreach(_source IN ITEMS ${SRC_FILES} ${HDR_FILES})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

##################################################################
###### tests
##################################################################

#add_subdirectory(tests)

##################################################################
###### virtual env
##################################################################

file(WRITE "${CMAKE_BINARY_DIR}/activate.bat" "set PATH=${TINYXML2_DIR}/bin;%PATH%")


