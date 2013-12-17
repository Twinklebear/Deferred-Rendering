#version 330 core

//A simple shader for rendering instanced geometry
uniform mat4 proj;
uniform mat4 view;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in mat4 model;

out vec4 world_pos;
out vec4 f_normal;

void main(void){
	world_pos = model * vec4(position, 1.f);
	gl_Position = proj * view * world_pos;
	f_normal = normalize(model * vec4(normal, 0.f));
}

