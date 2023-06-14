#version 450

layout(binding = 1) uniform sampler2D tex_sampler[16];

layout(push_constant) uniform Fragment_Constants {
	layout(offset = 64) int texture_index;
} pc;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = texture(tex_sampler[pc.texture_index], frag_tex_coord);
}