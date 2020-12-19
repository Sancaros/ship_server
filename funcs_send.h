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