set(CMAKE_MODULE_PATH "/usr/local/lib/cmake/${CMAKE_MODULE_PATH}")

cmake_minimum_required (VERSION 2.8)
project (AutoDimplePi)
find_package(raspicam REQUIRED)
find_package(OpenCV REQUIRED)
find_package(wiringPi REQUIRED)

add_executable(autodimplepi AutoDimplePi.cpp)
target_link_libraries(autodimplepi ${raspicam_CV_LIBS})
target_link_libraries(autodimplepi wiringPi)
add_executable(algorithmtest AlgorithmTest.cpp)
target_link_libraries(algorithmtest ${raspicam_CV_LIBS})
target_link_libraries(algorithmtest wiringPi)

SET(CMAKE_C_COMPILER /usr/bin/gcc)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

