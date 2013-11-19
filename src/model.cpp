#include <iostream>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "util.h"
#include "model.h"

Model::Model(const std::string &file, GLuint program, GLuint shadowProgram)
	: vao(0), nElems(0), program(program), shadowProgram(shadowProgram),
		matUnif(-1), shadowMatUnif(-1)
{
	load(file);
}
Model::~Model(){
	glDeleteBuffers(2, buf);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
	if (shadowProgram){
		glDeleteProgram(shadowProgram);
	}
}
void Model::bind(){
	glUseProgram(program);
	glBindVertexArray(vao);
}
void Model::bindShadow(){
	glUseProgram(shadowProgram);
	glBindVertexArray(vao);
}
void Model::setShadowVP(const glm::mat4 &vp){
	if (shadowProgram){
		glUseProgram(shadowProgram);
		GLuint vpUnif = glGetUniformLocation(shadowProgram, "view_proj");
		glUniformMatrix4fv(vpUnif, 1, GL_FALSE, glm::value_ptr(vp));
	}
}
size_t Model::elems(){
	return nElems;
}
void Model::translate(const glm::vec3 &vec){
	if (matUnif != -1){
		translation = glm::translate<GLfloat>(vec) * translation;
		updateMatrix();
	}
}
void Model::rotate(const glm::mat4 &rot){
	if (matUnif != -1){
		rotation = rot * rotation;
		updateMatrix();
	}
}
void Model::scale(const glm::vec3 &scale){
	if (matUnif != -1){
		scaling = glm::scale<GLfloat>(scale) * scaling;
		updateMatrix();
	}
}
void Model::load(const std::string &file){
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, buf);
	if (!util::loadOBJ(file, buf[0], buf[1], nElems)){
		std::cout << "Failed to load model: " << file << "\n";
		return;
	}
	//Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
	//Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT),
		(void*)(3 * sizeof(GL_FLOAT)));
	//UVs
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT),
		(void*)(6 * sizeof(GL_FLOAT)));
	//Send the identity for the initial model matrix, if that uniform is available
	glUseProgram(program);
	matUnif = glGetUniformLocation(program, "model");
	if (shadowProgram){
		glUseProgram(shadowProgram);
		shadowMatUnif = glGetUniformLocation(shadowProgram, "model");
	}
	if (matUnif != -1){
		translation = glm::translate<GLfloat>(0.f, 0.f, 0.f);
		rotation = glm::rotate<GLfloat>(0.f, 0.f, 1.f, 0.f);
		scaling = glm::scale<GLfloat>(1.f, 1.f, 1.f);
		updateMatrix();
	}
}
void Model::updateMatrix(){
	glUseProgram(program);
	glm::mat4 model = translation * rotation * scaling;
	glUniformMatrix4fv(matUnif, 1, GL_FALSE, glm::value_ptr(model));
	if (shadowProgram){
		glUseProgram(shadowProgram);
		glUniformMatrix4fv(shadowMatUnif, 1, GL_FALSE, glm::value_ptr(model));
	}
}

