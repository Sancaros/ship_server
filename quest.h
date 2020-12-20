#pragma once
int qflag(unsigned char* flag_data, unsigned flag, unsigned difficulty)
{
	if (flag_data[(difficulty * 0x80) + (flag >> 3)] & (1 << (7 - (flag & 0x07))))
		return 1;
	else
		return 0;
}

int qflag_ep1solo(unsigned char* flag_data, unsigned difficulty)
{
	int i;
	unsigned int quest_flag;

	for (i = 1;i <= 25;i++)
	{
		quest_flag = 0x63 + (i << 1);
		if (!qflag(flag_data, quest_flag, difficulty)) return 0;
	}
	return 1;
}

void SendA2(unsigned char episode, unsigned char solo, unsigned char category, unsigned char gov, BANANA* client)
{
	QUEST_MENU* qm = 0;
	QUEST* q;
	unsigned char qc = 0;
	unsigned short Offset;
	unsigned ch, ch2, ch3, show_quest, quest_flag;
	unsigned char quest_count;
	char quest_num[16];
	int qn, tier1, ep1solo;
	LOBBY* l;
	unsigned char diff;

	if (!client->lobby)
		return;

	l = (LOBBY *)client->lobby;

	memset(&PacketData[0], 0, 0x2510);

	diff = l->difficulty;

	if (l->battle)
	{
		qm = &quest_menus[9];
		qc = 10;
	}
	else
		if (l->challenge)
		{
			//缺失 Sancaros代码,尝试使用相近代码载入
			qm = &quest_menus[10];
			qc = 11;
		}
		else
		{
			switch (episode)
			{
			case 0x01:
				if (gov)
				{
					qm = &quest_menus[6];
					qc = 7;
				}
				else
				{
					if (solo)
					{
						qm = &quest_menus[3];
						qc = 4;
					}
					else
					{
						qm = &quest_menus[0];
						qc = 1;
					}
				}
				break;
			case 0x02:
				if (gov)
				{
					qm = &quest_menus[7];
					qc = 8;
				}
				else
				{
					if (solo)
					{
						qm = &quest_menus[4];
						qc = 5;
					}
					else
					{
						qm = &quest_menus[1];
						qc = 2;
					}
				}
				break;
			case 0x03:
				if (gov)
				{
					qm = &quest_menus[8];
					qc = 9;
				}
				else
				{
					if (solo)
					{
						qm = &quest_menus[5];
						qc = 6;
					}
					else
					{
						qm = &quest_menus[2];
						qc = 3;
					}
				}
				break;
			}
		}

	if ((qm) && (category == 0))
	{
		PacketData[0x02] = 0xA2;
		PacketData[0x04] = qm->num_categories;
		Offset = 0x08;
		for (ch = 0;ch<qm->num_categories;ch++)
		{
			PacketData[Offset + 0x07] = 0x0F;
			PacketData[Offset] = qc;
			PacketData[Offset + 2] = gov;
			PacketData[Offset + 4] = ch + 1;
			memcpy(&PacketData[Offset + 0x08], &qm->c_names[ch][0], 0x40);
			memcpy(&PacketData[Offset + 0x48], &qm->c_desc[ch][0], 0xF0);
			Offset += 0x13C;
		}
	}
	else
	{
		ch2 = 0;
		PacketData[0x02] = 0xA2;
		category--;
		quest_count = 0;
		Offset = 0x08;
		ep1solo = qflag_ep1solo(&client->character.quest_data1[0], diff);
		for (ch = 0;ch<qm->quest_counts[category];ch++)
		{
			q = &quests[qm->quest_indexes[category][ch]];
			show_quest = 0;
			if ((solo) && (episode == 0x01))
			{
				memcpy(&quest_num[0], &q->ql[0]->qdata[49], 3);
				quest_num[4] = 0;
				qn = atoi(&quest_num[0]);
				if ((ep1solo) || (qn > 26))
					show_quest = 1;
				if (!show_quest)
				{
					quest_flag = 0x63 + (qn << 1);
					if (qflag(&client->character.quest_data1[0], quest_flag, diff))
						show_quest = 2; // Sets a variance if they've cleared the quest...
					else
					{
						tier1 = 0;
						if ((qflag(&client->character.quest_data1[0], 0x65, diff)) && // Cleared first tier
							(qflag(&client->character.quest_data1[0], 0x67, diff)) &&
							(qflag(&client->character.quest_data1[0], 0x6B, diff)))
							tier1 = 1;
						if (qflag(&client->character.quest_data1[0], quest_flag, diff) == 0)
						{ // When the quest hasn't been completed... 当任务没有被完成时
						  // Forest quests
							switch (qn)
							{
							case 4: // Battle Training
							case 2: // Claiming a Stake
							case 1: // Magnitude of Metal
								show_quest = 1;
								break;
							case 5: // Journalistic Pursuit
							case 6: // The Fake In Yellow
							case 7: // Native Research
							case 9: // Gran Squall
								if (tier1)
									show_quest = 1;
								break;
							case 8: // Forest of Sorrow
								if (qflag(&client->character.quest_data1[0], 0x71, diff)) // Cleared Native Research
									show_quest = 1;
								break;
							case 26: // Central Dome Fire Swirl
								if (qflag(&client->character.quest_data1[0], 0x73, diff)) // Cleared Forest of Sorrow
									show_quest = 1;
								break;
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0], 0x1F9, diff)))
							{
								// Cave quests (shown after Dragon is defeated)
								switch (qn)
								{
								case 03: // The Value of Money
								case 11: // The Lost Bride
								case 14: // Secret Delivery
								case 17: // Grave's Butler
								case 10: // Addicting Food
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 12: // Waterfall Tears
								case 15: // Soul of a Blacksmith
									if ((qflag(&client->character.quest_data1[0], 0x77, diff)) && // Cleared Addicting Food
										(qflag(&client->character.quest_data1[0], 0x79, diff)) && // Cleared The Lost Bride
										(qflag(&client->character.quest_data1[0], 0x7F, diff)) && // Cleared Secret Delivery
										(qflag(&client->character.quest_data1[0], 0x85, diff)))  // Cleared Grave's Butler
										show_quest = 1;
									break;
								case 13: // Black Paper
									if (qflag(&client->character.quest_data1[0], 0x7B, diff)) // Cleared Waterfall Tears
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0], 0x1FF, diff)))
							{
								// Mine quests (shown after De Rol Le is defeated)
								switch (qn)
								{
								case 16: // Letter from Lionel
								case 18: // Knowing One's Heart
								case 20: // Dr. Osto's Research
									show_quest = 1; // Always shown if first tier was cleared
									break;
								case 21: // The Unsealed Door
									if ((qflag(&client->character.quest_data1[0], 0x8B, diff)) && // Cleared Dr. Osto's Research
										(qflag(&client->character.quest_data1[0], 0x7F, diff)))  // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0], 0x207, diff)))
							{
								// Ruins quests (shown after Vol Opt is defeated)
								switch (qn)
								{
								case 19: // The Retired Hunter
								case 24: // Seek My Master
									show_quest = 1;  // Always shown if first tier was cleared
									break;
								case 25: // From the Depths
								case 22: // Soul of Steel
									if (qflag(&client->character.quest_data1[0], 0x91, diff)) // Cleared Doc's Secret Plan
										show_quest = 1;
									break;
								case 23: // Doc's Secret Plan
									if (qflag(&client->character.quest_data1[0], 0x7F, diff)) // Cleared Secret Delivery
										show_quest = 1;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				show_quest = 1;
				if ((ch) && (gov))
				{
					// Check party's qualification for quests...
					switch (episode)
					{
					case 0x01:
						quest_flag = (0x1F3 + (ch << 1));
						for (ch2 = 0x1F5;ch2 <= quest_flag;ch2 += 2)
							for (ch3 = 0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					case 0x02:
						quest_flag = (0x211 + (ch << 1));
						for (ch2 = 0x213;ch2 <= quest_flag;ch2 += 2)
							for (ch3 = 0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					case 0x03:
						quest_flag = (0x2BC + ch);
						for (ch2 = 0x2BD;ch2 <= quest_flag;ch2++)
							for (ch3 = 0;ch3<4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					}
				}
			}
			if (show_quest)
			{
				PacketData[Offset + 0x07] = 0x0F;
				PacketData[Offset] = qc;
				PacketData[Offset + 1] = 0x01;
				PacketData[Offset + 2] = gov;
				PacketData[Offset + 3] = ((unsigned char)qm->quest_indexes[category][ch]) + 1;
				PacketData[Offset + 4] = category;
				if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				{
					memcpy(&PacketData[Offset + 0x08], &q->ql[client->character.lang]->qname[0], 0x40);
					memcpy(&PacketData[Offset + 0x48], &q->ql[client->character.lang]->qsummary[0], 0xF0);
				}
				else
				{
					memcpy(&PacketData[Offset + 0x08], &q->ql[0]->qname[0], 0x40);
					memcpy(&PacketData[Offset + 0x48], &q->ql[0]->qsummary[0], 0xF0);
				}

				if ((solo) && (episode == 0x01))
				{
					if (qn <= 26)
					{
						*(unsigned short*)&PacketData[Offset + 0x128] = ep1_unlocks[(qn - 1) * 2];
						*(unsigned short*)&PacketData[Offset + 0x12C] = ep1_unlocks[((qn - 1) * 2) + 1];
						*(int*)&PacketData[Offset + 0x130] = qn;
						if (show_quest == 2)
							PacketData[Offset + 0x138] = 1;
					}
				}
				Offset += 0x13C;
				quest_count++;
			}
		}
		PacketData[0x04] = quest_count;
	}
	*(unsigned short*)&PacketData[0] = (unsigned short)Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], Offset);
}