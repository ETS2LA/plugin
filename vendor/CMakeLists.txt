add_subdirectory(fmt)

if(INCLUDE_GUI)
    message("Including ImGui and PolyHook")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    add_subdirectory(polyhook2)


    file(GLOB imgui-src imgui/*.cpp imgui/*.h)

    set(IMGUI_SOURCES
        ${imgui-src}
        imgui/backends/imgui_impl_opengl3.cpp
    )

    add_library(imgui STATIC
        ${IMGUI_SOURCES}
    )

    target_include_directories(imgui PRIVATE imgui)
endif()
