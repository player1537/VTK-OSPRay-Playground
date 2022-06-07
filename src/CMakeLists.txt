cmake_minimum_required(VERSION 3.20)
project(vtkOSPRayExample)

find_package(ospray REQUIRED)
find_package(VTK REQUIRED)

add_executable(
    main
    main.cpp
)

target_link_libraries(
    main
    PUBLIC
        ospray::ospray
        ${VTK_LIBRARIES}
)
