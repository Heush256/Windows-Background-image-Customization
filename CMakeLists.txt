cmake_minimum_required(VERSION 3.10)
project(Wondows_Wallpaper_Rotator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(Wondows_Wallpaper_Rotator
        cJSON-1.7.19/cJSON.c
        miniaudio-0.11.25/miniaudio.c
        AttemptingAgain.c
)

target_include_directories(Wondows_Wallpaper_Rotator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cjson
        ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio-0.11.25
)

if(WIN32)
    # winmm is required for miniaudio to talk to the Windows audio drivers
    target_link_libraries(Wondows_Wallpaper_Rotator PRIVATE winmm)
endif()