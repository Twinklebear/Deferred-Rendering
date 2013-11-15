#version 330

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D depth;

in vec2 f_uv;

out vec4 color;

//Linearize the depth value, assumes near is 0.1 and far is 100
float linearize(float depth){
	float zNear = 0.1f;
	float zFar = 100.f;
	return 2.f * zNear / (zFar + zNear - depth * (zFar - zNear));
}

void main(void){
	color = texture(diffuse, f_uv);
	//color = texture(normal, f_uv);
	float d = linearize(texture(depth, f_uv).x);
	//color = vec4(d, d, d, 1.f);
}

