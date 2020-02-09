/*
Author: Eleanor Gregory
Date: Nov 2019

A class to hold, update and emit all particles making up a system.

/ᐠ .ᆺ. ᐟ\ﾉ

*/
#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(int noOfParticles, Vector3 centre, Vector4 colour, Vector3 velocity, float life)
{
	Mesh* m = Mesh::GenerateQuad();
	allParticles.reserve(noOfParticles);

	// Create requested particles at random positions around the defined centre and the corner of the heightmap.
	for (int i = 0; i < noOfParticles; ++i)
	{
		float x = (float)rand() / (float)RAND_MAX;
		float z = (float)rand() / (float)RAND_MAX;
		allParticles.push_back(new Particle(Vector3(rand() % (int)(RAW_WIDTH * HEIGHTMAP_X) + x, centre.y, rand() % (int)(RAW_HEIGHT * HEIGHTMAP_X) + z), velocity, colour, life, m));
	}

	last = 0;
	particleCount = 0;
	this->centre = centre;
	this->life = life;
	newParticle = 0;
}

ParticleSystem::~ParticleSystem()
{
	for (int i = 0; i < allParticles.size(); ++i)
		delete allParticles[i];
}

void ParticleSystem::UpdateSystem(float msec)
{
	newParticle += PARTICLES_PER_FRAME_LIMIT * msec;

	for (int i = 0; i < allParticles.size(); ++i)
	{
		if (allParticles[i]->IsActive())
		{
			allParticles[i]->ReduceLife(msec);
			if (allParticles[i]->GetLife() >= 0)
			{
				// Negative velocity to send the particles downwards.
				allParticles[i]->UpdatePosition(allParticles[i]->GetVelocity());
				allParticles[i]->SetVelocity(Vector3(0, -(0.1f * msec), 0));
				allParticles[i]->Update(msec);
			}
			else
			{
				// Reset the particle to a new position around the 'beginning' of the system for reuse.
				allParticles[i]->SetActive(false);
				allParticles[i]->SetLife(life);
				float x = (float)rand() / (float)RAND_MAX;
				float z = (float)rand() / (float)RAND_MAX;
				allParticles[i]->SetPosition(Vector3(rand() % (int)(RAW_WIDTH * HEIGHTMAP_X) + x, centre.y, rand() % (int)(RAW_HEIGHT * HEIGHTMAP_X) + z));
			}
		}
	}

	if (newParticle >= 1)
	{
		Emit();
		newParticle = 0;
	}
}

void ParticleSystem::Draw(GLuint shader)
{
	for (int i = 0; i < allParticles.size(); ++i)
	{
		if (allParticles[i]->IsActive())
		{
			glUniformMatrix4fv(glGetUniformLocation(shader, "particleMatrix"), 1, false, (float*)&allParticles[i]->GetParticleMatrix());
			allParticles[i]->Draw();
		}
	}
}

int ParticleSystem::FindFirstUnused()
{
	// Search backwards through system first as more likely to be at the end and return instantly.
	for (int i = last; i < allParticles.size(); ++i)
	{
		if (!allParticles[i]->IsActive())
		{
			last = i;
			return i;
		}
	}
	// Do a linear search otherwise.
	for (int i = 0; i < last; ++i)
	{
		if (!allParticles[i]->IsActive())
		{
			last = i;
			return i;
		}
	}

	// If every other particle is alive, override the first one.
	return allParticles.size() - 1;
}

void ParticleSystem::Emit()
{
	// Find the first particle that can be reused to create a 'new' one and set it to begin.
	int i = FindFirstUnused();
	allParticles[i]->SetVelocity(Vector3(0, 0, 0));
	allParticles[i]->SetActive(true);
}
