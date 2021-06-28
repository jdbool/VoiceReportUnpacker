cmake_minimum_required (VERSION 3.8)

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

add_executable (VoiceReportUnpacker main.cpp)

set_property (TARGET VoiceReportUnpacker PROPERTY CXX_STANDARD 17)

target_link_libraries (VoiceReportUnpacker Threads::Threads)
target_link_libraries (VoiceReportUnpacker ${CMAKE_SOURCE_DIR}/opus/win32/VS2015/x64/Release/opus.lib)

include_directories (${CMAKE_SOURCE_DIR}/opus/include)