/*
===========================================================================
Copyright (C) 2007-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

attribute vec4		attr_Position;
attribute vec4		attr_TexCoord0;
attribute vec3		attr_Tangent;
attribute vec3		attr_Binormal;
attribute vec3		attr_Normal;
attribute vec4		attr_Color;
#if defined(r_VertexSkinning)
attribute vec4		attr_BoneIndexes;
attribute vec4		attr_BoneWeights;
uniform int			u_VertexSkinning;
uniform mat4		u_BoneMatrix[128];
#endif

uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform int			u_InverseVertexColor;
uniform mat4		u_LightAttenuationMatrix;
uniform mat4		u_ShadowMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec4		var_Vertex;
varying vec4		var_TexDiffuse;
varying vec4		var_TexNormal;
varying vec2		var_TexSpecular;
varying vec4		var_TexAtten;
varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;

void	main()
{
#if defined(r_VertexSkinning)
	if(bool(u_VertexSkinning))
	{
		vec4 vertex = vec4(0.0);
		vec3 tangent = vec3(0.0);
		vec3 binormal = vec3(0.0);
		vec3 normal = vec3(0.0);

		for(int i = 0; i < 4; i++)
		{
			int boneIndex = int(attr_BoneIndexes[i]);
			float boneWeight = attr_BoneWeights[i];
			mat4  boneMatrix = u_BoneMatrix[boneIndex];
			
			vertex += (boneMatrix * attr_Position) * boneWeight;
		
			tangent += (boneMatrix * vec4(attr_Tangent, 0.0)).xyz * boneWeight;
			binormal += (boneMatrix * vec4(attr_Binormal, 0.0)).xyz * boneWeight;
			normal += (boneMatrix * vec4(attr_Normal, 0.0)).xyz * boneWeight;
		}

		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * vertex;
		
		// transform position into world space
		var_Vertex = u_ModelMatrix * vertex;
		
		var_Tangent.xyz = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
		var_Binormal.xyz = (u_ModelMatrix * vec4(binormal, 0.0)).xyz;
		var_Normal.xyz = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
		
		// calc light attenuation in light space
		var_TexAtten = u_LightAttenuationMatrix * vertex;
	
		// calc shadow attenuation in light space
		vec4 texShadow = u_ShadowMatrix * vertex;
	
		// Tr3B: put it into other varyings because we reached the maximum on a Geforce 6600
		var_Vertex.w = texShadow.s;
		var_Tangent.w = texShadow.t;
		var_Binormal.w = texShadow.p;
		var_Normal.w = texShadow.q;
	}
	else
#endif
	{
		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * attr_Position;
		
		// transform position into world space
		var_Vertex = u_ModelMatrix * attr_Position;
	
		var_Tangent.xyz = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
		var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;
		var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
		
		// calc light attenuation in light space
		var_TexAtten = u_LightAttenuationMatrix * attr_Position;
	
		// calc shadow attenuation in light space
		vec4 texShadow = u_ShadowMatrix * attr_Position;
	
		// Tr3B: put it into other varyings because we reached the maximum on a Geforce 6600
		var_Vertex.w = texShadow.s;
		var_Tangent.w = texShadow.t;
		var_Binormal.w = texShadow.p;
		var_Normal.w = texShadow.q;
	}
	
	// transform diffusemap texcoords
	var_TexDiffuse.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;
	
	// transform normalmap texcoords
	var_TexNormal.st = (u_NormalTextureMatrix * attr_TexCoord0).st;
	
	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;
	
	// assign color
	if(bool(u_InverseVertexColor))
	{
		var_TexDiffuse.p = 1.0 - attr_Color.r;
		var_TexNormal.p = 1.0 - attr_Color.g;
		var_TexNormal.q = 1.0 - attr_Color.b;
	}
	else
	{
		var_TexDiffuse.p = attr_Color.r;
		var_TexNormal.pq = attr_Color.gb;
	}
}
