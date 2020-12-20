#pragma once
unsigned free_game_item(LOBBY* l)
{
	unsigned ch, ch2, oldest_item;

	ch2 = oldest_item = 0xFFFFFFFF;

	// If the itemid at the current index is 0, just return that...如果物品ID在当前索引为0,则直接返回

	if ((l->gameItemCount < MAX_SAVED_ITEMS) && (l->gameItem[l->gameItemCount].item.itemid == 0))
		return l->gameItemCount;

	// Scan the gameItem array for any free item slots... 扫描gameItem数组中是否有可用的物品槽 

	for (ch = 0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if (l->gameItem[ch].item.itemid == 0)
		{
			ch2 = ch;
			break;
		}
	}

	if (ch2 != 0xFFFFFFFF)
		return ch2;

	// Out of inventory memory!  Time to delete the oldest dropped item in the game... 库存内存不足！是时候删除游戏中最旧的掉落物品了

	for (ch = 0;ch<MAX_SAVED_ITEMS;ch++)
	{
		if ((l->gameItem[ch].item.itemid < oldest_item) && (l->gameItem[ch].item.itemid >= 0x810000))
		{
			ch2 = ch;
			oldest_item = l->gameItem[ch].item.itemid;
		}
	}

	if (ch2 != 0xFFFFFFFF)
	{
		l->gameItem[ch2].item.itemid = 0; // Item deleted. 物品删除
		return ch2;
	}

	for (ch = 0;ch<4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			SendEE(L"Lobby inventory problem!  It's advised you quit this game and recreate it.", l->client[ch], 134);
	}

	return 0;
}

void UpdateGameItem(BANANA* client)
{
	// Updates the game item list for all of the client's items...  (Used strictly when a client joins a game...) 更新所有客户端项目的游戏项目列表（当客户加入游戏时严格使用…）

	LOBBY* l;
	unsigned ch;

	memset(&client->tekked, 0, sizeof(INVENTORY_ITEM)); // Reset tekking data 重做上次撤消的操作

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<client->character.inventoryUse;ch++) // By default this should already be sorted at the top, so no need for an in_use check...	 默认情况下，这应该已经在顶部排序，所以不需要使用中的检查 
		client->character.inventory[ch].item.itemid = l->playerItemID[client->clientID]++; // Keep synchronized 保持同步
}

// FFFF

INVENTORY_ITEM sort_data[30]; //背包物品数据量
BANK_ITEM bank_data[200]; //银行物品数据量

void SortClientItems(BANANA* client)
{
	unsigned ch, ch2, ch3, ch4, itemid;

	ch2 = 0x0C;

	memset(&sort_data[0], 0, sizeof(INVENTORY_ITEM) * 30);

	for (ch4 = 0;ch4<30;ch4++)
	{
		sort_data[ch4].item.data[1] = 0xFF;
		sort_data[ch4].item.itemid = 0xFFFFFFFF;
	}

	ch4 = 0;

	for (ch = 0;ch<30;ch++)
	{
		itemid = *(unsigned *)&client->decryptbuf[ch2];
		ch2 += 4;
		if (itemid != 0xFFFFFFFF)
		{
			for (ch3 = 0;ch3<client->character.inventoryUse;ch3++)
			{
				if ((client->character.inventory[ch3].in_use) && (client->character.inventory[ch3].item.itemid == itemid))
				{
					sort_data[ch4++] = client->character.inventory[ch3];
					break;
				}
			}
		}
	}

	for (ch = 0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];

}

void CleanUpBank(BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset(&bank_data[0], 0, sizeof(BANK_ITEM) * 200);

	for (ch2 = 0;ch2<200;ch2++)
		bank_data[ch2].itemid = 0xFFFFFFFF;

	ch2 = 0;

	for (ch = 0;ch<200;ch++)
	{
		if (client->character.bankInventory[ch].itemid != 0xFFFFFFFF)
			bank_data[ch2++] = client->character.bankInventory[ch];
	}

	client->character.bankUse = ch2;

	for (ch = 0;ch<200;ch++)
		client->character.bankInventory[ch] = bank_data[ch];

}

void CleanUpInventory(BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset(&sort_data[0], 0, sizeof(INVENTORY_ITEM) * 30);

	ch2 = 0;

	for (ch = 0;ch<30;ch++)
	{
		if (client->character.inventory[ch].in_use)
			sort_data[ch2++] = client->character.inventory[ch];
	}

	client->character.inventoryUse = ch2;

	for (ch = 0;ch<30;ch++)
		client->character.inventory[ch] = sort_data[ch];
}

void CleanUpGameInventory(LOBBY* l)
{
	unsigned ch, item_count;

	ch = item_count = 0;

	while (ch < l->gameItemCount)
	{
		// Combs the entire game inventory for items in use
		if (l->gameItemList[ch] != 0xFFFFFFFF)
		{
			if (ch > item_count)
				l->gameItemList[item_count] = l->gameItemList[ch];
			item_count++;
		}
		ch++;
	}

	if (item_count < MAX_SAVED_ITEMS)
		memset(&l->gameItemList[item_count], 0xFF, ((MAX_SAVED_ITEMS - item_count) * 4));

	l->gameItemCount = item_count;
}

unsigned AddItemToClient(unsigned itemid, BANANA* client)
{
	unsigned ch, itemNum = 0;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned count, stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	LOBBY* l;

	// Adds an item to the client's character data, but only if the item exists in the game item data 将物品添加到客户端的字符数据中,但前提是该项存在于游戏物品数据中时 
	// to begin with.

	if (!client->lobby)
		return 0;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<l->gameItemCount;ch++)
	{
		itemNum = l->gameItemList[ch]; // Lookup table for faster searching...快速查找索引表
		if (l->gameItem[itemNum].item.itemid == itemid)
		{
			if (l->gameItem[itemNum].item.data[0] == 0x04)
			{
				// Meseta...美赛塔最高限制
				count = *(unsigned *)&l->gameItem[itemNum].item.data2[0];
				client->character.meseta += count;
				if (client->character.meseta > 999999)
					client->character.meseta = 999999;
				item_added = 1;
			}
			else
				if (l->gameItem[itemNum].item.data[0] == 0x03)
					stackable = stackable_table[l->gameItem[itemNum].item.data[1]];
			if ((stackable) && (!l->gameItem[itemNum].item.data[5]))
				l->gameItem[itemNum].item.data[5] = 1;
			found_item = ch;
			break;
		}
	}

	if (found_item != -1) // We won't disconnect if the item isn't found because there's a possibility another
	{						// person may have nabbed it before our client due to lag...如果找不到物品,我们不会断开连接,因为有可能是另一个人在我们的客户端之前因为通信延迟而抢走了它
		if ((item_added == 0) && (stackable))
		{
			memcpy(&compare_item1, &l->gameItem[itemNum].item.data[0], 3);
			for (ch = 0;ch<client->character.inventoryUse;ch++)
			{
				memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					count = l->gameItem[itemNum].item.data[5];

					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;

					//if ((stack_count + count) > stackable)
					//{//缺失 Sancaros
					//Send1A(L"Trying to stack over the limit...", client, 59);
					//client->todc = 1;
					//}
					else
					{
						// Add item to the client's current count...将物品添加到客户端的当前计数
						client->character.inventory[ch].item.data[5] = (unsigned char)(stack_count + count);
						item_added = 1;
					}
					break;
				}
			}
		}

		if ((!client->todc) && (item_added == 0)) // Make sure the client isn't trying to pick up more than 30 items... 确保客户端不尝试收集超过30个物品 
		{
			//if (client->character.inventoryUse >= 30)
			//{//缺失 Sancaros
			//Send1A(L"Inventory limit reached.", client, 60);
			//client->todc = 1;
			//}
			//else
			{
				// Give item to client...将物品发送给客户端
				client->character.inventory[client->character.inventoryUse].in_use = 0x01;
				client->character.inventory[client->character.inventoryUse].flags = 0x00;
				memcpy(&client->character.inventory[client->character.inventoryUse].item, &l->gameItem[itemNum].item, sizeof(ITEM));
				client->character.inventoryUse++;
				item_added = 1;
			}
		}

		if (item_added)
		{
			// Delete item from game's inventory
			memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
			l->gameItemList[found_item] = 0xFFFFFFFF;
			CleanUpGameInventory(l);
		}
	}
	return item_added;
}

void DeleteMesetaFromClient(unsigned count, unsigned drop, BANANA* client)
{
	unsigned stack_count, newItemNum;
	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	stack_count = client->character.meseta;
	if (stack_count < count)
	{
		client->character.meseta = 0;
		count = stack_count;
	}
	else
		client->character.meseta -= count;
	if (drop)
	{
		memset(&PacketData[0x00], 0, 16);
		PacketData[0x00] = 0x2C;
		PacketData[0x01] = 0x00;
		PacketData[0x02] = 0x60;
		PacketData[0x03] = 0x00;
		PacketData[0x08] = 0x5D;
		PacketData[0x09] = 0x09;
		PacketData[0x0A] = client->clientID;
		*(unsigned *)&PacketData[0x0C] = client->drop_area;
		*(long long *)&PacketData[0x10] = client->drop_coords;
		PacketData[0x18] = 0x04;
		PacketData[0x19] = 0x00;
		PacketData[0x1A] = 0x00;
		PacketData[0x1B] = 0x00;
		memset(&PacketData[0x1C], 0, 0x08);
		*(unsigned *)&PacketData[0x24] = l->playerItemID[client->clientID];
		*(unsigned *)&PacketData[0x28] = count;
		SendToLobby(client->lobby, 4, &PacketData[0], 0x2C, 0);

		// 生成新的游戏物品...

		newItemNum = free_game_item(l);
		if (l->gameItemCount < MAX_SAVED_ITEMS)
			l->gameItemList[l->gameItemCount++] = newItemNum;
		memcpy(&l->gameItem[newItemNum].item.data[0], &PacketData[0x18], 12);
		*(unsigned *)&l->gameItem[newItemNum].item.data2[0] = count;
		l->gameItem[newItemNum].item.itemid = l->playerItemID[client->clientID];
		l->playerItemID[client->clientID]++;
	}
}


void SendItemToEnd(unsigned itemid, BANANA* client)
{
	unsigned ch;
	INVENTORY_ITEM i;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			i = client->character.inventory[ch];
			i.flags = 0x00;
			client->character.inventory[ch].in_use = 0;
			break;
		}
	}

	CleanUpInventory(client);

	// 给客户端发送物品.

	client->character.inventory[client->character.inventoryUse] = i;
	client->character.inventoryUse++;
}


void DeleteItemFromClient(unsigned itemid, unsigned count, unsigned drop, BANANA* client)
{
	unsigned ch, ch2, itemNum;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// 从客户端的角色数据中删除物品.

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
			{
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];
				if ((stackable) && (!count) && (!drop))
					count = 1;
			}

			if ((stackable) && (count))
			{
				stack_count = client->character.inventory[ch].item.data[5];
				if (!stack_count)
					stack_count = 1;

				if (stack_count < count)
				{//缺失 Sancaros
					Send1A(L"Trying to delete more items than posssessed!", client, 61);
					client->todc = 1;
				}
				else
				{
					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char)stack_count;

					if (!stack_count)
						delete_item = 1;

					if (drop)
					{
						memset(&PacketData[0x00], 0, 16);
						PacketData[0x00] = 0x28;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5D;
						PacketData[0x09] = 0x08;
						PacketData[0x0A] = client->clientID;
						*(unsigned *)&PacketData[0x0C] = client->drop_area;
						*(long long *)&PacketData[0x10] = client->drop_coords;
						memcpy(&PacketData[0x18], &client->character.inventory[ch].item.data[0], 12);
						PacketData[0x1D] = (unsigned char)count;
						*(unsigned *)&PacketData[0x24] = l->playerItemID[client->clientID];

						SendToLobby(client->lobby, 4, &PacketData[0], 0x28, 0);

						// Generate new game item...

						itemNum = free_game_item(l);
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memcpy(&l->gameItem[itemNum].item.data[0], &PacketData[0x18], 12);
						l->gameItem[itemNum].item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
					}
				}
			}
			else
			{
				delete_item = 1; // Not splitting a stack, item goes byebye from character's inventory.
				if (drop) // Client dropped the item on the floor?
				{
					// Copy to game's inventory
					itemNum = free_game_item(l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memcpy(&l->gameItem[itemNum].item, &client->character.inventory[ch].item, sizeof(ITEM));
				}
			}

			if (delete_item)
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}
	//sancaros
	//if (found_item == -1)
	//{
	//Send1A(L"Could not find item to delete.", client, 62);
	//client->todc = 1;
	//}
	//else
	CleanUpInventory(client);
	//此处代码有误,会导致100错误
}

unsigned WithdrawFromBank(unsigned itemid, unsigned count, BANANA* client)
{
	unsigned ch;
	int found_item = -1;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned delete_item = 0;
	LOBBY* l;

	// Adds an item to the client's character from it's bank, if the item is really there...

	if (!client->lobby)
		return 0;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<client->character.bankUse;ch++)
	{
		if (client->character.bankInventory[ch].itemid == itemid)
		{
			found_item = ch;
			if (client->character.bankInventory[ch].data[0] == 0x03)
			{
				stackable = stackable_table[client->character.bankInventory[ch].data[1]];

				if (stackable)
				{
					if (!count)
						count = 1;

					stack_count = (client->character.bankInventory[ch].bank_count & 0xFF);
					if (!stack_count)
						stack_count = 1;

					if (stack_count < count) // Naughty!
					{//缺失 Sancaros
						Send1A(L"Trying to pull a fast one on the bank teller.", client, 63);
						client->todc = 1;
						found_item = -1;
					}
					else
					{
						stack_count -= count;
						client->character.bankInventory[ch].bank_count = 0x10000 + stack_count;
						if (!stack_count)
							delete_item = 1;
					}
				}
			}
			break;
		}
	}

	if (found_item != -1)
	{
		if (stackable)
		{
			memcpy(&compare_item1, &client->character.bankInventory[found_item].data[0], 3);
			for (ch = 0;ch<client->character.inventoryUse;ch++)
			{
				memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ((stack_count + count) > stackable)
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char)(stack_count + count);
					item_added = 1;
					break;
				}
			}
		}

		if ((!client->todc) && (item_added == 0)) // Make sure the client isn't trying to withdraw more than 30 items...
		{
			/*if (client->character.inventoryUse >= 30)
			{//缺失 Sancaros
			Send1A(L"Inventory limit reached.", client, 60);
			dont_send = 1;
			//client->todc = 1;
			}
			else*/
			//{
			// Give item to client...
			client->character.inventory[client->character.inventoryUse].in_use = 0x01;
			client->character.inventory[client->character.inventoryUse].flags = 0x00;
			memcpy(&client->character.inventory[client->character.inventoryUse].item, &client->character.bankInventory[found_item].data[0], sizeof(ITEM));
			if (stackable)
			{
				memset(&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
				client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char)count;
			}
			client->character.inventory[client->character.inventoryUse].item.itemid = l->itemID;
			client->character.inventoryUse++;
			item_added = 1;
			//debug ("Item added to client...");
			//}
		}

		if (item_added)
		{
			// Let people know the client has a new toy...
			memset(&client->encryptbuf[0x00], 0, 0x24);
			client->encryptbuf[0x00] = 0x24;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0xBE;
			client->encryptbuf[0x09] = 0x09;
			client->encryptbuf[0x0A] = client->clientID;
			memcpy(&client->encryptbuf[0x0C], &client->character.bankInventory[found_item].data[0], 12);
			*(unsigned *)&client->encryptbuf[0x18] = l->itemID;
			l->itemID++;
			if (!stackable)
				*(unsigned *)&client->encryptbuf[0x1C] = *(unsigned *)&client->character.bankInventory[found_item].data2[0];
			else
				client->encryptbuf[0x11] = count;
			memset(&client->encryptbuf[0x20], 0, 4);
			SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x24, 0);
			if ((delete_item) || (!stackable))
				// Delete item from bank inventory
				client->character.bankInventory[found_item].itemid = 0xFFFFFFFF;
		}
		CleanUpBank(client);
	}
	else
	{
		//缺失 Sancaros
		Send1A(L"Could not find bank item to withdraw.", client, 64);
		client->todc = 1;
	}
	return item_added;
}

void SortBankItems(BANANA* client)
{
	unsigned ch, ch2;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned char swap_c;
	BANK_ITEM swap_item;
	BANK_ITEM b1;
	BANK_ITEM b2;

	if (client->character.bankUse > 1)
	{
		for (ch = 0;ch<client->character.bankUse - 1;ch++)
		{
			memcpy(&b1, &client->character.bankInventory[ch], sizeof(BANK_ITEM));
			swap_c = b1.data[0];
			b1.data[0] = b1.data[2];
			b1.data[2] = swap_c;
			memcpy(&compare_item1, &b1.data[0], 3);
			for (ch2 = ch + 1;ch2<client->character.bankUse;ch2++)
			{
				memcpy(&b2, &client->character.bankInventory[ch2], sizeof(BANK_ITEM));
				swap_c = b2.data[0];
				b2.data[0] = b2.data[2];
				b2.data[2] = swap_c;
				memcpy(&compare_item2, &b2.data[0], 3);
				if (compare_item2 < compare_item1) // compare_item2 should take compare_item1's place
				{
					memcpy(&swap_item, &client->character.bankInventory[ch], sizeof(BANK_ITEM));
					memcpy(&client->character.bankInventory[ch], &client->character.bankInventory[ch2], sizeof(BANK_ITEM));
					memcpy(&client->character.bankInventory[ch2], &swap_item, sizeof(BANK_ITEM));
					memcpy(&compare_item1, &compare_item2, 3);
				}
			}
		}
	}
}

void DepositIntoBank(unsigned itemid, unsigned count, BANANA* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned deposit_item = 0, deposit_done = 0;
	unsigned delete_item = 0;
	unsigned stack_count;

	// Moves an item from the client's character to it's bank.

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if (stackable)
			{
				if (!count)
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];

				if (!stack_count)
					stack_count = 1;

				if (stack_count < count)
				{//缺失 Sancaros
					Send1A(L"Trying to deposit more items than in possession.", client, 65);
					client->todc = 1; // Tried to deposit more than had?
				}
				else
				{
					deposit_item = 1;

					stack_count -= count;
					client->character.inventory[ch].item.data[5] = (unsigned char)stack_count;

					if (!stack_count)
						delete_item = 1;
				}
			}
			else
			{
				// Not stackable, remove from client completely.
				deposit_item = 1;
				delete_item = 1;
			}

			if (deposit_item)
			{
				if (stackable)
				{
					memcpy(&compare_item1, &client->character.inventory[ch].item.data[0], 3);
					for (ch2 = 0;ch2<client->character.bankUse;ch2++)
					{
						memcpy(&compare_item2, &client->character.bankInventory[ch2].data[0], 3);
						if (compare_item1 == compare_item2)
						{
							stack_count = (client->character.bankInventory[ch2].bank_count & 0xFF);
							if ((stack_count + count) > stackable)
							{
								count = stackable - stack_count;
								client->character.bankInventory[ch2].bank_count = 0x10000 + stackable;
							}
							else
								client->character.bankInventory[ch2].bank_count += count;
							deposit_done = 1;
							break;
						}
					}
				}

				if ((!client->todc) && (!deposit_done))
				{
					if (client->character.inventory[ch].item.data[0] == 0x01)
					{
						if ((client->character.inventory[ch].item.data[1] == 0x01) &&
							(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
						{
							for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
								if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
									(client->character.inventory[ch2].item.data[1] != 0x02) &&
									(client->character.inventory[ch2].flags & 0x08))
								{
									client->character.inventory[ch2].flags &= ~(0x08);
									client->character.inventory[ch2].item.data[4] = 0x00;
								}
						}
					}

					memcpy(&client->character.bankInventory[client->character.bankUse].data[0],
						&client->character.inventory[ch].item.data[0],
						sizeof(ITEM));

					if (stackable)
					{
						memset(&client->character.bankInventory[client->character.bankUse].data[4], 0, 4);
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10000 + count;
					}
					else
						client->character.bankInventory[client->character.bankUse].bank_count = 0x10001;

					client->character.bankInventory[client->character.bankUse].itemid = client->character.inventory[ch].item.itemid; // for now
					client->character.bankUse++;
				}

				if (delete_item)
					client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}

	if (found_item == -1)
	{//缺失 Sancaros
		Send1A(L"Could not find item to deposit.", client, 66);
		client->todc = 1;
	}
	else
		CleanUpInventory(client);
}

void DeleteFromInventory(INVENTORY_ITEM* i, unsigned count, BANANA* client)
{
	unsigned ch, ch2;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned compare_id;

	// Deletes an item from the client's character data.

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	memcpy(&compare_item1, &i->item.data[0], 3);
	if (i->item.itemid)
		compare_id = i->item.itemid;
	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
		if (!i->item.itemid)
			compare_id = client->character.inventory[ch].item.itemid;
		if ((compare_item1 == compare_item2) && (compare_id == client->character.inventory[ch].item.itemid)) // Found the item?
		{
			if (client->character.inventory[ch].item.data[0] == 0x03)
				stackable = stackable_table[client->character.inventory[ch].item.data[1]];

			if (stackable)
			{
				if (!count)
					count = 1;

				stack_count = client->character.inventory[ch].item.data[5];
				if (!stack_count)
					stack_count = 1;

				if (stack_count < count)
					count = stack_count;

				stack_count -= count;

				client->character.inventory[ch].item.data[5] = (unsigned char)stack_count;

				if (!stack_count)
					delete_item = 1;
			}
			else
				delete_item = 1;

			memset(&client->encryptbuf[0x00], 0, 0x14);
			client->encryptbuf[0x00] = 0x14;
			client->encryptbuf[0x02] = 0x60;
			client->encryptbuf[0x08] = 0x29;
			client->encryptbuf[0x09] = 0x05;
			client->encryptbuf[0x0A] = client->clientID;
			*(unsigned *)&client->encryptbuf[0x0C] = client->character.inventory[ch].item.itemid;
			client->encryptbuf[0x10] = (unsigned char)count;

			SendToLobby(l, 4, &client->encryptbuf[0x00], 0x14, 0);

			if (delete_item)
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
					}
				}
				client->character.inventory[ch].in_use = 0;
			}
			found_item = ch;
			break;
		}
	}
	/*if (found_item == -1)
	{//缺失 Sancaros
	Send1A(L"Could not find item to delete from inventory.", client, 67);
	client->todc = 1;
	}
	else*/
	CleanUpInventory(client);

}

unsigned AddToInventory(INVENTORY_ITEM* i, unsigned count, int shop, BANANA* client)
{
	unsigned ch;
	unsigned char stackable = 0;
	unsigned stack_count;
	unsigned compare_item1 = 0;
	unsigned compare_item2 = 0;
	unsigned item_added = 0;
	unsigned notsend;
	LOBBY* l;

	// Adds an item to the client's inventory... (out of thin air)
	// The new itemid must already be set to i->item.itemid

	if (!client->lobby)
		return 0;

	l = (LOBBY*)client->lobby;

	if (i->item.data[0] == 0x04)
	{
		// Meseta
		count = *(unsigned *)&i->item.data2[0];
		client->character.meseta += count;
		if (client->character.meseta > 999999)
			client->character.meseta = 999999;
		item_added = 1;
	}
	else
	{
		if (i->item.data[0] == 0x03)
			stackable = stackable_table[i->item.data[1]];
	}

	if ((!client->todc) && (!item_added))
	{
		if (stackable)
		{
			if (!count)
				count = 1;
			memcpy(&compare_item1, &i->item.data[0], 3);
			for (ch = 0;ch<client->character.inventoryUse;ch++)
			{
				memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ((stack_count + count) > stackable)
					{
						count = stackable - stack_count;
						client->character.inventory[ch].item.data[5] = stackable;
					}
					else
						client->character.inventory[ch].item.data[5] = (unsigned char)(stack_count + count);
					item_added = 1;
					break;
				}
			}
		}

		if (item_added == 0) // Make sure we don't go over the max inventory
		{
			/*if (client->character.inventoryUse >= 30)
			{//缺失 Sancaros
			Send1A(L"Inventory limit reached.", client, 60);
			client->todc = 1;
			}
			else*/
			//{
			// Give item to client...
			client->character.inventory[client->character.inventoryUse].in_use = 0x01;
			client->character.inventory[client->character.inventoryUse].flags = 0x00;
			memcpy(&client->character.inventory[client->character.inventoryUse].item, &i->item, sizeof(ITEM));
			if (stackable)
			{
				memset(&client->character.inventory[client->character.inventoryUse].item.data[4], 0, 4);
				client->character.inventory[client->character.inventoryUse].item.data[5] = (unsigned char)count;
			}
			client->character.inventoryUse++;
			item_added = 1;
			//}
		}
	}

	if ((!client->todc) && (item_added))
	{
		// Let people know the client has a new toy...
		memset(&client->encryptbuf[0x00], 0, 0x24);
		client->encryptbuf[0x00] = 0x24;
		client->encryptbuf[0x02] = 0x60;
		client->encryptbuf[0x08] = 0xBE;
		client->encryptbuf[0x09] = 0x09;
		client->encryptbuf[0x0A] = client->clientID;
		memcpy(&client->encryptbuf[0x0C], &i->item.data[0], 12);
		*(unsigned *)&client->encryptbuf[0x18] = i->item.itemid;
		if ((!stackable) || (i->item.data[0] == 0x04))
			*(unsigned *)&client->encryptbuf[0x1C] = *(unsigned *)&i->item.data2[0];
		else
			client->encryptbuf[0x11] = count;
		memset(&client->encryptbuf[0x20], 0, 4);
		if (shop)
			notsend = client->guildcard;
		else
			notsend = 0;
		SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x24, notsend);
	}
	return item_added;
}

void FixItem(ITEM* i)
{
	unsigned ch3;

	if (i->data[0] == 2) // Mag
	{
		MAG* m;
		short mDefense, mPower, mDex, mMind;
		int total_levels;

		m = (MAG*)&i->data[0];

		if (m->synchro > 120)
			m->synchro = 120;

		if (m->synchro < 0)
			m->synchro = 0;

		if (m->IQ > 200)
			m->IQ = 200;

		if ((m->defense < 0) || (m->power < 0) || (m->dex < 0) || (m->mind < 0))
			total_levels = 201; // Auto fail if any stat is under 0...
		else
		{
			mDefense = m->defense / 100;
			mPower = m->power / 100;
			mDex = m->dex / 100;
			mMind = m->mind / 100;
			total_levels = mDefense + mPower + mDex + mMind;
		}

		if ((total_levels > 200) || (m->level > 200))
		{
			// Mag fails IRL, initialize it
			m->defense = 500;
			m->power = 0;
			m->dex = 0;
			m->mind = 0;
			m->level = 5;
			m->blasts = 0;
			m->IQ = 0;
			m->synchro = 20;
			m->mtype = 0;
			m->PBflags = 0;
		}
	}

	if (i->data[0] == 1) // Normalize Armor & Barriers
	{
		switch (i->data[1])
		{
		case 0x01:
			if (i->data[6] > armor_dfpvar_table[i->data[2]])
				i->data[6] = armor_dfpvar_table[i->data[2]];
			if (i->data[8] > armor_evpvar_table[i->data[2]])
				i->data[8] = armor_evpvar_table[i->data[2]];
			break;
		case 0x02:
			if (i->data[6] > barrier_dfpvar_table[i->data[2]])
				i->data[6] = barrier_dfpvar_table[i->data[2]];
			if (i->data[8] > barrier_evpvar_table[i->data[2]])
				i->data[8] = barrier_evpvar_table[i->data[2]];
			break;
		}
	}

	if (i->data[0] == 0) // Weapon
	{
		signed char percent_table[6];
		signed char percent;
		unsigned max_percents, num_percents;
		int srank;

		if ((i->data[1] == 0x33) ||  // SJS & Lame max 2 percents
			(i->data[1] == 0xAB))
			max_percents = 2;
		else
			max_percents = 3;

		srank = 0;
		memset(&percent_table[0], 0, 6);
		num_percents = 0;

		for (ch3 = 6;ch3 <= 4 + (max_percents * 2);ch3 += 2)
		{
			if (i->data[ch3] & 128)
			{
				srank = 1; // S-Rank
				break;
			}

			if ((i->data[ch3]) &&
				(i->data[ch3] < 0x06))
			{
				// Percents over 100 or under -100 get set to 0
				percent = (char)i->data[ch3 + 1];
				if ((percent > 100) || (percent < -100))
					percent = 0;
				// Save percent
				percent_table[i->data[ch3]] =
					percent;
			}
		}

		if (!srank)
		{
			for (ch3 = 6;ch3 <= 4 + (max_percents * 2);ch3 += 2)
			{
				// Reset all %s
				i->data[ch3] = 0;
				i->data[ch3 + 1] = 0;
			}

			for (ch3 = 1;ch3 <= 5;ch3++)
			{
				// Rebuild %s
				if (percent_table[ch3])
				{
					i->data[6 + (num_percents * 2)] = ch3;
					i->data[7 + (num_percents * 2)] = (unsigned char)percent_table[ch3];
					num_percents++;
					if (num_percents == max_percents)
						break;
				}
			}
		}
	}
}