cmake_minimum_required(VERSION 2.6)
project(WadGen)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIR})

add_executable(WadGen
	src/DeflateN64.c
	src/Files.c
	src/Gfx.c
	src/Level.c
	src/Mem.c
	src/Png.c
	src/MD5.c
	src/Rom.c
	src/SndFont.c
	src/Sound.c
	src/Sprite.c
	src/Texture.c
	src/Wad.c
	src/WadGen.c
)

target_link_libraries(WadGen ${ZLIB_LIBRARY} ${PNG_LIBRARY})
if(WIN32)
	target_link_libraries(WadGen comctl32)
elseif()
	target_link_libraries(m)
endif()
