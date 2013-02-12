/*
**  ClanLib SDK
**  Copyright (c) 1997-2012 The ClanLib Team
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

#include "shader_color.h"

char ShaderColor::vertex[] =
	"\n"
	"#version 120\n"
	"\n"
	"attribute vec3 InPosition;"
	"attribute vec3 InNormal;"
	"uniform mat4 cl_ModelViewMatrix;"
	"uniform mat4 cl_ModelViewProjectionMatrix;"
	"uniform mat3 cl_NormalMatrix;"
	"\n"
	"varying vec3 WorldSpaceNormal; \n"
	"varying vec3 WorldSpacePosition; \n"
	"varying vec4 ObjPos;\n"
	"varying vec4 MaterialAmbient;\n"
	"\n"
	"void main()\n"
	"{\n"
	"	vec4 in_position = vec4(InPosition.xyz, 1.0);\n"
	"	gl_Position = cl_ModelViewProjectionMatrix * in_position;\n"
	"	WorldSpaceNormal = normalize( cl_NormalMatrix * InNormal);\n"
	"	WorldSpacePosition = InPosition;\n"
	"	ObjPos = cl_ModelViewMatrix * in_position;\n"
	"}\n"
	;

char ShaderColor::fragment[] =
	"\n"
	"#version 120\n"
	"\n"
	"varying vec3 WorldSpaceNormal; \n"
	"varying vec3 WorldSpacePosition; \n"
	"varying vec4 ObjPos;\n"
	"\n"
	"uniform float MaterialShininess;\n"
	"uniform vec4 MaterialEmission;\n"
	"uniform vec4 MaterialSpecular;\n"
	"uniform vec4 MaterialAmbient;\n"
	"\n"
	"uniform vec3 LightVector;\n"
	"uniform vec3 LightHalfVector;\n"
	"uniform vec4 LightSpecular;\n"
	"uniform vec4 LightDiffuse;\n"
	"uniform vec4 LightAmbient;\n"
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
	"	vec4 final_texture_color = vec4(MaterialAmbient.rgb,1.0);\n"
	"	gl_FragColor = LightAmbient * final_texture_color + (diff + MaterialEmission) * final_texture_color +spec * MaterialSpecular;\n"
	"	gl_FragColor.a = MaterialAmbient.a;\n"
	"}\n"
	;

ShaderColor::ShaderColor(GraphicContext &gc)
{
	ShaderObject vertex_shader(gc, shadertype_vertex, vertex);
	if(!vertex_shader.compile())
	{
		throw Exception(string_format("Unable to compile vertex shader object: %1", vertex_shader.get_info_log()));
	}

	ShaderObject fragment_shader(gc, shadertype_fragment, fragment);
	if(!fragment_shader.compile())
	{
		throw Exception(string_format("Unable to compile fragment shader object: %1", fragment_shader.get_info_log()));
	}

	program_object = ProgramObject(gc);
	program_object.attach(vertex_shader);
	program_object.attach(fragment_shader);
	program_object.bind_attribute_location(0, "InPosition");
	program_object.bind_attribute_location(1, "InNormal");
	if (!program_object.link())
	{
		throw Exception(string_format("Unable to link program object: %1", program_object.get_info_log()));
	}


	material_updated = false;
	light_updated = false;

	material_shininess = 64.0f;
	material_emission = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	material_ambient =  Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
	material_specular = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);

	light_ambient = Vec4f(0.2f, 0.2f, 0.2f, 1.0f);
	light_vector = Vec3f(0.0f, 0.0f, 1.0f);
	light_specular = Vec4f(0.7f, 0.7f, 0.7f, 1.0f);
	light_diffuse = Vec4f(0.7f, 0.7f, 0.7f, 1.0f);


}

void ShaderColor::Use(GraphicContext &gc)
{
	if (!material_updated)
	{
		material_updated = true;
		program_object.set_uniform1f("MaterialShininess", material_shininess);
		program_object.set_uniform4f("MaterialEmission", material_emission);
		program_object.set_uniform4f("MaterialSpecular", material_specular);
		program_object.set_uniform4f("MaterialAmbient", material_ambient);
	}

	if (!light_updated)
	{
		light_updated = true;
		program_object.set_uniform3f("LightVector", light_vector);
		Vec3f light_halfvector(0.0f, 0.0f, 1.0f);
		light_halfvector += light_vector;
		light_halfvector.normalize();
		program_object.set_uniform3f("LightHalfVector", light_halfvector);
		program_object.set_uniform4f("LightSpecular", light_specular);
		program_object.set_uniform4f("LightDiffuse", light_diffuse);
		program_object.set_uniform4f("LightAmbient", light_ambient);
	}

	gc.set_program_object(program_object);
}

void ShaderColor::SetMaterial(float new_material_shininess, const Vec4f &new_material_emission, const Vec4f &new_material_ambient, const Vec4f &new_material_specular)
{
	if (new_material_shininess != material_shininess)
	{
		material_updated = false;
		material_shininess = new_material_shininess;
	}

	if (new_material_emission != material_emission)
	{
		material_updated = false;
		material_emission = new_material_emission;
	}

	if (new_material_ambient != material_ambient)
	{
		material_updated = false;
		material_ambient = new_material_ambient;
	}

	if (new_material_specular != material_specular)
	{
		material_updated = false;
		material_specular = new_material_specular;
	}
}

void ShaderColor::SetLight(Vec3f &new_light_vector, Vec4f &new_light_specular, Vec4f &new_light_diffuse, Vec4f &new_light_ambient)
{
	if (new_light_vector != light_vector)
	{
		light_updated = false;
		light_vector = new_light_vector;
	}
	if (new_light_specular != light_specular)
	{
		light_updated = false;
		light_specular = new_light_specular;
	}
	if (new_light_diffuse != light_diffuse)
	{
		light_updated = false;
		light_diffuse = new_light_diffuse;
	}
	if (new_light_ambient != light_ambient)
	{
		light_updated = false;
		light_ambient = new_light_ambient;
	}
}

