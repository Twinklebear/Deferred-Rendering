#version 330

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D depth;
uniform sampler2DShadow shadow_map;
uniform mat4 inv_proj;
uniform mat4 inv_view;
uniform mat4 light_vp;
uniform vec4 light_dir;
uniform vec4 light_pos;
uniform vec4 view_pos;
uniform float spotlight_angle;

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
	vec4 world_pos = inv_view * compute_view_pos();
	vec4 n = texture(normal, f_uv);
	vec4 l = light_pos - world_pos;
	l.w = 0.f;
	l = normalize(l);
	vec4 v = view_pos - world_pos;
	v.w = 0.f;
	v = normalize(v);
	vec4 half_vect = normalize(l + v);
	n = n * 2.f - 1.f;
	n.w = 0.f;
	n = normalize(n);

	//Compute spotlight strength based on how far angle we are off from the max
	//spotlight angle
	float spotlight_str = floor(abs(dot(l, light_dir)) - spotlight_angle + .99f);
	float diff = spotlight_str * max(0.f, dot(n, l));
	float spec = spotlight_str * max(0.f, dot(n, half_vect));
	if (diff == 0.f){
		spec = 0.f;
	}
	else {
		//Just give everyone 50 shininess
		spec = pow(spec, 50.f);
	}
	//Check if we're in shadow
	vec4 shadow_pos = light_vp * world_pos;
	//Not needed for directional light but we do need to do the perspective
	//division for point lights (perspective proj.)
	shadow_pos /= shadow_pos.w;
	//Scale the depth values into projection space
	shadow_pos = (shadow_pos + 1.f) / 2.f;
	float f = textureProj(shadow_map, shadow_pos);
	//Apply some ambient as well and set light color to white
	//with a rather low strength
	vec3 scattered = vec3(0.2f, 0.2f, 0.2f) + f * diff;
	vec3 reflected = f * vec3(1.f * spec * 0.4f);
	color = texture(diffuse, f_uv);
	color.xyz = min(color.xyz * scattered + reflected, vec3(1.f));
}

