cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_STANDARD 17)
project(cloudphoto LANGUAGES CXX)
add_definitions(-Wall -O3)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_SHARED_LIBS ON CACHE STRING "Link to shared libraries by default.")

find_package(AWSSDK COMPONENTS s3 QUIET)
  if(NOT AWSSDK_FOUND)
    message(STATUS "Downloading and building AWS SDK dependency")
    FetchContent_Declare(AWSSDK
      GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp
      GIT_TAG eb196d341ee70368d84c4231a7b2c08abf0cca00
      GIT_PROGRESS TRUE
    )
    set(BUILD_ONLY "s3")
    set(ENABLE_TESTING OFF)
    FetchContent_MakeAvailable(AWSSDK)
  endif()

include_directories(include/)

file(GLOB modules ${AWSSDK_SOURCE_DIR}/*)
foreach(module ${modules})
  if(IS_DIRECTORY ${module})
    include_directories("${module}/include")
  endif()
endforeach()
include_directories("${AWSSDK_SOURCE_DIR}/crt/aws-crt-cpp/include")
include_directories("${AWSSDK_SOURCE_DIR}/crt/aws-crt-cpp/crt/aws-c-common/verification/cbmc/include")
file(GLOB crt_modules ${AWSSDK_SOURCE_DIR}/crt/aws-crt-cpp/crt/*)
foreach(crt_module ${crt_modules})
  if(IS_DIRECTORY ${crt_module})
    include_directories("${crt_module}/include")
  endif()
endforeach()
add_executable(${PROJECT_NAME} "main.cxx")
install(TARGETS ${PROJECT_NAME})

file(COPY resources DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
# target_link_libraries(${PROJECT_NAME} ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(${PROJECT_NAME} AWS::aws-cpp-sdk-s3 AWS::aws-cpp-sdk-core)
