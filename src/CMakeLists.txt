set(ENGINE_SOURCES
  main.cpp
)

add_executable(my_app ${ENGINE_SOURCES})

target_include_directories(my_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(my_app PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  SDL_MAIN_HANDLED            
)

target_link_libraries(my_app PRIVATE
  glm
  vma
  stb_image
  glad
  SDL2::SDL2
  OpenGL::GL
  imgui
  fmt::fmt
  fastgltf
)
