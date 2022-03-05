# this cmake lists compiles the project

cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     add_definitions("-std=c++17") 
# endif()

message("adding vcpkg")
set (CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(main VERSION 0.1.0)
message("main: ${CMAKE_SYSTEM_NAME}")
message("main: ${CMAKE_BUILD_TYPE}")
message("main: C: ${CMAKE_C_COMPILER_ID}")
message("main: CXX: ${CMAKE_CXX_COMPILER_ID}")

# add vcpkg packages
find_package(EnTT CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Add source files
file(GLOB_RECURSE SRC_FILES 
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(main
  ${SRC_FILES}
  # add icon to executable
  # ${CMAKE_CURRENT_BINARY_DIR}/assets/2d_game/textures/resource.rc
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

  # includes
  set (INCLUDES 
    ${INCLUDES} 
    ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/x64-windows/include
  )

  target_include_directories(main PRIVATE 
    ${INCLUDES}
    ${CMAKE_SOURCE_DIR}/src
  )

  # link libs
  target_link_libraries(main PRIVATE EnTT::EnTT)
  target_link_libraries(main PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)