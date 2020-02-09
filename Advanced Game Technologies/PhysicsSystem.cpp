#include "PhysicsSystem.h"
#include "PhysicsObject.h"
#include "GameObject.h"
#include "CollisionDetection.h"
#include "../../Common/Quaternion.h"

/* Only a selection of functions of the physics system are given as example here to demonstrate the broadphase extension and additional collision resolution. */
#include "Constraint.h"

#include "Debug.h"

#include <functional>
using namespace NCL;
using namespace CSC8503;

// Build up the sets of dynamic and static objects.
void PhysicsSystem::SetupQuadtree()
{
	// On event of scene reset.
	dynamicObjects.clear();

	// Must update object AABBs before inserting into a tree that won't bother doing so!
	UpdateObjectAABBs();

	staticTree.SetParams(Vector2(1024, 1024), 7, 6);

	// Iterate through all objects to create static tree and list of dynamic objects
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	gameWorld.GetObjectIterators(first, last);

	// Iterate through all game objects to insert them into the tree or the dynamic object vector depending on what they're marked as.
	for (auto i = first; i != last; ++i)
	{
		Vector3 halfSizes;
		if (!(*i)->GetBroadphaseAABB(halfSizes))
			continue;

		Vector3 pos = (*i)->GetConstTransform().GetWorldPosition();

		if ((*i)->IsStatic())
		{
			staticTree.Insert(*i, pos, halfSizes);
		}
		else
			dynamicObjects.emplace_back((*i));
		
	}
}


// Builds a dynamic quadtree every frame- still a collision acceleration as static objects left alone!
void PhysicsSystem::DynamicVersusDynamic() {
	QuadTree<GameObject*> dynamicTree(Vector2(1024, 1024), 7, 6);

	std::vector<GameObject*>::const_iterator first = dynamicObjects.begin();
	std::vector<GameObject*>::const_iterator last = dynamicObjects.end();

	for (auto i = first; i != last; ++i)
	{
		Vector3 halfSizes;
		if (!(*i)->GetBroadphaseAABB(halfSizes))
			continue;

		Vector3 pos = (*i)->GetConstTransform().GetWorldPosition();
		dynamicTree.Insert(*i, pos, halfSizes);
	}

	dynamicTree.OperateOnContents(
		[&](std::list<QuadTreeEntry<GameObject*>>& data) {
			CollisionDetection::CollisionInfo info;
			for (auto i = data.begin(); i != data.end(); ++i)
			{
				for (auto j = std::next(i); j != data.end(); ++j)
				{
					info.a = min((*i).object, (*j).object);
					info.b = max((*i).object, (*j).object);
					broadphaseCollisions.insert(info);
				}
			}
		}
	);
}

void PhysicsSystem::ImpulseResolveCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const {

	PhysicsObject* physA = a.GetPhysicsObject();
	PhysicsObject* physB = b.GetPhysicsObject();

	Transform& transformA = a.GetTransform();
	Transform& transformB = b.GetTransform();

	// Determine total inverse mass for later impulse calculation and projection. 
	float totalMass = physA->GetInverseMass() + physB->GetInverseMass();

	if (totalMass == 0.0f)
		return;

	// Seperate objects out using projection by pushing them along the collision normal proportional to penetration distance. 
	transformA.SetWorldPosition(transformA.GetWorldPosition() - (p.normal * p.penetration * (physA->GetInverseMass() / totalMass)));
	transformB.SetWorldPosition(transformB.GetWorldPosition() + (p.normal * p.penetration * (physB->GetInverseMass() / totalMass)));

	// Start building up impulse variables. 
	// Get collision points relative to each object's position. 
	Vector3 relativeA = p.localA;
	Vector3 relativeB = p.localB;

	// Determine how much the object is moving.
	Vector3 angVelocityA = Vector3::Cross(physA->GetAngularVelocity(), relativeA);
	Vector3 angVelocityB = Vector3::Cross(physB->GetAngularVelocity(), relativeB);

	// Determine velocities at which the objects are colliding. 
	Vector3 fullVelocityA = physA->GetLinearVelocity() + angVelocityA;
	Vector3 fullVelocityB = physB->GetLinearVelocity() + angVelocityB;

	Vector3 contactVelocity = fullVelocityB - fullVelocityA;

	if (Vector3::Dot(contactVelocity, p.normal) > 0)
		return;

	// Start building up the impulse vector J. 
	float impulseForce = Vector3::Dot(contactVelocity, p.normal);

	// Work out the effect of inertia. 
	Vector3 inertiaA = Vector3::Cross(physA->GetInertiaTensor() * Vector3::Cross(relativeA, p.normal), relativeA);
	Vector3 inertiaB = Vector3::Cross(physB->GetInertiaTensor() * Vector3::Cross(relativeB, p.normal), relativeB);

	float angularEffect = Vector3::Dot(inertiaA + inertiaB, p.normal);

	// Disperse some kinetic energy. 
	float cRestitution = physA->GetElasticity() * physB->GetElasticity();

	float j = (-(1.0f + cRestitution) * impulseForce) / (totalMass + angularEffect);
	
	Vector3 fullImpulse = p.normal * j;

	physA->ApplyLinearImpulse(-fullImpulse);
	physB->ApplyLinearImpulse(fullImpulse);

	physA->ApplyAngularImpulse(Vector3::Cross(relativeA, -fullImpulse));
	physB->ApplyAngularImpulse(Vector3::Cross(relativeB, fullImpulse));
}

// Apply penalty resolution via springs where the objects collide with each other
void PhysicsSystem::ResolveSpringCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const
{
	PhysicsObject* physA = a.GetPhysicsObject();
	PhysicsObject* physB = b.GetPhysicsObject();

	Vector3 springPosA = p.localA;
	Vector3 springPosB = p.localB;

	Vector3 springExtension = p.normal * p.penetration;

	// F = -kx
	Vector3 forceA = -springExtension * physB->GetStiffness();
	Vector3 forceB = springExtension * physA->GetStiffness();

	physA->AddForceAtPosition(forceA, springPosA);
	physB->AddForceAtPosition(forceB, springPosB);
}

void PhysicsSystem::BroadPhase() {
	// Create a quad tree with some default parameters
	broadphaseCollisions.clear();

	// Dynamic versus dynamic collisions 
	DynamicVersusDynamic(); 

	// Dynamic versus static: take the dynamic object down the tree to see where it would have gone, but just compare it to the node contents rather than inserting. 
	std::vector<GameObject*>::const_iterator first = dynamicObjects.begin();
	std::vector<GameObject*>::const_iterator last = dynamicObjects.end();

	for (auto i = first; i != last; ++i)
	{

		Vector3 halfSizes;
		if (!(*i)->GetBroadphaseAABB(halfSizes))
			continue;

		Vector3 pos = (*i)->GetConstTransform().GetWorldPosition();

		staticTree.DynamicObjectComparison([&](std::list<QuadTreeEntry<GameObject*>>& data)
			{
				CollisionDetection::CollisionInfo info;
				for (auto j = data.begin(); j != data.end(); ++j)
				{
					// is this pair of items already in the collision set-
					// if the same pair is in another quadtree node together etc
					info.a = min((*j).object, (*i));
					info.b = max((*j).object, (*i));
					broadphaseCollisions.insert(info);
				}
			},
			*i, pos, halfSizes);
	}
}

void PhysicsSystem::NarrowPhase() {
	// Iterate through all collisions added to the list, and if the two collision volumes are actually intersecting, resolve the collision
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = broadphaseCollisions.begin(); i != broadphaseCollisions.end(); ++i)
	{
		CollisionDetection::CollisionInfo info = *i;
	
		if (CollisionDetection::ObjectIntersection(info.a, info.b, info))
		{
			
			info.framesLeft = numCollisionFrames;

			// Determine what type of resolutions to use between these objects.
			CollisionType pairType = (CollisionType)((int)info.a->GetPhysicsObject()->GetCollisionType() & (int)info.b->GetPhysicsObject()->GetCollisionType());

			if (pairType == CollisionType::IMPULSE)
			{
				ImpulseResolveCollision(*info.a, *info.b, info.point);
			}

			if (pairType == CollisionType::SPRING)
			{
				ResolveSpringCollision(*info.a, *info.b, info.point);
			}

			allCollisions.insert(info); // insert into our main set
		}
	}
}

void PhysicsSystem::IntegrateAccel(float dt) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i)
	{
		PhysicsObject* object = (*i)->GetPhysicsObject();
		// Ignore the object if there's no physics object, or it's marked as sleeping or static.
		if (object == nullptr || (*i)->IsSleeping() || (*i)->IsStatic())
			continue; // No physics object for this game object

		Vector3 posDiff = object->GetTransform()->GetLocalPosition() - object->GetTransform()->GetPreviousPosition();
		float diff = posDiff.Length();

		float inverseMass = object->GetInverseMass();

		Vector3 linearVel = object->GetLinearVelocity();
		Vector3 force = object->GetForce();
		Vector3 accel = force * inverseMass;

		if (applyGravity && inverseMass > 0)
			accel += gravity; // Don't move infinitely heavy things

		linearVel += accel * dt; // integrate accel into a new velocity

		object->SetLinearVelocity(linearVel);

		// Angular motion stuff.
		Vector3 torque = object->GetTorque();
		Vector3 angVel = object->GetAngularVelocity();

		// Update tensor vs orientation.
		object->UpdateInertiaTensor();

		Vector3 angAccel = object->GetInertiaTensor() * torque;

		// Integrate angular accel
		angVel += angAccel * dt;

		object->SetAngularVelocity(angVel);
	}
}

void PhysicsSystem::IntegrateVelocity(float dt) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	gameWorld.GetObjectIterators(first, last);

	float dampingFactor = 1.0f - 0.95f;
	float frameDamping = powf(dampingFactor, dt);

	for (auto i = first; i != last; ++i)
	{
		PhysicsObject* object = (*i)->GetPhysicsObject();
		// Ignore the object if there's no physics object, or it's marked as sleeping or static.
		if (object == nullptr || (*i)->IsSleeping() || (*i)->IsStatic())
			continue;

		Transform& transform = (*i)->GetTransform();

		// Position stuff
		Vector3 position = transform.GetLocalPosition();
		(*i)->SetPreviousPosition(position);
		Vector3 linearVel = object->GetLinearVelocity();
		position += linearVel * dt;
		transform.SetLocalPosition(position);
		transform.SetWorldPosition(position);

		// Determine over a number of updates whether this object has been static enough to sleep- prevents infinitely jiggling objects
		float distanceMoved = (position - (*i)->GetPreviousPosition()).Length();
		(*i)->amountMoved += distanceMoved;
		(*i)->sleepPollCount += 1;
		if ((*i)->sleepPollCount >= 60)
		{
			float avgDistance = (*i)->amountMoved / 60;
			if (avgDistance < 0.01f)
			{
				(*i)->SetSleeping(true);
			}
			(*i)->amountMoved = 0;;
			(*i)->sleepPollCount = 0;
		}

		// Linear damping
		linearVel = linearVel * frameDamping; // slightly scale down the velocity based on the framerate
		object->SetLinearVelocity(linearVel);

		// Orientation stuff
		Quaternion orientation = transform.GetLocalOrientation();
		Vector3 angVel = object->GetAngularVelocity();

		// Actual integration. 
		// 0.5 just due to how quaternions work!
		orientation = orientation + (Quaternion(angVel * dt * 0.5f, 0.0f) * orientation);
		orientation.Normalise();

		transform.SetLocalOrientation(orientation);

		// Damp the angular velocity too so it doesn't just spin forever.
		angVel = angVel * frameDamping;
		object->SetAngularVelocity(angVel);
	}

}
