#version 330

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D depth;
uniform mat4 inv_proj;
uniform vec4 light_dir;
uniform vec4 half_vect;

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
	vec4 n = texture(normal, f_uv);
	n = n * 2.f - 1.f;
	n.w = 0.f;
	float diff = max(0.f, dot(n, light_dir));
	float spec = max(0.f, dot(n, half_vect));
	if (diff == 0.f){
		spec = 0.f;
	}
	else {
		//Just give everyone 50 shininess
		spec = pow(spec, 50.f);
	}
	//Apply some ambient as well and set light color to white
	//with a rather low strength
	vec3 scattered = vec3(0.2f, 0.2f, 0.2f) + 1.f * diff;
	vec3 reflected = vec3(1.f * spec * 0.4f);
	color = texture(diffuse, f_uv);
	color.xyz = min(color.xyz * scattered + reflected, vec3(1.f));
}

