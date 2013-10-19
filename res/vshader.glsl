#version 330

uniform mat4 model;
uniform mat4 proj;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

out vec3 f_color;
out vec2 f_uv;

void main(){
	gl_Position = proj * model * vec4(position, 1.f);
	f_color = abs(normal);
	f_uv = uv;
}
