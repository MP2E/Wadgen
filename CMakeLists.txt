cmake_minimum_required(VERSION 2.6)
project(WadGen)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIR})

add_executable(WadGen
    src/deflate-N64.c
    src/files.c
    src/gfx.c
    src/level.c
    src/mem.c
    src/png.c
    src/MD5.c
    src/rom.c
    src/sndfont.c
    src/sound.c
    src/sprite.c
    src/texture.c
    src/wad.c
    src/wadgen.c
)

if(WIN32)
    target_link_libraries(WadGen ${PNG_LIBRARY} ${ZLIB_LIBRARY} comctl32)
else()
    target_link_libraries(WadGen ${PNG_LIBRARY} ${ZLIB_LIBRARY} m)
endif()
