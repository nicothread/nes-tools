cmake_minimum_required(VERSION 4.3)
project(nes_tools C)

include_directories(include)

# Find and link PNG library
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# Create executable for png2chr
add_executable(png2chr
        src/png2chr.c
)

# Create executable for png2chr
add_executable(chr2png
        src/chr2png.c
)

target_link_libraries(png2chr ${PNG_LIBRARIES})

# Create executable for ttf2png
add_executable(ttf2png
        src/ttf2png.c
)

target_link_libraries(ttf2png ${PNG_LIBRARIES})
target_link_libraries(ttf2png Freetype::Freetype)