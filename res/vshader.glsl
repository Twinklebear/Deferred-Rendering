#version 330

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

out vec4 f_normal;
out vec2 f_uv;

void main(void){
	mat4 mv = view * model;
	gl_Position = proj * mv * vec4(position, 1.f);
	f_normal = normalize(mv * vec4(normal, 0.f));
	f_uv = uv;
}
