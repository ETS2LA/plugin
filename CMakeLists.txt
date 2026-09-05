cmake_minimum_required(VERSION 3.15)

set(PLUGIN_VERSION_MAJOR 1)
set(PLUGIN_VERSION_MINOR 60)
set(PLUGIN_VERSION_PATCH 0)

set(GAME_VERSION_MAJOR 1)
set(GAME_VERSION_MINOR 60)
set(GAME_VERSION_PATCH x)

project(ets2la_plugin VERSION ${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_PATCH})

add_library(${CMAKE_PROJECT_NAME} SHARED)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ./src/core.cpp
    ./src/main.cpp

    ./src/memory/memory_utils.shared.cpp
    ./src/memory/virtual/memory_handler.cpp

    ./src/prism/common.cpp
    ./src/prism/game_actor.cpp
    ./src/prism/camera/camera_manager.cpp
    ./src/prism/controllers/base_ctrl.cpp
    ./src/prism/controllers/game_ctrl.cpp
    ./src/prism/navigation/gps_manager.cpp
    ./src/prism/traffic/game_traffic.cpp
    ./src/prism/vehicles/game_trailer_actor.cpp
    ./src/prism/vehicles/game_physics_vehicle.cpp
    ./src/prism/vehicles/vehicle_shared.cpp

    ./src/processing/traffic.cpp
    ./src/processing/mp_players.cpp
)

if (WIN32)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/memory/memory_utils.win32.cpp
        ./src/main_truckersmp.cpp
    )
elseif (UNIX AND NOT APPLE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/memory/memory_utils.linux.cpp
    )
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp
)

if (WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        ${CMAKE_CURRENT_SOURCE_DIR}/ets2la_plugin.def
    )

endif()

target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_17)

if (WIN32)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ETS2LA_PLUGIN_EXPORTS UNICODE _UNICODE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE vendor/truckersmp_sdk/include)
elseif(UNIX)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_DEBUG=1>)
    if (NOT APPLE)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE rt)
    endif()
endif()

add_subdirectory(vendor)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src vendor/scs_sdk/include)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt-header-only)
