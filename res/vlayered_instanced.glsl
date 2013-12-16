#version 330 core

//A simple shader for rendering shadow maps w/ instanced geometry and
//a geometry shader pass

layout(location = 0) in vec3 position;
layout(location = 3) in mat4 model;

flat out int vcolor_idx;

void main(void){
	gl_Position = model * vec4(position, 1.f);
	vcolor_idx = gl_InstanceID;
}

