#pragma once
#include "..//..//nclgl/Matrix4.h"
#include "..//..//nclgl/Vector3.h"
#include "..//..//nclgl/Vector4.h"
#include "..//..//nclgl/Mesh.h"
#include "Shader.h"
#include "Light.h"
#include <vector>

class SceneNode
{
public: 
	SceneNode(Mesh* m = NULL, Shader* s = NULL, Light* l = NULL, Vector4 colour = Vector4(1, 1, 1, 1));
	~SceneNode(void);

	void SetTransform(const Matrix4& matrix) { transform = matrix; }
	const Matrix4& GetTransform() const { return transform; }
	Matrix4 GetWorldTransform() const { return worldTransform; }

	Vector4 GetColour() const { return colour; }
	void SetColour(Vector4 c) { colour = c; }

	Vector3 GetModelScale() const { return modelScale; }
	void SetModelScale(Vector3 s) { modelScale = s; }

	Mesh* GetMesh() const { return mesh; }
	void SetMesh(Mesh* m) { mesh = m; }

	Shader* GetShader() const{ return shader; }
	void SetShader(Shader* s) { shader = s; }

	Light* GetLight() const{ return light; }
	void SetLight(Light* l) { light = l; }

	void AddChild(SceneNode* s);

	void RemoveChild(SceneNode* s);

	SceneNode* GetParent() const { return parent; }

	virtual void Update(float msec); // traverse through scene graph to build up world transforms and update member variables in frame independant way 
	virtual void Draw(const OGLRenderer& r); // actually draw scene node

	float GetBoundingRadius() const { return boundingRadius; }
	void SetBoundingRadius(float f) { boundingRadius = f; }

	float GetCameraDistance() const { return distanceFromCamera; }
	void SetCameraDistance(float f) { distanceFromCamera = f; }

	// Model matrix and texture matrix required before this nodes content is drawn stored in the scene node to allow for efficent looping through all nodes in renderer. 
	Matrix4 GetModelMatrix() const { return model; }
	void SetModelMatrix(Matrix4 m) { model = m; }
	Matrix4 GetTextureMatrix() const { return texture; }
	void SetTextureMatrix(Matrix4 m) { texture = m; }

	static bool CompareByCameraDistance(SceneNode* a, SceneNode* b)
	{
		return (a->distanceFromCamera < b->distanceFromCamera) ? true : false;
	}

	void SetActive(bool a) { 
		isActive = a; 
		// Set all this node's children as the same level of activity: assuming if a parent is inactive, anything attached to it should be too. 
		for (vector<SceneNode*>::const_iterator i = children.begin(); i != children.end(); ++i)
			(*i)->SetActive(a);
	
	};
	bool IsActive() const { return isActive; };

	// If this node is animated, check if it's finished. 
	void SetAnimFinished(bool f) { animFinished = f; };
	bool IsAnimFinished();

	// const iterators to allow other classes to safely iterate over
	vector<SceneNode*>::const_iterator GetChildIteratorStart() { return children.begin();  }

	vector<SceneNode*>::const_iterator GetChildIteratorEnd() { return children.end(); }

	// Uniforms associated with the scene node's shader now set here. Overridden to allow for specific node types to send their own specific uniforms too. 
	virtual void SendUniforms();

	// For debugging purposes. 
	void SetName(string n) {
		name = n;
	}
	string GetName() const {
		return name;
	}

protected:
	SceneNode* parent;
	Mesh* mesh;
	Shader* shader;
	Light* light;
	Matrix4 worldTransform;
	Matrix4 transform; // local
	Vector3 modelScale; // seperate mesh size scale to scale without affecting transformation matrices of children
	Vector4 colour;
	vector<SceneNode*> children;

	float distanceFromCamera;
	float boundingRadius;

	Matrix4 model;
	Matrix4 texture;

	bool isActive;
	bool animFinished;

	string name; // for debug purposes
};

