/*
Author: Eleanor Gregory
Date: Dec 2019

A full instance of a goose game.

/ᐠ .ᆺ. ᐟ\ﾉ

*/

/* A selection of functions for game setup and running provided here for code example. */

#include "CourseworkGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../../Common/Maths.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/VectorPositionConstraint.h"
#include "../CSC8503Common/OrientationConstraint.h"
#include "../CSC8503Common/UpwardsConstraint.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../../Common/Assets.h"
#include <sstream>
#include <fstream>
#include <regex>

using namespace NCL;
using namespace CSC8503;

CourseworkGame::CourseworkGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	forceMagnitude = 10.0f;
	useGravity = true;
	physics->UseGravity(useGravity);
	inSelectionMode = false;
	camDistanceFromPlayer = 20;

	currentMenuState = StateType::MAIN;
	menuChoice = 0;
	exit = false;

	timeRemaining = 180; // three minutes in seconds 
	timeUp = false;
	currentTime = 0;
	timerRunning = false;

	heldItemIDCounter = 0;

	showHighScores = false;
	newPlayerJoined = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
	SetupMainMenuMachine();	
}

void CourseworkGame::UpdateGame(float dt) {
	if (currentMenuState == StateType::MAIN)
	{
		Vector2 arrowPos;
		if (menuChoice == 0) // single player
			arrowPos = Vector2(200, 450);
		else if (menuChoice == 1) // client player
			arrowPos = Vector2(200, 400);
		else if (menuChoice == 2) // server player
			arrowPos = Vector2(200, 350);
		else if (menuChoice == 3) // exit
			arrowPos = Vector2(200, 300);

		renderer->DrawString("Nameless Bird Entertainment", Vector2(300, 600), Vector4(0, 0, 1, 1));
		renderer->DrawString("Single Player", Vector2(300, 450), Vector4(0, 1, 0, 1));
		renderer->DrawString("Client Player", Vector2(300, 400), Vector4(0, 1, 0, 1));
		renderer->DrawString("Server Player", Vector2(300, 350), Vector4(0, 1, 0, 1));
		renderer->DrawString("Exit", Vector2(300, 300), Vector4(0, 1, 0, 1));
		renderer->DrawString("=>", arrowPos, Vector4(0, 0, 1, 1));
		world->UpdateWorld(dt);
		renderer->Update(dt);
		renderer->Render();
	}

	else if (currentMenuState == StateType::GAME)
	{
		// Timer updating- single player does this with no limits. 
		// Server will only start the timer once its signalled as having a client connected..
		// Clients don't update the timer at all though will check if its over.
		if (gameType == SINGLE || (gameType == SERVER && timerRunning))
		{
			currentTime += dt;
			if (currentTime >= 1.0f)
			{
				timeRemaining -= 1.0f;
				currentTime = 0;
				if (timeRemaining <= 0)
				{
					timeUp = true;
				}

				if (gameType == SERVER)
				{
					// send a packet with the new time to display
					server->SendGlobalPacket(IntPacket(timeRemaining));
				}
			}
		}
		else
		{
			if (timeRemaining <= 0)
			{
				timeUp = true;
			}
		}
		

		if (!inSelectionMode) {
			world->GetMainCamera()->UpdateCamera(dt);
		}
		if (lockedObject != nullptr) {
			LockedCameraMovement();
		}

		renderer->DrawString("Score: " + std::to_string(playerGoose->GetScore()), Vector2(10, 90)); 
		renderer->DrawString("Time: " + std::to_string(timeRemaining), Vector2(10, 0));
		UpdateKeys();

		Debug::Print("(P)ause", Vector2(10, 40));

		SelectObject();

		if (gameType != SERVER)
		{
			MoveGoose();
			if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT))
			{
				// Single player performs item pickup check as normal.
				if (gameType == SINGLE)
				{
					if (!playerGoose->IsHoldingItem())
					{
						// Send a ray out from the front of the player to try and pick up a held item in front of them.
						Ray ray = Ray(playerGoose->GetTransform().GetWorldPosition(), playerGoose->GetTransform().GetWorldOrientation() * Vector3(0, 0, 1));
						RayCollision closestCollision;

						if (world->Raycast(ray, closestCollision, playerGoose, true))
						{
							HeldItem* item = (HeldItem*)closestCollision.node;
							if (closestCollision.rayDistance < 3.0f && closestCollision.rayDistance > 0)
								playerGoose->PickUpItem(item);
						}
					}
					else
					{
						// Right click pressed to release a held item, so do that.
						playerGoose->DropHeldItem();
					}
				}
				// Client player needs to pass on the fact that item pickup button was pressed to the server.
				else
				{
					client->SendPacket(ClientPlayerInputPacket());
				}
			}
		}
		else
		{
			// Since the server only updates goose movement when player input comes through, any held item needs to be updated seperately still to prevent gravity stealing it.
			playerGoose->UpdateHeldItem();
		}

		for (int i = 0; i < enemies.size(); ++i)
			enemies[i]->UpdateEnemyMovement(dt);

		world->UpdateWorld(dt);
		renderer->Update(dt);
		physics->Update(dt);

		if (gameType == CLIENT)
		{
			// Signalling to the server that a new player's joined.
			if (newPlayerJoined)
			{
				client->SendPacket(NewPlayerPacket(0));
			}

			renderer->DrawString("CLIENT", Vector2(1000, 600), Vector4(0, 0, 1, 1));

			client->UpdateClient();
			
			// Show or hide the high score menu- opening the menu requests high score information from the server.
			if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::H))
			{
				client->SendPacket(RequestPacket());
				showHighScores = !showHighScores;
			}

			if (showHighScores)
				DisplayHighScoreTable();
			
		}
			   
		else if (gameType == SERVER)
		{
			renderer->DrawString("SERVER", Vector2(1000, 600), Vector4(0, 0, 1, 1));
			// Server moves the player and sends a packet to the client with its new position. 
			keeper->UpdateEnemyMovement(dt);
			server->SendGlobalPacket(PositionPacket(keeper->GetTransform().GetWorldPosition()));

			server->UpdateServer();

			if (sendHighScores)
				SendHighScoreTable();
		}
			

		Debug::FlushRenderables();
		renderer->Render();
	}
  
	else if (currentMenuState == StateType::PAUSE)
	{
		// Update everything except the physics and timer!
		renderer->DrawString("Paused!", Vector2(300, 600), Vector4(0, 0, 1, 1));
		Debug::Print("Un(P)ause", Vector2(10, 40));
		world->UpdateWorld(dt);
		renderer->Update(dt);
		Debug::FlushRenderables();
		renderer->Render();
	}

	else if (currentMenuState == StateType::TIMEUP)
	{
		Vector2 arrowPos;
		if (menuChoice == 0) // replay
			arrowPos = Vector2(200, 450);
		else if (menuChoice == 1) // exit
			arrowPos = Vector2(200, 400);

		renderer->DrawString("Time Up! Final Score " + std::to_string(playerGoose->GetScore()), Vector2(300, 600), Vector4(0, 0, 1, 1));
		renderer->DrawString("Replay Single Player", Vector2(300, 450), Vector4(0, 1, 0, 1));
		renderer->DrawString("Exit", Vector2(300, 400), Vector4(0, 1, 0, 1));
		renderer->DrawString("=>", arrowPos, Vector4(0, 0, 1, 1));
		world->UpdateWorld(dt);
		renderer->Update(dt);
		renderer->Render();
	}

	menuMachine->Update();
	
}

void CourseworkGame::ResetGame()
{
	timeUp = false;
	currentTime = 0;
	timeRemaining = 180;
	// Init world will overwrite everything else like the goose's score, starting position etc.
	InitWorld(); 
	// Need to recreate the physics!
	physics->SetupQuadtree();
	selectionObject = nullptr;
}

void CourseworkGame::MoveGoose()
{

	// Get the forward vector for where the goose currently faces.
	// Vector3(0,0,1) matches starting orientation of goose currently due to provided mesh being flipped. 
	Vector3 gooseFwd = playerGoose->GetTransform().GetWorldOrientation() * Vector3(0, 0, 1);
	
	// Set direction to move based on the keys pressed.
	// Adding to the target direction to allow for multiple keys e.g. walking diagonally. 
	Vector3 targetDir = Vector3(0, 0, 0);
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::LEFT))
		targetDir += Vector3(-1, 0, 0);

	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::RIGHT))
		targetDir += Vector3(1, 0, 0);

	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::UP))
		targetDir += Vector3(0, 0, -1);

	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::DOWN))
		targetDir += Vector3(0, 0, 1);

	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::CONTROL) && playerGoose)
	{
		// Limit the jump by checking the distance from the floor so goose can't fly off forever...
		Ray ray = Ray(playerGoose->GetTransform().GetWorldPosition(), Vector3(0, -1, 0));
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, playerGoose, true))
		{
			GameObject* closest = (GameObject*)closestCollision.node;
			if (closestCollision.rayDistance < 2.0f)
				targetDir += Vector3(0, 10, 0);
		}
		
	}
		
	// Base the movement on the orientation of the camera (using its yaw for this- around the y axis)
	float yaw = world->GetMainCamera()->GetYaw();
	Vector3 rotationTargetDir = Quaternion::EulerAnglesToQuaternion(0,yaw - 90, 0) * targetDir;
	Vector3 movementTargetDir = Quaternion::EulerAnglesToQuaternion(0, yaw, 0) * targetDir;

	// Flip the torque depending on the closest angle to the target direction.
	// Positive = rotate clockwise, Negative = rotate anticlockwise.
	float torqueDir = Vector3::Dot(gooseFwd, rotationTargetDir);
	float direction = (torqueDir < 0 ? -1 : 1);

	// Only move if we have somewhere to go!
	if (targetDir != Vector3(0,0,0))
	{
		// Wakey wakey you need to move!
		playerGoose->SetSleeping(false);

		// Actually move by applying force.
		playerGoose->GetPhysicsObject()->AddForce(movementTargetDir * forceMagnitude);

		// Subtract the torque result between where the goose is and where he wants to face from the amount of torque to apply. 
		// This will gradually scale the torque down towards 0 as the target is reached (with a bit left either side for waddle...)
		float yTorque = 5 - Vector3::Dot(gooseFwd.Normalised(), targetDir.Normalised());
		playerGoose->GetPhysicsObject()->AddTorque(Vector3(0, yTorque, 0) * direction);

		// Client needs to tell the server where it's trying to go so the server can maintain a consistent world state.
		if (gameType == CLIENT)
			client->SendPacket(PositionPacket(movementTargetDir * forceMagnitude, yTorque, direction));
	}

	// Held items need their own updating for how they're attached.
	playerGoose->UpdateHeldItem();

	// Update the camera along with the goose's position.
	// Camera smoothed out using sinosoidal easing functions. http://www.gizma.com/easing/#sin3
	if (!world->GetMainCamera()->IsFreeCam())
	{
		float camX = camDistanceFromPlayer * sinf(world->GetMainCamera()->GetYaw() * PI / 180);
		float camY = camDistanceFromPlayer * -sinf(world->GetMainCamera()->GetPitch() * PI / 180);
		float camZ = camDistanceFromPlayer * cosf(world->GetMainCamera()->GetYaw() * PI / 180);

		world->GetMainCamera()->SetPosition(playerGoose->GetTransform().GetWorldPosition() + Vector3(camX, camY, camZ));
	}	
}

void CourseworkGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	enemies.clear();

	if (currentMenuState == MAIN)
	{
		GooseObject* menuGoose = AddGooseToWorld(Vector3(20, -10, 5));
		menuGoose->GetTransform().SetWorldScale(Vector3(10, 10, 10));
		menuGoose->GetTransform().SetLocalOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -90));
	}
	else if (currentMenuState == GAME)
	{
		if (gameType == SINGLE)
			LoadWorldFromFile("GooseLevel.txt");
		else
			LoadWorldFromFile("MultiplayerGooseLevel.txt"); 

		for (int i = 0; i < enemies.size(); ++i)
		{
			enemies[i]->SetPlayer(playerGoose);
			enemies[i]->SetupStateMachine();
		}

		// Keeper information needs to be set up seperately to the rest of the enemies as server/client dependant.
		if (keeper)
		{
			keeper->SetPlayer(playerGoose);
			keeper->SetupStateMachine();
		}
	}
}

/*
* FILE KEY:
* . - Empty space
* I - Player goose island
* A - Apple
* x - Outer walls
* ~ - Water
* F - Fence (upper case: vertical fence, lower case: horizontal fence)
* B - Bonus item (sphere)
* E - AI (normal)
* T - Tree
* U - Bushes (upper case: large, lower case: small)
* H - Building
* # - nothing there, but the AI shouldn't go there anyway.
* - - Horizontal hedge, | - vertical hedge
* K - Park keeper
* O - Pillar
*/
void CourseworkGame::LoadWorldFromFile(std::string file)
{
	AddFloorToWorld(Vector3(0, -2, 0));

	std::ifstream infile(Assets::DATADIR + file);
	int width = 0;
	int height = 0; 
	int x = 0;
	int y = 0; 
	int z = 0;
	Vector3 cubeDims;
	int i = 0;
	if (infile.is_open())
	{
		string line;
		while (getline(infile, line))
		{
			// Read in the dimensions/node sizes to use at the top of the file- there's only three rows of numbers ever so hardcode it.
			if (std::regex_match(line, std::regex("[0-9]+")))
			{
				switch (i)
				{
				case 0: x = stoi(line); break;
				case 1: y = stoi(line); break;
				default: z = stoi(line); cubeDims = Vector3(x, y, z); break;
				}
				i++;
			}
			else
			{
				for (char& in : line)
				{
					// Position is offset like the navigation grid to start from the top left corner of the world.
					Vector3 pos = Vector3(width * x, 0, height * x) + Vector3(-100, 0, -100);
					switch (in)
					{
					case 'x':
						AddCubeToWorld(pos, cubeDims / 2, 0, Vector4(0.827, 0.827, 0.827, 1));
						break;
					case 'I':
						// Each island comes with a player goose- this could be used for setting up multiplayer later on.
						spawnIsland = AddIslandToWorld(Vector3(pos.x, -18, pos.z), 20);
						playerGoose = AddGooseToWorld(spawnIsland->GetTransform().GetWorldPosition() + Vector3(0, 11, 0));
						spawnIsland->SetStartingPlayer(playerGoose);
						break;
					case '~':
						AddWaterToWorld(pos, Vector3(cubeDims.x , 1, cubeDims.z)); break;
					case 'f':
						AddCubeToWorld(pos, Vector3(cubeDims.x - 1, 1, 1), 0); break;
					case 'F':
						AddCubeToWorld(pos, Vector3(1, 1, cubeDims.z / 2), 0); break;
					case 'A':
						heldItemsInWorld.emplace_back(AddAppleToWorld(Vector3(pos.x, 0.75f, pos.z))); break;
					case 'E':
						enemies.emplace_back(AddCharacterToWorld(Vector3(pos.x, 2, pos.z))); break;
					case 'K':
						keeper = AddParkKeeperToWorld(Vector3(pos.x, 2, pos.z)); break;
					case 'B':
						heldItemsInWorld.emplace_back(AddBonusSphereItemToWorld(Vector3(pos.x, 1.0f, pos.z), 0.8f, 10, 5)); break;
					case 'u':
						AddBushToWorld(Vector3(pos.x, -2, pos.z), 5.0f); break;
					case 'U':
						AddBushToWorld(Vector3(pos.x, -2, pos.z), 10.0f); break;
					case 'S':
						AddBenchToWorld(pos); break;
					case 'H':
						AddHouseToWorld(pos); break;
					case '-':
						AddHedgeToWorld(pos, Vector3(cubeDims.x, 5, 3)); break;
					case '|':
						AddHedgeToWorld(pos, Vector3(1, 1, cubeDims.z / 2)); break;
					case 'T':
						AddTreeToWorld(pos); break;
					case 'O':
						AddPillarToWorld(pos); break;
					}
					width++;
				}
				height++;
				width = 0;
			}
		}
		
		infile.close();
		
		// Seperate functions for now as they span multiple nodes.
		AddGateToWorld(Vector3(20, 0, 80));
		AddBridgeToWorld();
	
		physics->SetupQuadtree();
	}
}

/*
* Main - the main menu of the game, choose between single player, client player, server player, or exit 
* Single Player - start a game with no networking
* Pause - Pauses the game, shouldn't be done in multiplayer
* Time up - Game over state, stops everything and allows quit or game exit
*/
void CourseworkGame::SetupMainMenuMachine()
{
	menuMachine = new PushdownMachine();

	gameFunc = [](void* data, PushdownState** pushResult)
	{
		CourseworkGame* game = (CourseworkGame*)data;
		// Pause the game
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P))
		{
			*pushResult = new GenericPushdownState(game->pauseFunc, (void*)game);
			game->currentMenuState = StateType::PAUSE;
			return PushdownState::PushdownResult::Push;
		}
		if (game->timeUp)
		{
			*pushResult = new GenericPushdownState(game->timeUpFunc, (void*)game);
			game->currentMenuState = StateType::TIMEUP;
			return PushdownState::PushdownResult::Push;
		}
		return PushdownState::PushdownResult::NoChange; 
	};

	mainMenuFunc = [](void* data, PushdownState** pushResult)
	{
		CourseworkGame* game = (CourseworkGame*)data;
		// Register moving between options on the menu, cycling round if needs be. 
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN))
		{
			game->menuChoice++;
			if (game->menuChoice >= StateType::NO_OF_MENUS)
				game->menuChoice = 0;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP))
		{
			game->menuChoice--;
			if (game->menuChoice < 0)
				game->menuChoice = 3;
		}

		// Selecting an option- mark the choice and change the state appropriately 
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN))
		{
			switch (game->menuChoice)
			{
			case 0: // single player
				*pushResult = new GenericPushdownState(game->gameFunc, (void*)game);
				game->currentMenuState = GAME;
				game->gameType = SINGLE;
				game->InitWorld();
				return PushdownState::PushdownResult::Push;

			case 1: // client player
				*pushResult = new GenericPushdownState(game->gameFunc, (void*)game);
				game->currentMenuState = GAME;
				game->gameType = CLIENT;
				game->SetupNetworking();
				game->InitWorld();
				return PushdownState::PushdownResult::Push;

			case 2: // server player
				*pushResult = new GenericPushdownState(game->gameFunc, (void*)game);
				game->currentMenuState = GAME;
				game->gameType = SERVER;
				game->SetupNetworking();
				game->InitWorld();
				return PushdownState::PushdownResult::Push;

			case 3: // exit
				game->exit = true;
				return PushdownState::PushdownResult::NoChange;
			}
		}

		return PushdownState::PushdownResult::NoChange;
	};

	pauseFunc = [](void* data, PushdownState** pushResult)
	{
		CourseworkGame* game = (CourseworkGame*)data;
		// Unpause the game by taking this state off the stack.
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P))
		{
			game->currentMenuState = StateType::GAME;
			return PushdownState::PushdownResult::Pop;
		}
		return PushdownState::PushdownResult::NoChange; 
	};

	timeUpFunc = [](void* data, PushdownState** pushResult)
	{
		CourseworkGame* game = (CourseworkGame*)data;
		// Cycle through the options present at the end of the game: replay and exit
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN))
		{
			game->menuChoice++;
			if (game->menuChoice >= 2)
				game->menuChoice = 0;
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP))
		{
			game->menuChoice--;
			if (game->menuChoice < 0)
				game->menuChoice = 1;
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN))
		{
			if (game->menuChoice == 0)
			{
				*pushResult = new GenericPushdownState(game->gameFunc, (void*)game);
				game->currentMenuState = GAME;
				game->ResetGame();
				// Reset pops this state and returns to the game state, but resets all the variables for a new instance of it without queuing up multiple states to accidentally fire between.
				return PushdownState::PushdownResult::Pop;
			}
			else if (game->menuChoice == 1)
			{
				game->exit = true;
				return PushdownState::PushdownResult::NoChange;
			}

		}
		return PushdownState::PushdownResult::NoChange; 
	};

	// Add the main menu state as our first state. 
	GenericPushdownState* mainMenuState = new GenericPushdownState(mainMenuFunc, (void*)this);
	menuMachine->AddState(mainMenuState);
}


// Sets up the networking features and receivers appropriate for client or server games.
void CourseworkGame::SetupNetworking()
{
	NetworkBase::Initialise();

	int port = NetworkBase::GetDefaultPort();

	if (gameType == CLIENT)
	{
		client = new GameClient();
		clientReceiver = ClientPacketReceiver("Client", this, highScores);
		client->RegisterPacketHandler(String_Message, &clientReceiver);
		client->RegisterPacketHandler(Position_Message, &clientReceiver);
		client->RegisterPacketHandler(Held_Item_Update, &clientReceiver);
		client->RegisterPacketHandler(Player_Connected, &clientReceiver);
		client->RegisterPacketHandler(Int_Message, &clientReceiver);
		connected = client->Connect(127, 0, 0, 1, port);
		// Signal to the main game that it needs to update with a new player and send the appropriate packet.
		if (connected)
		{
			newPlayerJoined = true;
		}
			
	}
		
	else if (gameType == SERVER)
	{
		server = new GameServer(port, 1);
		serverReceiver = ServerPacketReceiver("Server", this, sendHighScores);
		server->RegisterPacketHandler(String_Message, &serverReceiver);
		server->RegisterPacketHandler(Request, &serverReceiver);
		server->RegisterPacketHandler(Position_Message, &serverReceiver);
		server->RegisterPacketHandler(Client_Player_Input, &serverReceiver);
		server->RegisterPacketHandler(Player_Connected, &serverReceiver);
	}
}

void CourseworkGame::SendHighScoreTable()
{
	// Load the data from the high score file into a string
	std::ifstream infile(Assets::DATADIR + "HighScores.txt");
	std::stringstream scoreBuffer;
	scoreBuffer << infile.rdbuf();
	server->SendGlobalPacket(StringPacket(scoreBuffer.str()));
}

void CourseworkGame::DisplayHighScoreTable()
{
	renderer->DrawString("HIGH SCORES", Vector2(300, 600), Vector4(0, 0, 1, 1));
	Debug::Print(highScores[0], Vector2(300, 450), Vector4(0, 1, 0, 1));
	Debug::Print(highScores[1], Vector2(300, 400), Vector4(0, 1, 0, 1));
	Debug::Print(highScores[2], Vector2(300, 350), Vector4(0, 1, 0, 1));
	Debug::Print(highScores[3], Vector2(300, 300), Vector4(0, 1, 0, 1));
	Debug::Print(highScores[4], Vector2(300, 250), Vector4(0, 1, 0, 1));
}

void CourseworkGame::MoveKeeperForClients(Vector3 newPos)
{
	keeper->GetTransform().SetWorldPosition(newPos);
}

void CourseworkGame::MoveGooseForServer(Vector3 force, float yTorque, float direction)
{
	// Must wake the goose up here as server applies no forces elsewhere to wake it up!
	playerGoose->SetSleeping(false);

	// Apply the sent values.
	playerGoose->GetPhysicsObject()->AddForce(force);
	playerGoose->GetPhysicsObject()->AddTorque(Vector3(0, yTorque, 0) * direction);

	playerGoose->UpdateHeldItem();

	if (!world->GetMainCamera()->IsFreeCam())
	{
		float camX = camDistanceFromPlayer * sinf(world->GetMainCamera()->GetYaw() * PI / 180);
		float camY = camDistanceFromPlayer * -sinf(world->GetMainCamera()->GetPitch() * PI / 180);
		float camZ = camDistanceFromPlayer * cosf(world->GetMainCamera()->GetYaw() * PI / 180);

		world->GetMainCamera()->SetPosition(playerGoose->GetTransform().GetWorldPosition() + Vector3(camX, camY, camZ));
	}
}

// Server performs the raycast check for an object being in front of the client goose or not to maintain a consistent world state and allow the park keeper (run by server only) to detect 
// when the goose has an item.
void CourseworkGame::ServerPickUpItem()
{
	int itemID = -1; // If the client gets -1 back from this test, there was nothing to pick up.

	// Perform raycast test to determine if there's an item in front to pick up.
	if (!playerGoose->IsHoldingItem())
	{
		Ray ray = Ray(playerGoose->GetTransform().GetWorldPosition(), playerGoose->GetTransform().GetWorldOrientation() * Vector3(0, 0, 1));
		RayCollision closestCollision;

		// If there is, pick up the item with this goose and change the ID to send back to the client.
		if (world->Raycast(ray, closestCollision, playerGoose, true))
		{
			HeldItem* item = (HeldItem*)closestCollision.node;
			if (closestCollision.rayDistance < 3.0f && closestCollision.rayDistance > 0)
			{
				playerGoose->PickUpItem(item);
				itemID = item->GetItemID();
			}
				
		}
	}
	else
	{
		// Drop the held item and signal to the client that this right click was to drop an item.
		playerGoose->DropHeldItem();
		// An id longer than the number of items in the world signals it was dropped.
		itemID = heldItemIDCounter + 1;
	}

	// Tell the client what happened if anything. 
	server->SendGlobalPacket(HeldItemUpdatePacket(itemID));

}

// Get the item ID picked up from the server and give the appropriate item to the goose.
void CourseworkGame::ClientPickUpItem(int id)
{
	if (id < heldItemIDCounter + 1)
	{
		// Need to find the particular held item object with this ID. 
		auto it = find_if(heldItemsInWorld.begin(), heldItemsInWorld.end(), [&id](const HeldItem* item) { return item->GetItemID() == id; });

		if (it != heldItemsInWorld.end())
		{
			HeldItem* foundItem = *it;
			playerGoose->PickUpItem(foundItem);
		}
	}
	else
	{
		// Actually dropped this one!
		playerGoose->DropHeldItem();
	}
}


//From here on it's functions to add in objects to the world!
/* Contrasting in depth examples (dynamic and static with different collision types) for public demonstration. */

GooseObject* CourseworkGame::AddGooseToWorld(const Vector3& position)
{
	float size = 1.0f;
	float inverseMass = 1.0f;

	GooseObject* goose = new GooseObject("Goose");

	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size, size, size));
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();
	goose->GetPhysicsObject()->SetElasticity(0.7f);
	goose->GetPhysicsObject()->SetStiffness(200.0f);

	goose->SetCollisionLayer(GameObject::PLAYER);

	goose->GetPhysicsObject()->SetCollisionType(CollisionType::IMPULSE | CollisionType::SPRING);

	world->AddGameObject(goose);

	UpwardsConstraint* constraint = new UpwardsConstraint(goose, goose->GetTransform().GetWorldOrientation() * Vector3(0, 1, 0));
	//world->AddConstraint(constraint);

	return goose;
}

void CourseworkGame::AddTreeToWorld(const Vector3& position)
{
	GameObject* trunk = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(1,15,1));

	trunk->SetBoundingVolume((CollisionVolume*)volume);

	trunk->GetTransform().SetWorldPosition(position);
	trunk->GetTransform().SetWorldScale(Vector3(1, 15, 1));

	trunk->SetRenderObject(new RenderObject(&trunk->GetTransform(), cubeMesh, NULL, basicShader));
	trunk->SetPhysicsObject(new PhysicsObject(&trunk->GetTransform(), trunk->GetBoundingVolume()));
	trunk->GetRenderObject()->SetColour(Vector4(0.545, 0.271, 0.075, 1));

	trunk->GetPhysicsObject()->SetInverseMass(0);
	trunk->GetPhysicsObject()->InitCubeInertia();

	trunk->SetIsStatic(true);
	trunk->SetCollisionLayer(GameObject::SETTING);

	trunk->GetPhysicsObject()->SetCollisionType(CollisionType::IMPULSE);

	world->AddGameObject(trunk);

	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(8, 8, 8);
	SphereVolume* leafVolume = new SphereVolume(8);
	sphere->SetBoundingVolume((CollisionVolume*)leafVolume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position + Vector3(0,15,0));

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, nullptr, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));
	sphere->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

	sphere->GetPhysicsObject()->SetInverseMass(0);
	sphere->GetPhysicsObject()->InitSphereInertia();

	sphere->SetIsStatic(true);
	sphere->SetCollisionLayer(GameObject::SETTING);


	sphere->GetPhysicsObject()->SetCollisionType(CollisionType::IMPULSE);

	world->AddGameObject(sphere);

	
}

EnemyObject* CourseworkGame::AddCharacterToWorld(const Vector3& position) {
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	EnemyObject* character = new EnemyObject(position);

	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();

	character->GetPhysicsObject()->SetCollisionType(CollisionType::IMPULSE);
	character->SetCollisionLayer(GameObject::NPC);

	world->AddGameObject(character);

	return character;
}

HeldItem* CourseworkGame::AddAppleToWorld(const Vector3& position) {
	HeldItem* apple = new HeldItem("Apple", 1, position, heldItemIDCounter);

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(4, 4, 4));
	apple->GetTransform().SetWorldPosition(position);
	apple->SetPreviousPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	apple->SetCollisionLayer(GameObject::ITEM);

	apple->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

	apple->GetPhysicsObject()->SetCollisionType(CollisionType::IMPULSE);

	apple->SetSleeping(true);

	world->AddGameObject(apple);

	heldItemIDCounter++;

	return apple;
}

