#version 330

//A simple shader for rendering shadow maps for instanced geometry

uniform mat4 view_proj;

layout(location = 0) in vec3 position;
layout(location = 3) in mat4 model;

void main(void){
	gl_Position = view_proj * model * vec4(position, 1.f);
}

