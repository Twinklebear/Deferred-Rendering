#version 330

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D depth;
uniform mat4 inv_proj;
uniform vec2 depth_range;

in vec2 f_uv;
in vec2 ndc_pos;

out vec4 color;

//Linearize the depth value, assumes near is 0.1 and far is 100
float linearize(float depth){
	return 2.f * depth_range.x
		/ (depth_range.y + depth_range.x - depth * (depth_range.y - depth_range.x));
}

void main(void){
	float d = linearize(texture(depth, f_uv).x);
	vec4 view_pos = inv_proj * vec4(ndc_pos, d, 1.f);
	color = view_pos;
}

