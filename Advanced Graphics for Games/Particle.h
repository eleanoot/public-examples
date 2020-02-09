/*
Author: Eleanor Gregory
Date: Nov 2019

A class to represent a single particle in a system.

/ᐠ .ᆺ. ᐟ\ﾉ

*/

#pragma once
#include "Vector4.h"
#include "Mesh.h"
class Particle
{
public:
	Particle(Vector3 pos, Vector3 vel, Vector4 col, float life, Mesh*& m);
	~Particle() {};

	Vector3 GetPositon() const { return position; }
	void SetPosition(Vector3 pos) { position = pos; particleMatrix = Matrix4::Translation(position); }
	void UpdatePosition(Vector3 pos) { position += pos; }

	Vector3 GetVelocity() const { return velocity; }
	void SetVelocity(Vector3 vel) { velocity = vel; }
	void UpdateVelocity(Vector3 vel) { velocity = vel; }

	Vector4 GetColour() const { return colour; }
	void SetColour(Vector4 col) { colour = col; }

	float GetLife() const { return life; }
	void SetLife(float l) { life = l; }
	void ReduceLife(float l) { life -= l; }

	Matrix4 GetParticleMatrix() { return particleMatrix; }

	void SetActive(bool a) { active = a; }
	bool IsActive() const { return active; }

	void Draw();
	void Update(float msec);


protected:
	Mesh* quad;
	Matrix4 particleMatrix;
	Vector3 position;
	Vector3 velocity;
	Vector4 colour;
	float life;
	bool active;
};
