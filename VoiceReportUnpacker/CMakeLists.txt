cmake_minimum_required (VERSION 3.8)

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

add_executable (VoiceReportUnpacker main.cpp)

set_property (TARGET VoiceReportUnpacker PROPERTY CXX_STANDARD 17)

target_link_libraries (VoiceReportUnpacker Threads::Threads)