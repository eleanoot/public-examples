/*
Author: Eleanor Gregory
Date: Nov 2019

A special SceneNode specifically to hold and update tesslated water data.

/ᐠ .ᆺ. ᐟ\ﾉ

*/
#include "WaterSceneNode.h"
WaterSceneNode::WaterSceneNode(Mesh* m, Shader* s, Light* l, Vector4 colour, GLuint cm)
	: SceneNode(m, s, l, colour)
{
	waterRotate = 0;
	model = Matrix4::Translation(Vector3((RAW_WIDTH * HEIGHTMAP_X / 2.0f), 240 * HEIGHTMAP_Y / 3.0f, (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f))) *
		Matrix4::Scale(Vector3((RAW_WIDTH * HEIGHTMAP_X / 2.0f), 1, (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f))) *
		Matrix4::Rotation(90, Vector3(1.0f, 0.0f, 0.0f));
	texture = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) * Matrix4::Rotation(waterRotate, Vector3(0.0f, 0.0f, 1.0f));
	cubeMap = cm;
}

WaterSceneNode::~WaterSceneNode()
{

}

void WaterSceneNode::SendUniforms()
{
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glUniform1i(glGetUniformLocation(shader->GetProgram(), "heights"), 1);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "cubeTex"), 2);

	glUniform1f(glGetUniformLocation(shader->GetProgram(), "waterWave"), waterRotate);
	glUniform1f(glGetUniformLocation(shader->GetProgram(), "time"), waterRotate * 1000);


	texture = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) * Matrix4::Rotation(waterRotate, Vector3(0.0f, 0.0f, 1.0f));
}

void WaterSceneNode::Draw(const OGLRenderer& r)
{
	// Draw overridden to ensure the quad is changed to patches for tesselation. 
	mesh->SetType(GL_PATCHES);
	glPatchParameteri(GL_PATCH_VERTICES, 4);

	SceneNode::Draw(r);
	mesh->SetType(GL_TRIANGLE_STRIP);
}
