# src/CMakeLists.txt

file(GLOB_RECURSE ENGINE_CPP
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE ENGINE_HDR
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(OpenGL-Engine
    ${ENGINE_CPP}
    ${ENGINE_HDR}  
)


target_include_directories(OpenGL-Engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(OpenGL-Engine PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  SDL_MAIN_HANDLED            
)

target_link_libraries(OpenGL-Engine PRIVATE
  glm
  vma
  stb_image
  glad
  SDL2::SDL2
  OpenGL::GL
  imgui
  fmt::fmt
  fastgltf
  assimp
)

file(GLOB_RECURSE SHADER_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
)

source_group(
    TREE "${CMAKE_SOURCE_DIR}/shaders"
    PREFIX "Shaders"
    FILES ${SHADER_FILES}
)

target_compile_definitions(OpenGL-Engine PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  SDL_MAIN_HANDLED
  SHADERS_DIR="${CMAKE_SOURCE_DIR}/shaders"
)

add_custom_target(Shaders SOURCES ${SHADER_FILES})
add_dependencies(OpenGL-Engine Shaders)