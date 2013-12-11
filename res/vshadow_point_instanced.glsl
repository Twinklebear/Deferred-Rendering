#version 430

/*
 * Shadow for translating vertices into world space where the light is
 * at the origin for the successive cube shadow mapping stages
 */

layout(std140) uniform PointLight {
	//Matrix to translate point light to origin
	mat4 model;
	mat4 proj;
	//The viewing matrices for pos/neg x/y/z faces
	mat4 view[6];
} VS_PointLight;

layout(location = 0) in vec3 position;
layout(location = 3) in mat4 model;

void main(void){
	gl_Position = VS_PointLight.model * model * vec4(position, 1.f);
}

