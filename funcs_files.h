#pragma once

void load_mask_file()
{
	char mask_data[255];
	unsigned ch = 0;

	FILE* fp;

	// Load masks.txt for IP ban masks

	num_masks = 0;

	if ((fp = fopen("masks.txt", "r")))
	{
		while (fgets(&mask_data[0], 255, fp) != NULL)
		{
			if (mask_data[0] != 0x23)
			{
				ch = strlen(&mask_data[0]);
				if (mask_data[ch - 1] == 0x0A)
					mask_data[ch--] = 0x00;
				mask_data[ch] = 0;
			}
			convertMask(&mask_data[0], ch + 1, &ship_banmasks[num_masks++][0]);
		}
	}
}

void load_language_file()
{
	FILE* fp;
	FILE* fp_messages;
	char lang_data[256];
	wchar_t message_data[256];
	int langExt = 0;
	unsigned ch;
	unsigned cha;
	unsigned chb;

	for (ch = 0;ch<10;ch++)
	{
		languageNames[ch] = malloc(256);
		memset(languageNames[ch], 0, 256);
		languageExts[ch] = malloc(256);
		memset(languageExts[ch], 0, 256);
		for (cha = 0;cha < 256;cha++)
		{
			languageMessages[ch][cha] = malloc(256);
			memset(languageMessages[ch][cha], 0, 256);
		}
	}

	if ((fp = fopen("lang.ini", "r")) == NULL)
	{
		printf("语言文件不存在...\n将只使用中文...\n\n");
		numLanguages = 1;
		strcat(languageNames[0], "Chinese");
	}
	else
	{
		while ((fgets(&lang_data[0], 255, fp) != NULL) && (numLanguages < 10))
		{
			if (!langExt)
			{
				memcpy(languageNames[numLanguages], &lang_data[0], strlen(&lang_data[0]) + 1);
				for (ch = 0;ch<strlen(languageNames[numLanguages]);ch++)
					if ((languageNames[numLanguages][ch] == 10) || (languageNames[numLanguages][ch] == 13))
						languageNames[numLanguages][ch] = 0;
				langExt = 1;
			}
			else
			{
				memcpy(languageExts[numLanguages], &lang_data[0], strlen(&lang_data[0]) + 1);
				for (ch = 0;ch<strlen(languageExts[numLanguages]);ch++)
					if ((languageExts[numLanguages][ch] == 10) || (languageExts[numLanguages][ch] == 13))
						languageExts[numLanguages][ch] = 0;
				numLanguages++;
				printf("语言文件 %u (%s:%s)\n", numLanguages, languageNames[numLanguages - 1], languageExts[numLanguages - 1]);
				langExt = 0;

				//custom lang messages
				char messageDir[256] = "";
				if (strlen(languageExts[numLanguages - 1])) {
					strcat(messageDir, "messages/messages_");
					strcat(messageDir, languageExts[numLanguages - 1]);
					strcat(messageDir, ".ini");
				}
				else {
					strcat(messageDir, "messages/messages.ini");
				}
				if ((fp_messages = fopen(messageDir, "r,ccs=UTF-8")) == NULL)
				{
					printf("%s 自定义信息文件 (%s) 不存在...\n将使用中文 (默认) 信息显示...\n\n", languageNames[numLanguages - 1], messageDir);
				}
				else
				{
					int count = 0;
					while ((fgetws(&message_data[0], 255, fp_messages) != NULL))
					{
						memcpy(languageMessages[numLanguages - 1][count], &message_data[0], wstrlen(&message_data[0]) + 1);
						if ((languageMessages[numLanguages - 1][count][wcslen(languageMessages[numLanguages - 1][count]) - 1] == 10) || (languageMessages[numLanguages - 1][count][wcslen(languageMessages[numLanguages - 1][count]) - 1] == 13))
							languageMessages[numLanguages - 1][count][wcslen(languageMessages[numLanguages - 1][count]) - 1] = 0;

						for (chb = 0;chb<wcslen(languageMessages[numLanguages - 1][count]);chb++)
							if ((languageMessages[numLanguages - 1][count][chb - 1] == 92) && (languageMessages[numLanguages - 1][count][chb] == 110))
							{
								languageMessages[numLanguages - 1][count][chb - 1] = 32;
								languageMessages[numLanguages - 1][count][chb] = 10;
							}
						count++;
					}

					fclose(fp_messages);
				}
			}
		}
		fclose(fp);
		if (numLanguages < 1)
		{
			numLanguages = 1;
			strcat(languageNames[0], "Chinese");
		}
	}
}

void load_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch = 0;

	FILE* fp;

	EXPERIENCE_RATE = 1; // Default to 100% EXP

	if ((fp = fopen("ship.ini", "r")) == NULL)
	{
		printf("舰船设置文件 ship.ini 缺失了.\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	else
		while (fgets(&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					ch = strlen(&config_data[0]);
					if (config_data[ch - 1] == 0x0A)
						config_data[ch--] = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case 0x00:
					// Login server host name or IP
				{
					unsigned p;
					unsigned alpha;
					alpha = 0;
					for (p = 0;p<ch;p++)
						if (((config_data[p] >= 65) && (config_data[p] <= 90)) ||
							((config_data[p] >= 97) && (config_data[p] <= 122)))
						{
							alpha = 1;
							break;
						}
					if (alpha)
					{
						struct hostent *IP_host;
						//这里域名竟然-1,待解决
						//config_data[strlen(&config_data[0]) - 1] = 0x00;
						config_data[strlen(&config_data[0])] = 0x00;
						printf("解析中 %s ...\n", (char*)&config_data[0]);
						IP_host = gethostbyname(&config_data[0]);
						if (!IP_host)
						{
							printf("无法解析该域名.");
							printf("按下 [回车键] 退出");
							gets_s(&dp[0], 0);
							exit(1);
						}
						*(unsigned *)&serverIP[0] = *(unsigned *)IP_host->h_addr;
					}
					else
						convertIPString(&config_data[0], ch + 1, 1, &serverIP[0]);
				}
				break;
				/*case 0x00:
					// Server IP address
				{
					if ((config_data[0] == 0x41) || (config_data[0] == 0x61))
					{
						autoIP = 1;
					}
					else
					{
						convertIPString(&config_data[0], ch + 1, 1, &serverIP[0]);
					}
				}
				break;*/
				case 0x01:
					// Server Listen Port
					serverPort = atoi(&config_data[0]);
					break;
				case 0x02:
					// Number of blocks
					serverBlocks = atoi(&config_data[0]);
					if (serverBlocks > 10)
					{
						printf("你不能托管超过10个船舱... 已自动调整.\n");
						serverBlocks = 10;
					}
					if (serverBlocks == 0)
					{
						printf("必须至少托管1个舰仓... 已自动调整.\n");
						serverBlocks = 1;
					}
					break;
				case 0x03:
					// Max Client Connections
					serverMaxConnections = atoi(&config_data[0]);
					if (serverMaxConnections > (serverBlocks * 180))
					{
						printf("\n您尝试的连接数超过了舰船服务器所允许的数量.\n已自动调整为180...\n");
						serverMaxConnections = serverBlocks * 180;
					}
					if (serverMaxConnections > SHIP_COMPILED_MAX_CONNECTIONS)
					{
						printf("此版本的船舶服务软件尚未编译为接受\n超过%u个连接.\n已自动调整...\n", SHIP_COMPILED_MAX_CONNECTIONS);
						serverMaxConnections = SHIP_COMPILED_MAX_CONNECTIONS;
					}
					break;
				case 0x04:
					// Login server host name or IP
				{
					unsigned p;
					unsigned alpha;
					alpha = 0;
					for (p = 0;p<ch;p++)
						if (((config_data[p] >= 65) && (config_data[p] <= 90)) ||
							((config_data[p] >= 97) && (config_data[p] <= 122)))
						{
							alpha = 1;
							break;
						}
					if (alpha)
					{
						struct hostent *IP_host;
						//这里域名竟然-1,待解决
						//config_data[strlen(&config_data[0]) - 1] = 0x00;
						config_data[strlen(&config_data[0])] = 0x00;
						printf("解析中 %s ...\n", (char*)&config_data[0]);
						IP_host = gethostbyname(&config_data[0]);
						if (!IP_host)
						{
							printf("无法解析该域名.");
							printf("按下 [回车键] 退出");
							gets_s(&dp[0], 0);
							exit(1);
						}
						*(unsigned *)&loginIP[0] = *(unsigned *)IP_host->h_addr;
					}
					else
						convertIPString(&config_data[0], ch + 1, 1, &loginIP[0]);
				}
				break;
				case 0x05:
					// Ship Name
					memset(&Ship_Name[0], 0, 255);
					memcpy(&Ship_Name[0], &config_data[0], ch + 1);
					Ship_Name[12] = 0x00;
					break;
				case 0x06:
					// Event
					shipEvent = (unsigned char)atoi(&config_data[0]);
					PacketDA[0x04] = shipEvent;
					break;
				case 0x07:
					WEAPON_DROP_RATE = atoi(&config_data[0]);
					break;
				case 0x08:
					ARMOR_DROP_RATE = atoi(&config_data[0]);
					break;
				case 0x09:
					MAG_DROP_RATE = atoi(&config_data[0]);
					break;
				case 0x0A:
					TOOL_DROP_RATE = atoi(&config_data[0]);
					break;
				case 0x0B:
					MESETA_DROP_RATE = atoi(&config_data[0]);
					break;
				case 0x0C:
					EXPERIENCE_RATE = atoi(&config_data[0]);
					if (EXPERIENCE_RATE > 99)
					{
						printf("\n警告: 你的经验倍率设置非常高.\n");
						printf("关于舰船服务器 版本0.038, 你现在只用个位数\n");
						printf("表示100%%增量. 例如.1或2\n");
						printf("如果您故意设置了 %u%% 经验的超高指,\n", EXPERIENCE_RATE * 100);
						printf("按下 [回车键] 继续, 或者按下 CTRL+C 终止程序.\n");
						printf(":");
						gets_s(&dp[0], 0);
						printf("\n\n");
					}
					break;
				case 0x0D:
					ship_support_extnpc = atoi(&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
	fclose(fp);

	if (config_index < 0x0D)
	{
		printf("ship.ini 文件貌似已损坏.\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	common_rates[0] = 100000 / WEAPON_DROP_RATE;
	common_rates[1] = 100000 / ARMOR_DROP_RATE;
	common_rates[2] = 100000 / MAG_DROP_RATE;
	common_rates[3] = 100000 / TOOL_DROP_RATE;
	common_rates[4] = 100000 / MESETA_DROP_RATE;
	load_mask_file();
}
