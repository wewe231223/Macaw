cmake_minimum_required(VERSION 3.20)
project(imgui-neo-sequencer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(imgui CONFIG REQUIRED)

add_library(imgui-neo-sequencer STATIC imgui_neo_internal.cpp imgui_neo_sequencer.cpp)
target_link_libraries(imgui-neo-sequencer PRIVATE imgui::imgui)

if(MSVC)
    target_compile_options(imgui-neo-sequencer PRIVATE /utf-8)
endif()

install(TARGETS imgui-neo-sequencer ARCHIVE DESTINATION lib)
install(FILES imgui_neo_sequencer.h DESTINATION include)
