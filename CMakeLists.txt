cmake_minimum_required(VERSION 3.5)
project(RationaLP)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS program_options REQUIRED)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package(GMP REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS})
link_directories(${Boost_LIBRARIES_DIR} ${GMP_LIBARIES_DIR})


set(SOURCE_FILES main.cpp LinearProgram.h Tableau.h Pivot.h)
add_executable(RationaLP ${SOURCE_FILES})
target_link_libraries(RationaLP ${GMP_LIBRARIES} ${Boost_LIBRARIES})
#target_compile_features(RationaLP PUBLIC cxx_std_17)
