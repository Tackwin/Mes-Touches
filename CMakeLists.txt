cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(Mes_Touches VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/res)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
# Second, for multi-config builds (e.g. msvc)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
 
link_directories(${CMAKE_SOURCE_DIR})

find_package(GLEW REQUIRED)

add_library(win_hook SHARED ${CMAKE_SOURCE_DIR}/src/cbt_hook.cpp)

add_executable(Mes_Touches WIN32
	${CMAKE_SOURCE_DIR}/src/Main.cpp
	${CMAKE_SOURCE_DIR}/src/Common.cpp

	${CMAKE_SOURCE_DIR}/src/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/imgui_ext.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/imgui_impl_opengl3.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/imgui_impl_win32.cpp
	${CMAKE_SOURCE_DIR}/src/imgui/imgui_widgets.cpp

	${CMAKE_SOURCE_DIR}/src/keyboard.cpp
	${CMAKE_SOURCE_DIR}/src/Event.cpp
	${CMAKE_SOURCE_DIR}/src/Logs.cpp
	${CMAKE_SOURCE_DIR}/src/Mouse.cpp
	${CMAKE_SOURCE_DIR}/src/render_stats.cpp
	${CMAKE_SOURCE_DIR}/src/Settings.cpp
	${CMAKE_SOURCE_DIR}/src/TimeInfo.cpp

	${CMAKE_SOURCE_DIR}/src/OS/win/FileInfo.cpp
	${CMAKE_SOURCE_DIR}/src/File_Win.cpp
	${CMAKE_SOURCE_DIR}/src/ErrorCode_Win.cpp
	${CMAKE_SOURCE_DIR}/src/Screen_Win.cpp
	${CMAKE_SOURCE_DIR}/src/NotifyIcon.cpp
	${CMAKE_SOURCE_DIR}/src/Mes_Touches.rc
)

target_link_libraries(Mes_Touches PUBLIC win_hook)
target_link_libraries(Mes_Touches PUBLIC version.lib)
target_link_libraries(Mes_Touches PRIVATE GLEW::GLEW)
set_property(TARGET Mes_Touches PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")