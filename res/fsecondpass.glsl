#version 330

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D depth;
uniform mat4 inv_proj;

in vec2 f_uv;

out vec4 color;

//Linearize the depth value passed in
float linearize(float d){
	return (2.f * d - gl_DepthRange.near - gl_DepthRange.far)
		/ (gl_DepthRange.far - gl_DepthRange.near);
}
/*
 * Reconstruct the view-space position, this relies on the fact that
 * the second pass draws the quad over the entire screen so the
 * uv coords map to our NDC coords
 */
vec4 compute_view_pos(void){
	float z = linearize(texture(depth, f_uv).x);
	vec4 pos = vec4(f_uv * 2.f - 1.f, z, 1.f);
	pos = inv_proj * pos;
	return pos / pos.w;
}


void main(void){
	color = texture(diffuse, f_uv);
}

