add_executable(Render main.cpp util.cpp)
target_link_libraries(Render ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})
install(TARGETS Render DESTINATION "${DeferredRenderer_SOURCE_DIR}/bin")

