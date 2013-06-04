/*
**  ClanLib SDK
**  Copyright (c) 1997-2013 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Mark Page
*/

#include "precomp.h"

#include "shader_color_instanced.h"

const char ShaderColorInstanced::vertex_hlsl[] = "FIXME;\n";
const char ShaderColorInstanced::fragment_hlsl[] = "FIXME;\n";

const char ShaderColorInstanced::vertex_glsl[] =
	"\n"
	"#version 150\n"
	"\n"
	"in vec3 InPosition;"
	"in vec3 InNormal;"
	"layout (std140) uniform ProgramUniforms\n"
	"{\n"
	"	vec4 MaterialAmbientSpecular[256];\n"
	"	vec4 Centers[256];\n"
	"	mat4 cl_ModelViewMatrix;\n"
	"	mat4 cl_ModelViewProjectionMatrix;\n"
	"	mat3 cl_NormalMatrix;\n"
	"	vec4 padding;\n"
	"	vec4 MaterialEmission;\n"
	//"	vec4 padding2;\n"
	//"	vec4 padding3;\n"
	"	vec4 LightSpecular;\n"
	"	vec4 LightDiffuse;\n"
	"	vec4 LightAmbient;\n"
	"	vec3 LightVector;\n"
	"	vec3 LightHalfVector;\n"
	"	float MaterialShininess;\n"
	"};\n"
	"out vec3 WorldSpaceNormal; \n"
	"out vec3 WorldSpacePosition; \n"
	"out vec4 ObjPos;\n"
	"flat out vec4 xMaterialAmbientSpecular;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec4 in_position = vec4(InPosition.xyz, 1.0);\n"
	"	in_position.xyz += Centers[gl_InstanceID].xyz;\n"
	"	gl_Position = cl_ModelViewProjectionMatrix * in_position;\n"
	"	WorldSpaceNormal = normalize( cl_NormalMatrix * InNormal);\n"
	"	WorldSpacePosition = InPosition;\n"
	"	ObjPos = cl_ModelViewMatrix * in_position;\n"
	"	xMaterialAmbientSpecular = MaterialAmbientSpecular[gl_InstanceID];\n"
	"}\n"
	;

const char ShaderColorInstanced::fragment_glsl[] =
	"\n"
	"#version 150\n"
	"\n"
	"in vec3 WorldSpaceNormal; \n"
	"in vec3 WorldSpacePosition; \n"
	"in vec4 ObjPos;\n"
	"flat in vec4 xMaterialAmbientSpecular;\n"
	"out vec4 cl_FragColor;\n"
	"layout (std140) uniform ProgramUniforms\n"
	"{\n"
	"	vec4 MaterialAmbientSpecular[256];\n"
	"	vec4 Centers[256];\n"
	"	mat4 cl_ModelViewMatrix;\n"
	"	mat4 cl_ModelViewProjectionMatrix;\n"
	"	mat3 cl_NormalMatrix;\n"
	"	vec4 padding;\n"
	"	vec4 MaterialEmission;\n"
	//"	vec4 padding2;\n"
	//"	vec4 padding3;\n"
	"	vec4 LightSpecular;\n"
	"	vec4 LightDiffuse;\n"
	"	vec4 LightAmbient;\n"
	"	vec3 LightVector;\n"
	"	vec3 LightHalfVector;\n"
	"	float MaterialShininess;\n"
	"};\n"
 	"\n"
	"void main()\n"
	"{\n"
	"	vec3 eye = -normalize(ObjPos.xyz); \n"
	"	vec4 diff = vec4(0); \n"
	"	vec4 spec = vec4(0); \n"
	"\n"
	"	vec3 world_space_normal = normalize(WorldSpaceNormal);\n"
	"	float nDotL = max(0.0, dot(world_space_normal, LightVector)); \n"
	"	float pf; \n"
	"	if (nDotL == 0.0)\n"
	"	{\n"
	"		pf = 0.0; \n"
	"	}else\n"
	"	{\n"
	"			float nDotHV = max(0.0, dot(world_space_normal, LightHalfVector));\n"
	"			pf = pow(nDotHV, MaterialShininess);\n"
	"	}\n"
	"	spec += LightSpecular * pf; \n"
	"	diff += LightDiffuse * nDotL;\n"
	"	vec4 final_texture_color = vec4(xMaterialAmbientSpecular.rgb,1.0);\n"
	"	cl_FragColor = LightAmbient * final_texture_color + (diff + MaterialEmission) * final_texture_color +spec * xMaterialAmbientSpecular;\n"
	"	cl_FragColor.a = xMaterialAmbientSpecular.a;\n"
	"}\n"
	;

ShaderColorInstanced::ShaderColorInstanced(GraphicContext &gc)
{
	ShaderLanguage shader_language = gc.get_shader_language();
	
	ShaderObject vertex_shader(gc, shadertype_vertex, shader_language==shader_glsl ? vertex_glsl : vertex_hlsl);
	if(!vertex_shader.compile())
	{
		std::string log = vertex_shader.get_info_log();
		throw Exception(string_format("Unable to compile vertex shader object: %1", log));
	}

	ShaderObject fragment_shader(gc, shadertype_fragment, shader_language==shader_glsl ? fragment_glsl : fragment_hlsl);
	if(!fragment_shader.compile())
	{
		std::string log = fragment_shader.get_info_log();
		throw Exception(string_format("Unable to compile fragment shader object: %1", log));
	}

	program_object = ProgramObject(gc);
	program_object.attach(vertex_shader);
	program_object.attach(fragment_shader);
	program_object.bind_attribute_location(0, "InPosition");
	program_object.bind_attribute_location(1, "InNormal");
	program_object.bind_frag_data_location(0, "cl_FragColor");
	if (!program_object.link())
	{
		throw Exception(string_format("Unable to link program object: %1", program_object.get_info_log()));
	}

	// Uniforms
	program_object.set_uniform_buffer_index("ProgramUniforms", 0);

	gpu_uniforms = clan::UniformVector<ProgramUniforms>(gc, 1);

	uniforms.MaterialShininess = 64.0f;
	uniforms.MaterialEmission = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	uniforms.LightAmbient = Vec4f(0.2f, 0.2f, 0.2f, 1.0f);
	uniforms.LightVector = Vec3f(0.0f, 0.0f, 1.0f);
	uniforms.LightSpecular = Vec4f(0.7f, 0.7f, 0.7f, 1.0f);
	uniforms.LightDiffuse = Vec4f(0.7f, 0.7f, 0.7f, 1.0f);

}

void ShaderColorInstanced::SetCenters(std::vector<Vec3f> &centers)
{
	if (centers.size() > max_instances)
		throw Exception("Too many instances for this example");

	for (unsigned cnt=0; cnt<centers.size(); cnt++)
	{
		uniforms.Centers[cnt] = Vec4f(centers[cnt]);
	}
}

void ShaderColorInstanced::SetMaterialAmbientSpecular(std::vector<Vec4f> &colors)
{
	if (colors.size() > max_instances)
		throw Exception("Too many instances for this example");

	memcpy( &uniforms.MaterialAmbientSpecular[0].r, &colors[0].r, colors.size() * sizeof(Vec4f));
}

void ShaderColorInstanced::Use(GraphicContext &gc, const Mat4f &matrix_modelview, const Mat4f &matrix_modelview_projection, const Mat4f &matrix_normal)
{
	uniforms.cl_ModelViewProjectionMatrix = matrix_modelview_projection;
	uniforms.cl_ModelViewMatrix = matrix_modelview;
	uniforms.cl_NormalMatrix = matrix_normal;
	gpu_uniforms.upload_data(gc, &uniforms, 1);
	gc.set_uniform_buffer(0, gpu_uniforms);
	gc.set_program_object(program_object);
}

void ShaderColorInstanced::SetMaterial(float new_material_shininess, const Vec4f &new_material_emission)
{
	uniforms.MaterialShininess = new_material_shininess;
	uniforms.MaterialEmission = new_material_emission;

}

void ShaderColorInstanced::SetLight(Vec3f &new_light_vector, Vec4f &new_light_specular, Vec4f &new_light_diffuse, Vec4f &new_light_ambient)
{

	uniforms.LightAmbient = new_light_ambient;
	uniforms.LightVector = new_light_vector;
	uniforms.LightSpecular =new_light_specular;
	uniforms.LightDiffuse = new_light_diffuse;

	Vec3f light_halfvector(0.0f, 0.0f, 1.0f);
	light_halfvector += new_light_vector;
	light_halfvector.normalize();
	uniforms.LightHalfVector = light_halfvector;
}



