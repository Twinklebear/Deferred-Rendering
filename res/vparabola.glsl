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

void main(void){
	gl_Position = model * quad_verts[gl_VertexID];
	gl_Position /= gl_Position.w;
	float len = length(gl_Position);
	gl_Position = normalize(gl_Position) + parabola_dirs[1];
	gl_Position.xy /= gl_Position.z;
	//near = 1, far = 100, shadow bias will be applied w/ PolygonOffset
	gl_Position.z = (len - 1) / (100 - 1);
	gl_Position.w = 1;
}

