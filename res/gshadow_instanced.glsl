#version 430

/*
 * Geometry shader for generating geometry for rendering to each face of
 * a cube map for shadow mapping. Shader is invoked per face and emits the primitive
 * transformed with the faces view/projection matrix. Incoming geometry should be
 * in world space with the point light at the origin. The instance id is the face to write
 * to using the same indices that OpenGL uses (ie. 0 = pos x, 1 = neg x, etc)
 * Perhaps later do some culling and only emit triangles that are visible to that face?
 * I suppose to do this for GL 3.3 I'd have to do it w/o geometry shader instancing somehow.
 */

layout(triangles, invocations=6) in;
layout(triangle_strip, max_vertices=3) out;

layout(std140) uniform PointLight {
	//Matrix to translate point light to origin
	mat4 model;
	mat4 proj;
	//The viewing matrices for pos/neg x/y/z faces
	mat4 view[6];
} GS_PointLight;

void main(void){
	for (int i = 0; i < gl_in.length(); ++i){
		gl_Layer = gl_InvocationID;
		gl_Position = GS_PointLight.proj * GS_PointLight.view[gl_InvocationID] * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}

