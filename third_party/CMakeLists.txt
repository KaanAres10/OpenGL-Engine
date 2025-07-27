find_package(OpenGL REQUIRED)

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_library(glad STATIC
  glad/src/glad.c
)
target_include_directories(glad PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
)
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/vma
)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/stb_image
)

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp

  imgui/backends/imgui_impl_sdl2.cpp
  imgui/backends/imgui_impl_opengl3.cpp	
  imgui/backends/imgui_impl_opengl3_loader.h
  imgui/ImGuizmo.cpp
)
target_include_directories(imgui PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_link_libraries(imgui PUBLIC
  SDL2::SDL2
  OpenGL::GL
  glad
)

add_subdirectory(fastgltf)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
