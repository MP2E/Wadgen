cmake_minimum_required(VERSION 2.6)
project(WadGen)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIR})

add_executable(WadGen
	DeflateN64.c
	Files.c
	Gfx.c
	Level.c
	Mem.c
	Png.c
	MD5.c
	Rom.c
	SndFont.c
	Sound.c
	Sprite.c
	Texture.c
	Wad.c
	WadGen.c
)

target_link_libraries(WadGen -lm ${ZLIB_LIBRARY} ${PNG_LIBRARY})
if(WIN32)
	target_link_libraries(WadGen comctl32)
endif(WIN32)
