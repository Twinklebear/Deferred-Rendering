#version 330

uniform sampler2D tex_diffuse;
uniform vec4 light_dir;
uniform vec4 half_vect;

in vec4 f_normal;
in vec2 f_uv;

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 normal;

void main(void){
	//This is a forward rendering test w/ directional light
	//so that we can compare with the deferred lighting results
	float diff = max(0.f, dot(f_normal, light_dir));
	float spec = max(0.f, dot(f_normal, half_vect));
	if (diff == 0.f){
		spec = 0.f;
	}
	else {
		//Apply shininess coeffecient
		spec = pow(spec, 50.f);
	}
	//Apply some ambient as well, and set light color to white
	vec3 scattered = vec3(0.2f, 0.2f, 0.2f) + 1.f * diff;
	//Light is white with low strength
	vec3 reflected = vec3(1.f * spec * 0.4f);
	diffuse = texture(tex_diffuse, f_uv);
	diffuse.xyz = min(diffuse.xyz * scattered + reflected, vec3(1.f));
	//Scale the normal values from [-1,1] range to [0,1] range
	normal = ((f_normal + 1.f) / 2.f);
	normal.w = 0.f;
}

