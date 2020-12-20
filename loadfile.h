#pragma once
long CalculateChecksum(void* data, unsigned long size)
{
	long offset, y, cs = 0xFFFFFFFF;
	for (offset = 0; offset < (long)size; offset++)
	{
		cs ^= *(unsigned char*)((long)data + offset);
		for (y = 0; y < 8; y++)
		{
			if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
			else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
		}
	}
	return (cs ^ 0xFFFFFFFF);
}


void LoadBattleParam(BATTLEPARAM* dest, const char* filename, unsigned num_records, long expected_checksum)
{
	FILE* fp;
	long battle_checksum;

	printf("载入 %s ... ", filename);
	fp = fopen(filename, "rb");
	if (!fp)
	{
		printf("%s 文件缺失了.\n", filename);
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	if ((fread(dest, 1, sizeof(BATTLEPARAM) * num_records, fp) != sizeof(BATTLEPARAM) * num_records))
	{
		printf("%s 文件已损坏.\n", filename);
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	fclose(fp);

	printf("确认!\n");

	battle_checksum = CalculateChecksum(dest, sizeof(BATTLEPARAM) * num_records);

	if (battle_checksum != expected_checksum)
	{
		printf("文件校对: %08x\n", battle_checksum);
		printf("警告: 战斗参数文件已修改.\n");
	}
}

unsigned char qpd_buffer[PRS_BUFFER];
unsigned char qpdc_buffer[PRS_BUFFER];
//LOBBY fakelobby;

void LoadQuests(const char* filename, unsigned category)
{
	/*unsigned oldIndex;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	LOBBY *l;*/
	FILE* fp;
	FILE* qf;
	FILE* qd;
	unsigned qs;
	char qfile[256];
	char qfile2[256];
	char qfile3[256];
	char qfile4[256];
	char qname[256];
	unsigned qnl = 0;
	QUEST* q;
	unsigned ch, ch2, ch3, ch4, ch5, qf2l;
	unsigned short qps, qpc;
	unsigned qps2;
	QUEST_MENU* qm;
	unsigned* ed;
	unsigned ed_size, ed_ofs;
	unsigned num_records, num_objects, qm_ofs = 0, qb_ofs = 0;
	char true_filename[16];
	QDETAILS* ql;
	int extf;

	qm = &quest_menus[category];
	printf("正在从 %s 文件中载入任务列表... \n", filename);
	fp = fopen(filename, "r");
	if (!fp)
	{
		printf("%s 文件已缺失.\n", filename);
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	while (fgets(&qfile[0], 255, fp) != NULL)
	{
		for (ch = 0;ch<strlen(&qfile[0]);ch++)
			if ((qfile[ch] == 10) || (qfile[ch] == 13))
				qfile[ch] = 0; // Reserved
		qfile3[0] = 0;
		strcat(&qfile3[0], "quest\\");
		strcat(&qfile3[0], &qfile[0]);
		memcpy(&qfile[0], &qfile3[0], strlen(&qfile3[0]) + 1);
		strcat(&qfile3[0], "quest.lst");
		qf = fopen(&qfile3[0], "r");
		if (!qf)
		{
			printf("%s 文件已缺失.\n", qfile3);
			printf("按下 [回车键] 退出");
			gets_s(&dp[0], 0);
			exit(1);
		}
		if (fgets(&qname[0], 64, fp) == NULL)
		{
			printf("%s 文件已损坏.\n", filename);
			printf("按下 [回车键] 退出");
			gets_s(&dp[0], 0);
			exit(1);
		}
		for (ch = 0;ch<64;ch++)
		{
			if (qname[ch] != 0x00)
			{
				qm->c_names[qm->num_categories][ch * 2] = qname[ch];
				qm->c_names[qm->num_categories][(ch * 2) + 1] = 0x00;
			}
			else
			{
				qm->c_names[qm->num_categories][ch * 2] = 0x00;
				qm->c_names[qm->num_categories][(ch * 2) + 1] = 0x00;
				break;
			}
		}
		if (fgets(&qname[0], 120, fp) == NULL)
		{
			printf("%s 文件已损坏.\n", filename);
			printf("按下 [回车键] 退出");
			gets_s(&dp[0], 0);
			exit(1);
		}
		for (ch = 0;ch<120;ch++)
		{
			if (qname[ch] != 0x00)
			{
				if (qname[ch] == 0x24)
					qm->c_desc[qm->num_categories][ch * 2] = 0x0A;
				else
					qm->c_desc[qm->num_categories][ch * 2] = qname[ch];
				qm->c_desc[qm->num_categories][(ch * 2) + 1] = 0x00;
			}
			else
			{
				qm->c_desc[qm->num_categories][ch * 2] = 0x00;
				qm->c_desc[qm->num_categories][(ch * 2) + 1] = 0x00;
				break;
			}
		}
		memcpy(&qfile2[0], &qfile[0], strlen(&qfile[0]) + 1);
		qf2l = strlen(&qfile2[0]);
		while (fgets(&qfile2[qf2l], 255, qf) != NULL)
		{
			for (ch = 0;ch<strlen(&qfile2[0]);ch++)
				if ((qfile2[ch] == 10) || (qfile2[ch] == 13))
					qfile2[ch] = 0; // Reserved

			for (ch4 = 0;ch4<numLanguages;ch4++)
			{
				memcpy(&qfile4[0], &qfile2[0], strlen(&qfile2[0]) + 1);

				// Add extension to .qst and .raw for languages

				extf = 0;

				if (strlen(languageExts[ch4]))
				{
					if ((strlen(&qfile4[0]) - qf2l) > 3)
						for (ch5 = qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5 + 1]) == 113) &&
								(tolower(qfile4[ch5 + 2]) == 115) &&
								(tolower(qfile4[ch5 + 3]) == 116))
							{
								qfile4[ch5] = 0;
								strcat(&qfile4[ch5], "_");
								strcat(&qfile4[ch5], languageExts[ch4]);
								strcat(&qfile4[ch5], ".qst");
								extf = 1;
								break;
							}
						}

					if (((strlen(&qfile4[0]) - qf2l) > 3) && (!extf))
						for (ch5 = qf2l;ch5<strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5 + 1]) == 114) &&
								(tolower(qfile4[ch5 + 2]) == 97) &&
								(tolower(qfile4[ch5 + 3]) == 119))
							{
								qfile4[ch5] = 0;
								strcat(&qfile4[ch5], "_");
								strcat(&qfile4[ch5], languageExts[ch4]);
								strcat(&qfile4[ch5], ".raw");
								break;
							}
						}
				}

				qd = fopen(&qfile4[0], "rb");
				if (qd != NULL)
				{
					if (ch4 == 0)
					{
						q = &quests[numQuests];
						memset(q, 0, sizeof(QUEST));
					}
					ql = q->ql[ch4] = malloc(sizeof(QDETAILS));
					memset(ql, 0, sizeof(QDETAILS));
					fseek(qd, 0, SEEK_END);
					ql->qsize = qs = ftell(qd);
					fseek(qd, 0, SEEK_SET);
					ql->qdata = malloc(qs);
					questsMemory += qs;
					fread(ql->qdata, 1, qs, qd);
					ch = 0;
					ch2 = 0;
					while (ch < qs)
					{
						qpc = *(unsigned short*)&ql->qdata[ch + 2];
						if ((qpc == 0x13) && (strstr(&ql->qdata[ch + 8], ".bin")) && (ch2 < PRS_BUFFER))
						{
							memcpy(&true_filename[0], &ql->qdata[ch + 8], 16);
							qps2 = *(unsigned*)&ql->qdata[ch + 0x418];
							memcpy(&qpd_buffer[ch2], &ql->qdata[ch + 0x18], qps2);
							ch2 += qps2;
						}
						else
							if (ch2 >= PRS_BUFFER)
							{
								printf("PRS 缓冲区太小了...\n");
								printf("按下 [回车键] 退出");
								gets_s(&dp[0], 0);
								exit(1);
							}
						qps = *(unsigned short*)&ql->qdata[ch];
						if (qps % 8)
							qps += (8 - (qps % 8));
						ch += qps;
					}
					ed_size = prs_decompress(&qpd_buffer[0], &qpdc_buffer[0]);
					if (ed_size > PRS_BUFFER)
					{
						printf("内存受损!\n");
						//printf(ed_size);
						printf("\n按下 [回车键] 退出");
						gets_s(&dp[0], 0);
						exit(1);
					}
					fclose(qd);

					if (ch4 == 0)
						qm->quest_indexes[qm->num_categories][qm->quest_counts[qm->num_categories]++] = numQuests++;

					qnl = 0;
					for (ch2 = 0x18;ch2<0x48;ch2 += 2)
					{
						if (*(unsigned short *)&qpdc_buffer[ch2] != 0x0000)
						{
							qname[qnl] = qpdc_buffer[ch2];
							if (qname[qnl] < 32)
								qname[qnl] = 32;
							qnl++;
						}
						else
							break;
					}

					qname[qnl] = 0;
					memcpy(&ql->qname[0], &qpdc_buffer[0x18], 0x40);
					ql->qname[qnl] = 0x0000;
					memcpy(&ql->qsummary[0], &qpdc_buffer[0x58], 0x100);
					memcpy(&ql->qdetails[0], &qpdc_buffer[0x158], 0x200);

					if (ch4 == 0)
					{
						// Load enemy data

						ch = 0;
						ch2 = 0;

						while (ch < qs)
						{
							qpc = *(unsigned short*)&ql->qdata[ch + 2];
							if ((qpc == 0x13) && (strstr(&ql->qdata[ch + 8], ".dat")) && (ch2 < PRS_BUFFER))
							{
								qps2 = *(unsigned *)&ql->qdata[ch + 0x418];
								memcpy(&qpd_buffer[ch2], &ql->qdata[ch + 0x18], qps2);
								ch2 += qps2;
							}
							else
								if (ch2 >= PRS_BUFFER)
								{
									printf("PRS 缓冲区太小了...\n");
									printf("按下 [回车键] 退出");
									gets_s(&dp[0], 0);
									exit(1);
								}

							qps = *(unsigned short*)&ql->qdata[ch];
							if (qps % 8)
								qps += (8 - (qps % 8));
							ch += qps;
						}
						ed_size = prs_decompress(&qpd_buffer[0], &qpdc_buffer[0]);
						if (ed_size > PRS_BUFFER)
						{
							printf("内存受损!\n");
							//printf(ed_size);
							printf("按下 [回车键] 退出");
							gets_s(&dp[0], 0);
							exit(1);
						}
						ed_ofs = 0;
						ed = (unsigned*)&qpdc_buffer[0];
						qm_ofs = 0;
						qb_ofs = 0;
						num_objects = 0;
						while (ed_ofs < ed_size)
						{
							switch (*ed)
							{
							case 0x01:
								if (ed[2] > 17)
								{
									printf("任务区域超出范围!\n");
									printf("按下 [回车键] 退出");
									gets_s(&dp[0], 0);
									exit(1);
								}
								num_records = ed[3] / 68L;
								num_objects += num_records;
								*(unsigned *)&qpd_buffer[qb_ofs] = *(unsigned *)&ed[2];
								qb_ofs += 4;
								//printf ("area: %u, object count: %u\n", ed[2], num_records);
								*(unsigned *)&qpd_buffer[qb_ofs] = num_records;
								qb_ofs += 4;
								memcpy(&qpd_buffer[qb_ofs], &ed[4], ed[3]);
								qb_ofs += num_records * 68L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x03:
								//printf ("data type: %u\n", *ed );
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							case 0x02:
								num_records = ed[3] / 72L;
								*(unsigned *)&dp[qm_ofs] = *(unsigned *)&ed[2];
								//printf ("area: %u, mid count: %u\n", ed[2], num_records);
								if (ed[2] > 17)
								{
									printf("任务区域超出范围!\n");
									printf("按下 [回车键] 退出");
									gets_s(&dp[0], 0);
									exit(1);
								}
								qm_ofs += 4;
								*(unsigned *)&dp[qm_ofs] = num_records;
								qm_ofs += 4;
								memcpy(&dp[qm_ofs], &ed[4], ed[3]);
								qm_ofs += num_records * 72L;
								ed_ofs += ed[1]; // Read how many bytes to skip
								ed += ed[1] / 4L;
								break;
							default:
								// Probably done loading...
								ed_ofs = ed_size;
								break;
							}
						}

						// Do objects
						q->max_objects = num_objects;
						questsMemory += qb_ofs;
						q->objectdata = malloc(qb_ofs);
						// Need to sort first...
						ch3 = 0;
						for (ch = 0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < qb_ofs)
							{
								unsigned qa;

								qa = *(unsigned *)&qpd_buffer[ch2];
								num_records = *(unsigned *)&qpd_buffer[ch2 + 4];
								if (qa == ch)
								{
									memcpy(&q->objectdata[ch3], &qpd_buffer[ch2 + 8], (num_records * 68));
									ch3 += (num_records * 68);
								}
								ch2 += (num_records * 68) + 8;
							}
						}

						// Do enemies

						qm_ofs += 4;
						questsMemory += qm_ofs;
						q->mapdata = malloc(qm_ofs);
						*(unsigned *)q->mapdata = qm_ofs;
						// Need to sort first...
						ch3 = 4;
						for (ch = 0;ch<18;ch++)
						{
							ch2 = 0;
							while (ch2 < (qm_ofs - 4))
							{
								unsigned qa;

								qa = *(unsigned *)&dp[ch2];
								num_records = *(unsigned *)&dp[ch2 + 4];
								if (qa == ch)
								{
									memcpy(&q->mapdata[ch3], &dp[ch2], (num_records * 72) + 8);
									ch3 += (num_records * 72) + 8;
								}
								ch2 += (num_records * 72) + 8;
							}
						}
						for (ch = 0;ch<num_objects;ch++)
						{
							// Swap fields in advance
							dp[0] = q->objectdata[(ch * 68) + 0x37];
							dp[1] = q->objectdata[(ch * 68) + 0x36];
							dp[2] = q->objectdata[(ch * 68) + 0x35];
							dp[3] = q->objectdata[(ch * 68) + 0x34];
							*(unsigned *)&q->objectdata[(ch * 68) + 0x34] = *(unsigned *)&dp[0];
						}
						printf("已载入任务 %s (%s),\n对象数量: %u, 敌人数量: %u\n", qname, true_filename, num_objects, (qm_ofs - 4) / 72L);
					}
					/*
					// Time to load the map data...
					l = &fakelobby;
					memset ( l, 0, sizeof (LOBBY) );
					l->bptable = &ep2battle[0];
					memset ( &l->mapData[0], 0, 0xB50 * sizeof (MAP_MONSTER) ); // Erase!
					l->mapIndex = 0;
					l->rareIndex = 0;
					for (ch=0;ch<0x20;ch++)
					l->rareData[ch] = 0xFF;

					qmap = q->mapdata;
					qm_length = *(unsigned*) qmap;
					qmap += 4;
					ch = 4;
					while ( ( qm_length - ch ) >= 80 )
					{
					oldIndex = l->mapIndex;
					qa = *(unsigned*) qmap; // Area
					qmap += 4;
					nr = *(unsigned*) qmap; // Number of monsters
					qmap += 4;
					if ( ( l->episode == 0x03 ) && ( qa > 5 ) )
					ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					if ( ( l->episode == 0x02 ) && ( qa > 15 ) )
					ParseMapData ( l, (MAP_MONSTER*) qmap, 1, nr );
					else
					ParseMapData ( l, (MAP_MONSTER*) qmap, 0, nr );
					qmap += ( nr * 72 );
					ch += ( ( nr * 72 ) + 8 );
					debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
					}
					exit (1);
					*/
				}
				else
				{
					if (ch4 == 0)
					{
						printf("任务文件 %s 已缺失!  无法载入该任务.\n", qfile4);
						printf("按下 [回车键] 退出");
						gets_s(&dp[0], 0);
						exit(1);
					}
					else
					{
						printf("注意: 交替任务语言文件 %s 文件已缺失.\n", qfile4);
					}
				}
			}
		}
		fclose(qf);
		qm->num_categories++;
	}
	fclose(fp);
}

unsigned csv_lines = 0;
char* csv_params[1024][64]; // 1024 lines which can carry 64 parameters each

							// Release RAM from loaded CSV

void FreeCSV()
{
	unsigned ch, ch2;

	for (ch = 0;ch<csv_lines;ch++)
	{
		for (ch2 = 0;ch2<64;ch2++)
			if (csv_params[ch][ch2] != NULL) free(csv_params[ch][ch2]);
	}
	csv_lines = 0;
	memset(&csv_params, 0, sizeof(csv_params));
}

// Load CSV into RAM

void LoadCSV(const char* filename)
{
	FILE* fp;
	char csv_data[1024];
	unsigned ch, ch2, ch3 = 0;
	//unsigned ch4;
	int open_quote = 0;
	char* csv_param;

	csv_lines = 0;
	memset(&csv_params, 0, sizeof(csv_params));

	//printf ("Loading CSV file %s ...\n", filename );

	if ((fp = fopen(filename, "r")) == NULL)
	{
		printf("参数文件 %s 似乎缺失了.\n", filename);
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	while (fgets(&csv_data[0], 1023, fp) != NULL)
	{
		// ch2 = current parameter we're on
		// ch3 = current index into the parameter string
		ch2 = ch3 = 0;
		open_quote = 0;
		csv_param = csv_params[csv_lines][0] = malloc(256); // allocate memory for parameter
		for (ch = 0;ch<strlen(&csv_data[0]);ch++)
		{
			if ((csv_data[ch] == 44) && (!open_quote)) // comma not surrounded by quotations
			{
				csv_param[ch3] = 0; // null terminate current parameter
				ch3 = 0;
				ch2++; // new parameter
				csv_param = csv_params[csv_lines][ch2] = malloc(256); // allocate memory for parameter
			}
			else
			{
				if (csv_data[ch] == 34) // quotation mark
					open_quote = !open_quote;
				else
					if (csv_data[ch] > 31) // no loading low ascii
						csv_param[ch3++] = csv_data[ch];
			}
		}
		if (ch3)
		{
			ch2++;
			csv_param[ch3] = 0;
		}
		/*
		for (ch4=0;ch4<ch2;ch4++)
		printf ("%s,", csv_params[csv_lines][ch4]);
		printf ("\n");
		*/
		csv_lines++;
		if (csv_lines > 1023)
		{
			printf("CSV 文件的条目太多.\n");
			printf("按下 [回车键] 退出");
			gets_s(&dp[0], 0);
			exit(1);
		}
	}
	printf("已载入 %u 行...\r\n", csv_lines);
	fclose(fp);
}

void LoadArmorParam()
{
	unsigned ch, wi1;

	LoadCSV("param\\armorpmt.ini");
	for (ch = 0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte(&csv_params[ch][0][6]);
		armor_dfpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][17]);
		armor_evpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][18]);
		armor_equip_table[wi1] = (unsigned char)atoi(csv_params[ch][10]);
		armor_level_table[wi1] = (unsigned char)atoi(csv_params[ch][11]);
		//printf ("armor index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, armor_dfpvar_table[wi1], armor_evpvar_table[wi1], armor_equip_table[wi1], armor_level_table[wi1]);
	}
	FreeCSV();
	LoadCSV("param\\shieldpmt.ini");
	for (ch = 0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte(&csv_params[ch][0][6]);
		barrier_dfpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][17]);
		barrier_evpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][18]);
		barrier_equip_table[wi1] = (unsigned char)atoi(csv_params[ch][10]);
		barrier_level_table[wi1] = (unsigned char)atoi(csv_params[ch][11]);
		//printf ("barrier index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, barrier_dfpvar_table[wi1], barrier_evpvar_table[wi1], barrier_equip_table[wi1], barrier_level_table[wi1]);
	}
	FreeCSV();
	// Set up the stack table too.
	for (ch = 0;ch<0x09;ch++)
	{
		if (ch != 0x02)
			stackable_table[ch] = 10;
	}
	stackable_table[0x10] = 99;

}

void LoadWeaponParam()
{
	unsigned ch, wi1, wi2;

	LoadCSV("param\\weaponpmt.ini");
	for (ch = 0;ch<csv_lines;ch++)
	{
		wi1 = hexToByte(&csv_params[ch][0][4]);
		wi2 = hexToByte(&csv_params[ch][0][6]);
		weapon_equip_table[wi1][wi2] = (unsigned)atoi(csv_params[ch][6]);
		*(unsigned short*)&weapon_atpmax_table[wi1][wi2] = (unsigned)atoi(csv_params[ch][8]);
		grind_table[wi1][wi2] = (unsigned char)atoi(csv_params[ch][14]);
		if ((((wi1 >= 0x70) && (wi1 <= 0x88)) ||
			((wi1 >= 0xA5) && (wi1 <= 0xA9))) &&
			(wi2 == 0x10))
			special_table[wi1][wi2] = 0x0B; // Fix-up S-Rank King's special
		else
			special_table[wi1][wi2] = (unsigned char)atoi(csv_params[ch][16]);
		//printf ("weapon index %02x%02x, eq: %u, grind: %u, atpmax: %u, special: %u \n", wi1, wi2, weapon_equip_table[wi1][wi2], grind_table[wi1][wi2], weapon_atpmax_table[wi1][wi2], special_table[wi1][wi2] );
	}
	FreeCSV();
}

void LoadTechParam()
{
	unsigned ch, ch2;

	LoadCSV("param\\tech.ini");
	if (csv_lines != 19)
	{
		printf("科技 tech.ini 文件CSV内容已损坏.\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	for (ch = 0;ch<19;ch++) // For technique
	{
		for (ch2 = 0;ch2<12;ch2++) // For class
		{
			if (csv_params[ch][ch2 + 1])
				max_tech_level[ch][ch2] = ((char)atoi(csv_params[ch][ch2 + 1])) - 1;
			else
			{
				printf("科技 tech.ini 文件CSV内容已损坏.\n");
				printf("按下 [回车键] 退出");
				gets_s(&dp[0], 0);
				exit(1);
			}
		}
	}
	FreeCSV();
}

void LoadShopData2()
{
	FILE *fp;
	fp = fopen("shop\\shop2.dat", "rb");
	if (!fp)
	{
		printf("shop\\shop2.dat 文件已缺失.");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	fread(&equip_prices[0], 1, sizeof(equip_prices), fp);
	fclose(fp);
}