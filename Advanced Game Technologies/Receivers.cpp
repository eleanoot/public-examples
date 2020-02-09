/* RECEIVER CONTENT */
void ClientPacketReceiver::ReceivePacket(int type, GamePacket* payload, int source)
{
	if (type == String_Message)
	{
		// This will be high score data!
		StringPacket* realPacket = (StringPacket*)payload;
		string msg = realPacket->GetStringFromData();
		game->FillHighScores(msg);
	}
	else if (type == Position_Message)
	{
		PositionPacket* realPacket = (PositionPacket*)payload;
		Vector3 pos = realPacket->force;

		// Client worlds update their park keeper with the position sent by the server.
		game->MoveKeeperForClients(pos);
	}
	else if (type == Held_Item_Update)
	{
		HeldItemUpdatePacket* realPacket = (HeldItemUpdatePacket*)payload;
		int itemId = realPacket->itemId;

		if (itemId > -1)
		{
			// There was a successful item event, so grab that particular item and give it to the goose.
			game->ClientPickUpItem(itemId);
		}
	}
	else if (type == Int_Message)
	{
		// This will be timer data!
		IntPacket* realPacket = (IntPacket*)payload;
		int timeValue = realPacket->i;

		game->ClientSetTimer(timeValue);
	}
}

void ServerPacketReceiver::ReceivePacket(int type, GamePacket* payload, int source)
{
	if (type == String_Message)
	{
		StringPacket* realPacket = (StringPacket*)payload;
		string msg = realPacket->GetStringFromData();

	}
	else if (type == Request)
	{
		// Client has requested the high score table
		game->SendHighScoreTable();
	}
	else if (type == Position_Message)
	{
		// Update the goose to match what the client is doing.
		PositionPacket* realPacket = (PositionPacket*)payload;
		Vector3 force = realPacket->force;
		float yTorque = realPacket->yTorque;
		float direction = realPacket->direction;
		game->MoveGooseForServer(force, yTorque, direction);
	}
	else if (type == Client_Player_Input)
	{
		// Client player has tried to pick up an item.
		game->ServerPickUpItem();
	}
	else if (type == Player_Connected)
	{
		// Start the timer now that a player has joined so the timer and game length will be in sync.
		game->StartTimer();
	}
}
