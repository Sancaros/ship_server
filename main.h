/********************************************************
**
**		main  :-
**
********************************************************/

int main()
{
	unsigned ch, ch2, ch3, ch4, ch5, connectNum;
	int wep_rank;
	PTDATA ptd;
	unsigned wep_counters[24] = { 0 };
	unsigned tool_counters[28] = { 0 };
	unsigned tech_counters[19] = { 0 };
	struct in_addr ship_in;
	struct sockaddr_in listen_in;
	unsigned listen_length;
	int block_sockfd[10] = { -1 };
	struct in_addr block_in[10];
	int ship_sockfd = -1;
	int pkt_len, pkt_c, bytes_sent;
	int wserror;
	WSADATA winsock_data;
	FILE* fp;
	unsigned char* connectionChunk;
	unsigned char* connectionPtr;
	unsigned char* blockPtr;
	unsigned char* blockChunk;
	//unsigned short this_packet;
	unsigned long logon_this_packet;
	HINSTANCE hinst;
	NOTIFYICONDATA nid = { 0 };
	WNDCLASS wc = { 0 };
	HWND hwndWindow;
	MSG msg;

	ch = 0;

	consoleHwnd = GetConsoleWindow();
	hinst = GetModuleHandle(NULL);

	dp[0] = 0;

	strcat(&dp[0], "Tethealla 舰船服务器 版本 ");
	strcat(&dp[0], SERVER_VERSION);
	strcat(&dp[0], " 作者 Sodaboy 编译 Sancaros");
	strcat(&dp[0], " 当前舰船");
	strcat(&dp[0], Ship_Name[255]);//缺失 Sancaros
	strcat(&dp[0], "名称代码未完成");//缺失 Sancaros
	SetConsoleTitle(&dp[0]);

	printf("\n特提塞拉 舰船服务器 版本 %s  版权作者 (C) 2008  Terry Chatman Jr.\n", SERVER_VERSION);
	printf("\n编译 Sancaros. 2020.12\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf("这个程序绝对没有保证: 详情参见说明\n");
	printf("请参阅GPL-3.0.TXT中的第15节\n");
	printf("这是免费软件,欢迎您重新发布\n");
	printf("在某些条件下,详见GPL-3.0.TXT.\n");

	/*for (ch=0;ch<5;ch++)
	{
	printf (".");
	Sleep (1000);
	}*/
	printf("\n\n");

	WSAStartup(MAKEWORD(1, 1), &winsock_data);

	printf("正在从 ship.ini 中加载配置...");
#ifdef LOG_60
	debugfile = fopen("60packets.txt", "a");
	if (!debugfile)
	{
		printf("无法生成 60packets.txt");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
#endif
	mt_bestseed();
	load_config_file();
	printf("确认!\n\n");

	printf("载入语言文件...\n");

	load_language_file();

	printf("确认!\n\n");

	printf("载入 ship_key.bin 文件... ");

	fp = fopen("ship_key.bin", "rb");
	if (!fp)
	{
		printf("未发现 ship_key.bin 文件!\n");
		printf("按下 [回车键] 退出...");
		gets_s(&dp[0], 0);
		exit(1);
	}

	fread(&ship_index, 1, 4, fp);
	fread(&ship_key[0], 1, 128, fp);
	fclose(fp);

	printf("确认!\n\n加载武器参数文件...\n");
	LoadWeaponParam();
	printf("\n.. 完成!\n\n");

	printf("加载装甲和盾牌参数文件...\n");
	LoadArmorParam();
	printf("\n.. 完成!\n\n");

	printf("加载技术参数文件...\n");
	LoadTechParam();
	printf("\n.. 完成!\n\n");

	for (ch = 1;ch<200;ch++)
		tnlxp[ch] = tnlxp[ch - 1] + tnlxp[ch];

	printf("加载战斗参数文件...\n\n");
	LoadBattleParam(&ep1battle_off[0], "param\\BattleParamEntry.dat", 374, 0x8fef1ffe);
	LoadBattleParam(&ep1battle[0], "param\\BattleParamEntry_on.dat", 374, 0xb8a2d950);
	LoadBattleParam(&ep2battle_off[0], "param\\BattleParamEntry_lab.dat", 374, 0x3dc217f5);
	LoadBattleParam(&ep2battle[0], "param\\BattleParamEntry_lab_on.dat", 374, 0x4d4059cf);
	LoadBattleParam(&ep4battle_off[0], "param\\BattleParamEntry_ep4.dat", 332, 0x50841167);
	LoadBattleParam(&ep4battle[0], "param\\BattleParamEntry_ep4_on.dat", 332, 0x42bf9716);

	for (ch = 0;ch<374;ch++)
		if (ep2battle_off[ch].HP)
		{
			ep2battle_off[ch].XP = (ep2battle_off[ch].XP * 130) / 100; // 30% boost to EXP
			ep2battle[ch].XP = (ep2battle[ch].XP * 130) / 100;
		}

	printf("\n.. 完成!\n\n建立通用表... \n\n");
	printf("武器掉率: %03f%%\n", (float)WEAPON_DROP_RATE / 1000);
	printf("盔甲掉率: %03f%%\n", (float)ARMOR_DROP_RATE / 1000);
	printf("玛古掉率: %03f%%\n", (float)MAG_DROP_RATE / 1000);
	printf("工具掉率: %03f%%\n", (float)TOOL_DROP_RATE / 1000);
	printf("美赛塔掉率: %03f%%\n", (float)MESETA_DROP_RATE / 1000);
	printf("经验倍率: %u%%\n\n", EXPERIENCE_RATE * 100);

	ch = 0;
	while (ch < 100000)
	{
		for (ch2 = 0;ch2<5;ch2++)
		{
			common_counters[ch2]++;
			if ((common_counters[ch2] >= common_rates[ch2]) && (ch<100000))
			{
				common_table[ch++] = (unsigned char)ch2;
				common_counters[ch2] = 0;
			}
		}
	}

	printf(".. 完成!\n\n");

	printf("正在加载物品参数\\ItemPT.gsl...\n");
	fp = fopen("param\\ItemPT.gsl", "rb");
	if (!fp)
	{
		printf("缺少 ItemPT.gsl 文件\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	fseek(fp, 0x3000, SEEK_SET);

	// Load up that EP1 data
	printf("正在解析 章节 I 的数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2<4;ch2++) // For each difficulty
	{
		for (ch = 0;ch<10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);

			ptd.enemy_dar[44] = 100; // Dragon
			ptd.enemy_dar[45] = 100; // De Rol Le
			ptd.enemy_dar[46] = 100; // Vol Opt
			ptd.enemy_dar[47] = 100; // Falz

			for (ch3 = 0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}

			for (ch3 = 0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}

			ptd.enemy_meseta[47][0] = ptd.enemy_meseta[46][0] + 400 + (100 * ch2); // Give Falz some meseta
			ptd.enemy_meseta[47][1] = ptd.enemy_meseta[46][1] + 400 + (100 * ch2);

			for (ch3 = 0;ch3<23;ch3++)
			{
				for (ch4 = 0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}

			for (ch3 = 0;ch3<28;ch3++)
			{
				for (ch4 = 0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}

			memcpy(&pt_tables_ep1[ch][ch2], &ptd, sizeof(PTDATA));

			// Set up the weapon drop table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if (wep_rank >= 0)
							{
								weapon_drops_ep1[ch][ch2][ch5][ch3++] = (ch4 + 1) + ((unsigned char)wep_rank << 8);
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}

			// Set up the slot table

			memset(&wep_counters[0], 0, 4 * 24);
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4 = 0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep1[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5 = 0;ch5<4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep1[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5 = 0;ch5<6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep1[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep1[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}

	// Load up that EP2 data
	printf("正在解析 章节 II 的数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2<4;ch2++) // For each difficulty
	{
		for (ch = 0;ch<10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);

			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow

			for (ch3 = 0;ch3<10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}

			for (ch3 = 0;ch3<0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}

			ptd.enemy_meseta[78][0] = ptd.enemy_meseta[77][0] + 400 + (100 * ch2); // Give Flow some meseta
			ptd.enemy_meseta[78][1] = ptd.enemy_meseta[77][1] + 400 + (100 * ch2);

			for (ch3 = 0;ch3<23;ch3++)
			{
				for (ch4 = 0;ch4<6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}

			for (ch3 = 0;ch3<28;ch3++)
			{
				for (ch4 = 0;ch4<10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}

			memcpy(&pt_tables_ep2[ch][ch2], &ptd, sizeof(PTDATA));

			// Set up the weapon drop table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3<4096))
						{
							wep_rank = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if (wep_rank >= 0)
							{
								weapon_drops_ep2[ch][ch2][ch5][ch3++] = (ch4 + 1) + ((unsigned char)wep_rank << 8);
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}


			// Set up the slot table

			memset(&wep_counters[0], 0, 4 * 24);
			ch3 = 0;

			while (ch3 < 4096)
			{
				for (ch4 = 0;ch4<5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
					{
						slots_ep2[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5 = 0;ch5<4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3<4096))
						{
							power_patterns_ep2[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5 = 0;ch5<6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							percent_patterns_ep2[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3<4096))
						{
							tool_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3<4096))
						{
							attachment_ep2[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5 = 0;ch5<10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4<19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3<4096))
							{
								tech_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}


	fclose(fp);
	printf("\n.. 完成!\n\n");
	printf("正在加载等级参数\\PlyLevelTbl.bin ... ");
	fp = fopen("param\\PlyLevelTbl.bin", "rb");
	if (!fp)
	{
		printf("缺少 PlyLevelTbl.bin 文件!\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	fread(&startingData, 1, 12 * 14, fp);
	fseek(fp, 0xE4, SEEK_SET);
	fread(&playerLevelData, 1, 28800, fp);
	fclose(fp);

	printf("确认!\n\n.. 完成!\n\n现在开始载入任务...\n\n");

	memset(&quest_menus[0], 0, sizeof(quest_menus));

	// 0 = Episode 1 Team
	// 1 = Episode 2 Team
	// 2 = Episode 4 Team
	// 3 = Episode 1 Solo
	// 4 = Episode 2 Solo
	// 5 = Episode 4 Solo
	// 6 = Episode 1 Government
	// 7 = Episode 2 Government
	// 8 = Episode 4 Government
	// 9 = Battle
	// 10 = Challenge

	LoadQuests("quest\\ep1team.ini", 0);
	LoadQuests("quest\\ep2team.ini", 1);
	LoadQuests("quest\\ep4team.ini", 2);
	LoadQuests("quest\\ep1solo.ini", 3);
	LoadQuests("quest\\ep2solo.ini", 4);
	LoadQuests("quest\\ep4solo.ini", 5);
	LoadQuests("quest\\ep1gov.ini", 6);
	LoadQuests("quest\\ep2gov.ini", 7);
	LoadQuests("quest\\ep4gov.ini", 8);
	LoadQuests("quest\\battle.ini", 9);
	LoadQuests("quest\\challenge.ini", 10);

	printf("\n分配 %u 字节的内存给 %u 个任务...\n\n", questsMemory, numQuests);

	printf("载入商店数据 shop\\shop.dat ...");

	fp = fopen("shop\\shop.dat", "rb");

	if (!fp)
	{
		printf("缺少 shop.dat 文件!\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	if (fread(&shops[0], 1, 7000 * sizeof(SHOP), fp) != (7000 * sizeof(SHOP)))
	{
		printf("无法读取商店数据文件...\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	fclose(fp);

	shop_checksum = CalculateChecksum(&shops[0], 7000 * sizeof(SHOP));

	printf("完成!\n\n");

	LoadShopData2();

	readLocalGMFile();

	// Set up shop indexes based on character levels...

	for (ch = 0;ch<200;ch++)
	{
		switch (ch / 20L)
		{
		case 0:	// Levels 1-20
			shopidx[ch] = 0;
			break;
		case 1: // Levels 21-40
			shopidx[ch] = 1000;
			break;
		case 2: // Levels 41-80
		case 3:
			shopidx[ch] = 2000;
			break;
		case 4: // Levels 81-120
		case 5:
			shopidx[ch] = 3000;
			break;
		case 6: // Levels 121-160
		case 7:
			shopidx[ch] = 4000;
			break;
		case 8: // Levels 161-180
			shopidx[ch] = 5000;
			break;
		default: // Levels 180+
			shopidx[ch] = 6000;
			break;
		}
	}

	memcpy(&Packet03[0x54], &Message03[0], sizeof(Message03));
	printf("\n发送服务器参数\n");
	printf("///////////////////////\n");
	printf("舰船IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
	printf("舰船端口: %u\n", serverPort);
	printf("舰舱数量: %u\n", serverBlocks);
	printf("最大连接数: %u\n", serverMaxConnections);
	printf("登陆服务器IP: %u.%u.%u.%u\n", loginIP[0], loginIP[1], loginIP[2], loginIP[3]);

	printf("\n连接至登陆服务器...\n");
	initialize_logon();
	reconnect_logon();

	printf("\n分配 %u 字节的内存给舰舱... ", sizeof(BLOCK) * serverBlocks);
	blockChunk = malloc(sizeof(BLOCK) * serverBlocks);
	if (!blockChunk)
	{
		printf("内存不足!\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	blockPtr = blockChunk;
	memset(blockChunk, 0, sizeof(BLOCK) * serverBlocks);
	for (ch = 0;ch<serverBlocks;ch++)
	{
		blocks[ch] = (BLOCK*)blockPtr;
		blockPtr += sizeof(BLOCK);
	}

	printf("确认!\n");

	printf("\n分配 %u 字节的内存给数据连接... ", sizeof(BANANA) * serverMaxConnections);
	connectionChunk = malloc(sizeof(BANANA) * serverMaxConnections);
	if (!connectionChunk)
	{
		printf("内存不足!\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	connectionPtr = connectionChunk;
	for (ch = 0;ch<serverMaxConnections;ch++)
	{
		connections[ch] = (BANANA*)connectionPtr;
		connections[ch]->guildcard = 0;
		connections[ch]->character_backup = NULL;
		connections[ch]->mode = 0;
		initialize_connection(connections[ch]);
		connectionPtr += sizeof(BANANA);
	}

	printf("确认!\n\n");

	printf("载入封禁系统数据... ");
	fp = fopen("bandata.dat", "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		ch = ftell(fp);
		num_bans = ch / sizeof(BANDATA);
		if (num_bans > 5000)
			num_bans = 5000;
		fseek(fp, 0, SEEK_SET);
		fread(&ship_bandata[0], 1, num_bans * sizeof(BANDATA), fp);
		fclose(fp);
	}
	printf("完成!\n\n%u 个封禁数据已载入.\n%u 个IP掩码封禁数据已载入.\n\n", num_bans, num_masks);

	/* Open the ship port... */

	printf("开放舰船服务器端口 %u 用于数据连接.\n", serverPort);

#ifdef USEADDR_ANY
	ship_in.s_addr = INADDR_ANY;
#else
	memcpy(&ship_in.s_addr, &serverIP[0], 4);
#endif
	ship_sockfd = tcp_sock_open(ship_in, serverPort);

	tcp_listen(ship_sockfd);

	for (ch = 1;ch <= serverBlocks;ch++)
	{
		printf("开放舰仓端口 %u (BLOCK%u) 用于数据连接.\n", serverPort + ch, ch);
#ifdef USEADDR_ANY
		block_in[ch - 1].s_addr = INADDR_ANY;
#else
		memcpy(&block_in[ch - 1].s_addr, &serverIP[0], 4);
#endif
		block_sockfd[ch - 1] = tcp_sock_open(block_in[ch - 1], serverPort + ch);
		if (block_sockfd[ch - 1] < 0)
		{
			printf("无法开放端口 %u 用于数据连接.\n", serverPort + ch);
			printf("按下 [回车键] 退出");
			gets_s(&dp[0], 0);
			exit(1);
		}

		tcp_listen(block_sockfd[ch - 1]);

	}

	if (ship_sockfd < 0)
	{
		printf("无法开放舰船端口连接.\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	printf("\n监听中...\n");
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hinst, IDC_ARROW);
	wc.hInstance = hinst;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "sodaboy";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wc))
	{
		printf("注册 Class 文件失败 .\n");
		exit(1);
	}

	hwndWindow = CreateWindow("sodaboy", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
		NULL,
		NULL,
		hinst,
		NULL);

	if (!hwndWindow)
	{
		printf("无法生成窗口程序.");
		exit(1);
	}

	ShowWindow(hwndWindow, SW_HIDE);
	UpdateWindow(hwndWindow);
	ShowWindow(consoleHwnd, SW_HIDE);
	UpdateWindow(consoleHwnd);

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwndWindow;
	nid.uID = 100;
	nid.uCallbackMessage = MYWM_NOTIFYICON;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));
	nid.szTip[0] = 0;
	strcat(&nid.szTip[0], "Tethealla 舰船服务器 ");
	strcat(&nid.szTip[0], SERVER_VERSION);
	strcat(&nid.szTip[0], " - 双击以显示/隐藏");
	Shell_NotifyIcon(NIM_ADD, &nid);

	for (;;)
	{
		int nfds = 0;

		/* Process the system tray icon */

		if (PeekMessage(&msg, hwndWindow, 0, 0, 1))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		/* Ping pong?! */

		servertime = time(NULL);

		/* Clear socket activity flags. */

		FD_ZERO(&ReadFDs);
		FD_ZERO(&WriteFDs);
		FD_ZERO(&ExceptFDs);

		// Stop blocking connections after everyone has been disconnected...

		if ((serverNumConnections == 0) && (blockConnections))
		{
			blockConnections = 0;
			printf("不再阻止新的连接...\n");
		}

		// Process player packets

		for (ch = 0;ch<serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			workConnect = connections[connectNum];

			if (workConnect->plySockfd >= 0)
			{
				if (blockConnections)
				{
					if (blockTick != (unsigned)servertime)
					{
						blockTick = (unsigned)servertime;
						printf("断开连接的用户 %u, 离开并断开连接: %u\n", workConnect->guildcard, serverNumConnections - 1);
						Send1A(L"You were disconnected by a GM...", workConnect, 104);
						workConnect->todc = 1;
					}
				}

				if (workConnect->lastTick != (unsigned)servertime)
				{
					Send1D(workConnect);
					if (workConnect->lastTick > (unsigned)servertime)
						ch2 = 1;
					else
						ch2 = 1 + ((unsigned)servertime - workConnect->lastTick);
					workConnect->lastTick = (unsigned)servertime;
					workConnect->packetsSec /= ch2;
					workConnect->toBytesSec /= ch2;
					workConnect->fromBytesSec /= ch2;
				}

				FD_SET(workConnect->plySockfd, &ReadFDs);
				nfds = max(nfds, workConnect->plySockfd);
				FD_SET(workConnect->plySockfd, &ExceptFDs);
				nfds = max(nfds, workConnect->plySockfd);

				if (workConnect->snddata - workConnect->sndwritten)
				{
					FD_SET(workConnect->plySockfd, &WriteFDs);
					nfds = max(nfds, workConnect->plySockfd);
				}
			}
		}


		// Read from logon server (if connected)

		if (logon->sockfd >= 0)
		{
			if ((unsigned)servertime - logon->last_ping > 60)
			{
				printf("登录服务器ping超时.  正在尝试在 %u 秒内重新连接...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon();
			}
			else
			{
				if (logon->packetdata)
				{
					logon_this_packet = *(unsigned *)&logon->packet[logon->packetread];
					memcpy(&logon->decryptbuf[0], &logon->packet[logon->packetread], logon_this_packet);

					LogonProcessPacket(logon);

					logon->packetread += logon_this_packet;

					if (logon->packetread == logon->packetdata)
						logon->packetread = logon->packetdata = 0;
				}

				FD_SET(logon->sockfd, &ReadFDs);
				nfds = max(nfds, logon->sockfd);

				if (logon->snddata - logon->sndwritten)
				{
					FD_SET(logon->sockfd, &WriteFDs);
					nfds = max(nfds, logon->sockfd);
				}
			}
		}
		else
		{
			logon_tick++;
			if (logon_tick >= LOGIN_RECONNECT_SECONDS * 100)
			{
				printf("正在重新连接到登录服务器...\n");
				reconnect_logon();
			}
		}


		// Listen for block connections

		for (ch = 0;ch<serverBlocks;ch++)
		{
			FD_SET(block_sockfd[ch], &ReadFDs);
			nfds = max(nfds, block_sockfd[ch]);
		}

		// Listen for ship connections

		FD_SET(ship_sockfd, &ReadFDs);
		nfds = max(nfds, ship_sockfd);

		/* Check sockets for activity. */

		if (select(nfds + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &select_timeout) > 0)
		{
			if (FD_ISSET(ship_sockfd, &ReadFDs))
			{
				// Someone's attempting to connect to the ship server.
				ch = free_connection();
				if (ch != 0xFFFF)
				{
					listen_length = sizeof(listen_in);
					workConnect = connections[ch];
					if ((workConnect->plySockfd = tcp_accept(ship_sockfd, (struct sockaddr*) &listen_in, &listen_length)) > 0)
					{
						if (!blockConnections)
						{
							workConnect->connection_index = ch;
							serverConnectionList[serverNumConnections++] = ch;
							memcpy(&workConnect->IP_Address[0], inet_ntoa(listen_in.sin_addr), 16);
							*(unsigned *)&workConnect->ipaddr = *(unsigned *)&listen_in.sin_addr;
							printf("舰舱收到来自 %s:%u 的登船请求\n", workConnect->IP_Address, listen_in.sin_port);
							printf("玩家统计: %u\n", serverNumConnections);

							if ((fp = fopen("playerCount.txt", "w")) == NULL)
							{
								printf("playerCount.txt 文件不存在.\n");
							}
							else
							{
								fprintf(fp, "%u", serverNumConnections);
								fclose(fp);
							}

							ShipSend0E(logon);
							start_encryption(workConnect);
							/* Doin' ship process... */
							workConnect->block = 0;
						}
						else
							initialize_connection(workConnect);
					}
				}
			}

			for (ch = 0;ch<serverBlocks;ch++)
			{
				if (FD_ISSET(block_sockfd[ch], &ReadFDs))
				{
					// Someone's attempting to connect to the block server.
					ch2 = free_connection();
					if (ch2 != 0xFFFF)
					{
						listen_length = sizeof(listen_in);
						workConnect = connections[ch2];
						if ((workConnect->plySockfd = tcp_accept(block_sockfd[ch], (struct sockaddr*) &listen_in, &listen_length)) > 0)
						{
							if (!blockConnections)
							{
								workConnect->connection_index = ch2;
								serverConnectionList[serverNumConnections++] = ch2;
								memcpy(&workConnect->IP_Address[0], inet_ntoa(listen_in.sin_addr), 16);
								printf("舰舱已接收来自 %s:%u 的登船请求\n", inet_ntoa(listen_in.sin_addr), listen_in.sin_port);
								*(unsigned *)&workConnect->ipaddr = *(unsigned *)&listen_in.sin_addr;
								printf("玩家统计: %u\n", serverNumConnections);
								ShipSend0E(logon);
								start_encryption(workConnect);
								/* Doin' block process... */
								workConnect->block = ch + 1;
							}
							else
								initialize_connection(workConnect);
						}
					}
				}
			}


			// Process client connections

			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				workConnect = connections[connectNum];

				if (workConnect->plySockfd >= 0)
				{
					if (FD_ISSET(workConnect->plySockfd, &WriteFDs))
					{
						// Write shit.

						bytes_sent = send(workConnect->plySockfd, &workConnect->sndbuf[workConnect->sndwritten],
							workConnect->snddata - workConnect->sndwritten, 0);

						if (bytes_sent == SOCKET_ERROR)
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not send data to client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection(workConnect);
						}
						else
						{
							workConnect->toBytesSec += bytes_sent;
							workConnect->sndwritten += bytes_sent;
						}

						if (workConnect->sndwritten == workConnect->snddata)
							workConnect->sndwritten = workConnect->snddata = 0;
					}

					// Disconnect those violators of the law...

					if (workConnect->todc)
						initialize_connection(workConnect);

					if (FD_ISSET(workConnect->plySockfd, &ReadFDs))
					{
						// Read shit.
						if ((pkt_len = recv(workConnect->plySockfd, &tmprcv[0], TCP_BUFFER_SIZE - 1, 0)) <= 0)
						{
							/*
							wserror = WSAGetLastError();
							printf ("Could not read data from client...\n");
							printf ("Socket Error %u.\n", wserror );
							*/
							initialize_connection(workConnect);
						}
						else
						{
							workConnect->fromBytesSec += (unsigned)pkt_len;
							// Work with it.

							for (pkt_c = 0;pkt_c<pkt_len;pkt_c++)
							{
								workConnect->rcvbuf[workConnect->rcvread++] = tmprcv[pkt_c];

								if (workConnect->rcvread == 8)
								{
									// Decrypt the packet header after receiving 8 bytes.

									cipher_ptr = &workConnect->client_cipher;

									decryptcopy(&workConnect->decryptbuf[0], &workConnect->rcvbuf[0], 8);

									// Make sure we're expecting a multiple of 8 bytes.

									workConnect->expect = *(unsigned short*)&workConnect->decryptbuf[0];

									if (workConnect->expect % 8)
										workConnect->expect += (8 - (workConnect->expect % 8));

									if (workConnect->expect > TCP_BUFFER_SIZE)
									{
										initialize_connection(workConnect);
										break;
									}
								}

								if ((workConnect->rcvread == workConnect->expect) && (workConnect->expect != 0))
								{
									// Decrypt the rest of the data if needed.

									cipher_ptr = &workConnect->client_cipher;

									if (workConnect->rcvread > 8)
										decryptcopy(&workConnect->decryptbuf[8], &workConnect->rcvbuf[8], workConnect->expect - 8);

									workConnect->packetsSec++;

									if (
										//(workConnect->packetsSec   > 89)    ||
										(workConnect->fromBytesSec > 30000) ||
										(workConnect->toBytesSec   > 150000)
										)
									{
										printf("%u 由于可能的DDOS攻击而断开连接. (p/s: %u, tb/s: %u, fb/s: %u)\n", workConnect->guildcard, workConnect->packetsSec, workConnect->toBytesSec, workConnect->fromBytesSec);
										initialize_connection(workConnect);
										break;
									}
									else
									{
										switch (workConnect->block)
										{
										case 0x00:
											// Ship Server
											ShipProcessPacket(workConnect);
											break;
										default:
											// Block server
											BlockProcessPacket(workConnect);
											break;
										}
									}
									workConnect->rcvread = 0;
								}
							}
						}
					}

					if (FD_ISSET(workConnect->plySockfd, &ExceptFDs)) // Exception?
						initialize_connection(workConnect);

				}
			}


			// Process logon server connection

			if (logon->sockfd >= 0)
			{
				if (FD_ISSET(logon->sockfd, &WriteFDs))
				{
					// Write shit.

					bytes_sent = send(logon->sockfd, &logon->sndbuf[logon->sndwritten],
						logon->snddata - logon->sndwritten, 0);

					if (bytes_sent == SOCKET_ERROR)
					{
						wserror = WSAGetLastError();
						printf("无法将数据发送到登录服务器...\n");
						printf("套接字错误 %u.\n", wserror);
						initialize_logon();
						printf("与登录服务器的连接中断...\n");
						printf("将在 %u 秒后重连...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
						logon->sndwritten += bytes_sent;

					if (logon->sndwritten == logon->snddata)
						logon->sndwritten = logon->snddata = 0;
				}

				if (FD_ISSET(logon->sockfd, &ReadFDs))
				{
					// Read shit.
					if ((pkt_len = recv(logon->sockfd, &tmprcv[0], PACKET_BUFFER_SIZE - 1, 0)) <= 0)
					{
						wserror = WSAGetLastError();
						printf("无法将数据发送到登录服务器...\n");
						printf("套接字错误 %u.\n", wserror);
						initialize_logon();
						printf("与登录服务器的连接中断...\n");
						printf("将在 %u 秒后重连...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
					{
						// Work with it.
						for (pkt_c = 0;pkt_c<pkt_len;pkt_c++)
						{
							logon->rcvbuf[logon->rcvread++] = tmprcv[pkt_c];

							if (logon->rcvread == 4)
							{
								/* Read out how much data we're expecting this packet. */
								logon->expect = *(unsigned *)&logon->rcvbuf[0];

								if (logon->expect > TCP_BUFFER_SIZE)
								{
									printf("从登录服务器接收太多数据.\n正在断开连接,并将在 %u 秒后重新连接...\n", LOGIN_RECONNECT_SECONDS);
									initialize_logon();
								}
							}

							if ((logon->rcvread == logon->expect) && (logon->expect != 0))
							{
								decompressShipPacket(logon, &logon->decryptbuf[0], &logon->rcvbuf[0]);

								logon->expect = *(unsigned *)&logon->decryptbuf[0];

								if (logon->packetdata + logon->expect < PACKET_BUFFER_SIZE)
								{
									memcpy(&logon->packet[logon->packetdata], &logon->decryptbuf[0], logon->expect);
									logon->packetdata += logon->expect;
								}
								else
									initialize_logon();

								if (logon->sockfd < 0)
									break;

								logon->rcvread = 0;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}



void tcp_listen(int sockfd)
{
	if (listen(sockfd, 10) < 0)
	{
		debug_perror("无法监听连接");
		debug_perror("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
}

int tcp_accept(int sockfd, struct sockaddr *client_addr, int *addr_len)
{
	int fd;

	if ((fd = accept(sockfd, client_addr, addr_len)) < 0)
		debug_perror("无法接受连接");

	return (fd);
}

int tcp_sock_connect(char* dest_addr, int port)
{
	int fd;
	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if (fd < 0)
		debug_perror("无法创建套接字");
	else
	{

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr(dest_addr);
		sa.sin_port = htons((unsigned short)port);

		if (connect(fd, (struct sockaddr*) &sa, sizeof(sa)) < 0)
		{
			debug_perror("无法建立TCP连接");
			return -1;
		}
	}
	return(fd);
}

/*****************************************************************************/
int tcp_sock_open(struct in_addr ip, int port)
{
	int fd, turn_on_option_flag = 1, rcSockopt;

	struct sockaddr_in sa;

	/* Clear it out */
	memset((void *)&sa, 0, sizeof(sa));

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if (fd < 0) {
		debug_perror("无法创建套接字");
		debug_perror("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	sa.sin_family = AF_INET;
	memcpy((void *)&sa.sin_addr, (void *)&ip, sizeof(struct in_addr));
	sa.sin_port = htons((unsigned short)port);

	/* Reuse port */

	rcSockopt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&turn_on_option_flag, sizeof(turn_on_option_flag));

	/* bind() the socket to the interface */
	if (bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0) {
		debug_perror("无法绑定端口");
		debug_perror("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	return(fd);
}

/*****************************************************************************
* same as debug_perror but writes to debug output.
*
*****************************************************************************/
void debug_perror(char * msg) {
	debug("%s : %s\n", msg, strerror(errno));
}
/*****************************************************************************/
void debug(char *fmt, ...)
{
#define MAX_MESG_LEN 1024

	va_list args;
	char text[MAX_MESG_LEN];

	va_start(args, fmt);
	strcpy(text + vsprintf(text, fmt, args), "\r\n");
	va_end(args);

	fprintf(stderr, "%s", text);
}

/* Blue Burst encryption routines */

static void pso_crypt_init_key_bb(unsigned char *data)
{
	unsigned x;
	for (x = 0; x < 48; x += 3)
	{
		data[x] ^= 0x19;
		data[x + 1] ^= 0x16;
		data[x + 2] ^= 0x18;
	}
}


void pso_crypt_decrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
	length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *)&data[edx];
		ebx = ebx ^ pcry->tbl[5];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[4];
		ebp ^= *(unsigned long *)&data[edx + 4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12] + pcry->tbl[((ebp >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[3];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[2];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12] + pcry->tbl[((ebp >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[1];
		ebp = ebp ^ pcry->tbl[0];
		ebx = ebx ^ edi;
		*(unsigned long *)&data[edx] = ebp;
		*(unsigned long *)&data[edx + 4] = ebx;
		edx = edx + 8;
	}
}


void pso_crypt_encrypt_bb(PSO_CRYPT *pcry, unsigned char *data, unsigned
	length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long *)&data[edx];
		ebx = ebx ^ pcry->tbl[0];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[1];
		ebp ^= *(unsigned long *)&data[edx + 4];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12] + pcry->tbl[((ebp >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[2];
		ebx = ebx ^ edi;
		esi = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ esi ^ pcry->tbl[3];
		edi = ((pcry->tbl[(ebp >> 0x18) + 0x12] + pcry->tbl[((ebp >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebp & 0xff) + 0x312];
		edi = edi ^ pcry->tbl[4];
		ebp = ebp ^ pcry->tbl[5];
		ebx = ebx ^ edi;
		*(unsigned long *)&data[edx] = ebp;
		*(unsigned long *)&data[edx + 4] = ebx;
		edx = edx + 8;
	}
}

void encryptcopy(BANANA* client, const unsigned char* src, unsigned size)
{
	unsigned char* dest;

	// Bad pointer check...错误的指针检查
	if (((unsigned)client < (unsigned)connections[0]) ||
		((unsigned)client >(unsigned)connections[serverMaxConnections - 1]))
		return;
	if (TCP_BUFFER_SIZE - client->snddata < ((int)size + 7))
		client->todc = 1;
	else
	{
		dest = &client->sndbuf[client->snddata];
		memcpy(dest, src, size);
		while (size % 8)
			dest[size++] = 0x00;
		client->snddata += (int)size;
		pso_crypt_encrypt_bb(cipher_ptr, dest, size);
	}
}


void decryptcopy(unsigned char* dest, const unsigned char* src, unsigned size)
{
	memcpy(dest, src, size);
	pso_crypt_decrypt_bb(cipher_ptr, dest, size);
}


void pso_crypt_table_init_bb(PSO_CRYPT *pcry, const unsigned char *salt)
{
	unsigned long eax, ecx, edx, ebx, ebp, esi, edi, ou, x;
	unsigned char s[48];
	unsigned short* pcryp;
	unsigned short* bbtbl;
	unsigned short dx;

	pcry->cur = 0;
	pcry->mangle = NULL;
	pcry->size = 1024 + 18;

	memcpy(s, salt, sizeof(s));
	pso_crypt_init_key_bb(s);

	bbtbl = (unsigned short*)&bbtable[0];
	pcryp = (unsigned short*)&pcry->tbl[0];

	eax = 0;
	ebx = 0;

	for (ecx = 0;ecx<0x12;ecx++)
	{
		dx = bbtbl[eax++];
		dx = ((dx & 0xFF) << 8) + (dx >> 8);
		pcryp[ebx] = dx;
		dx = bbtbl[eax++];
		dx ^= pcryp[ebx++];
		pcryp[ebx++] = dx;
	}
	//sancaros 做了注释 不明所以
	/*
	pcry->tbl[0] = 0x243F6A88;
	pcry->tbl[1] = 0x85A308D3;
	pcry->tbl[2] = 0x13198A2E;
	pcry->tbl[3] = 0x03707344;
	pcry->tbl[4] = 0xA4093822;
	pcry->tbl[5] = 0x299F31D0;
	pcry->tbl[6] = 0x082EFA98;
	pcry->tbl[7] = 0xEC4E6C89;
	pcry->tbl[8] = 0x452821E6;
	pcry->tbl[9] = 0x38D01377;
	pcry->tbl[10] = 0xBE5466CF;
	pcry->tbl[11] = 0x34E90C6C;
	pcry->tbl[12] = 0xC0AC29B7;
	pcry->tbl[13] = 0xC97C50DD;
	pcry->tbl[14] = 0x3F84D5B5;
	pcry->tbl[15] = 0xB5470917;
	pcry->tbl[16] = 0x9216D5D9;
	pcry->tbl[17] = 0x8979FB1B;

	*/

	memcpy(&pcry->tbl[18], &bbtable[18], 4096);

	ecx = 0;
	//total key[0] length is min 0x412
	ebx = 0;

	while (ebx < 0x12)
	{
		//in a loop 在一个循环中
		ebp = ((unsigned long)(s[ecx])) << 0x18;
		eax = ecx + 1;
		edx = eax - ((eax / 48) * 48);
		eax = (((unsigned long)(s[edx])) << 0x10) & 0xFF0000;
		ebp = (ebp | eax) & 0xffff00ff;
		eax = ecx + 2;
		edx = eax - ((eax / 48) * 48);
		eax = (((unsigned long)(s[edx])) << 0x8) & 0xFF00;
		ebp = (ebp | eax) & 0xffffff00;
		eax = ecx + 3;
		ecx = ecx + 4;
		edx = eax - ((eax / 48) * 48);
		eax = (unsigned long)(s[edx]);
		ebp = ebp | eax;
		eax = ecx;
		edx = eax - ((eax / 48) * 48);
		pcry->tbl[ebx] = pcry->tbl[ebx] ^ ebp;
		ecx = edx;
		ebx++;
	}

	ebp = 0;
	esi = 0;
	ecx = 0;
	edi = 0;
	ebx = 0;
	edx = 0x48;

	while (edi < edx)
	{
		esi = esi ^ pcry->tbl[0];
		eax = esi >> 0x18;
		ebx = (esi >> 0x10) & 0xff;
		eax = pcry->tbl[eax + 0x12] + pcry->tbl[ebx + 0x112];
		ebx = (esi >> 8) & 0xFF;
		eax = eax ^ pcry->tbl[ebx + 0x212];
		ebx = esi & 0xff;
		eax = eax + pcry->tbl[ebx + 0x312];

		eax = eax ^ pcry->tbl[1];
		ecx = ecx ^ eax;
		ebx = ecx >> 0x18;
		eax = (ecx >> 0x10) & 0xFF;
		ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
		eax = (ecx >> 8) & 0xff;
		ebx = ebx ^ pcry->tbl[eax + 0x212];
		eax = ecx & 0xff;
		ebx = ebx + pcry->tbl[eax + 0x312];

		for (x = 0; x <= 5; x++)
		{
			ebx = ebx ^ pcry->tbl[(x * 2) + 2];
			esi = esi ^ ebx;
			ebx = esi >> 0x18;
			eax = (esi >> 0x10) & 0xFF;
			ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
			eax = (esi >> 8) & 0xff;
			ebx = ebx ^ pcry->tbl[eax + 0x212];
			eax = esi & 0xff;
			ebx = ebx + pcry->tbl[eax + 0x312];

			ebx = ebx ^ pcry->tbl[(x * 2) + 3];
			ecx = ecx ^ ebx;
			ebx = ecx >> 0x18;
			eax = (ecx >> 0x10) & 0xFF;
			ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
			eax = (ecx >> 8) & 0xff;
			ebx = ebx ^ pcry->tbl[eax + 0x212];
			eax = ecx & 0xff;
			ebx = ebx + pcry->tbl[eax + 0x312];
		}

		ebx = ebx ^ pcry->tbl[14];
		esi = esi ^ ebx;
		eax = esi >> 0x18;
		ebx = (esi >> 0x10) & 0xFF;
		eax = pcry->tbl[eax + 0x12] + pcry->tbl[ebx + 0x112];
		ebx = (esi >> 8) & 0xff;
		eax = eax ^ pcry->tbl[ebx + 0x212];
		ebx = esi & 0xff;
		eax = eax + pcry->tbl[ebx + 0x312];

		eax = eax ^ pcry->tbl[15];
		eax = ecx ^ eax;
		ecx = eax >> 0x18;
		ebx = (eax >> 0x10) & 0xFF;
		ecx = pcry->tbl[ecx + 0x12] + pcry->tbl[ebx + 0x112];
		ebx = (eax >> 8) & 0xff;
		ecx = ecx ^ pcry->tbl[ebx + 0x212];
		ebx = eax & 0xff;
		ecx = ecx + pcry->tbl[ebx + 0x312];

		ecx = ecx ^ pcry->tbl[16];
		ecx = ecx ^ esi;
		esi = pcry->tbl[17];
		esi = esi ^ eax;
		pcry->tbl[(edi / 4)] = esi;
		pcry->tbl[(edi / 4) + 1] = ecx;
		edi = edi + 8;
	}


	eax = 0;
	edx = 0;
	ou = 0;
	while (ou < 0x1000)
	{
		edi = 0x48;
		edx = 0x448;

		while (edi < edx)
		{
			esi = esi ^ pcry->tbl[0];
			eax = esi >> 0x18;
			ebx = (esi >> 0x10) & 0xff;
			eax = pcry->tbl[eax + 0x12] + pcry->tbl[ebx + 0x112];
			ebx = (esi >> 8) & 0xFF;
			eax = eax ^ pcry->tbl[ebx + 0x212];
			ebx = esi & 0xff;
			eax = eax + pcry->tbl[ebx + 0x312];

			eax = eax ^ pcry->tbl[1];
			ecx = ecx ^ eax;
			ebx = ecx >> 0x18;
			eax = (ecx >> 0x10) & 0xFF;
			ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
			eax = (ecx >> 8) & 0xff;
			ebx = ebx ^ pcry->tbl[eax + 0x212];
			eax = ecx & 0xff;
			ebx = ebx + pcry->tbl[eax + 0x312];

			for (x = 0; x <= 5; x++)
			{
				ebx = ebx ^ pcry->tbl[(x * 2) + 2];
				esi = esi ^ ebx;
				ebx = esi >> 0x18;
				eax = (esi >> 0x10) & 0xFF;
				ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
				eax = (esi >> 8) & 0xff;
				ebx = ebx ^ pcry->tbl[eax + 0x212];
				eax = esi & 0xff;
				ebx = ebx + pcry->tbl[eax + 0x312];

				ebx = ebx ^ pcry->tbl[(x * 2) + 3];
				ecx = ecx ^ ebx;
				ebx = ecx >> 0x18;
				eax = (ecx >> 0x10) & 0xFF;
				ebx = pcry->tbl[ebx + 0x12] + pcry->tbl[eax + 0x112];
				eax = (ecx >> 8) & 0xff;
				ebx = ebx ^ pcry->tbl[eax + 0x212];
				eax = ecx & 0xff;
				ebx = ebx + pcry->tbl[eax + 0x312];
			}

			ebx = ebx ^ pcry->tbl[14];
			esi = esi ^ ebx;
			eax = esi >> 0x18;
			ebx = (esi >> 0x10) & 0xFF;
			eax = pcry->tbl[eax + 0x12] + pcry->tbl[ebx + 0x112];
			ebx = (esi >> 8) & 0xff;
			eax = eax ^ pcry->tbl[ebx + 0x212];
			ebx = esi & 0xff;
			eax = eax + pcry->tbl[ebx + 0x312];

			eax = eax ^ pcry->tbl[15];
			eax = ecx ^ eax;
			ecx = eax >> 0x18;
			ebx = (eax >> 0x10) & 0xFF;
			ecx = pcry->tbl[ecx + 0x12] + pcry->tbl[ebx + 0x112];
			ebx = (eax >> 8) & 0xff;
			ecx = ecx ^ pcry->tbl[ebx + 0x212];
			ebx = eax & 0xff;
			ecx = ecx + pcry->tbl[ebx + 0x312];

			ecx = ecx ^ pcry->tbl[16];
			ecx = ecx ^ esi;
			esi = pcry->tbl[17];
			esi = esi ^ eax;
			pcry->tbl[(ou / 4) + (edi / 4)] = esi;
			pcry->tbl[(ou / 4) + (edi / 4) + 1] = ecx;
			edi = edi + 8;
		}
		ou = ou + 0x400;
	}
}

unsigned RleEncode(unsigned char *src, unsigned char *dest, unsigned src_size)
{
	unsigned char currChar, prevChar;             /* current and previous characters */
	unsigned short count;                /* number of characters in a run */
	unsigned src_end, dest_start;

	dest_start = (unsigned)dest;
	src_end = (unsigned)src + src_size;

	prevChar = 0xFF - *src;

	while ((unsigned)src < src_end)
	{
		currChar = *(dest++) = *(src++);

		if (currChar == prevChar)
		{
			if ((unsigned)src == src_end)
			{
				*(dest++) = 0;
				*(dest++) = 0;
			}
			else
			{
				count = 0;
				while (((unsigned)src < src_end) && (count < 0xFFF0))
				{
					if (*src == prevChar)
					{
						count++;
						src++;
						if ((unsigned)src == src_end)
						{
							*(unsigned short*)dest = count;
							dest += 2;
						}
					}
					else
					{
						*(unsigned short*)dest = count;
						dest += 2;
						prevChar = 0xFF - *src;
						break;
					}
				}
			}
		}
		else
			prevChar = currChar;
	}
	return (unsigned)dest - dest_start;
}

void RleDecode(unsigned char *src, unsigned char *dest, unsigned src_size)
{
	unsigned char currChar, prevChar;             /* current and previous characters */
	unsigned short count;                /* number of characters in a run */
	unsigned src_end;

	src_end = (unsigned)src + src_size;

	/* decode */

	prevChar = 0xFF - *src;     /* force next char to be different */

								/* read input until there's nothing left */

	while ((unsigned)src < src_end)
	{
		currChar = *(src++);

		*(dest++) = currChar;

		/* check for run */
		if (currChar == prevChar)
		{
			/* we have a run.  write it out. */
			count = *(unsigned short*)src;
			src += 2;
			while (count > 0)
			{
				*(dest++) = currChar;
				count--;
			}

			prevChar = 0xFF - *src;     /* force next char to be different */
		}
		else
		{
			/* no run 未运行 */
			prevChar = currChar;
		}
	}
}

/* expand a key (makes a rc4_key)  展开一个密钥（生成一个rc4_密钥） */

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key)
{
	unsigned index1, index2, counter;
	unsigned char *state;

	state = key->state;

	for (counter = 0; counter < 256; counter++)
		state[counter] = (unsigned char)counter;

	key->x = key->y = index1 = index2 = 0;

	for (counter = 0; counter < 256; counter++) {
		index2 = (keydata[index1] + state[counter] + index2) & 255;

		/* swap */
		state[counter] ^= state[index2];
		state[index2] ^= state[counter];
		state[counter] ^= state[index2];

		index1 = (index1 + 1) % len;
	}
}

/* 可逆加密, 将编码缓冲区更新密钥 */

void rc4(unsigned char *buffer, unsigned len, struct rc4_key *key)
{
	unsigned x, y, xorIndex, counter;
	unsigned char *state;

	/* get local copies 获取本地副本 */
	x = key->x; y = key->y;
	state = key->state;

	for (counter = 0; counter < len; counter++) {
		x = (x + 1) & 255;
		y = (state[x] + y) & 255;

		/* swap 转换坐标 */
		state[x] ^= state[y];
		state[y] ^= state[x];
		state[x] ^= state[y];

		xorIndex = (state[y] + state[x]) & 255;

		buffer[counter] ^= state[xorIndex];
	}

	key->x = x; key->y = y;
}
//压缩舰船数据包
void compressShipPacket(ORANGE* ship, unsigned char* src, unsigned long src_size)
{
	unsigned char* dest;
	unsigned long result;

	if (ship->sockfd >= 0)
	{
		if (PACKET_BUFFER_SIZE - ship->snddata < (int)(src_size + 100))
			initialize_logon();
		else
		{
			dest = &ship->sndbuf[ship->snddata];
			// Store the original packet size before RLE compression at offset 0x04 of the new packet. 将RLE压缩前的原始数据包大小存储在新数据包的偏移量0x04处 
			dest += 4;
			*(unsigned *)dest = src_size;
			// Compress packet using RLE, storing at offset 0x08 of new packet. 使用RLE压缩数据包,在新数据包的偏移量0x08处存储.
			//
			// result = size of RLE compressed data + a DWORD for the original packet size.
			result = RleEncode(src, dest + 4, src_size) + 4;
			// Encrypt with RC4
			rc4(dest, result, &ship->sc_key);
			// Increase result by the size of a DWORD for the final ship packet size.
			result += 4;
			// Copy it to the front of the packet.把它复制到数据包的前面
			*(unsigned *)&ship->sndbuf[ship->snddata] = result;
			ship->snddata += (int)result;
		}
	}
}

//解压舰船数据包
void decompressShipPacket(ORANGE* ship, unsigned char* dest, unsigned char* src)
{
	unsigned src_size, dest_size;
	unsigned char *srccpy;

	if (ship->crypt_on)
	{
		src_size = *(unsigned *)src;
		src_size -= 8;
		src += 4;
		srccpy = src;
		// Decrypt RC4
		rc4(src, src_size + 4, &ship->cs_key);
		// The first four bytes of the src should now contain the expected uncompressed data size.
		dest_size = *(unsigned *)srccpy;
		// Increase expected size by 4 before inserting into the destination buffer.  (To take account for the packet
		// size DWORD...)
		dest_size += 4;
		*(unsigned *)dest = dest_size;
		// Decompress the data...
		RleDecode(srccpy + 4, dest + 4, src_size);
	}
	else
	{
		src_size = *(unsigned *)src;
		memcpy(dest + 4, src + 4, src_size);
		src_size += 4;
		*(unsigned *)dest = src_size;
	}
}