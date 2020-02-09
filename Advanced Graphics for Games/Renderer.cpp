/* The main Renderer of the coursework. */
/* A selection of functions of my own have been provided here for demonstration purposes to show off particle system use and my personal arrangement/use of the extended scene graph in particular. */

#include "Renderer.h"
Renderer::Renderer(Window& parent) : OGLRenderer(parent)
{
/* Mesh creation, camera setup, shader loading, texture loading omitted for public example */
	/* SCENE GRAPH SETUP */

	root = new SceneNode();
	root->SetActive(true);
	root->SetBoundingRadius(100000);

	heightMapNode = new HeightSceneNode(heightMap, heightmapShader, mainLight);
	heightMapNode->SetBoundingRadius(10000.0f);
	heightMapNode->SetActive(true);
	root->AddChild(heightMapNode);

	waterNode = new WaterSceneNode(waterQuad, reflectShader, mainLight, Vector4(1,1,1,1), cubeMap);
	waterNode->SetBoundingRadius(10000.0f);
	waterNode->SetActive(true);
	root->AddChild(waterNode);
	waterRotate = 0.0f;

	hellData = new MD5FileData(MESHDIR"hellknight.md5mesh");
	hellNode = new MD5Node(*hellData);
	hellNode->SetActive(true);
	hellNode->SetBoundingRadius(50);
	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellNode->PlayAnim(MESHDIR"idle2.md5anim");
	hellNode->SetShader(hellknightShader);
	hellNode->SetLight(mainLight);
	hellNode->SetModelMatrix(Matrix4::Translation(Vector3(2503.0, 70.0f, 2503.0f)));
	root->AddChild(hellNode);

	/* TREE GRAPH CREATION */
	// Transition node to enable all tree growing to be activated at the same time in one operation when camera reaches it. 
	allTrees = new SceneNode();
	allTrees->SetShader(treeShader);
	allTrees->SetLight(mainLight);
	unsigned int barkTex = SOIL_load_OGL_texture(TEXTUREDIR"bark.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	unsigned int leafTex = SOIL_load_OGL_texture(TEXTUREDIR"bush2.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	if (!barkTex || !leafTex)
		return;

	SetTextureRepeating(barkTex, true);
	SetTextureRepeating(leafTex, true);
	
	for (int i = 0; i < treePositions.size(); ++i)
	{
		Tree* tree = new Tree(rand() % 20 + 30, rand() % 20 + 50, barkTex, leafTex, 0, 0);

		tree->SetTransform(Matrix4::Translation(treePositions[i]));
		tree->SetBoundingRadius((tree->GetFinalHeight() > tree->GetFinalRadius() ? tree->GetFinalHeight() : tree->GetFinalRadius()));
		allTrees->AddChild(tree);
	}
	root->AddChild(allTrees);

	
	/* SNOW PARTICLE SYSTEM SETUP */

	snow = new ParticleSystem(10000, Vector3(5.0f, 100.0f, 200.0f), Vector4(1, 1, 1, 1), Vector3(0, 0, 0), 10000.0f);
	snow->SetTransform(Matrix4::Translation(Vector3(0, 500, 0)));
	snow->SetModelScale(Vector3(100, 100, 100));
	snow->SetBoundingRadius(10000.0f);
	snow->SetColour(Vector4(1, 1, 1, 1));

	isSnowing = false;
	meltSnow = false;
	snowMix = -1;
  
  /* Shadow, post processing, and UI setup omitted for public example */
	
	init = true;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); 

}

void Renderer::RenderScene()
{
	glDisable(GL_CULL_FACE);

	BuildNodeLists(root); 
	SortNodeLists();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawShadowScene();
	viewMatrix = mainCamera->BuildViewMatrix();
	// Reset the projection matrix just in case shadow mapping didn't!
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f); 
	

	if (splitScreen)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, screenFBO[0]);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		viewMatrix = splitScreenCam1->BuildViewMatrix();
		DrawSkybox();
		DrawCombinedScene();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, screenFBO[1]);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		viewMatrix = splitScreenCam2->BuildViewMatrix();
		DrawSkybox();
		DrawCombinedScene();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, screenFBO[2]);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		viewMatrix = splitScreenCam3->BuildViewMatrix();
		DrawSkybox();
		DrawCombinedScene();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, screenFBO[3]);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		viewMatrix = splitScreenCam4->BuildViewMatrix();
		DrawSkybox();
		DrawCombinedScene();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		DrawSplitScreens();
	}
	else
	{
		if (doPostProcess)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		DrawSkybox();

		DrawCombinedScene();

		if (doPostProcess)
		{
			DrawPostProcess();
			PresentScene();
		}
	}
		
	// Draw UI after the post processing so the text isn't distorted!
	if (showUI)
		DrawUI();


	// Start growing the trees only when all trees have finished growing.
	if (heightMap->GetFinished() && cameraPositions[1].reached)
		allTrees->SetActive(true);

	// Move the camera on from watching the trees grow when they're done. 
	if (allTrees->IsAnimFinished() && cameraGoTo == 2)
		mainCamera->SetPaused(false);
		
	SwapBuffers();
	ClearNodeLists();
}

void Renderer::DrawCombinedScene()
{	
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f); // just in case!

	DrawNodes();

	// Only draw the snow system if the camera has reached the point to show it off!
	if (cameraPositions[4].reached && isSnowing)
		DrawParticles();

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawNode(SceneNode* n)
{
	// Only run through with drawing the node content if we actually want to see it in the scene right now!
	if (n->IsActive())
	{
		// If there's a mesh, update model matrix and draw the mesh.
		if (n->GetMesh())
		{
			// If this node doesn't have an assigned shader or light, climb up its parents until one is found to use. 
			SceneNode* ancestor = n; 
			while (ancestor->GetShader() == NULL)
				ancestor = ancestor->GetParent();
			
			SetCurrentShader(ancestor->GetShader());
			while (ancestor->GetLight() == NULL)
				ancestor = ancestor->GetParent();

			SetShaderLight(*(ancestor->GetLight()));

			n->SendUniforms();

			glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&mainCamera->GetPosition());
			glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
			glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
			glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "snowAmount"), snowMix);

			if (drawingShadows)
			{
				glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 10);
				glActiveTexture(GL_TEXTURE10);
				glBindTexture(GL_TEXTURE_2D, shadowTex);
			}

			glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), (int)n->GetMesh()->GetTexture());

			modelMatrix = n->GetModelMatrix();
			textureMatrix = n->GetTextureMatrix();
			UpdateShaderMatrices();

			n->Draw(*this);
		}
	}
}

void Renderer::DrawNodes()
{
	for (vector<SceneNode*>::const_iterator it = opaqueNodeList.begin(); it != opaqueNodeList.end(); ++it)
		DrawNode((*it));
}
