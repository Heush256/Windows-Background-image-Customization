cmake_minimum_required(VERSION 3.10)
project(Wondows_Wallpaper_Rotator)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(Wondows_Wallpaper_Rotator
#        CJson.c
#        Nmain.c
        cJSON-1.7.19/cJSON.c
#        CurrentWork.c
#        Attempt.c
        AttemptingAgain.c
)

target_include_directories(Wondows_Wallpaper_Rotator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cjson
)

if(WIN32)
    target_link_libraries(Wondows_Wallpaper_Rotator PRIVATE)
endif()