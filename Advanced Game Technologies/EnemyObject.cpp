/*
Author: Eleanor Gregory
Date: Dec 2019

A subclass of GameObject to represent an enemy.
Includes the state machine for the enemy's different behaviours.

/ᐠ .ᆺ. ᐟ\ﾉ

*/
#include "EnemyObject.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/StateTransition.h"
using namespace NCL;
using namespace NCL::CSC8503;

EnemyObject::EnemyObject(Vector3 initialPos, string name) : GameObject(name)
{
	initialPosition = initialPos;
	lastPlayerScore = 0;
	moving = false;
	pathForce = Vector3(0, 0, 0);
	currentState = IDLE;
	isSleeping = true;
	timeSinceLastPathFound = 0;
}

EnemyObject::~EnemyObject()
{
	delete enemyMovementMachine;
}

void EnemyObject::SetupStateMachine()
{
	enemyMovementMachine = new StateMachine();

	// Idle state - doing nothing
	StateFunc idleFunc = [](void* data) {
		EnemyObject* realData = (EnemyObject*)data;
		realData->currentState = IDLE;
	};

	// Chasing the player
	StateFunc chaseFunc = [](void* data) {
		EnemyObject* realData = (EnemyObject*)data;
		realData->currentState = CHASE;
		// Update the pathfinding periodically every half second.
		if (realData->timeSinceLastPathFound >= 0.5f)
		{
			realData->timeSinceLastPathFound = 0;
			NavigationGrid grid("GooseLevel.txt", Vector3(-100, 0, -100));

			Vector3 startPos = realData->GetTransform().GetWorldPosition();
			Vector3 endPos = realData->player->GetTransform().GetWorldPosition();
			startPos.y = 0;
			endPos.y = 0;

			float distanceBetween = (startPos - endPos).Length();

			// If too close to the target, pathfinding unnecessary and may not be found due to the navgrid size so just move along a simple direction vector.
			if (distanceBetween < grid.GetNodeSize())
			{
				realData->pathForce = (endPos - startPos).Normalised() * 10.0f;
			}
			else
			{
				// Actual A* since target is pretty far away!
				grid.FindPath(startPos, endPos, realData->pathToTake);

				// Toss the first waypoint since this will usually be the enemy's actual grid position.
				Vector3 moveTo;
				realData->pathToTake.PopWaypoint(moveTo);

				// Now we can get the first actual point to move to. 
				realData->pathToTake.PopWaypoint(moveTo);

				realData->pathForce = (moveTo - startPos).Normalised() * 10.0f;
			}
		}

		realData->GetPhysicsObject()->AddForce(realData->pathForce);
	};

	// Returning to initial position after the goose has returned an item or lost one.
	StateFunc returnFunc = [](void* data) {
		EnemyObject* realData = (EnemyObject*)data;
		realData->currentState = RETURN;
		if (realData->lastPlayerScore != realData->playerScore)
			realData->lastPlayerScore = realData->playerScore;

		// Update force
		Vector3 moveFrom = realData->GetTransform().GetWorldPosition();
		realData->pathForce = (realData->nodeTarget - moveFrom).Normalised() * 10.0f;

		// If there's no best path to follow, calculate one. 
		if (realData->pathToTake.IsEmpty())
		{
			NavigationGrid grid("GooseLevel.txt", Vector3(-100, 0, -100));

			Vector3 startPos = realData->GetTransform().GetWorldPosition();
			Vector3 endPos = realData->initialPosition;
			grid.FindPath(startPos, endPos, realData->pathToTake);
		}
		// If we have a path to follow, but haven't started moving yet. 
		if (!realData->pathToTake.IsEmpty() && !realData->moving)
		{
			Vector3 moveTo;
			realData->pathToTake.PopWaypoint(moveTo);
			realData->pathToTake.PopWaypoint(moveTo);

			realData->moving = true;
			realData->SetNodeTarget(moveTo);
		}
		// If the next target has been decided, apply force to move in that direction before moving onto the next target step.
		if (realData->moving)
		{
			Vector3 currentPos = realData->GetTransform().GetWorldPosition();
			Vector3 targetPos = realData->nodeTarget;
			targetPos.y = currentPos.y;
			float length = (currentPos - targetPos).Length();

			// Close enough to the waypoint to move on to the next node.
			if (length < 2.0f)
			{
				realData->moving = false;
			}
			else
			{
				realData->GetPhysicsObject()->AddForce(realData->pathForce);
			}

		}
	};
	
	// A function to execute on transition to the chase or return states: clears the pathfinding data and wakes the enemy up.
	StateFunc clearPathOnTransition = [](void* data) {
		EnemyObject* realData = (EnemyObject*)data;
		realData->pathToTake.Clear();
		realData->pathForce = Vector3(0, 0, 0);
		realData->moving = false;
		realData->SetSleeping(false);
	};

	// The same pathfinding clearing, but needs to sleep the gameobject too so enemies don't just standing there vibrating...
	StateFunc transitionToIdle = [](void* data) {
		EnemyObject* realData = (EnemyObject*)data;
		realData->pathToTake.Clear();
		realData->pathForce = Vector3(0, 0, 0);
		realData->moving = false;
		realData->SetSleeping(true);
	};

	/* ADDING STATES AND TRANSITIONS  TO MOVEMENT MACHINE */

	GenericState* idleState = new GenericState(idleFunc, (void*)this, transitionToIdle, (void*)this);
	GenericState* chaseState = new GenericState(chaseFunc, (void*)this, clearPathOnTransition, (void*)this);
	GenericState* returnState = new GenericState(returnFunc, (void*)this, clearPathOnTransition, (void*)this);

	enemyMovementMachine->AddState(idleState);
	enemyMovementMachine->AddState(chaseState);
	enemyMovementMachine->AddState(returnState);

	// Idle -> Chase based on player distance
	PlayerChaseTransition* idleToChase = new PlayerChaseTransition(player, this, 20.0f, idleState, chaseState);

	// Chase -> Returning based on player getting item back to island
	GenericTransition<int&, int&>* chaseToReturn = new GenericTransition<int&, int&>(GenericTransition<int&, int&>::GreaterThanTransition, playerScore, lastPlayerScore, chaseState, returnState);

	// Chase -> return based on player being caught and losing held item
	GooseBasedTransition* catchToReturn = new GooseBasedTransition(player, false, chaseState, returnState);

	// Return -> Idle based on being back at its initial position. Should be able to equals this one as initial positions are based on the pathfinding grid!
	DistanceTransition* returnToIdle = new DistanceTransition(initialPosition, this, 3.0f, returnState, idleState);

	// Return -> Chase: go after the goose again if it passes with another item while enemy is returning to initial position.
	PlayerChaseTransition* returnToChase = new PlayerChaseTransition(player, this, 20.0f, returnState, chaseState);

	enemyMovementMachine->AddTransition(idleToChase);
	enemyMovementMachine->AddTransition(chaseToReturn);
	enemyMovementMachine->AddTransition(returnToIdle);
	enemyMovementMachine->AddTransition(catchToReturn);
	enemyMovementMachine->AddTransition(returnToChase);
}

void EnemyObject::UpdateEnemyMovement(float dt)
{
	timeSinceLastPathFound += dt;
	// Poll the player's current score- used to tell when an item has been returned to the island.
	int newPlayerScore = player->GetScore();
	// Prevent a score mismatch if the player increases their score before coming past to be chased.
	if (newPlayerScore != playerScore && currentState != CHASE)
		lastPlayerScore = newPlayerScore;
	playerScore = newPlayerScore;
	enemyMovementMachine->Update();
}

void EnemyObject::OnCollisionBegin(GameObject* otherObject)
{
	if (otherObject->GetPhysicsObject()->GetCollisionType() == CollisionType::SPRING)
	{
		// Force the enemy to move away and recalculate so it doesn't bounce off the side of the water forever just in case!
		pathForce = -pathForce;
		physicsObject->AddForce(pathForce);
		timeSinceLastPathFound = 10.0f;
	}
}
