#pragma once
void Send08(BANANA* client)
{
	BLOCK* b;
	unsigned ch, ch2, qNum;
	unsigned char game_flags, total_games;
	LOBBY* l;
	unsigned Offset;
	QUEST* q;

	if (client->block <= serverBlocks)
	{
		total_games = 0;
		b = blocks[client->block - 1];
		Offset = 0x34;
		for (ch = 16;ch<(16 + SHIP_COMPILED_MAX_GAMES);ch++)
		{
			l = &b->lobbies[ch];
			if (l->in_use)
			{
				memset(&PacketData[Offset], 0, 44);
				// Output game
				Offset += 2;
				PacketData[Offset] = 0x03;
				Offset += 2;
				*(unsigned *)&PacketData[Offset] = ch;
				Offset += 4;
				PacketData[Offset++] = 0x22 + l->difficulty;
				PacketData[Offset++] = l->lobbyCount;
				memcpy(&PacketData[Offset], &l->gameName[0], 30);
				Offset += 32;
				if (!l->oneperson)
					PacketData[Offset++] = 0x40 + l->episode;
				else
					PacketData[Offset++] = 0x10 + l->episode;
				if (l->inpquest)
				{
					game_flags = 0x80;
					// Grey out Government quests that the player is not qualified for...sancaros
					q = &quests[l->quest_loaded - 1];
					memcpy(&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qNum = (unsigned)atoi(&dp[0]);
					switch (l->episode)
					{
					case 0x01:
						qNum -= 401;
						qNum <<= 1;
						qNum += 0x1F3;
						for (ch2 = 0x1F5;ch2 <= qNum;ch2 += 2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x02:
						qNum -= 451;
						qNum <<= 1;
						qNum += 0x211;
						for (ch2 = 0x213;ch2 <= qNum;ch2 += 2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					case 0x03:
						qNum -= 701;
						qNum += 0x2BC;
						for (ch2 = 0x2BD;ch2 <= qNum;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								game_flags |= 0x04;
						break;
					}
				}
				else
					game_flags = 0x40;
				// Get flags for battle and one person games...sancaros
				if ((l->gamePassword[0x00] != 0x00) ||
					(l->gamePassword[0x01] != 0x00))
					game_flags |= 0x02;
				if ((l->quest_in_progress) || (l->oneperson)) // Can't join!
					game_flags |= 0x04;
				if (l->battle)
					game_flags |= 0x10;
				if (l->challenge)
					game_flags |= 0x20;
				// Wonder what flags 0x01 and 0x08 control....
				PacketData[Offset++] = game_flags;
				total_games++;
			}
		}
		*(unsigned short*)&client->encryptbuf[0x00] = (unsigned short)Offset;
		memcpy(&client->encryptbuf[0x02], &Packet08[2], 0x32);
		client->encryptbuf[0x04] = total_games;
		client->encryptbuf[0x08] = (unsigned char)client->lobbyNum;
		if (client->block == 10)
		{
			client->encryptbuf[0x1C] = 0x31;
			client->encryptbuf[0x1E] = 0x30;
		}
		else
			client->encryptbuf[0x1E] = 0x30 + client->block;

		if (client->lobbyNum > 9)
			client->encryptbuf[0x24] = 0x30 - (10 - client->lobbyNum);
		else
			client->encryptbuf[0x22] = 0x30 + client->lobbyNum;
		memcpy(&client->encryptbuf[0x34], &PacketData[0x34], Offset - 0x34);
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0x00], Offset);
	}
}

//无法识别中文
void Send1A(const wchar_t *mes, BANANA* client, int line)
{
	if (line>-1 && strlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	}
	unsigned short x1A_Len;

	memcpy(&PacketData[0], &Packet1A[0], sizeof(Packet1A));
	x1A_Len = sizeof(Packet1A);

	while (*mes != 0x00)
	{
		PacketData[x1A_Len++] = *(mes++);
		PacketData[x1A_Len++] = 0x00;
	}

	PacketData[x1A_Len++] = 0x00;
	PacketData[x1A_Len++] = 0x00;

	while (x1A_Len % 8)
		PacketData[x1A_Len++] = 0x00;

	*(unsigned short*)&PacketData[0] = x1A_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], x1A_Len);
}

void Send1D(BANANA* client)
{
	unsigned num_minutes;

	if ((((unsigned)servertime - client->savetime) / 60L) >= 5)
	{
		// Backup character data every 5 minutes.每5分钟备份一次角色数据至数据库
		client->savetime = (unsigned)servertime;
		ShipSend04(0x02, client, logon);
	}

	num_minutes = ((unsigned)servertime - client->response) / 60L;
	if (num_minutes)
	{
		if (num_minutes > 2)
			initialize_connection(client); // If the client hasn't responded in over two minutes, drop the connection.如果客户端没有回应,则断开连接
		else
		{
			cipher_ptr = &client->server_cipher;
			encryptcopy(client, &Packet1D[0], sizeof(Packet1D));
		}
	}
}

void Send83(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &Packet83[0], sizeof(Packet83));

}

void Send64(BANANA* client)
{
	LOBBY* l;
	unsigned Offset;
	unsigned ch;

	if (!client->lobby)
		return;
	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<4;ch++)
	{
		if (!l->slot_use[ch])
		{
			l->slot_use[ch] = 1; // Slot now in use目前使用的角色槽位
			l->client[ch] = client;
			// lobbyNum should be set before joining the game房间号码应该在加入游戏前产生
			client->clientID = ch;
			l->gamePlayerCount++;
			l->gamePlayerID[ch] = l->gamePlayerCount;
			break;
		}
	}
	l->lobbyCount = 0;
	for (ch = 0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			l->lobbyCount++;
	}
	memset(&PacketData[0], 0, 0x1A8);
	PacketData[0x00] = 0xA8;
	PacketData[0x01] = 0x01;
	PacketData[0x02] = 0x64;
	PacketData[0x04] = (unsigned char)l->lobbyCount;
	memcpy(&PacketData[0x08], &l->gameMap[0], 128);
	Offset = 0x88;
	for (ch = 0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
		{
			PacketData[Offset + 2] = 0x01;
			Offset += 0x04;
			*(unsigned *)&PacketData[Offset] = l->client[ch]->guildcard;
			Offset += 0x10;
			PacketData[Offset] = l->client[ch]->clientID;
			Offset += 0x0C;
			memcpy(&PacketData[Offset], &l->client[ch]->character.name[0], 24);
			Offset += 0x20;
			PacketData[Offset] = 0x02;
			Offset += 0x04;
			if ((l->client[ch]->guildcard == client->guildcard) && (l->lobbyCount > 1))
			{
				memset(&PacketData2[0], 0, 1316);
				PacketData2[0x00] = 0x34;
				PacketData2[0x01] = 0x05;
				PacketData2[0x02] = 0x65;
				PacketData2[0x04] = 0x01;
				PacketData2[0x08] = client->clientID;
				PacketData2[0x09] = l->leader;
				PacketData2[0x0A] = 0x01; // ?? 未知
				PacketData2[0x0B] = 0xFF; // ?? 未知
				PacketData2[0x0C] = 0x01; // ?? 未知
				PacketData2[0x0E] = 0x01; // ?? 未知
				PacketData2[0x16] = 0x01;
				*(unsigned *)&PacketData2[0x18] = client->guildcard;
				PacketData2[0x30] = client->clientID;
				memcpy(&PacketData2[0x34], &client->character.name[0], 24);
				PacketData2[0x54] = 0x02; // ?? 未知
				memcpy(&PacketData2[0x58], &client->character.inventoryUse, 0x4DC);
				// Prevent crashing with NPC skins... 防止NPC皮肤崩溃 ,貌似没有成功,会导致玩家人物短暂丢失
				if (client->character.skinFlag)
					memset(&PacketData2[0x58 + 0x3A8], 0, 10);
				SendToLobby(client->lobby, 4, &PacketData2[0x00], 1332, client->guildcard);
			}
		}
	}

	if (l->lobbyCount < 4)
		PacketData[0x194] = 0x02;
	if (l->lobbyCount < 3)
		PacketData[0x150] = 0x02;
	if (l->lobbyCount < 2)
		PacketData[0x10C] = 0x02;

	// Most of the 0x64 packet has been generated... now for the important stuff. =p 大多数的0x64数据包已经生成,目前很重要需要解决
	// Leader ID @ 0x199
	// Difficulty @ 0x19B
	// Event @ 0x19D
	// Section ID of Leader @ 0x19E
	// Game Monster @ 0x1A0 (4 bytes)
	// Episode @ 0x1A4
	// 0x01 @ 0x1A5
	// One-person @ 0x1A6

	PacketData[0x198] = client->clientID;
	PacketData[0x199] = l->leader;
	PacketData[0x19B] = l->difficulty;
	PacketData[0x19C] = l->battle;
	if ((shipEvent < 7) && (shipEvent != 0x02))
		PacketData[0x19D] = shipEvent;
	else
		PacketData[0x19D] = 0;
	PacketData[0x19E] = l->sectionID;
	PacketData[0x19F] = l->challenge;
	*(unsigned *)&PacketData[0x1A0] = *(unsigned *)&l->gameMonster;
	PacketData[0x1A4] = l->episode;
	PacketData[0x1A5] = 0x01; // ??
	PacketData[0x1A6] = l->oneperson;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], 0x1A8);

	/* Let's send the team data... */

	SendToLobby(client->lobby, 4, MakePacketEA15(client), 2152, client->guildcard);

	client->bursting = 1;
}

void Send67(BANANA* client, unsigned char preferred)
{
	BLOCK* b;
	BANANA* lClient;
	LOBBY* l;
	unsigned Offset = 0, Offset2 = 0;
	unsigned ch, ch2;

	if (!client->lobbyOK)
		return;

	client->lobbyOK = 0;

	ch = 0;

	b = blocks[client->block - 1];
	if ((preferred != 0xFF) && (preferred < 0x0F))
	{
		if (b->lobbies[preferred].lobbyCount >= 12)
			preferred = 0x00;
		ch = preferred;
	}

	for (ch = ch;ch<15;ch++)
	{
		l = &b->lobbies[ch];
		if (l->lobbyCount < 12)
		{
			for (ch2 = 0;ch2<12;ch2++)
				if (l->slot_use[ch2] == 0)
				{
					l->slot_use[ch2] = 1;
					l->client[ch2] = client;
					l->arrow_color[ch2] = 0;
					client->lobbyNum = ch + 1;
					client->lobby = (void*)&b->lobbies[ch];
					client->clientID = ch2;
					break;
				}
			// Send68 here with joining client (use ch2 for clientid)
			l->lobbyCount = 0;
			for (ch2 = 0;ch2<12;ch2++)
			{
				if ((l->slot_use[ch2]) && (l->client[ch2]))
					l->lobbyCount++;
			}

			memset(&PacketData[0x00], 0, 0x10);
			PacketData[0x04] = l->lobbyCount;
			PacketData[0x08] = client->clientID;
			PacketData[0x0B] = ch;
			PacketData[0x0C] = client->block;
			PacketData[0x0E] = shipEvent;
			Offset = 0x16;
			for (ch2 = 0;ch2<12;ch2++)
			{
				if ((l->slot_use[ch2]) && (l->client[ch2]))
				{
					memset(&PacketData[Offset], 0, 1316);
					Offset2 = Offset;
					PacketData[Offset++] = 0x01;
					PacketData[Offset++] = 0x00;
					lClient = l->client[ch2];
					*(unsigned *)&PacketData[Offset] = lClient->guildcard;
					Offset += 24;
					*(unsigned *)&PacketData[Offset] = ch2;
					Offset += 4;
					memcpy(&PacketData[Offset], &lClient->character.name[0], 24);
					Offset += 32;
					PacketData[Offset++] = 0x02;
					Offset += 3;
					memcpy(&PacketData[Offset], &lClient->character.inventoryUse, 1246);
					// Prevent crashing with NPCs
					if (lClient->character.skinFlag)
						memset(&PacketData[Offset + 0x3A8], 0, 10);
					Offset += 1246;
					if (lClient->isgm == 1)
						*(unsigned *)&PacketData[Offset2 + 0x3CA] = globalName;
					else
						if (isLocalGM(lClient->guildcard))
							*(unsigned *)&PacketData[Offset2 + 0x3CA] = localName;
						else
							*(unsigned *)&PacketData[Offset2 + 0x3CA] = normalName;
					if ((lClient->guildcard == client->guildcard) && (l->lobbyCount > 1))
					{
						memcpy(&PacketData2[0x00], &PacketData[0], 0x16);
						PacketData2[0x00] = 0x34;
						PacketData2[0x01] = 0x05;
						PacketData2[0x02] = 0x68;
						PacketData2[0x04] = 0x01;
						memcpy(&PacketData2[0x16], &PacketData[Offset2], 1316);
						SendToLobby(client->lobby, 12, &PacketData2[0x00], 1332, client->guildcard);
					}
				}
			}
			*(unsigned short*)&PacketData[0] = (unsigned short)Offset;
			PacketData[2] = 0x67;
			break;
		}
	}

	if (Offset > 0)
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &PacketData[0], Offset);
	}

	// Quest data

	memset(&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0] = 0x10;
	client->encryptbuf[1] = 0x02;
	client->encryptbuf[2] = 0x60;
	client->encryptbuf[8] = 0x6F;
	client->encryptbuf[9] = 0x84;
	memcpy(&client->encryptbuf[0x0C], &client->character.quest_data1[0], 0x210);
	memset(&client->encryptbuf[0x20C], 0, 4);
	encryptcopy(client, &client->encryptbuf[0x00], 0x210);

	/* Let's send the team data... */

	SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, client->guildcard);

	client->bursting = 1;
}

void Send95(BANANA* client)
{
	client->lobbyOK = 1;
	memset(&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0x00] = 0x08;
	client->encryptbuf[0x02] = 0x95;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &client->encryptbuf[0], 8);
	// Restore permanent character...
	if (client->character_backup)
	{
		if (client->mode)
		{
			memcpy(&client->character, client->character_backup, sizeof(client->character));
			client->mode = 0;
		}
		free(client->character_backup);
		client->character_backup = NULL;
	}
}

void SendA0(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketA0Data[0], *(unsigned short *)&PacketA0Data[0]);
}


void Send07(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &Packet07Data[0], *(unsigned short *)&Packet07Data[0]);
}


void SendB0(const wchar_t *mes, BANANA* client, int line)
{
	if (line>-1 && strlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	}
	unsigned short xB0_Len;

	memcpy(&PacketData[0], &PacketB0[0], sizeof(PacketB0));
	xB0_Len = sizeof(PacketB0);

	while (*mes != 0x00)
	{
		PacketData[xB0_Len++] = *(mes++);
		PacketData[xB0_Len++] = 0x00;
	}

	PacketData[xB0_Len++] = 0x00;
	PacketData[xB0_Len++] = 0x00;

	while (xB0_Len % 8)
		PacketData[xB0_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xB0_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], xB0_Len);
}

void SendEE(const wchar_t *mes, BANANA* client, int line)
{
	if (line>-1 && strlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	}
	unsigned short xEE_Len;

	memcpy(&PacketData[0], &PacketEE[0], sizeof(PacketEE));
	xEE_Len = sizeof(PacketEE);

	while (*mes != 0x00)
	{
		PacketData[xEE_Len++] = *(mes++);
		PacketData[xEE_Len++] = 0x00;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xEE_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], xEE_Len);
}

void Send19(unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4, unsigned short ipp, BANANA* client)
{
	memcpy(&client->encryptbuf[0], &Packet19, sizeof(Packet19));
	client->encryptbuf[0x08] = ip1;
	client->encryptbuf[0x09] = ip2;
	client->encryptbuf[0x0A] = ip3;
	client->encryptbuf[0x0B] = ip4;
	*(unsigned short*)&client->encryptbuf[0x0C] = ipp;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &client->encryptbuf[0], sizeof(Packet19));
}

void SendEA(unsigned char command, BANANA* client)
{
	switch (command)
	{
	case 0x02:
		memset(&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 8);
		break;
	case 0x04:
		memset(&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x04;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 8);
		break;
	case 0x0E:
		memset(&client->encryptbuf[0x00], 0, 0x38);
		client->encryptbuf[0x00] = 0x38;
		client->encryptbuf[0x01] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x0E;
		*(unsigned *)&client->encryptbuf[0x08] = client->guildcard;
		*(unsigned *)&client->encryptbuf[0x0C] = client->character.teamID;
		memcpy(&client->encryptbuf[0x18], &client->character.teamName[0], 28);
		client->encryptbuf[0x34] = 0x84;
		client->encryptbuf[0x35] = 0x6C;
		memcpy(&client->encryptbuf[0x36], &client->character.teamFlag[0], 0x800);
		client->encryptbuf[0x836] = 0xFF;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x838);
		break;
	case 0x10:
		memset(&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x10;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 8);
		break;
	case 0x11:
		memset(&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x11;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 8);
		break;
	case 0x12:
		memset(&client->encryptbuf[0x00], 0, 0x40);
		client->encryptbuf[0x00] = 0x40;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x12;
		if (client->character.teamID)
		{
			*(unsigned *)&client->encryptbuf[0x0C] = client->guildcard;
			*(unsigned *)&client->encryptbuf[0x10] = client->character.teamID;
			client->encryptbuf[0x1C] = (unsigned char)client->character.privilegeLevel;
			memcpy(&client->encryptbuf[0x20], &client->character.teamName[0], 28);
			client->encryptbuf[0x3C] = 0x84;
			client->encryptbuf[0x3D] = 0x6C;
			client->encryptbuf[0x3E] = 0x98;
		}
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x40);
		/*
		if ( client->lobbyNum < 0x10 ) //这里有关于创建房间的代码
		SendToLobby ( client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
		else
		SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
		*/
		break;
	case 0x13:
	{
		LOBBY *l;
		BANANA *lClient;
		unsigned ch, total_clients, EA15Offset, maxc;

		if (!client->lobby)
			break;

		l = (LOBBY*)client->lobby;

		if (client->lobbyNum < 0x10)
			maxc = 12;
		else
			maxc = 4;
		EA15Offset = 0x08;
		total_clients = 0;
		for (ch = 0;ch<maxc;ch++)
		{
			if ((l->slot_use[ch]) && (l->client[ch]))
			{
				lClient = l->client[ch];
				*(unsigned *)&client->encryptbuf[EA15Offset] = lClient->character.guildCard2;
				EA15Offset += 0x04;
				*(unsigned *)&client->encryptbuf[EA15Offset] = lClient->character.teamID;
				EA15Offset += 0x04;
				memset(&client->encryptbuf[EA15Offset], 0, 8);
				EA15Offset += 0x08;
				client->encryptbuf[EA15Offset] = (unsigned char)lClient->character.privilegeLevel;
				EA15Offset += 4;
				memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamName[0], 28);
				EA15Offset += 28;
				if (lClient->character.teamID != 0)
				{
					client->encryptbuf[EA15Offset++] = 0x84;
					client->encryptbuf[EA15Offset++] = 0x6C;
					client->encryptbuf[EA15Offset++] = 0x98;
					client->encryptbuf[EA15Offset++] = 0x00;
				}
				else
				{
					memset(&client->encryptbuf[EA15Offset], 0, 4);
					EA15Offset += 4;
				}
				*(unsigned *)&client->encryptbuf[EA15Offset] = lClient->character.guildCard;
				EA15Offset += 4;
				client->encryptbuf[EA15Offset++] = lClient->clientID;
				memset(&client->encryptbuf[EA15Offset], 0, 3);
				EA15Offset += 3;
				memcpy(&client->encryptbuf[EA15Offset], &lClient->character.name[0], 24);
				EA15Offset += 24;
				memset(&client->encryptbuf[EA15Offset], 0, 8);
				EA15Offset += 8;
				memcpy(&client->encryptbuf[EA15Offset], &lClient->character.teamFlag[0], 0x800);
				EA15Offset += 0x800;
				total_clients++;
			}
		}
		*(unsigned short*)&client->encryptbuf[0x00] = (unsigned short)EA15Offset;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x13;
		*(unsigned*)&client->encryptbuf[0x04] = total_clients;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], EA15Offset);
	}
	break;
	case 0x18:
		memset(&client->encryptbuf[0x00], 0, 0x4C);
		client->encryptbuf[0x00] = 0x4C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x18;
		client->encryptbuf[0x14] = 0x01;
		client->encryptbuf[0x18] = 0x01;
		client->encryptbuf[0x1C] = (unsigned char)client->character.privilegeLevel;
		*(unsigned *)&client->encryptbuf[0x20] = client->character.guildCard;
		memcpy(&client->encryptbuf[0x24], &client->character.name[0], 24);
		client->encryptbuf[0x48] = 0x02;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x4C);
		break;
	case 0x19:
		memset(&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x19;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1A:
		memset(&client->encryptbuf[0x00], 0, 0x0C);
		client->encryptbuf[0x00] = 0x0C;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1A;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x0C);
		break;
	case 0x1D:
		memset(&client->encryptbuf[0x00], 0, 8);
		client->encryptbuf[0x00] = 0x08;
		client->encryptbuf[0x02] = 0xEA;
		client->encryptbuf[0x03] = 0x1D;
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 8);
		break;
	default:
		break;
	}
}

void ShipSend04(unsigned char command, BANANA* client, ORANGE* ship)
{
	//unsigned ch; //为何不做查询

	ship->encryptbuf[0x00] = 0x04;
	switch (command)
	{
	case 0x00:
		// Request character data from server 从服务器请求角色数据
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *)&ship->encryptbuf[0x02] = client->guildcard;
		*(unsigned short *)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
		*(int *)&ship->encryptbuf[0x08] = client->plySockfd;
		*(unsigned *)&ship->encryptbuf[0x0C] = serverID;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x10);
		break;
	case 0x02:
		// Send character data to server when not using a temporary character. 在不使用临时角色的情况下将角色数据发送到服务器
		if ((!client->mode) && (client->gotchardata == 1))
		{
			ship->encryptbuf[0x01] = 0x02;
			*(unsigned *)&ship->encryptbuf[0x02] = client->guildcard;
			*(unsigned short*)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
			memcpy(&ship->encryptbuf[0x08], &client->character.packetSize, sizeof(CHARDATA));

			// Include character bank in packet 包含角色银行数据
			memcpy(&ship->encryptbuf[0x08 + 0x700], &client->char_bank, sizeof(BANK));

			// Include common bank in packet 包含公共银行数据
			memcpy(&ship->encryptbuf[0x08 + sizeof(CHARDATA)], &client->common_bank, sizeof(BANK));

			compressShipPacket(ship, &ship->encryptbuf[0x00], sizeof(BANK) + sizeof(CHARDATA) + 8);
		}
		break;
	}
}

void ShipSend0E(ORANGE* ship)
{
	if (logon_ready) //准备登陆
	{
		ship->encryptbuf[0x00] = 0x0E;
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned *)&ship->encryptbuf[0x02] = serverID;
		*(unsigned *)&ship->encryptbuf[0x06] = serverNumConnections;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
	}
}

void ShipSend0D(unsigned char command, BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0D;
	switch (command)
	{
	case 0x00:
		// Requesting ship list.请求舰船列表
		ship->encryptbuf[0x01] = 0x00;
		*(int *)&ship->encryptbuf[0x02] = client->plySockfd;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 6);
		break;
	default:
		break;
	}
}

void ShipSend0B(BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0B;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned *)&ship->encryptbuf[0x02] = *(unsigned *)&client->decryptbuf[0x0C];
	*(unsigned *)&ship->encryptbuf[0x06] = *(unsigned *)&client->decryptbuf[0x18];
	*(long long *)&ship->encryptbuf[0x0A] = *(long long*)&client->decryptbuf[0x8C];
	*(long long *)&ship->encryptbuf[0x12] = *(long long*)&client->decryptbuf[0x94];
	*(long long *)&ship->encryptbuf[0x1A] = *(long long*)&client->decryptbuf[0x9C];
	*(long long *)&ship->encryptbuf[0x22] = *(long long*)&client->decryptbuf[0xA4];
	*(long long *)&ship->encryptbuf[0x2A] = *(long long*)&client->decryptbuf[0xAC];
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x32);
}