#version 130

uniform mat4 model;
uniform mat4 proj;

in vec3 position;
in vec3 normal;

out vec3 f_color;

void main(){
	gl_Position = proj * model * vec4(position, 1.f);
	f_color = abs(normal);
}
