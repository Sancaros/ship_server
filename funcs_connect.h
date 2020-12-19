#pragma once
unsigned free_connection()
{
	unsigned fc;
	BANANA* wc;

	for (fc = 0;fc<serverMaxConnections;fc++)
	{
		wc = connections[fc];
		if (wc->plySockfd<0)
			return fc;
	}
	return 0xFFFF;
}

void initialize_connection(BANANA* connect)
{
	unsigned ch, ch2;

	// Free backup character memory

	if (connect->character_backup)
	{
		if (connect->mode)
			memcpy(&connect->character, connect->character_backup, sizeof(connect->character));
		free(connect->character_backup);
		connect->character_backup = NULL;
	}

	if (connect->guildcard)
	{
		removeClientFromLobby(connect);

		if ((connect->block) && (connect->block <= serverBlocks))
			blocks[connect->block - 1]->count--;

		if (connect->gotchardata == 1)
		{
			connect->character.playTime += (unsigned)servertime - connect->connected;
			ShipSend04(0x02, connect, logon);
		}
	}

	if (connect->plySockfd >= 0)
	{
		ch2 = 0;
		for (ch = 0;ch<serverNumConnections;ch++)
		{
			if (serverConnectionList[ch] != connect->connection_index)
				serverConnectionList[ch2++] = serverConnectionList[ch];
		}
		serverNumConnections = ch2;
		closesocket(connect->plySockfd);
	}

	if (logon_ready)
	{
		printf("玩家数量: %u\n", serverNumConnections);
		ShipSend0E(logon);
	}

	memset(connect, 0, sizeof(BANANA));
	connect->plySockfd = -1;
	connect->block = -1;
	connect->lastTick = 0xFFFFFFFF;
	connect->slotnum = -1;
	connect->sending_quest = -1;
}

void start_encryption(BANANA* connect)
{
	unsigned c, c3, c4, connectNum;
	BANANA *workConnect, *c5;

	// Limit the number of connections from an IP address to MAX_SIMULTANEOUS_CONNECTIONS.

	c3 = 0;

	for (c = 0;c<serverNumConnections;c++)
	{
		connectNum = serverConnectionList[c];
		workConnect = connections[connectNum];
		//debug ("%s comparing to %s", (char*) &workConnect->IP_Address[0], (char*) &connect->IP_Address[0]);
		if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
			(workConnect->plySockfd >= 0))
			c3++;
	}

	//debug ("Matching count: %u", c3);

	if (c3 > MAX_SIMULTANEOUS_CONNECTIONS)
	{
		// More than MAX_SIMULTANEOUS_CONNECTIONS connections from a certain IP address...
		// Delete oldest connection to server.
		c4 = 0xFFFFFFFF;
		c5 = NULL;
		for (c = 0;c<serverNumConnections;c++)
		{
			connectNum = serverConnectionList[c];
			workConnect = connections[connectNum];
			if ((!strcmp(&workConnect->IP_Address[0], &connect->IP_Address[0])) &&
				(workConnect->plySockfd >= 0))
			{
				if (workConnect->connected < c4)
				{
					c4 = workConnect->connected;
					c5 = workConnect;
				}
			}
		}
		if (c5)
		{
			workConnect = c5;
			initialize_connection(workConnect);
		}
	}

	memcpy(&connect->sndbuf[0], &Packet03[0], sizeof(Packet03));
	for (c = 0;c<0x30;c++)
	{
		connect->sndbuf[0x68 + c] = (unsigned char)mt_lrand() % 255;
		connect->sndbuf[0x98 + c] = (unsigned char)mt_lrand() % 255;
	}
	connect->snddata += sizeof(Packet03);
	cipher_ptr = &connect->server_cipher;
	pso_crypt_table_init_bb(cipher_ptr, &connect->sndbuf[0x68]);
	cipher_ptr = &connect->client_cipher;
	pso_crypt_table_init_bb(cipher_ptr, &connect->sndbuf[0x98]);
	connect->crypt_on = 1;
	connect->sendCheck[SEND_PACKET_03] = 1;
	connect->connected = connect->response = connect->savetime = (unsigned)servertime;
}

void SendToLobby(LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend)
{
	unsigned ch;

	if (!l)
		return;

	for (ch = 0;ch<max_send;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->guildcard != nosend))
		{
			cipher_ptr = &l->client[ch]->server_cipher;
			encryptcopy(l->client[ch], src, size);
		}
	}
}

//缺少房间类型判断
void removeClientFromLobby(BANANA* client)
{
	unsigned ch, maxch, lowestID;

	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	if (client->clientID < 12)
	{
		l->slot_use[client->clientID] = 0;
		l->client[client->clientID] = 0;
	}

	if (client->lobbyNum > 0x0F)
		maxch = 4;
	else
		maxch = 12;

	l->lobbyCount = 0;

	for (ch = 0;ch<maxch;ch++)
	{
		if ((l->client[ch]) && (l->slot_use[ch]))
			l->lobbyCount++;
	}

	if (l->lobbyCount)
	{
		if (client->lobbyNum < 0x10)
		{
			Packet69[0x08] = client->clientID;
			SendToLobby(client->lobby, 12, &Packet69[0], 0x0C, client->guildcard);
		}
		else
		{
			Packet66[0x08] = client->clientID;
			if (client->clientID == l->leader)
			{
				// Leader change...
				lowestID = 0xFFFFFFFF;
				for (ch = 0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->gamePlayerID[ch] < lowestID))
					{
						// Change leader to oldest person to join game...
						lowestID = l->gamePlayerID[ch];
						l->leader = ch;
					}
				}
				Packet66[0x0A] = 0x01;
			}
			else
				Packet66[0x0A] = 0x00;
			Packet66[0x09] = l->leader;
			SendToLobby(client->lobby, 4, &Packet66[0], 0x0C, client->guildcard);
		}
	}
	else
		memset(l, 0, sizeof(LOBBY));
	client->hasquest = 0;
	client->lobbyNum = 0;
	client->lobby = 0;
}