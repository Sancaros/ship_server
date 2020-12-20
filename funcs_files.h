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
		printf("�����ļ�������...\n��ֻʹ������...\n\n");
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
				printf("�����ļ� %u (%s:%s)\n", numLanguages, languageNames[numLanguages - 1], languageExts[numLanguages - 1]);
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
					printf("%s �Զ�����Ϣ�ļ� (%s) ������...\n��ʹ������ (Ĭ��) ��Ϣ��ʾ...\n\n", languageNames[numLanguages - 1], messageDir);
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
		printf("���������ļ� ship.ini ȱʧ��.\n");
		printf("���� [�س���] �˳�");
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
						//����������Ȼ-1,�����
						//config_data[strlen(&config_data[0]) - 1] = 0x00;
						config_data[strlen(&config_data[0])] = 0x00;
						printf("������ %s ...\n", (char*)&config_data[0]);
						IP_host = gethostbyname(&config_data[0]);
						if (!IP_host)
						{
							printf("�޷�����������.");
							printf("���� [�س���] �˳�");
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
						printf("�㲻���йܳ���10������... ���Զ�����.\n");
						serverBlocks = 10;
					}
					if (serverBlocks == 0)
					{
						printf("���������й�1������... ���Զ�����.\n");
						serverBlocks = 1;
					}
					break;
				case 0x03:
					// Max Client Connections
					serverMaxConnections = atoi(&config_data[0]);
					if (serverMaxConnections > (serverBlocks * 180))
					{
						printf("\n�����Ե������������˽��������������������.\n���Զ�����Ϊ180...\n");
						serverMaxConnections = serverBlocks * 180;
					}
					if (serverMaxConnections > SHIP_COMPILED_MAX_CONNECTIONS)
					{
						printf("�˰汾�Ĵ������������δ����Ϊ����\n����%u������.\n���Զ�����...\n", SHIP_COMPILED_MAX_CONNECTIONS);
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
						//����������Ȼ-1,�����
						//config_data[strlen(&config_data[0]) - 1] = 0x00;
						config_data[strlen(&config_data[0])] = 0x00;
						printf("������ %s ...\n", (char*)&config_data[0]);
						IP_host = gethostbyname(&config_data[0]);
						if (!IP_host)
						{
							printf("�޷�����������.");
							printf("���� [�س���] �˳�");
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
						printf("\n����: ��ľ��鱶�����÷ǳ���.\n");
						printf("���ڽ��������� �汾0.038, ������ֻ�ø�λ��\n");
						printf("��ʾ100%%����. ����.1��2\n");
						printf("��������������� %u%% ����ĳ���ָ,\n", EXPERIENCE_RATE * 100);
						printf("���� [�س���] ����, ���߰��� CTRL+C ��ֹ����.\n");
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
		printf("ship.ini �ļ�ò������.\n");
		printf("���� [�س���] �˳�");
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
