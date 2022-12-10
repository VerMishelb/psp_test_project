cmake_minimum_required(VERSION 3.10)

project(psp-cmake-test
	VERSION 0.1
	DESCRIPTION "CMake PSP test project"
	LANGUAGES CXX)

include_directories(include)
add_subdirectory(src)
