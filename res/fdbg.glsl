#version 330 core

//0 is diffuse, 1 is depth
uniform samplerCube cube_maps[2];
uniform int map;

in vec3 f_uv;

out vec4 color;

float linearize(float d){
	return (2.f * d - gl_DepthRange.near - gl_DepthRange.far)
		/ (gl_DepthRange.far - gl_DepthRange.near);
}

void main(void){
	if (map == 0){
		color = texture(cube_maps[0], f_uv);
	}
	else {
		float d = linearize(texture(cube_maps[1], f_uv).x);
		color = vec4(d, d, d, 1.f);
	}
}

