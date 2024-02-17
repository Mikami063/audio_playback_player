cmake_minimum_required(VERSION 3.14)
project(audio_playback_player VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 14)

add_subdirectory(lib/FFmpeg)
add_executable(audio_playback_player main.cpp)

target_link_libraries(audio_playback_player FFmpeg)