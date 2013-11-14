#ifndef UTIL_H
#define UTIL_H

#include <array>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace util {
	/*
	* Read the entire contents of a file into a string, if an error occurs
	* the string will be empty
	*/
	std::string readFile(const std::string &fName);
	/*
	 * Load a GLSL shader from some file, returns -1 if loading failed
	 */
	GLint loadShader(const std::string &file, GLenum type);
	/*
	 * Load a simple shader program using the vertex and fragment
	 * shaders in the files passed
	 * returns -1 if loading failed
	 */
	GLint loadProgram(const std::string &vertfname, const std::string &fragfname);
	/*
	 * Check for an OpenGL error and log it along with the message passed
	 * if an error occured. Will return true if an error occured & was logged
	 */
	bool logGLError(const std::string &msg);
	/*
	 * A debug callback for the GL_ARB_debug_out extension
	 */
	void glDebugCallback(GLenum src, GLenum type, GLuint id, GLenum severity,
		GLsizei len, const GLchar *msg, GLvoid *user);
	/*
	* Load an OBJ model file into the vbo and ebo passed in
	* The model must have vertex, texture and normal data and be a triangle mesh
	* vbo & ebo should be already created and will be filled with the model data
	* vbo should be GL_ARRAY_BUFFER and ebo should be GL_ELEMENT_ARRAY_BUFFER, data will
	* be written with GL_STATIC_DRAW, the element indices will be stored as GL_UNSIGNED_SHORT
	* The vbo will be packed: vec3 pos, vec3 normal, vec3 uv
	* returns true on success, false on failure
	*/
	bool loadOBJ(const std::string &fName, GLuint &vbo, GLuint &ebo, size_t &nElems);
	/*
	* Functions to get values from formatted strings, for use in reading the
	* model file
	*/
	glm::vec2 captureVec2(const std::string &str);
	glm::vec3 captureVec3(const std::string &str);
	/*
	* Get the vertex informaton for a face as an array of 3 strings
	*/
	std::array<std::string, 3> captureFaces(const std::string &str);
	/*
	* Capture the indices of the vertex components, the array contains the
	* face information: v/vt/vn
	*/
	std::array<unsigned int, 3> captureVertex(const std::string &str);
}

#endif
