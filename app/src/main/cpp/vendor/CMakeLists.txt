project(Vendor)
include(FetchContent)

# Dobby — use submodule if present, otherwise fetch
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Dobby/CMakeLists.txt")
    add_subdirectory(Dobby)
else()
    FetchContent_Declare(Dobby
        GIT_REPOSITORY https://github.com/jmpews/Dobby
        GIT_TAG        85ed4c2df9dddc4a46fac64a1b42f9e644d584b2
    )
    FetchContent_MakeAvailable(Dobby)
endif()

# imgui — expose IMGUI_DIR for ModMenu; fetch if submodule is missing
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h")
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" CACHE PATH "imgui source dir" FORCE)
else()
    FetchContent_Declare(imgui_content
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG        e57871bb95faec757e51214bc0e1ae29b13258ab
    )
    FetchContent_MakeAvailable(imgui_content)
    set(IMGUI_DIR "${imgui_content_SOURCE_DIR}" CACHE PATH "imgui source dir" FORCE)
endif()
