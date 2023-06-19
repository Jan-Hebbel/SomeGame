#version 450

layout(binding = 0) uniform Uniform_Buffer_Object {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform Vertex_Constants {
	mat4 model;
	vec2 position;
	vec2 tex_coord;
} pc;

layout(location = 0) out vec2 frag_tex_coord;

void main()
{
	gl_Position = ubo.proj * ubo.view * pc.model * vec4(pc.position, 0.0, 1.0);
	frag_tex_coord = pc.tex_coord;
}