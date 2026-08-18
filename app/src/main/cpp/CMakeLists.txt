cmake_minimum_required(VERSION 3.18..3.25)
project(Main)

# vendor must come first so IMGUI_DIR is set before ModMenu uses it
add_subdirectory(vendor)
add_subdirectory(ModMenu)
add_subdirectory(src)
