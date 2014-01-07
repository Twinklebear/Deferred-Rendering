#version 330 core

//Shader to perform parabolic mapping
const vec4 parabola_dirs[2] = vec4[](
	vec4(0, 0, 1, 0), vec4(0, 0, -1, 0)
);
//Quad vertices
const vec4 quad_verts[4] = vec4[](
	vec4(-1, -1, 0, 1),
	vec4(1, -1, 0, 1),
	vec4(-1, 1, 0, 1),
	vec4(1, 1, 0, 1)
);

uniform mat4 model;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

out vec4 f_normal;

void main(void){
	gl_Position = model * vec4(pos, 1);
	f_normal = vec4(normal, 0);
	gl_Position /= gl_Position.w;
	float len = length(gl_Position);
	gl_Position = normalize(gl_Position) + parabola_dirs[0];
	gl_Position.xy /= gl_Position.z;
	gl_Position.z = (len - 1) / (100 - 1);
	gl_Position.w = 1;
}

