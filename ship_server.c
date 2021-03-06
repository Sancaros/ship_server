/************************************************************************
Tethealla Ship Server
Copyright (C) 2008  Terry Chatman Jr.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

//修改日期12.28

// To do: 即将要做的
//
// Firewall for Team 队伍屏蔽
//
// Challenge 挑战模式
//
// Allow quests to be reloaded while people are in them... somehow! 激活任务状态重载
/* Local To do:
*
* Allow safe exit & restart 允许安全退出和重启
* Allow simple mail/announcements from terminal 控制台的公告/邮件功能
* Automatic shutdown mail when not using quick shutdown 当没有使用快速关闭时的自动关闭邮件
未完成真正的中文汉化

挑战模式物品无法分配

挑战模式任务无法根据ep选择来进行划分

改变注释习惯，每次改动都用相同的注释进行标注以便下次查看

*/

#include	<windows.h>
#include	"stdafx.h"
#include	"DEBUG.h"
#include	<mbstring.h>
#include	<time.h>
#include	<locale.h> //12.22
#include	<math.h>

#include	"defines.h"
#include	"resource.h"
#include	"pso_crypt.h" //新增缺失的参数
#include	"bbtable.h" //新增缺失的参数
#include	"localgms.h"
#include	"prs.cpp" //新增缺失的参数 未理解
#include	"def_map.h" // Map file name definitions 地图文件
#include	"def_block.h" // Blocked packet definitions 舰仓数据
#include	"def_packets.h" // Pre-made packet definitions 预加载数据
#include	"def_structs.h" // Various structure definitions 各种结构定义
#include	"def_tables.h" // Various pre-made table definitions 各种预制表定义
#include	"functions.h"
#include	"variables.h"
#include	"ship_server.h" //12.22

const unsigned char Message03[] = { "Tethealla 舰船服务器 v.148" }; //12.22

unsigned wstrlen(unsigned short* dest)
{
	unsigned l = 0;
	while (*dest != 0x0000)
	{
		l += 2;
		dest++;
	}
	return l;
}

void wstrcpy(unsigned short* dest, const unsigned short* src)
{
	while (*src != 0x0000)
		*(dest++) = *(src++);
	*(dest++) = 0x0000;
}

void wstrcpy_char(char* dest, const char* src)
{
	while (*src != 0x00)
	{
		*(dest++) = *(src++);
		*(dest++) = 0x00;
	}
	*(dest++) = 0x00;
	*(dest++) = 0x00;
}

void packet_to_text(unsigned char* buf, int len)
{
	int c, c2, c3, c4;

	c = c2 = c3 = c4 = 0;

	for (c = 0;c < len;c++)
	{
		if (c3 == 16)
		{
			for (;c4 < c;c4++)
				if (buf[c4] >= 0x20)
					dp[c2++] = buf[c4];
				else
					dp[c2++] = 0x2E;
			c3 = 0;
			sprintf(&dp[c2++], "\n");
		}

		if ((c == 0) || !(c % 16))
		{
			sprintf(&dp[c2], "(%04X) ", c);
			c2 += 7;
		}

		sprintf(&dp[c2], "%02X ", buf[c]);
		c2 += 3;
		c3++;
	}

	if (len % 16)
	{
		c3 = len;
		while (c3 % 16)
		{
			sprintf(&dp[c2], "   ");
			c2 += 3;
			c3++;
		}
	}

	for (;c4 < c;c4++)
		if (buf[c4] >= 0x20)
			dp[c2++] = buf[c4];
		else
			dp[c2++] = 0x2E;

	dp[c2] = 0;
}

//显示数据用
void display_packet(unsigned char* buf, int len)
{
	packet_to_text(buf, len);
	printf("%s\n\n", &dp[0]);
}

void convertIPString(char* IPData, unsigned IPLen, int fromConfig, unsigned char* IPStore)
{
	unsigned p, p2, p3;
	char convert_buffer[5];

	p2 = 0;
	p3 = 0;
	for (p = 0;p < IPLen;p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p]; else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				IPStore[p2] = atoi(&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2 > 3)
				{
					if (fromConfig)
						printf("ship.ini 文件已损坏1. (无法从文件中读取IP信息!)\n"); else //12.22
						printf("无法确定IP地址.\n"); //12.22
					printf("按下 [回车键] 退出"); //12.22
					fgets(&dp[0], 0, stdin); //12.22
					exit(1);
				}
			}
			else
			{
				IPStore[p2] = atoi(&convert_buffer[0]);
				if (p2 != 3)
				{
					if (fromConfig)
						printf("ship.ini 文件已损坏2. (无法从文件中读取IP信息!)\n"); else //12.22
						printf("无法确定IP地址.\n"); //12.22
					printf("按下 [回车键] 退出"); //12.22
					gets_s(&dp[0], 0); //12.22
					exit(1);
				}
				break;
			}
		}
	}
}

void convertMask(char* IPData, unsigned IPLen, unsigned short* IPStore)
{
	unsigned p, p2, p3;
	char convert_buffer[5];

	p2 = 0;
	p3 = 0;
	for (p = 0;p < IPLen;p++)
	{
		if ((IPData[p] > 0x20) && (IPData[p] != 46))
			convert_buffer[p3++] = IPData[p]; else
		{
			convert_buffer[p3] = 0;
			if (IPData[p] == 46) // .
			{
				if (convert_buffer[0] == 42)
					IPStore[p2] = 0x8000;
				else
					IPStore[p2] = atoi(&convert_buffer[0]);
				p2++;
				p3 = 0;
				if (p2 > 3)
				{
					printf("遇到坏的掩码 masks.txt...\n"); //12.22
					memset(&IPStore[0], 0, 8);
					break;
				}
			}
			else
			{
				IPStore[p2] = atoi(&convert_buffer[0]);
				if (p2 != 3)
				{
					printf("遇到坏的掩码 in masks.txt...\n"); //12.22
					memset(&IPStore[0], 0, 8);
					break;
				}
				break;
			}
		}
	}
}

unsigned char hexToByte(char* hs)
{
	unsigned b;

	if (hs[0] < 58) b = (hs[0] - 48); else b = (hs[0] - 55);
	b *= 16;
	if (hs[1] < 58) b += (hs[1] - 48); else b += (hs[1] - 55);
	return (unsigned char)b;
}

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
	FILE* fp_messages; //12.22
	char lang_data[256];
	wchar_t message_data[256]; //12.22
	int langExt = 0;
	unsigned ch;
	unsigned cha; //12.22
	unsigned chb; //12.22

	for (ch = 0;ch < 10;ch++)
	{
		languageNames[ch] = malloc(256);
		memset(languageNames[ch], 0, 256);
		languageExts[ch] = malloc(256);
		memset(languageExts[ch], 0, 256);
		for (cha = 0;cha < 256;cha++) //12.22
		{
			languageMessages[ch][cha] = malloc(256);
			memset(languageMessages[ch][cha], 0, 256);
		}
	}

	if ((fp = fopen("lang.ini", "r")) == NULL)
	{
		printf("语言文件不存在...\n将只使用中文...\n\n"); //12.22
		numLanguages = 1;
		strcat(languageNames[0], "Chinese"); //12.22
	}
	else
	{
		//while ((fgets(&lang_data[0], 255, fp) != NULL) && (numLanguages < 10))
		while ((fgets(&lang_data[0], 255, fp) != NULL) && (numLanguages < 10))
		{
			if (!langExt)
			{
				//wmemcpy(languageNames[numLanguages], &lang_data[0], wstrlen(&lang_data[0]) + 1);
				memcpy(languageNames[numLanguages], &lang_data[0], strlen(&lang_data[0]) + 1);
				for (ch = 0;ch < strlen(languageNames[numLanguages]);ch++)
					if ((languageNames[numLanguages][ch] == 10) || (languageNames[numLanguages][ch] == 13))
						languageNames[numLanguages][ch] = 0;
				langExt = 1;
			}
			else
			{
				memcpy(languageExts[numLanguages], &lang_data[0], strlen(&lang_data[0]) + 1);
				for (ch = 0;ch < strlen(languageExts[numLanguages]);ch++)
					if ((languageExts[numLanguages][ch] == 10) || (languageExts[numLanguages][ch] == 13))
						languageExts[numLanguages][ch] = 0;
				numLanguages++;
				printf("语言文件 %u (%s:%s)\n", numLanguages, languageNames[numLanguages - 1], languageExts[numLanguages - 1]);
				langExt = 0;

				//custom lang messages //12.22
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

						for (chb = 0;chb < wcslen(languageMessages[numLanguages - 1][count]);chb++)
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
			strcat(languageNames[0], "Chinese"); //12.22
		}
	}
}

void load_config_file()
{
	int config_index = 0;
	char config_data[255];
	unsigned ch = 0;

	unsigned mob_rate;
	long long mob_calc;

	FILE* fp;

	EXPERIENCE_RATE = 1; // Default to 100% EXP

	if ((fp = fopen("ship.ini", "r")) == NULL)
	{
		printf("舰船设置文件 ship.ini 缺失了.\n"); //12.22
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	else
		while (fgets(&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)  // if not a comment
			{
				// If IP settings, IP Address, or Ship Name
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					// Remove newlines and carriage returns
					ch = strlen(&config_data[0]);
					if (config_data[ch - 1] == 0x0A)
						config_data[ch--] = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index) // Parse the lines
				{
				case 0x00:
					// Login server host name or IP
				{
					unsigned p; //12.22
					unsigned alpha;
					alpha = 0;
					for (p = 0;p < ch;p++)
						if (((config_data[p] >= 65) && (config_data[p] <= 90)) ||
							((config_data[p] >= 97) && (config_data[p] <= 122)))
						{
							alpha = 1;
							break;
						}
					if (alpha)
					{
						struct hostent* IP_host;
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
						*(unsigned*)&serverIP[0] = *(unsigned*)IP_host->h_addr;
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
					for (p = 0;p < ch;p++)
						if (((config_data[p] >= 65) && (config_data[p] <= 90)) ||
							((config_data[p] >= 97) && (config_data[p] <= 122)))
						{
							alpha = 1;
							break;
						}
					if (alpha)
					{
						struct hostent* IP_host;
						//这里域名竟然-1,待解决
						//config_data[strlen(&config_data[0]) - 1] = 0x00;
						config_data[strlen(&config_data[0])] = 0x00; //貌似这样就解决了
						printf("解析中 %s ...\n", (char*)&config_data[0]);
						IP_host = gethostbyname(&config_data[0]);
						if (!IP_host)
						{
							printf("无法解析该域名.");
							printf("按下 [回车键] 退出");
							gets_s(&dp[0], 0);
							exit(1);
						}
						*(unsigned*)&loginIP[0] = *(unsigned*)IP_host->h_addr;
					}
					else
						convertIPString(&config_data[0], ch + 1, 1, &loginIP[0]);
				}
				break;
				case 0x05:
					// 舰船名称 sancaros 汉化失败
					//memset (&Ship_Name[0], 0, 255 );
					//memcpy (&Ship_Name[0], &config_data[0], ch+1 );
					memset(&Ship_Name[0], 0, 255);
					memcpy(&Ship_Name[0], (unsigned char*)&config_data[0], ch + 1); //发送至舰船列表
					Ship_Name[12] = 0x00; //发送至登录服务器
#ifdef DEBUG
					printf("\n控制台中舰船名称: %Ls\n", &Ship_Name[0]);
					printf("\n设置中舰船名称: %s\n", &config_data[0]);
					printf("\n设置中舰船名称2: %Ls\n", (wchar_t*)(&config_data[0]));
					printf("\n设置中舰船名称3: %s\n", (char*)(&config_data[0]));
					//printf("\nUTA控制台舰船名称: %s\n", ASCII_to_Unicode((unsigned short*)&Ship_Name[0]));
					printf("\nUTA设置舰船名称: %s\n", Unicode_to_ASCII((unsigned short*)&config_data[0]));
					/*printf("\n舰船名称: %s\n", (wchar_t*)(&Ship_Name[0]));
					printf("\n舰船名称: %s\n", &config_data[0]);
					printf("\n舰船名称: %s\n", (wchar_t*)(&config_data[0]));*/
#endif // DEBUG
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
				case 0x0C:  // Rare box drop multiplier
					rare_box_mult = atoi(config_data);
					//printf(" Rare box drop multiplier set to %d\n", rare_box_mult);
					break;
				case 0x0D:  // Rare mob drop multiplier
					rare_mob_drop_mult = atoi(config_data);
					//printf(" Rare mob drop multiplier set to %d\n", rare_mob_drop_mult);
					break;
				case 0x0E:  // Global rare drop multiplier
					global_rare_mult = atoi(config_data);
					//printf(" Global rare drop multiplier set to %d\n", global_rare_mult);
					break;
					// Rare mob rates
				case 0x0F:
					// Multiplier for rare mob appearances
					rare_mob_mult = atoi(config_data);
					//printf(" Global rare mob multiplier set to %d\n", rare_mob_mult);
					break;
				case 0x10:
					// Hildebear rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Hildebear appearance rate: %3f%%\n", (float)mob_rate / 1000);
					hildebear_rate = (unsigned)mob_calc;
					break;
				case 0x11:
					// Rappy rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Rappy appearance rate: %3f%%\n", (float)mob_rate / 1000);
					rappy_rate = (unsigned)mob_calc;
					break;
				case 0x12:
					// Lily rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Lily appearance rate: %3f%%\n", (float)mob_rate / 1000);
					lily_rate = (unsigned)mob_calc;
					break;
				case 0x13:
					// Slime rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Pouilly Slime appearance rate: %3f%%\n", (float)mob_rate / 1000);
					slime_rate = (unsigned)mob_calc;
					break;
				case 0x14:
					// Merissa rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Merissa AA appearance rate: %3f%%\n", (float)mob_rate / 1000);
					merissa_rate = (unsigned)mob_calc;
					break;
				case 0x15:
					// Pazuzu rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Pazuzu appearance rate: %3f%%\n", (float)mob_rate / 1000);
					pazuzu_rate = (unsigned)mob_calc;
					break;
				case 0x16:
					// Dorphon Eclair rate
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Dorphon Eclair appearance rate: %3f%%\n", (float)mob_rate / 1000);
					dorphon_rate = (unsigned)mob_calc;
					break;
				case 0x17:
					// Kondrieu rate (last mob rate)
					mob_rate = rare_mob_mult * atoi(&config_data[0]);
					mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
					//printf(" Kondrieu appearance rate: %3f%%\n", (float)mob_rate / 1000);
					kondrieu_rate = (unsigned)mob_calc;
					break;
				case 0x18:
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
				case 0x19:  // NiGHTS skin support
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

ORANGE logon_structure;
BANANA* connections[SHIP_COMPILED_MAX_CONNECTIONS];
ORANGE* logon_connecion;
BANANA* workConnect;
ORANGE* logon;
unsigned logon_tick = 0;
unsigned logon_ready = 0;

//中文汉化
//const char serverName[] = { "T\0E\0T\0H\0E\0A\0L\0L\0A\0" };
//const char shipSelectString[] = {"S\0h\0i\0p\0 \0S\0e\0l\0e\0c\0t\0"};
//const char blockString[] = {"B\0L\0O\0C\0K\0"};
const wchar_t serverName[6] = { L"母舰 \0" }; //sancaros 汉化有问题
const wchar_t shipSelectString[8] = { L"港    口\0" }; //sancaros 汉化有问题
const wchar_t blockString[8] = { L"选择舰仓 \0" }; //sancaros 汉化有问题

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
		for (ch = 16;ch < (16 + SHIP_COMPILED_MAX_GAMES);ch++)
		{
			l = &b->lobbies[ch];
			if (l->in_use)
			{
				memset(&PacketData[Offset], 0, 44);
				// Output game 输出游戏房间
				Offset += 2;
				PacketData[Offset] = 0x03;
				Offset += 2;
				*(unsigned*)&PacketData[Offset] = ch;
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
					// Grey out Government quests that the player is not qualified for...不符合资格的玩家显示灰色的政府任务 sancaros
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
						qNum -= 451; //qNum = qNum - 451
						qNum <<= 1; //qNum = qNum 左移 1位 后赋值 十进制
						qNum += 0x211; //qNum = qNum + 0x211     (int(529))
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
					//printf("12222222222222222222222222222222222222");
					//如果到了这个地方，会导致进入空房间无法退出					
					game_flags = 0x40;
				// Get flags for battle and one person games...sancaros 设置对战模式和单人游戏 
				if ((l->gamePassword[0x00] != 0x00) ||
					(l->gamePassword[0x01] != 0x00))
					game_flags |= 0x02;
				if ((l->quest_in_progress) || (l->oneperson)) // Can't join! -无法加入游戏的房间
					game_flags |= 0x04;
				if (l->battle)
					game_flags |= 0x10;
				if (l->challenge)
					game_flags |= 0x20;
				// Wonder what flags 0x01 and 0x08 control.... 想知道什么标志被0x01和0x08控制 sancaros
				PacketData[Offset++] = game_flags;
				total_games++;
			}
		}
		//debug("这里用于显示房间列表时触发这段代码");
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
	//Sancaros Send debug的方式
#ifdef DEBUG
	unsigned short size;
	size = *(unsigned short*)&client->decryptbuf[0x00];
	WriteLog("Send08 指令 \"%02x\" 数据记录如下. (数据如下)", client->decryptbuf[0x08]);
	packet_to_text(&client->decryptbuf[0x00], size);
	WriteLog("\n %s", &dp[0]);
#endif // DEBUG

}

//舰船数据包
void ConstructBlockPacket()
{
	unsigned short Offset;
	unsigned ch;
	char tempName[255];
	char* tn;
	unsigned BlockID;

	memset(&Packet07Data[0], 0, 0x4000);

	Packet07Data[0x02] = 0x07;
	Packet07Data[0x04] = serverBlocks + 1;
	_itoa(serverID, &tempName[0], 10); //这里涉及大厅右上角和设置界面右下角的名称 中文汉化
	if (serverID < 10)
	{
		tempName[0] = 0x30;
		tempName[1] = 0x30 + serverID;
		tempName[2] = 0x00;
	}
	else
		_itoa(serverID, &tempName[0], 10);

	strcat(&tempName[0], ":");
	strcat(&tempName[0], &Ship_Name[0]);

	Packet07Data[0x32] = 0x08;
	Offset = 0x12;
	tn = &tempName[0];
	while (*tn != 0x00)
	{
		Packet07Data[Offset++] = *(tn++);
		Packet07Data[Offset++] = 0x00;
	}
	Offset = 0x36;
	for (ch = 0;ch < serverBlocks;ch++)
	{
		Packet07Data[Offset] = 0x12;
		BlockID = 0xEFFFFFFF - ch;
		*(unsigned*)&Packet07Data[Offset + 2] = BlockID;
		memcpy(&Packet07Data[Offset + 0x08], &blockString[0], sizeof(blockString));
		if (ch + 1 < 10)
		{
			Packet07Data[Offset + 0x12] = 0x30;
			Packet07Data[Offset + 0x14] = 0x30 + (ch + 1);
		}
		else
		{
			Packet07Data[Offset + 0x12] = 0x31;
			Packet07Data[Offset + 0x14] = 0x30;
		}
		Offset += 0x2C;
	}
	Packet07Data[Offset] = 0x12;
	BlockID = 0xFFFFFF00;
	*(unsigned*)&Packet07Data[Offset + 2] = BlockID;
	memcpy(&Packet07Data[Offset + 0x08], &shipSelectString[0], sizeof(shipSelectString));
	Offset += 0x2C;
	while (Offset % 8)
		Packet07Data[Offset++] = 0x00;
	*(unsigned short*)&Packet07Data[0x00] = (unsigned short)Offset;
	Packet07Size = Offset;
}

//日志代码
void initialize_logon()
{
	unsigned ch;

	logon_ready = 0;
	logon_tick = 0;
	logon = &logon_structure;
	if (logon->sockfd >= 0)
		closesocket(logon->sockfd);
	memset(logon, 0, sizeof(ORANGE));
	logon->sockfd = -1;
	for (ch = 0;ch < 128;ch++)
		logon->key_change[ch] = -1;
	*(unsigned*)&logon->_ip.s_addr = *(unsigned*)&loginIP[0];
}

//日志代码
void reconnect_logon()
{
	// Just in case this is called because of an error in communication with the logon server.
	logon->sockfd = tcp_sock_connect(inet_ntoa(logon->_ip), 3455);
	if (logon->sockfd >= 0)
	{
		printf("舰船成功连接至登陆服务器!\n");
		logon->last_ping = (unsigned)time(NULL);
	}
	else
	{
		printf("连接失败.将在 %u 秒后重试...\n", LOGIN_RECONNECT_SECONDS);
		logon_tick = 0;
	}
}

//数据连接
unsigned free_connection()
{
	unsigned fc;
	BANANA* wc;

	for (fc = 0;fc < serverMaxConnections;fc++)
	{
		wc = connections[fc];
		if (wc->plySockfd < 0)
			return fc;
	}
	return 0xFFFF;
}

//加载连接代码
void initialize_connection(BANANA* connect)
{
	unsigned ch, ch2;
	// Free backup character memory 释放备份角色内存


	if (connect->character_backup)
	{
		if (connect->mode && connect->guildcard) {

			//ShipSend04(0x03, connect, logon);
			//ShipSend04(0x04, connect, logon);
			memcpy(&connect->character, connect->character_backup, sizeof(connect->character));
			memcpy(&connect->character.challengeData, connect->challenge_data.challengeData, 320);
			memcpy(&connect->character.battleData, connect->battle_data.battleData, 92);
			//WriteLog("测试在这里获取更好的数据");
		}

		free(connect->character_backup);
		connect->character_backup = NULL;
	}
	// OK
	//WriteLog("debug客户端断点1");
	if (connect->guildcard)
	{
		removeClientFromLobby(connect);

		if ((connect->block) && (connect->block <= serverBlocks))
		{
			blocks[connect->block - 1]->count--;
		}

		if (connect->gotchardata == 1)
		{
			connect->character.playTime += (unsigned)servertime - connect->connected;
			ShipSend04(0x02, connect, logon);
			//ShipSend04(0x03, connect, logon);
			//ShipSend04(0x04, connect, logon);
		}
	}

	// OK
	//WriteLog("debug客户端断点2");
	if (connect->plySockfd >= 0)
	{
		ch2 = 0;
		for (ch = 0;ch < serverNumConnections;ch++)
		{
			if (serverConnectionList[ch] != connect->connection_index)
				serverConnectionList[ch2++] = serverConnectionList[ch];
		}
		serverNumConnections = ch2;
		closesocket(connect->plySockfd);
	}
	// OK
	//WriteLog("debug客户端断点3");

	if (logon_ready)
	{
		printf("玩家数量: %u\n", serverNumConnections);
		ShipSend0E(logon);
	}
	// OK
	//WriteLog("debug客户端断点4");

	memset(connect, 0, sizeof(BANANA));
	connect->plySockfd = -1;
	connect->block = -1;
	connect->lastTick = 0xFFFFFFFF;
	connect->slotnum = -1;
	connect->sending_quest = -1;
	// OK
	//WriteLog("debug客户端断点5");
}

//开始解包数据代码
void start_encryption(BANANA* connect)
{
	unsigned c, c3, c4, connectNum;
	BANANA* workConnect, * c5;

	// Limit the number of connections from an IP address to MAX_SIMULTANEOUS_CONNECTIONS.

	c3 = 0;

	for (c = 0;c < serverNumConnections;c++)
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
		for (c = 0;c < serverNumConnections;c++)
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
			//WriteLog("debug客户端断点6");
		}
	}

	memcpy(&connect->sndbuf[0], &Packet03[0], sizeof(Packet03));
	for (c = 0;c < 0x30;c++)
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

//发送至大厅数据代码
void SendToLobby(LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend)
{
	unsigned ch;

	if (!l)
		return;

	for (ch = 0;ch < max_send;ch++)
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

	for (ch = 0;ch < maxch;ch++)
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
				for (ch = 0;ch < 4;ch++)
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

//void Send1A (const char *mes, BANANA* client)
//常用文本代码 需要汉化支持宽字符 未完成
void Send1A(const wchar_t* mes, BANANA* client, int line)
{
	setlocale(LC_ALL, "chs");
	if (line > -1 && wstrlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	} //中文汉化失败
	unsigned short x1A_Len;

	memcpy(&PacketData[0], &Packet1A[0], sizeof(Packet1A));
	x1A_Len = sizeof(Packet1A);

	while (*mes != 0x00)
	{
		PacketData[x1A_Len++] = (unsigned char)*(mes++);
		PacketData[x1A_Len++] = 0x00;
	}

	PacketData[x1A_Len++] = 0x00;
	PacketData[x1A_Len++] = 0x00;

	while (x1A_Len % 8)
		PacketData[x1A_Len++] = 0x00;

	*(unsigned short*)&PacketData[0] = x1A_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], x1A_Len);
	//WriteLog("Send1A发送至客户端如下信息\n %d", mes);
}

//备份角色数据 客户端超过1分钟没有回应,断开连接
void Send1D(BANANA* client)
{
	unsigned num_minutes;

	if ((((unsigned)servertime - client->savetime) / 60L) >= 5)
	{
		// Backup character data every 5 minutes.每5分钟备份一次角色数据至数据库 已改为每分钟备份1次角色数据至数据库
		client->savetime = (unsigned)servertime;
		ShipSend04(0x02, client, logon);
		//ShipSend04(0x03, client, logon);//30挑战 必须要循环的检测模式中才会保存数据 千万别忘了
		//ShipSend04(0x04, client, logon);//30对战
	}

	num_minutes = ((unsigned)servertime - client->response) / 60L;
	if (num_minutes)
	{
		if (num_minutes > 2)
			initialize_connection(client); // If the client hasn't responded in over two minutes, drop the connection.如果客户端没有回应,则断开连接
										   //WriteLog("debug客户端断点7");
		else
		{
			cipher_ptr = &client->server_cipher;
			encryptcopy(client, &Packet1D[0], sizeof(Packet1D));
		}
	}
	//sancaros Send debug的方式
#ifdef DEBUG
	unsigned short size;
	size = *(unsigned short*)&client->decryptbuf[0x00];
	WriteLog("Send1D 指令 \"%02x\" 数据记录如下. (数据如下)", client->decryptbuf[0x1D]);
	packet_to_text(&client->decryptbuf[0x00], size);
	WriteLog("\n %s", &dp[0]);
#endif // DEBUG

}

//83数据包
void Send83(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &Packet83[0], sizeof(Packet83));
	//WriteLog("Send83发送如下信息\n %s", client); //sancaros Send debug的方式

}

//释放游戏
unsigned free_game(BANANA* client)
{
	unsigned ch;
	LOBBY* l;

	for (ch = 16;ch < (16 + SHIP_COMPILED_MAX_GAMES);ch++)
	{
		l = &blocks[client->block - 1]->lobbies[ch];
		if (l->in_use == 0)
			return ch;
	}
	return 0;
}

//配置稀有怪物的出现概率
void ParseMapData(LOBBY* l, MAP_MONSTER* mapData, int aMob, unsigned num_records)
{
	MAP_MONSTER* mm;
	unsigned ch, ch2;
	unsigned num_recons;
	int r;

	for (ch2 = 0;ch2 < num_records;ch2++)
	{
		if (l->mapIndex >= 0xB50)
			break;
		memcpy(&l->mapData[l->mapIndex], mapData, 72);
		mapData++;
		mm = &l->mapData[l->mapIndex];
		mm->exp = 0;
		switch (mm->base)
		{
		case 64:
			// Hildebear and Hildetorr
			r = 0;
			if (mm->skin & 0x01) // Set rare from a quest?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < hildebear_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
			{
				mm->rt_index = 0x02;
				mm->exp = l->bptable[0x4A].XP;
			}
			else
			{
				mm->rt_index = 0x01;
				mm->exp = l->bptable[0x49].XP;
			}
			break;
		case 65:
			// Rappies
			r = 0;
			if (mm->skin & 0x01) // Set rare from a quest?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < rappy_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (l->episode == 0x03)
			{
				// Del Rappy and Sand Rappy
				if (aMob)
				{
					if (r)
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x18].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x17].XP;
					}
				}
				else
				{
					if (r)
					{
						mm->rt_index = 18;
						mm->exp = l->bptable[0x06].XP;
					}
					else
					{
						mm->rt_index = 17;
						mm->exp = l->bptable[0x05].XP;
					}
				}
			}
			else
			{
				// Rag Rappy, Al Rappy, Love Rappy and Seasonal Rappies
				if (r)
				{
					if (l->episode == 0x01)
						mm->rt_index = 6; // Al Rappy
					else
					{
						switch (shipEvent)
						{
						case 0x01:
							mm->rt_index = 79; // St. Rappy
							break;
						case 0x04:
							mm->rt_index = 81; // Easter Rappy
							break;
						case 0x05:
							mm->rt_index = 80; // Halo Rappy
							break;
						default:
							mm->rt_index = 51; // Love Rappy
							break;
						}
					}
					mm->exp = l->bptable[0x19].XP;
				}
				else
				{
					mm->rt_index = 5;
					mm->exp = l->bptable[0x18].XP;
				}
			}
			break;
		case 66:
			// Monest + 30 Mothmants
			mm->exp = l->bptable[0x01].XP;
			mm->rt_index = 4;

			for (ch = 0;ch < 30;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 3;
				mm->exp = l->bptable[0x00].XP;
			}
			break;
		case 67:
			// Savage Wolf and Barbarous Wolf
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
			{
				mm->rt_index = 8;
				mm->exp = l->bptable[0x03].XP;
			}
			else
			{
				mm->rt_index = 7;
				mm->exp = l->bptable[0x02].XP;
			}
			break;
		case 68:
			// Booma family
			if (mm->skin & 0x02)
			{
				mm->rt_index = 11;
				mm->exp = l->bptable[0x4D].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					mm->rt_index = 10;
					mm->exp = l->bptable[0x4C].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable[0x4B].XP;
				}
			break;
		case 96:
			// Grass Assassin
			mm->rt_index = 12;
			mm->exp = l->bptable[0x4E].XP;
			break;
		case 97:
			// Del Lily, Poison Lily, Nar Lily
			r = 0;
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < lily_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if ((l->episode == 0x02) && (aMob))
			{
				mm->rt_index = 83;
				mm->exp = l->bptable[0x25].XP;
			}
			else
				if (r)
				{
					mm->rt_index = 14;
					mm->exp = l->bptable[0x05].XP;
				}
				else
				{
					mm->rt_index = 13;
					mm->exp = l->bptable[0x04].XP;
				}
			break;
		case 98:
			// Nano Dragon
			mm->rt_index = 15;
			mm->exp = l->bptable[0x1A].XP;
			break;
		case 99:
			// Shark family
			if (mm->skin & 0x02)
			{
				mm->rt_index = 18;
				mm->exp = l->bptable[0x51].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					mm->rt_index = 17;
					mm->exp = l->bptable[0x50].XP;
				}
				else
				{
					mm->rt_index = 16;
					mm->exp = l->bptable[0x4F].XP;
				}
			break;
		case 100:
			// Slime + 4 clones
			r = 0;
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < slime_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
			{
				mm->rt_index = 20;
				mm->exp = l->bptable[0x2F].XP;
			}
			else
			{
				mm->rt_index = 19;
				mm->exp = l->bptable[0x30].XP;
			}
			for (ch = 0;ch < 4;ch++)
			{
				l->mapIndex++;
				mm++;
				r = 0;
				if ((l->rareIndex < 0x1E) && (mt_lrand() < slime_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
				if (r)
				{
					mm->rt_index = 20;
					mm->exp = l->bptable[0x2F].XP;
				}
				else
				{
					mm->rt_index = 19;
					mm->exp = l->bptable[0x30].XP;
				}
			}
			break;
		case 101:
			// Pan Arms, Migium, Hidoom
			mm->rt_index = 21;
			mm->exp = l->bptable[0x31].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 22;
			mm->exp = l->bptable[0x32].XP;
			l->mapIndex++;
			mm++;
			mm->rt_index = 23;
			mm->exp = l->bptable[0x33].XP;
			break;
		case 128:
			// Dubchic and Gilchic
			if (mm->skin & 0x01)
			{
				mm->exp = l->bptable[0x1C].XP;
				mm->rt_index = 50;
			}
			else
			{
				mm->exp = l->bptable[0x1B].XP;
				mm->rt_index = 24;
			}
			break;
		case 129:
			// Garanz
			mm->rt_index = 25;
			mm->exp = l->bptable[0x1D].XP;
			break;
		case 130:
			// Sinow Beat and Gold
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
			{
				mm->rt_index = 27;
				mm->exp = l->bptable[0x13].XP;
			}
			else
			{
				mm->rt_index = 26;
				mm->exp = l->bptable[0x06].XP;
			}

			if ((mm->reserved[0] >> 16) == 0)
				l->mapIndex += 4; // Add 4 clones but only if there's no add value there already...
			break;
		case 131:
			// Canadine
			mm->rt_index = 28;
			mm->exp = l->bptable[0x07].XP;
			break;
		case 132:
			// Canadine Group
			mm->rt_index = 29;
			mm->exp = l->bptable[0x09].XP;
			for (ch = 0;ch < 8;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 28;
				mm->exp = l->bptable[0x08].XP;
			}
			break;
		case 133:
			// Dubwitch
			break;
		case 160:
			// Delsaber
			mm->rt_index = 30;
			mm->exp = l->bptable[0x52].XP;
			break;
		case 161:
			// Chaos Sorcerer + 2 Bits
			mm->rt_index = 31;
			mm->exp = l->bptable[0x0A].XP;
			l->mapIndex += 2;
			break;
		case 162:
			// Dark Gunner
			mm->rt_index = 34;
			mm->exp = l->bptable[0x1E].XP;
			break;
		case 164:
			// Chaos Bringer
			mm->rt_index = 36;
			mm->exp = l->bptable[0x0D].XP;
			break;
		case 165:
			// Dark Belra
			mm->rt_index = 37;
			mm->exp = l->bptable[0x0E].XP;
			break;
		case 166:
			// Dimenian family
			if (mm->skin & 0x02)
			{
				mm->rt_index = 43;
				mm->exp = l->bptable[0x55].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					mm->rt_index = 42;
					mm->exp = l->bptable[0x54].XP;
				}
				else
				{
					mm->rt_index = 41;
					mm->exp = l->bptable[0x53].XP;
				}
			break;
		case 167:
			// Bulclaw + 4 claws
			mm->rt_index = 40;
			mm->exp = l->bptable[0x1F].XP;
			for (ch = 0;ch < 4;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->rt_index = 38;
				mm->exp = l->bptable[0x20].XP;
			}
			break;
		case 168:
			// Claw
			mm->rt_index = 38;
			mm->exp = l->bptable[0x20].XP;
			break;
		case 192:
			// Dragon or Gal Gryphon
			if (l->episode == 0x01)
			{
				mm->rt_index = 44;
				mm->exp = l->bptable[0x12].XP;
			}
			else
				if (l->episode == 0x02)
				{
					mm->rt_index = 77;
					mm->exp = l->bptable[0x1E].XP;
				}
			break;
		case 193:
			// De Rol Le
			mm->rt_index = 45;
			mm->exp = l->bptable[0x0F].XP;
			break;
		case 194:
			// Vol Opt form 1
			break;
		case 197:
			// Vol Opt form 2
			mm->rt_index = 46;
			mm->exp = l->bptable[0x25].XP;
			break;
		case 200:
			// Dark Falz + 510 Helpers
			mm->rt_index = 47;
			if (l->difficulty)
				mm->exp = l->bptable[0x38].XP; // Form 2
			else
				mm->exp = l->bptable[0x37].XP;

			for (ch = 0;ch < 510;ch++)
			{
				l->mapIndex++;
				mm++;
				mm->base = 200;
				mm->exp = l->bptable[0x35].XP;
			}
			break;
		case 202:
			// Olga Flow
			mm->rt_index = 78;
			mm->exp = l->bptable[0x2C].XP;
			l->mapIndex += 512;
			break;
		case 203:
			// Barba Ray
			mm->rt_index = 73;
			mm->exp = l->bptable[0x0F].XP;
			l->mapIndex += 47;
			break;
		case 204:
			// Gol Dragon
			mm->rt_index = 76;
			mm->exp = l->bptable[0x12].XP;
			l->mapIndex += 5;
			break;
		case 212:
			// Sinow Berill & Spigell
			/* if ( ( ( mm->reserved11 - FLOAT_PRECISION ) < (float) 1.00000 ) &&
			( ( mm->reserved11 + FLOAT_PRECISION ) > (float) 1.00000 ) ) */
			if (mm->skin >= 0x01) // set rare?
			{
				mm->rt_index = 63;
				mm->exp = l->bptable[0x13].XP;
			}
			else
			{
				mm->rt_index = 62;
				mm->exp = l->bptable[0x06].XP;
			}
			l->mapIndex += 4; // Add 4 clones which are never used...
			break;
		case 213:
			// Merillia & Meriltas
			if (mm->skin & 0x01)
			{
				mm->rt_index = 53;
				mm->exp = l->bptable[0x4C].XP;
			}
			else
			{
				mm->rt_index = 52;
				mm->exp = l->bptable[0x4B].XP;
			}
			break;
		case 214:
			if (mm->skin & 0x02)
			{
				// Mericus
				mm->rt_index = 58;
				mm->exp = l->bptable[0x46].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					// Merikle
					mm->rt_index = 57;
					mm->exp = l->bptable[0x45].XP;
				}
				else
				{
					// Mericarol
					mm->rt_index = 56;
					mm->exp = l->bptable[0x3A].XP;
				}
			break;
		case 215:
			// Ul Gibbon and Zol Gibbon
			if (mm->skin & 0x01)
			{
				mm->rt_index = 60;
				mm->exp = l->bptable[0x3C].XP;
			}
			else
			{
				mm->rt_index = 59;
				mm->exp = l->bptable[0x3B].XP;
			}
			break;
		case 216:
			// Gibbles
			mm->rt_index = 61;
			mm->exp = l->bptable[0x3D].XP;
			break;
		case 217:
			// Gee
			mm->rt_index = 54;
			mm->exp = l->bptable[0x07].XP;
			break;
		case 218:
			// Gi Gue
			mm->rt_index = 55;
			mm->exp = l->bptable[0x1A].XP;
			break;
		case 219:
			// Deldepth
			mm->rt_index = 71;
			mm->exp = l->bptable[0x30].XP;
			break;
		case 220:
			// Delbiter
			mm->rt_index = 72;
			mm->exp = l->bptable[0x0D].XP;
			break;
		case 221:
			// Dolmolm and Dolmdarl
			if (mm->skin & 0x01)
			{
				mm->rt_index = 65;
				mm->exp = l->bptable[0x50].XP;
			}
			else
			{
				mm->rt_index = 64;
				mm->exp = l->bptable[0x4F].XP;
			}
			break;
		case 222:
			// Morfos
			mm->rt_index = 66;
			mm->exp = l->bptable[0x40].XP;
			break;
		case 223:
			// Recobox & Recons
			mm->rt_index = 67;
			mm->exp = l->bptable[0x41].XP;
			num_recons = (mm->reserved[0] >> 16);
			for (ch = 0;ch < num_recons;ch++)
			{
				if (l->mapIndex >= 0xB50)
					break;
				l->mapIndex++;
				mm++;
				mm->rt_index = 68;
				mm->exp = l->bptable[0x42].XP;
			}
			break;
		case 224:
			if ((l->episode == 0x02) && (aMob))
			{
				// Epsilon
				mm->rt_index = 84;
				mm->exp = l->bptable[0x23].XP;
				l->mapIndex += 4;
			}
			else
			{
				// Sinow Zoa and Zele
				if (mm->skin & 0x01)
				{
					mm->rt_index = 70;
					mm->exp = l->bptable[0x44].XP;
				}
				else
				{
					mm->rt_index = 69;
					mm->exp = l->bptable[0x43].XP;
				}
			}
			break;
		case 225:
			// Ill Gill
			mm->rt_index = 82;
			mm->exp = l->bptable[0x26].XP;
			break;
		case 272:
			// Astark
			mm->rt_index = 1;
			mm->exp = l->bptable[0x09].XP;
			break;
		case 273:
			// Satellite Lizard and Yowie
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
			{
				if (aMob)
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x1E].XP;
				}
				else
				{
					mm->rt_index = 2;
					mm->exp = l->bptable[0x0E].XP;
				}
			}
			else
			{
				if (aMob)
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x1D].XP;
				}
				else
				{
					mm->rt_index = 3;
					mm->exp = l->bptable[0x0D].XP;
				}
			}
			break;
		case 274:
			// Merissa A/AA
			r = 0;
			if (mm->skin & 0x01) // Set rare from a quest?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < merissa_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
			{
				mm->rt_index = 5;
				mm->exp = l->bptable[0x1A].XP;
			}
			else
			{
				mm->rt_index = 4;
				mm->exp = l->bptable[0x19].XP;
			}
			break;
		case 275:
			// Girtablulu
			mm->rt_index = 6;
			mm->exp = l->bptable[0x1F].XP;
			break;
		case 276:
			// Zu and Pazuzu
			r = 0;
			if (mm->skin & 0x01) // Set rare from a quest?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < pazuzu_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
			{
				if (aMob)
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x1C].XP;
				}
				else
				{
					mm->rt_index = 8;
					mm->exp = l->bptable[0x08].XP;
				}
			}
			else
			{
				if (aMob)
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x1B].XP;
				}
				else
				{
					mm->rt_index = 7;
					mm->exp = l->bptable[0x07].XP;
				}
			}
			break;
		case 277:
			// Boota family
			if (mm->skin & 0x02)
			{
				mm->rt_index = 11;
				mm->exp = l->bptable[0x03].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					mm->rt_index = 10;
					mm->exp = l->bptable[0x01].XP;
				}
				else
				{
					mm->rt_index = 9;
					mm->exp = l->bptable[0x00].XP;
				}
			break;
		case 278:
			// Dorphon and Eclair
			r = 0;
			if (mm->skin & 0x01) // Set rare from a quest?
				r = 1;
			else
				if ((l->rareIndex < 0x1E) && (mt_lrand() < dorphon_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
			{
				mm->rt_index = 13;
				mm->exp = l->bptable[0x10].XP;
			}
			else
			{
				mm->rt_index = 12;
				mm->exp = l->bptable[0x0F].XP;
			}
			break;
		case 279:
			// Goran family
			if (mm->skin & 0x02)
			{
				mm->rt_index = 15;
				mm->exp = l->bptable[0x13].XP;
			}
			else
				if (mm->skin & 0x01)
				{
					mm->rt_index = 16;
					mm->exp = l->bptable[0x12].XP;
				}
				else
				{
					mm->rt_index = 14;
					mm->exp = l->bptable[0x11].XP;
				}
			break;
		case 281:
			// Saint Million, Shambertin, and Kondrieu
			r = 0;
			if (((mm->reserved11 - FLOAT_PRECISION) < (float)1.00000) &&
				((mm->reserved11 + FLOAT_PRECISION) > (float)1.00000)) // set rare?
				r = 1;
			else
				if ((l->rareIndex < 0x20) && (mt_lrand() < kondrieu_rate))
				{
					*(unsigned short*)&l->rareData[l->rareIndex] = (unsigned short)l->mapIndex;
					l->rareIndex += 2;
					r = 1;
				}
			if (r)
				mm->rt_index = 21;
			else
			{
				if (mm->skin & 0x01)
					mm->rt_index = 20;
				else
					mm->rt_index = 19;
			}
			mm->exp = l->bptable[0x22].XP;
			break;
		default:
			//debug ("enemy not handled: %u", mm->base);
			break;
		}
		if (mm->reserved[0] >> 16) // Have to do
			l->mapIndex += (mm->reserved[0] >> 16);
		l->mapIndex++;
	}
}

//对象数据有缺失
void LoadObjectData(LOBBY* l, int unused, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records, ch, ch2;
	char new_file[256];

	if (!l)
		return;

	memcpy(&new_file[0], filename, strlen(filename) + 1);

	if (filename[strlen(filename) - 5] == 101)
		new_file[strlen(filename) - 5] = 111; // change e to o

											  //debug ("Loading object %s... current index: %u", new_file, l->objIndex);

	fp = fopen(&new_file[0], "rb");
	if (!fp)
		WriteLog("无法从 %s 加载对象数据\n", new_file);
	else
	{
		fseek(fp, 0, SEEK_END);
		num_records = ftell(fp) / 68;
		fseek(fp, 0, SEEK_SET);
		fread(&dp[0], 1, 68 * num_records, fp);
		fclose(fp);
		oldIndex = l->objIndex;
		ch2 = 0;
		for (ch = 0;ch < num_records;ch++)
		{
			if (l->objIndex < 0xB50)
			{
				memcpy(&l->objData[l->objIndex], &dp[ch2 + 0x28], 12);
				l->objData[l->objIndex].drop[3] = 0;
				l->objData[l->objIndex].drop[2] = dp[ch2 + 0x35];
				l->objData[l->objIndex].drop[1] = dp[ch2 + 0x36];
				l->objData[l->objIndex++].drop[0] = dp[ch2 + 0x37];
				ch2 += 68;
			}
			else
				break;
		}
		//debug ("Added %u objects, total: %u", l->objIndex - oldIndex, l->objIndex );
	}
};

//载入地图数据
void LoadMapData(LOBBY* l, int aMob, const char* filename)
{
	FILE* fp;
	unsigned oldIndex, num_records;

	if (!l)
		return;

	//debug ("Loading map %s... current index: %u", filename, l->mapIndex);

	fp = fopen(filename, "rb");
	if (!fp)
		WriteLog("无法从 %s 加载地图数据\n", filename);
	else
	{
		fseek(fp, 0, SEEK_END);
		num_records = ftell(fp) / 72;
		fseek(fp, 0, SEEK_SET);
		fread(&dp[0], 1, sizeof(MAP_MONSTER) * num_records, fp);
		fclose(fp);
		oldIndex = l->mapIndex;
		ParseMapData(l, (MAP_MONSTER*)&dp[0], aMob, num_records);
		//debug ("Added %u mids, total: %u", l->mapIndex - oldIndex, l->mapIndex );
	}
};

//加载游戏 已修改为不断线
void initialize_game(BANANA* client)
{
	LOBBY* l;
	unsigned ch;
	int dont_send = 0;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;
	memset(l, 0, sizeof(LOBBY));

	l->difficulty = client->decryptbuf[0x50];
	l->battle = client->decryptbuf[0x51];
	l->challenge = client->decryptbuf[0x52];
	l->episode = client->decryptbuf[0x53];
	l->oneperson = client->decryptbuf[0x54];
	l->start_time = (unsigned)servertime;
	if (l->difficulty > 0x03)
	{
		WriteLog("difficulty > 4");
		//client->todc = 1; //判断选择难度超出了4 OK
		dont_send = 1;
		return;
	}
	else
		if ((l->battle) && (l->challenge))
		{
			WriteLog("(l->battle) && (l->challenge)");
			//client->todc = 1; //判断同时出现在挑战模式和对战模式 OK
			dont_send = 1;
			return;
		}
		else
			if (l->episode > 0x03)
			{
				WriteLog("l->episode > 3");
				//client->todc = 1; //判断同时出现在挑战模式和对战模式 OK
				dont_send = 1;
				return;
			}
			else
				if ((l->oneperson) && ((l->challenge) || (l->battle)))
				{
					WriteLog("(l->oneperson) && ((l->challenge) || (l->battle))");
					//client->todc = 1; //判断同时出现在挑战模式和对战模式 OK
					dont_send = 1;
					return;
				}
				else
					if (l->challenge && l->episode > 0x03)
					{
						WriteLog("(l->challenge && l->episode > 0x03)");
						//client->todc = 1; //判断同时出现在挑战模式和对战模式 OK
						dont_send = 1;
						return;
					}
	if (!dont_send)
	{
		if (l->battle)
			l->battle = 1;
		if (l->challenge)
			l->challenge = 1;
		if (l->oneperson)
			l->oneperson = 1;
		memcpy(&l->gameName[0], &client->decryptbuf[0x14], 30);
		memcpy(&l->gamePassword[0], &client->decryptbuf[0x30], 32);
		l->in_use = 1; //房间使用中
		l->gameMonster[0] = (unsigned char)mt_lrand() % 256;//对应四种难度怪物
		l->gameMonster[1] = (unsigned char)mt_lrand() % 256;
		l->gameMonster[2] = (unsigned char)mt_lrand() % 256;
		l->gameMonster[3] = (unsigned char)mt_lrand() % 16;
		memset(&l->gameMap[0], 0, 128);//初始化游戏地图
		l->playerItemID[0] = 0x10000; //初始化四种难度的玩家物品
		l->playerItemID[1] = 0x210000;
		l->playerItemID[2] = 0x410000;
		l->playerItemID[3] = 0x610000;
		l->bankItemID[0] = 0x10000; //初始化四种难度还是四个玩家的银行物品
		l->bankItemID[1] = 0x210000;
		l->bankItemID[2] = 0x410000;
		l->bankItemID[3] = 0x610000;
		l->leader = 0; //初始化队长为0
		l->sectionID = client->character.sectionID;
		l->itemID = 0x810000;
		l->mapIndex = 0;//初始化地图索引
		memset(&l->mapData[0], 0, sizeof(l->mapData));
		l->rareIndex = 0;
		for (ch = 0;ch < 0x20;ch++)
			l->rareData[ch] = 0xFF;
		switch (l->episode)
		{
		case 0x01:
			// Episode 1 章节1
			if (!l->oneperson) //非单人模式下的地图数据载入
			{
				l->bptable = &ep1battle[0x60 * l->difficulty];

				LoadMapData(l, 0, "map\\map_city00_00e.dat");
				LoadObjectData(l, 0, "map\\map_city00_00o.dat");

				l->gameMap[12] = (unsigned char)mt_lrand() % 5; // Forest 1
				LoadMapData(l, 0, Forest1_Online_Maps[l->gameMap[12]]);
				LoadObjectData(l, 0, Forest1_Online_Maps[l->gameMap[12]]);

				l->gameMap[20] = (unsigned char)mt_lrand() % 5; // Forest 2
				LoadMapData(l, 0, Forest2_Online_Maps[l->gameMap[20]]);
				LoadObjectData(l, 0, Forest2_Online_Maps[l->gameMap[20]]);

				l->gameMap[24] = (unsigned char)mt_lrand() % 3; // Cave 1
				l->gameMap[28] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Cave1_Online_Maps[(l->gameMap[24] * 2) + l->gameMap[28]]);
				LoadObjectData(l, 0, Cave1_Online_Maps[(l->gameMap[24] * 2) + l->gameMap[28]]);

				l->gameMap[32] = (unsigned char)mt_lrand() % 3; // Cave 2
				l->gameMap[36] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Cave2_Online_Maps[(l->gameMap[32] * 2) + l->gameMap[36]]);
				LoadObjectData(l, 0, Cave2_Online_Maps[(l->gameMap[32] * 2) + l->gameMap[36]]);

				l->gameMap[40] = (unsigned char)mt_lrand() % 3; // Cave 3
				l->gameMap[44] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Cave3_Online_Maps[(l->gameMap[40] * 2) + l->gameMap[44]]);
				LoadObjectData(l, 0, Cave3_Online_Maps[(l->gameMap[40] * 2) + l->gameMap[44]]);

				l->gameMap[48] = (unsigned char)mt_lrand() % 3; // Mine 1
				l->gameMap[52] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Mine1_Online_Maps[(l->gameMap[48] * 2) + l->gameMap[52]]);
				LoadObjectData(l, 0, Mine1_Online_Maps[(l->gameMap[48] * 2) + l->gameMap[52]]);

				l->gameMap[56] = (unsigned char)mt_lrand() % 3; // Mine 2
				l->gameMap[60] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Mine2_Online_Maps[(l->gameMap[56] * 2) + l->gameMap[60]]);
				LoadObjectData(l, 0, Mine2_Online_Maps[(l->gameMap[56] * 2) + l->gameMap[60]]);

				l->gameMap[64] = (unsigned char)mt_lrand() % 3; // Ruins 1
				l->gameMap[68] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins1_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);
				LoadObjectData(l, 0, Ruins1_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);

				l->gameMap[72] = (unsigned char)mt_lrand() % 3; // Ruins 2
				l->gameMap[76] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins2_Online_Maps[(l->gameMap[72] * 2) + l->gameMap[76]]);
				LoadObjectData(l, 0, Ruins2_Online_Maps[(l->gameMap[72] * 2) + l->gameMap[76]]);

				l->gameMap[80] = (unsigned char)mt_lrand() % 3; // Ruins 3
				l->gameMap[84] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins3_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);
				LoadObjectData(l, 0, Ruins3_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);
			}
			else
			{
				l->bptable = &ep1battle_off[0x60 * l->difficulty];

				LoadMapData(l, 0, "map\\map_city00_00e_s.dat");
				LoadObjectData(l, 0, "map\\map_city00_00o_s.dat");

				l->gameMap[12] = (unsigned char)mt_lrand() % 3; // Forest 1
				LoadMapData(l, 0, Forest1_Offline_Maps[l->gameMap[12]]);
				LoadObjectData(l, 0, Forest1_Offline_Objects[l->gameMap[12]]);

				l->gameMap[20] = (unsigned char)mt_lrand() % 3; // Forest 2
				LoadMapData(l, 0, Forest2_Offline_Maps[l->gameMap[20]]);
				LoadObjectData(l, 0, Forest2_Offline_Objects[l->gameMap[20]]);

				l->gameMap[24] = (unsigned char)mt_lrand() % 3; // Cave 1
				LoadMapData(l, 0, Cave1_Offline_Maps[l->gameMap[24]]);
				LoadObjectData(l, 0, Cave1_Offline_Objects[l->gameMap[24]]);

				l->gameMap[32] = (unsigned char)mt_lrand() % 3; // Cave 2
				LoadMapData(l, 0, Cave2_Offline_Maps[l->gameMap[32]]);
				LoadObjectData(l, 0, Cave2_Offline_Objects[l->gameMap[32]]);

				l->gameMap[40] = (unsigned char)mt_lrand() % 3; // Cave 3
				LoadMapData(l, 0, Cave3_Offline_Maps[l->gameMap[40]]);
				LoadObjectData(l, 0, Cave3_Offline_Objects[l->gameMap[40]]);

				l->gameMap[48] = (unsigned char)mt_lrand() % 3; // Mine 1
				l->gameMap[52] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Mine1_Online_Maps[(l->gameMap[48] * 2) + l->gameMap[52]]);
				LoadObjectData(l, 0, Mine1_Online_Maps[(l->gameMap[48] * 2) + l->gameMap[52]]);

				l->gameMap[56] = (unsigned char)mt_lrand() % 3; // Mine 2
				l->gameMap[60] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Mine2_Online_Maps[(l->gameMap[56] * 2) + l->gameMap[60]]);
				LoadObjectData(l, 0, Mine2_Online_Maps[(l->gameMap[56] * 2) + l->gameMap[60]]);

				l->gameMap[64] = (unsigned char)mt_lrand() % 3; // Ruins 1
				l->gameMap[68] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins1_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);
				LoadObjectData(l, 0, Ruins1_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);

				l->gameMap[72] = (unsigned char)mt_lrand() % 3; // Ruins 2
				l->gameMap[76] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins2_Online_Maps[(l->gameMap[72] * 2) + l->gameMap[76]]);
				LoadObjectData(l, 0, Ruins2_Online_Maps[(l->gameMap[72] * 2) + l->gameMap[76]]);

				l->gameMap[80] = (unsigned char)mt_lrand() % 3; // Ruins 3
				l->gameMap[84] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Ruins3_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);
				LoadObjectData(l, 0, Ruins3_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);
			}
			LoadMapData(l, 0, "map\\map_boss01e.dat");
			LoadObjectData(l, 0, "map\\map_boss01o.dat");
			LoadMapData(l, 0, "map\\map_boss02e.dat");
			LoadObjectData(l, 0, "map\\map_boss02o.dat");
			LoadMapData(l, 0, "map\\map_boss03e.dat");
			LoadObjectData(l, 0, "map\\map_boss03o.dat");
			LoadMapData(l, 0, "map\\map_boss04e.dat");
			if (l->oneperson)
				LoadObjectData(l, 0, "map\\map_boss04_offo.dat");
			else
				LoadObjectData(l, 0, "map\\map_boss04o.dat");
			break;
		case 0x02:
			// Episode 2
			if (!l->oneperson)
			{
				l->bptable = &ep2battle[0x60 * l->difficulty];
				LoadMapData(l, 0, "map\\map_labo00_00e.dat");
				LoadObjectData(l, 0, "map\\map_labo00_00o.dat");

				l->gameMap[8] = (unsigned char)mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				LoadMapData(l, 0, Temple1_Online_Maps[l->gameMap[8]]);
				LoadObjectData(l, 0, Temple1_Online_Maps[l->gameMap[8]]);

				l->gameMap[16] = (unsigned char)mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				LoadMapData(l, 0, Temple2_Online_Maps[l->gameMap[16]]);
				LoadObjectData(l, 0, Temple2_Online_Maps[l->gameMap[16]]);

				l->gameMap[24] = (unsigned char)mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				LoadMapData(l, 0, Spaceship1_Online_Maps[l->gameMap[24]]);
				LoadObjectData(l, 0, Spaceship1_Online_Maps[l->gameMap[24]]);

				l->gameMap[32] = (unsigned char)mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00;
				LoadMapData(l, 0, Spaceship2_Online_Maps[l->gameMap[32]]);
				LoadObjectData(l, 0, Spaceship2_Online_Maps[l->gameMap[32]]);

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char)mt_lrand() % 3; // Jungle 1
				LoadMapData(l, 0, Jungle1_Online_Maps[l->gameMap[44]]);
				LoadObjectData(l, 0, Jungle1_Online_Maps[l->gameMap[44]]);

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char)mt_lrand() % 3; // Jungle 2
				LoadMapData(l, 0, Jungle2_Online_Maps[l->gameMap[52]]);
				LoadObjectData(l, 0, Jungle2_Online_Maps[l->gameMap[52]]);

				l->gameMap[56] = 0x00;
				l->gameMap[60] = (unsigned char)mt_lrand() % 3; // Jungle 3
				LoadMapData(l, 0, Jungle3_Online_Maps[l->gameMap[60]]);
				LoadObjectData(l, 0, Jungle3_Online_Maps[l->gameMap[60]]);

				l->gameMap[64] = (unsigned char)mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Jungle4_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);
				LoadObjectData(l, 0, Jungle4_Online_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char)mt_lrand() % 3; // Jungle 5
				LoadMapData(l, 0, Jungle5_Online_Maps[l->gameMap[76]]);
				LoadObjectData(l, 0, Jungle5_Online_Maps[l->gameMap[76]]);

				l->gameMap[80] = (unsigned char)mt_lrand() % 2; // Seabed 1
				l->gameMap[84] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Seabed1_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);
				LoadObjectData(l, 0, Seabed1_Online_Maps[(l->gameMap[80] * 2) + l->gameMap[84]]);

				l->gameMap[88] = (unsigned char)mt_lrand() % 2; // Seabed 2
				l->gameMap[92] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Seabed2_Online_Maps[(l->gameMap[88] * 2) + l->gameMap[92]]);
				LoadObjectData(l, 0, Seabed2_Online_Maps[(l->gameMap[88] * 2) + l->gameMap[92]]);
			}
			else
			{
				l->bptable = &ep2battle_off[0x60 * l->difficulty];
				LoadMapData(l, 0, "map\\map_labo00_00e_s.dat");
				LoadObjectData(l, 0, "map\\map_labo00_00o_s.dat");

				l->gameMap[8] = (unsigned char)mt_lrand() % 2; // Temple 1
				l->gameMap[12] = 0x00;
				LoadMapData(l, 0, Temple1_Offline_Maps[l->gameMap[8]]);
				LoadObjectData(l, 0, Temple1_Offline_Maps[l->gameMap[8]]);

				l->gameMap[16] = (unsigned char)mt_lrand() % 2; // Temple 2
				l->gameMap[20] = 0x00;
				LoadMapData(l, 0, Temple2_Offline_Maps[l->gameMap[16]]);
				LoadObjectData(l, 0, Temple2_Offline_Maps[l->gameMap[16]]);

				l->gameMap[24] = (unsigned char)mt_lrand() % 2; // Spaceship 1
				l->gameMap[28] = 0x00;
				LoadMapData(l, 0, Spaceship1_Offline_Maps[l->gameMap[24]]);
				LoadObjectData(l, 0, Spaceship1_Offline_Maps[l->gameMap[24]]);

				l->gameMap[32] = (unsigned char)mt_lrand() % 2; // Spaceship 2
				l->gameMap[36] = 0x00;
				LoadMapData(l, 0, Spaceship2_Offline_Maps[l->gameMap[32]]);
				LoadObjectData(l, 0, Spaceship2_Offline_Maps[l->gameMap[32]]);

				l->gameMap[40] = 0x00;
				l->gameMap[44] = (unsigned char)mt_lrand() % 3; // Jungle 1
				LoadMapData(l, 0, Jungle1_Offline_Maps[l->gameMap[44]]);
				LoadObjectData(l, 0, Jungle1_Offline_Maps[l->gameMap[44]]);

				l->gameMap[48] = 0x00;
				l->gameMap[52] = (unsigned char)mt_lrand() % 3; // Jungle 2
				LoadMapData(l, 0, Jungle2_Offline_Maps[l->gameMap[52]]);
				LoadObjectData(l, 0, Jungle2_Offline_Maps[l->gameMap[52]]);

				l->gameMap[56] = 0x00;
				l->gameMap[60] = (unsigned char)mt_lrand() % 3; // Jungle 3
				LoadMapData(l, 0, Jungle3_Offline_Maps[l->gameMap[60]]);
				LoadObjectData(l, 0, Jungle3_Offline_Maps[l->gameMap[60]]);

				l->gameMap[64] = (unsigned char)mt_lrand() % 2; // Jungle 4
				l->gameMap[68] = (unsigned char)mt_lrand() % 2;
				LoadMapData(l, 0, Jungle4_Offline_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);
				LoadObjectData(l, 0, Jungle4_Offline_Maps[(l->gameMap[64] * 2) + l->gameMap[68]]);

				l->gameMap[72] = 0x00;
				l->gameMap[76] = (unsigned char)mt_lrand() % 3; // Jungle 5
				LoadMapData(l, 0, Jungle5_Offline_Maps[l->gameMap[76]]);
				LoadObjectData(l, 0, Jungle5_Offline_Maps[l->gameMap[76]]);

				l->gameMap[80] = (unsigned char)mt_lrand() % 2; // Seabed 1
				LoadMapData(l, 0, Seabed1_Offline_Maps[l->gameMap[80]]);
				LoadObjectData(l, 0, Seabed1_Offline_Maps[l->gameMap[80]]);

				l->gameMap[88] = (unsigned char)mt_lrand() % 2; // Seabed 2
				LoadMapData(l, 0, Seabed2_Offline_Maps[l->gameMap[88]]);
				LoadObjectData(l, 0, Seabed2_Offline_Maps[l->gameMap[88]]);
			}
			LoadMapData(l, 0, "map\\map_boss05e.dat");
			LoadMapData(l, 0, "map\\map_boss06e.dat");
			LoadMapData(l, 0, "map\\map_boss07e.dat");
			LoadMapData(l, 0, "map\\map_boss08e.dat");
			if (l->oneperson) //单人模式的BOSS地图
			{
				LoadObjectData(l, 0, "map\\map_boss05_offo.dat");
				LoadObjectData(l, 0, "map\\map_boss06_offo.dat");
				LoadObjectData(l, 0, "map\\map_boss07_offo.dat");
				LoadObjectData(l, 0, "map\\map_boss08_offo.dat");
			}
			else
			{
				LoadObjectData(l, 0, "map\\map_boss05o.dat");
				LoadObjectData(l, 0, "map\\map_boss06o.dat");
				LoadObjectData(l, 0, "map\\map_boss07o.dat");
				LoadObjectData(l, 0, "map\\map_boss08o.dat");
			}
			break;
		case 0x03:
			// Episode 4
			if (!l->oneperson)
			{
				l->bptable = &ep4battle[0x60 * l->difficulty];
				LoadMapData(l, 0, "map\\map_city02_00_00e.dat");
				LoadObjectData(l, 0, "map\\map_city02_00_00o.dat");
			}
			else
			{
				l->bptable = &ep4battle_off[0x60 * l->difficulty];
				LoadMapData(l, 0, "map\\map_city02_00_00e_s.dat");
				LoadObjectData(l, 0, "map\\map_city02_00_00o_s.dat");
			}

			l->gameMap[12] = (unsigned char)mt_lrand() % 3; // Crater East
			LoadMapData(l, 0, Crater_East_Online_Maps[l->gameMap[12]]);
			LoadObjectData(l, 0, Crater_East_Online_Maps[l->gameMap[12]]);

			l->gameMap[20] = (unsigned char)mt_lrand() % 3; // Crater West
			LoadMapData(l, 0, Crater_West_Online_Maps[l->gameMap[20]]);
			LoadObjectData(l, 0, Crater_West_Online_Maps[l->gameMap[20]]);

			l->gameMap[28] = (unsigned char)mt_lrand() % 3; // Crater South
			LoadMapData(l, 0, Crater_South_Online_Maps[l->gameMap[28]]);
			LoadObjectData(l, 0, Crater_South_Online_Maps[l->gameMap[28]]);

			l->gameMap[36] = (unsigned char)mt_lrand() % 3; // Crater North
			LoadMapData(l, 0, Crater_North_Online_Maps[l->gameMap[36]]);
			LoadObjectData(l, 0, Crater_North_Online_Maps[l->gameMap[36]]);

			l->gameMap[44] = (unsigned char)mt_lrand() % 3; // Crater Interior
			LoadMapData(l, 0, Crater_Interior_Online_Maps[l->gameMap[44]]);
			LoadObjectData(l, 0, Crater_Interior_Online_Maps[l->gameMap[44]]);

			l->gameMap[48] = (unsigned char)mt_lrand() % 3; // Desert 1
			LoadMapData(l, 1, Desert1_Online_Maps[l->gameMap[48]]);
			LoadObjectData(l, 1, Desert1_Online_Maps[l->gameMap[48]]);

			l->gameMap[60] = (unsigned char)mt_lrand() % 3; // Desert 2
			LoadMapData(l, 1, Desert2_Online_Maps[l->gameMap[60]]);
			LoadObjectData(l, 1, Desert2_Online_Maps[l->gameMap[60]]);

			l->gameMap[64] = (unsigned char)mt_lrand() % 3; // Desert 3
			LoadMapData(l, 1, Desert3_Online_Maps[l->gameMap[64]]);
			LoadObjectData(l, 1, Desert3_Online_Maps[l->gameMap[64]]);

			LoadMapData(l, 0, "map\\map_boss09_00_00e.dat");
			LoadObjectData(l, 0, "map\\map_boss09_00_00o.dat");
			//LoadMapData (l, "map\\map_test01_00_00e.dat");
			break;
		default:
			break;
		}
	}
	else
		Send1A(L"Bad game arguments supplied.", client, 58);
}

void Send64(BANANA* client)
{
	LOBBY* l;
	unsigned Offset;
	unsigned ch;

	if (!client->lobby)
		return;
	l = (LOBBY*)client->lobby;

	for (ch = 0;ch < 4;ch++)
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
	for (ch = 0;ch < 4;ch++)
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
	for (ch = 0;ch < 4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
		{
			PacketData[Offset + 2] = 0x01;
			Offset += 0x04;
			*(unsigned*)&PacketData[Offset] = l->client[ch]->guildcard;
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
				PacketData2[0x0A] = 0x01; // 注意 这个数据包要留意 未知
				PacketData2[0x0B] = 0xFF; // 注意 这个数据包要留意 未知
				PacketData2[0x0C] = 0x01; // 注意 这个数据包要留意 未知
				PacketData2[0x0E] = 0x01; // 注意 这个数据包要留意 未知
				PacketData2[0x16] = 0x01;
				*(unsigned*)&PacketData2[0x18] = client->guildcard;
				PacketData2[0x30] = client->clientID;
				memcpy(&PacketData2[0x34], &client->character.name[0], 24);
				PacketData2[0x54] = 0x02; // 注意 这个数据包要留意 未知
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
	// Leader ID @ 0x199 这个数据已处理
	// Difficulty @ 0x19B 这个数据已处理
	// Event @ 0x19D 这个数据已处理
	// Section ID of Leader @ 0x19E 这个数据已处理
	// Game Monster @ 0x1A0 (4 bytes) 这个数据已处理
	// Episode @ 0x1A4 这个数据已处理
	// 0x01 @ 0x1A5 
	// One-person @ 0x1A6 这个数据已处理

	//这里做个客户端输出查看一下数据是如何出现的
	/*wchar_t*packet[128];
	packet[128] = 0x01;
	Send1A(packet[128], client, -1);*/


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
	*(unsigned*)&PacketData[0x1A0] = *(unsigned*)&l->gameMonster;
	PacketData[0x1A4] = l->episode;
	PacketData[0x1A5] = 0x01; // 注意 这个数据包要留意 未知 应该是游戏中
	PacketData[0x1A6] = l->oneperson;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], 0x1A8);

	/* Let's send the team data...发送公会信息 */

	SendToLobby(client->lobby, 4, MakePacketEA15(client), 2152, client->guildcard);

	client->bursting = 1;
	//WriteLog("Send64发送如下信息\n %s", client); //sancaros Send debug的方式
}

//发送服务器数据给客户端
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

	for (ch = ch;ch < 15;ch++)
	{
		l = &b->lobbies[ch];
		if (l->lobbyCount < 12)
		{
			for (ch2 = 0;ch2 < 12;ch2++)
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
			// Send68 here with joining client (use ch2 for clientid) 指令68在这发送了加入的客户端
			l->lobbyCount = 0;
			for (ch2 = 0;ch2 < 12;ch2++)
			{
				if ((l->slot_use[ch2]) && (l->client[ch2]))
					l->lobbyCount++;
			}

			memset(&PacketData[0x00], 0, 0x14);
			PacketData[0x04] = l->lobbyCount;
			PacketData[0x08] = client->clientID;
			PacketData[0x0B] = ch;
			PacketData[0x0C] = client->block;
			PacketData[0x0E] = shipEvent;
			Offset = 0x14;
			for (ch2 = 0;ch2 < 12;ch2++)
			{
				if ((l->slot_use[ch2]) && (l->client[ch2]))
				{
					memset(&PacketData[Offset], 0, 1312);
					Offset2 = Offset;
					*(unsigned*)&PacketData[Offset] = 0x00010000;
					Offset += 4;
					lClient = l->client[ch2];
					*(unsigned*)&PacketData[Offset] = lClient->guildcard;
					Offset += 24;
					*(unsigned*)&PacketData[Offset] = ch2;
					Offset += 4;
					memcpy(&PacketData[Offset], &lClient->character.name[0], 24);
					Offset += 32;
					PacketData[Offset++] = 0x02;
					Offset += 3;
					memcpy(&PacketData[Offset], &lClient->character.inventoryUse, 1244);
					// Prevent crashing with NPCs 防止NPC们崩溃
					if (lClient->character.skinFlag)
						memset(&PacketData[Offset + 0x3A8], 0, 10);

					if (lClient->isgm == 1)
						*(unsigned*)&PacketData[Offset + 0x388] = globalName;
					else
						if (isLocalGM(lClient->guildcard))
							*(unsigned*)&PacketData[Offset + 0x388] = localName;
						else
							*(unsigned*)&PacketData[Offset + 0x388] = normalName;
					if ((lClient->guildcard == client->guildcard) && (l->lobbyCount > 1))
					{
						memcpy(&PacketData2[0x00], &PacketData[0], 0x14);
						PacketData2[0x00] = 0x34;
						PacketData2[0x01] = 0x05;
						PacketData2[0x02] = 0x68;
						PacketData2[0x04] = 0x01;
						memcpy(&PacketData2[0x14], &PacketData[Offset2], 1312);
						SendToLobby(client->lobby, 12, &PacketData2[0x00], 1332, client->guildcard);
					}
					Offset += 1244;
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
	memset(&client->encryptbuf[0x00], 0, 8);//8 * 4 = 32
	client->encryptbuf[0] = 0x10;//16
	client->encryptbuf[1] = 0x02;//2
	client->encryptbuf[2] = 0x60;//96
	client->encryptbuf[8] = 0x6F;//111
	client->encryptbuf[9] = 0x84;//132
	memcpy(&client->encryptbuf[0x0C], &client->character.quest_data1[0], 0x210);//528 +8 =536
	memset(&client->encryptbuf[0x20C], 0, 4);//4 * 4 = 16
	encryptcopy(client, &client->encryptbuf[0x00], 0x210);//528 528 *2=1056*2=2112

	SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, client->guildcard);//2152 - 2112 = 40
	client->bursting = 1;
}

//房间指令集
void Send95(BANANA* client)
{
	debug("测试123123");
	client->lobbyOK = 1;
	memset(&client->encryptbuf[0x00], 0, 8);
	client->encryptbuf[0x00] = 0x08;
	client->encryptbuf[0x02] = 0x95;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &client->encryptbuf[0], 8);
	// 恢复永久角色
	if (client->character_backup)
	{
		if (client->mode && client->guildcard)
		{
			memcpy(&client->challenge_data.challengeData, client->character.challengeData, 320);
			memcpy(&client->battle_data.battleData, client->character.battleData, 92);
			memcpy(&client->character, client->character_backup, sizeof(client->character));
			memcpy(&client->character.challengeData, client->challenge_data.challengeData, 320);
			memcpy(&client->character.battleData, client->battle_data.battleData, 92);

			client->mode = 0;
			debug("测试3232");
		}
		free(client->character_backup);
		client->character_backup = NULL;
	}
	debug("测试11111111113");
}

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

	l = (LOBBY*)client->lobby;

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
			switch (episode)
			{
			case 0x01:
			{
				qm = &quest_menus[10];
				qc = 11;
			}
			break;
			case 0x02:
			{
				qm = &quest_menus[11];
				qc = 12;
			}
			break;
			case 0x03:
			{
				printf("No challenge quests for ep4");
			}
			break;
			}
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
		for (ch = 0;ch < qm->num_categories;ch++)
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
		for (ch = 0;ch < qm->quest_counts[category];ch++)
		{
			q = &quests[qm->quest_indexes[category][ch]];
			show_quest = 0;
			if ((solo) && (episode == 0x01)) //如果单人模式并且章节 1 时
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
						show_quest = 2; // Sets a variance if they've cleared the quest... 设置方差，如果他们已经清除了任务 
					else
					{
						tier1 = 0;
						if ((qflag(&client->character.quest_data1[0], 0x65, diff)) &&  // Cleared first tier 当清理完第一章时
							(qflag(&client->character.quest_data1[0], 0x67, diff)) &&
							(qflag(&client->character.quest_data1[0], 0x6B, diff)))
							tier1 = 1;
						if (qflag(&client->character.quest_data1[0], quest_flag, diff) == 0)
						{ // When the quest hasn't been completed... 当任务没有被完成时
						  // Forest quests 森林区域任务
							switch (qn)
							{
							case 4: // Battle Training 试练
							case 2: // Claiming a Stake 拉古奥尔的大地主
							case 1: // Magnitude of Metal 钢之心
								show_quest = 1;
								break;
							case 5: // Journalistic Pursuit 先驱者二号的新闻
							case 6: // The Fake In Yellow 黄色的伪装
							case 7: // Native Research 大地的呼声
							case 9: // Gran Squall 疾风号
								if (tier1)
									show_quest = 1;
								break;
							case 8: // Forest of Sorrow 恸哭的森林
								if (qflag(&client->character.quest_data1[0], 0x71, diff)) // Cleared Native Research 完成大地的呼声时 隐藏任务
									show_quest = 1;
								break;
							case 26: // Central Dome Fire Swirl 中心圆顶的火灾
								if (qflag(&client->character.quest_data1[0], 0x73, diff)) // Cleared Forest of Sorrow 完成恸哭的森林时 隐藏任务
									show_quest = 1;
								break;
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0], 0x1F9, diff)))
							{
								// Cave quests (shown after Dragon is defeated) 洞穴任务（当巨龙被击败时）
								switch (qn)
								{
								case 03: // The Value of Money 金钱的价值
								case 11: // The Lost Bride 消失的新娘
								case 14: // Secret Delivery 秘密货物
								case 17: // Grave's Butler 格雷夫家的管家
								case 10: // Addicting Food 恶魔的食物
									show_quest = 1; // Always shown if first tier was cleared  如果第一层被清除，总是显示
									break;
								case 12: // Waterfall Tears 无归瀑
								case 15: // Soul of a Blacksmith 匠之魂
									if ((qflag(&client->character.quest_data1[0], 0x77, diff)) && // Cleared Addicting Food 当完成恶魔的食物
										(qflag(&client->character.quest_data1[0], 0x79, diff)) && // Cleared The Lost Bride 当完成消失的新娘
										(qflag(&client->character.quest_data1[0], 0x7F, diff)) && // Cleared Secret Delivery 当完成秘密货物
										(qflag(&client->character.quest_data1[0], 0x85, diff)))  // Cleared Grave's Butler 当完成格雷夫家的管家
										show_quest = 1;
									break;
								case 13: // Black Paper 黑页
									if (qflag(&client->character.quest_data1[0], 0x7B, diff)) // Cleared Waterfall Tears 当完成无归瀑
										show_quest = 1;
									break;
								}
							}

							if ((tier1) && (qflag(&client->character.quest_data1[0], 0x1FF, diff)))
							{
								// Mine quests (shown after De Rol Le is defeated) 洞窟任务（当击败De Rol Le怪物时）
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
					// Check party's qualification for quests... 检查队伍的任务资格 
					switch (episode)
					{
					case 0x01:
						quest_flag = (0x1F3 + (ch << 1));
						for (ch2 = 0x1F5;ch2 <= quest_flag;ch2 += 2)
							for (ch3 = 0;ch3 < 4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					case 0x02:
						quest_flag = (0x211 + (ch << 1));
						for (ch2 = 0x213;ch2 <= quest_flag;ch2 += 2)
							for (ch3 = 0;ch3 < 4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					case 0x03:
						quest_flag = (0x2BC + ch);
						for (ch2 = 0x2BD;ch2 <= quest_flag;ch2++)
							for (ch3 = 0;ch3 < 4;ch3++)
								if ((l->client[ch3]) && (!qflag(&l->client[ch3]->character.quest_data1[0], ch2, diff)))
									show_quest = 0;
						break;
					}
				}
			}
			if (show_quest) //如果显示任务 判断任务是否已解锁
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
	//WriteLog("SendA2发送如下信息\n %s", client); //Sancaros Send debug的方式
}

//A0指令
void SendA0(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketA0Data[0], *(unsigned short*)&PacketA0Data[0]);
	//WriteLog("SendA0发送如下信息\n %s", client); //Sancaros Send debug的方式
}

//07指令
void Send07(BANANA* client)
{
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &Packet07Data[0], *(unsigned short*)&Packet07Data[0]);
	//Sancaros Send debug的方式
#ifdef DEBUG
	unsigned short size;
	size = *(unsigned short*)&client->decryptbuf[0x00];
	WriteLog("Send1D 指令 \"%02x\" 数据记录如下. (数据如下)", client->decryptbuf[0x07]);
	packet_to_text(&client->decryptbuf[0x00], size);
	WriteLog("\n %s", &dp[0]);
#endif // DEBUG
}

//B0常用文本指令
void SendB0(const wchar_t* mes, BANANA* client, int line)
{
	setlocale(LC_ALL, "chs");
	if (line > -1 && wstrlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	}
	unsigned short xB0_Len;

	memcpy(&PacketData[0], &PacketB0[0], sizeof(PacketB0));
	xB0_Len = sizeof(PacketB0);

	while (*mes != 0x00)
	{
		PacketData[xB0_Len++] = (unsigned char)*(mes++);
		PacketData[xB0_Len++] = 0x00;
	}

	PacketData[xB0_Len++] = 0x00;
	PacketData[xB0_Len++] = 0x00;

	while (xB0_Len % 8)
		PacketData[xB0_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xB0_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], xB0_Len);

	//WriteLog("SendB0发送如下信息\n %s \n%s",client ,mes); //Sancaros Send debug的方式
}

//给全玩家发送公告的代码
void BroadcastToAll(unsigned short* mes, BANANA* client)
{
	unsigned short xEE_Len;
	unsigned short* pd;
	unsigned short* cname;
	unsigned ch, connectNum;

	memcpy(&PacketData[0], &PacketEE[0], sizeof(PacketEE));
	xEE_Len = sizeof(PacketEE);

	pd = (unsigned short*)&PacketData[xEE_Len];
	cname = (unsigned short*)&client->character.name[4];

	*(pd++) = 91;
	*(pd++) = 71;
	*(pd++) = 77;
	*(pd++) = 93;

	xEE_Len += 8;

	while (*cname != 0x0000)
	{
		*(pd++) = *(cname++);
		xEE_Len += 2;
	}

	*(pd++) = 58;
	*(pd++) = 32;

	xEE_Len += 4;

	while (*mes != 0x0000)
	{
		if (*mes == 0x0024)
		{
			*(pd++) = 0x0009;
			mes++;
		}
		else
		{
			*(pd++) = *(mes++);
		}
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xEE_Len;
	if (client->announce == 1)
	{
		// Local announcement
		for (ch = 0;ch < serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard)
			{
				cipher_ptr = &connections[connectNum]->server_cipher;
				encryptcopy(connections[connectNum], &PacketData[0], xEE_Len);
			}
		}
	}
	else
	{
		// Global announcement 全服公告
		if ((logon) && (logon->sockfd >= 0))
		{
			// Send the announcement to the logon server. 发送公告给全服玩家
			logon->encryptbuf[0x00] = 0x12;
			logon->encryptbuf[0x01] = 0x00;
			*(unsigned*)&logon->encryptbuf[0x02] = client->guildcard;
			memcpy(&logon->encryptbuf[0x06], &PacketData[sizeof(PacketEE)], xEE_Len - sizeof(PacketEE));
			compressShipPacket(logon, &logon->encryptbuf[0x00], 6 + xEE_Len - sizeof(PacketEE));
		}
	}
	client->announce = 0;
	//WriteLog("BroadcastToAll发送如下信息\n %s \n%s", client, mes); //Sancaros Send debug的方式
}

//给全球发送公告的代码
void GlobalBroadcast(unsigned short* mes)
{
	unsigned short xEE_Len;
	unsigned short* pd;
	unsigned ch, connectNum;

	memcpy(&PacketData[0], &PacketEE[0], sizeof(PacketEE));
	xEE_Len = sizeof(PacketEE);

	pd = (unsigned short*)&PacketData[xEE_Len];

	while (*mes != 0x0000)
	{
		*(pd++) = *(mes++);
		xEE_Len += 2;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xEE_Len;
	for (ch = 0;ch < serverNumConnections;ch++)
	{
		connectNum = serverConnectionList[ch];
		if (connections[connectNum]->guildcard)
		{
			cipher_ptr = &connections[connectNum]->server_cipher;
			encryptcopy(connections[connectNum], &PacketData[0], xEE_Len);
		}
	}
	//WriteLog("GlobalBroadcast发送如下信息\n %s \n%s", mes); //Sancaros Send debug的方式
}

//EE指令文本信息代码
void SendEE(const wchar_t* mes, BANANA* client, int line)
{
	setlocale(LC_ALL, "chs");
	if (line > -1 && wstrlen(languageMessages[client->character.lang][line]))
	{
		mes = languageMessages[client->character.lang][line];
	}
	unsigned short xEE_Len;

	memcpy(&PacketData[0], &PacketEE[0], sizeof(PacketEE));
	xEE_Len = sizeof(PacketEE);

	while (*mes != 0x00)
	{
		PacketData[xEE_Len++] = (unsigned char)*(mes++);
		PacketData[xEE_Len++] = 0x00;
	}

	PacketData[xEE_Len++] = 0x00;
	PacketData[xEE_Len++] = 0x00;

	while (xEE_Len % 8)
		PacketData[xEE_Len++] = 0x00;
	*(unsigned short*)&PacketData[0] = xEE_Len;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], xEE_Len);
	//WriteLog("SendEE发送如下信息\n %s \n%s", client, mes); //Sancaros Send debug的方式
}

//用于交换IP地址的代码
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
	//WriteLog("Send19发送如下信息\n %s \n %s.%s.%s.%s", client, ip1, ip2, ip3, ip4); //Sancaros Send debug的方式
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
		*(unsigned*)&client->encryptbuf[0x08] = client->guildcard;
		*(unsigned*)&client->encryptbuf[0x0C] = client->character.teamID;
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
			*(unsigned*)&client->encryptbuf[0x0C] = client->guildcard;
			*(unsigned*)&client->encryptbuf[0x10] = client->character.teamID;
			client->encryptbuf[0x1C] = (unsigned char)client->character.privilegeLevel;
			memcpy(&client->encryptbuf[0x20], &client->character.teamName[0], 28);
			client->encryptbuf[0x3C] = 0x84;
			client->encryptbuf[0x3D] = 0x6C;
			client->encryptbuf[0x3E] = 0x98;
		}
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &client->encryptbuf[0], 0x40);
		if (client->lobbyNum < 0x10) //这里有关于创建房间的代码 sancaros 注意 原来处于屏蔽状态
			SendToLobby(client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0);
		else
			SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0);
		break;
	case 0x13:
	{
		LOBBY* l;
		BANANA* lClient;
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
		for (ch = 0;ch < maxc;ch++)
		{
			if ((l->slot_use[ch]) && (l->client[ch]))
			{
				lClient = l->client[ch];
				*(unsigned*)&client->encryptbuf[EA15Offset] = lClient->character.serial_number;
				EA15Offset += 0x04;
				*(unsigned*)&client->encryptbuf[EA15Offset] = lClient->character.teamID;
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
				*(unsigned*)&client->encryptbuf[EA15Offset] = lClient->character.guildCard;
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
		*(unsigned*)&client->encryptbuf[0x20] = client->character.guildCard;
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
	//WriteLog("SendEA发送如下信息\n %s \n%s", client, command); //Sancaros Send debug的方式
}


//sancaros 不太明白这段的意义 是否是关乎公会的代码
unsigned char* MakePacketEA15(BANANA* client)
{
# pragma warning (disable:4819)
	sprintf(&PacketData[0x00], "\x64\x08\xEA\x15\x01");
	memset(&PacketData[0x05], 0, 3);
	*(unsigned*)&PacketData[0x08] = client->guildcard;
	*(unsigned*)&PacketData[0x0C] = client->character.teamID;
	PacketData[0x18] = (unsigned char)client->character.privilegeLevel;
	memcpy(&PacketData[0x1C], &client->character.teamName[0], 28);
	sprintf(&PacketData[0x38], "\x84\x6C\x98");
	*(unsigned*)&PacketData[0x3C] = client->guildcard;
	PacketData[0x40] = client->clientID;
	memcpy(&PacketData[0x44], &client->character.name[0], 24);
	memcpy(&PacketData[0x64], &client->character.teamFlag[0], 0x800);
	return &PacketData[0];
}

//释放物品内存空间
unsigned free_game_item(LOBBY* l)
{
	unsigned ch, ch2, oldest_item;

	ch2 = oldest_item = 0xFFFFFFFF;

	// If the itemid at the current index is 0, just return that...如果物品ID在当前索引为0,则直接返回

	if ((l->gameItemCount < MAX_SAVED_ITEMS) && (l->gameItem[l->gameItemCount].item.itemid == 0))
		return l->gameItemCount;

	// Scan the gameItem array for any free item slots... 扫描gameItem数组中是否有可用的物品槽 

	for (ch = 0;ch < MAX_SAVED_ITEMS;ch++)
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

	for (ch = 0;ch < MAX_SAVED_ITEMS;ch++)
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

	for (ch = 0;ch < 4;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
			SendEE(L"Lobby inventory problem!  It's advised you quit this game and recreate it.", l->client[ch], 134);
	}

	return 0;
}

//更新游戏中的物品
void UpdateGameItem(BANANA* client)
{
	// Updates the game item list for all of the client's items...  (Used strictly when a client joins a game...) 更新所有客户端背包的游戏物品列表（当客户加入游戏时严格使用…）
	// 更新所有客户端物品的游戏物品列表
	LOBBY* l;
	unsigned ch;

	memset(&client->tekked, 0, sizeof(INVENTORY_ITEM)); // Reset tekking data 重做上次撤消的操作

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	for (ch = 0;ch < client->character.inventoryUse;ch++) // By default this should already be sorted at the top, so no need for an in_use check...	 默认情况下，这应该已经在顶部排序，所以不需要使用中的检查 
		client->character.inventory[ch].item.itemid = l->playerItemID[client->clientID]++; // Keep synchronized 保持同步
}

//背包物品数据量 30组元素 对应30格
INVENTORY_ITEM sort_data[30];
//银行物品数据量 200组元素 对应200格
BANK_ITEM bank_data[200];

//整理客户端物品
void SortClientItems(BANANA* client)
{
	unsigned ch, ch2, ch3, ch4, itemid;

	ch2 = 0x0C;

	memset(&sort_data[0], 0, sizeof(INVENTORY_ITEM) * 30);

	for (ch4 = 0;ch4 < 30;ch4++)
	{
		sort_data[ch4].item.data[1] = 0xFF;
		sort_data[ch4].item.itemid = 0xFFFFFFFF;
	}

	ch4 = 0;

	for (ch = 0;ch < 30;ch++)
	{
		itemid = *(unsigned*)&client->decryptbuf[ch2];
		ch2 += 4;
		if (itemid != 0xFFFFFFFF)
		{
			for (ch3 = 0;ch3 < client->character.inventoryUse;ch3++)
			{
				if ((client->character.inventory[ch3].in_use) && (client->character.inventory[ch3].item.itemid == itemid))
				{
					sort_data[ch4++] = client->character.inventory[ch3];
					break;
				}
			}
		}
	}

	for (ch = 0;ch < 30;ch++)
		client->character.inventory[ch] = sort_data[ch];

}

//清理银行
void CleanUpBank(BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset(&bank_data[0], 0, sizeof(BANK_ITEM) * 200);

	for (ch2 = 0;ch2 < 200;ch2++)
		bank_data[ch2].itemid = 0xFFFFFFFF;

	ch2 = 0;

	for (ch = 0;ch < 200;ch++)
	{
		if (client->character.bankInventory[ch].itemid != 0xFFFFFFFF)
			bank_data[ch2++] = client->character.bankInventory[ch];
	}

	client->character.bankUse = ch2;

	for (ch = 0;ch < 200;ch++)
		client->character.bankInventory[ch] = bank_data[ch];

}

//清理背包
void CleanUpInventory(BANANA* client)
{
	unsigned ch, ch2 = 0;

	memset(&sort_data[0], 0, sizeof(INVENTORY_ITEM) * 30);

	ch2 = 0;

	for (ch = 0;ch < 30;ch++)
	{
		if (client->character.inventory[ch].in_use)
			sort_data[ch2++] = client->character.inventory[ch];
	}

	client->character.inventoryUse = ch2;

	for (ch = 0;ch < 30;ch++)
		client->character.inventory[ch] = sort_data[ch];
}

//清理掉落物
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

//给客户端增加物品
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

	// Adds an item to the client's character data, but only if the item exists in the game item data 
	// to begin with.
	//将物品添加到客户端的字符数据中,但前提是该项存在于游戏物品数据中时 
	if (!client->lobby)
		return 0;

	l = (LOBBY*)client->lobby;

	if (l->challenge) //为了修复挑战模式无法拾取物品的BUG OK
	{
		found_item = 1;
		for (ch = 0;ch < l->gameItemCount;ch++)
		{
			itemNum = l->gameItemList[ch]; // Lookup table for faster searching...快速查找索引表
			if (l->gameItem[itemNum].item.itemid == itemid)
			{
				if (l->gameItem[itemNum].item.data[0] == 0x04)
				{
					count = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
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

		if ((item_added == 0) && (stackable))
		{
			memcpy(&compare_item1, &l->gameItem[itemNum].item.data[0], 3);
			for (ch = 0;ch < client->character.inventoryUse;ch++)
			{
				memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					count = l->gameItem[itemNum].item.data[5];
					stack_count = client->character.inventory[ch].item.data[5];
					if (!stack_count)
						stack_count = 1;
					if ((stack_count + count) > stackable)
					{
						if ((!l->challenge) && (!l->battle)) { //修复挑战模式的错误逻辑
							Send1A(L"Trying to stack over the limit...", client, 59);
							break;
						}
					}
					else
					{
						client->character.inventory[ch].item.data[5] = (unsigned char)(stack_count + count);
						item_added = 1;
					}
					break;
				}
			}
		}
		if (found_item != -1) // We won't disconnect if the item isn't found because there's a possibility another
		{						// person may have nabbed it before our client due to lag...如果找不到物品,我们不会断开连接,因为有可能是另一个人在我们的客户端之前因为通信延迟而抢走了它
			if ((item_added == 0) && (stackable))
			{
				memcpy(&compare_item1, &l->gameItem[itemNum].item.data[0], 3);
				for (ch = 0;ch < client->character.inventoryUse;ch++)
				{
					memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
					if (compare_item1 == compare_item2)
					{
						count = l->gameItem[itemNum].item.data[5];
						stack_count = client->character.inventory[ch].item.data[5];
						if (!stack_count)
							stack_count = 1;
						if ((stack_count + count) > stackable)
						{
							if ((!l->challenge) && (!l->battle)) { //修复挑战模式的错误逻辑
								Send1A(L"Trying to stack over the limit...323123123", client, 59);
								client->todc = 1;
								break;
							}
						}
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
			if (((!client->todc) && (item_added == 0)) || !(l->challenge)) // Make sure the client isn't trying to pick up more than 30 items... 
			{
				if (client->character.inventoryUse >= 30)
				{//缺失 Sancaros
					Send1A(L"Inventory limit reached.", client, 60);
					item_added = 0;
				}
				else
				{
					client->character.inventory[client->character.inventoryUse].in_use = 0x01;
					client->character.inventory[client->character.inventoryUse].flags = 0x00;
					memcpy(&client->character.inventory[client->character.inventoryUse].item, &l->gameItem[itemNum].item, sizeof(ITEM));
					client->character.inventoryUse++;
					item_added = 1;
				}
			}
			if (item_added)
			{
				memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
				l->gameItemList[found_item] = 0xFFFFFFFF;
				CleanUpGameInventory(l);
			}
		}
		return item_added;
	}
	else
	{
		for (ch = 0;ch < l->gameItemCount;ch++)
		{
			itemNum = l->gameItemList[ch]; // Lookup table for faster searching...快速查找索引表
			if (l->gameItem[itemNum].item.itemid == itemid)
			{
				if (l->gameItem[itemNum].item.data[0] == 0x04)
				{
					// Meseta...美赛塔最高限制
					count = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
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
				for (ch = 0;ch < client->character.inventoryUse;ch++)
				{
					memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
					if (compare_item1 == compare_item2)
					{
						count = l->gameItem[itemNum].item.data[5];

						stack_count = client->character.inventory[ch].item.data[5];
						if (!stack_count)
							stack_count = 1;
						//为了判断是否超过上限，如超过上限则断开连接
						if ((stack_count + count) > stackable)
						{
							Send1A(L"Trying to stack over the limit...", client, 59);
							client->todc = 1;
						}
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

			if ((!client->todc) && (item_added == 0)) // Make sure the client isn't trying to pick up more than 30 items... 
													  //确保客户端不尝试收集超过30个物品 需要修复挑战模式下物品无法拾取或超限的问题
			{
				if (client->character.inventoryUse >= 30)
				{//缺失 Sancaros
					Send1A(L"Inventory limit reached.", client, 60);
					//client->todc = 1;
					//return item_added;
				}
				else
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
				// Delete item from game's inventory 从房间游戏清单中删除该物品
				memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
				l->gameItemList[found_item] = 0xFFFFFFFF;
				CleanUpGameInventory(l);
			}
		}
	}
	return item_added;
}

//从客户端移除美赛塔
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
		*(unsigned*)&PacketData[0x0C] = client->drop_area;
		*(long long*)&PacketData[0x10] = client->drop_coords;
		PacketData[0x18] = 0x04;
		PacketData[0x19] = 0x00;
		PacketData[0x1A] = 0x00;
		PacketData[0x1B] = 0x00;
		memset(&PacketData[0x1C], 0, 0x08);
		*(unsigned*)&PacketData[0x24] = l->playerItemID[client->clientID];
		*(unsigned*)&PacketData[0x28] = count;
		SendToLobby(client->lobby, 4, &PacketData[0], 0x2C, 0);

		// 生成新的游戏物品...

		newItemNum = free_game_item(l);
		if (l->gameItemCount < MAX_SAVED_ITEMS)
			l->gameItemList[l->gameItemCount++] = newItemNum;
		memcpy(&l->gameItem[newItemNum].item.data[0], &PacketData[0x18], 12);
		*(unsigned*)&l->gameItem[newItemNum].item.data2[0] = count;
		l->gameItem[newItemNum].item.itemid = l->playerItemID[client->clientID];
		l->playerItemID[client->clientID]++;
	}
}

//应该是直接获得物品
void SendItemToEnd(unsigned itemid, BANANA* client)
{
	unsigned ch;
	INVENTORY_ITEM i;

	for (ch = 0;ch < client->character.inventoryUse;ch++)
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
	//WriteLog("SendItemToEnd发送如下信息\n %s \n%s", client, itemid); //Sancaros Send debug的方式
}

// 从客户端的角色数据中删除物品. 未完成堆叠物品的计算
void DeleteItemFromClient(unsigned itemid, unsigned count, unsigned drop, BANANA* client)
{
	unsigned ch, ch2, itemNum;
	int found_item = -1;
	LOBBY* l;
	unsigned char stackable = 0;
	unsigned delete_item = 0;
	unsigned stack_count;


	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	if (l->challenge && l->inpquest) {
		Send1A(L"123 1", client, -1);
		for (ch = 0;ch < client->character.inventoryUse;ch++)
		{
			found_item = 1;
			delete_item = 1; // Not splitting a stack, item goes byebye from character's inventory. 不拆分堆栈，物品从角色的库存中退出。 
			if (drop) // Client dropped the item on the floor?客户把东西掉在地上了
			{
				// Copy to game's inventory 复制至游戏的物品清单客户把东西掉在地上了
				itemNum = free_game_item(l);
				Send1A(L"123 2", client, -1);
				if (l->gameItemCount < MAX_SAVED_ITEMS)
					l->gameItemList[l->gameItemCount++] = itemNum;
				Send1A(L"123 4", client, -1);
				memcpy(&l->gameItem[itemNum].item, &client->character.inventory[ch].item, sizeof(ITEM));
				Send1A(L"123 5", client, -1);
			}
			if (delete_item)
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					Send1A(L"123 6", client, -1);
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[4] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}

						Send1A(L"123 7", client, -1);
					}
				}
				client->character.inventory[ch].in_use = 0;

				Send1A(L"123 8", client, -1);
				//Send1A(L"22222 19!", client, -1);
			}
			found_item = (~ch + 1);

			Send1A(L"123 8", client, -1);
			break;
		}
		CleanUpInventory(client);
		Send1A(L"123 10", client, -1);
		return;
	}
	else {
		memset(&ch, 0, sizeof(ch));
		for (ch = 0;ch < client->character.inventoryUse;ch++)
		{
			if (client->character.inventory[ch].item.itemid == itemid) //此处尝试修复挑战模式物品掉落在地
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
					{
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
							*(unsigned*)&PacketData[0x0C] = client->drop_area;
							*(long long*)&PacketData[0x10] = client->drop_coords;
							memcpy(&PacketData[0x18], &client->character.inventory[ch].item.data[0], 12);
							PacketData[0x1D] = (unsigned char)count;
							*(unsigned*)&PacketData[0x24] = l->playerItemID[client->clientID];
							SendToLobby(client->lobby, 4, &PacketData[0], 0x28, 0);
							// 生成新的游戏物品...

							//Send1A(L"22222 18!", client, -1);
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
					delete_item = 1; // Not splitting a stack, item goes byebye from character's inventory. 不拆分堆栈，物品从角色的库存中退出。 
					if (drop) // Client dropped the item on the floor?客户把东西掉在地上了
					{
						// Copy to game's inventory 复制至游戏的物品清单客户把东西掉在地上了
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
							for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
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
					//Send1A(L"22222 19!", client, -1);
				}
				found_item = ch;
				break;
			}
		}
		//sancaros 挑战模式报错点
		if ((found_item == -1) && (!l->challenge) && (!l->battle))
		{
			Send1A(L"Could not find item to delete.", client, 62);
			//client->todc = 1;
		}
		else
			//Send1A(L"22222 20!", client, -1);
			CleanUpInventory(client);
	}
	//此处代码有误,会导致100错误
	//WriteLog("DeleteItemFromClient发送如下信息\n %s \n%s", client, itemid); //Sancaros Send debug的方式
}

//从银行中取出代码
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

	for (ch = 0;ch < client->character.bankUse;ch++)
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
			for (ch = 0;ch < client->character.inventoryUse;ch++)
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
			if (client->character.inventoryUse >= 30 && (!l->challenge) && (!l->battle))
			{//缺失 Sancaros
				Send1A(L"Inventory limit reached.", client, 60);
				client->todc = 1;
			}
			else
			{
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
			}
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
			*(unsigned*)&client->encryptbuf[0x18] = l->itemID;
			l->itemID++;
			if (!stackable)
				*(unsigned*)&client->encryptbuf[0x1C] = *(unsigned*)&client->character.bankInventory[found_item].data2[0];
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

//整理仓库物品
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
		for (ch = 0;ch < client->character.bankUse - 1;ch++)
		{
			memcpy(&b1, &client->character.bankInventory[ch], sizeof(BANK_ITEM));
			swap_c = b1.data[0];
			b1.data[0] = b1.data[2];
			b1.data[2] = swap_c;
			memcpy(&compare_item1, &b1.data[0], 3);
			for (ch2 = ch + 1;ch2 < client->character.bankUse;ch2++)
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

//存入银行代码
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

	for (ch = 0;ch < client->character.inventoryUse;ch++)
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

				if (stack_count < count && !l->challenge && (!l->battle))
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
					for (ch2 = 0;ch2 < client->character.bankUse;ch2++)
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
							for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
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

//从背包中删除代码
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
	if (l->challenge) { //尝试修复处于挑战模式下删除从背包中删除物品
		found_item = 1;
	}
	if (i->item.itemid)
		compare_id = i->item.itemid;
	for (ch = 0;ch < client->character.inventoryUse;ch++)
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
			*(unsigned*)&client->encryptbuf[0x0C] = client->character.inventory[ch].item.itemid;
			client->encryptbuf[0x10] = (unsigned char)count;

			SendToLobby(l, 4, &client->encryptbuf[0x00], 0x14, 0);

			if (delete_item)
			{
				if (client->character.inventory[ch].item.data[0] == 0x01)
				{
					if ((client->character.inventory[ch].item.data[1] == 0x01) &&
						(client->character.inventory[ch].flags & 0x08)) // equipped armor, remove slot items
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
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
	if (found_item == -1)
	{//缺失 Sancaros
		Send1A(L"Could not find item to delete from inventory.", client, 67);
		client->todc = 1;
	}
	else
		CleanUpInventory(client);

}

//增加至背包代码
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
	//将项目添加到客户的库存。。。（凭空）

	//新的itemid必须已经设置为i->item.itemid

	if (!client->lobby)
		return 0;

	l = (LOBBY*)client->lobby;

	if (i->item.data[0] == 0x04)
	{
		// Meseta
		count = *(unsigned*)&i->item.data2[0];
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
			for (ch = 0;ch < client->character.inventoryUse;ch++)
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

		if (item_added == 0) // Make sure we don't go over the max inventory确保我们没有超过最大库存
		{
			if (client->character.inventoryUse >= 30)
			{//缺失 Sancaros
				if (!l->challenge && (!l->battle)) {
					Send1A(L"Inventory limit reached.", client, 60);
					client->todc = 1;
				}
			}
			else
			{
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
			}
		}
	}

	if ((!client->todc) && (item_added))
	{
		// Let people know the client has a new toy...让人们知道该玩家获得了一个新玩具
		memset(&client->encryptbuf[0x00], 0, 0x24);
		client->encryptbuf[0x00] = 0x24;
		client->encryptbuf[0x02] = 0x60;
		client->encryptbuf[0x08] = 0xBE;
		client->encryptbuf[0x09] = 0x09;
		client->encryptbuf[0x0A] = client->clientID;
		memcpy(&client->encryptbuf[0x0C], &i->item.data[0], 12);
		*(unsigned*)&client->encryptbuf[0x18] = i->item.itemid;
		if ((!stackable) || (i->item.data[0] == 0x04))
			*(unsigned*)&client->encryptbuf[0x1C] = *(unsigned*)&i->item.data2[0];
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

//测试检测的数据是否含有反斜杠
void strd(char* a) {
	unsigned char* p = "";
	//需要的子串
	if (strstr(a, p))
		WriteLog("\n[警告]:！！ \n 数据 %p 字符串为 %s\n字节长度; %d\n", a, a, sizeof(a));
}

//舰船传输数据代码 OK 别改了
void ShipSend04(unsigned char command, BANANA* client, ORANGE* ship)
{
	//unsigned ch; //为何不做查询

	ship->encryptbuf[0x00] = 0x04;
	switch (command)
	{
	case 0x00:
		// Request character data from server 从服务器请求角色数据
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned*)&ship->encryptbuf[0x02] = client->guildcard;
		*(unsigned short*)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
		*(int*)&ship->encryptbuf[0x08] = client->plySockfd;
		*(unsigned*)&ship->encryptbuf[0x0C] = serverID;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x10);
		//WriteLog("正在向服务器请求角色数据");
		break;
	case 0x02:
		// Send character data to server when not using a temporary character. 在不使用临时角色的情况下将角色数据发送到  服务器
		if ((!client->mode) && (client->gotchardata == 1)) {
			if ((client->guildcard))
			{
				ship->encryptbuf[0x01] = 0x02;
				*(unsigned*)&ship->encryptbuf[0x02] = client->guildcard;
				*(unsigned short*)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
				memcpy(&ship->encryptbuf[0x08], &client->character.packetSize, sizeof(CHARDATA)); //定义数据包大小并加密

				// Include character bank in packet 包含角色银行数据
				memcpy(&ship->encryptbuf[0x08 + 0x700], &client->char_bank, sizeof(BANK));

				memcpy(&ship->encryptbuf[0x08 + 0x2CC0], &client->challenge_data.challengeData, 320);

				memcpy(&ship->encryptbuf[0x08 + 0x2E50], &client->battle_data.battleData, 92);

				// Include common bank in packet 包含公共银行数据
				memcpy(&ship->encryptbuf[0x08 + sizeof(CHARDATA)], &client->common_bank, sizeof(BANK));
			}
			compressShipPacket(ship, &ship->encryptbuf[0x00], sizeof(BANK) + sizeof(CHARDATA) + 8);
			//WriteLog("正在向服务器发送银行数据\n");
		}
		break;
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case 0x03:
		if ((client->mode) && (client->gotchardata == 1))
		{
			if ((client->guildcard))
			{
				ship->encryptbuf[0x01] = 0x02;
				*(unsigned*)&ship->encryptbuf[0x02] = client->guildcard;
				*(unsigned short*)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
				if (strcmp((const char*)&client->challenge_data.challengeData, (const char*)&client->character.challengeData) == -1) {
					memcpy(&client->challenge_data.challengeData[0], &client->character.challengeData, 320);
					//WriteLog("正在向服务器存储处于挑战模式中用户 %u 新的挑战数据\n", client->guildcard);
				}
				memcpy(&ship->encryptbuf[0x08 + 0x2CC0], &client->challenge_data.challengeData, 320);
				//display_packet(&ship->encryptbuf[0x08 + 0x2CC0],320);
				compressShipPacket(ship, &ship->encryptbuf[0x00], 320 + 8);
				WriteLog("用户 %u 槽位 %u 的挑战信息匹配", client->guildcard, client->slotnum);
			}
			else {
				WriteLog("用户 %u 槽位 %u 的挑战信息不匹配", client->guildcard, client->slotnum);
			}
		}
		break;
	case 0x04:
		if ((client->mode) && (client->gotchardata == 1))
		{
			if ((client->guildcard))
			{
				ship->encryptbuf[0x01] = 0x02;
				*(unsigned*)&ship->encryptbuf[0x02] = client->guildcard;
				*(unsigned short*)&ship->encryptbuf[0x06] = (unsigned short)client->slotnum;
				if (strcmp((const char*)&client->battle_data.battleData, (const char*)&client->character.battleData) == -1)
				{
					memcpy(&client->battle_data.battleData[0], &client->character.battleData, 92);
				}
				memcpy(&ship->encryptbuf[0x08 + 0x2E50], &client->battle_data.battleData, 92);
				compressShipPacket(ship, &ship->encryptbuf[0x00], 92 + 8);
			}
			else {
				WriteLog("用户 %u 槽位 %u 的挑战信息不匹配", client->guildcard, client->slotnum);
			}
		}
		break;
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
}

//客户端登录后向舰船发送数据
void ShipSend0E(ORANGE* ship)
{
	if (logon_ready) //准备登陆
	{
		ship->encryptbuf[0x00] = 0x0E;
		ship->encryptbuf[0x01] = 0x00;
		*(unsigned*)&ship->encryptbuf[0x02] = serverID;
		*(unsigned*)&ship->encryptbuf[0x06] = serverNumConnections;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
	}
}

//请求舰船列表数据
void ShipSend0D(unsigned char command, BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0D;
	switch (command)
	{
	case 0x00:
		// Requesting ship list.请求舰船列表
		ship->encryptbuf[0x01] = 0x00;
		*(int*)&ship->encryptbuf[0x02] = client->plySockfd;
		compressShipPacket(ship, &ship->encryptbuf[0x00], 6);
		break;
	default:
		printf("客户端未找到舰船列表,断开与客户端的连接.\n"); //12.25
		client->todc = 1; //12.25
		break;
	}
}

//舰船发送客户端数据至服务器
void ShipSend0B(BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x0B;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned*)&ship->encryptbuf[0x02] = *(unsigned*)&client->decryptbuf[0x0C];
	*(unsigned*)&ship->encryptbuf[0x06] = *(unsigned*)&client->decryptbuf[0x18];
	*(long long*)&ship->encryptbuf[0x0A] = *(long long*)&client->decryptbuf[0x8C];
	*(long long*)&ship->encryptbuf[0x12] = *(long long*)&client->decryptbuf[0x94];
	*(long long*)&ship->encryptbuf[0x1A] = *(long long*)&client->decryptbuf[0x9C];
	*(long long*)&ship->encryptbuf[0x22] = *(long long*)&client->decryptbuf[0xA4];
	*(long long*)&ship->encryptbuf[0x2A] = *(long long*)&client->decryptbuf[0xAC];
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x32);
}

//修复数据错误的物品代码
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

//大厅名称信息
const wchar_t lobbyString[5] = { L"大厅 \0" };

//登陆服务器传输数据代码
void LogonProcessPacket(ORANGE* ship)
{
	unsigned gcn, ch, ch2, connectNum;
	unsigned char episode, part;
	unsigned mob_rate;
	long long mob_calc;

	switch (ship->decryptbuf[0x04])
	{
	case 0x00:
		// Server has sent it's welcome packet.  Start encryption and send ship info...服务器已经发送了它的欢迎数据包,启动加密并发送舰船信息
		memcpy(&ship->user_key[0], &RC4publicKey[0], 32);
		ch2 = 0;
		for (ch = 0x1C;ch < 0x5C;ch += 2)
		{
			ship->key_change[ch2 + (ship->decryptbuf[ch] % 4)] = ship->decryptbuf[ch + 1];
			ch2 += 4;
		}
		prepare_key(&ship->user_key[0], 32, &ship->cs_key);
		prepare_key(&ship->user_key[0], 32, &ship->sc_key);
		ship->crypt_on = 1;
		memcpy(&ship->encryptbuf[0x00], &ship->decryptbuf[0x04], 0x28);
		memcpy(&ship->encryptbuf[0x00], &ShipPacket00[0x00], 0x10); // Yep! :) 这里涉及到登陆服务器发给
		ship->encryptbuf[0x00] = 1;
		memcpy(&ship->encryptbuf[0x28], &Ship_Name[0], 12); //中文汉化
															//memcpy(&ship->encryptbuf[0x28], &Ship_Name[0], 12); //原代码
		*(unsigned*)&ship->encryptbuf[0x34] = serverNumConnections;
		*(unsigned*)&ship->encryptbuf[0x38] = *(unsigned*)&serverIP[0];
		*(unsigned short*)&ship->encryptbuf[0x3C] = (unsigned short)serverPort;
		*(unsigned*)&ship->encryptbuf[0x3E] = shop_checksum;
		*(unsigned*)&ship->encryptbuf[0x42] = ship_index;
		memcpy(&ship->encryptbuf[0x46], &ship_key[0], 32);
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x66);
		break;
	case 0x02:
		// Server's result of our authentication packet. 服务器的身份验证数据包的结果 
		if (ship->decryptbuf[0x05] != 0x01)
		{
			switch (ship->decryptbuf[0x05])
			{
			case 0x00:
				printf("此舰船服务器的版本与登录服务器不兼容.\n");
				printf("按下 [回车键] 退出");
				reveal_window;
				gets_s(&dp[0], 0);
				exit(1);
				break;
			case 0x02:
				printf("此舰船服务器的IP地址已在登录服务器上注册.\n");
				printf("IP地址不能注册两次.  %u 秒后重试...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x03:
				printf("这艘船没有通过登录服务器在其上运行的连接测试.\n");
				printf("请确保 ship.ini 中指定的IP地址是正确的\n");
				printf("你的防火墙在允许 ship_serve.exe 产生连接的情况下\n");
				printf("请确保您的端口被转发.  %u 秒后重试...\n", LOGIN_RECONNECT_SECONDS);
				reveal_window;
				break;
			case 0x04:
				printf("连接到此登录服务器时\n");
				printf("请不要修改任何未经指示的数据...\n");
				printf("按下 [回车键] 退出");
				reveal_window;
				gets_s(&dp[0], 0);
				exit(1);
				break;
			case 0x05:
				printf("ship_key.bin 文件无效.\n");
				printf("按下 [回车键] 退出");
				reveal_window;
				gets_s(&dp[0], 0);
				exit(1);
				break;
			case 0x06:
				printf("舰船密匙似乎已在使用中!\n");
				printf("按下 [回车键] 退出");
				reveal_window;
				gets_s(&dp[0], 0);
				exit(1);
				break;
			}
			initialize_logon();
		}
		else
		{
			serverID = *(unsigned*)&ship->decryptbuf[0x06];
			if (serverIP[0] == 0x00)
			{
				*(unsigned*)&serverIP[0] = *(unsigned*)&ship->decryptbuf[0x0A];
				printf("更新IP地址为 %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
			}
			serverID++;
			if (serverID != 0xFFFFFFFF)
			{
				printf("舰船已成功注册到登录服务器!!! 舰船 ID: %u\n", serverID);
				printf("构建舰仓列表数据包...\n\n");
				ConstructBlockPacket();
				printf("正在载入任务物品奖励...\n");
				quest_numallows = *(unsigned*)&ship->decryptbuf[0x0E];
				if (quest_allow)
					free(quest_allow);

				quest_allow = malloc(quest_numallows * 4);
				if (quest_allow <= 0) {
					printf("内存分配不成功！\n");
					return;
				}
				else {
					memcpy(quest_allow, &ship->decryptbuf[0x12], quest_numallows * 4);
				}
				printf("任务补助物品数量: %u\n\n", quest_numallows);
				normalName = *(unsigned*)&ship->decryptbuf[0x12 + (quest_numallows * 4)];
				localName = *(unsigned*)&ship->decryptbuf[0x16 + (quest_numallows * 4)];
				globalName = *(unsigned*)&ship->decryptbuf[0x1A + (quest_numallows * 4)];
				memcpy(&ship->user_key[0], &ship_key[0], 128); // 1024-bit key

															   // Change keys

				for (ch2 = 0;ch2 < 128;ch2++)
					if (ship->key_change[ch2] != -1)
						ship->user_key[ch2] = (unsigned char)ship->key_change[ch2]; // update the key

				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->cs_key);
				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->sc_key);
				memset(&ship->encryptbuf[0x00], 0, 8);
				ship->encryptbuf[0x00] = 0x0F;	//15
				ship->encryptbuf[0x01] = 0x00;  //0
				printf("从服务器请求稀有掉落数据表...\n");
				compressShipPacket(ship, &ship->encryptbuf[0x00], 4);
			}
			else
			{
				printf("该舰船未能对登录服务器进行身份验证.  将在 %u 秒后重试...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon();
			}
		}
		break;
	case 0x03:
		// Reserved 此处做了一个字节保留 sancaros
		printf("检测LogonProcessPacket 0x03的作用在哪里");
		break;
	case 0x04:
		switch (ship->decryptbuf[0x05])
		{
		case 0x01:
		{
			// Receive and store full player data here.在这里接收并存储完整的玩家数据
			//
			BANANA* client;
			unsigned guildcard, ch, ch2, eq_weapon, eq_armor, eq_shield, eq_mag;
			int sockfd;
			unsigned short baseATP, baseMST, baseEVP, baseHP, baseDFP, baseATA;
			unsigned char* cd;
			//unsigned char quest_data1;
			//unsigned char challengeData;

			guildcard = *(unsigned*)&ship->decryptbuf[0x06];
			sockfd = *(int*)&ship->decryptbuf[0x0C];
			//WriteLog("这里正在接收完整的角色数据");
			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->plySockfd == sockfd) && (connections[connectNum]->guildcard == guildcard))
				{
					client = connections[connectNum];
					client->gotchardata = 1;
					memcpy(&client->character.packetSize, &ship->decryptbuf[0x10], sizeof(CHARDATA));

					/* Set up copies of the banks 复制一份银行仓库的副本 */

					memcpy(&client->char_bank, &client->character.bankUse, sizeof(BANK));

					memcpy(&client->common_bank, &ship->decryptbuf[0x10 + sizeof(CHARDATA)], sizeof(BANK));


					//复制一份挑战数据 对战数据的副本以作备用 1.2

					memcpy(&client->challenge_data.challengeData, &client->character.challengeData, 320);

					memcpy(&client->battle_data.battleData, &client->character.battleData, 92);

					cipher_ptr = &client->server_cipher;
					if (client->isgm == 1)
						*(unsigned*)&client->character.nameColorBlue = globalName;
					else
						if (isLocalGM(client->guildcard))
							*(unsigned*)&client->character.nameColorBlue = localName;
						else
							*(unsigned*)&client->character.nameColorBlue = normalName;

					if (client->character.inventoryUse > 30)
						client->character.inventoryUse = 30;

					client->equip_flags = 0;
					switch (client->character._class)
					{
					case CLASS_HUMAR:
						client->equip_flags |= HUNTER_FLAG;
						client->equip_flags |= HUMAN_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					case CLASS_HUNEWEARL:
						client->equip_flags |= HUNTER_FLAG;
						client->equip_flags |= NEWMAN_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_HUCAST:
						client->equip_flags |= HUNTER_FLAG;
						client->equip_flags |= DROID_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					case CLASS_HUCASEAL:
						client->equip_flags |= HUNTER_FLAG;
						client->equip_flags |= DROID_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_RAMAR:
						client->equip_flags |= RANGER_FLAG;
						client->equip_flags |= HUMAN_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					case CLASS_RACAST:
						client->equip_flags |= RANGER_FLAG;
						client->equip_flags |= DROID_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					case CLASS_RACASEAL:
						client->equip_flags |= RANGER_FLAG;
						client->equip_flags |= DROID_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_RAMARL:
						client->equip_flags |= RANGER_FLAG;
						client->equip_flags |= HUMAN_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_FONEWM:
						client->equip_flags |= FORCE_FLAG;
						client->equip_flags |= NEWMAN_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					case CLASS_FONEWEARL:
						client->equip_flags |= FORCE_FLAG;
						client->equip_flags |= NEWMAN_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_FOMARL:
						client->equip_flags |= FORCE_FLAG;
						client->equip_flags |= HUMAN_FLAG;
						client->equip_flags |= FEMALE_FLAG;
						break;
					case CLASS_FOMAR:
						client->equip_flags |= FORCE_FLAG;
						client->equip_flags |= HUMAN_FLAG;
						client->equip_flags |= MALE_FLAG;
						break;
					}

					// Let's fix hacked mags and weapons 让我来修复作弊玛古和武器

					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					{
						if (client->character.inventory[ch2].in_use)
							FixItem(&client->character.inventory[ch2].item);
					}

					// Fix up equipped weapon, armor, shield, and mag equipment information 修复武器,装甲,盾,还有玛古装备信息

					eq_weapon = 0;
					eq_armor = 0;
					eq_shield = 0;
					eq_mag = 0;

					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					{
						if (client->character.inventory[ch2].flags & 0x08)
						{
							switch (client->character.inventory[ch2].item.data[0])
							{
							case 0x00:
								eq_weapon++;
								break;
							case 0x01:
								switch (client->character.inventory[ch2].item.data[1])
								{
								case 0x01:
									eq_armor++;
									break;
								case 0x02:
									eq_shield++;
									break;
								}
								break;
							case 0x02:
								eq_mag++;
								break;
							}
						}
					}

					if (eq_weapon > 1)
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						{
							// Unequip all weapons when there is more than one equipped.  当装备了多个武器时,取消所装备武器
							if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
								(client->character.inventory[ch2].flags & 0x08))
								client->character.inventory[ch2].flags &= ~(0x08);
						}

					}

					if (eq_armor > 1)
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						{
							// Unequip all armor and slot items when there is more than one armor equipped. 当装备了多个护甲时，取消装备所有护甲和槽道具。 
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[3] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
						}
					}

					if (eq_shield > 1)
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						{
							// Unequip all shields when there is more than one equipped. 当装备了多个护盾时，取消装备所有护盾。 
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].item.data[3] = 0x00;
								client->character.inventory[ch2].flags &= ~(0x08);
							}
						}
					}

					if (eq_mag > 1)
					{
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						{
							// Unequip all mags when there is more than one equipped. 当装备了多个玛古时，取消装备所有玛古。 
							if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
								client->character.inventory[ch2].flags &= ~(0x08);
						}
					}

					for (ch2 = 0;ch2 < client->character.bankUse;ch2++)
						FixItem((ITEM*)&client->character.bankInventory[ch2]);

					baseATP = *(unsigned short*)&startingData[(client->character._class * 14)];
					baseMST = *(unsigned short*)&startingData[(client->character._class * 14) + 2];
					baseEVP = *(unsigned short*)&startingData[(client->character._class * 14) + 4];
					baseHP = *(unsigned short*)&startingData[(client->character._class * 14) + 6];
					baseDFP = *(unsigned short*)&startingData[(client->character._class * 14) + 8];
					baseATA = *(unsigned short*)&startingData[(client->character._class * 14) + 10];

					for (ch2 = 0;ch2 < client->character.level;ch2++)
					{
						baseATP += playerLevelData[client->character._class][ch2].ATP;
						baseMST += playerLevelData[client->character._class][ch2].MST;
						baseEVP += playerLevelData[client->character._class][ch2].EVP;
						baseHP += playerLevelData[client->character._class][ch2].HP;
						baseDFP += playerLevelData[client->character._class][ch2].DFP;
						baseATA += playerLevelData[client->character._class][ch2].ATA;
					}

					client->matuse[0] = (client->character.ATP - baseATP) / 2;
					client->matuse[1] = (client->character.MST - baseMST) / 2;
					client->matuse[2] = (client->character.EVP - baseEVP) / 2;
					client->matuse[3] = (client->character.DFP - baseDFP) / 2;
					client->matuse[4] = (client->character.LCK - 10) / 2;

					client->character.lang = 0x00; //语言？

					cd = (unsigned char*)&client->character.packetSize;

					cd[(8 * 28) + 0x0F] = client->matuse[0];
					cd[(9 * 28) + 0x0F] = client->matuse[1];
					cd[(10 * 28) + 0x0F] = client->matuse[2];
					cd[(11 * 28) + 0x0F] = client->matuse[3];
					cd[(12 * 28) + 0x0F] = client->matuse[4];


					//memcpy(&client->character.challengeData, &client->challenge_data.challengeData, sizeof(CHALLENGEDATA));

					//memcpy(&client->character.battleData, &client->battle_data.battleData, sizeof(BATTLEDATA));

					encryptcopy(client, (unsigned char*)&client->character.packetSize, sizeof(CHARDATA));
					client->preferred_lobby = 0xFF;

					cd[(8 * 28) + 0x0F] = 0x00; // Clear this stuff out to not mess up our item procedures. 把这些东西清理干净，以免弄乱我们的物品处理程序。 
					cd[(9 * 28) + 0x0F] = 0x00;
					cd[(10 * 28) + 0x0F] = 0x00;
					cd[(11 * 28) + 0x0F] = 0x00;
					cd[(12 * 28) + 0x0F] = 0x00;

					for (ch2 = 0;ch2 < MAX_SAVED_LOBBIES;ch2++)
					{
						if (savedlobbies[ch2].guildcard == client->guildcard)
						{
							client->preferred_lobby = savedlobbies[ch2].lobby - 1;
							savedlobbies[ch2].guildcard = 0;
							break;
						}
					}

					Send95(client);

					if ((client->isgm) || (isLocalGM(client->guildcard)))
						WriteGM("管理员 %u (%s) 已连接", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
					else
						WriteLog("玩家 %u (%s) 已连接", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
					break;
				}
			}
		}
		break;
		case 0x03:
		{
			unsigned guildcard;
			BANANA* client;

			guildcard = *(unsigned*)&ship->decryptbuf[0x06];

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard == guildcard) && (connections[connectNum]->released == 1))
				{
					// Let the released client roam free...!让释放的客户免费漫游
					client = connections[connectNum];
					Send19(client->releaseIP[0], client->releaseIP[1], client->releaseIP[2], client->releaseIP[3],
						client->releasePort, client);
					break;
				}
			}
		}
		}
		break;
	case 0x05:
		// Reserved 这里又做了功能保留 sancaros
		printf("未知0x05指令用途 ~~~~~~~~~~~~~~");
		break;
	case 0x06:
		// Reserved 这里又做了功能保留 sancaros
		printf("未知0x06指令用途 ~~~~~~~~~~~~~~");
		break;
	case 0x07:
		// Card database full.卡片数据库已满
		gcn = *(unsigned*)&ship->decryptbuf[0x06];

		for (ch = 0;ch < serverNumConnections;ch++)
		{
			connectNum = serverConnectionList[ch];
			if (connections[connectNum]->guildcard == gcn)
			{
				Send1A(L"Your guild card database on the server is full.\n\nYou were unable to accept the guild card.\n\nPlease delete some cards.  (40 max)", connections[connectNum], 68);
				break;
			}
		}
		break;
	case 0x08:
		//用户在线时被其他客户端登陆时的处理程序
		switch (ship->decryptbuf[0x05])
		{
		case 0x00:
		{
			gcn = *(unsigned*)&ship->decryptbuf[0x06];
			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					Send1A(L"This account has just logged on.\n\nYou are now being disconnected.", connections[connectNum], 69);
					connections[connectNum]->todc = 1;
					break;
				}
			}
		}
		break;
		case 0x01:
		{
			// Someone's doing a guild card search...   Check to see if that guild card is on our ship... 有人在搜索公会卡。。。检查一下公会卡是否在我们船上。。。 

			unsigned client_gcn, ch2;
			unsigned char* n;
			unsigned char* c;
			unsigned short blockPort;

			gcn = *(unsigned*)&ship->decryptbuf[0x06];
			client_gcn = *(unsigned*)&ship->decryptbuf[0x0A];

			// requesting ship ID @ 0x0E 请求ship ID号

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard == gcn) && (connections[connectNum]->lobbyNum))
				{
					if (connections[connectNum]->lobbyNum < 0x10)
						for (ch2 = 0;ch2 < MAX_SAVED_LOBBIES;ch2++)
						{
							if (savedlobbies[ch2].guildcard == 0)
							{
								savedlobbies[ch2].guildcard = client_gcn;
								savedlobbies[ch2].lobby = connections[connectNum]->lobbyNum;
								break;
							}
						}
					ship->encryptbuf[0x00] = 0x08;
					ship->encryptbuf[0x01] = 0x02;
					*(unsigned*)&ship->encryptbuf[0x02] = serverID;
					*(unsigned*)&ship->encryptbuf[0x06] = *(unsigned*)&ship->decryptbuf[0x0E];
					// 0x10 = 41 result packet
					memset(&ship->encryptbuf[0x0A], 0, 0x136);
					ship->encryptbuf[0x10] = 0x30;
					ship->encryptbuf[0x11] = 0x01;
					ship->encryptbuf[0x12] = 0x41;
					ship->encryptbuf[0x1A] = 0x01;
					*(unsigned*)&ship->encryptbuf[0x1C] = client_gcn;
					*(unsigned*)&ship->encryptbuf[0x20] = gcn;
					ship->encryptbuf[0x24] = 0x10;
					ship->encryptbuf[0x26] = 0x19;
					*(unsigned*)&ship->encryptbuf[0x2C] = *(unsigned*)&serverIP[0];
					blockPort = serverPort + connections[connectNum]->block;
					*(unsigned short*)&ship->encryptbuf[0x30] = (unsigned short)blockPort;
					memcpy(&ship->encryptbuf[0x34], (unsigned char*)&lobbyString[0], 12);
					if (connections[connectNum]->lobbyNum < 0x10)
					{
						if (connections[connectNum]->lobbyNum < 10)
						{
							ship->encryptbuf[0x40] = 0x30;
							ship->encryptbuf[0x42] = 0x30 + connections[connectNum]->lobbyNum;
						}
						else
						{
							ship->encryptbuf[0x40] = 0x31;
							ship->encryptbuf[0x42] = 0x26 + connections[connectNum]->lobbyNum;
						}
					}
					else
					{
						ship->encryptbuf[0x40] = 0x30;
						ship->encryptbuf[0x42] = 0x31;
					}
					ship->encryptbuf[0x44] = 0x2C;
					memcpy(&ship->encryptbuf[0x46], &blockString[0], 10);
					if (connections[connectNum]->block < 10)
					{
						ship->encryptbuf[0x50] = 0x30;
						ship->encryptbuf[0x52] = 0x30 + connections[connectNum]->block;
					}
					else
					{
						ship->encryptbuf[0x50] = 0x31;
						ship->encryptbuf[0x52] = 0x26 + connections[connectNum]->block;
					}

					ship->encryptbuf[0x54] = 0x2C;
					if (serverID < 10)
					{
						ship->encryptbuf[0x56] = 0x30;
						ship->encryptbuf[0x58] = 0x30 + serverID;
					}
					else
					{
						ship->encryptbuf[0x56] = 0x30 + (serverID / 10);
						ship->encryptbuf[0x58] = 0x30 + (serverID % 10);
					}
					ship->encryptbuf[0x5A] = 0x3A;
					n = (unsigned char*)&ship->encryptbuf[0x5C];
					c = (unsigned char*)&Ship_Name[0];
					while (*c != 0x00)
					{
						*(n++) = *(c++);
						n++;
					}
					if (connections[connectNum]->lobbyNum < 0x10)
						ship->encryptbuf[0xBC] = (unsigned char)connections[connectNum]->lobbyNum; else
						ship->encryptbuf[0xBC] = 0x01;
					ship->encryptbuf[0xBE] = 0x1A;
					memcpy(&ship->encryptbuf[0x100], &connections[connectNum]->character.name[0], 24);
					compressShipPacket(ship, &ship->encryptbuf[0x00], 0x140);
					break;
				}
			}
		}
		break;
		case 0x02:
			// Send guild result to user发送公会信息给用户
		{
			gcn = *(unsigned*)&ship->decryptbuf[0x20];

			// requesting ship ID @ 0x0E 获取SHIP ID号

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					cipher_ptr = &connections[connectNum]->server_cipher;
					encryptcopy(connections[connectNum], &ship->decryptbuf[0x14], 0x130);
					break;
				}
			}
		}
		break;
		case 0x03:
			// Send mail to user发送邮件给用户
		{
			gcn = *(unsigned*)&ship->decryptbuf[0x36];

			// requesting ship ID @ 0x0E获取SHIP ID号

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					cipher_ptr = &connections[connectNum]->server_cipher;
					encryptcopy(connections[connectNum], &ship->decryptbuf[0x06], 0x45C);
					break;
				}
			}
		}
		break;
		default:
			break;
		}
		break;
	case 0x09:
		// Reserved for team functions.应该就是缺少的团队功能了
		switch (ship->decryptbuf[0x05])
		{
			BANANA* client;
			unsigned char CreateResult;

		case 0x00:
			CreateResult = ship->decryptbuf[0x06];
			gcn = *(unsigned*)&ship->decryptbuf[0x07];
			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					client = connections[connectNum];
					switch (CreateResult)
					{
					case 0x00:
						// All good!!! 一切运行良好
						client->character.teamID = *(unsigned*)&ship->decryptbuf[0x823];
						memcpy(&client->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						client->character.privilegeLevel = 0x40;
						client->character.teamRank = 0x00986C84; // ??这段不懂 公会排行榜
						client->character.teamName[0] = 0x09;
						client->character.teamName[2] = 0x45;
						client->character.privilegeLevel = 0x40;
						memcpy(&client->character.teamName[4], &ship->decryptbuf[0x80B], 24);
						SendEA(0x02, client);
						SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, 0);
						SendEA(0x12, client);
						SendEA(0x1D, client);
						break;
					case 0x01:
						Send1A(L"The server failed to create the team due to a MySQL error.\n\nPlease contact the server administrator.", client, 70);
						break;
					case 0x02:
						Send01(L"Cannot create team\nbecause team\n already exists!!!", client, 71);
						break;
					case 0x03:
						Send01(L"Cannot create team\nbecause you are\nalready in a team!", client, 72);
						break;
					}
					break;
				}
			}
			break;
		case 0x01:
			// Flag updated 标志已更新
		{
			unsigned teamid;
			BANANA* tClient;

			teamid = *(unsigned*)&ship->decryptbuf[0x07];

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
				{
					tClient = connections[connectNum];
					memcpy(&tClient->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
					SendToLobby(tClient->lobby, 12, MakePacketEA15(tClient), 2152, 0);
				}
			}
		}
		break;
		case 0x02:
			// Team dissolved 队伍解散了
		{
			unsigned teamid;
			BANANA* tClient;

			teamid = *(unsigned*)&ship->decryptbuf[0x07];

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
				{
					tClient = connections[connectNum];
					memset(&tClient->character.serial_number, 0, 2108);
					SendToLobby(tClient->lobby, 12, MakePacketEA15(tClient), 2152, 0);
					SendEA(0x12, tClient);
				}
			}
		}
		break;
		case 0x04:
			// Team chat 队伍聊天
		{
			unsigned teamid, size;
			BANANA* tClient;

			size = *(unsigned*)&ship->decryptbuf[0x00];
			size -= 10;

			teamid = *(unsigned*)&ship->decryptbuf[0x06];

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
				{
					tClient = connections[connectNum];
					cipher_ptr = &tClient->server_cipher;
					encryptcopy(tClient, &ship->decryptbuf[0x0A], size);
				}
			}
		}
		break;
		case 0x05:
			// Request Team List 队伍列表
		{
			unsigned gcn;
			unsigned short size;
			BANANA* tClient;

			gcn = *(unsigned*)&ship->decryptbuf[0x0A];
			size = *(unsigned short*)&ship->decryptbuf[0x0E];

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					tClient = connections[connectNum];
					cipher_ptr = &tClient->server_cipher;
					encryptcopy(tClient, &ship->decryptbuf[0x0E], size);
					break;
				}
			}
		}
		break;
		}
		break;
	case 0x0A:
		// Reserved 功能保留
		break;
	case 0x0B:
		// Player authentication result from the logon server.
		gcn = *(unsigned*)&ship->decryptbuf[0x06];
		if (ship->decryptbuf[0x05] == 0)
		{
			BANANA* client;

			// Finish up the logon process here. 在这里完成登录过程 

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					client = connections[connectNum];
					client->slotnum = ship->decryptbuf[0x0A];
					client->isgm = ship->decryptbuf[0x0B];
					memcpy(&client->encryptbuf[0], &PacketE6[0], sizeof(PacketE6));
					*(unsigned*)&client->encryptbuf[0x10] = gcn;
					client->guildcard = gcn;
					*(unsigned*)&client->encryptbuf[0x14] = *(unsigned*)&ship->decryptbuf[0x0C];
					*(long long*)&client->encryptbuf[0x38] = *(long long*)&ship->decryptbuf[0x10];
					if (client->decryptbuf[0x16] < 0x05)
					{
						//网络不同步造成断线
						Send1A(L"Client/Server synchronization error.", client, 73);
						client->todc = 1;
					}
					else
					{
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &client->encryptbuf[0], sizeof(PacketE6));
						client->lastTick = (unsigned)servertime;
						if (client->block == 0)
						{
							if (logon->sockfd >= 0)
								Send07(client);
							else
							{
								Send1A(L"This ship has unfortunately lost it's connection with the logon server...\nData cannot be saved.\n\nPlease reconnect later.", client, 74);
								client->todc = 1;
							}
						}
						else
						{
							blocks[client->block - 1]->count++;
							// Request E7 information from server...从服务器中请求E7指令信息
							Send83(client); // Lobby data 大厅数据
							ShipSend04(0x00, client, logon);
							//printf("这里是不是用来登陆的时候触发的");
						}
					}
					break;
				}
			}
		}
		else
		{
			// Deny connection here. 拒绝连接
			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					Send1A(L"Security violation.", connections[connectNum], 75);
					connections[connectNum]->todc = 1;
					break;
				}
			}
		}
		break;
	case 0x0D:
		// 00 = Request ship list
		// 01 = Ship list data (include IP addresses)
		switch (ship->decryptbuf[0x05])
		{
		case 0x01:
		{
			unsigned char ch;
			int sockfd;
			unsigned short pOffset;

			// Retrieved ship list data.  Send to client...检索舰船列表数据,发送给客户端

			sockfd = *(int*)&ship->decryptbuf[0x06];
			pOffset = *(unsigned short*)&ship->decryptbuf[0x0A];
			memcpy(&PacketA0Data[0x00], &ship->decryptbuf[0x0A], pOffset);
			pOffset += 0x0A;

			totalShips = 0;

			for (ch = 0;ch < PacketA0Data[0x04];ch++)
			{
				shipdata[ch].shipID = *(unsigned*)&ship->decryptbuf[pOffset];
				pOffset += 4;
				*(unsigned*)&shipdata[ch].ipaddr[0] = *(unsigned*)&ship->decryptbuf[pOffset];
				pOffset += 4;
				shipdata[ch].port = *(unsigned short*)&ship->decryptbuf[pOffset];
				pOffset += 2;
				totalShips++;
			}

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->plySockfd == sockfd)
				{
					SendA0(connections[connectNum]);
					break;
				}
			}
		}
		break;
		default:
			break;
		}
		break;
	case 0x0F:
		// Receiving drop chart
		episode = ship->decryptbuf[0x05];
		part = ship->decryptbuf[0x06];
		if (ship->decryptbuf[0x06] == 0)
			printf("从登录服务器接收稀有掉落数据表...\n");
		switch (episode)
		{
		case 0x01:
			if (part == 0)
				printf("章节 I ...");
			else
				printf(" 确认!\n");
			memcpy(&rt_tables_ep1[(sizeof(rt_tables_ep1) >> 3) * part], &ship->decryptbuf[0x07], sizeof(rt_tables_ep1) >> 1);
			break;
		case 0x02:
			if (part == 0)
				printf("章节 II ...");
			else
				printf(" 确认!\n");
			memcpy(&rt_tables_ep2[(sizeof(rt_tables_ep2) >> 3) * part], &ship->decryptbuf[0x07], sizeof(rt_tables_ep2) >> 1);
			break;
		case 0x03:
			if (part == 0)
				printf("章节 IV ...");
			else
				printf(" 确认!\n");
			memcpy(&rt_tables_ep4[(sizeof(rt_tables_ep4) >> 3) * part], &ship->decryptbuf[0x07], sizeof(rt_tables_ep4) >> 1);
			break;
		}
		*(unsigned*)&ship->encryptbuf[0x00] = *(unsigned*)&ship->decryptbuf[0x04];
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x04);
		break;
	case 0x10:
		// Monster appearance rates
		printf("\n从服务器收到稀有怪物出现倍率...\n");
		for (ch = 0;ch < 8;ch++)
		{
			mob_rate = *(unsigned*)&ship->decryptbuf[0x06 + (ch * 4)];
			mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
			//此处有个稀有怪物随机概率计算
			/*
			times_won = 0;
			for (ch2=0;ch2<1000000;ch2++)
			{
			if (mt_lrand() < mob_calc)
			times_won++;
			}
			*/
			switch (ch)
			{
			case 0x00:
				printf("狂暴巨猿出现倍率: %3f%%\n", (float)mob_rate / 1000);
				hildebear_rate = (unsigned)mob_calc;
				break;
			case 0x01:
				printf("拉比出现倍率: %3f%%\n", (float)mob_rate / 1000);
				rappy_rate = (unsigned)mob_calc;
				break;
			case 0x02:
				printf("Lily出现倍率: %3f%%\n", (float)mob_rate / 1000);
				lily_rate = (unsigned)mob_calc;
				break;
			case 0x03:
				printf("Pouilly Slime出现倍率: %3f%%\n", (float)mob_rate / 1000);
				slime_rate = (unsigned)mob_calc;
				break;
			case 0x04:
				printf("Merissa AA出现倍率: %3f%%\n", (float)mob_rate / 1000);
				merissa_rate = (unsigned)mob_calc;
				break;
			case 0x05:
				printf("Pazuzu出现倍率: %3f%%\n", (float)mob_rate / 1000);
				pazuzu_rate = (unsigned)mob_calc;
				break;
			case 0x06:
				printf("Dorphon Eclair出现倍率: %3f%%\n", (float)mob_rate / 1000);
				dorphon_rate = (unsigned)mob_calc;
				break;
			case 0x07:
				printf("Kondrieu出现倍率: %3f%%\n", (float)mob_rate / 1000);
				kondrieu_rate = (unsigned)mob_calc;
				break;
			}
			//debug ("Actual rate: %3f%%\n", ((float) times_won / 1000000) * 100);
		}
		printf("\n舰船已准备完毕,可以接收玩家登船...\n");
		logon_ready = 1;
		break;
	case 0x11:
		// Ping received 收到ping请求
		ship->last_ping = (unsigned)servertime;
		*(unsigned*)&ship->encryptbuf[0x00] = *(unsigned*)&ship->decryptbuf[0x04];
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x04);
		break;
	case 0x12:
		// Global announce 全服公告
		gcn = *(unsigned*)&ship->decryptbuf[0x06];
		GlobalBroadcast((unsigned short*)&ship->decryptbuf[0x0A]);
		WriteGM("GM %u 发布全服公告: %s", gcn, Unicode_to_ASCII((unsigned short*)&ship->decryptbuf[0x0A])); //此处需要汉化
		break;
	default:
		// Unknown 未知
		break;
	}
}

//玛古同步概率的代码
void AddPB(unsigned char* flags, unsigned char* blasts, unsigned char pb)
{
	int pb_exists = 0;
	unsigned char pbv;
	unsigned pb_slot;

	if ((*flags & 0x01) == 0x01)
	{
		if ((*blasts & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x02) == 0x02)
	{
		if (((*blasts / 8) & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x04) == 0x04)
		pb_exists = 1;

	if (!pb_exists)
	{
		if ((*flags & 0x01) == 0)
			pb_slot = 0;
		else
			if ((*flags & 0x02) == 0)
				pb_slot = 1;
			else
				pb_slot = 2;
		switch (pb_slot)
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ((*blasts & 0x07) < pb)
				pbv--;
			if (((*blasts / 8) & 0x07) < pb)
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags |= 0x04;
		}
		*blasts |= pb;
	}
}

//玛古的代码

int MagAlignment(MAG* m)
{
	int v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->power;
	v2 = m->dex;
	v1 = m->mind;
	if (v2 < v3)
	{
		if (v1 < v3)
			v4 = 8;
	}
	if (v3 < v2)
	{
		if (v1 < v2)
			v4 |= 0x10u;
	}
	if (v2 < v1)
	{
		if (v3 < v1)
			v4 |= 0x20u;
	}
	v6 = 0;
	v5 = v3;
	if (v3 <= v2)
		v5 = v2;
	if (v5 <= v1)
		v5 = v1;
	if (v5 == v3)
		v6 = 1;
	if (v5 == v2)
		++v6;
	if (v5 == v1)
		++v6;
	if (v6 >= 2)
		v4 |= 0x100u;
	return v4;
}

int MagSpecialEvolution(MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass)
{
	unsigned char oldType;
	short mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->defense / 100;
		mPower = m->power / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch (sectionID)
		{
		case ID_Viridia:
		case ID_Bluefull:
		case ID_Redria:
		case ID_Whitill:
			if ((mDefense + mDex) == (mPower + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Skyly:
		case ID_Pinkal:
		case ID_Yellowboze:
			if ((mDefense + mPower) == (mDex + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Greennill:
		case ID_Oran:
		case ID_Purplenum:
			if ((mDefense + mMind) == (mPower + mDex))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int)(oldType != m->mtype);
}

void MagLV50Evolution(MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass)
{
	int v10, v11, v12, v13;

	int Alignment = MagAlignment(m);

	if (EvolutionClass > 3) // Don't bother to check if a special mag.
		return;

	v10 = m->power / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->defense / 100;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			if (sectionID & 1)
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Apsaras;
					AddPB(&m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB(&m->PBflags, &m->blasts, PB_Pilla);
				}
			}
			else
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Bhirava;
					AddPB(&m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB(&m->PBflags, &m->blasts, PB_Golla);
				}
			}
		}
		else
		{
			if (Alignment & 0x10)
			{
				if (sectionID & 1)
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Garuda;
						AddPB(&m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						AddPB(&m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Ila;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						AddPB(&m->PBflags, &m->blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (sectionID & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Soma;
							AddPB(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							AddPB(&m->PBflags, &m->blasts, PB_Estlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Ushasu;
							AddPB(&m->PBflags, &m->blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110)
		{
			if (sectionID & 1)
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Kaitabha;
					AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					AddPB(&m->PBflags, &m->blasts, PB_Golla);
				}
			}
			else
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Bhirava;
					AddPB(&m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					AddPB(&m->PBflags, &m->blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (sectionID & 1)
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Kaitabha;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Bhirava;
						AddPB(&m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						AddPB(&m->PBflags, &m->blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (sectionID & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Durga;
							AddPB(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Apsaras;
							AddPB(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							AddPB(&m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120)
		{
			if (v13 > 44)
			{
				m->mtype = Mag_Bana;
				AddPB(&m->PBflags, &m->blasts, PB_Estlla);
			}
			else
			{
				if (sectionID & 1)
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Ila;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						AddPB(&m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Kabanda;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (v13 > 44)
				{
					m->mtype = Mag_Andhaka;
					AddPB(&m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					if (sectionID & 1)
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Naga;
							AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							AddPB(&m->PBflags, &m->blasts, PB_Pilla);
						}
					}
					else
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Ravana;
							AddPB(&m->PBflags, &m->blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							AddPB(&m->PBflags, &m->blasts, PB_Golla);
						}
					}
				}
			}
			else
			{
				if (Alignment & 0x10)
				{
					if (v13 > 44)
					{
						m->mtype = Mag_Bana;
						AddPB(&m->PBflags, &m->blasts, PB_Estlla);
					}
					else
					{
						if (sectionID & 1)
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Garuda;
								AddPB(&m->PBflags, &m->blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								AddPB(&m->PBflags, &m->blasts, PB_Pilla);
							}
						}
						else
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Ribhava;
								AddPB(&m->PBflags, &m->blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								AddPB(&m->PBflags, &m->blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

void MagLV35Evolution(MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass)
{
	int Alignment = MagAlignment(m);

	if (EvolutionClass > 3) // Don't bother to check if a special mag.
		return;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			m->mtype = Mag_Rudra;
			AddPB(&m->PBflags, &m->blasts, PB_Golla);
			return;
		}
		else
		{
			if (Alignment & 0x10)
			{
				m->mtype = Mag_Marutah;
				AddPB(&m->PBflags, &m->blasts, PB_Pilla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					m->mtype = Mag_Vayu;
					AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110)
		{
			m->mtype = Mag_Mitra;
			AddPB(&m->PBflags, &m->blasts, PB_Pilla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				m->mtype = Mag_Surya;
				AddPB(&m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					m->mtype = Mag_Tapas;
					AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120)
		{
			m->mtype = Mag_Namuci;
			AddPB(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				m->mtype = Mag_Sumba;
				AddPB(&m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x10)
				{
					m->mtype = Mag_Ashvinau;
					AddPB(&m->PBflags, &m->blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

void MagLV10Evolution(MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass)
{
	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		m->mtype = Mag_Varuna;
		AddPB(&m->PBflags, &m->blasts, PB_Farlla);
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		m->mtype = Mag_Kalki;
		AddPB(&m->PBflags, &m->blasts, PB_Estlla);
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		m->mtype = Mag_Vritra;
		AddPB(&m->PBflags, &m->blasts, PB_Leilla);
		break;
	}
}

void CheckMagEvolution(MAG* m, unsigned char sectionID, unsigned char type, int EvolutionClass)
{
	if ((m->level < 10) || (m->level >= 35))
	{
		if ((m->level < 35) || (m->level >= 50))
		{
			if (m->level >= 50)
			{
				if (!(m->level % 5)) // Divisible by 5 with no remainder.
				{
					if (EvolutionClass <= 3)
					{
						if (!MagSpecialEvolution(m, sectionID, type, EvolutionClass))
							MagLV50Evolution(m, sectionID, type, EvolutionClass);
					}
				}
			}
		}
		else
		{
			if (EvolutionClass < 2)
				MagLV35Evolution(m, sectionID, type, EvolutionClass);
		}
	}
	else
	{
		if (EvolutionClass <= 0)
			MagLV10Evolution(m, sectionID, type, EvolutionClass);
	}
}

//喂养玛古
void FeedMag(unsigned magid, unsigned itemid, BANANA* client)
{
	LOBBY* l;
	if (!client->lobby)
		return;
	l = (LOBBY*)client->lobby;

	int found_mag = -1;
	int found_item = -1;
	unsigned ch, ch2, mt_index;
	int EvolutionClass = 0;
	MAG* mag;
	unsigned short* ft;
	short mIQ, mDefense, mPower, mDex, mMind;

	memset(&mag, 0, sizeof(MAG));

	for (ch = 0;ch < client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == magid)
		{
			// Found mag
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
			{
				if (l->challenge) {
					found_mag = (~ch + 1);
				}

				else {
					found_mag = ch;
				}
				mag = (MAG*)&client->character.inventory[ch].item.data[0];
				for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
				{
					if (client->character.inventory[ch2].item.itemid == itemid)
					{
						// Found item to feed
						if ((client->character.inventory[ch2].item.data[0] == 0x03) &&
							(client->character.inventory[ch2].item.data[1] < 0x07) &&
							(client->character.inventory[ch2].item.data[1] != 0x02) &&
							(client->character.inventory[ch2].item.data[5] > 0x00))
						{
							if (l->challenge) {
								found_item = (~ch2 + 1);
							}
							else {
								found_item = ch2;
							}
							switch (client->character.inventory[ch2].item.data[1])
							{
							case 0x00:
								mt_index = client->character.inventory[ch2].item.data[2];
								break;
							case 0x01:
								mt_index = 3 + client->character.inventory[ch2].item.data[2];
								break;
							case 0x03:
							case 0x04:
							case 0x05:
								mt_index = 5 + client->character.inventory[ch2].item.data[1];
								break;
							case 0x06:
								mt_index = 6 + client->character.inventory[ch2].item.data[2];
								break;
							}
						}
						break;
					}
				}
			}
			break;
		}
	}
	//代码缺失 Sancaros 修复挑战模式喂养玛古的报错断线
	if ((found_mag == -1) || (found_item == -1))
	{
		if (!l->challenge && (!l->battle)) {
			Send1A(L"Could not find mag to feed or item to feed said mag.", client, 76);
			client->todc = 1;
		}
	}
	else
	{
		DeleteItemFromClient(itemid, 1, 0, client);

		memset(&mag, 0, sizeof(MAG));
		// Rescan to update Mag pointer (if changed due to clean up) 重新扫描以更新磁指针（如果由于清理而更改） 
		for (ch = 0;ch < client->character.inventoryUse;ch++)
		{
			if (client->character.inventory[ch].item.itemid == magid)
			{
				// Found mag (again) 再次搜寻玛古
				if ((client->character.inventory[ch].item.data[0] == 0x02) &&
					(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
				{
					if (l->challenge) {
						found_mag = (~ch + 1);
					}

					else {
						found_mag = ch;
					}
					mag = (MAG*)&client->character.inventory[ch].item.data[0];
					break;
				}
			}
		}

		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (mag->mtype)
		{
		case Mag_Mag:
			ft = &Feed_Table0[0];
			EvolutionClass = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			EvolutionClass = 1;
			ft = &Feed_Table1[0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &Feed_Table2[0];
			EvolutionClass = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &Feed_Table3[0];
			EvolutionClass = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			EvolutionClass = 3;
			ft = &Feed_Table4[0];
			break;
		case Mag_Ila:
		case Mag_Garuda:
		case Mag_Sita:
		case Mag_Soma:
		case Mag_Durga:
		case Mag_Nandin:
		case Mag_Yaksa:
		case Mag_Ribhava:
			EvolutionClass = 3;
			ft = &Feed_Table5[0];
			break;
		case Mag_Andhaka:
		case Mag_Kabanda:
		case Mag_Naga:
		case Mag_Naraka:
		case Mag_Bana:
		case Mag_Marica:
		case Mag_Madhu:
		case Mag_Ravana:
			EvolutionClass = 3;
			ft = &Feed_Table6[0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &Feed_Table5[0];
			EvolutionClass = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &Feed_Table6[0];
			EvolutionClass = 4;
			break;
		default:
			ft = &Feed_Table7[0];
			EvolutionClass = 4;
			break;
		}
		mt_index *= 6;
		mag->synchro += ft[mt_index];
		if (mag->synchro < 0)
			mag->synchro = 0;
		if (mag->synchro > 120)
			mag->synchro = 120;
		mIQ = mag->IQ;
		mIQ += ft[mt_index + 1];
		if (mIQ < 0)
			mIQ = 0;
		if (mIQ > 200)
			mIQ = 200;
		mag->IQ = (unsigned char)mIQ;

		// Add Defense

		mDefense = mag->defense % 100;
		mDefense += ft[mt_index + 2];

		if (mDefense < 0)
			mDefense = 0;

		if (mDefense >= 100)
		{
			if (mag->level == 200)
				mDefense = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->defense = ((mag->defense / 100) * 100) + mDefense;
			CheckMagEvolution(mag, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			mag->defense = ((mag->defense / 100) * 100) + mDefense;

		// Add Power

		mPower = mag->power % 100;
		mPower += ft[mt_index + 3];

		if (mPower < 0)
			mPower = 0;

		if (mPower >= 100)
		{
			if (mag->level == 200)
				mPower = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->power = ((mag->power / 100) * 100) + mPower;
			CheckMagEvolution(mag, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			mag->power = ((mag->power / 100) * 100) + mPower;

		// Add Dex

		mDex = mag->dex % 100;
		mDex += ft[mt_index + 4];

		if (mDex < 0)
			mDex = 0;

		if (mDex >= 100)
		{
			if (mag->level == 200)
				mDex = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->dex = ((mag->dex / 100) * 100) + mDex;
			CheckMagEvolution(mag, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			mag->dex = ((mag->dex / 100) * 100) + mDex;

		// Add Mind

		mMind = mag->mind % 100;
		mMind += ft[mt_index + 5];

		if (mMind < 0)
			mMind = 0;

		if (mMind >= 100)
		{
			if (mag->level == 200)
				mMind = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->mind = ((mag->mind / 100) * 100) + mMind;
			CheckMagEvolution(mag, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			mag->mind = ((mag->mind / 100) * 100) + mMind;
	}
}

void CheckMaxGrind(INVENTORY_ITEM* i)
{
	if (i->item.data[3] > grind_table[i->item.data[1]][i->item.data[2]])
		i->item.data[3] = grind_table[i->item.data[1]][i->item.data[2]];
}

void CreateItem(char* data1, char* data2, char* data3, char* data4, BANANA* client) {
	char* itemHex[] = { data1, data2, data3, data4 };
	LOBBY* l;
	l = (LOBBY*)client->lobby;
	int itemNum = free_game_item(l);
	l->gameItem[itemNum].item.data[0] = hexToByte(&itemHex[0][0]);
	l->gameItem[itemNum].item.data[1] = hexToByte(&itemHex[0][2]);
	l->gameItem[itemNum].item.data[2] = hexToByte(&itemHex[0][4]);
	l->gameItem[itemNum].item.data[3] = hexToByte(&itemHex[0][6]);
	l->gameItem[itemNum].item.data[4] = hexToByte(&itemHex[1][0]);
	l->gameItem[itemNum].item.data[5] = hexToByte(&itemHex[1][2]);
	l->gameItem[itemNum].item.data[6] = hexToByte(&itemHex[1][4]);
	l->gameItem[itemNum].item.data[7] = hexToByte(&itemHex[1][6]);
	l->gameItem[itemNum].item.data[8] = hexToByte(&itemHex[2][0]);
	l->gameItem[itemNum].item.data[9] = hexToByte(&itemHex[2][2]);
	l->gameItem[itemNum].item.data[10] = hexToByte(&itemHex[2][4]);
	l->gameItem[itemNum].item.data[11] = hexToByte(&itemHex[2][6]);
	l->gameItem[itemNum].item.data2[0] = hexToByte(&itemHex[3][0]);
	l->gameItem[itemNum].item.data2[1] = hexToByte(&itemHex[3][2]);
	l->gameItem[itemNum].item.data2[2] = hexToByte(&itemHex[3][4]);
	l->gameItem[itemNum].item.data2[3] = hexToByte(&itemHex[3][6]);
	l->gameItem[itemNum].item.itemid = l->itemID++;
	if (l->gameItemCount < MAX_SAVED_ITEMS)
		l->gameItemList[l->gameItemCount++] = itemNum;
	memset(&PacketData[0], 0, 0x2C);
	PacketData[0x00] = 0x2C;
	PacketData[0x02] = 0x60;
	PacketData[0x08] = 0x5D;
	PacketData[0x09] = 0x09;
	PacketData[0x0A] = 0xFF;
	PacketData[0x0B] = 0xFB;
	*(unsigned*)&PacketData[0x0C] = l->floor[client->clientID];
	*(unsigned*)&PacketData[0x10] = l->clientx[client->clientID];
	*(unsigned*)&PacketData[0x14] = l->clienty[client->clientID];
	memcpy(&PacketData[0x18], &l->gameItem[itemNum].item.data[0], 12);
	*(unsigned*)&PacketData[0x24] = l->gameItem[itemNum].item.itemid;
	*(unsigned*)&PacketData[0x28] = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
	SendToLobby(client->lobby, 4, &PacketData[0], 0x2C, 0);
}
//使用物品
void UseItem(unsigned itemid, BANANA* client)
{
	unsigned found_item = 0, ch, ch2;
	INVENTORY_ITEM i;
	int eq_wep, eq_armor, eq_shield, eq_mag = -1;
	LOBBY* l;
	unsigned new_item, TotalMatUse, HPMatUse, max_mat;
	int mat_exceed;

	// Check item stuff here...  Like converting certain things to certain things...
	//

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;
	//修复挑战模式无法找到玩家物品
	if (l->challenge) {
		found_item = 1;
	}

	for (ch = 0;ch < client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;

			// Copy item before deletion (needed for consumables) 删除前复制物品（耗材需要） 
			memcpy(&i, &client->character.inventory[ch], sizeof(INVENTORY_ITEM));

			// Unwrap mag 展开玛古
			if ((i.item.data[0] == 0x02) && (i.item.data2[2] & 0x40))
			{
				client->character.inventory[ch].item.data2[2] &= ~(0x40);
				break;
			}

			// Unwrap item 展开物品
			if ((i.item.data[0] != 0x02) && (i.item.data[4] & 0x40))
			{
				client->character.inventory[ch].item.data[4] &= ~(0x40);
				break;
			}

			if (i.item.data[0] == 0x03) // Delete consumable item right away 立刻删除临时耗材物品
				DeleteItemFromClient(itemid, 1, 0, client);

			break;
		}
	}

	if (!found_item && !l->challenge && (!l->battle))
	{//缺失 Sancaros
		Send1A(L"Could not find item to \"use\".", client, 77);
		client->todc = 1;
	}
	else
	{
		// Setting the eq variables here should fix problem with ADD SLOT and such.
		eq_wep = eq_armor = eq_shield = eq_mag = -1;

		for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
		{
			if (client->character.inventory[ch2].flags & 0x08)
			{
				switch (client->character.inventory[ch2].item.data[0])
				{
				case 0x00:
					eq_wep = ch2;
					break;
				case 0x01:
					switch (client->character.inventory[ch2].item.data[1])
					{
					case 0x01:
						eq_armor = ch2;
						break;
					case 0x02:
						eq_shield = ch2;
						break;
					}
					break;
				case 0x02:
					eq_mag = ch2;
					break;
				}
			}
		}

		switch (i.item.data[0])
		{
		case 0x00:
			switch (i.item.data[1])
			{
			case 0x33:
				client->character.inventory[ch].item.data[1] = 0x32; // Sealed J-Sword -> Tsumikiri J-Sword
				SendItemToEnd(itemid, client);
				break;
			case 0x1E:
				// Heaven Punisher used...使用天堂冲击
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xAF) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Heaven Punisher
				break;
			case 0x42:
				// Handgun: Guld or Master Raven used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x43) &&
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Guld or Raven...
				break;
			case 0x43:
				// Handgun: Milla or Last Swan used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x42) &&
					(client->character.inventory[eq_wep].item.data[2] == i.item.data[2]) &&
					(client->character.inventory[eq_wep].item.data[3] == 0x09))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x4B; // Guld Milla or Dual Bird
					client->character.inventory[eq_wep].item.data[2] = i.item.data[2];
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Milla or Swan...
				break;
			case 0x8A:
				// Sange or Yasha...
				if (eq_wep != -1)
				{
					if (client->character.inventory[eq_wep].item.data[2] == !(i.item.data[2]))
					{
						client->character.inventory[eq_wep].item.data[1] = 0x89;
						client->character.inventory[eq_wep].item.data[2] = 0x03;
						client->character.inventory[eq_wep].item.data[3] = 0x00;
						client->character.inventory[eq_wep].item.data[4] = 0x00;
						SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
					}
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of the other sword...
				break;
			case 0xAB:
				client->character.inventory[ch].item.data[1] = 0xAC; // Convert Lame d'Argent into Excalibur
				SendItemToEnd(itemid, client);
				break;
			case 0xAF:
				// Ophelie Seize used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x1E) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB0; // Mille Marteaux
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Ophelie Seize
				break;
			case 0xB6:
				// Guren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB7) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Guren
				break;
			case 0xB7:
				// Shouren used...
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0xB6) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0xB8; // Jizai
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Shouren
				break;
			}
			break;
		case 0x01:
			if (i.item.data[1] == 0x03)
			{
				if (i.item.data[2] == 0x4D) // Limiter -> Adept
				{
					client->character.inventory[ch].item.data[2] = 0x4E;
					SendItemToEnd(itemid, client);
				}

				if (i.item.data[2] == 0x4F) // Swordsman Lore -> Proof of Sword-Saint
				{
					client->character.inventory[ch].item.data[2] = 0x50;
					SendItemToEnd(itemid, client);
				}
			}
			break;
		case 0x02:
			switch (i.item.data[1])
			{
			case 0x2B:
				// Chao Mag used
				if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x68) &&
					(client->character.inventory[eq_wep].item.data[2] == 0x00))
				{
					client->character.inventory[eq_wep].item.data[1] = 0x58; // Striker of Chao
					client->character.inventory[eq_wep].item.data[2] = 0x00;
					client->character.inventory[eq_wep].item.data[3] = 0x00;
					client->character.inventory[eq_wep].item.data[4] = 0x00;
					SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Chao
				break;
			case 0x2C:
				// Chu Chu mag used
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[2] == 0x1C))
				{
					client->character.inventory[eq_armor].item.data[2] = 0x2C; // Chuchu Fever
					SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
				}
				DeleteItemFromClient(itemid, 1, 0, client); // Get rid of Chu Chu
				break;
			}
			break;
		case 0x03:
			switch (i.item.data[1])
			{
			case 0x02:
				if (i.item.data[4] < 19)
				{
					if (((char)i.item.data[2] > max_tech_level[i.item.data[4]][client->character._class]) ||
						(client->equip_flags & DROID_FLAG))
					{//缺失 Sancaros
						if (!l->challenge && (!l->battle)) {
							Send1A(L"You can't learn that technique.", client, 78);
							client->todc = 1;
						}
					}
					else
						client->character.techniques[i.item.data[4]] = i.item.data[2]; // Learn technique
				}
				break;
			case 0x0A:
				if (eq_wep != -1 || l->challenge)
				{
					client->character.inventory[eq_wep].item.data[3] += (i.item.data[2] + 1);
					CheckMaxGrind(&client->character.inventory[eq_wep]);
					break;
				}
				break;
			case 0x0B:
				if (!client->mode)
				{
					HPMatUse = (client->character.HPmat + client->character.TPmat) / 2;
					TotalMatUse = 0;
					for (ch2 = 0;ch2 < 5;ch2++)
						TotalMatUse += client->matuse[ch2];
					mat_exceed = 0;
					if (client->equip_flags & HUMAN_FLAG)
						max_mat = 250;
					else
						max_mat = 150;
				}
				else
				{
					TotalMatUse = 0;
					HPMatUse = 0;
					max_mat = 999;
					mat_exceed = 0;
				}
				switch (i.item.data[2])  // Materials
				{
				case 0x00:
					if (TotalMatUse < max_mat)
					{
						client->character.ATP += 2;
						if (!client->mode)
							client->matuse[0]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x01:
					if (TotalMatUse < max_mat)
					{
						client->character.MST += 2;
						if (!client->mode)
							client->matuse[1]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x02:
					if (TotalMatUse < max_mat)
					{
						client->character.EVP += 2;
						if (!client->mode)
							client->matuse[2]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x03:
					if ((client->character.HPmat < 250) && (HPMatUse < 250))
						client->character.HPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x04:
					if ((client->character.TPmat < 250) && (HPMatUse < 250))
						client->character.TPmat += 2;
					else
						mat_exceed = 1;
					break;
				case 0x05:
					if (TotalMatUse < max_mat)
					{
						client->character.DFP += 2;
						if (!client->mode)
							client->matuse[3]++;
					}
					else
						mat_exceed = 1;
					break;
				case 0x06:
					if (TotalMatUse < max_mat)
					{
						client->character.LCK += 2;
						if (!client->mode)
							client->matuse[4]++;
					}
					else
						mat_exceed = 1;
					break;
				default:
					break;
				}
				if (mat_exceed)
				{//缺失 Sancaros
					if (!l->challenge && (!l->battle)) {
						Send1A(L"Attempt to exceed material usage limit.", client, 79);
						client->todc = 1;
					}
				}
				break;
			case 0x0C:
				switch (i.item.data[2])
				{
				case 0x00: // Mag Cell 502
					if (eq_mag != -1)
					{
						if (client->character.sectionID & 0x01)
							client->character.inventory[eq_mag].item.data[1] = 0x1D;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x21;
					}
					break;
				case 0x01: // Mag Cell 213
					if (eq_mag != -1)
					{
						if (client->character.sectionID & 0x01)
							client->character.inventory[eq_mag].item.data[1] = 0x27;
						else
							client->character.inventory[eq_mag].item.data[1] = 0x22;
					}
					break;
				case 0x02: // Parts of RoboChao
					if (eq_mag != -1)
						client->character.inventory[eq_mag].item.data[1] = 0x28;
					break;
				case 0x03: // Heart of Opa Opa
					if (eq_mag != -1)
						client->character.inventory[eq_mag].item.data[1] = 0x29;
					break;
				case 0x04: // Heart of Pian
					if (eq_mag != -1)
						client->character.inventory[eq_mag].item.data[1] = 0x2A;
					break;
				case 0x05: // Heart of Chao
					if (eq_mag != -1)
						client->character.inventory[eq_mag].item.data[1] = 0x2B;
					break;
				}
				break;
			case 0x0E:
				if ((eq_shield != -1) && (i.item.data[2] > 0x15) && (i.item.data[2] < 0x26))
				{
					// Merges 擦除物品代码
					if (i.item.data[2] < 0x1D)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x3A + (i.item.data[2] - 0x16);
					}
					else if (i.item.data[2] < 0x20)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x3A + (i.item.data[2] - 0x15);
					}
					else if (i.item.data[2] < 0x23)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x3A + (i.item.data[2] - 0x14);
					}
					else if (i.item.data[2] == 0x23)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x41;
					}
					else if (i.item.data[2] == 0x24)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x45;
					}
					else if (i.item.data[2] == 0x25)
					{
						client->character.inventory[eq_shield].item.data[2] = 0x49;
					}
					else
					{
						client->character.inventory[eq_shield].item.data[2] = 0x3A + (i.item.data[2] - 0x16);
					}
					SendItemToEnd(client->character.inventory[eq_shield].item.itemid, client);
				}
				else
					switch (i.item.data[2])
					{
					case 0x00:
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x8E))
						{
							client->character.inventory[eq_wep].item.data[1] = 0x8E;
							client->character.inventory[eq_wep].item.data[2] = 0x01; // S-Berill's Hands #1
							client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4] = 0x00;
							SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x01: // Parasitic Gene "Flow"
						if (eq_wep != -1)
						{
							switch (client->character.inventory[eq_wep].item.data[1])
							{
							case 0x02:
								client->character.inventory[eq_wep].item.data[1] = 0x9D; // Dark Flow
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x09:
								client->character.inventory[eq_wep].item.data[1] = 0x9E; // Dark Meteor
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x0B:
								client->character.inventory[eq_wep].item.data[1] = 0x9F; // Dark Bridge
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x02: // Magic Stone "Iritista"
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x05))
						{
							client->character.inventory[eq_wep].item.data[1] = 0x9C; // Rainbow Baton
							client->character.inventory[eq_wep].item.data[2] = 0x00;
							client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4] = 0x00;
							SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x03: // Blue-Black Stone
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x2F) &&
							(client->character.inventory[eq_wep].item.data[2] == 0x00) &&
							(client->character.inventory[eq_wep].item.data[3] == 0x19))
						{
							client->character.inventory[eq_wep].item.data[2] = 0x01; // Black King Bar
							client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4] = 0x00;
							SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
							break;
						}
						break;
					case 0x04: // Syncesta
						if (eq_wep != -1)
						{
							switch (client->character.inventory[eq_wep].item.data[1])
							{
							case 0x1F:
								client->character.inventory[eq_wep].item.data[1] = 0x38; // Lavis Blade
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x38:
								client->character.inventory[eq_wep].item.data[1] = 0x30; // Double Cannon
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							case 0x30:
								client->character.inventory[eq_wep].item.data[1] = 0x1F; // Lavis Cannon
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
						}
						break;
					case 0x05: // Magic Water
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x56))
						{
							if (client->character.inventory[eq_wep].item.data[2] == 0x00)
							{
								client->character.inventory[eq_wep].item.data[1] = 0x5D; // Plantain Fan
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00;
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								break;
							}
							else
								if (client->character.inventory[eq_wep].item.data[2] == 0x01)
								{
									client->character.inventory[eq_wep].item.data[1] = 0x63; // Plantain Huge Fan
									client->character.inventory[eq_wep].item.data[2] = 0x00;
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
									break;
								}
						}
						break;
					case 0x06: // Parasitic Cell Type D
						if (eq_armor != -1)
							switch (client->character.inventory[eq_armor].item.data[2])
							{
							case 0x1D:
								client->character.inventory[eq_armor].item.data[2] = 0x20; // Parasite Wear: De Rol
								SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x20:
								client->character.inventory[eq_armor].item.data[2] = 0x21; // Parsite Wear: Nelgal
								SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x21:
								client->character.inventory[eq_armor].item.data[2] = 0x22; // Parasite Wear: Vajulla
								SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
								break;
							case 0x22:
								client->character.inventory[eq_armor].item.data[2] = 0x2F; // Virus Armor: Lafuteria
								SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
								break;
							}
						break;
					case 0x07: // Magic Rock "Heart Key"
						if (eq_armor != -1)
						{
							if (client->character.inventory[eq_armor].item.data[2] == 0x1C)
							{
								client->character.inventory[eq_armor].item.data[2] = 0x2D; // Love Heart
								SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
							}
							else
								if (client->character.inventory[eq_armor].item.data[2] == 0x2D)
								{
									client->character.inventory[eq_armor].item.data[2] = 0x45; // Sweetheart
									SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
								}
								else
									if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x0C))
									{
										client->character.inventory[eq_wep].item.data[1] = 0x24; // Magical Piece
										client->character.inventory[eq_wep].item.data[2] = 0x00;
										client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
										client->character.inventory[eq_wep].item.data[4] = 0x00;
										SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
									}
									else
										if ((eq_shield != -1) && (client->character.inventory[eq_shield].item.data[2] == 0x15))
										{
											client->character.inventory[eq_shield].item.data[2] = 0x2A; // Safety Heart
											SendItemToEnd(client->character.inventory[eq_shield].item.itemid, client);
										}
						}
						break;
					case 0x08: // Magic Rock "Moola"
						if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[2] == 0x1C))
						{
							client->character.inventory[eq_armor].item.data[2] = 0x31; // Aura Field
							SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x0A))
							{
								client->character.inventory[eq_wep].item.data[1] = 0x4F; // Summit Moon
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00; // No attribute
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
							}
						break;
					case 0x09: // Star Amplifier
						if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[2] == 0x1C))
						{
							client->character.inventory[eq_armor].item.data[2] = 0x30; // Brightness Circle
							SendItemToEnd(client->character.inventory[eq_armor].item.itemid, client);
						}
						else
							if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x0C))
							{
								client->character.inventory[eq_wep].item.data[1] = 0x5C; // Twinkle Star
								client->character.inventory[eq_wep].item.data[2] = 0x00;
								client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
								client->character.inventory[eq_wep].item.data[4] = 0x00; // No attribute
								SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
							}
						break;
					case 0x0A: // Book of Hitogata
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x8C) &&
							(client->character.inventory[eq_wep].item.data[2] == 0x02) &&
							(client->character.inventory[eq_wep].item.data[3] == 0x09))
						{
							client->character.inventory[eq_wep].item.data[1] = 0x8C;
							client->character.inventory[eq_wep].item.data[2] = 0x03;
							client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4] = 0x00;
							SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0B: // Heart of Chu Chu
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x2C;
						break;
					case 0x0C: // Parts of Egg Blaster
						if ((eq_wep != -1) && (client->character.inventory[eq_wep].item.data[1] == 0x06))
						{
							client->character.inventory[eq_wep].item.data[1] = 0x1C; // Egg Blaster
							client->character.inventory[eq_wep].item.data[2] = 0x00;
							client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							client->character.inventory[eq_wep].item.data[4] = 0x00;
							SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
						}
						break;
					case 0x0D: // Heart of Angel
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x2E;
						break;
					case 0x0E: // Heart of Devil
						if (eq_mag != -1)
						{
							if (client->character.inventory[eq_mag].item.data[1] == 0x2F)
								client->character.inventory[eq_mag].item.data[1] = 0x38;
							else
								client->character.inventory[eq_mag].item.data[1] = 0x2F;
						}
						break;
					case 0x0F: // Kit of Hamburger
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x36;
						break;
					case 0x10: // Panther's Spirit
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x37;
						break;
					case 0x11: // Kit of Mark 3
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x31;
						break;
					case 0x12: // Kit of Master System
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x32;
						break;
					case 0x13: // Kit of Genesis
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x33;
						break;
					case 0x14: // Kit of Sega Saturn
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x34;
						break;
					case 0x15: // Kit of Dreamcast
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x35;
						break;
					case 0x26: // Heart of Kapukapu
						if (eq_mag != -1)
							client->character.inventory[eq_mag].item.data[1] = 0x2D;
						break;
					case 0x27: // Photon Booster
						if (eq_wep != -1)
						{
							switch (client->character.inventory[eq_wep].item.data[1])
							{
							case 0x15:
								if (client->character.inventory[eq_wep].item.data[3] == 0x09)
								{
									client->character.inventory[eq_wep].item.data[2] = 0x01; // Burning Visit
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x45:
								if (client->character.inventory[eq_wep].item.data[3] == 0x09)
								{
									client->character.inventory[eq_wep].item.data[2] = 0x01; // Snow Queen
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x4E:
								if (client->character.inventory[eq_wep].item.data[3] == 0x09)
								{
									client->character.inventory[eq_wep].item.data[2] = 0x01; // Iron Faust
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							case 0x6D:
								if (client->character.inventory[eq_wep].item.data[3] == 0x14)
								{
									client->character.inventory[eq_wep].item.data[2] = 0x01; // Power Maser
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
								}
								break;
							}
						}
						break;

					}
				break;
			case 0x12:
				//アイテムIDの5, 6文字目が1, 2のアイテムの場合（0x0312 * *のとき）
				switch (i.item.data[2])
					//スイッチ文。「使用」するアイテムIDの7, 8文字目をスイッチに使う。
				{
					/*
					case 0x07: // weapons bone
				   //アイテムIDの7,8文字目が0,7のとき。（0x031207のとき　つまりウェポンズバッジ骨のとき）
						if (eq_wep != -1)
							//詳細不明、武器装備時？
						{
							switch (client->character.inventory[eq_wep].item.data[1])
								//スイッチにキャラクターの装備している武器のアイテムIDの3,4文字目をスイッチに使う
							{
							case 0x22:
								//キャラクターの装備している武器のアイテムIDの3,4文字目が2,2のとき（0x002200のとき　つまりカジューシース）
								if (client->character.inventory[eq_wep].item.data[3] == 0x09)
									//もしキャラクターの装備している武器のアイテムIDの7,8文字目が0,9だったら（つまり強化値が + 9だったら）
								{
									client->character.inventory[eq_wep].item.data[2] = 0x01; // melcrius
							   //キャラクターの装備している武器のアイテムIDの5,6文字目を0,1に変更する
							   //（つまりカジューシース + 9（0x00220009）→メルクリウスロッド + 9（0x00220109）にする）
									client->character.inventory[eq_wep].item.data[3] = 0x00; // Not grinded
							   //強化値を０にする（つまりメルクリウスロッド + 9（0x00220109）→メルクリウスロッド（0x00220100）にする）
									client->character.inventory[eq_wep].item.data[4] = 0x00;
									SendItemToEnd(client->character.inventory[eq_wep].item.itemid, client);
									// 詳細不明。装備している武器をインベントリの最終行に送る？
								}
								break;
							}
						}
						break;*/
				case 0x00: // weapons bronze
					if (eq_wep != -1)
					{
						unsigned ai, plustype, num_attribs = 0;
						plustype = 1;
						ai = 0;
						if ((client->character.inventory[eq_wep].item.data[6] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[6] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[6] == plustype)
								ai = 7;
						}
						if ((client->character.inventory[eq_wep].item.data[8] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[8] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[8] == plustype)
								ai = 9;
						}
						if ((client->character.inventory[eq_wep].item.data[10] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[10] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[10] == plustype)
								ai = 11;
						}
						if (ai)
						{
							// Attribute already on weapon, increase it
							client->character.inventory[eq_wep].item.data[ai] += 0x0A;
							if (client->character.inventory[eq_wep].item.data[ai] > 100)
								client->character.inventory[eq_wep].item.data[ai] = 100;
						}
						else
						{
							// Attribute not on weapon, add it if there isn't already 3 attributes
							if (num_attribs < 3)
							{
								client->character.inventory[eq_wep].item.data[6 + (num_attribs * 2)] = 0x01;
								client->character.inventory[eq_wep].item.data[7 + (num_attribs * 2)] = 0x0A;
							}
						}
					}
					break;
				case 0x01: // weapons silver
					if (eq_wep != -1)
					{
						unsigned ai, plustype, num_attribs = 0;
						plustype = 2;
						ai = 0;
						if ((client->character.inventory[eq_wep].item.data[6] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[6] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[6] == plustype)
								ai = 7;
						}
						if ((client->character.inventory[eq_wep].item.data[8] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[8] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[8] == plustype)
								ai = 9;
						}
						if ((client->character.inventory[eq_wep].item.data[10] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[10] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[10] == plustype)
								ai = 11;
						}
						if (ai)
						{
							// Attribute already on weapon, increase it
							client->character.inventory[eq_wep].item.data[ai] += 0x0A;
							if (client->character.inventory[eq_wep].item.data[ai] > 100)
								client->character.inventory[eq_wep].item.data[ai] = 100;
						}
						else
						{
							// Attribute not on weapon, add it if there isn't already 3 attributes
							if (num_attribs < 3)
							{
								client->character.inventory[eq_wep].item.data[6 + (num_attribs * 2)] = 0x02;
								client->character.inventory[eq_wep].item.data[7 + (num_attribs * 2)] = 0x0A;
							}
						}
					}
					break;
				case 0x02: // weapons gold
					if (eq_wep != -1)
					{
						unsigned ai, plustype, num_attribs = 0;
						plustype = 3;
						ai = 0;
						if ((client->character.inventory[eq_wep].item.data[6] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[6] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[6] == plustype)
								ai = 7;
						}
						if ((client->character.inventory[eq_wep].item.data[8] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[8] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[8] == plustype)
								ai = 9;
						}
						if ((client->character.inventory[eq_wep].item.data[10] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[10] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[10] == plustype)
								ai = 11;
						}
						if (ai)
						{
							// Attribute already on weapon, increase it
							client->character.inventory[eq_wep].item.data[ai] += 0x0A;
							if (client->character.inventory[eq_wep].item.data[ai] > 100)
								client->character.inventory[eq_wep].item.data[ai] = 100;
						}
						else
						{
							// Attribute not on weapon, add it if there isn't already 3 attributes
							if (num_attribs < 3)
							{
								client->character.inventory[eq_wep].item.data[6 + (num_attribs * 2)] = 0x03;
								client->character.inventory[eq_wep].item.data[7 + (num_attribs * 2)] = 0x0A;
							}
						}
					}
					break;
				case 0x03: // weapons cristal
					if (eq_wep != -1)
					{
						unsigned ai, plustype, num_attribs = 0;
						char attrib_add = 10;
						plustype = 4;
						ai = 0;
						if ((client->character.inventory[eq_wep].item.data[6] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[6] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[6] == plustype)
								ai = 7;
						}
						if ((client->character.inventory[eq_wep].item.data[8] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[8] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[8] == plustype)
								ai = 9;
						}
						if ((client->character.inventory[eq_wep].item.data[10] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[10] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[10] == plustype)
								ai = 11;
						}
						if (ai)
						{
							// Attribute already on weapon, increase it
							client->character.inventory[eq_wep].item.data[ai] += 0x0A;
							if (client->character.inventory[eq_wep].item.data[ai] > 100)
								client->character.inventory[eq_wep].item.data[ai] = 100;
						}
						else
						{
							// Attribute not on weapon, add it if there isn't already 3 attributes
							if (num_attribs < 3)
							{
								client->character.inventory[eq_wep].item.data[6 + (num_attribs * 2)] = 0x04;
								client->character.inventory[eq_wep].item.data[7 + (num_attribs * 2)] = 0x0A;
							}
						}
					}
					break;
				case 0x04: // weapons steal
					if (eq_wep != -1)
					{
						unsigned ai, plustype, num_attribs = 0;
						plustype = 5;
						ai = 0;
						if ((client->character.inventory[eq_wep].item.data[6] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[6] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[6] == plustype)
								ai = 7;
						}
						if ((client->character.inventory[eq_wep].item.data[8] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[8] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[8] == plustype)
								ai = 9;
						}
						if ((client->character.inventory[eq_wep].item.data[10] > 0x00) &&
							(!(client->character.inventory[eq_wep].item.data[10] & 128)))
						{
							num_attribs++;
							if (client->character.inventory[eq_wep].item.data[10] == plustype)
								ai = 11;
						}
						if (ai)
						{
							// Attribute already on weapon, increase it
							client->character.inventory[eq_wep].item.data[ai] += 0x05;
							if (client->character.inventory[eq_wep].item.data[ai] > 100)
								client->character.inventory[eq_wep].item.data[ai] = 100;
						}
						else
						{
							// Attribute not on weapon, add it if there isn't already 3 attributes
							if (num_attribs < 3)
							{
								client->character.inventory[eq_wep].item.data[6 + (num_attribs * 2)] = 0x05;
								client->character.inventory[eq_wep].item.data[7 + (num_attribs * 2)] = 0x05;
							}
						}
					}
				}
				break;
			case 0x0D: //enemy part
				new_item = 0;
				switch (i.item.data[2]) {
				case 0x00:
					new_item = 0x1700;
					break;
				case 0x01:
					new_item = 0x1800;
					break;
				case 0x02:
					new_item = 0x1900;
					break;
				case 0x03:
					new_item = 0x1A00;
					break;
				case 0x04:
					new_item = 0x1B00;
					break;
				case 0x05:
					new_item = 0x01021A;
					break;
				case 0x06:
					new_item = 0x6200;
					break;
				case 0x07:
					new_item = 0x6000;
					break;
				case 0x08:
					new_item = 0x5300;
					break;
				case 0x09:
					new_item = 0x5400;
					break;
				case 0x0A:
					new_item = 0x6700;
					break;
				case 0x0B:
					new_item = 0x4D00;
					break;
				case 0x0C:
					new_item = 0x9100;
					break;
				case 0x0D:
					new_item = 0x8E00;
					break;
				case 0x0E:
					new_item = 0xA000;
					break;
				case 0x0F:
					new_item = 0xA200;
					break;
				case 0x10:
					new_item = 0xA201;
					break;
				case 0x11:
					new_item = 0xA202;
					break;
				case 0x12:
					new_item = 0x9600;
					break;
				case 0x13:
					new_item = 0xA100;
					break;
				}
				if (new_item)
				{
					INVENTORY_ITEM add_item;
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					*(unsigned*)&add_item.item.data[0] = new_item;
					add_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory(&add_item, 1, 0, client);
				}
				break;
			case 0x14://新增的
				switch (i.item.data[2])
				{
				case 0x03: // WeaponsSilverBadge 031403
					//Add Exp
					if (client->character.level < 200) {
						AddExp(100000, client);
					}
					break;
				case 0x04: // WeaponseGold→ルクミンM150
					CreateItem("023C96A3", "00000000", "0000983A", "78C80704", client);
					break;
				case 0x05: // WeaponseCrystal→DB全セット
					CreateItem("01012800", "00040000", "00000000", "00000000", client); //DB鎧 スロット4
					CreateItem("0001052C", "00000532", "03640464", "00000000", client); //DB剣 Machine 100% Ruin 100% Hit 50%
					CreateItem("01022600", "00000000", "00000000", "00000000", client); //DB盾
					break;
				}
				break;
			case 0x0F: // Add Slot
				if ((eq_armor != -1) && (client->character.inventory[eq_armor].item.data[5] < 0x04))
					client->character.inventory[eq_armor].item.data[5]++;
				break;
			case 0x15:
				new_item = 0;
				switch (i.item.data[2])
				{
				case 0x00:
					new_item = 0x0D0E03 + ((mt_lrand() % 9) * 0x10000);
					break;
				case 0x01:
					new_item = easter_drops[mt_lrand() % 9];
					break;
				case 0x02:
					new_item = jacko_drops[mt_lrand() % 8];
					break;
				default:
					break;
				}

				if (new_item)
				{
					INVENTORY_ITEM add_item;

					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					*(unsigned*)&add_item.item.data[0] = new_item;
					add_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory(&add_item, 1, 0, client);
				}
				break;
			case 0x18: // Ep4 Mag Cells
				if (eq_mag != -1)
					client->character.inventory[eq_mag].item.data[1] = 0x42 + (i.item.data[2]);
				break;
			}
			break;
		default:
			break;
		}
	}
}

//检查装备
int check_equip(unsigned char eq_flags, unsigned char cl_flags)
{
	int eqOK = 1;
	unsigned ch;

	for (ch = 0;ch < 8;ch++)
	{
		if ((cl_flags & (1 << ch)) && (!(eq_flags & (1 << ch))))
		{
			eqOK = 0;
			break;
		}
	}
	return eqOK;
}

//装备物品
void EquipItem(unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item, found_slot;
	unsigned slot[4];

	found_item = 0;
	found_slot = 0;

	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	//这里需要修改
	if (!found_item && l->challenge) {
		found_item = 1;
	}

	for (ch = 0;ch < client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			//debug ("Equipped %u", itemid);
			found_item = 1;
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Check weapon equip requirements
				if (!check_equip(weapon_equip_table[client->character.inventory[ch].item.data[1]][client->character.inventory[ch].item.data[2]], client->equip_flags))
				{
					if (!l->challenge && (!l->battle)) {
						Send1A(L"\"God/Equip\" is disallowed.", client, 80);
						client->todc = 1;
					}
				}
				if (!client->todc)
				{
					// De-equip any other weapon on character. (Prevent stacking)
					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
							(client->character.inventory[ch2].flags & 0x08))
							client->character.inventory[ch2].flags &= ~(0x08);
				}
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01: // Check armor equip requirements
					if ((!check_equip(armor_equip_table[client->character.inventory[ch].item.data[2]], client->equip_flags)) ||
						(client->character.level < armor_level_table[client->character.inventory[ch].item.data[2]]))
					{
						if (!l->challenge && (!l->battle)) {
							Send1A(L"\"God/Equip\" is disallowed.", client, 80);
							client->todc = 1;
						}
					}
					if (!client->todc)
					{
						// Remove any other armor and equipped slot items.
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] != 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
						break;
					}
					break;
				case 0x02: // Check barrier equip requirements
					if ((!check_equip(barrier_equip_table[client->character.inventory[ch].item.data[2]] & client->equip_flags, client->equip_flags)) ||
						(client->character.level < barrier_level_table[client->character.inventory[ch].item.data[2]]))
					{
						if (!l->challenge && (!l->battle)) {
							Send1A(L"\"God/Equip\" is disallowed.", client, 80);
							client->todc = 1;
						}
					}
					if (!client->todc)
					{
						// Remove any other barrier
						for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
							if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
								(client->character.inventory[ch2].item.data[1] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
							{
								client->character.inventory[ch2].flags &= ~(0x08);
								client->character.inventory[ch2].item.data[4] = 0x00;
							}
					}
					break;
				case 0x03: // Assign unit a slot
					for (ch2 = 0;ch2 < 4;ch2++)
						slot[ch2] = 0;
					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					{
						// Another loop ;(
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] == 0x03))
						{
							if ((client->character.inventory[ch2].flags & 0x08) &&
								(client->character.inventory[ch2].item.data[4] < 0x04))
								slot[client->character.inventory[ch2].item.data[4]] = 1;
						}
					}
					for (ch2 = 0;ch2 < 4;ch2++)
						if (slot[ch2] == 0)
						{
							found_slot = ch2 + 1;
							break;
						}
					if (found_slot)
					{
						found_slot--;
						client->character.inventory[ch].item.data[4] = (unsigned char)(found_slot);
					}
					else
					{//缺失 Sancaros
						if (!l->challenge && (!l->battle)) {
							client->character.inventory[ch].flags &= ~(0x08);
							Send1A(L"There are no free slots on your armor.  Equip unit failed.", client, 81);
							client->todc = 1;
						}
					}
					break;
				}
				break;
			case 0x02:
				// Remove equipped mag
				for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			if (!client->todc)  // Finally, equip item 终于可以穿上了
				client->character.inventory[ch].flags |= 0x08;
			break;
		}
	}
	if (!found_item && !l->challenge && (!l->battle))
	{//缺失 Sancaros
		Send1A(L"Could not find item to equip.", client, 82);
		client->todc = 1;
	}
}

//解除装备
void DeequipItem(unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item = 0;

	LOBBY* l;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;

	if (!found_item && l->challenge) {
		found_item = 1;
	}

	for (ch = 0;ch < client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;
			client->character.inventory[ch].flags &= ~(0x08);
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Remove any other weapon (in case of a glitch... or stacking)
				for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01:
					// Remove any other armor (stacking?) and equipped slot items.
					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] != 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
					break;
				case 0x02:
					// Remove any other barrier (stacking?)
					for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
						if ((client->character.inventory[ch2].item.data[0] == 0x01) &&
							(client->character.inventory[ch2].item.data[1] == 0x02) &&
							(client->character.inventory[ch2].flags & 0x08))
						{
							client->character.inventory[ch2].flags &= ~(0x08);
							client->character.inventory[ch2].item.data[4] = 0x00;
						}
					break;
				case 0x03:
					// Remove unit from slot
					client->character.inventory[ch].item.data[4] = 0x00;
					break;
				}
				break;
			case 0x02:
				// Remove any other mags (stacking?)
				for (ch2 = 0;ch2 < client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			break;
		}
	}
	if (!found_item && !l->challenge && (!l->battle))
	{
		Send1A(L"Could not find item to unequip.", client, 83);
		client->todc = 1;
	}
}

//获取商店价格
unsigned GetShopPrice(INVENTORY_ITEM* ci)
{
	unsigned compare_item, ch;
	int percent_add, price;
	unsigned char variation;
	float percent_calc;
	float price_calc;

	price = 10;

	//可以检索商店物品的数据
	/*	printf ("Raw item data for this item is:\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n",
	ci->item.data[0], ci->item.data[1], ci->item.data[2], ci->item.data[3],
	ci->item.data[4], ci->item.data[5], ci->item.data[6], ci->item.data[7],
	ci->item.data[8], ci->item.data[9], ci->item.data[10], ci->item.data[11],
	ci->item.data[12], ci->item.data[13], ci->item.data[14], ci->item.data[15] ); */

	switch (ci->item.data[0])
	{
	case 0x00: // Weapons 武器
		if (ci->item.data[4] & 0x80)
			price = 1; // Untekked = 1 meseta 取消选中 = 1美赛塔
		else
		{
			if ((ci->item.data[1] < 0x0D) && (ci->item.data[2] < 0x05))
			{
				if ((ci->item.data[1] > 0x09) && (ci->item.data[2] > 0x03)) // Canes, Rods, Wands become rare faster  拐杖、棍棒、魔杖越来越稀少 
					break;
				price = weapon_atpmax_table[ci->item.data[1]][ci->item.data[2]] + ci->item.data[3];
				price *= price;
				price_calc = (float)price;
				switch (ci->item.data[1])
				{
				case 0x01:
					price_calc /= 5.0;
					break;
				case 0x02:
					price_calc /= 4.0;
					break;
				case 0x03:
				case 0x04:
					price_calc *= 2.0;
					price_calc /= 3.0;
					break;
				case 0x05:
					price_calc *= 4.0;
					price_calc /= 5.0;
					break;
				case 0x06:
					price_calc *= 10.0;
					price_calc /= 21.0;
					break;
				case 0x07:
					price_calc /= 3.0;
					break;
				case 0x08:
					price_calc *= 25.0;
					break;
				case 0x09:
					price_calc *= 10.0;
					price_calc /= 9.0;
					break;
				case 0x0A:
					price_calc /= 2.0;
					break;
				case 0x0B:
					price_calc *= 2.0;
					price_calc /= 5.0;
					break;
				case 0x0C:
					price_calc *= 4.0;
					price_calc /= 3.0;
					break;
				}

				percent_add = 0;
				if (ci->item.data[6])
					percent_add += (char)ci->item.data[7];
				if (ci->item.data[8])
					percent_add += (char)ci->item.data[9];
				if (ci->item.data[10])
					percent_add += (char)ci->item.data[11];

				if (percent_add != 0)
				{
					percent_calc = price_calc;
					percent_calc /= 300.0;
					percent_calc *= percent_add;
					price_calc += percent_calc;
				}
				price_calc /= 8.0;
				price = (int)(price_calc);
				price += attrib[ci->item.data[4]];
			}
		}
		break;
	case 0x01:
		switch (ci->item.data[1])
		{
		case 0x01: // Armor 装备
			if (ci->item.data[2] < 0x18)
			{
				// Calculate the amount to boost because of slots...
				if (ci->item.data[5] > 4)
					price = armor_prices[(ci->item.data[2] * 5) + 4];
				else
					price = armor_prices[(ci->item.data[2] * 5) + ci->item.data[5]];
				price -= armor_prices[(ci->item.data[2] * 5)];
				if (ci->item.data[6] > armor_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= armor_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price += equip_prices[1][1][ci->item.data[2]][variation];
			}
			break;
		case 0x02: // Shield 盾牌
			if (ci->item.data[2] < 0x15)
			{
				if (ci->item.data[6] > barrier_dfpvar_table[ci->item.data[2]])
					variation = 0;
				else
					variation = ci->item.data[6];
				if (ci->item.data[8] <= barrier_evpvar_table[ci->item.data[2]])
					variation += ci->item.data[8];
				price = equip_prices[1][2][ci->item.data[2]][variation];
			}
			break;
		case 0x03: // Units 插槽
			if (ci->item.data[2] < 0x40)
				price = unit_prices[ci->item.data[2]];
			break;
		}
		break;
	case 0x03:
		// Tool 工具
		if (ci->item.data[1] == 0x02) // Technique 魔法科技
		{
			if (ci->item.data[4] < 0x13)
				price = ((int)(ci->item.data[2] + 1) * tech_prices[ci->item.data[4]]) / 100L;
		}
		else
		{
			compare_item = 0;
			memcpy(&compare_item, &ci->item.data[0], 3);
			for (ch = 0;ch < (sizeof(tool_prices) / 4);ch += 2)
				if (compare_item == tool_prices[ch])
				{
					price = tool_prices[ch + 1];
					break;
				}
		}
		break;
	}
	if (price < 0)
		price = 0;
	//printf ("GetShopPrice = %u\n", price);
	return (unsigned)price;
}

//跳过等级给MAG添加数值
void SkipToLevel(unsigned short target_level, BANANA* client, int quiet)
{
	MAG* m;
	unsigned short ch, finalDFP, finalATP, finalATA, finalMST;

	if (target_level > 199)
		target_level = 199;

	if ((!client->lobby) || (client->character.level >= target_level))
		return;

	if (!quiet)
	{
		PacketBF[0x0A] = (unsigned char)client->clientID;
		*(unsigned*)&PacketBF[0x0C] = tnlxp[target_level - 1] - client->character.XP;
		SendToLobby(client->lobby, 4, &PacketBF[0], 0x10, 0);
	}

	while (client->character.level < target_level)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
	}

	client->character.XP = tnlxp[target_level - 1];

	finalDFP = client->character.DFP;
	finalATP = client->character.ATP;
	finalATA = client->character.ATA;
	finalMST = client->character.MST;

	// Add the mag bonus to the 0x30 packet  向0x30包添加MAG加值 
	for (ch = 0;ch < client->character.inventoryUse;ch++)
	{
		if ((client->character.inventory[ch].item.data[0] == 0x02) &&
			(client->character.inventory[ch].flags & 0x08))
		{
			m = (MAG*)&client->character.inventory[ch].item.data[0];
			finalDFP += (m->defense / 100);
			finalATP += (m->power / 100) * 2;
			finalATA += (m->dex / 100) / 2;
			finalMST += (m->mind / 100) * 2;
			break;
		}
	}

	if (!quiet)
	{
		*(unsigned short*)&Packet30[0x0C] = finalATP;
		*(unsigned short*)&Packet30[0x0E] = finalMST;
		*(unsigned short*)&Packet30[0x10] = client->character.EVP;
		*(unsigned short*)&Packet30[0x12] = client->character.HP;
		*(unsigned short*)&Packet30[0x14] = finalDFP;
		*(unsigned short*)&Packet30[0x16] = finalATA;
		*(unsigned short*)&Packet30[0x18] = client->character.level;
		Packet30[0x0A] = (unsigned char)client->clientID;
		SendToLobby(client->lobby, 4, &Packet30[0x00], 0x1C, 0);
	}
}

// 跳过等级给MAG添加经验
void AddExp(unsigned XP, BANANA* client)
{
	MAG* m;
	unsigned short levelup, ch, finalDFP, finalATP, finalATA, finalMST;

	if (!client->lobby)
		return;

	client->character.XP += XP;

	PacketBF[0x0A] = (unsigned char)client->clientID;
	*(unsigned*)&PacketBF[0x0C] = XP;
	SendToLobby(client->lobby, 4, &PacketBF[0], 0x10, 0);
	if (client->character.XP >= tnlxp[client->character.level])
		levelup = 1;
	else
		levelup = 0;

	while (levelup)
	{
		client->character.ATP += playerLevelData[client->character._class][client->character.level].ATP;
		client->character.MST += playerLevelData[client->character._class][client->character.level].MST;
		client->character.EVP += playerLevelData[client->character._class][client->character.level].EVP;
		client->character.HP += playerLevelData[client->character._class][client->character.level].HP;
		client->character.DFP += playerLevelData[client->character._class][client->character.level].DFP;
		client->character.ATA += playerLevelData[client->character._class][client->character.level].ATA;
		client->character.level++;
		if ((client->character.level == 199) || (client->character.XP < tnlxp[client->character.level]))
			break;
	}

	if (levelup)
	{
		finalDFP = client->character.DFP;
		finalATP = client->character.ATP;
		finalATA = client->character.ATA;
		finalMST = client->character.MST;

		// Add the mag bonus to the 0x30 packet  向0x30包添加MAG加值 
		for (ch = 0;ch < client->character.inventoryUse;ch++)
		{
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].flags & 0x08))
			{
				m = (MAG*)&client->character.inventory[ch].item.data[0];
				finalDFP += (m->defense / 100);
				finalATP += (m->power / 100) * 2;
				finalATA += (m->dex / 100) / 2;
				finalMST += (m->mind / 100) * 2;
				break;
			}
		}

		*(unsigned short*)&Packet30[0x0C] = finalATP;
		*(unsigned short*)&Packet30[0x0E] = finalMST;
		*(unsigned short*)&Packet30[0x10] = client->character.EVP;
		*(unsigned short*)&Packet30[0x12] = client->character.HP;
		*(unsigned short*)&Packet30[0x14] = finalDFP;
		*(unsigned short*)&Packet30[0x16] = finalATA;
		*(unsigned short*)&Packet30[0x18] = client->character.level;
		Packet30[0x0A] = (unsigned char)client->clientID;
		SendToLobby(client->lobby, 4, &Packet30[0x00], 0x1C, 0);
	}
}

void PrepGuildCard(unsigned from, unsigned to)
{
	int gc_present = 0;
	unsigned hightime = 0xFFFFFFFF;
	unsigned highidx = 0;
	unsigned ch;

	if (ship_gcsend_count < MAX_GCSEND)
	{
		for (ch = 0;ch < (ship_gcsend_count * 3);ch += 3)
		{
			if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch + 1] == to))
			{
				gc_present = 1;
				break;
			}
		}

		if (!gc_present)
		{
			highidx = ship_gcsend_count * 3;
			ship_gcsend_count++;
		}
	}
	else
	{
		// Erase oldest sent card
		for (ch = 0;ch < (MAX_GCSEND * 3);ch += 3)
		{
			if (ship_gcsend_list[ch + 2] < hightime)
			{
				hightime = ship_gcsend_list[ch + 2];
				highidx = ch;
			}
		}
	}
	ship_gcsend_list[highidx] = from;
	ship_gcsend_list[highidx + 1] = to;
	ship_gcsend_list[highidx + 2] = (unsigned)servertime;
}

int PreppedGuildCard(unsigned from, unsigned to)
{
	int gc_present = 0;
	unsigned ch, ch2;

	for (ch = 0;ch < (ship_gcsend_count * 3);ch += 3)
	{
		if ((ship_gcsend_list[ch] == from) && (ship_gcsend_list[ch + 1] == to))
		{
			ship_gcsend_list[ch] = 0;
			ship_gcsend_list[ch + 1] = 0;
			ship_gcsend_list[ch + 2] = 0;
			gc_present = 1;
			break;
		}
	}

	if (gc_present)
	{
		// Clean up the list
		ch2 = 0;
		for (ch = 0;ch < (ship_gcsend_count * 3);ch += 3)
		{
			if ((ship_gcsend_list[ch] != 0) && (ch != ch2))
			{
				ship_gcsend_list[ch2] = ship_gcsend_list[ch];
				ship_gcsend_list[ch2 + 1] = ship_gcsend_list[ch + 1];
				ship_gcsend_list[ch2 + 2] = ship_gcsend_list[ch + 2];
				ch2 += 3;
			}
		}
		ship_gcsend_count = ch2 / 3;
	}

	return gc_present;
}

int ban(unsigned gc_num, unsigned* ipaddr, long long* hwinfo, unsigned type, BANANA* client)
{
	int banned = 1;
	unsigned ch, ch2;
	FILE* fp;

	for (ch = 0;ch < num_bans;ch++)
	{
		if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == type))
		{
			banned = 0;
			ship_bandata[ch].guildcard = 0;
			ship_bandata[ch].type = 0;
			break;
		}
	}

	if (banned)
	{
		if (num_bans < 5000)
		{
			ship_bandata[num_bans].guildcard = gc_num;
			ship_bandata[num_bans].type = type;
			ship_bandata[num_bans].ipaddr = *ipaddr;
			ship_bandata[num_bans++].hwinfo = *hwinfo;
			fp = fopen("bandata.dat", "wb");
			if (fp)
			{
				fwrite(&ship_bandata[0], 1, sizeof(BANDATA) * num_bans, fp);
				fclose(fp);
			}
			else
				WriteLog("无法打开并写入 bandata.dat 文件!!");
		}
		else
		{
			banned = 0; // Can't ban with a full list...
			SendB0(L"Ship ban list is full.", client, 0);
		}
	}
	else
	{
		ch2 = 0;
		for (ch = 0;ch < num_bans;ch++)
		{
			if ((ship_bandata[ch].type != 0) && (ch != ch2))
				memcpy(&ship_bandata[ch2++], &ship_bandata[ch], sizeof(BANDATA));
		}
		num_bans = ch2;
		fp = fopen("bandata.dat", "wb");
		if (fp)
		{
			fwrite(&ship_bandata[0], 1, sizeof(BANDATA) * num_bans, fp);
			fclose(fp);
		}
		else
			WriteLog("无法打开并写入 bandata.dat 文件!!");
	}
	return banned;
}

int stfu(unsigned gc_num)
{
	int result = 0;
	unsigned ch;

	for (ch = 0;ch < ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			result = 1;
			break;
		}
	}

	return result;
}

int toggle_stfu(unsigned gc_num, BANANA* client)
{
	int ignored = 1;
	unsigned ch, ch2;

	for (ch = 0;ch < ship_ignore_count;ch++)
	{
		if (ship_ignore_list[ch] == gc_num)
		{
			ignored = 0;
			ship_ignore_list[ch] = 0;
			break;
		}
	}

	if (ignored)
	{
		if (ship_ignore_count < 300)
			ship_ignore_list[ship_ignore_count++] = gc_num;
		else
		{
			ignored = 0; // Can't ignore with a full list... 不能忽略一个完整的列表 
			SendB0(L"Ship ignore list is full.", client, 1);
		}
	}
	else
	{
		ch2 = 0;
		for (ch = 0;ch < ship_ignore_count;ch++)
		{
			if ((ship_ignore_list[ch] != 0) && (ch != ch2))
				ship_ignore_list[ch2++] = ship_ignore_list[ch];
		}
		ship_ignore_count = ch2;
	}
	return ignored;
}

//进入游戏模式模式
void Send60(BANANA* client)
{
	unsigned short size, size_check_index;
	unsigned short sizecheck = 0;
	int dont_send = 0;
	LOBBY* l;
	int boss_floor = 0;
	unsigned itemid, magid, count, drop;
	unsigned short mid;
	short mHP;
	unsigned XP, ch, ch2, max_send, shop_price;
	int mid_mismatch;
	int ignored;
	int ws_ok;
	unsigned short ws_data, counter;
	BANANA* lClient;

	size = *(unsigned short*)&client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (!client->lobby)
		return;

#ifdef LOG_60
	packet_to_text(&client->decryptbuf[0], size);
	fprintf(debugfile, "%s\n", dp);
#endif

	if (size != sizecheck)
	{
		debug("客户端发送了一个0x60数据包 其大小检查 != size.\n"); //对战模式未完成的代码
		debug("指令: %02X | 大小: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	l = (LOBBY*)client->lobby;

	if (client->lobbyNum < 0x10)
	{
		size_check_index = client->decryptbuf[0x08];
		size_check_index *= 2;

		if (client->decryptbuf[0x08] == 0x06)
			sizecheck = 0x114;
		else
			sizecheck = size_check_table[size_check_index + 1] + 4;

		if ((size != sizecheck) && (sizecheck > 4))
			dont_send = 1;

		if (sizecheck == 4) // No size check packet encountered while in lobby mode...
		{
			debug("没有0x60房间数据包的大小检查信息 %02x", client->decryptbuf[0x08]);
			dont_send = 1;
		}
	}
	else
	{
		if (dont_send_60[(client->decryptbuf[0x08] * 2) + 1] == 1)
		{
			dont_send = 1;
			WriteLog("60 指令 \"%02x\" 被游戏屏蔽了. (数据在下面)", client->decryptbuf[0x08]);
			packet_to_text(&client->decryptbuf[0x00], size);
			WriteLog("%s", &dp[0]);
		}
	}

	if ((client->decryptbuf[0x0A] != client->clientID) &&
		(size_check_table[(client->decryptbuf[0x08] * 2) + 1] != 0x00) &&
		(client->decryptbuf[0x08] != 0x07) &&
		(client->decryptbuf[0x08] != 0x79))
		dont_send = 1;

	if ((client->decryptbuf[0x08] == 0x07) &&
		(client->decryptbuf[0x0C] != client->clientID))
		dont_send = 1;

	if (client->decryptbuf[0x08] == 0x72)
		dont_send = 1;

	if (!dont_send)
	{
		switch (client->decryptbuf[0x08])
		{
		case 0x07:
			// Symbol chat (throttle for spam)  符号聊天（垃圾邮件节流） 
			dont_send = 1;
			if ((((unsigned)servertime - client->command_cooldown[0x07]) >= 1) && (!stfu(client->guildcard)))
			{
				client->command_cooldown[0x07] = (unsigned)servertime;
				if (client->lobbyNum < 0x10)
					max_send = 12;
				else
					max_send = 4;
				for (ch = 0;ch < max_send;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						ignored = 0;
						lClient = l->client[ch];
						for (ch2 = 0;ch2 < lClient->ignore_count;ch2++)
						{
							if (lClient->ignore_list[ch2] == client->guildcard)
							{
								ignored = 1;
								break;
							}
						}
						if ((!ignored) && (lClient->guildcard != client->guildcard))
						{
							cipher_ptr = &lClient->server_cipher;
							encryptcopy(lClient, &client->decryptbuf[0x00], size);
						}
					}
				}
			}
			break;
		case 0x0A:
			if (client->lobbyNum > 0x0F)
			{
				// Player hit a monster 玩家攻击一个怪物
				mid = *(unsigned short*)&client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if ((mid < 0xB50) && (l->floor[client->clientID] != 0))
				{
					mHP = *(short*)&client->decryptbuf[0x0E];
					l->monsterData[mid].HP = mHP;
				}
			}
			else
				client->todc = 1;
			break;
		case 0x1F:
			// Remember client's position. 记住客户端的位置
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			break;
		case 0x20:
			// Remember client's position. 记住客户端的方位
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned*)&client->decryptbuf[0x18];
			l->clientx[client->clientID] = *(unsigned*)&client->decryptbuf[0x10];
			break;
		case 0x25: //穿上装备
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned*)&client->decryptbuf[0x0C];
				EquipItem(itemid, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x26: //卸载装备
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned*)&client->decryptbuf[0x0C];
				DeequipItem(itemid, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x27:
			// Use item 使用物品
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned*)&client->decryptbuf[0x0C];
				UseItem(itemid, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x28:
			// Mag feeding 喂养玛古
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				magid = *(unsigned*)&client->decryptbuf[0x0C];
				itemid = *(unsigned*)&client->decryptbuf[0x10];
				FeedMag(magid, itemid, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x29:
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				// Client dropping or destroying an item...
				itemid = *(unsigned*)&client->decryptbuf[0x0C];
				count = *(unsigned*)&client->decryptbuf[0x10];
				if (client->drop_item == itemid)
				{
					client->drop_item = 0;
					drop = 1;
				}
				else
					drop = 0;
				if (itemid != 0xFFFFFFFF)
					DeleteItemFromClient(itemid, count, drop, client); // Item
				else
					DeleteMesetaFromClient(count, drop, client); // Meseta
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x2A:
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))//处于游戏房间中且匹配颜色ID
			{
				// Client dropping complete item 客户端掉落完整物品
				itemid = *(unsigned*)&client->decryptbuf[0x10];
				DeleteItemFromClient(itemid, 0, 1, client);
			}
			else
			{
				dont_send = 1;
				printf("客户端没有掉落东西出来");
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x3E:
		case 0x3F:
			l->clientx[client->clientID] = *(unsigned*)&client->decryptbuf[0x14];
			l->clienty[client->clientID] = *(unsigned*)&client->decryptbuf[0x1C];
			break;
		case 0x40:
		case 0x42:
			l->clientx[client->clientID] = *(unsigned*)&client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned*)&client->decryptbuf[0x10];
			client->dead = 0;
			break;
		case 0x47:
		case 0x48:
			if (l->floor[client->clientID] == 0)
			{
				Send1A(L"Using techniques on Pioneer 2 is disallowed.", client, 84);
				dont_send = 1;
				client->todc = 1;
				break;
			}
			else
				if (client->clientID == client->decryptbuf[0x0A])
				{
					if (client->equip_flags & DROID_FLAG)
					{
						Send1A(L"Androids cannot cast techniques.", client, 85);
						dont_send = 1;
						client->todc = 1;
					}
					else
					{
						if (client->decryptbuf[0x0C] > 18)
						{
							Send1A(L"Invalid technique cast.", client, 86);
							dont_send = 1;
							client->todc = 1;
						}
						else
						{
							if (max_tech_level[client->decryptbuf[0x0C]][client->character._class] == -1)
							{
								Send1A(L"You cannot cast that technique.", client, 87);
								dont_send = 1;
								client->todc = 1;
							}
						}
					}
				}
			break;
		case 0x4D:
			// Decrease mag sync on death 减少死亡的MAG同步 
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				client->dead = 1;
				for (ch = 0;ch < client->character.inventoryUse;ch++)
				{
					if ((client->character.inventory[ch].item.data[0] == 0x02) &&
						(client->character.inventory[ch].flags & 0x08))
					{
						if (client->character.inventory[ch].item.data2[0] >= 5)
							client->character.inventory[ch].item.data2[0] -= 5;
						else
							client->character.inventory[ch].item.data2[0] = 0;
					}
				}
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x68:
			// Telepipe check 传送门检查
			if ((client->lobbyNum < 0x10) || (client->decryptbuf[0x0E] > 0x11))
			{
				Send1A(L"Incorrect telepipe.", client, 88);
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0x74:
			// W/S (throttle for spam)垃圾邮件限制
			dont_send = 1;
			if ((((unsigned)servertime - client->command_cooldown[0x74]) >= 1) && (!stfu(client->guildcard)))
			{
				client->command_cooldown[0x74] = (unsigned)servertime;
				ws_ok = 1;
				ws_data = *(unsigned short*)&client->decryptbuf[0x0C];
				if ((ws_data == 0) || (ws_data > 3))
					ws_ok = 0;
				ws_data = *(unsigned short*)&client->decryptbuf[0x0E];
				if ((ws_data == 0) || (ws_data > 3))
					ws_ok = 0;
				if (ws_ok)
				{
					for (ch = 0;ch < client->decryptbuf[0x0C];ch++)
					{
						ws_data = *(unsigned short*)&client->decryptbuf[0x10 + (ch * 2)];
						if (ws_data > 0x685)
						{
							if (ws_data > 0x697)
								ws_ok = 0;
							else
							{
								ws_data -= 0x68C;
								if (ws_data >= l->lobbyCount)
									ws_ok = 0;
							}
						}
					}
					ws_data = 0xFFFF;
					for (ch = client->decryptbuf[0x0C];ch < 8;ch++)
						*(unsigned short*)&client->decryptbuf[0x10 + (ch * 2)] = ws_data;

					if (ws_ok)
					{
						if (client->lobbyNum < 0x10)
							max_send = 12;
						else
							max_send = 4;

						for (ch = 0;ch < max_send;ch++)
						{
							if ((l->slot_use[ch]) && (l->client[ch]))
							{
								ignored = 0;
								lClient = l->client[ch];
								for (ch2 = 0;ch2 < lClient->ignore_count;ch2++)
								{
									if (lClient->ignore_list[ch2] == client->guildcard)
									{
										ignored = 1;
										break;
									}
								}
								if ((!ignored) && (lClient->guildcard != client->guildcard))
								{
									cipher_ptr = &lClient->server_cipher;
									encryptcopy(lClient, &client->decryptbuf[0x00], size);
								}
							}
						}
					}
				}
			}
			break;
		case 0x75:
		{
			// Set player flag 设置玩家旗帜

			unsigned short flag;

			if (!client->decryptbuf[0x0E])
			{
				flag = *(unsigned short*)&client->decryptbuf[0x0C];
				if (flag < 1024)
					client->character.quest_data1[((unsigned)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - (flag & 0x07));
			}
		}
		break;
		case 0xC0:
			// Client selling item 客户端卖出物品
			if ((client->lobbyNum > 0x0F) && (l->floor[client->clientID] == 0))
			{
				itemid = *(unsigned*)&client->decryptbuf[0x0C];
				for (ch = 0;ch < client->character.inventoryUse;ch++)
				{
					if (client->character.inventory[ch].item.itemid == itemid)
					{
						count = client->decryptbuf[0x10];
						if ((count > 1) && (client->character.inventory[ch].item.data[0] != 0x03))
							client->todc = 1;
						else
						{
							shop_price = GetShopPrice(&client->character.inventory[ch]) * count;
							DeleteItemFromClient(itemid, count, 0, client);
							if (!client->todc)
							{
								client->character.meseta += shop_price;
								if (client->character.meseta > 999999)
									client->character.meseta = 999999;
							}
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0xC3:
			// Client setting coordinates for stack drop 为堆栈丢弃的客户端设置坐标
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				client->drop_area = *(unsigned*)&client->decryptbuf[0x0C];
				client->drop_coords = *(long long*)&client->decryptbuf[0x10];
				client->drop_item = *(unsigned*)&client->decryptbuf[0x18];
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0xC4:
			// Inventory sort 背包分类
			if (client->lobbyNum > 0x0F)
			{
				dont_send = 1;
				SortClientItems(client);
			}
			else
				client->todc = 1;
			break;
		case 0xC5:
			// Visiting hospital 医院观光
			if (client->lobbyNum > 0x0F)
				DeleteMesetaFromClient(10, 0, client);
			else
				client->todc = 1;
			break;
		case 0xC6:
			// Steal Exp 掠夺经验
			if (client->lobbyNum > 0x0F)
			{
				unsigned exp_percent = 0;
				unsigned exp_to_add;
				unsigned char special = 0;

				mid = *(unsigned short*)&client->decryptbuf[0x0C];
				mid &= 0xFFF;
				if (mid < 0xB50)
				{
					for (ch = 0;ch < client->character.inventoryUse;ch++)
					{
						if ((client->character.inventory[ch].flags & 0x08) &&
							(client->character.inventory[ch].item.data[0] == 0x00))
						{
							if ((client->character.inventory[ch].item.data[1] < 0x0A) &&
								(client->character.inventory[ch].item.data[2] < 0x05))
								special = (client->character.inventory[ch].item.data[4] & 0x1F);
							else
								if ((client->character.inventory[ch].item.data[1] < 0x0D) &&
									(client->character.inventory[ch].item.data[2] < 0x04))
									special = (client->character.inventory[ch].item.data[4] & 0x1F);
								else
									special = special_table[client->character.inventory[ch].item.data[1]]
									[client->character.inventory[ch].item.data[2]];
							switch (special)
							{
							case 0x09:
								// Master's
								exp_percent = 8;
								break;
							case 0x0A:
								// Lord's
								exp_percent = 10;
								break;
							case 0x0B:
								// King's
								exp_percent = 12;
								if ((l->difficulty == 0x03) &&
									(client->equip_flags & DROID_FLAG))
									exp_percent += 30;
								break;
							}
							break;
						}
					}

					if (exp_percent)
					{
						exp_to_add = (l->mapData[mid].exp * exp_percent) / 100L;
						if (exp_to_add > 80)  // Limit the amount of exp stolen to 80
							exp_to_add = 80;
						AddExp(exp_to_add, client);
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC7:
			// Charge action 充能动作
			if (client->lobbyNum > 0x0F)
			{
				int meseta;

				meseta = *(int*)&client->decryptbuf[0x0C];
				if (meseta > 0)
				{
					if (client->character.meseta >= (unsigned)meseta)
						DeleteMesetaFromClient(meseta, 0, client);
					else
						DeleteMesetaFromClient(client->character.meseta, 0, client);
				}
				else
				{
					meseta = -meseta;
					client->character.meseta += (unsigned)meseta;
					if (client->character.meseta > 999999)
						client->character.meseta = 999999;
				}
			}
			else
				client->todc = 1;
			break;
		case 0xC8:
			// Monster is dead
			if (client->lobbyNum > 0x0F)
			{
				mid = *(unsigned short*)&client->decryptbuf[0x0A];
				mid &= 0xFFF;
				if (mid < 0xB50)
				{
					if (l->monsterData[mid].dead[client->clientID] == 0)
					{
						l->monsterData[mid].dead[client->clientID] = 1;

						double EXP_RATE = EXPERIENCE_RATE;
						double EIC = 0;
						for (ch = 0; ch < client->character.inventoryUse; ch++)
						{
							if (client->character.inventory[ch].flags & 0x08)
							{
								switch (client->character.inventory[ch].item.data[0])
								{
								case 0x01:
									if ((client->character.inventory[ch].item.data[1] == 0x01))
									{
										EIC += armor_eic_table[client->character.inventory[ch].item.data[2]];
									}
									else if ((client->character.inventory[ch].item.data[1] == 0x02)) {
										//item.data[2]];
										EIC += barrier_eic_table[client->character.inventory[ch].item.data[2]];
									}
									break;
								default:
									break;
								}
							}
						}
						EXP_RATE += EIC / 100;
						client->encryptbuf[0x0A] = (unsigned char)EXP_RATE;
						XP = l->mapData[mid].exp * (int)EXP_RATE;

						FILE* fp;
						fpos_t position = 0;
						int killCount = 0;
						if ((fp = fopen("killCount.txt", "r+")) == NULL)
						{
							printf("击杀统计文件 killCount.txt 不存在.\n");
						}
						else
						{
							fscanf(fp, "%d", &killCount);
							killCount++;
							position = 0;
							fsetpos(fp, &position);
							fprintf(fp, "%d", killCount);
							fclose(fp);
						}

						if (!l->quest_loaded)
						{
							mid_mismatch = 0;

							switch (l->episode)
							{
							case 0x01:
								if (l->floor[client->clientID] > 10)
								{
									switch (l->floor[client->clientID])
									{
									case 11:
										// Dragon
										if (l->mapData[mid].base != 192)
											mid_mismatch = 1;
										break;
									case 12:
										// De Rol Le
										if (l->mapData[mid].base != 193)
											mid_mismatch = 1;
										break;
									case 13:
										// Vol Opt
										if ((l->mapData[mid].base != 197) && (l->mapData[mid].base != 194))
											mid_mismatch = 1;
										break;
									case 14:
										// Dark Falz
										if (l->mapData[mid].base != 200)
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x02:
								if (l->floor[client->clientID] > 10)
								{
									switch (l->floor[client->clientID])
									{
									case 12:
										// Gal Gryphon
										if (l->mapData[mid].base != 192)
											mid_mismatch = 1;
										break;
									case 13:
										// Olga Flow
										if (l->mapData[mid].base != 202)
											mid_mismatch = 1;
										break;
									case 14:
										// Barba Ray
										if (l->mapData[mid].base != 203)
											mid_mismatch = 1;
										break;
									case 15:
										// Gol Dragon
										if (l->mapData[mid].base != 204)
											mid_mismatch = 1;
										break;
									}
								}
								break;
							case 0x03:
								if ((l->floor[client->clientID] == 9) &&
									(l->mapData[mid].base != 280) &&
									(l->mapData[mid].base != 281) &&
									(l->mapData[mid].base != 41))
									mid_mismatch = 1;
								break;
							}

							if (mid_mismatch)
							{
								//网络不同步造成断线
								SendEE(L"Client/server data synchronization error.  Please reinstall your client and all patches.", client, 135);
								client->todc = 1;
							}
						}

						//debug ("mid death: %u  base: %u, skin: %u, reserved11: %f, exp: %u", mid, l->mapData[mid].base, l->mapData[mid].skin, l->mapData[mid].reserved11, XP);

						if (client->decryptbuf[0x10] != 1) // Not the last player who hit?
							//XP = (XP * 77) / 100L;//ヒューマー、レイマー、レイマール、フォーマー、フォマールの場合経験値20%UP
							switch (client->character._class)
							{
							case CLASS_HUMAR:
							case CLASS_RAMAR:
							case CLASS_RAMARL:
							case CLASS_FOMARL:
							case CLASS_FOMAR:
								XP = (XP * 120) / 100L;
								break;
							}

						if (client->character.level < 199)
							AddExp(XP, client);

						// Increase kill counters for SJS, Lame d'Argent, Limiter and Swordsman Lore

						for (ch = 0;ch < client->character.inventoryUse;ch++)
						{
							if (client->character.inventory[ch].flags & 0x08)
							{
								counter = 0;
								switch (client->character.inventory[ch].item.data[0])
								{
								case 0x00:
									if ((client->character.inventory[ch].item.data[1] == 0x33) ||
										(client->character.inventory[ch].item.data[1] == 0xAB))
										counter = 1;
									break;
								case 0x01:
									if ((client->character.inventory[ch].item.data[1] == 0x03) &&
										((client->character.inventory[ch].item.data[2] == 0x4D) ||
											(client->character.inventory[ch].item.data[2] == 0x4F)))
										counter = 1;
									break;
								default:
									break;
								}
								if (counter)
								{
									counter = *(unsigned short*)&client->character.inventory[ch].item.data[10];
									if (counter < 0x8000)
										counter = 0x8000;
									counter++;
									*(unsigned short*)&client->character.inventory[ch].item.data[10] = counter;
								}
							}
						}
					}
				}
			}
			else
				client->todc = 1;
			break;
		case 0xCC:
			// Exchange item for team points 转换物品为公会点数
		{
			unsigned deleteid;

			deleteid = *(unsigned*)&client->decryptbuf[0x0C];
			DeleteItemFromClient(deleteid, 1, 0, client);
			if (!client->todc)
			{
				SendB0(L"Item donated to server.", client, 2);
			}
		}
		break;
		case 0xCF:
			if ((l->battle) && (client->mode))
			{
				// 战斗重新开始...
				//
				// 如果第1条规则 我们将把角色数据备份复制到角色数组, 否则
				// 我们会重置角色...
				//
				for (ch = 0;ch < 4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						lClient = l->client[ch];
						switch (lClient->mode)
						{
							//对战模式挑战模式
						case 0x01: //用于对战模式
						case 0x02: //用于挑战模式
								   // Copy character backup 备份角色数据
							if (lClient->character_backup && lClient->guildcard) //请求恢复备份
							{
								memcpy(&lClient->character, lClient->character_backup, sizeof(lClient->character));
								memcpy(&lClient->character.challengeData, lClient->challenge_data.challengeData, 320);
								memcpy(&lClient->character.battleData, lClient->battle_data.battleData, 92);
							}
							if (lClient->mode == 0x02)
							{
								for (ch2 = 0;ch2 < lClient->character.inventoryUse;ch2++)
								{
									if (lClient->character.inventory[ch2].item.data[0] == 0x02)
										lClient->character.inventory[ch2].in_use = 0;
								}
								CleanUpInventory(lClient);
								for (ch2 = 0;ch2 < lClient->character.inventoryUse;ch2++)
								{
									if (lClient->character.inventory[ch2].item.data[0] == 0x02)
										lClient->character.inventory[ch2].in_use = 0x01;
								}
								lClient->character.meseta = 0;
							}
							break;
						case 0x03:
							// Wipe items and reset level. 擦除物品和重设等级
							for (ch2 = 0;ch2 < 30;ch2++)
								lClient->character.inventory[ch2].in_use = 0;
							CleanUpInventory(lClient);
							lClient->character.level = 0;
							lClient->character.XP = 0;
							lClient->character.ATP = *(unsigned short*)&startingData[(lClient->character._class * 14)];
							lClient->character.MST = *(unsigned short*)&startingData[(lClient->character._class * 14) + 2];
							lClient->character.EVP = *(unsigned short*)&startingData[(lClient->character._class * 14) + 4];
							lClient->character.HP = *(unsigned short*)&startingData[(lClient->character._class * 14) + 6];
							lClient->character.DFP = *(unsigned short*)&startingData[(lClient->character._class * 14) + 8];
							lClient->character.ATA = *(unsigned short*)&startingData[(lClient->character._class * 14) + 10];
							if (l->battle_level > 1)
								SkipToLevel(l->battle_level - 1, lClient, 1);
							lClient->character.meseta = 0;
							break;
						default:
							printf("出现未知模式");
							break;
						}
					}
				}
				// Reset boxes and monsters...重设盒子和怪物的数据
				memset(&l->boxHit, 0, 0xB50); // Reset box and monster data 重设盒子和美赛塔的数据
				memset(&l->monsterData, 0, sizeof(l->monsterData)); // Reset boxes and monsters...重设盒子和怪物的数据
			}
			break;
		case 0xD2: //似乎和对战模式有关
			WriteLog("有待探索这里 1.");
			// Gallon seems to write to this area... 加仑似乎在这个区域写东西 
			//dont_send = 1; Sancaros 12.23 取消一个注释
			if (client->lobbyNum > 0x0F)
			{
				unsigned qofs;

				WriteLog("有待探索这里 2.");
				qofs = *(unsigned*)&client->decryptbuf[0x0C];
				if (qofs < 23)
				{
					qofs *= 4;
					*(unsigned*)&client->character.battleData[qofs] = *(unsigned*)&client->decryptbuf[0x10];
					memcpy(&client->encryptbuf[0x00], &client->decryptbuf[0x00], 0x14);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->decryptbuf[0x00], 0x14);
				}
			}
			else
			{
				dont_send = 1;
				WriteLog("有待探索这里 3.");
				client->todc = 1;
			}
			break;
		case 0xD5:
			// Exchange an item 兑换一个物品
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				INVENTORY_ITEM work_item;
				unsigned compare_item = 0, ci;

				memset(&work_item, 0, sizeof(INVENTORY_ITEM));
				memcpy(&work_item.item.data[0], &client->decryptbuf[0x0C], 3);
				DeleteFromInventory(&work_item, 1, client);

				if (!client->todc)
				{
					memset(&work_item, 0, sizeof(INVENTORY_ITEM));
					memcpy(&compare_item, &client->decryptbuf[0x20], 3);
					for (ci = 0; ci < quest_numallows; ci++)
					{
						if (compare_item == quest_allow[ci])
						{
							memcpy(&work_item.item.data[0], &client->decryptbuf[0x20], 3);
							work_item.item.itemid = l->playerItemID[client->clientID];
							l->playerItemID[client->clientID]++;
							AddToInventory(&work_item, 1, 0, client);
							memset(&client->encryptbuf[0x00], 0, 0x0C);
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							// BLAH :)
							*(unsigned short*)&client->encryptbuf[0x08] = *(unsigned short*)&client->decryptbuf[0x34];
							cipher_ptr = &client->server_cipher;
							encryptcopy(client, &client->encryptbuf[0x00], 0x0C);
							break;
						}
					}
					if (!work_item.item.itemid)
					{
						Send1A(L"Attempting to exchange for disallowed item.", client, 89);
						client->todc = 1;
					}
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD7:
			// Trade PDs for an item from Hopkins' dad  用PDs换霍普金斯爸爸的东西 
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				INVENTORY_ITEM work_item;
				unsigned ci, compare_item = 0;

				memset(&work_item, 0, sizeof(INVENTORY_ITEM));
				memcpy(&compare_item, &client->decryptbuf[0x0C], 3);
				for (ci = 0; ci < (sizeof(gallons_shop_hopkins) / 4); ci += 2)
				{
					if (compare_item == gallons_shop_hopkins[ci])
					{
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						work_item.item.data[2] = 0x00;
						break;
					}
				}
				if (work_item.item.data[0] == 0x03)
				{
					DeleteFromInventory(&work_item, 0xFF, client); // Delete all Photon Drops 删除所有PD
					if (!client->todc)
					{
						memcpy(&work_item.item.data[0], &client->decryptbuf[0x0C], 12);
						*(unsigned*)&work_item.item.data2[0] = *(unsigned*)&client->decryptbuf[0x18];
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory(&work_item, 1, 0, client);
						memset(&client->encryptbuf[0x00], 0, 0x0C);
						// I guess this is a sort of action confirmed by server thing...
						//我想这是一种由服务器确认的行为
						// Also starts an animation and sound... with the wrong values, the camera pans weirdly...
						// 也开始一个动画和声音。。。用错误的值，相机奇怪地平移。。。 
						client->encryptbuf[0x00] = 0x0C;
						client->encryptbuf[0x02] = 0xAB;
						client->encryptbuf[0x03] = 0x01;
						*(unsigned short*)&client->encryptbuf[0x08] = *(unsigned short*)&client->decryptbuf[0x20];
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &client->encryptbuf[0x00], 0x0C);
					}
				}
				else
				{
					Send1A(L"No photon drops in user's inventory\nwhen encountering exchange command.", client, 90);
					dont_send = 1;
					client->todc = 1;
				}
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0xD8:
			// Add attribute to S-rank weapon (not implemented yet) 为S级武器添加属性（尚未实现）  挑战模式
			break;
		case 0xD9:
			// Momoka Item Exchange  Momoka物品交换 
		{
			unsigned compare_item, ci;
			unsigned itemid = 0;
			INVENTORY_ITEM add_item;

			dont_send = 1;

			if (client->lobbyNum > 0x0F)
			{
				compare_item = 0x00091203;
				for (ci = 0; ci < client->character.inventoryUse; ci++)
				{
					if (*(unsigned*)&client->character.inventory[ci].item.data[0] == compare_item)
					{
						itemid = client->character.inventory[ci].item.itemid;
						break;
					}
				}
				if (!itemid)
				{
					memset(&client->encryptbuf[0x00], 0, 8);
					client->encryptbuf[0x00] = 0x08;
					client->encryptbuf[0x02] = 0x23;
					client->encryptbuf[0x04] = 0x01;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 8);
				}
				else
				{
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					compare_item = *(unsigned*)&client->decryptbuf[0x20];
					for (ci = 0; ci < quest_numallows; ci++)
					{
						if (compare_item == quest_allow[ci])
						{
							*(unsigned*)&add_item.item.data[0] = *(unsigned*)&client->decryptbuf[0x20];
							break;
						}
					}
					if (*(unsigned*)&add_item.item.data[0] == 0)
					{
						client->todc = 1;
						Send1A(L"Requested item not allowed.", client, 91);
					}
					else
					{
						DeleteItemFromClient(itemid, 1, 0, client);
						memset(&client->encryptbuf[0x00], 0, 0x18);
						client->encryptbuf[0x00] = 0x18;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDB;
						client->encryptbuf[0x09] = 0x06;
						client->encryptbuf[0x0C] = 0x01;
						*(unsigned*)&client->encryptbuf[0x10] = itemid;
						client->encryptbuf[0x14] = 0x01;
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &client->encryptbuf[0x00], 0x18);

						// Let everybody else know that item no longer exists...

						memset(&client->encryptbuf[0x00], 0, 0x14);
						client->encryptbuf[0x00] = 0x14;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0x29;
						client->encryptbuf[0x09] = 0x05;
						client->encryptbuf[0x0A] = client->clientID;
						*(unsigned*)&client->encryptbuf[0x0C] = itemid;
						client->encryptbuf[0x10] = 0x01;
						SendToLobby(l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard);
						add_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory(&add_item, 1, 0, client);
						memset(&client->encryptbuf[0x00], 0, 8);
						client->encryptbuf[0x00] = 0x08;
						client->encryptbuf[0x02] = 0x23;
						client->encryptbuf[0x04] = 0x00;
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &client->encryptbuf[0x00], 8);
					}
				}
			}
		}
		break;
		case 0xDA:

			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				INVENTORY_ITEM work_item, work_item2;
				unsigned ci, ai,
					compare_itemid = 0, compare_item1 = 0, compare_item2 = 0, num_attribs = 0;
				char attrib_add;

				memcpy(&compare_item1, &client->decryptbuf[0x0C], 3);
				compare_itemid = *(unsigned*)&client->decryptbuf[0x20];
				for (ci = 0; ci < client->character.inventoryUse; ci++)
				{
					memcpy(&compare_item2, &client->character.inventory[ci].item.data[0], 3);
					if ((client->character.inventory[ci].item.itemid == compare_itemid) &&
						(compare_item1 == compare_item2) && (client->character.inventory[ci].item.data[0] == 0x00))
					{
						memset(&work_item, 0, sizeof(INVENTORY_ITEM));
						work_item.item.data[0] = 0x03;
						work_item.item.data[1] = 0x10;
						if (client->decryptbuf[0x2C])
							work_item.item.data[2] = 0x01;
						else
							work_item.item.data[2] = 0x00;
						// Copy before shift
						memcpy(&work_item2, &client->character.inventory[ci], sizeof(INVENTORY_ITEM));
						DeleteFromInventory(&work_item, client->decryptbuf[0x28], client);
						if (!client->todc)
						{
							switch (client->decryptbuf[0x28])
							{
							case 0x01:
								// 1 PS = 30%
								if (client->decryptbuf[0x2C])
									attrib_add = 30;
								break;
							case 0x04:
								// 4 PDs = 1%
								attrib_add = 1;
								break;
							case 0x14:
								// 20 PDs = 5%
								attrib_add = 5;
								break;
							default:
								attrib_add = 0;
								break;
							}
							ai = 0;
							if ((work_item2.item.data[6] > 0x00) &&
								(!(work_item2.item.data[6] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[6] == client->decryptbuf[0x24])
									ai = 7;
							}
							if ((work_item2.item.data[8] > 0x00) &&
								(!(work_item2.item.data[8] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[8] == client->decryptbuf[0x24])
									ai = 9;
							}
							if ((work_item2.item.data[10] > 0x00) &&
								(!(work_item2.item.data[10] & 128)))
							{
								num_attribs++;
								if (work_item2.item.data[10] == client->decryptbuf[0x24])
									ai = 11;
							}
							if (ai)
							{
								// Attribute already on weapon, increase it
								(char)work_item2.item.data[ai] += attrib_add;
								if (work_item2.item.data[ai] > 100)
									work_item2.item.data[ai] = 100;
							}
							else
							{
								// Attribute not on weapon, add it if there isn't already 3 attributes
								if (num_attribs < 3)
								{
									work_item2.item.data[6 + (num_attribs * 2)] = client->decryptbuf[0x24];
									(char)work_item2.item.data[7 + (num_attribs * 2)] = attrib_add;
								}
							}
							DeleteItemFromClient(work_item2.item.itemid, 1, 0, client);
							memset(&client->encryptbuf[0x00], 0, 0x14);
							client->encryptbuf[0x00] = 0x14;
							client->encryptbuf[0x02] = 0x60;
							client->encryptbuf[0x08] = 0x29;
							client->encryptbuf[0x09] = 0x05;
							client->encryptbuf[0x0A] = client->clientID;
							*(unsigned*)&client->encryptbuf[0x0C] = work_item2.item.itemid;
							client->encryptbuf[0x10] = 0x01;
							SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x14, 0);
							AddToInventory(&work_item2, 1, 0, client);
							memset(&client->encryptbuf[0x00], 0, 0x0C);
							// Don't know...
							client->encryptbuf[0x00] = 0x0C;
							client->encryptbuf[0x02] = 0xAB;
							client->encryptbuf[0x03] = 0x01;
							*(unsigned short*)&client->encryptbuf[0x08] = *(unsigned short*)&client->decryptbuf[0x30];
							cipher_ptr = &client->server_cipher;
							encryptcopy(client, &client->encryptbuf[0x00], 0x0C);
						}
						break;
					}
				}
				if (client->todc)
					dont_send = 1;
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0xDE: //祝你好运
		{
			unsigned compare_item, ci;
			unsigned itemid = 0;
			INVENTORY_ITEM add_item;

			dont_send = 1;

			if (client->lobbyNum > 0x0F)
			{
				compare_item = 0x00031003;
				for (ci = 0; ci < client->character.inventoryUse; ci++)
				{
					if (*(unsigned*)&client->character.inventory[ci].item.data[0] == compare_item)
					{
						itemid = client->character.inventory[ci].item.itemid;
						break;
					}
				}
				if (!itemid)
				{
					memset(&client->encryptbuf[0x00], 0, 0x2C);
					client->encryptbuf[0x00] = 0x2C;
					client->encryptbuf[0x02] = 0x24;
					client->encryptbuf[0x04] = 0x01;
					client->encryptbuf[0x08] = client->decryptbuf[0x0E];
					client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
					for (ci = 0;ci < 8;ci++)
						client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x2C);
				}
				else
				{
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					*(unsigned*)&add_item.item.data[0] = good_luck[mt_lrand() % (sizeof(good_luck) >> 2)];
					DeleteItemFromClient(itemid, 1, 0, client);
					memset(&client->encryptbuf[0x00], 0, 0x18);
					client->encryptbuf[0x00] = 0x18;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDB;
					client->encryptbuf[0x09] = 0x06;
					client->encryptbuf[0x0C] = 0x01;
					*(unsigned*)&client->encryptbuf[0x10] = itemid;
					client->encryptbuf[0x14] = 0x01;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x18);

					// Let everybody else know that item no longer exists...

					memset(&client->encryptbuf[0x00], 0, 0x14);
					client->encryptbuf[0x00] = 0x14;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x29;
					client->encryptbuf[0x09] = 0x05;
					client->encryptbuf[0x0A] = client->clientID;
					*(unsigned*)&client->encryptbuf[0x0C] = itemid;
					client->encryptbuf[0x10] = 0x01;
					SendToLobby(l, 4, &client->encryptbuf[0x00], 0x14, client->guildcard);
					add_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory(&add_item, 1, 0, client);
					memset(&client->encryptbuf[0x00], 0, 0x2C);
					client->encryptbuf[0x00] = 0x2C;
					client->encryptbuf[0x02] = 0x24;
					client->encryptbuf[0x04] = 0x00;
					client->encryptbuf[0x08] = client->decryptbuf[0x0E];
					client->encryptbuf[0x0A] = client->decryptbuf[0x0D];
					for (ci = 0;ci < 8;ci++)
						client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x2C);
				}
			}
		}
		break;
		case 0xE1:
		{
			// Gallon's Plan opcode

			INVENTORY_ITEM work_item;
			unsigned ch, compare_item1, compare_item2, pt_itemid;

			compare_item2 = 0;
			compare_item1 = 0x041003;
			pt_itemid = 0;

			for (ch = 0;ch < client->character.inventoryUse;ch++)
			{
				memcpy(&compare_item2, &client->character.inventory[ch].item.data[0], 3);
				if (compare_item1 == compare_item2)
				{
					pt_itemid = client->character.inventory[ch].item.itemid;
					break;
				}
			}

			if (!pt_itemid)
				client->todc = 1;

			if (!client->todc)
			{
				memset(&work_item, 0, sizeof(INVENTORY_ITEM));
				switch (client->decryptbuf[0x0E])
				{
				case 0x01:
					// Kan'ei Tsuho
					DeleteItemFromClient(pt_itemid, 10, 0, client); // Delete Photon Tickets
					if (!client->todc)
					{
						work_item.item.data[0] = 0x00;
						work_item.item.data[1] = 0xD5;
						work_item.item.data[2] = 0x00;
					}
					break;
				case 0x02:
					// Lollipop
					DeleteItemFromClient(pt_itemid, 15, 0, client); // Delete Photon Tickets
					if (!client->todc)
					{
						work_item.item.data[0] = 0x00;
						work_item.item.data[1] = 0x0A;
						work_item.item.data[2] = 0x07;
					}
					break;
				case 0x03:
					// Stealth Suit
					DeleteItemFromClient(pt_itemid, 20, 0, client); // Delete Photon Tickets
					if (!client->todc)
					{
						work_item.item.data[0] = 0x01;
						work_item.item.data[1] = 0x01;
						work_item.item.data[2] = 0x57;
					}
					break;
				default:
					client->todc = 1;
					break;
				}

				if (!client->todc)
				{
					memset(&client->encryptbuf[0x00], 0, 0x18);
					client->encryptbuf[0x00] = 0x18;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDB;
					client->encryptbuf[0x09] = 0x06;
					client->encryptbuf[0x0C] = 0x01;
					*(unsigned*)&client->encryptbuf[0x10] = pt_itemid;
					client->encryptbuf[0x14] = 0x05 + (client->decryptbuf[0x0E] * 5);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x18);
					work_item.item.itemid = l->playerItemID[client->clientID];
					l->playerItemID[client->clientID]++;
					AddToInventory(&work_item, 1, 0, client);
					// Gallon's Plan result
					memset(&client->encryptbuf[0x00], 0, 0x10);
					client->encryptbuf[0x00] = 0x10;
					client->encryptbuf[0x02] = 0x25;
					client->encryptbuf[0x08] = client->decryptbuf[0x10];
					client->encryptbuf[0x0A] = 0x3C;
					client->encryptbuf[0x0B] = 0x08;
					client->encryptbuf[0x0D] = client->decryptbuf[0x0E];
					client->encryptbuf[0x0E] = 0x9A;
					client->encryptbuf[0x0F] = 0x08;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x10);
				}
			}
		}
		break;
		case 0x17:
		case 0x18:
			boss_floor = 0;
			switch (l->episode)
			{
			case 0x01:
				if ((l->floor[client->clientID] > 10) && (l->floor[client->clientID] < 15))
					boss_floor = 1;
				break;
			case 0x02:
				if ((l->floor[client->clientID] > 11) && (l->floor[client->clientID] < 16))
					boss_floor = 1;
				break;
			case 0x03:
				if (l->floor[client->clientID] == 9)
					boss_floor = 1;
				break;
			}
			if (!boss_floor)
				dont_send = 1;
			break;
		default:
			// Temporary 临时查看
			//debug("0x60 指令322222222222222来自 %u: \n", client->guildcard);
			//display_packet(&client->decryptbuf[0x00], size);
			//WriteLog("0x60 指令322222222222222来自\n %u   :  \n%s", client->guildcard, (char*)&dp[0]);
			break;
		}
		if ((!dont_send) && (!client->todc))
		{
			if (client->lobbyNum < 0x10)
				SendToLobby(client->lobby, 12, &client->decryptbuf[0], size, client->guildcard);
			else
				SendToLobby(client->lobby, 4, &client->decryptbuf[0], size, client->guildcard);
		}
	}
}

unsigned long ExpandDropRate(unsigned char pc)
{
	long shift = ((pc >> 3) & 0x1F) - 4;
	if (shift < 0) shift = 0;
	return ((2 << (unsigned long)shift) * ((pc & 7) + 7));
}

void GenerateRandomAttributes(unsigned char sid, GAME_ITEM* i, LOBBY* l, BANANA* client)
{
	unsigned ch, num_percents, max_percent, meseta, do_area, r;
	PTDATA* ptd;
	int rare;
	unsigned area;
	unsigned did_area[6] = { 0 };
	char percent;

	if ((!l) || (!i))
		return;

	if (l->episode == 0x01)
		ptd = &pt_tables_ep1[sid][l->difficulty];

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];

	if (l->episode == 0x03)
		ptd = &pt_tables_ep4[sid][l->difficulty];

	if ((l->episode == 0x01) && (l->challenge))
		ptd = &pt_tables_ep1c[sid][l->difficulty];

	if ((l->episode == 0x02) && (l->challenge))
		ptd = &pt_tables_ep2c[sid][l->difficulty];

	area = 0;

	switch (l->episode)
	{
	case 0x01:
		switch (l->floor[client->clientID])
		{
		case 11:
			// dragon
			area = 3;
			break;
		case 12:
			// de rol
			area = 6;
			break;
		case 13:
			// vol opt
			area = 8;
			break;
		case 14:
			// falz
			area = 10;
			break;
		default:
			area = l->floor[client->clientID];
			break;
		}
		break;
	case 0x02:
		switch (l->floor[client->clientID])
		{
		case 14:
			// barba ray
			area = 3;
			break;
		case 15:
			// gol dragon
			area = 6;
			break;
		case 12:
			// gal gryphon
			area = 9;
			break;
		case 13:
			// olga flow
			area = 10;
			break;
		default:
			// could be tower
			if (l->floor[client->clientID] <= 11)
				area = ep2_rtremap[(l->floor[client->clientID] * 2) + 1];
			else
				area = 0x0A;
			break;
		}
		break;
	case 0x03:
		area = l->floor[client->clientID] + 1;
		break;
	}

	if (!area)
		return;

	if (area > 10)
		area = 10;

	area--; // Our tables are zero based.

	switch (i->item.data[0])
	{
	case 0x00:
		rare = 0;
		if (i->item.data[1] > 0x0C)
			rare = 1;
		else
			if ((i->item.data[1] > 0x09) && (i->item.data[2] > 0x03))
				rare = 1;
			else
				if ((i->item.data[1] < 0x0A) && (i->item.data[2] > 0x04))
					rare = 1;
		if (!rare)
		{

			r = 100 - (mt_lrand() % 100);

			if ((r > ptd->element_probability[area]) && (ptd->element_ranking[area]))
			{
				i->item.data[4] = 0xFF;
				while (i->item.data[4] == 0xFF) // give a special
					i->item.data[4] = elemental_table[(12 * (ptd->element_ranking[area] - 1)) + (mt_lrand() % 12)];
			}
			else
				i->item.data[4] = 0;

			if (i->item.data[4])
				i->item.data[4] |= 0x80; // untekked

										 // Add a grind

			if (l->episode == 0x01)
				ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			if (l->episode == 0x02)
				ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			if (l->episode == 0x03)
				ch = power_patterns_ep4[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				ch = power_patterns_ep1c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				ch = power_patterns_ep2c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

			i->item.data[3] = (unsigned char)ch;
		}
		else
			i->item.data[4] |= 0x80; // rare

		num_percents = 0;

		if ((i->item.data[1] == 0x33) ||
			(i->item.data[1] == 0xAB)) // SJS and Lame max 2 percents
			max_percent = 2;
		else
			max_percent = 3;

		for (ch = 0;ch < max_percent;ch++)
		{
			if (l->episode == 0x01)
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x03)
				do_area = attachment_ep4[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				do_area = attachment_ep1c[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				do_area = attachment_ep2c[sid][l->difficulty][area][mt_lrand() % 4096];

			if ((do_area) && (!did_area[do_area]))
			{
				did_area[do_area] = 1;
				i->item.data[6 + (num_percents * 2)] = (unsigned char)do_area;
				if (l->episode == 0x01)
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x02)
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x03)
					percent = percent_patterns_ep4[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x01 && l->challenge)
					percent = percent_patterns_ep1c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x02 && l->challenge)
					percent = percent_patterns_ep2c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				percent -= 2;
				percent *= 5;
				(char)i->item.data[6 + (num_percents * 2) + 1] = percent;
				num_percents++;
			}
		}
		break;
	case 0x01:
		switch (i->item.data[1])
		{
		case 0x01:
			// Armor variance
			r = mt_lrand() % 11;
			if (r < 7)
			{
				if (armor_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char)(mt_lrand() % (armor_dfpvar_table[i->item.data[2]] + 1));
				if (armor_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char)(mt_lrand() % (armor_evpvar_table[i->item.data[2]] + 1));
			}

			// Slots
			if (l->episode == 0x01)
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x02)
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x03)
				i->item.data[5] = slots_ep4[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				i->item.data[5] = slots_ep1c[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				i->item.data[5] = slots_ep2c[sid][l->difficulty][mt_lrand() % 4096];

			break;
		case 0x02:
			// Shield variance
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char)(mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char)(mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data[2] = 0x05;
		i->item.data[4] = 0xF4;
		i->item.data[5] = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
		if (i->item.data[1] == 0x02) // Technique
		{
			if (l->episode == 0x01)
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02)
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x03)
				i->item.data[4] = tech_drops_ep4[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				i->item.data[4] = tech_drops_ep1c[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				i->item.data[4] = tech_drops_ep2c[sid][l->difficulty][area][mt_lrand() % 4096];

			i->item.data[2] = (unsigned char)ptd->tech_levels[i->item.data[4]][area * 2];
			if (ptd->tech_levels[i->item.data[4]][(area * 2) + 1] > ptd->tech_levels[i->item.data[4]][area * 2])
				i->item.data[2] += (unsigned char)mt_lrand() % ((ptd->tech_levels[i->item.data[4]][(area * 2) + 1] - ptd->tech_levels[i->item.data[4]][(area * 2)]) + 1);
		}
		if (stackable_table[i->item.data[1]])
			i->item.data[5] = 0x01;
		break;
	case 0x04:
		// meseta
		meseta = ptd->box_meseta[area][0];
		if (ptd->box_meseta[area][1] > ptd->box_meseta[area][0])
			meseta += mt_lrand() % ((ptd->box_meseta[area][1] - ptd->box_meseta[area][0]) + 1);
		*(unsigned*)&i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
}

void GenerateCommonItem(int item_type, int is_enemy, unsigned char sid, GAME_ITEM* i, LOBBY* l, BANANA* client)
{
	unsigned ch, num_percents, item_set, meseta, do_area, r, eq_type;
	unsigned short ch2;
	PTDATA* ptd;
	unsigned area, fl;
	unsigned did_area[6] = { 0 };
	char percent;

	if ((!l) || (!i))
		return;

	if (l->episode == 0x01)
		ptd = &pt_tables_ep1[sid][l->difficulty];

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];

	if (l->episode == 0x03)
		ptd = &pt_tables_ep4[sid][l->difficulty];

	if (l->episode == 0x01 && l->challenge)
		ptd = &pt_tables_ep1c[sid][l->difficulty];

	if (l->episode == 0x02 && l->challenge)
		ptd = &pt_tables_ep2c[sid][l->difficulty];

	area = 0;

	switch (l->episode)
	{
	case 0x01:
		switch (l->floor[client->clientID])
		{
		case 11:
			// dragon
			area = 3;
			break;
		case 12:
			// de rol
			area = 6;
			break;
		case 13:
			// vol opt
			area = 8;
			break;
		case 14:
			// falz
			area = 10;
			break;
		default:
			area = l->floor[client->clientID];
			break;
		}
		break;
	case 0x02:
		switch (l->floor[client->clientID])
		{
		case 14:
			// barba ray
			area = 3;
			break;
		case 15:
			// gol dragon
			area = 6;
			break;
		case 12:
			// gal gryphon
			area = 9;
			break;
		case 13:
			// olga flow
			area = 10;
			break;
		default:
			// could be tower
			if (l->floor[client->clientID] <= 11)
				area = ep2_rtremap[(l->floor[client->clientID] * 2) + 1];
			else
				area = 0x0A;
			break;
		}
		break;
	case 0x03:
		area = l->floor[client->clientID] + 1;
		break;
	}

	if (!area)
		return;

	if ((l->battle) && (l->quest_loaded))
	{
		if ((!l->battle_level) || (l->battle_level > 5))
			area = 6; // Use mines equipment for rule #1, #5 and #8
		else
			area = 3; // Use caves 1 equipment for all other rules...
	}

	if (area > 10)
		area = 10;

	fl = area;
	area--; // Our tables are zero based.

	if (is_enemy)
	{
		if ((mt_lrand() % 100) > 40)
			item_set = 3;
		else
		{
			switch (item_type)
			{
			case 0x00:
				item_set = 0;
				break;
			case 0x01:
				item_set = 1;
				break;
			case 0x02:
				item_set = 1;
				break;
			case 0x03:
				item_set = 1;
				break;
			default:
				item_set = 3;
				break;
			}
		}
	}
	else
	{
		if ((l->meseta_boost) && ((mt_lrand() % 100) > 25))
			item_set = 4; // Boost amount of meseta dropped during rules #4 and #5
		else
		{
			if (item_type == 0xFF)
				item_set = common_table[mt_lrand() % 100000];
			else
				item_set = item_type;
		}
	}

	memset(&i->item.data[0], 0, 12);
	memset(&i->item.data2[0], 0, 4);

	switch (item_set)
	{
	case 0x00:
		// Weapon

		if (l->episode == 0x01)
			ch2 = weapon_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

		if (l->episode == 0x02)
			ch2 = weapon_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];

		if (l->episode == 0x03)
			ch2 = weapon_drops_ep4[sid][l->difficulty][area][mt_lrand() % 4096];

		if (l->episode == 0x01 && l->challenge)
			ch2 = weapon_drops_ep1c[sid][l->difficulty][area][mt_lrand() % 4096];

		if (l->episode == 0x02 && l->challenge)
			ch2 = weapon_drops_ep2c[sid][l->difficulty][area][mt_lrand() % 4096];

		i->item.data[1] = ch2 & 0xFF;
		i->item.data[2] = ch2 >> 8;

		if (i->item.data[1] > 0x09)
		{
			if (i->item.data[2] > 0x03)
				i->item.data[2] = 0x03;
		}
		else
		{
			if (i->item.data[2] > 0x04)
				i->item.data[2] = 0x04;
		}

		r = 100 - (mt_lrand() % 100);

		if ((r > ptd->element_probability[area]) && (ptd->element_ranking[area]))
		{
			i->item.data[4] = 0xFF;
			while (i->item.data[4] == 0xFF) // give a special
				i->item.data[4] = elemental_table[(12 * (ptd->element_ranking[area] - 1)) + (mt_lrand() % 12)];
		}
		else
			i->item.data[4] = 0;

		if (i->item.data[4])
			i->item.data[4] |= 0x80; // untekked

		num_percents = 0;

		for (ch = 0;ch < 3;ch++)
		{
			if (l->episode == 0x01)
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x03)
				do_area = attachment_ep4[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x01&&l->challenge)
				do_area = attachment_ep1c[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				do_area = attachment_ep2c[sid][l->difficulty][area][mt_lrand() % 4096];

			if ((do_area) && (!did_area[do_area]))
			{
				did_area[do_area] = 1;
				i->item.data[6 + (num_percents * 2)] = (unsigned char)do_area;
				if (l->episode == 0x01)
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x02)
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x03)
					percent = percent_patterns_ep4[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x01 && l->challenge)
					percent = percent_patterns_ep1c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				if (l->episode == 0x02 && l->challenge)
					percent = percent_patterns_ep2c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

				percent -= 2;
				percent *= 5;
				(char)i->item.data[6 + (num_percents * 2) + 1] = percent;
				num_percents++;
			}
		}

		// Add a grind

		if (l->episode == 0x01)
			ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		if (l->episode == 0x02)
			ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		if (l->episode == 0x03)
			ch = power_patterns_ep4[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		if (l->episode == 0x01 && l->challenge)
			ch = power_patterns_ep1c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		if (l->episode == 0x02 && l->challenge)
			ch = power_patterns_ep2c[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

		i->item.data[3] = (unsigned char)ch;

		break;
	case 0x01:
		r = mt_lrand() % 100;
		if (!is_enemy)
		{
			// Probabilities (Box): Armor 41%, Shields 41%, Units 18%
			if (r > 82)
				eq_type = 3;
			else
				if (r > 59)
					eq_type = 2;
				else
					eq_type = 1;
		}
		else
			eq_type = (unsigned)item_type;

		switch (eq_type)
		{
		case 0x01:
			// Armor
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x01;
			i->item.data[2] = (unsigned char)(fl / 3L) + (5 * l->difficulty) + (mt_lrand() % (((unsigned char)fl / 2L) + 2));
			if (i->item.data[2] > 0x17)
				i->item.data[2] = 0x17;
			r = mt_lrand() % 11;
			if (r < 7)
			{
				if (armor_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char)(mt_lrand() % (armor_dfpvar_table[i->item.data[2]] + 1));
				if (armor_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char)(mt_lrand() % (armor_evpvar_table[i->item.data[2]] + 1));
			}

			// Slots
			if (l->episode == 0x01)
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x02)
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x03)
				i->item.data[5] = slots_ep4[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				i->item.data[5] = slots_ep1c[sid][l->difficulty][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				i->item.data[5] = slots_ep2c[sid][l->difficulty][mt_lrand() % 4096];

			break;
		case 0x02:
			// Shield
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x02;
			i->item.data[2] = (unsigned char)(fl / 3L) + (4 * l->difficulty) + (mt_lrand() % (((unsigned char)fl / 2L) + 2));
			if (i->item.data[2] > 0x14)
				i->item.data[2] = 0x14;
			r = mt_lrand() % 11;
			if (r < 2)
			{
				if (barrier_dfpvar_table[i->item.data[2]])
					i->item.data[6] = (unsigned char)(mt_lrand() % (barrier_dfpvar_table[i->item.data[2]] + 1));
				if (barrier_evpvar_table[i->item.data[2]])
					i->item.data[8] = (unsigned char)(mt_lrand() % (barrier_evpvar_table[i->item.data[2]] + 1));
			}
			break;
		case 0x03:
			// unit
			i->item.data[0] = 0x01;
			i->item.data[1] = 0x03;
			if ((ptd->unit_level[area] >= 2) && (ptd->unit_level[area] <= 8))
			{
				i->item.data[2] = 0xFF;
				while (i->item.data[2] == 0xFF)
					i->item.data[2] = unit_drop[mt_lrand() % ((ptd->unit_level[area] - 1) * 10)];
			}
			else
			{
				i->item.data[0] = 0x03;
				i->item.data[1] = 0x00;
				i->item.data[2] = 0x00; // Give a monomate when failed to look up unit.
			}
			break;
		}
		break;
	case 0x02:
		// Mag
		i->item.data[0] = 0x02;
		i->item.data[2] = 0x05;
		i->item.data[4] = 0xF4;
		i->item.data[5] = 0x01;
		i->item.data2[3] = mt_lrand() % 0x11;
		break;
	case 0x03:
		// Tool
		if (l->episode == 0x01)
			*(unsigned*)&i->item.data[0] = tool_remap[tool_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096]];

		if (l->episode == 0x02)
			*(unsigned*)&i->item.data[0] = tool_remap[tool_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096]];

		if (l->episode == 0x03)
			*(unsigned*)&i->item.data[0] = tool_remap[tool_drops_ep4[sid][l->difficulty][area][mt_lrand() % 4096]];

		if (l->episode == 0x01 && l->challenge)
			*(unsigned*)&i->item.data[0] = tool_remap[tool_drops_ep1c[sid][l->difficulty][area][mt_lrand() % 4096]];

		if (l->episode == 0x02 && l->challenge)
			*(unsigned*)&i->item.data[0] = tool_remap[tool_drops_ep2c[sid][l->difficulty][area][mt_lrand() % 4096]];

		if (i->item.data[1] == 0x02) // Technique
		{
			if (l->episode == 0x01)
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02)
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x03)
				i->item.data[4] = tech_drops_ep4[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x01 && l->challenge)
				i->item.data[4] = tech_drops_ep1c[sid][l->difficulty][area][mt_lrand() % 4096];

			if (l->episode == 0x02 && l->challenge)
				i->item.data[4] = tech_drops_ep2c[sid][l->difficulty][area][mt_lrand() % 4096];

			i->item.data[2] = (unsigned char)ptd->tech_levels[i->item.data[4]][area * 2];
			if (ptd->tech_levels[i->item.data[4]][(area * 2) + 1] > ptd->tech_levels[i->item.data[4]][area * 2])
				i->item.data[2] += (unsigned char)mt_lrand() % ((ptd->tech_levels[i->item.data[4]][(area * 2) + 1] - ptd->tech_levels[i->item.data[4]][(area * 2)]) + 1);
		}
		if (stackable_table[i->item.data[1]])
			i->item.data[5] = 0x01;
		break;
	case 0x04:
		// Meseta
		i->item.data[0] = 0x04;
		meseta = ptd->box_meseta[area][0];
		if (ptd->box_meseta[area][1] > ptd->box_meseta[area][0])
			meseta += mt_lrand() % ((ptd->box_meseta[area][1] - ptd->box_meseta[area][0]) + 1);
		*(unsigned*)&i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
	i->item.itemid = l->itemID++;
}

void Send62(BANANA* client) //挑战模式有涉及
{
	BANANA* lClient;
	unsigned bank_size, bank_use;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t, maxt;
	unsigned itemid;
	int dont_send = 1;
	LOBBY* l;
	unsigned rt_index = 0;
	unsigned rare_lookup, rare_rate, rare_item,
		rare_roll, box_rare, ch, itemNum;
	unsigned short mid, count;
	unsigned char* rt_table;
	unsigned char* rt_table2;
	unsigned meseta;
	unsigned DAR;
	unsigned floor_check = 0;
	SHOP* shopp;
	SHOP_ITEM* shopi;
	PTDATA* ptd;
	MAP_BOX* mb;

	if (!client->lobby)
		return;

	l = (LOBBY*)client->lobby;
	// don't support target @ 0x02
	t = client->decryptbuf[0x04];
	if (client->lobbyNum < 0x10)
		maxt = 12;
	else
		maxt = 4;

	size = *(unsigned short*)&client->decryptbuf[0x00];
	sizecheck = client->decryptbuf[0x09];

	sizecheck *= 4;
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug("Client sent a 0x62 packet whose sizecheck != size.\n");
		debug("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
			size, client->decryptbuf[0x09], sizecheck);
		client->decryptbuf[0x09] = ((size / 4) - 2);
	}

	switch (client->decryptbuf[0x08])
	{
	case 0x06:
		// Send guild card 发送名片
		if ((size == 0x0C) && (t < maxt))
		{
			if ((l->slot_use[t]) && (l->client[t]))
			{
				lClient = l->client[t];
				PrepGuildCard(client->guildcard, lClient->guildcard);
				memset(&PacketData[0], 0, 0x114);//初始化 276 字节的数据
				sprintf(&PacketData[0x00], "\x14\x01\x60"); //4字节的十六进制 未弄懂是啥
				PacketData[0x03] = 0x00;
				PacketData[0x04] = 0x00;
				PacketData[0x08] = 0x06;
				PacketData[0x09] = 0x43;
				*(unsigned*)&PacketData[0x0C] = client->guildcard;
				memcpy(&PacketData[0x10], &client->character.name[0], 24);
				memcpy(&PacketData[0x60], &client->character.friendText[0], 176);
				PacketData[0x110] = 0x01; // ?
				PacketData[0x112] = (char)client->character.sectionID;
				PacketData[0x113] = (char)client->character._class;
				cipher_ptr = &lClient->server_cipher;
				encryptcopy(lClient, &PacketData[0], 0x114);
			}
		}
		break;
	case 0x5A:
		if (client->lobbyNum > 0x0F)
		{
			itemid = *(unsigned*)&client->decryptbuf[0x0C];
			if (AddItemToClient(itemid, client) == 1)
			{
				memset(&PacketData[0], 0, 16);
				PacketData[0x00] = 0x14;
				PacketData[0x02] = 0x60;
				PacketData[0x08] = 0x59;
				PacketData[0x09] = 0x03;
				PacketData[0x0A] = (unsigned char)client->clientID;
				PacketData[0x0E] = client->decryptbuf[0x10];
				PacketData[0x0C] = (unsigned char)client->clientID;
				*(unsigned*)&PacketData[0x10] = itemid;
				SendToLobby(client->lobby, 4, &PacketData[0x00], 0x14, 0);
			}
		}
		else
			client->todc = 1;
		break;
	case 0x60:
		// Requesting a drop from a monster.
		if (client->lobbyNum > 0x0F)
		{
			if (!l->drops_disabled)
			{
				mid = *(unsigned short*)&client->decryptbuf[0x0E];
				mid &= 0xFFF;

				if ((mid < 0xB50) && (l->monsterData[mid].drop == 0))
				{
					if (l->episode == 0x01)
						ptd = &pt_tables_ep1[client->character.sectionID][l->difficulty];

					if (l->episode == 0x02)
						ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];

					if (l->episode == 0x03)
						ptd = &pt_tables_ep4[client->character.sectionID][l->difficulty];

					if (l->episode == 0x01 && l->challenge)
						ptd = &pt_tables_ep1c[client->character.sectionID][l->difficulty];

					if (l->episode == 0x02 && l->challenge)
						ptd = &pt_tables_ep2c[client->character.sectionID][l->difficulty];

					if ((l->episode == 0x01) && (client->decryptbuf[0x0D] == 35) &&
						(l->mapData[mid].rt_index == 34))
						rt_index = 35; // Save Death Gunner index...
					else
						rt_index = l->mapData[mid].rt_index; // Use map's index instead of what the client says...

					if (rt_index < 0x64)
					{
						if (l->episode == 0x03)
						{
							if (rt_index < 0x16)
							{
								meseta = ep4_rtremap[(rt_index * 2) + 1];
								// Past a certain point is Episode II data...
								if (meseta > 0x2F)
									ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
							}
							else
								meseta = 0;
						}
						else
							meseta = rt_index;
						if ((l->episode == 0x03) &&
							(rt_index >= 19) &&
							(rt_index <= 21))
							DAR = 1;
						else
						{
							if ((ptd->enemy_dar[meseta] == 100) || (l->redbox))
								DAR = 1;
							else
							{

								DAR = 100 - ptd->enemy_dar[meseta];
								if ((mt_lrand() % 100) >= DAR)
									DAR = 1;
								else
									DAR = 0;
							}
						}
					}
					else
						DAR = 0;

					if (DAR)
					{
						if (rt_index < 0x64)
						{
							rt_index += ((0x1400 * l->difficulty) + (client->character.sectionID * 0x200));
							switch (l->episode)
							{
							case 0x02:
								rare_lookup = rt_tables_ep2[rt_index];
								break;
							case 0x03:
								rare_lookup = rt_tables_ep4[rt_index];
								break;
							default:
								rare_lookup = rt_tables_ep1[rt_index];
								break;
							}
							rare_rate = ExpandDropRate(rare_lookup & 0xFF);
							rare_item = rare_lookup >> 8;
							rare_roll = mt_lrand();
							//debug ("rare_roll = %u", rare_roll );
							if (((rare_lookup & 0xFF) != 0) && ((rare_roll < rare_rate) || (l->redbox)))
							{
								// Drop a rare item
								itemNum = free_game_item(l);
								memset(&l->gameItem[itemNum].item.data[0], 0, 12);
								memset(&l->gameItem[itemNum].item.data2[0], 0, 4);
								memcpy(&l->gameItem[itemNum].item.data[0], &rare_item, 3);
								GenerateRandomAttributes(client->character.sectionID, &l->gameItem[itemNum], l, client);
								l->gameItem[itemNum].item.itemid = l->itemID++;
							}
							else
							{
								// Drop a common item
								itemNum = free_game_item(l);
								if (((mt_lrand() % 100) < 60) || (ptd->enemy_drop < 0))
								{
									memset(&l->gameItem[itemNum].item.data[0], 0, 12);
									memset(&l->gameItem[itemNum].item.data2[0], 0, 4);
									l->gameItem[itemNum].item.data[0] = 0x04;
									rt_index = meseta;
									meseta = ptd->enemy_meseta[rt_index][0];
									if (ptd->enemy_meseta[rt_index][1] > ptd->enemy_meseta[rt_index][0])
										meseta += mt_lrand() % ((ptd->enemy_meseta[rt_index][1] - ptd->enemy_meseta[rt_index][0]) + 1);
									*(unsigned*)&l->gameItem[itemNum].item.data2[0] = meseta;
									l->gameItem[itemNum].item.itemid = l->itemID++;
								}
								else
								{
									rt_index = meseta;
									GenerateCommonItem(ptd->enemy_drop[rt_index], 1, client->character.sectionID, &l->gameItem[itemNum], l, client);
								}
							}

							if (l->gameItem[itemNum].item.itemid != 0)
							{
								if (l->gameItemCount < MAX_SAVED_ITEMS)
									l->gameItemList[l->gameItemCount++] = itemNum;
								memset(&PacketData[0x00], 0, 16);
								PacketData[0x00] = 0x30;
								PacketData[0x01] = 0x00;
								PacketData[0x02] = 0x60;
								PacketData[0x03] = 0x00;
								PacketData[0x08] = 0x5F;
								PacketData[0x09] = 0x0D;
								*(unsigned*)&PacketData[0x0C] = *(unsigned*)&client->decryptbuf[0x0C];
								memcpy(&PacketData[0x10], &client->decryptbuf[0x10], 10);
								memcpy(&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12);
								*(unsigned*)&PacketData[0x28] = l->gameItem[itemNum].item.itemid;
								*(unsigned*)&PacketData[0x2C] = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
								SendToLobby(client->lobby, 4, &PacketData[0], 0x30, 0);
							}
						}
					}
					l->monsterData[mid].drop = 1;
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0x6F: //这里两个和并的指令
	case 0x71:
		if ((client->lobbyNum > 0x0F) && (t < maxt))
		{
			if (l->leader == client->clientID)
			{
				if ((l->slot_use[t]) && (l->client[t]))
				{
					if (l->client[t]->bursting == 1)
						dont_send = 0; // More user joining game stuff...
				}
			}
		}
		break;
	case 0xA2:
		if (client->lobbyNum > 0x0F)
		{
			if (!l->drops_disabled)
			{
				// box drop
				mid = *(unsigned short*)&client->decryptbuf[0x0E];
				mid &= 0xFFF;

				if ((mid < 0xB50) && (l->boxHit[mid] == 0))
				{
					box_rare = 0;
					mb = 0;

					//debug("任务已载入: %i", l->quest_loaded);

					if ((l->quest_loaded) && ((unsigned)l->quest_loaded <= numQuests))
					{
						QUEST* q;

						q = &quests[l->quest_loaded - 1];
						if (mid < q->max_objects)
							mb = (MAP_BOX*)&q->objectdata[(unsigned)(68 * mid) + 0x28];
					}
					else
						mb = &l->objData[mid];

					if (mb)
					{

						if (mb->flag1 == 0)
						{
							if (((mb->flag2 - FLOAT_PRECISION) < (float)1.00000) &&
								((mb->flag2 + FLOAT_PRECISION) > (float)1.00000))
							{
								// Fixed item alert!!!

								box_rare = 1;
								itemNum = free_game_item(l);
								if (((mb->flag3 - FLOAT_PRECISION) < (float)1.00000) &&
									((mb->flag3 + FLOAT_PRECISION) > (float)1.00000))
								{
									// Fully fixed!

									*(unsigned*)&l->gameItem[itemNum].item.data[0] = *(unsigned*)&mb->drop[0];

									// Not used... for now.
									l->gameItem[itemNum].item.data[3] = 0;

									if (l->gameItem[itemNum].item.data[0] == 0x04)
										GenerateCommonItem(0x04, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
									else
										if ((l->gameItem[itemNum].item.data[0] == 0x00) &&
											(l->gameItem[itemNum].item.data[1] == 0x00))
											GenerateCommonItem(0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
										else
										{
											memset(&l->gameItem[itemNum].item.data2[0], 0, 4);
											if (l->gameItem[itemNum].item.data[0] < 0x02)
												l->gameItem[itemNum].item.data[1]++; // Fix item offset
											GenerateRandomAttributes(client->character.sectionID, &l->gameItem[itemNum], l, client);
											l->gameItem[itemNum].item.itemid = l->itemID++;
										}
								}
								else
									GenerateCommonItem(mb->drop[0], 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
							}
						}
					}

					if (!box_rare)
					{
						switch (l->episode)
						{
						case 0x02:
							rt_table = (unsigned char*)&rt_tables_ep2[0];
							break;
						case 0x03:
							rt_table = (unsigned char*)&rt_tables_ep4[0];
							break;
						default:
							rt_table = (unsigned char*)&rt_tables_ep1[0];
							break;
						}
						rt_table += ((0x5000 * l->difficulty) + (client->character.sectionID * 0x800)) + 0x194;
						rt_table2 = rt_table + 0x1E;
						rare_item = 0;

						switch (l->episode)
						{
						case 0x01:
							switch (l->floor[client->clientID])
							{
							case 11:
								// dragon
								floor_check = 3;
								break;
							case 12:
								// de rol
								floor_check = 6;
								break;
							case 13:
								// vol opt
								floor_check = 8;
								break;
							case 14:
								// falz
								floor_check = 10;
								break;
							default:
								floor_check = l->floor[client->clientID];
								break;
							}
							break;
						case 0x02:
							switch (l->floor[client->clientID])
							{
							case 14:
								// barba ray
								floor_check = 3;
								break;
							case 15:
								// gol dragon
								floor_check = 6;
								break;
							case 12:
								// gal gryphon
								floor_check = 9;
								break;
							case 13:
								// olga flow
								floor_check = 10;
								break;
							default:
								// could be tower
								if (l->floor[client->clientID] <= 11)
									floor_check = ep2_rtremap[(l->floor[client->clientID] * 2) + 1];
								else
									floor_check = 10;
								break;
							}
							break;
						case 0x03:
							floor_check = l->floor[client->clientID];
							break;
						}

						for (ch = 0;ch < 30;ch++)
						{
							if (*rt_table == floor_check)
							{
								rare_rate = ExpandDropRate(*rt_table2);
								memcpy(&rare_item, &rt_table2[1], 3);
								rare_roll = mt_lrand();
								if ((rare_roll < rare_rate) || (l->redbox == 1))
								{
									box_rare = 1;
									itemNum = free_game_item(l);
									memset(&l->gameItem[itemNum].item.data[0], 0, 12);
									memset(&l->gameItem[itemNum].item.data2[0], 0, 4);
									memcpy(&l->gameItem[itemNum].item.data[0], &rare_item, 3);
									GenerateRandomAttributes(client->character.sectionID, &l->gameItem[itemNum], l, client);
									l->gameItem[itemNum].item.itemid = l->itemID++;
									break;
								}
							}
							rt_table++;
							rt_table2 += 0x04;
						}
					}

					if (!box_rare)
					{
						itemNum = free_game_item(l);
						GenerateCommonItem(0xFF, 0, client->character.sectionID, &l->gameItem[itemNum], l, client);
					}

					if (l->gameItem[itemNum].item.itemid != 0)
					{
						if (l->gameItemCount < MAX_SAVED_ITEMS)
							l->gameItemList[l->gameItemCount++] = itemNum;
						memset(&PacketData[0], 0, 16);
						PacketData[0x00] = 0x30;
						PacketData[0x01] = 0x00;
						PacketData[0x02] = 0x60;
						PacketData[0x03] = 0x00;
						PacketData[0x08] = 0x5F;
						PacketData[0x09] = 0x0D;
						*(unsigned*)&PacketData[0x0C] = *(unsigned*)&client->decryptbuf[0x0C];
						memcpy(&PacketData[0x10], &client->decryptbuf[0x10], 10);
						memcpy(&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12);
						*(unsigned*)&PacketData[0x28] = l->gameItem[itemNum].item.itemid;
						*(unsigned*)&PacketData[0x2C] = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
						SendToLobby(client->lobby, 4, &PacketData[0], 0x30, 0);
					}
					l->boxHit[mid] = 1;
				}
			}
		}
		break;
	case 0xA6:
		// 交易 (还未完成)Sancaros
		break;
	case 0xAE:
		// Chair info 座位信息
		if ((size == 0x18) && (client->lobbyNum < 0x10) && (t < maxt))
			dont_send = 0;
		break;
	case 0xB5:
		// Client requesting shop 客户端请求商店
		if (client->lobbyNum > 0x0F)
		{
			if ((l->floor[client->clientID] == 0)
				&& (client->decryptbuf[0x0C] < 0x03))
			{
				if (client->shoptype == 2) {//vipcommand
					client->doneshop[client->decryptbuf[0x0C]] = shopidx[181] + (333 * ((unsigned)client->decryptbuf[0x0C])) + (mt_lrand() % 333);
				}
				else {
					client->doneshop[client->decryptbuf[0x0C]] = shopidx[client->character.level] + (333 * ((unsigned)client->decryptbuf[0x0C])) + (mt_lrand() % 333);
				}
				shopp = &shops[client->doneshop[client->decryptbuf[0x0C]]];
				cipher_ptr = &client->server_cipher;
				encryptcopy(client, (unsigned char*)&shopp->packet_length, shopp->packet_length);
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB7:
		// Client buying an item 客户端购买一个物品
		if (client->lobbyNum > 0x0F)
		{
			if ((l->floor[client->clientID] == 0)
				&& (client->decryptbuf[0x10] < 0x03)
				&& (client->doneshop[client->decryptbuf[0x10]]))
			{
				if (client->decryptbuf[0x11] < shops[client->doneshop[client->decryptbuf[0x10]]].num_items)
				{
					shopi = &shops[client->doneshop[client->decryptbuf[0x10]]].item[client->decryptbuf[0x11]];
					if ((client->decryptbuf[0x12] > 1) && (shopi->data[0] != 0x03))
						client->todc = 1;
					else
						if (client->character.meseta < ((unsigned)client->decryptbuf[0x12] * shopi->price))
						{
							Send1A(L"Not enough meseta for purchase.", client, 92);
							client->todc = 1;
						}
						else
						{
							INVENTORY_ITEM i;

							memset(&i, 0, sizeof(INVENTORY_ITEM));
							memcpy(&i.item.data[0], &shopi->data[0], 12);
							// Update player item ID
							l->playerItemID[client->clientID] = *(unsigned*)&client->decryptbuf[0x0C];
							i.item.itemid = l->playerItemID[client->clientID]++;
							AddToInventory(&i, client->decryptbuf[0x12], 1, client);
							DeleteMesetaFromClient(shopi->price * (unsigned)client->decryptbuf[0x12], 0, client);
						}
				}
				else
					client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xB8:
		// Client is tekking a weapon.
		if (client->lobbyNum > 0x0F)
		{
			unsigned compare_item;

			INVENTORY_ITEM* i;

			i = NULL;

			compare_item = *(unsigned*)&client->decryptbuf[0x0C];

			for (ch = 0;ch < client->character.inventoryUse;ch++)
			{
				if ((client->character.inventory[ch].item.itemid == compare_item) &&
					(client->character.inventory[ch].item.data[0] == 0x00) &&
					(client->character.inventory[ch].item.data[4] & 0x80) &&
					(client->character.meseta >= 100))
				{
					char percent_mod;
					unsigned attrib;

					i = &client->character.inventory[ch];
					attrib = i->item.data[4] & ~(0x80);

					client->tekked = *i;

					if (attrib < 0x29)
					{
						client->tekked.item.data[4] = tekker_attributes[(attrib * 3) + 1];
						if ((mt_lrand() % 100) > 70)
							client->tekked.item.data[4] += mt_lrand() % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
					}
					else
						client->tekked.item.data[4] = 0;
					if ((mt_lrand() % 10) < 2) percent_mod = -10;
					else
						if ((mt_lrand() % 10) < 2) percent_mod = -5;
						else
							if ((mt_lrand() % 10) < 2) percent_mod = 5;
							else
								if ((mt_lrand() % 10) < 2) percent_mod = 10;
								else
									percent_mod = 0;
					if ((!(i->item.data[6] & 128)) && (i->item.data[7] > 0))
						(char)client->tekked.item.data[7] += percent_mod;
					if ((!(i->item.data[8] & 128)) && (i->item.data[9] > 0))
						(char)client->tekked.item.data[9] += percent_mod;
					if ((!(i->item.data[10] & 128)) && (i->item.data[11] > 0))
						(char)client->tekked.item.data[11] += percent_mod;
					DeleteMesetaFromClient(100, 0, client);
					memset(&client->encryptbuf[0x00], 0, 0x20);
					client->encryptbuf[0x00] = 0x20;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xB9;
					client->encryptbuf[0x09] = 0x08;
					client->encryptbuf[0x0A] = 0x79;
					memcpy(&client->encryptbuf[0x0C], &client->tekked.item.data[0], 16);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x20);
					break;
				}
			}

			if (i == NULL)
			{
				Send1A(L"Could not find item to Tek.", client, 93);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBA:
		// Client accepting tekked version of weapon.客户端接受武器的刻录版本
		if ((client->lobbyNum > 0x0F) && (client->tekked.item.itemid))
		{
			unsigned ch2;

			for (ch = 0;ch < 4;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					for (ch2 = 0;ch2 < l->client[ch]->character.inventoryUse;ch2++)
						if (l->client[ch]->character.inventory[ch2].item.itemid == client->tekked.item.itemid)
						{
							Send1A(L"Item duplication attempt!", client, 94);
							client->todc = 1;
							break;
						}
				}
			}

			for (ch = 0;ch < l->gameItemCount;l++)
			{
				itemNum = l->gameItemList[ch];
				if (l->gameItem[itemNum].item.itemid == client->tekked.item.itemid)
				{
					// Added to the game's inventory by the client...
					// Delete it and avoid duping...
					//被客户端添加到游戏目录中。。。
					//删除它并避免复制。。
					memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
					l->gameItemList[ch] = 0xFFFFFFFF;
					break;
				}
			}

			CleanUpGameInventory(l);

			if (!client->todc)
			{
				AddToInventory(&client->tekked, 1, 1, client);
				memset(&client->tekked, 0, sizeof(INVENTORY_ITEM));
			}
		}
		else
			client->todc = 1;
		break;
	case 0xBB:
		// Client accessing bank 客户端访问银行
		if (client->lobbyNum < 0x10)
			client->todc = 1;
		else
		{
			if ((l->floor[client->clientID] == 0) && (((unsigned)servertime - client->command_cooldown[0xBB]) >= 1))
			{
				client->command_cooldown[0xBB] = (unsigned)servertime;

				/* Which bank are we accessing? */

				client->bankAccess = client->bankType;

				if (client->bankAccess)
					memcpy(&client->character.bankUse, &client->common_bank, sizeof(BANK));
				else
					memcpy(&client->character.bankUse, &client->char_bank, sizeof(BANK));

				for (ch = 0;ch < client->character.bankUse;ch++)
					client->character.bankInventory[ch].itemid = l->bankItemID[client->clientID]++;
				memset(&client->encryptbuf[0x00], 0, 0x34);
				client->encryptbuf[0x02] = 0x6C;
				client->encryptbuf[0x08] = 0xBC;
				bank_size = 0x18 * (client->character.bankUse + 1);
				*(unsigned*)&client->encryptbuf[0x0C] = bank_size;
				bank_size += 4;
				*(unsigned short*)&client->encryptbuf[0x00] = (unsigned short)bank_size;
				bank_use = mt_lrand();
				*(unsigned*)&client->encryptbuf[0x10] = bank_use;
				bank_use = client->character.bankUse;
				*(unsigned*)&client->encryptbuf[0x14] = bank_use;
				*(unsigned*)&client->encryptbuf[0x18] = client->character.bankMeseta;
				if (client->character.bankUse)
					memcpy(&client->encryptbuf[0x1C], &client->character.bankInventory[0], sizeof(BANK_ITEM) * client->character.bankUse);
				cipher_ptr = &client->server_cipher;
				encryptcopy(client, &client->encryptbuf[0x00], bank_size);
			}
		}
		break;
	case 0xBD:
		if (client->lobbyNum < 0x10)
		{
			dont_send = 1;
			client->todc = 1;
		}
		else
		{
			if (l->floor[client->clientID] == 0)
			{
				switch (client->decryptbuf[0x14])
				{
				case 0x00:
					// Making a deposit
					itemid = *(unsigned*)&client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned*)&client->decryptbuf[0x10];

						if (client->character.meseta >= meseta)
						{
							client->character.bankMeseta += meseta;
							client->character.meseta -= meseta;
							if (client->character.bankMeseta > 999999)
								client->character.bankMeseta = 999999;
						}
						else
						{
							//网络不同步造成断线
							Send1A(L"Client/server data synchronization error.", client, 95);
							client->todc = 1;
						}
					}
					else
					{
						if (client->character.bankUse < 200)
						{
							// Depositing something else...
							count = client->decryptbuf[0x15];
							DepositIntoBank(itemid, count, client);
							if (!client->todc)
								SortBankItems(client);
						}
						else
						{
							Send1A(L"Can't deposit.  Bank is full.", client, 96);
							client->todc = 1;
						}
					}
					break;
				case 0x01:
					itemid = *(unsigned*)&client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned*)&client->decryptbuf[0x10];
						if (client->character.bankMeseta >= meseta)
						{
							client->character.bankMeseta -= meseta;
							client->character.meseta += meseta;
						}
						else
							client->todc = 1;
					}
					else
					{
						// Withdrawing something else...
						count = client->decryptbuf[0x15];
						WithdrawFromBank(itemid, count, client);
					}
					break;
				default:
					break;
				}

				/* Update bank. */

				if (client->bankAccess)
					memcpy(&client->common_bank, &client->character.bankUse, sizeof(BANK));
				else
					memcpy(&client->char_bank, &client->character.bankUse, sizeof(BANK));

			}
		}
		break;
	case 0xC1:
	case 0xC2:
	case 0xCD: //sancaros
	case 0xCE: //sancaros
		if (t < maxt)
		{
			// Team invite for C1 & C2, Master Transfer for CD & CE.
			if (size == 0x64)
				dont_send = 0;

			if (client->decryptbuf[0x08] == 0xC2)
			{
				unsigned gcn;

				gcn = *(unsigned*)&client->decryptbuf[0x0C];
				if ((client->decryptbuf[0x10] == 0x02) &&
					(client->guildcard == gcn))
					client->teamaccept = 1;
			}

			if (client->decryptbuf[0x08] == 0xCD)
			{
				if (client->character.privilegeLevel != 0x40)
				{
					dont_send = 1;
					Send01(L"You aren't the master of your team.", client, 133);
				}
				else
					client->masterxfer = 1;
			}
		}
		break;
	case 0xC9:
		if (client->lobbyNum > 0x0F)
		{
			INVENTORY_ITEM add_item;
			int meseta;

			if (l->quest_loaded)
			{
				meseta = *(int*)&client->decryptbuf[0x0C];
				if (meseta < 0)
				{
					meseta = -meseta;
					client->character.meseta -= (unsigned)meseta;
				}
				else
				{
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					add_item.item.data[0] = 0x04;
					*(unsigned*)&add_item.item.data2[0] = *(unsigned*)&client->decryptbuf[0x0C];
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory(&add_item, 1, 0, client);
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xCA:
		if (client->lobbyNum > 0x0F)
		{
			INVENTORY_ITEM add_item;

			if (l->quest_loaded)
			{
				unsigned ci, compare_item = 0;

				memset(&add_item, 0, sizeof(INVENTORY_ITEM));
				memcpy(&compare_item, &client->decryptbuf[0x0C], 3);
				for (ci = 0; ci < quest_numallows; ci++)
				{
					if (compare_item == quest_allow[ci])
					{
						add_item.item.data[0] = 0x01;
						break;
					}
				}
				if (add_item.item.data[0] == 0x01)
				{
					memcpy(&add_item.item.data[0], &client->decryptbuf[0x0C], 12);
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory(&add_item, 1, 0, client);
				}
				else
				{
					SendEE(L"You did not receive the quest reward.  The item requested is not on the allow list.  Your request and guild card have been logged for the server administrator.", client, 136);
					WriteLog("User %u attempted to claim quest reward %08x but item is not in the allow list.", client->guildcard, compare_item);
				}
			}
		}
		else
			client->todc = 1;
		break;
	case 0xD0:
		// Level up player?
		// Player to level @ 0x0A
		// Levels to gain @ 0x0C
		if ((t < maxt) && (l->battle) && (l->quest_loaded))
		{
			if ((client->decryptbuf[0x0A] < 4) && (l->client[client->decryptbuf[0x0A]]))
			{
				unsigned target_lv;

				lClient = l->client[client->decryptbuf[0x0A]];
				target_lv = lClient->character.level;
				target_lv += client->decryptbuf[0x0C];

				if (target_lv > 199)
					target_lv = 199;

				SkipToLevel(target_lv, lClient, 0);
			}
		}
		break;
	case 0xD6:
		// Wrap an item
		if (client->lobbyNum > 0x0F)
		{
			unsigned wrap_id;
			INVENTORY_ITEM backup_item;

			memset(&backup_item, 0, sizeof(INVENTORY_ITEM));
			wrap_id = *(unsigned*)&client->decryptbuf[0x18];

			for (ch = 0;ch < client->character.inventoryUse;ch++)
			{
				if (client->character.inventory[ch].item.itemid == wrap_id)
				{
					memcpy(&backup_item, &client->character.inventory[ch], sizeof(INVENTORY_ITEM));
					break;
				}
			}

			if (backup_item.item.itemid)
			{
				DeleteFromInventory(&backup_item, 1, client);
				if (!client->todc)
				{
					if (backup_item.item.data[0] == 0x02)
						backup_item.item.data2[2] |= 0x40; // Wrap a mag
					else
						backup_item.item.data[4] |= 0x40; // Wrap other
					AddToInventory(&backup_item, 1, 0, client);
				}
			}
			else
			{
				Send1A(L"Could not find item to wrap.", client, 97);
				client->todc = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xDF:
		if (client->lobbyNum > 0x0F)
		{
			if ((l->oneperson) && (l->quest_loaded) && (!l->drops_disabled))
			{
				INVENTORY_ITEM work_item;

				memset(&work_item, 0, sizeof(INVENTORY_ITEM));
				work_item.item.data[0] = 0x03;
				work_item.item.data[1] = 0x10;
				work_item.item.data[2] = 0x02;
				DeleteFromInventory(&work_item, 1, client);
				if (!client->todc)
					l->drops_disabled = 1;
			}
		}
		else
			client->todc = 1;
		break;
	case 0xE0: //各种难度的掉落
		if (client->lobbyNum > 0x0F)
		{
			if ((l->oneperson) && (l->quest_loaded) && (l->drops_disabled) && (!l->questE0))
			{
				unsigned bp, bpl, new_item;

				if (client->decryptbuf[0x0D] > 0x03)
					bpl = 1;
				else
					bpl = l->difficulty + 1;

				for (bp = 0;bp < bpl;bp++)
				{
					new_item = 0;

					switch (client->decryptbuf[0x0D])
					{
					case 0x00:
						// bp1 dorphon route
						switch (l->difficulty)
						{
						case 0x00:
							new_item = bp_dorphon_normal[mt_lrand() % (sizeof(bp_dorphon_normal) / 4)];
							break;
						case 0x01:
							new_item = bp_dorphon_hard[mt_lrand() % (sizeof(bp_dorphon_hard) / 4)];
							break;
						case 0x02:
							new_item = bp_dorphon_vhard[mt_lrand() % (sizeof(bp_dorphon_vhard) / 4)];
							break;
						case 0x03:
							new_item = bp_dorphon_ultimate[mt_lrand() % (sizeof(bp_dorphon_ultimate) / 4)];
							break;
						}
						break;
					case 0x01:
						// bp1 rappy route
						switch (l->difficulty)
						{
						case 0x00:
							new_item = bp_rappy_normal[mt_lrand() % (sizeof(bp_rappy_normal) / 4)];
							break;
						case 0x01:
							new_item = bp_rappy_hard[mt_lrand() % (sizeof(bp_rappy_hard) / 4)];
							break;
						case 0x02:
							new_item = bp_rappy_vhard[mt_lrand() % (sizeof(bp_rappy_vhard) / 4)];
							break;
						case 0x03:
							new_item = bp_rappy_ultimate[mt_lrand() % (sizeof(bp_rappy_ultimate) / 4)];
							break;
						}
						break;
					case 0x02:
						// bp1 zu route
						switch (l->difficulty)
						{
						case 0x00:
							new_item = bp_zu_normal[mt_lrand() % (sizeof(bp_zu_normal) / 4)];
							break;
						case 0x01:
							new_item = bp_zu_hard[mt_lrand() % (sizeof(bp_zu_hard) / 4)];
							break;
						case 0x02:
							new_item = bp_zu_vhard[mt_lrand() % (sizeof(bp_zu_vhard) / 4)];
							break;
						case 0x03:
							new_item = bp_zu_ultimate[mt_lrand() % (sizeof(bp_zu_ultimate) / 4)];
							break;
						}
						break;
					case 0x04:
						// bp2
						switch (l->difficulty)
						{
						case 0x00:
							new_item = bp2_normal[mt_lrand() % (sizeof(bp2_normal) / 4)];
							break;
						case 0x01:
							new_item = bp2_hard[mt_lrand() % (sizeof(bp2_hard) / 4)];
							break;
						case 0x02:
							new_item = bp2_vhard[mt_lrand() % (sizeof(bp2_vhard) / 4)];
							break;
						case 0x03:
							new_item = bp2_ultimate[mt_lrand() % (sizeof(bp2_ultimate) / 4)];
							break;
						}
						break;
					}
					l->questE0 = 1;
					memset(&client->encryptbuf[0x00], 0, 0x2C);
					client->encryptbuf[0x00] = 0x2C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x5D;
					client->encryptbuf[0x09] = 0x09;
					client->encryptbuf[0x0A] = 0xFF;
					client->encryptbuf[0x0B] = 0xFB;
					memcpy(&client->encryptbuf[0x0C], &client->decryptbuf[0x0C], 12);
					*(unsigned*)&client->encryptbuf[0x18] = new_item;
					*(unsigned*)&client->encryptbuf[0x24] = l->itemID;
					itemNum = free_game_item(l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
					*(unsigned*)&l->gameItem[itemNum].item.data[0] = new_item;
					if (new_item == 0x04)
					{
						new_item = pt_tables_ep1[client->character.sectionID][l->difficulty].enemy_meseta[0x2E][0];
						new_item += mt_lrand() % 100;
						*(unsigned*)&client->encryptbuf[0x28] = new_item;
						*(unsigned*)&l->gameItem[itemNum].item.data2[0] = new_item;
					}
					if (l->gameItem[itemNum].item.data[0] == 0x00)
					{
						l->gameItem[itemNum].item.data[4] = 0x80; // Untekked
						client->encryptbuf[0x1C] = 0x80;
					}
					l->gameItem[itemNum].item.itemid = l->itemID++;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x2C);
				}
			}
		}
		break;
	case 0xD1: //挑战模式需要完成的数据保存代码
	{
		if (client->lobbyNum > 0x0F)
		{
			if ((l->challenge) && (l->quest_loaded)) //进入对战模式触发
			{
				memcpy(&client->challenge_data.challengeData, &client->character.challengeData, 320);
				Send1A(L"Challenge date save test\n -By Sancaros", client, -1);
				//ShipSend04(0x03, client, logon);//30挑战
				WriteLog("测试挑战数据牌子的作用");

				INVENTORY_ITEM add_item;

				unsigned ci, compare_item = 0;

				memset(&add_item, 0, sizeof(INVENTORY_ITEM));
				memcpy(&compare_item, &client->decryptbuf[0x0C], 3);
				for (ci = 0; ci < quest_numallows; ci++)
				{
					if (compare_item == quest_allow[ci])
					{
						add_item.item.data[0] = 0x01;
						break;
					}
				}
				if (add_item.item.data[0] == 0x01)
				{
					memcpy(&add_item.item.data[0], &client->decryptbuf[0x0C], 12);
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory(&add_item, 1, 0, client);
				}
				else
				{
					memcpy(&add_item.item.data[0], &client->decryptbuf[0x0C], 12);
					add_item.item.itemid = l->itemID;
					l->itemID++;
					AddToInventory(&add_item, 1, 0, client);
					//SendEE(L"You did not receive the quest reward.  The item requested is not on the allow list.  Your request and guild card have been logged for the server administrator.", client, 136);
					WriteLog("玩家 %u 尝试获取任务物品 %08x 但不在允许的范围.", client->guildcard, compare_item);
				}
			}

			if ((l->battle) && (l->quest_loaded)) //进入对战模式触发
			{
				//尝试做第一次修复
				memcpy(&client->battle_data.battleData, &client->character.battleData, 92);
				//ShipSend04(0x04, client, logon);//30对战
				Send1A(L"Battle date save test\n -By Sancaros", client, -1);
				WriteLog("测试对战模式0xD1的作用");
			}
		}
		else {
			WriteLog("测试非挑战数据牌子的作用");
		}
	}
	break;
	default:
		if (client->lobbyNum > 0x0F)
		{
			WriteLog("62 指令 0x\"%02x\" 未被游戏进行接收处理. (数据如下)", client->decryptbuf[0x08]);
			packet_to_text(&client->decryptbuf[0x00], size);
			WriteLog("\n %s", &dp[0]);
		}
		break;
	}

	if ((dont_send == 0) && (!client->todc))
	{
		if ((l->slot_use[t]) && (l->client[t]))
		{
			lClient = l->client[t];
			cipher_ptr = &lClient->server_cipher;
			encryptcopy(lClient, &client->decryptbuf[0], size);
		}
	}
}

//建立房间归属的相关代码
void Send6D(BANANA* client)
{
	BANANA* lClient;
	unsigned short size;
	unsigned short sizecheck = 0;
	unsigned char t;
	int dont_send = 0;
	LOBBY* l;

	if (!client->lobby)
		return;

	size = *(unsigned short*)&client->decryptbuf[0x00];
	sizecheck = *(unsigned short*)&client->decryptbuf[0x0C];
	sizecheck += 8;

	if (size != sizecheck)
	{
		debug("Client sent a 0x6D packet whose sizecheck != size.\n");
		debug("Command: %02X | Size: %04X | Sizecheck: %04x\n", client->decryptbuf[0x08],
			size, sizecheck);
		dont_send = 1;
	}

	l = (LOBBY*)client->lobby;
	t = client->decryptbuf[0x04];
	if (t >= 0x04)
		dont_send = 1;

	if (!dont_send)
	{
		switch (client->decryptbuf[0x08])
		{
		case 0x70:
			if (client->lobbyNum > 0x0F)
			{
				if ((l->slot_use[t]) && (l->client[t]))
				{
					if (l->client[t]->bursting == 1)
					{
						unsigned ch;

						dont_send = 0; // It's cool to send them as long as this user is bursting.
									   // Let's reconstruct the 0x70 as much as possible.
									   //
									   // Could check guild card # here
									   //尽可能的让用户传送。
									   //让我们尽可能多地重建0x70。
									   //
									   //可以在这里查看帮会卡吗
						*(unsigned*)&client->decryptbuf[0x7C] = client->guildcard;
						// Check techniques...检查魔法,如果是机器人就不该有魔法
						if (!(client->equip_flags & DROID_FLAG))
						{
							for (ch = 0;ch < 19;ch++)
							{
								if ((char)client->decryptbuf[0xC4 + ch] > max_tech_level[ch][client->character._class])
								{
									(char)client->character.techniques[ch] = -1; // Unlearn broken technique.忘掉不该拥有的技能
									client->todc = 1;
								}
							}
							if (client->todc)
								Send1A(L"Technique data check failed.\n\nSome techniques have been unlearned.", client, 98);
						}
						memcpy(&client->decryptbuf[0xC4], &client->character.techniques, 20);
						// Could check character structure here 可以查看角色结构
						memcpy(&client->decryptbuf[0xD8], &client->character.gcString, 104);
						// Prevent crashing with NPC skins... 防止NPC皮肤崩溃
						if (client->character.skinFlag)
							memset(&client->decryptbuf[0x110], 0, 10);
						// Could check stats here 查看基础信息
						memcpy(&client->decryptbuf[0x148], &client->character.ATP, 36);
						// Could check inventory here 查看背包
						client->decryptbuf[0x16C] = client->character.inventoryUse;
						memcpy(&client->decryptbuf[0x170], &client->character.inventory[0], 30 * sizeof(INVENTORY_ITEM));
					}
				}
			}
			break;
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x72:
			if (client->lobbyNum > 0x0F)
			{
				if (l->leader == client->clientID)
				{
					if ((l->slot_use[t]) && (l->client[t]))
					{
						if (l->client[t]->bursting == 1)
							dont_send = 0; // It's cool to send them as long as this user is bursting and we're the leader. 只要用户传送，我们是队长，这是很酷的。 
					}
				}
			}
			break;
		default:
			dont_send = 1;
			break;
		}
	}

	if (dont_send == 0)
	{
		lClient = l->client[t];
		cipher_ptr = &lClient->server_cipher;
		encryptcopy(lClient, &client->decryptbuf[0], size);
	}
}

//Send01文本类代码
void Send01(const wchar_t* text, BANANA* client, int line) {
	if (line > -1 && wcslen(languageMessages[client->character.lang][line]))//此处修复了长度
	{
		text = languageMessages[client->character.lang][line];
	}
	unsigned short mesgOfs = 0x10;
	unsigned ch;

	memset(&PacketData[0], 0, 16);
	PacketData[mesgOfs++] = 0x09;
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x45;
	PacketData[mesgOfs++] = 0x00;
	for (ch = 0;ch < wcslen(text);ch++) //此处修复了长度
	{
		PacketData[mesgOfs++] = (unsigned char)text[ch];
		PacketData[mesgOfs++] = 0x00;
	}
	PacketData[mesgOfs++] = 0x00;
	PacketData[mesgOfs++] = 0x00;
	while (mesgOfs % 8)
		PacketData[mesgOfs++] = 0x00;
	*(unsigned short*)&PacketData[0] = mesgOfs;
	PacketData[0x02] = 0x01;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], mesgOfs);
}

unsigned char chatBuf[4000];
unsigned char cmdBuf[4000];
char* myCommand;
char* myArgs[64];

//Unicode转换为ASCII格式代码
char* Unicode_to_ASCII(unsigned short* ucs)
{
	char* s, c;

	s = (char*)&chatBuf[0];
	while (*ucs != 0x0000)
	{
		c = *(ucs++) & 0xFF;
		if ((c >= 0x20) && (c <= 0x80))
			*(s++) = c;
		else
			*(s++) = 0x20;
	}
	*(s++) = 0x00;
	return (char*)&chatBuf[0];
}

//控制台记录日志代码
void WriteLog(char* fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[MAX_GM_MESG_LEN];
	SYSTEMTIME rawtime;

	FILE* fp;

	GetLocalTime(&rawtime);
	va_start(args, fmt);
	strcpy(text + vsprintf(text, fmt, args), "\r\n");
	va_end(args);

	fp = fopen("ship.log", "a");
	if (!fp)
	{
		printf("无法保存日志文件 ship.log\n");
	}

	fprintf(fp, "[%u年%02u月%02u日, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
	fclose(fp);

	printf("[%u年%02u月%02u日, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
}

//控制台记录GM日志代码
void WriteGM(char* fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[MAX_GM_MESG_LEN];
	SYSTEMTIME rawtime;

	FILE* fp;

	GetLocalTime(&rawtime);
	va_start(args, fmt);
	strcpy(text + vsprintf(text, fmt, args), "\r\n");
	va_end(args);

	fp = fopen("gm.log", "a");
	if (!fp)
	{
		printf("无法写入GM操作日志 gm.log\n");
	}

	fprintf(fp, "[%u年%02u月%02u日, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
	fclose(fp);

	printf("[%u年%02u月%02u日, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
}

//定义角色文件字符长度
char character_file[255];

//Send06 大厅相关功能代码
void Send06(BANANA* client)
{
	FILE* fp;
	unsigned short chatsize;
	unsigned pktsize;
	unsigned ch, ch2;
	unsigned char stackable, count;
	unsigned short* n;
	unsigned short target;
	unsigned myCmdArgs, itemNum, connectNum, gc_num;
	unsigned short npcID;
	unsigned max_send;
	BANANA* lClient;
	int i, z, commandLen, ignored, found_ban, writeData;
	LOBBY* l;
	INVENTORY_ITEM ii;

	writeData = 0;
	fp = NULL;

	// Player must be in lobby
	if (!client->lobby)
		return;
	l = client->lobby;  // Save lobby info 保存大厅信息

						// Packet size cannot be larger than 0x100 数据包不能大于 256
						// Why?  Who knows 为何 谁知道
	pktsize = *(unsigned short*)&client->decryptbuf[0x00];
	if (pktsize > 0x100)
		return;

	memset(&chatBuf[0x00], 0, 0x0A);  // Global variable of size 4000, but only zero out the first 0x0A 全局变量的大小为4000，但只有零出第一个0x0A 

	chatBuf[0x02] = 0x06;
	chatBuf[0x0A] = client->clientID;
	*(unsigned*)&chatBuf[0x0C] = client->guildcard;
	chatsize = 0x10;
	// Add character name to chatbuf?
	n = (unsigned short*)&client->character.name[4];
	for (ch = 0;ch < 10;ch++)
	{
		if (*n == 0x0000)
			break;
		*(unsigned short*)&chatBuf[chatsize] = *n;
		chatsize += 2;
		n++;
	}
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	chatBuf[chatsize++] = 0x09;
	chatBuf[chatsize++] = 0x00;
	n = (unsigned short*)&client->decryptbuf[0x12];
	if ((*(n + 1) == 0x002F) && (*(n + 2) != 0x002F))  // Command parsing code in here
	{
		commandLen = 0;  // Initialize commandLen... not a loop so lol

		for (ch = 0;ch < (pktsize - 0x14);ch += 2)
		{
			// Until you reach a 0, count the command length
			if (client->decryptbuf[(0x14 + ch)] != 0x00)
				cmdBuf[commandLen++] = client->decryptbuf[(0x14 + ch)];// Anotehr global lvariable, also size of 4000
			else
				break;
		}

		cmdBuf[commandLen] = 0;

		myCmdArgs = 0;
		myCommand = &cmdBuf[1];

		// If command has any arguments
		if ((i = strcspn(&cmdBuf[1], " ,")) != (strlen(&cmdBuf[1])))
		{
			// count the arguments and store the addresses in myArgs char* array
			i++;
			cmdBuf[i++] = 0;  // clear command address from buffer (myCommand has it)
			while ((i < commandLen) && (myCmdArgs < 64))  // max 64 args (63?)
			{
				z = strcspn(&cmdBuf[i], ",");  // length of arg
				myArgs[myCmdArgs++] = &cmdBuf[i]; // save address of arg into myArgs
				i += z;
				cmdBuf[i++] = 0;
			}
		}

		if (commandLen)  // If a command was entered
		{
			if (!strcmp(myCommand, "help")) //新增帮助指令 sancaros
			{
				Send01(L"测试文本\n 1234\n english.\0", client, -1);
			}

			if (!strcmp(myCommand, "debug"))
			{
				writeData = 1;
			}
			if (!strcmp(myCommand, "arrow")) //新增箭头指令 sancaros
			{
				if (myCmdArgs == 0)
					SendB0(L"Need arrow digit.", client, 5);
				else
				{
					l->arrow_color[client->clientID] = atoi(myArgs[0]);
					ShowArrows(client, 1);
				}
			}
			//设置新的密码
			if (!strcmp(myCommand, "setpass"))
			{
				if (!client->lobbyNum < 0x10)
				{
					if (myCmdArgs == 0)
						SendB0(L"Need new password.", client, 3);
					else
					{
						ch = 0;
						while (myArgs[0][ch] != 0)
						{
							l->gamePassword[ch * 2] = myArgs[0][ch];
							l->gamePassword[(ch * 2) + 1] = 0;
							ch++;
							if (ch == 31) break; // Limit amount of characters...
						}
						l->gamePassword[ch * 2] = 0;
						l->gamePassword[(ch * 2) + 1] = 0;
						for (ch = 0;ch < 4;ch++)
						{
							if ((l->slot_use[ch]) && (l->client[ch]))
								SendB0(L"Room password changed.", l->client[ch], 4);
						}
					}
				}
			}

			if (!strcmp(myCommand, "arrow"))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need arrow digit.", client, 5);
				else
				{
					l->arrow_color[client->clientID] = atoi(myArgs[0]);
					ShowArrows(client, 1);
				}
			}

			if (!strcmp(myCommand, "lang"))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need language digit.", client, 6);
				else
				{
					npcID = atoi(myArgs[0]);

					if (npcID > numLanguages) npcID = 1;

					if (npcID == 0)
						npcID = 1;

					npcID--;

					client->character.lang = (unsigned char)npcID;

					SendB0(L"Current language:\n", client, 7);
					//SendB0(languageNames[npcID], client, -1);

					//language name char to wchar
					const size_t cSize = strlen(languageNames[npcID]) + 1;
					wchar_t wc[10];
					size_t tmp = 0;
					mbstowcs_s(&tmp, wc, cSize, languageNames[npcID], cSize);

					SendB0(wc, client, -1);

				}
			}


			if (!strcmp(myCommand, "npc"))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need NPC digit. (max = 11, 0 to unskin)", client, 8);
				else
				{
					npcID = atoi(myArgs[0]);

					if (npcID > 7)
					{
						if ((npcID > 11) || (!ship_support_extnpc))
							npcID = 7;
					}

					if (npcID == 0)
					{
						client->character.skinFlag = 0x00;
						client->character.skinID = 0x00;
					}
					else
					{
						client->character.skinFlag = 0x02;
						client->character.skinID = npcID - 1;
					}
					SendB0(L"Skin updated, change blocks for it to take effect.", client, 9);
				}
			}

			// Process GM commands
			// The second argument to playerHasRight() is the bit position of the maks of the command

			if ((!strcmp(myCommand, "event")) && ((client->isgm) || (playerHasRights(client->guildcard, 0))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need event digit.", client, 10);
				else
				{

					shipEvent = atoi(myArgs[0]);

					WriteGM("GM %u 已修改舰船事件为 %u", client->guildcard, shipEvent);

					PacketDA[0x04] = shipEvent;

					for (ch = 0;ch < serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard)
						{
							cipher_ptr = &connections[connectNum]->server_cipher;
							encryptcopy(connections[connectNum], &PacketDA[0], 8);
						}
					}
				}
			}

			if ((!strcmp(myCommand, "redbox")) && (client->isgm))
			{
				if (l->redbox)
				{
					l->redbox = 0;
					SendB0(L"Red box mode turned off.", client, 11);
					WriteGM("GM %u 已停用红盒模式", client->guildcard);
				}
				else
				{
					l->redbox = 1;
					SendB0(L"Red box mode turned on!", client, 12);
					WriteGM("GM %u 已开启红盒模式", client->guildcard);
				}
			}

			if ((!strcmp(myCommand, "item")) && ((client->isgm) || (playerHasRights(client->guildcard, 1))))
			{
				// Item creation...
				if (client->lobbyNum < 0x10)
					SendB0(L"Cannot make items in the lobby!!!", client, 13);
				else
					if (myCmdArgs < 4)
						SendB0(L"You must specify at least four arguments for the desired item.", client, 14);
					else
						if (strlen(myArgs[0]) < 8)
							SendB0(L"Main arguments is an incorrect length.", client, 15);
						else
						{
							if ((strlen(myArgs[1]) < 8) ||
								(strlen(myArgs[2]) < 8) ||
								(strlen(myArgs[3]) < 8))
								SendB0(L"Some arguments were incorrect and replaced.", client, 16);

							WriteGM("GM %u 制造了一件物品", client->guildcard);

							itemNum = free_game_item(l);

							_strupr(myArgs[0]);
							l->gameItem[itemNum].item.data[0] = hexToByte(&myArgs[0][0]);
							l->gameItem[itemNum].item.data[1] = hexToByte(&myArgs[0][2]);
							l->gameItem[itemNum].item.data[2] = hexToByte(&myArgs[0][4]);
							l->gameItem[itemNum].item.data[3] = hexToByte(&myArgs[0][6]);


							if (strlen(myArgs[1]) >= 8)
							{
								_strupr(myArgs[1]);
								l->gameItem[itemNum].item.data[4] = hexToByte(&myArgs[1][0]);
								l->gameItem[itemNum].item.data[5] = hexToByte(&myArgs[1][2]);
								l->gameItem[itemNum].item.data[6] = hexToByte(&myArgs[1][4]);
								l->gameItem[itemNum].item.data[7] = hexToByte(&myArgs[1][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[4] = 0;
								l->gameItem[itemNum].item.data[5] = 0;
								l->gameItem[itemNum].item.data[6] = 0;
								l->gameItem[itemNum].item.data[7] = 0;
							}

							if (strlen(myArgs[2]) >= 8)
							{
								_strupr(myArgs[2]);
								l->gameItem[itemNum].item.data[8] = hexToByte(&myArgs[2][0]);
								l->gameItem[itemNum].item.data[9] = hexToByte(&myArgs[2][2]);
								l->gameItem[itemNum].item.data[10] = hexToByte(&myArgs[2][4]);
								l->gameItem[itemNum].item.data[11] = hexToByte(&myArgs[2][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data[8] = 0;
								l->gameItem[itemNum].item.data[9] = 0;
								l->gameItem[itemNum].item.data[10] = 0;
								l->gameItem[itemNum].item.data[11] = 0;
							}

							if (strlen(myArgs[3]) >= 8)
							{
								_strupr(myArgs[3]);
								l->gameItem[itemNum].item.data2[0] = hexToByte(&myArgs[3][0]);
								l->gameItem[itemNum].item.data2[1] = hexToByte(&myArgs[3][2]);
								l->gameItem[itemNum].item.data2[2] = hexToByte(&myArgs[3][4]);
								l->gameItem[itemNum].item.data2[3] = hexToByte(&myArgs[3][6]);
							}
							else
							{
								l->gameItem[itemNum].item.data2[0] = 0;
								l->gameItem[itemNum].item.data2[1] = 0;
								l->gameItem[itemNum].item.data2[2] = 0;
								l->gameItem[itemNum].item.data2[3] = 0;
							}

							// check stackable shit

							stackable = 0;

							if (l->gameItem[itemNum].item.data[0] == 0x03)
								stackable = stackable_table[l->gameItem[itemNum].item.data[1]];

							if ((stackable) && (l->gameItem[itemNum].item.data[5] == 0x00))
								l->gameItem[itemNum].item.data[5] = 0x01; // force at least 1 of a stack to drop

							WriteGM("物品数据: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								l->gameItem[itemNum].item.data[0], l->gameItem[itemNum].item.data[1], l->gameItem[itemNum].item.data[2], l->gameItem[itemNum].item.data[3],
								l->gameItem[itemNum].item.data[4], l->gameItem[itemNum].item.data[5], l->gameItem[itemNum].item.data[6], l->gameItem[itemNum].item.data[7],
								l->gameItem[itemNum].item.data[8], l->gameItem[itemNum].item.data[9], l->gameItem[itemNum].item.data[10], l->gameItem[itemNum].item.data[11],
								l->gameItem[itemNum].item.data2[0], l->gameItem[itemNum].item.data2[1], l->gameItem[itemNum].item.data2[2], l->gameItem[itemNum].item.data2[3]);

							l->gameItem[itemNum].item.itemid = l->itemID++;
							if (l->gameItemCount < MAX_SAVED_ITEMS)
								l->gameItemList[l->gameItemCount++] = itemNum;
							memset(&PacketData[0], 0, 0x2C);
							PacketData[0x00] = 0x2C;
							PacketData[0x02] = 0x60;
							PacketData[0x08] = 0x5D;
							PacketData[0x09] = 0x09;
							PacketData[0x0A] = 0xFF;
							PacketData[0x0B] = 0xFB;
							*(unsigned*)&PacketData[0x0C] = l->floor[client->clientID];
							*(unsigned*)&PacketData[0x10] = l->clientx[client->clientID];
							*(unsigned*)&PacketData[0x14] = l->clienty[client->clientID];
							memcpy(&PacketData[0x18], &l->gameItem[itemNum].item.data[0], 12);
							*(unsigned*)&PacketData[0x24] = l->gameItem[itemNum].item.itemid;
							*(unsigned*)&PacketData[0x28] = *(unsigned*)&l->gameItem[itemNum].item.data2[0];
							SendToLobby(client->lobby, 4, &PacketData[0], 0x2C, 0);
							SendB0(L"Item created.", client, 17);
						}
			}

			if ((!strcmp(myCommand, "give")) && ((client->isgm) || (playerHasRights(client->guildcard, 1))))
			{
				// Insert item into inventory
				if (client->lobbyNum < 0x10)
					SendB0(L"Cannot give items in the lobby!!!", client, 18);
				else
					if (myCmdArgs < 4)
						SendB0(L"You must specify at least four arguments for the desired item.", client, 14);
					else
						if (strlen(myArgs[0]) < 8)
							SendB0(L"Main arguments is an incorrect length.", client, 15);
						else
						{
							if ((strlen(myArgs[1]) < 8) ||
								(strlen(myArgs[2]) < 8) ||
								(strlen(myArgs[3]) < 8))
								SendB0(L"Some arguments were incorrect and replaced.", client, 16);

							WriteGM("GM %u 制造了一件物品", client->guildcard);

							_strupr(myArgs[0]);
							ii.item.data[0] = hexToByte(&myArgs[0][0]);
							ii.item.data[1] = hexToByte(&myArgs[0][2]);
							ii.item.data[2] = hexToByte(&myArgs[0][4]);
							ii.item.data[3] = hexToByte(&myArgs[0][6]);


							if (strlen(myArgs[1]) >= 8)
							{
								_strupr(myArgs[1]);
								ii.item.data[4] = hexToByte(&myArgs[1][0]);
								ii.item.data[5] = hexToByte(&myArgs[1][2]);
								ii.item.data[6] = hexToByte(&myArgs[1][4]);
								ii.item.data[7] = hexToByte(&myArgs[1][6]);
							}
							else
							{
								ii.item.data[4] = 0;
								ii.item.data[5] = 0;
								ii.item.data[6] = 0;
								ii.item.data[7] = 0;
							}

							if (strlen(myArgs[2]) >= 8)
							{
								_strupr(myArgs[2]);
								ii.item.data[8] = hexToByte(&myArgs[2][0]);
								ii.item.data[9] = hexToByte(&myArgs[2][2]);
								ii.item.data[10] = hexToByte(&myArgs[2][4]);
								ii.item.data[11] = hexToByte(&myArgs[2][6]);
							}
							else
							{
								ii.item.data[8] = 0;
								ii.item.data[9] = 0;
								ii.item.data[10] = 0;
								ii.item.data[11] = 0;
							}

							if (strlen(myArgs[3]) >= 8)
							{
								_strupr(myArgs[3]);
								ii.item.data2[0] = hexToByte(&myArgs[3][0]);
								ii.item.data2[1] = hexToByte(&myArgs[3][2]);
								ii.item.data2[2] = hexToByte(&myArgs[3][4]);
								ii.item.data2[3] = hexToByte(&myArgs[3][6]);
							}
							else
							{
								ii.item.data2[0] = 0;
								ii.item.data2[1] = 0;
								ii.item.data2[2] = 0;
								ii.item.data2[3] = 0;
							}

							// check stackable shit

							stackable = 0;

							if (ii.item.data[0] == 0x03)
								stackable = stackable_table[ii.item.data[1]];

							if (stackable)
							{
								if (ii.item.data[5] == 0x00)
									ii.item.data[5] = 0x01; // force at least 1 of a stack to drop
								count = ii.item.data[5];
							}
							else
								count = 1;

							WriteGM("物品数据: %02X%02X%02X%02X,%02X%02X%02X%02X,%02X%02x%02x%02x,%02x%02x%02x%02x",
								ii.item.data[0], ii.item.data[1], ii.item.data[2], ii.item.data[3],
								ii.item.data[4], ii.item.data[5], ii.item.data[6], ii.item.data[7],
								ii.item.data[8], ii.item.data[9], ii.item.data[10], ii.item.data[11],
								ii.item.data2[0], ii.item.data2[1], ii.item.data2[2], ii.item.data2[3]);

							ii.item.itemid = l->itemID++;
							AddToInventory(&ii, count, 0, client);
							SendB0(L"Item obtained.", client, 19);
						}
			}

			if ((!strcmp(myCommand, "warpme")) && ((client->isgm) || (playerHasRights(client->guildcard, 3))))
			{
				if (client->lobbyNum < 0x10)
					SendB0(L"Can't warp in the lobby!!!", client, 20);
				else
					if (myCmdArgs == 0)
						SendB0(L"Need area to warp to...", client, 21);
					else
					{
						target = atoi(myArgs[0]);
						if (target > 17)
							SendB0(L"Warping past area 17 would probably crash your client...", client, 22);
						else
						{
							warp_packet[0x0C] = (unsigned char)atoi(myArgs[0]);
							cipher_ptr = &client->server_cipher;
							encryptcopy(client, &warp_packet[0], sizeof(warp_packet));
						}
					}
			}

			if ((!strcmp(myCommand, "dc")) && ((client->isgm) || (playerHasRights(client->guildcard, 4))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to disconnect.", client, 23);
				else
				{
					gc_num = atoi(myArgs[0]);
					for (ch = 0;ch < serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0(L"You may not disconnect this user.", client, 24);
							else
							{
								WriteGM("GM %u 断开用户 %u (%s) 的连接", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
								Send1A(L"You've been disconnected by a GM.", connections[connectNum], 99);
								connections[connectNum]->todc = 1;
								break;
							}
						}
					}
				}
			}

			if ((!strcmp(myCommand, "ban")) && ((client->isgm) || (playerHasRights(client->guildcard, 11))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to ban.", client, 25);
				else
				{
					gc_num = atoi(myArgs[0]);
					found_ban = 0;

					for (ch = 0;ch < num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 1))
						{
							found_ban = 1;
							ban(gc_num, (unsigned*)&client->ipaddr, &client->hwinfo, 1, client); // Should unban...
							WriteGM("GM %u has removed ban from guild card %u.", client->guildcard, gc_num);
							SendB0(L"Ban removed.", client, 26);
							break;
						}
					}

					if (!found_ban)
					{
						for (ch = 0;ch < serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0(L"You may not ban this user.", client, 27);
								else
								{
									if (ban(gc_num, (unsigned*)&connections[connectNum]->ipaddr,
										&connections[connectNum]->hwinfo, 1, client))
									{
										WriteGM("GM %u has banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
										Send1A(L"You've been banned by a GM.", connections[connectNum], 100);
										SendB0(L"User has been banned.", client, 28);
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ((!strcmp(myCommand, "ipban")) && ((client->isgm) || (playerHasRights(client->guildcard, 12))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to IP ban.", client, 29);
				else
				{
					gc_num = atoi(myArgs[0]);
					found_ban = 0;

					for (ch = 0;ch < num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 2))
						{
							found_ban = 1;
							ban(gc_num, (unsigned*)&client->ipaddr, &client->hwinfo, 2, client); // Should unban...
							WriteGM("GM %u has removed IP ban from guild card %u.", client->guildcard, gc_num);
							SendB0(L"IP ban removed.", client, 30);
							break;
						}
					}

					if (!found_ban)
					{
						for (ch = 0;ch < serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0(L"You may not ban this user.", client, 27);
								else
								{
									if (ban(gc_num, (unsigned*)&connections[connectNum]->ipaddr,
										&connections[connectNum]->hwinfo, 2, client))
									{
										WriteGM("GM %u has IP banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
										Send1A(L"You've been banned by a GM.", connections[connectNum], 100);
										SendB0(L"User has been IP banned.", client, 31);
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}

			if ((!strcmp(myCommand, "hwban")) && ((client->isgm) || (playerHasRights(client->guildcard, 12))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to HW ban.", client, 32);
				else
				{
					gc_num = atoi(myArgs[0]);
					found_ban = 0;

					for (ch = 0;ch < num_bans;ch++)
					{
						if ((ship_bandata[ch].guildcard == gc_num) && (ship_bandata[ch].type == 3))
						{
							found_ban = 1;
							ban(gc_num, (unsigned*)&client->ipaddr, &client->hwinfo, 3, client); // Should unban...
							WriteGM("GM %u has removed HW ban from guild card %u.", client->guildcard, gc_num);
							SendB0(L"HW ban removed.", client, 33);
							break;
						}
					}

					if (!found_ban)
					{
						for (ch = 0;ch < serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gc_num)
							{
								if ((connections[connectNum]->isgm) || (isLocalGM(connections[connectNum]->guildcard)))
									SendB0(L"You may not ban this user.", client, 27);
								else
								{
									if (ban(gc_num, (unsigned*)&connections[connectNum]->ipaddr,
										&connections[connectNum]->hwinfo, 3, client))
									{
										WriteGM("GM %u has HW banned user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
										Send1A(L"You've been banned by a GM.", connections[connectNum], 100);
										SendB0(L"User has been HW banned.", client, 34);
										connections[connectNum]->todc = 1;
									}
									break;
								}
							}
						}
					}
				}
			}


			if ((!strcmp(myCommand, "dcall")) && ((client->isgm) || (playerHasRights(client->guildcard, 5))))
			{
				printf("Blocking connections until all users are disconnected...\n");
				WriteGM("GM %u has disconnected all users", client->guildcard);
				blockConnections = 1;
			}

			if ((!strcmp(myCommand, "announce")) && ((client->isgm) || (playerHasRights(client->guildcard, 6))))
			{
				if (client->announce != 0)
				{
					SendB0(L"Announce\ncancelled.", client, 35);
					client->announce = 0;
				}
				else
				{
					SendB0(L"Announce by\nsending a\nmail.", client, 36);
					client->announce = 1;
				}
			}

			if ((!strcmp(myCommand, "global")) && (client->isgm))
			{
				if (client->announce != 0)
				{
					SendB0(L"Announce\ncancelled.", client, 35);
					client->announce = 0;
				}
				else
				{
					SendB0(L"Global announce\nby sending\na mail.", client, 37);
					client->announce = 2;
				}
			}

			if ((!strcmp(myCommand, "levelup")) && ((client->isgm) || (playerHasRights(client->guildcard, 7))))
			{
				if (client->lobbyNum < 0x10)
					SendB0(L"Cannot level up in the lobby!!!", client, 38);
				else
					if (l->floor[client->clientID] == 0)
						SendB0(L"Please leave Pioneer 2 before using this command...", client, 39);
					else
						if (myCmdArgs == 0)
							SendB0(L"Must specify a target level to level up to...", client, 40);
						else
						{
							target = atoi(myArgs[0]);
							if ((client->character.level + 1) >= target)
								SendB0(L"Target level must be higher than your current level...", client, 41);
							else
							{
								// Do the level up!!!

								if (target > 200)
									target = 200;

								target -= 2;

								AddExp(tnlxp[target] - client->character.XP, client);
							}
						}
			}

			if ((!strcmp(myCommand, "updatelocalgms")) && ((client->isgm) || (playerHasRights(client->guildcard, 8))))
			{
				SendB0(L"Local GM file reloaded.", client, 42);
				readLocalGMFile();
			}

			if ((!strcmp(myCommand, "updatemasks")) && ((client->isgm) || (playerHasRights(client->guildcard, 12))))
			{
				SendB0(L"IP ban masks file reloaded.", client, 43);
				load_mask_file();
			}

			if (!strcmp(myCommand, "bank"))
			{
				if (client->bankType)
				{
					client->bankType = 0;
					SendB0(L"Bank: Character", client, 44);
				}
				else
				{
					client->bankType = 1;
					SendB0(L"Bank: Common", client, 45);
				}
			}

			if (!strcmp(myCommand, "vip"))//新增VIP指令
			{
				if (client->character.sectionID == ID_Yellowboze) {
					if (client->shoptype)
					{
						client->shoptype = 0;
						SendB0(L"shoptype: Normal", client, -1);
					}
					else
					{
						client->shoptype = 2;
						SendB0(L"shoptype: Vip", client, -1);
					}
				}
				else {
					SendB0(L"Oh.. I'm not VIP.", client, -1);
				}
			}
			if (!strcmp(myCommand, "ignore"))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to ignore.", client, 46);
				else
				{
					gc_num = atoi(myArgs[0]);
					ignored = 0;

					for (ch = 0;ch < client->ignore_count;ch++)
					{
						if (client->ignore_list[ch] == gc_num)
						{
							ignored = 1;
							client->ignore_list[ch] = 0;
							SendB0(L"User no longer being ignored.", client, 47);
							break;
						}
					}

					if (!ignored)
					{
						if (client->ignore_count < 100)
						{
							client->ignore_list[client->ignore_count++] = gc_num;
							SendB0(L"User is now ignored.", client, 48);
						}
						else
							SendB0(L"Ignore list is full.", client, 49);
					}
					else
					{
						ch2 = 0;
						for (ch = 0;ch < client->ignore_count;ch++)
						{
							if ((client->ignore_list[ch] != 0) && (ch != ch2))
								client->ignore_list[ch2++] = client->ignore_list[ch];
						}
						client->ignore_count = ch2;
					}
				}
			}

			if ((!strcmp(myCommand, "stfu")) && ((client->isgm) || (playerHasRights(client->guildcard, 9))))
			{
				if (myCmdArgs == 0)
					SendB0(L"Need a guild card # to silence.", client, 50);
				else
				{
					gc_num = atoi(myArgs[0]);
					for (ch = 0;ch < serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gc_num)
						{
							if ((connections[connectNum]->isgm) && (isLocalGM(client->guildcard)))
								SendB0(L"You may not silence this user.", client, 51);
							else
							{
								if (toggle_stfu(connections[connectNum]->guildcard, client))
								{
									WriteGM("GM %u has silenced user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
									SendB0(L"User has been silenced.", client, 52);
									SendB0(L"You've been silenced.", connections[connectNum], 53);
								}
								else
								{
									WriteGM("GM %u has removed silence from user %u (%s)", client->guildcard, gc_num, Unicode_to_ASCII((unsigned short*)&connections[connectNum]->character.name[4]));
									SendB0(L"User is now allowed to speak.", client, 54);
									SendB0(L"You may now speak freely.", connections[connectNum], 55);
								}
								break;
							}
						}
					}
				}
			}

			if ((!strcmp(myCommand, "warpall")) && ((client->isgm) || (playerHasRights(client->guildcard, 10))))
			{
				if (client->lobbyNum < 0x10)
					SendB0(L"Can't warp in the lobby!!!", client, 20);
				else
					if (myCmdArgs == 0)
						SendB0(L"Need area to warp to...", client, 21);
					else
					{
						target = atoi(myArgs[0]);
						if (target > 17)
							SendB0(L"Warping past area 17 would probably crash your client...", client, 22);
						else
						{
							warp_packet[0x0C] = (unsigned char)atoi(myArgs[0]);
							SendToLobby(client->lobby, 4, &warp_packet[0], sizeof(warp_packet), 0);
						}
					}
			} //新增重载指令
			if ((!strcmp(myCommand, "reloadconfig")) && ((client->isgm) || (playerHasRights(client->guildcard, 8))))
			{
				load_config_file();
				SendB0(L"Ship config file reloaded.", client, -1);
			}
			if ((!strcmp(myCommand, "setval")) && ((client->isgm) || (playerHasRights(client->guildcard, 2))))
			{
				if (myCmdArgs < 1)
					SendB0(L"You must provide at least one argument.\nType \"/setval help\" or\n\"/setval help,[topic]\" for more info.", client, -1);
				else
				{
					if (!strcmp(myArgs[0], "help"))
					{
						if (myArgs[1] == NULL)
						{
							SendB0(L"Usage: /setval [var],[value]", client, -1);
							SendB0(L"Args for var: help, exp, raremult, rmob", client, -1);
						}
						else if (!strcmp(myArgs[1], "exp"))
							SendB0(L"The rate (x100%) of experience earned.", client, -1);
						else if (!strcmp(myArgs[1], "rboxd"))
							SendB0(L"A multiplier of rare item occurence\nin boxes.", client, -1);
						else if (!strcmp(myArgs[1], "raremult"))
							SendB0(L"A multiplier to be applied to\nthe drop rates of rare items.", client, -1);
						else if (!strcmp(myArgs[1], "raremult"))
							SendB0(L"A multiplier to be applied to\nthe occurence rate of rare mobs.", client, -1);
					}
					if (!strncmp(myArgs[0], "exp", 3))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a num to set the exp rate to.", client, -1);
						else
						{
							EXPERIENCE_RATE = atoi(myArgs[1]);
							if (EXPERIENCE_RATE > 100)
							{
								SendB0(L"Too large -- truncated to 100.", client, -1);
								EXPERIENCE_RATE = 100;
							}
							if (EXPERIENCE_RATE < 1)
							{
								SendB0(L"Must be a num greater than 0.\nSet to 1.", client, -1);
								EXPERIENCE_RATE = 1;
							}
							WriteGM("GM %u (%s) has set the exp rate to %d%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), EXPERIENCE_RATE * 100);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Exp is increased by ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%d%%", EXPERIENCE_RATE * 100);
							SendEE(L"Exp is increased by ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "rboxd", 6))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a num to set the rare box multiplier.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							if (val > 100)
							{
								SendB0(L"Too large -- truncated to 100.", client, -1);
								val = 100;
							}
							if (val < 1)
							{
								SendB0(L"Must be a num greater than 0.\nSet to 1.", client, -1);
								val = 1;
							}
							rare_box_mult = val;
							WriteGM("GM %u (%s) has set the rare item box drop rate to %d%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val * 100);
							unsigned char mesg[] = "Rare item occurence rate in boxes is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%d%%", val * 100);
							SendEE(L"Rare item occurence rate in boxes is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "raremult", 8))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a num to set the rare drop multiplier.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							if (val > 100)
							{
								SendB0(L"Too large -- truncated to 100.", client, -1);
								val = 100;
							}
							if (val < 1)
							{
								SendB0(L"Must be a num greater than 0.\nSet to 1.", client, -1);
								val = 1;
							}
							global_rare_mult = val;
							WriteGM("GM %u (%s) has set the rare item drop multiplier to %d%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val * 100);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to get a rare item is increased by ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%d%%", val * 100);
							SendEE(L"Chance to get a rare item is increased by ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "rmob", 4))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a num to set the rare mob multiplier.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							if (val > 100)
							{
								SendB0(L"Too large -- truncated to 100.", client, -1);
								val = 100;
							}
							if (val < 1)
							{
								SendB0(L"Must be a num greater than 0.\nSet to 1.", client, -1);
								val = 1;
							}
							rare_mob_mult = val;
							WriteGM("GM %u (%s) has set the rare mob multiplier to %d%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val * 100);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare monster is increased by ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%d%%", val * 100);
							SendEE(L"Chance to encounter a rare monster is increased by ", client, -1);
						}
					}
				}
			}
			if ((!strcmp(myCommand, "setrare")) && ((client->isgm) || (playerHasRights(client->guildcard, 2))))
			{
				if (myCmdArgs < 1)
				{
					SendB0(L"Provide a mob and a rate, with those\ntwo values separated by a comma.", client, -1);
					SendB0(L"Type \"\\setrare list\" for a list of settable mobs.", client, -1);
				}
				else
				{
					if (!strncmp(myArgs[0], "list", strlen("list")))
					{
						SendB0(L"All, dorphon, hildebear, kondrieu, lilly,\nmerissa, pazuzu, rappy, slime.", client, -1);
					}
					if (!strncmp(myArgs[0], "all", strlen("all")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							hildebear_rate = val;
							rappy_rate = val;
							lily_rate = val;
							slime_rate = val;
							merissa_rate = val;
							pazuzu_rate = val;
							dorphon_rate = val;
							kondrieu_rate = val;
							WriteGM("GM %u (%s) has set the rare rate to all mobs to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter any rare mob is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter any rare mob is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "hildebear", strlen("hildebear")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							hildebear_rate = val;
							WriteGM("GM %u (%s) has set the rare hildebear rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare hildebear is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare hildebear is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "rappy", strlen("rappy")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							rappy_rate = val;
							WriteGM("GM %u (%s) has set the rare rappy rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare rappy is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare rappy is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "lilly", strlen("lilly")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							lily_rate = val;
							WriteGM("GM %u (%s) has set the rare lilly rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare lilly is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare lilly is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "slime", strlen("slime")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							slime_rate = val;
							WriteGM("GM %u (%s) has set the rare slime rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare slime is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare slime is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "merissa", strlen("merissa")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							merissa_rate = val;
							WriteGM("GM %u (%s) has set the rare merissa rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare merissa is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare merissa is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "pazuzu", strlen("pazuzu")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							pazuzu_rate = val;
							WriteGM("GM %u (%s) has set the rare pazuzu rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare pazuzu is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare pazuzu is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "dorphon", strlen("dorphon")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							dorphon_rate = val;
							WriteGM("GM %u (%s) has set the rare dorphon eclair rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare dorphon eclair is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare dorphon eclair is now ", client, -1);
						}
					}
					if (!strncmp(myArgs[0], "kondrieu", strlen("kondrieu")))
					{
						if (myCmdArgs < 2)
							SendB0(L"Provide a rate. Rate is basically the number divided by 1000.", client, -1);
						else
						{
							int val = atoi(myArgs[1]);
							kondrieu_rate = val;
							WriteGM("GM %u (%s) has set the rare kondrieu rate to %f%%", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]), val / 10.0);
							SendB0(L"New value set.", client, -1);
							unsigned char mesg[] = "Chance to encounter a rare kondrieu is now ";
							int i = strlen(mesg);
							sprintf(&mesg[i], "%f%%", val / 10.0);
							SendEE(L"Chance to encounter a rare kondrieu is now ", client, -1);
						}
					}
				}
			}
		}
	}
	else
	{
		for (ch = 0;ch < (pktsize - 0x12);ch += 2)
		{
			if ((*n == 0x0000) || (chatsize == 0xC0))
				break;
			if ((*n == 0x0009) || (*n == 0x000A))
				*n = 0x0020;
			*(unsigned short*)&chatBuf[chatsize] = *n;
			chatsize += 2;
			n++;
		}
		chatBuf[chatsize++] = 0x00;
		chatBuf[chatsize++] = 0x00;
		while (chatsize % 8)
			chatBuf[chatsize++] = 0x00;
		*(unsigned short*)&chatBuf[0x00] = chatsize;
		if (!stfu(client->guildcard))
		{
			if (client->lobbyNum < 0x10)
				max_send = 12;
			else
				max_send = 4;
			for (ch = 0;ch < max_send;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					ignored = 0;
					lClient = l->client[ch];
					for (ch2 = 0;ch2 < lClient->ignore_count;ch2++)
					{
						if (lClient->ignore_list[ch2] == client->guildcard)
						{
							ignored = 1;
							break;
						}
					}
					if (!ignored)
					{
						cipher_ptr = &lClient->server_cipher;
						encryptcopy(lClient, &chatBuf[0x00], chatsize);
					}
				}
			}
		}
	}

	if (writeData)
	{
		if (!client->debugged)
		{
			client->debugged = 1;
			_itoa(client->character.guildCard, &character_file[0], 10);
			strcat(&character_file[0], Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
			strcat(&character_file[0], ".bbc");
			fp = fopen(&character_file[0], "wb");
			if (fp)
			{
				fwrite(&client->character.packetSize, 1, sizeof(CHARDATA), fp);
				fclose(fp);
			}
			WriteLog("用户 %u (%s) 已写入字符调试数据.", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
			SendB0(L"Your debug data has been saved.", client, 56);
		}
		else
			SendB0(L"Your debug data has already been saved.", client, 57);
	}
}

//与newserv的void process_change_account_data_bb 定义一样的结构
void CommandED(BANANA* client) //客户端设置数据转储
{
	switch (client->decryptbuf[0x03])
	{
	case 0x01:
		// Options 选项
		*(unsigned*)&client->character.options[0] = *(unsigned*)&client->decryptbuf[0x08];
		break;
	case 0x02:
		// Symbol Chats 表情聊天
		memcpy(&client->character.symbol_chats, &client->decryptbuf[0x08], 1248);
		break;
	case 0x03:
		// Shortcuts 快捷菜单
		memcpy(&client->character.shortcuts, &client->decryptbuf[0x08], 2624);
		break;
	case 0x04:
		// Global Key Config 全局设置
		memcpy(&client->character.keyConfigGlobal, &client->decryptbuf[0x08], 364);
		break;
	case 0x05:
		// Global Joystick Config 全局按键设置
		memcpy(&client->character.joyConfigGlobal, &client->decryptbuf[0x08], 56);
		break;
	case 0x06:
		// Technique Config 魔法设置
		memcpy(&client->character.techConfig, &client->decryptbuf[0x08], 40);
		break;
	case 0x07:
		// Character Key Config 角色键位设置
		memcpy(&client->character.keyConfig, &client->decryptbuf[0x08], 232);
		break;
	case 0x08:
		// C-Rank and Battle Config 挑战设置
		memcpy(&client->character.challengeData, &client->decryptbuf[0x08], 320); //原屏蔽的设置代码 关于挑战模式
		memcpy(&client->character.battleData, &client->decryptbuf[0x08], 92); //原屏蔽的设置代码 关于挑战模式
		break;
	}
}

//名片卡搜索代码
void Command40(BANANA* client, ORANGE* ship)
{
	// Guild Card Search

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned*)&ship->encryptbuf[0x02] = *(unsigned*)&client->decryptbuf[0x10];
	*(unsigned*)&ship->encryptbuf[0x06] = *(unsigned*)&client->guildcard;
	*(unsigned*)&ship->encryptbuf[0x0A] = serverID;
	*(unsigned*)&ship->encryptbuf[0x0E] = client->character.teamID;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x21);
}

//房间任务信息代码
void Command09(BANANA* client)
{
	QUEST* q;
	BANANA* c;
	LOBBY* l;
	unsigned lobbyNum, Packet11_Length, ch;
	char lb[10];
	int num_hours, num_minutes;

	switch (client->decryptbuf[0x0F])
	{
	case 0x00:
		// Team info 公会信息
		if (client->lobbyNum < 0x10)
		{
			if ((!client->block) || (client->block > serverBlocks))
			{
				initialize_connection(client);
				//WriteLog("debug客户端断点8");
				return;
			}

			lobbyNum = *(unsigned*)&client->decryptbuf[0x0C];

			if ((lobbyNum < 0x10) || (lobbyNum >= 16 + SHIP_COMPILED_MAX_GAMES))
			{
				initialize_connection(client);
				//WriteLog("debug客户端断点9");
				return;
			}

			l = &blocks[client->block - 1]->lobbies[lobbyNum];
			memset(&PacketData, 0, 0x10);
			PacketData[0x02] = 0x11;
			PacketData[0x0A] = 0x20;
			PacketData[0x0C] = 0x20;
			PacketData[0x0E] = 0x20;
			if (l->in_use)
			{
				Packet11_Length = 0x10;
				if ((client->team_info_request != lobbyNum) || (client->team_info_flag == 0))
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 1;
					for (ch = 0;ch < 4;ch++)
						if ((l->slot_use[ch]) && (l->client[ch]))
						{
							c = l->client[ch];
							wstrcpy((unsigned short*)&PacketData[Packet11_Length], (unsigned short*)&c->character.name[0]);
							Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4C;
							PacketData[Packet11_Length++] = 0x00;
							_itoa(l->client[ch]->character.level + 1, &lb[0], 10);
							wstrcpy_char(&PacketData[Packet11_Length], &lb[0]);
							Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							switch (c->character._class)
							{
							case CLASS_HUMAR:
								wstrcpy_char(&PacketData[Packet11_Length], "HUmar");
								break;
							case CLASS_HUNEWEARL:
								wstrcpy_char(&PacketData[Packet11_Length], "HUnewearl");
								break;
							case CLASS_HUCAST:
								wstrcpy_char(&PacketData[Packet11_Length], "HUcast");
								break;
							case CLASS_HUCASEAL:
								wstrcpy_char(&PacketData[Packet11_Length], "HUcaseal");
								break;
							case CLASS_RAMAR:
								wstrcpy_char(&PacketData[Packet11_Length], "RAmar");
								break;
							case CLASS_RACAST:
								wstrcpy_char(&PacketData[Packet11_Length], "RAcast");
								break;
							case CLASS_RACASEAL:
								wstrcpy_char(&PacketData[Packet11_Length], "RAcaseal");
								break;
							case CLASS_RAMARL:
								wstrcpy_char(&PacketData[Packet11_Length], "RAmarl");
								break;
							case CLASS_FONEWM:
								wstrcpy_char(&PacketData[Packet11_Length], "FOnewm");
								break;
							case CLASS_FONEWEARL:
								wstrcpy_char(&PacketData[Packet11_Length], "FOnewearl");
								break;
							case CLASS_FOMARL:
								wstrcpy_char(&PacketData[Packet11_Length], "FOmarl");
								break;
							case CLASS_FOMAR:
								wstrcpy_char(&PacketData[Packet11_Length], "FOmar");
								break;
							default:
								wstrcpy_char(&PacketData[Packet11_Length], "Unknown");
								break;
							}
							Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
						}
				}
				else
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 0;
					wstrcpy_char(&PacketData[Packet11_Length], "Time : ");
					Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
					num_minutes = ((unsigned)servertime - l->start_time) / 60L;
					num_hours = num_minutes / 60L;
					num_minutes %= 60;
					_itoa(num_hours, &lb[0], 10);
					wstrcpy_char(&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
					PacketData[Packet11_Length++] = 0x3A;
					PacketData[Packet11_Length++] = 0x00;
					_itoa(num_minutes, &lb[0], 10);
					if (num_minutes < 10)
					{
						lb[1] = lb[0];
						lb[0] = 0x30;
						lb[2] = 0x00;
					}
					wstrcpy_char(&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
					PacketData[Packet11_Length++] = 0x0A;
					PacketData[Packet11_Length++] = 0x00;
					if (l->quest_loaded)
					{
						wstrcpy_char(&PacketData[Packet11_Length], "Quest : ");
						Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
						PacketData[Packet11_Length++] = 0x0A;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						q = &quests[l->quest_loaded - 1];
						if ((client->character.lang < 10) && (q->ql[client->character.lang]))
							wstrcpy((unsigned short*)&PacketData[Packet11_Length], (unsigned short*)&q->ql[client->character.lang]->qname[0]);
						else
							wstrcpy((unsigned short*)&PacketData[Packet11_Length], (unsigned short*)&q->ql[0]->qname[0]);
						Packet11_Length += wstrlen((unsigned short*)&PacketData[Packet11_Length]);
					}
				}
			}
			else
			{
				wstrcpy_char(&PacketData[0x10], "Game no longer active.");
				Packet11_Length = 0x10 + (strlen("Game no longer active.") * 2);
			}
			PacketData[Packet11_Length++] = 0x00;
			PacketData[Packet11_Length++] = 0x00;
			*(unsigned short*)&PacketData[0x00] = (unsigned short)Packet11_Length;
			cipher_ptr = &client->server_cipher;
			encryptcopy(client, &PacketData[0], Packet11_Length);
		}
		break;
	case 0x0F:
		// Quest info 任务信息
		if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0B] <= numQuests))
		{
			q = &quests[client->decryptbuf[0x0B] - 1];
			memset(&PacketData[0x00], 0, 8);
			PacketData[0x00] = 0x48;
			PacketData[0x01] = 0x02;
			PacketData[0x02] = 0xA3;
			if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				memcpy(&PacketData[0x08], &q->ql[client->character.lang]->qdetails[0], 0x200);
			else
				memcpy(&PacketData[0x08], &q->ql[0]->qdetails[0], 0x200);
			cipher_ptr = &client->server_cipher;
			encryptcopy(client, &PacketData[0], 0x248);
		}
		break;
	default:
		break;
	}
}

//房间任务匹配代码
void Command10(unsigned blockServer, BANANA* client)
{
	unsigned char select_type, selected;
	unsigned full_select, ch, ch2, failed_to_join, lobbyNum, password_match, oldIndex;
	LOBBY* l;
	unsigned short* p;
	unsigned short* c;
	unsigned short fqs, barule;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	QUEST* q;
	char quest_num[16];
	unsigned qn;
	int do_quest;
	unsigned quest_flag;

	if (client->guildcard)
	{
		select_type = (unsigned char)client->decryptbuf[0x0F];
		selected = (unsigned char)client->decryptbuf[0x0C];
		full_select = *(unsigned*)&client->decryptbuf[0x0C];

		switch (select_type)
		{
		case 0x00:
			if ((blockServer) && (client->lobbyNum < 0x10))
			{
				// Team 队伍

				if ((!client->block) || (client->block > serverBlocks))
				{
					initialize_connection(client);
					//WriteLog("debug客户端断点10");
					return;
				}

				lobbyNum = *(unsigned*)&client->decryptbuf[0x0C];

				if ((lobbyNum < 0x10) || (lobbyNum >= 16 + SHIP_COMPILED_MAX_GAMES))
				{
					initialize_connection(client);
					//WriteLog("debug客户端断点11");
					return;
				}

				failed_to_join = 0;
				l = &blocks[client->block - 1]->lobbies[lobbyNum];

				if ((!client->isgm) && (!isLocalGM(client->guildcard)))
				{
					switch (l->episode)
					{
					case 0x01:
						if ((l->difficulty == 0x01) && (client->character.level < 19))
						{
							Send01(L"Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client, 105);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 49))
							{
								Send01(L"Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client, 106);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 89))
								{
									Send01(L"Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client, 107);
									failed_to_join = 1;
								}
						break;
					case 0x02:
						if ((l->difficulty == 0x01) && (client->character.level < 29))
						{
							Send01(L"Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client, 108);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 59))
							{
								Send01(L"Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client, 109);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 99))
								{
									Send01(L"Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client, 110);
									failed_to_join = 1;
								}
						break;
					case 0x03:
						if ((l->difficulty == 0x01) && (client->character.level < 39))
						{
							Send01(L"Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client, 111);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 69))
							{
								Send01(L"Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client, 112);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 109))
								{
									Send01(L"Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client, 113);
									failed_to_join = 1;
								}
						break;
					}
				}

				//房间已经不存在了
				if ((!l->in_use) && (!failed_to_join))
				{
					Send01(L"Game no longer active.", client, 114);
					failed_to_join = 1;
				}
				//队伍已满判断
				if ((l->lobbyCount == 4) && (!failed_to_join))
				{
					Send01(L"Game is full", client, 115);
					failed_to_join = 1;
				}
				//任务进行中
				if ((l->quest_in_progress) && (!failed_to_join))
				{
					Send01(L"Quest already in progress.", client, 116);
					failed_to_join = 1;
				}
				//单人任务无法加入
				if ((l->oneperson) && (!failed_to_join))
				{
					Send01(L"Cannot join a one\nperson game.", client, 117);
					failed_to_join = 1;
				}
				//房间加密码
				if (((l->gamePassword[0x00] != 0x00) || (l->gamePassword[0x01] != 0x00)) &&
					(!failed_to_join))
				{
					password_match = 1;
					p = (unsigned short*)&l->gamePassword[0x00];
					c = (unsigned short*)&client->decryptbuf[0x10];
					while (*p != 0x00)
					{
						if (*p != *c)
							password_match = 0;
						p++;
						c++;
					}
					if ((password_match == 0) && (client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
					{
						Send01(L"Incorrect password.", client, 118);
						failed_to_join = 1;
					}
				}
				//其他判断无法加入的原因
				if (!failed_to_join)
				{
					for (ch = 0;ch < 4;ch++)
					{
						if ((l->slot_use[ch]) && (l->client[ch]))
						{
							//房主正在进入房间
							if (l->client[ch]->bursting == 1)
							{
								Send01(L"Player is bursting.\nPlease wait a\nmoment.", client, 119);
								failed_to_join = 1;
							}
							else
								//房主正在载入任务中
								if ((l->inpquest) && (!l->client[ch]->hasquest))
								{
									Send01(L"Player is loading\nquest.\nPlease wait a\nmoment.", client, 120);
									failed_to_join = 1;
								}
						}
					}
				}
				// 检查玩家是否有资格加入政府任务 
				if ((l->inpquest) && (!failed_to_join))
				{
					// Check if player qualifies to join Government quest...
					q = &quests[l->quest_loaded - 1];
					memcpy(&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qn = (unsigned)atoi(&dp[0]);
					switch (l->episode)
					{
					case 0x01:
						qn -= 401;
						qn <<= 1;
						qn += 0x1F3;
						for (ch2 = 0x1F5;ch2 <= qn;ch2 += 2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x02:
						qn -= 451;
						qn <<= 1;
						qn += 0x211;
						for (ch2 = 0x213;ch2 <= qn;ch2 += 2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x03:
						qn -= 701;
						qn += 0x2BC;
						for (ch2 = 0x2BD;ch2 <= qn;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					}
					if (failed_to_join)
					{
						if ((client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
							Send01(L"You must progress\nfurther in the\ngame before you\ncan join this\nquest.", client, 121);
						else
							failed_to_join = 0;
					}
				}
				//可以加入游戏
				if (failed_to_join == 0)
				{
					removeClientFromLobby(client);
					client->lobbyNum = lobbyNum + 1;
					client->lobby = (void*)l;
					Send64(client);
					memset(&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDD;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = (unsigned char)EXPERIENCE_RATE;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x0C);
					UpdateGameItem(client);
				}
			}
			break;
		case 0x0F:
			// Quest selection 任务选择
			if ((blockServer) && (client->lobbyNum > 0x0F))
			{
				if (!client->lobby)
					break;

				l = (LOBBY*)client->lobby;

				if (client->decryptbuf[0x0B] == 0)
				{
					if (client->decryptbuf[0x0C] < 11)
						SendA2(l->episode, l->oneperson, client->decryptbuf[0x0C], client->decryptbuf[0x0A], client);
				}
				else
				{
					if (l->leader == client->clientID)
					{
						if (l->quest_loaded == 0)
						{
							if (client->decryptbuf[0x0B] <= numQuests)
							{
								q = &quests[client->decryptbuf[0x0B] - 1];

								do_quest = 1;

								// Check "One-Person" quest ability to repeat...检查 单人任务 是否可以重复

								if ((l->oneperson) && (l->episode == 0x01))
								{
									memcpy(&quest_num[0], &q->ql[0]->qdata[49], 3);
									quest_num[4] = 0;
									qn = atoi(&quest_num[0]);
									quest_flag = 0x63 + (qn << 1);
									if (qflag(&client->character.quest_data1[0], quest_flag, l->difficulty))
									{
										if (!qflag_ep1solo(&client->character.quest_data1[0], l->difficulty))
											do_quest = 0;
									}
									if (!do_quest)
										Send01(L"Please clear\nthe remaining\nquests before\nredoing this one.", client, 122);
								}

								// Check party Government quest qualification.  (Teamwork?!) 检查队伍的政府任务资格

								if (client->decryptbuf[0x0A])
								{
									memcpy(&dp[0], &q->ql[0]->qdata[0x31], 3);
									dp[4] = 0;
									qn = (unsigned)atoi(&dp[0]);
									switch (l->episode)
									{
									case 0x01:
										qn -= 401;
										qn <<= 1;
										qn += 0x1F3;
										for (ch2 = 0x1F5;ch2 <= qn;ch2 += 2)
											for (ch = 0;ch < 4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x02:
										qn -= 451;
										qn <<= 1;
										qn += 0x211;
										for (ch2 = 0x213;ch2 <= qn;ch2 += 2)
											for (ch = 0;ch < 4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x03:
										qn -= 701;
										qn += 0x2BC;
										for (ch2 = 0x2BD;ch2 <= qn;ch2++)
											for (ch = 0;ch < 4;ch++)
												if ((l->client[ch]) && (!qflag(&l->client[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									}
									if (!do_quest)
										Send01(L"The party no longer\nqualifies to\nstart this quest.", client, 123);
								}

								if (do_quest)
								{
									ch2 = 0; //初始化任务编号
									barule = 0; //初始化战斗规则

									while (q->ql[0]->qname[ch2] != 0x00)
									{
										// Search for a number in the quest name to determine battle rule # 在任务名称中搜索一个数字来确定战斗规则 sancaros
										if ((q->ql[0]->qname[ch2] >= 0x31) && (q->ql[0]->qname[ch2] <= 0x38))
										{
											if (l->challenge) {
												barule = 0x32;
											}
											else {
												barule = q->ql[0]->qname[ch2];
											}
											break;
										}
										ch2++;
									}

									for (ch = 0;ch < 4;ch++)
										if ((l->slot_use[ch]) && (l->client[ch]))
										{
											if ((l->battle) || (l->challenge) && l->client[ch]->guildcard) //如果为对战模式或者挑战模式
											{
												// Copy character to backup buffer.先将角色备份至缓冲
												if (l->client[ch]->character_backup) {
													free(l->client[ch]->character_backup);
												}

												l->client[ch]->character_backup = malloc(sizeof(l->client[ch]->character));

												memcpy(&l->client[ch]->challenge_data.challengeData, &l->client[ch]->character.challengeData, sizeof(l->client[ch]->character.challengeData));
												memcpy(&l->client[ch]->battle_data.battleData, &l->client[ch]->character.battleData, sizeof(l->client[ch]->character.battleData));
												memcpy(l->client[ch]->character_backup, &l->client[ch]->character, sizeof(l->client[ch]->character));

												l->battle_level = 0; //初始化战斗规则

												switch (barule) //编号49-56的战斗规则 sancaros
												{
												case 0x31:
													// Rule #1战斗规则
													l->client[ch]->mode = 1;
													WriteLog("使用 Rule #1战斗规则..."); //测试规则
													break;
												case 0x32:
													// Rule #2战斗规则
													l->battle_level = 1;
													l->client[ch]->mode = 3;
													WriteLog("使用 Rule #2战斗规则..."); //测试规则
													break;
												case 0x33:
													// Rule #3战斗规则
													l->battle_level = 5;
													l->client[ch]->mode = 3;
													WriteLog("使用 Rule #3战斗规则..."); //测试规则
													break;
												case 0x34:
													// Rule #4战斗规则
													l->battle_level = 2;
													l->client[ch]->mode = 3;
													l->meseta_boost = 1;
													WriteLog("使用 Rule #4战斗规则..."); //测试规则
													break;
												case 0x35:
													// Rule #5战斗规则
													l->client[ch]->mode = 2;
													l->meseta_boost = 1;
													WriteLog("使用 Rule #5战斗规则..."); //测试规则
													break;
												case 0x36:
													// Rule #6战斗规则
													l->battle_level = 20;
													l->client[ch]->mode = 3;
													WriteLog("使用 Rule #6战斗规则..."); //测试规则
													break;
												case 0x37:
													// Rule #7战斗规则
													l->battle_level = 1;
													l->client[ch]->mode = 3;
													WriteLog("使用 Rule #7战斗规则..."); //测试规则
													break;
												case 0x38:
													// Rule #8战斗规则
													l->battle_level = 20;
													l->client[ch]->mode = 3;
													WriteLog("使用 Rule #8战斗规则..."); //测试规则
													break;
												default:
													// 默认战斗规则
													WriteLog("使用默认规则的战斗规则..."); //测试规则
													l->battle_level = 20;
													l->client[ch]->mode = 3;
													break;
												}

												switch (l->client[ch]->mode)
												{
													//case 0x01:
												case 0x02:
													// Delete all mags and meseta...移除玛古和美赛塔 尝试通过这里来修复挑战模式
													for (ch2 = 0;ch2 < l->client[ch]->character.inventoryUse;ch2++)
													{
														if (l->client[ch]->character.inventory[ch2].item.data[0] == 0x02)
															l->client[ch]->character.inventory[ch2].in_use = 0;
													}
													CleanUpInventory(l->client[ch]);
													l->client[ch]->character.meseta = 0;
													break;
												case 0x03:
													// Wipe items and reset level.移除物品和等级 尝试通过这里来修复挑战模式
													for (ch2 = 0;ch2 < 30;ch2++)
														l->client[ch]->character.inventory[ch2].in_use = 0;
													CleanUpInventory(l->client[ch]);
													l->client[ch]->character.level = 0;
													l->client[ch]->character.XP = 0;
													l->client[ch]->character.ATP = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14)];
													l->client[ch]->character.MST = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14) + 2];
													l->client[ch]->character.EVP = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14) + 4];
													l->client[ch]->character.HP = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14) + 6];
													l->client[ch]->character.DFP = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14) + 8];
													l->client[ch]->character.ATA = *(unsigned short*)&startingData[(l->client[ch]->character._class * 14) + 10];
													if (l->battle_level > 1)
														SkipToLevel(l->battle_level - 1, l->client[ch], 1);
													l->client[ch]->character.meseta = 0;
												}
											}
											//显示任务的语言
											if ((l->client[ch]->character.lang < 10) &&
												(q->ql[l->client[ch]->character.lang]))
											{
												fqs = *(unsigned short*)&q->ql[l->client[ch]->character.lang]->qdata[0];
												if (fqs % 8)
													fqs += (8 - (fqs % 8));
												cipher_ptr = &l->client[ch]->server_cipher;
												encryptcopy(l->client[ch], &q->ql[l->client[ch]->character.lang]->qdata[0], fqs);
											}
											else
											{
												fqs = *(unsigned short*)&q->ql[0]->qdata[0];
												if (fqs % 8)
													fqs += (8 - (fqs % 8));
												cipher_ptr = &l->client[ch]->server_cipher;
												encryptcopy(l->client[ch], &q->ql[0]->qdata[0], fqs);
											}
											l->client[ch]->bursting = 1; //传送客户端
											l->client[ch]->sending_quest = client->decryptbuf[0x0B] - 1;
											l->client[ch]->qpos = fqs; //找到任务
										}
									if (!client->decryptbuf[0x0A])//如果客户端未收到指令 0A
										l->quest_in_progress = 1; // when a government quest, this won't be set 进行政府的任务，这是不会设置的 
									else
										l->inpquest = 1;

									l->quest_loaded = client->decryptbuf[0x0B]; //0B指令载入任务

																				// Time to load the map data...载入地图数据

									memset(&l->mapData[0], 0, 0xB50 * sizeof(MAP_MONSTER)); // Erase!初始化内存
									l->mapIndex = 0;
									l->rareIndex = 0;
									for (ch = 0;ch < 0x20;ch++)
										l->rareData[ch] = 0xFF;

									qmap = q->mapdata;
									qm_length = *(unsigned*)qmap;
									qmap += 4;
									ch = 4;
									while ((qm_length - ch) >= 80)
									{
										oldIndex = l->mapIndex;
										qa = *(unsigned*)qmap; // Area区域
										qmap += 4;
										nr = *(unsigned*)qmap; // Number of monsters怪物数量
										qmap += 4;
										if ((l->episode == 0x03) && (qa > 5))
											ParseMapData(l, (MAP_MONSTER*)qmap, 1, nr);
										else
											if ((l->episode == 0x02) && (qa > 15))
												ParseMapData(l, (MAP_MONSTER*)qmap, 1, nr);
											else
												ParseMapData(l, (MAP_MONSTER*)qmap, 0, nr);
										qmap += (nr * 72);
										ch += ((nr * 72) + 8);
										//debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
									}
								}
							}
						}
						else
							Send01(L"Quest already loaded.", client, 124);
					}
					else
						Send01(L"Only the leader of a team can start quests.", client, 125);
				}
			}
			break;
		case 0xEF:
			if (client->lobbyNum < 0x10)
			{
				// Blocks 舰仓

				unsigned blockNum;

				blockNum = 0x100 - selected;//最大256 减去已选择的客户端数量

				if (blockNum <= serverBlocks)
				{
					if (blocks[blockNum - 1]->count < 180)
					{
						if ((client->lobbyNum) && (client->lobbyNum < 0x10))
						{
							for (ch = 0;ch < MAX_SAVED_LOBBIES;ch++)
							{
								if (savedlobbies[ch].guildcard == 0)
								{
									savedlobbies[ch].guildcard = client->guildcard;
									savedlobbies[ch].lobby = client->lobbyNum;
									break;
								}
							}
						}

						if (client->gotchardata)
						{
							client->character.playTime += (unsigned)servertime - client->connected;
							ShipSend04(0x02, client, logon);
							//ShipSend04(0x03, client, logon);//30挑战
							//ShipSend04(0x04, client, logon);//30对战
							client->gotchardata = 0;
							client->released = 1;
							*(unsigned*)&client->releaseIP[0] = *(unsigned*)&serverIP[0];
							client->releasePort = serverPort + blockNum;
						}
						else
							Send19(serverIP[0], serverIP[1], serverIP[2], serverIP[3],
								serverPort + blockNum, client);
					}
					else
					{
						Send01(L"Block is full.", client, 126);
						Send07(client);
					}
				}
			}
			break;
		case 0xFF:
			if (client->lobbyNum < 0x10)
			{
				// Ship select 舰船选择
				if (selected == 0x00)
					ShipSend0D(0x00, client, logon);
				else
					// Ships 舰船
					for (ch = 0;ch < totalShips;ch++)
					{
						if (full_select == shipdata[ch].shipID)
						{
							if (client->gotchardata)
							{
								client->character.playTime += (unsigned)servertime - client->connected;
								ShipSend04(0x02, client, logon);
								//ShipSend04(0x03, client, logon);//30挑战
								//ShipSend04(0x04, client, logon);//30对战
								client->gotchardata = 0;
								client->released = 1;
								*(unsigned*)&client->releaseIP[0] = *(unsigned*)&shipdata[ch].ipaddr[0];
								client->releasePort = shipdata[ch].port;
							}
							else
								//发送舰船IP端口地址
								Send19(shipdata[ch].ipaddr[0], shipdata[ch].ipaddr[1],
									shipdata[ch].ipaddr[2], shipdata[ch].ipaddr[3],
									shipdata[ch].port, client);

							break;
						}
					}
			}
			break;
		default:
			break;
		}
	}
}

//玩家信息板代码
void CommandD9(BANANA* client)
{
	unsigned short* n;
	unsigned short* g;
	unsigned short s = 2;


	// Client writing to info board 客户端写入信息板

	n = (unsigned short*)&client->decryptbuf[0x0A];
	g = (unsigned short*)&client->character.GCBoard[0];

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 85))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	// null terminate 空为停止
	*(g++) = 0x0000;
}

//增加名片代码
void AddGuildCard(unsigned myGC, unsigned friendGC, unsigned char* friendName,
	unsigned char* friendText, unsigned char friendSecID, unsigned char friendClass,
	ORANGE* ship)
{
	// Instruct the logon server to add the guild card命令登录服务器添加公会卡

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x00;
	*(unsigned*)&ship->encryptbuf[0x02] = myGC;
	*(unsigned*)&ship->encryptbuf[0x06] = friendGC;
	memcpy(&ship->encryptbuf[0x0A], friendName, 24);
	memcpy(&ship->encryptbuf[0x22], friendText, 176);
	ship->encryptbuf[0xD2] = friendSecID;
	ship->encryptbuf[0xD3] = friendClass;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0xD4);
}

//删除名片卡代码
void DeleteGuildCard(unsigned myGC, unsigned friendGC, ORANGE* ship)
{
	// Instruct the logon server to delete the guild card命令登录服务器删除公会卡

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned*)&ship->encryptbuf[0x02] = myGC;
	*(unsigned*)&ship->encryptbuf[0x06] = friendGC;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
}

//修改名片卡代码
void ModifyGuildCardComment(unsigned myGC, unsigned friendGC, unsigned short* n, ORANGE* ship)
{
	unsigned s = 1;
	unsigned short* g;

	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*)&ship->encryptbuf[0x02] = myGC;
	*(unsigned*)&ship->encryptbuf[0x06] = friendGC;

	// Client writing to info board客户端写入信息板

	g = (unsigned short*)&ship->encryptbuf[0x0A];

	memset(g, 0, 0x44);

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 33))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	*(g++) = 0x0000;

	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x4E);
}

//整理名片卡代码
void SortGuildCard(BANANA* client, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x07;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*)&ship->encryptbuf[0x02] = client->guildcard;
	*(unsigned*)&ship->encryptbuf[0x06] = *(unsigned*)&client->decryptbuf[0x08];
	*(unsigned*)&ship->encryptbuf[0x0A] = *(unsigned*)&client->decryptbuf[0x0C];
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x10);
}

//名片卡相关设置代码
void CommandE8(BANANA* client)
{
	unsigned gcn;


	switch (client->decryptbuf[0x03])
	{
	case 0x04:
	{
		// Accepting sent guild card 接收公会卡 登陆服务器那边有注释
		LOBBY* l;
		BANANA* lClient;
		unsigned ch, maxch;

		if (!client->lobby)
			break;

		l = (LOBBY*)client->lobby;
		gcn = *(unsigned*)&client->decryptbuf[0x08];
		if (client->lobbyNum < 0x10)
			maxch = 12;
		else
			maxch = 4;
		for (ch = 0;ch < maxch;ch++)
		{
			if ((l->client[ch]) && (l->client[ch]->character.guildCard == gcn))
			{
				lClient = l->client[ch];
				if (PreppedGuildCard(lClient->guildcard, client->guildcard))
				{
					AddGuildCard(client->guildcard, gcn, &client->decryptbuf[0x0C], &client->decryptbuf[0x5C],
						client->decryptbuf[0x10E], client->decryptbuf[0x10F], logon);
				}
				break;
			}
		}
	}
	break;
	case 0x05:
		// Deleting a guild card 登陆服务器那边有注释
		gcn = *(unsigned*)&client->decryptbuf[0x08];
		DeleteGuildCard(client->guildcard, gcn, logon);
		break;
	case 0x06:
		// Setting guild card text 登陆服务器那边有注释
	{
		unsigned short* n;
		unsigned short* g;
		unsigned short s = 2;

		// Client writing to info board 登陆服务器那边有注释

		n = (unsigned short*)&client->decryptbuf[0x5E];
		g = (unsigned short*)&client->character.GCBoard[0];

		*(g++) = 0x0009;

		while ((*n != 0x0000) && (s < 85))
		{
			if ((*n == 0x0009) || (*n == 0x000A))
				*(g++) = 0x0020;
			else
				*(g++) = *n;
			n++;
			s++;
		}
		// null terminate
		*(g++) = 0x0000;
	}
	break;
	case 0x07:
		// Add blocked user 登陆服务器那边有注释
		// User @ 0x08, Name of User @ 0x0C
		break;
	case 0x08:
		// Remove blocked user 登陆服务器那边有注释
		// User @ 0x08
		break;
	case 0x09:
		// Write comment on user 登陆服务器那边有注释
		// E8 09 writing a comment on a user...  not sure were comment goes in the DC packet... 正在为用户写评论。。。不确定DC包中是否有评论
		// User @ 0x08 comment @ 0x0C
		gcn = *(unsigned*)&client->decryptbuf[0x08];
		ModifyGuildCardComment(client->guildcard, gcn, (unsigned short*)&client->decryptbuf[0x0E], logon);
		break;
	case 0x0A:
		// Sort guild card 整理公会卡
		// (Moves from one position to another)
		SortGuildCard(client, logon);
		break;
	}
}

//显示信息板代码
void CommandD8(BANANA* client)
{
	unsigned ch, maxch;
	unsigned short D8Offset;
	unsigned char totalClients = 0;
	LOBBY* l;
	BANANA* lClient;

	if (!client->lobby)
		return;

	memset(&PacketData[0], 0, 8);

	PacketData[0x02] = 0xD8;
	D8Offset = 8;

	l = client->lobby;

	if (client->lobbyNum < 0x10)
		maxch = 12;
	else
		maxch = 4;

	for (ch = 0;ch < maxch;ch++)
	{
		if ((l->slot_use[ch]) && (l->client[ch]))
		{
			totalClients++;
			lClient = l->client[ch];
			memcpy(&PacketData[D8Offset], &lClient->character.name[4], 20);
			D8Offset += 0x20;
			memcpy(&PacketData[D8Offset], &lClient->character.GCBoard[0], 172);
			D8Offset += 0x158;
		}
	}
	PacketData[0x04] = totalClients;
	*(unsigned short*)&PacketData[0x00] = (unsigned short)D8Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy(client, &PacketData[0], D8Offset);
}

//全服公告代码
void Command81(BANANA* client, ORANGE* ship)
{
	unsigned short* n;

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x03;
	memcpy(&ship->encryptbuf[0x02], &client->decryptbuf[0x00], 0x45C);
	*(unsigned*)&ship->encryptbuf[0x0E] = client->guildcard;
	memcpy(&ship->encryptbuf[0x12], &client->character.name[0], 24);
	n = (unsigned short*)&ship->encryptbuf[0x62];
	while (*n != 0x0000)
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*n = 0x0020;
		n++;
	}
	*n = 0x0000;
	*(unsigned*)&ship->encryptbuf[0x45E] = client->character.teamID;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x462);
}

//创建公会
void CreateTeam(unsigned short* teamname, unsigned guildcard, ORANGE* ship)
{
	unsigned short* g;
	unsigned n;

	n = 0;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x00;

	g = (unsigned short*)&ship->encryptbuf[0x02];

	memset(g, 0, 24);
	while ((*teamname != 0x0000) && (n < 11))
	{
		if ((*teamname != 0x0009) && (*teamname != 0x000A))
			*(g++) = *teamname;
		else
			*(g++) = 0x0020;
		teamname++;
		n++;
	}
	*(unsigned*)&ship->encryptbuf[0x1A] = guildcard;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x1E);
}

//更新公会旗帜
void UpdateTeamFlag(unsigned char* flag, unsigned teamid, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x01;
	memcpy(&ship->encryptbuf[0x02], flag, 0x800);
	*(unsigned*)&ship->encryptbuf[0x802] = teamid;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x806);
}

//解散公会
void DissolveTeam(unsigned teamid, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x02;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x06);
}

//移除公会成员
void RemoveTeamMember(unsigned teamid, unsigned guildcard, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x03;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	*(unsigned*)&ship->encryptbuf[0x06] = guildcard;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
}

//公会聊天
void TeamChat(unsigned short* text, unsigned short chatsize, unsigned teamid, ORANGE* ship)
{
	unsigned size;

	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x04;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	while (chatsize % 8)
		ship->encryptbuf[6 + (chatsize++)] = 0x00;
	*text = chatsize;
	memcpy(&ship->encryptbuf[0x06], text, chatsize);
	size = chatsize + 6;
	compressShipPacket(ship, &ship->encryptbuf[0x00], size);
}

//获取公会列表
void RequestTeamList(unsigned teamid, unsigned guildcard, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x05;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	*(unsigned*)&ship->encryptbuf[0x06] = guildcard;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
}

//提升公会成员
void PromoteTeamMember(unsigned teamid, unsigned guildcard, unsigned char newlevel, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x06;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	*(unsigned*)&ship->encryptbuf[0x06] = guildcard;
	(unsigned char)ship->encryptbuf[0x0A] = newlevel;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0B);
}

//新增公会成员
void AddTeamMember(unsigned teamid, unsigned guildcard, ORANGE* ship)
{
	ship->encryptbuf[0x00] = 0x09;
	ship->encryptbuf[0x01] = 0x07;
	*(unsigned*)&ship->encryptbuf[0x02] = teamid;
	*(unsigned*)&ship->encryptbuf[0x06] = guildcard;
	compressShipPacket(ship, &ship->encryptbuf[0x00], 0x0A);
}


void CommandEA(BANANA* client, ORANGE* ship)
{
	unsigned connectNum;

	if ((client->decryptbuf[0x03] < 32) && ((unsigned)servertime - client->team_cooldown[client->decryptbuf[0x03]] >= 1))
	{
		client->team_cooldown[client->decryptbuf[0x03]] = (unsigned)servertime;
		switch (client->decryptbuf[0x03])
		{
		case 0x01:
			// Create team 创建一个公会
			if (client->character.teamID == 0)
				CreateTeam((unsigned short*)&client->decryptbuf[0x0C], client->guildcard, ship);
			break;
		case 0x03:
			// Add a team member 新增一名成员
		{
			BANANA* tClient;
			unsigned gcn, ch;

			if ((client->character.teamID != 0) && (client->character.privilegeLevel >= 0x30))
			{
				gcn = *(unsigned*)&client->decryptbuf[0x08];
				for (ch = 0;ch < serverNumConnections;ch++)
				{
					connectNum = serverConnectionList[ch];
					if (connections[connectNum]->guildcard == gcn)
					{
						if ((connections[connectNum]->character.teamID == 0) && (connections[connectNum]->teamaccept == 1))
						{
							AddTeamMember(client->character.teamID, gcn, ship);
							tClient = connections[connectNum];
							tClient->teamaccept = 0;
							memset(&tClient->character.serial_number, 0, 2108);
							tClient->character.teamID = client->character.teamID;
							tClient->character.privilegeLevel = 0;
							tClient->character.teamRank = client->character.teamRank;
							memcpy(&tClient->character.teamName[0], &client->character.teamName[0], 28);
							memcpy(&tClient->character.teamFlag[0], &client->character.teamFlag[0], 2048);
							*(long long*)&tClient->character.teamRewards[0] = *(long long*)&client->character.teamRewards[0];
							if (tClient->lobbyNum < 0x10)
								SendToLobby(tClient->lobby, 12, MakePacketEA15(tClient), 2152, 0);
							else
								SendToLobby(tClient->lobby, 4, MakePacketEA15(tClient), 2152, 0);
							SendEA(0x12, tClient);
							SendEA(0x04, client);
							SendEA(0x04, tClient);
							break;
						}
						else
							Send01(L"Player already\nbelongs to a team!", client, 127);
					}
				}
			}
		}
		break;
		case 0x05:
			// Remove member from team 移除公会成员
			if (client->character.teamID != 0)
			{
				unsigned gcn, ch;
				BANANA* tClient;

				gcn = *(unsigned*)&client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						RemoveTeamMember(client->character.teamID, gcn, ship);
						SendEA(0x06, client);
						for (ch = 0;ch < serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if (tClient->character.privilegeLevel < client->character.privilegeLevel)
								{
									memset(&tClient->character.serial_number, 0, 2108);
									memset(&client->encryptbuf[0x00], 0, 0x40);
									client->encryptbuf[0x00] = 0x40;
									client->encryptbuf[0x02] = 0xEA;
									client->encryptbuf[0x03] = 0x12;
									*(unsigned*)&client->encryptbuf[0x0C] = tClient->guildcard;
									if (tClient->lobbyNum < 0x10)
									{
										SendToLobby(tClient->lobby, 12, MakePacketEA15(tClient), 2152, 0);
										SendToLobby(tClient->lobby, 12, &client->encryptbuf[0x00], 0x40, 0);
									}
									else
									{
										SendToLobby(tClient->lobby, 4, MakePacketEA15(tClient), 2152, 0);
										SendToLobby(tClient->lobby, 4, &client->encryptbuf[0x00], 0x40, 0);
									}
									Send01(L"Member removed.", client, 128);
								}
								else
									Send01(L"Your privilege level is\ntoo low.", client, 129);
								break;
							}
						}
					}
					else
						Send01(L"Your privilege level is\ntoo low.", client, 129);
				}
				else
				{
					RemoveTeamMember(client->character.teamID, gcn, ship);
					memset(&client->character.serial_number, 0, 2108);
					memset(&client->encryptbuf[0x00], 0, 0x40);
					client->encryptbuf[0x00] = 0x40;
					client->encryptbuf[0x02] = 0xEA;
					client->encryptbuf[0x03] = 0x12;
					*(unsigned*)&client->encryptbuf[0x0C] = client->guildcard;
					if (client->lobbyNum < 0x10)
					{
						SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, 0);
						SendToLobby(client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0);
					}
					else
					{
						SendToLobby(client->lobby, 4, MakePacketEA15(client), 2152, 0);
						SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0);
					}
				}
			}
			break;
		case 0x07:
			// 公会聊天
			if (client->character.teamID != 0)
			{
				unsigned short size;
				unsigned short* n;

				size = *(unsigned short*)&client->decryptbuf[0x00];

				if (size > 0x2B)
				{
					n = (unsigned short*)&client->decryptbuf[0x2C];
					while (*n != 0x0000)
					{
						if ((*n == 0x0009) || (*n == 0x000A))
							*n = 0x0020;
						n++;
					}
					TeamChat((unsigned short*)&client->decryptbuf[0x00], size, client->character.teamID, ship);
				}
			}
			break;
		case 0x08:
			// Member Promotion / Demotion / Expulsion / Master Transfer
			//会员升职/降职/开除/调任
			if (client->character.teamID != 0)
				RequestTeamList(client->character.teamID, client->guildcard, ship);
			break;
		case 0x0D:
			SendEA(0x0E, client);
			break;
		case 0x0F:
			// Set flag
			// 设置公会旗帜
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
				UpdateTeamFlag(&client->decryptbuf[0x08], client->character.teamID, ship);
			break;
		case 0x10:
			// Dissolve team
			// 解散队伍
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
			{
				DissolveTeam(client->character.teamID, ship);
				SendEA(0x10, client);
				memset(&client->character.serial_number, 0, 2108);
				SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, 0);
				SendEA(0x12, client);
			}
			break;
		case 0x11:
			// 对公会会员的操作
			// Promote member 提升会员
			if (client->character.teamID != 0)
			{
				unsigned gcn, ch;
				BANANA* tClient;

				gcn = *(unsigned*)&client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						PromoteTeamMember(client->character.teamID, gcn, client->decryptbuf[0x04], ship);

						if (client->decryptbuf[0x04] == 0x40)
						{
							// Master Transfer
							PromoteTeamMember(client->character.teamID, client->guildcard, 0x30, ship);
							client->character.privilegeLevel = 0x30;
							SendToLobby(client->lobby, 12, MakePacketEA15(client), 2152, 0);
						}

						for (ch = 0;ch < serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if (tClient->character.privilegeLevel != client->decryptbuf[0x04]) // only if changed
								{
									tClient->character.privilegeLevel = client->decryptbuf[0x04];
									if (tClient->lobbyNum < 0x10)
										SendToLobby(tClient->lobby, 12, MakePacketEA15(tClient), 2152, 0);
									else
										SendToLobby(tClient->lobby, 4, MakePacketEA15(tClient), 2152, 0);
								}
								SendEA(0x12, tClient);
								SendEA(0x11, client);
								break;
							}
						}
					}
				}
			}
			break;
		case 0x13:
			// A type of lobby list...
			SendEA(0x13, client);
			break;
		case 0x14:
			// Do nothing.什么也不做 应该是关闭了吧
			break;
		case 0x18:
			// Buying privileges and point information
			SendEA(0x18, client);
			break;
		case 0x19:
			// Privilege list
			SendEA(0x19, client);
			break;
		case 0x1C:
			// Ranking 公会排名 SendEA 中 0x1C 未完成的公会排行榜
			Send1A(L"Tethealla Ship Server coded by Sodaboy\nhttp://www.pioneer2.net/\n\nEnjoy!", client, 101);
			break;
		case 0x1A://不知道做什么用的
			SendEA(0x1A, client);
			break;
		default:
			break;
		}
	}
}

//显示指示箭头,应该就是指选择器
void ShowArrows(BANANA* client, int to_all)
{
	LOBBY* l;
	unsigned ch, total_clients, Packet88Offset;

	total_clients = 0;
	memset(&PacketData[0x00], 0, 8);
	PacketData[0x02] = 0x88;
	PacketData[0x03] = 0x00;
	Packet88Offset = 8;

	if (!client->lobby)
		return;
	l = (LOBBY*)client->lobby;

	for (ch = 0;ch < 12;ch++)
	{
		if ((l->slot_use[ch] != 0) && (l->client[ch]))
		{
			total_clients++;
			PacketData[Packet88Offset + 2] = 0x01;
			*(unsigned*)&PacketData[Packet88Offset + 4] = l->client[ch]->character.guildCard;
			PacketData[Packet88Offset + 8] = l->arrow_color[ch];
			Packet88Offset += 12;
		}
	}
	*(unsigned*)&PacketData[0x04] = total_clients;
	*(unsigned short*)&PacketData[0x00] = (unsigned short)Packet88Offset;
	if (to_all)
		SendToLobby(client->lobby, 12, &PacketData[0x00], Packet88Offset, 0);
	else
	{
		cipher_ptr = &client->server_cipher;
		encryptcopy(client, &PacketData[0x00], Packet88Offset);
	}
}

//舰仓切换代码
void BlockProcessPacket(BANANA* client)
{
	if (client->guildcard)
	{
		switch (client->decryptbuf[0x02])
		{
			//这里截断无任何效果
		case 0x05:
			//当用户离线时显示
			//WriteLog("用户自动离线时显示");
			break;
		case 0x06:
			//计算GM指令冷却时间的
			if (((unsigned)servertime - client->command_cooldown[0x06]) >= 1)
			{
				client->command_cooldown[0x06] = (unsigned)servertime;
				//WriteLog("test222222 1223123123123123123123123");
				Send06(client);
			}
			break;
		case 0x08:
			// Get game list 获取游戏房间列表
			if ((client->lobbyNum < 0x10) && (((unsigned)servertime - client->command_cooldown[0x08]) >= 1))
			{
				client->command_cooldown[0x08] = (unsigned)servertime;
				Send08(client);
			}
			else
				if (client->lobbyNum < 0x10)
					Send01(L"You must wait\nawhile before\ntrying that.", client, 130);
			break;
		case 0x09:
			//任务房间相关的 可能和职业ID颜色有关
			Command09(client);
			break;
		case 0x10:
			//房间任务相关
			Command10(1, client);
			break;
		case 0x1D:
			client->response = (unsigned)servertime;
			break;
		case 0x40:
			// Guild Card search 公会卡搜索
			if (((unsigned)servertime - client->command_cooldown[0x40]) >= 1)
			{
				client->command_cooldown[0x40] = (unsigned)servertime;
				Command40(client, logon);
			}
			break;
		case 0x13:
		case 0x44:
			if ((client->lobbyNum > 0x0F) && (client->sending_quest != -1))
			{
				unsigned short qps;

				if ((client->character.lang < 10) && (quests[client->sending_quest].ql[client->character.lang]))
				{
					if (client->qpos < quests[client->sending_quest].ql[client->character.lang]->qsize)
					{
						qps = *(unsigned short*)&quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos];
						if (qps % 8)
							qps += (8 - (qps % 8));
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &quests[client->sending_quest].ql[client->character.lang]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
				else
				{
					if (client->qpos < quests[client->sending_quest].ql[0]->qsize)
					{
						qps = *(unsigned short*)&quests[client->sending_quest].ql[0]->qdata[client->qpos];
						if (qps % 8)
							qps += (8 - (qps % 8));
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &quests[client->sending_quest].ql[0]->qdata[client->qpos], qps);
						client->qpos += qps;
					}
					else
						client->sending_quest = -1;
				}
			}
			break;
		case 0x60:
			if ((client->bursting) && (client->decryptbuf[0x08] == 0x23) && (client->lobbyNum < 0x10) && (client->lobbyNum))
			{
				// If a client has just appeared, send him the team information of everyone in the lobby 如果一个客户端出现了,给他发送在大厅的所有人房间信息
				if (client->guildcard)
				{
					unsigned ch;
					LOBBY* l;

					if (!client->lobby)
						break;

					l = client->lobby;

					for (ch = 0;ch < 12;ch++)
						if ((l->slot_use[ch] != 0) && (l->client[ch]))
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy(client, MakePacketEA15(l->client[ch]), 2152);
						}
					ShowArrows(client, 0);
					client->bursting = 0;
				}
			}
			// Lots of fun commands here. 这里又很多有趣的指令
			Send60(client);
			break;
		case 0x61://这里漏了参数
			memcpy(&client->character.option_flags[0], &client->decryptbuf[0x362], 10); //提前载入游戏设置
			Send67(client, client->preferred_lobby);
			break;
		case 0x62:
			Send62(client);
			break;
		case 0x6D:
			if (client->lobbyNum > 0x0F)
				Send6D(client);
			else
				initialize_connection(client);
			//WriteLog("debug客户端断点12");
			break;
		case 0x6F:
			if ((client->lobbyNum > 0x0F) && (client->bursting))
			{
				LOBBY* l;
				unsigned short fqs, ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				if ((l->inpquest) && (!client->hasquest))
				{
					// Send the quest 发送任务
					client->bursting = 1;
					if ((client->character.lang < 10) && (quests[l->quest_loaded - 1].ql[client->character.lang]))
					{
						fqs = *(unsigned short*)&quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0];
						if (fqs % 8)
							fqs += (8 - (fqs % 8));
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &quests[l->quest_loaded - 1].ql[client->character.lang]->qdata[0], fqs);
					}
					else
					{
						fqs = *(unsigned short*)&quests[l->quest_loaded - 1].ql[0]->qdata[0];
						if (fqs % 8)
							fqs += (8 - (fqs % 8));
						client->sending_quest = l->quest_loaded - 1;
						client->qpos = fqs;
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &quests[l->quest_loaded - 1].ql[0]->qdata[0], fqs);
					}
				}
				else
				{
					// Rare monster data go...稀有怪物数据
					memset(&client->encryptbuf[0x00], 0, 0x08);
					client->encryptbuf[0x00] = 0x28;
					client->encryptbuf[0x02] = 0xDE;
					memcpy(&client->encryptbuf[0x08], &l->rareData[0], 0x20);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x28);
					memset(&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0x72;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = 0x18;
					client->encryptbuf[0x0B] = 0x08;
					SendToLobby(client->lobby, 4, &client->encryptbuf[0x00], 0x0C, 0);
					for (ch = 0;ch < 4;ch++)
						if ((l->slot_use[ch] != 0) && (l->client[ch]))
						{
							cipher_ptr = &client->server_cipher;
							encryptcopy(client, MakePacketEA15(l->client[ch]), 2152);
						}
					client->bursting = 0;
				}
			}
			break;
		case 0x81:
			if (client->announce)
			{
				if (client->announce == 1)
					WriteGM("管理员 %u 发布公告: %s", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->decryptbuf[0x60]));
				BroadcastToAll((unsigned short*)&client->decryptbuf[0x60], client);
			}
			else
			{
				if (((unsigned)servertime - client->command_cooldown[0x81]) >= 1)
				{
					client->command_cooldown[0x81] = (unsigned)servertime;
					Command81(client, logon);
				}
			}
			break;
		case 0x84:
			if (client->decryptbuf[0x0C] < 0x0F)
			{
				BLOCK* b;
				b = blocks[client->block - 1];
				if (client->lobbyNum > 0x0F)
				{
					removeClientFromLobby(client);
					client->preferred_lobby = client->decryptbuf[0x0C];
					Send95(client);
				}
				else
				{
					if (((unsigned)servertime - client->command_cooldown[0x84]) >= 1)
					{
						if (b->lobbies[client->decryptbuf[0x0C]].lobbyCount < 12)
						{
							client->command_cooldown[0x84] = (unsigned)servertime;
							removeClientFromLobby(client);
							client->preferred_lobby = client->decryptbuf[0x0C];
							Send95(client);
						}
						else
							Send01(L"Lobby is full!", client, 131);
					}
					else
						Send01(L"You must wait\nawhile before\ntrying that.", client, 130);
				}
			}
			break;
			//(client->lobbyNum < 0x10) 判断客户端在1-15的大厅
		case 0x89:
			if ((client->lobbyNum < 0x10) && (client->lobbyNum) && (((unsigned)servertime - client->command_cooldown[0x89]) >= 1))
			{
				LOBBY* l; //赋值 l 为大厅

				client->command_cooldown[0x89] = (unsigned)servertime;
				if (!client->lobby)
					break;
				l = client->lobby;
				l->arrow_color[client->clientID] = client->decryptbuf[0x04];
				ShowArrows(client, 1);
			}
			break;
			//if (client->lobbyNum > 0x0F) 判断客户端不在大厅
			//这里应该是获取
		case 0x8A:
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;
				if (l->in_use)
				{
					memset(&PacketData[0], 0, 0x28);
					PacketData[0x00] = 0x28;
					PacketData[0x02] = 0x8A;
					memcpy(&PacketData[0x08], &l->gameName[0], 30);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &PacketData[0], 0x28);
				}
			}
			break;
		case 0xA0:
			// Ship list 舰船列表
			if (client->lobbyNum < 0x10)
				ShipSend0D(0x00, client, logon);
			break;
		case 0xA1:
			// Block list 舰仓列表
			if (client->lobbyNum < 0x10)
				Send07(client);
			break;
		case 0xA2:
			// Quest list 任务列表
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;

				if (!client->lobby)
					break;
				l = client->lobby;

				if (l->floor[client->clientID] == 0)
				{
					if (client->decryptbuf[0x04])
						SendA2(l->episode, l->oneperson, 0, 1, client);
					else
						SendA2(l->episode, l->oneperson, 0, 0, client);
				}
			}
			break;
		case 0xAC:
			// Quest load complete 任务加载完成
			if (client->lobbyNum > 0x0F)
			{
				LOBBY* l;
				int all_quest;
				unsigned ch;

				if (!client->lobby)
					break;

				l = client->lobby;

				client->hasquest = 1;
				all_quest = 1;

				for (ch = 0;ch < 4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]) && (l->client[ch]->hasquest == 0))
						all_quest = 0;
				}

				if (all_quest)
				{
					// Send the 0xAC when all clients have the quest

					client->decryptbuf[0x00] = 0x08;
					client->decryptbuf[0x01] = 0x00;
					SendToLobby(l, 4, &client->decryptbuf[0x00], 8, 0);
				}

				client->sending_quest = -1;

				if ((l->inpquest) && (client->bursting))
				{
					// Let the leader know it's time to send the remaining state of the quest...
					cipher_ptr = &l->client[l->leader]->server_cipher;
					memset(&client->encryptbuf[0], 0, 8);
					client->encryptbuf[0] = 0x08;
					client->encryptbuf[2] = 0xDD;
					client->encryptbuf[4] = client->clientID;
					encryptcopy(l->client[l->leader], &client->encryptbuf[0], 8);
				}
			}
			break;
		case 0xC1:
			// Create game 创建游戏
			if (client->lobbyNum < 0x10)
			{
				//if (client->decryptbuf[0x52])
				//挑战模式进入房间
				//Send1A("Challenge games are NOT supported right now.\nCheck back later.\n\n- Sancaros", client, 102);
				//else
				//{

				unsigned lNum, failed_to_create;

				failed_to_create = 0;

				if ((!client->isgm) && (!isLocalGM(client->guildcard)))
				{
					if ((client->decryptbuf[0x53] == 0x03) && (client->decryptbuf[0x52] == 0x00
						|| client->decryptbuf[0x52] == 0x01
						|| client->decryptbuf[0x52] == 0x02
						|| client->decryptbuf[0x52] == 0x03))
					{
						//挑战模式进入EP4房间
						Send1A(L"Challenge games are NOT supported EP4 right now.\nCheck back later.\n\n- Sancaros", client, 102);
						failed_to_create = 1;
					}
					else
						//episode 章节选择
						switch (client->decryptbuf[0x53])
						{
						case 0x01:
							if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 19))
							{
								Send01(L"Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client, 105);
								failed_to_create = 1;
							}
							else
								if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 49))
								{
									Send01(L"Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client, 106);
									failed_to_create = 1;
								}
								else
									if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 89))
									{
										Send01(L"Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client, 107);
										failed_to_create = 1;
									}
							break;
						case 0x02:
							if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 29))
							{
								Send01(L"Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client, 108);
								failed_to_create = 1;
							}
							else
								if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 59))
								{
									Send01(L"Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client, 109);
									failed_to_create = 1;
								}
								else
									if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 99))
									{
										Send01(L"Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client, 110);
										failed_to_create = 1;
									}
							break;
						case 0x03:
							if ((client->decryptbuf[0x50] == 0x01) && (client->character.level < 39))
							{
								Send01(L"Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client, 111);
								failed_to_create = 1;
							}
							else
								if ((client->decryptbuf[0x50] == 0x02) && (client->character.level < 69))
								{
									Send01(L"Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client, 112);
									failed_to_create = 1;
								}
								else
									if ((client->decryptbuf[0x50] == 0x03) && (client->character.level < 109))
									{
										Send01(L"Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client, 113);
										failed_to_create = 1;
									}
							break;
						default:
							SendB0(L"Lol, nub.", client, -1);
							break;
						}
				}

				if (!failed_to_create)
				{
					lNum = free_game(client);//房间号
					if (lNum)
					{
						removeClientFromLobby(client);//将客户端从大厅移除
						client->lobbyNum = (unsigned short)lNum + 1;
						client->lobby = &blocks[client->block - 1]->lobbies[lNum];
						initialize_game(client);//加载游戏
						Send64(client);//不太懂 应该是发送什么东西
						memset(&client->encryptbuf[0x00], 0, 0x0C);
						client->encryptbuf[0x00] = 0x0C;
						client->encryptbuf[0x02] = 0x60;
						client->encryptbuf[0x08] = 0xDD;
						client->encryptbuf[0x09] = 0x03;
						client->encryptbuf[0x0A] = (unsigned char)EXPERIENCE_RATE;
						cipher_ptr = &client->server_cipher;
						encryptcopy(client, &client->encryptbuf[0x00], 0x0C); //输出游戏数据
						UpdateGameItem(client); //更新游戏物品
					}
					else
						Send01(L"Sorry, limit of game\ncreation has been\nreached.\n\nPlease join a game\nor change ships.", client, 132);
				}
				//}
			}
			break;
		case 0xD8:
			// Show info board 显示信息板
			if (((unsigned)servertime - client->command_cooldown[0xD8]) >= 1)
			{
				client->command_cooldown[0xD8] = (unsigned)servertime;
				CommandD8(client);
			}
			break;
		case 0xD9:
			// Write on info board 编辑信息板
			CommandD9(client);
			break;
		case 0xE7:
			// Client sending character data... 客户端离线发送角色数据
			if (client->guildcard)
			{
				if ((client->isgm) || (isLocalGM(client->guildcard)))
					WriteGM("管理员 %u (%s) 已离线", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
				else
					WriteLog("玩家 %u (%s) 已离线", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
				client->todc = 1;
			}
			break;
		case 0xE8:
			// Guild card stuff 工会卡的东西
			CommandE8(client);
			break;
		case 0xEA:
			// Team shit 公会信息
			CommandEA(client, logon);
			break;
		case 0xED:
			// Set options 设置选项
			CommandED(client);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (client->decryptbuf[0x02])
		{
		case 0x05:
			printf("客户端已离开港口.\n");
			client->todc = 1;
			break;
		case 0x93:
		{
			unsigned ch, ch2, ipaddr;
			int banned = 0, match;

			client->temp_guildcard = *(unsigned*)&client->decryptbuf[0x0C];
			client->hwinfo = *(long long*)&client->decryptbuf[0x84];
			ipaddr = *(unsigned*)&client->ipaddr[0];
			for (ch = 0;ch < num_bans;ch++)
			{
				if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
				{
					banned = 1;
					break;
				}
				if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
				{
					banned = 1;
					break;
				}
			}

			for (ch = 0;ch < num_masks;ch++)
			{
				match = 1;
				for (ch2 = 0;ch2 < 4;ch2++)
				{
					if ((ship_banmasks[ch][ch2] != 0x8000) &&
						((unsigned char)ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
						match = 0;
				}
				if (match)
				{
					banned = 1;
					break;
				}
			}

			if (banned)
			{
				Send1A(L"You are banned from this ship.", client, 103);
				client->todc = 1;
			}
			else
				if (!client->sendCheck[RECEIVE_PACKET_93])
				{
					ShipSend0B(client, logon);
					client->sendCheck[RECEIVE_PACKET_93] = 0x01;
					printf("允许客户端登船.\n");
				}
		}
		break;

		default:
			printf("无效数据包.\n");
			break;
		}
	}
}

//舰船传输数据包代码
void ShipProcessPacket(BANANA* client)
{
	switch (client->decryptbuf[0x02])
	{
	case 0x05:
		printf("客户端已离开港口.\n");
		client->todc = 1;
		break;
	case 0x10:
		Command10(0, client);
		break;
	case 0x1D:
		client->response = (unsigned)servertime;
		break;
	case 0x93:
	{
		unsigned ch, ch2, ipaddr;
		int banned = 0, match;

		client->temp_guildcard = *(unsigned*)&client->decryptbuf[0x0C];
		client->hwinfo = *(long long*)&client->decryptbuf[0x84];
		ipaddr = *(unsigned*)&client->ipaddr[0];

		for (ch = 0;ch < num_bans;ch++)
		{
			if ((ship_bandata[ch].guildcard == client->temp_guildcard) && (ship_bandata[ch].type == 1))
			{
				banned = 1;
				break;
			}
			if ((ship_bandata[ch].ipaddr == ipaddr) && (ship_bandata[ch].type == 2))
			{
				banned = 1;
				break;
			}
			if ((ship_bandata[ch].hwinfo == client->hwinfo) && (ship_bandata[ch].type == 3))
			{
				banned = 1;
				break;
			}
		}

		for (ch = 0;ch < num_masks;ch++)
		{
			match = 1;
			for (ch2 = 0;ch2 < 4;ch2++)
			{
				if ((ship_banmasks[ch][ch2] != 0x8000) &&
					((unsigned char)ship_banmasks[ch][ch2] != client->ipaddr[ch2]))
					match = 0;
			}
			if (match)
			{
				banned = 1;
				break;
			}
		}

		if (banned)
		{
			Send1A(L"You are banned from this ship.", client, 103);
			client->todc = 1;
		}
		else
			if (!client->sendCheck[RECEIVE_PACKET_93])
			{
				ShipSend0B(client, logon);
				client->sendCheck[RECEIVE_PACKET_93] = 0x01;
			}
	}
	break;
	default:
		break;
	}
}

//计算代码
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

//载入战斗参数代码
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

//载入任务代码
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
		for (ch = 0;ch < strlen(&qfile[0]);ch++)
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
		for (ch = 0;ch < 64;ch++)
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
		for (ch = 0;ch < 120;ch++)
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
			for (ch = 0;ch < strlen(&qfile2[0]);ch++)
				if ((qfile2[ch] == 10) || (qfile2[ch] == 13))
					qfile2[ch] = 0; // Reserved

			for (ch4 = 0;ch4 < numLanguages;ch4++)
			{
				memcpy(&qfile4[0], &qfile2[0], strlen(&qfile2[0]) + 1);

				// Add extension to .qst and .raw for languages

				extf = 0;

				if (strlen((char*)languageExts[ch4]))
				{
					if ((strlen(&qfile4[0]) - qf2l) > 3)
						for (ch5 = qf2l;ch5 < strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5 + 1]) == 113) &&
								(tolower(qfile4[ch5 + 2]) == 115) &&
								(tolower(qfile4[ch5 + 3]) == 116))
							{
								qfile4[ch5] = 0;
								strcat(&qfile4[ch5], "_");
								strcat(&qfile4[ch5], (char*)languageExts[ch4]);
								strcat(&qfile4[ch5], ".qst");
								extf = 1;
								break;
							}
						}

					if (((strlen(&qfile4[0]) - qf2l) > 3) && (!extf))
						for (ch5 = qf2l;ch5 < strlen(&qfile4[0]) - 3;ch5++)
						{
							if ((qfile4[ch5] == 46) &&
								(tolower(qfile4[ch5 + 1]) == 114) &&
								(tolower(qfile4[ch5 + 2]) == 97) &&
								(tolower(qfile4[ch5 + 3]) == 119))
							{
								qfile4[ch5] = 0;
								strcat(&qfile4[ch5], "_");
								strcat(&qfile4[ch5], (char*)languageExts[ch4]);
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
					for (ch2 = 0x18;ch2 < 0x48;ch2 += 2)
					{
						if (*(unsigned short*)&qpdc_buffer[ch2] != 0x0000)
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
								*(unsigned*)&qpd_buffer[qb_ofs] = *(unsigned*)&ed[2];
								qb_ofs += 4;
								//printf ("area: %u, object count: %u\n", ed[2], num_records);
								*(unsigned*)&qpd_buffer[qb_ofs] = num_records;
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
								*(unsigned*)&dp[qm_ofs] = *(unsigned*)&ed[2];
								//printf ("area: %u, mid count: %u\n", ed[2], num_records);
								if (ed[2] > 17)
								{
									printf("任务区域超出范围!\n");
									printf("按下 [回车键] 退出");
									gets_s(&dp[0], 0);
									exit(1);
								}
								qm_ofs += 4;
								*(unsigned*)&dp[qm_ofs] = num_records;
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
						for (ch = 0;ch < 18;ch++)
						{
							ch2 = 0;
							while (ch2 < qb_ofs)
							{
								unsigned qa;

								qa = *(unsigned*)&qpd_buffer[ch2];
								num_records = *(unsigned*)&qpd_buffer[ch2 + 4];
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
						*(unsigned*)q->mapdata = qm_ofs;
						// Need to sort first...
						ch3 = 4;
						for (ch = 0;ch < 18;ch++)
						{
							ch2 = 0;
							while (ch2 < (qm_ofs - 4))
							{
								unsigned qa;

								qa = *(unsigned*)&dp[ch2];
								num_records = *(unsigned*)&dp[ch2 + 4];
								if (qa == ch)
								{
									memcpy(&q->mapdata[ch3], &dp[ch2], (num_records * 72) + 8);
									ch3 += (num_records * 72) + 8;
								}
								ch2 += (num_records * 72) + 8;
							}
						}
						for (ch = 0;ch < num_objects;ch++)
						{
							// Swap fields in advance
							dp[0] = q->objectdata[(ch * 68) + 0x37];
							dp[1] = q->objectdata[(ch * 68) + 0x36];
							dp[2] = q->objectdata[(ch * 68) + 0x35];
							dp[3] = q->objectdata[(ch * 68) + 0x34];
							*(unsigned*)&q->objectdata[(ch * 68) + 0x34] = *(unsigned*)&dp[0];
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
//释放CSV数据占用的内存
void FreeCSV()
{
	unsigned ch, ch2;

	for (ch = 0;ch < csv_lines;ch++)
	{
		for (ch2 = 0;ch2 < 64;ch2++)
			if (csv_params[ch][ch2] != NULL) free(csv_params[ch][ch2]);
	}
	csv_lines = 0;
	memset(&csv_params, 0, sizeof(csv_params));
}

// Load CSV into RAM
//载入CSV数据至内存
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
		for (ch = 0;ch < strlen(&csv_data[0]);ch++)
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

//载入盔甲参数
void LoadArmorParam()
{
	unsigned ch, wi1;

	LoadCSV("param\\armorpmt.ini");
	/*这是该文件的参考格式
	0x010100, Frame, 640, -1, -1, 0, 5, 5, 0, 0, 255, 0, 5, 0, 0, 5, 0, 2, 2, 0, 0, 0
	0x010101, Armor, 641, -1, -1, 0, 7, 7, 0, 0, 251, 3, 0, 0, 5, 0, 0, 2, 2, 0, 0, 0
	*/
	for (ch = 0;ch < csv_lines;ch++)
	{
		wi1 = hexToByte(&csv_params[ch][0][6]);
		armor_dfpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][17]);
		armor_evpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][18]);
		armor_equip_table[wi1] = (unsigned char)atoi(csv_params[ch][10]);
		armor_level_table[wi1] = (unsigned char)atoi(csv_params[ch][11]);
		// 属性テーブル作成
		armor_eic_table[wi1] = (unsigned char)atoi(csv_params[ch][14]);
		//printf ("armor index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, armor_dfpvar_table[wi1], armor_evpvar_table[wi1], armor_equip_table[wi1], armor_level_table[wi1]);
	}
	FreeCSV();
	LoadCSV("param\\shieldpmt.ini");
	for (ch = 0;ch < csv_lines;ch++)
	{
		wi1 = hexToByte(&csv_params[ch][0][6]);
		barrier_dfpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][17]);
		barrier_evpvar_table[wi1] = (unsigned char)atoi(csv_params[ch][18]);
		barrier_equip_table[wi1] = (unsigned char)atoi(csv_params[ch][10]);
		barrier_level_table[wi1] = (unsigned char)atoi(csv_params[ch][11]);
		// 属性テーブル作成
		barrier_eic_table[wi1] = (unsigned char)atoi(csv_params[ch][14]);
		//printf ("barrier index %02x, dfp: %u, evp: %u, eq: %u, lv: %u \n", wi1, barrier_dfpvar_table[wi1], barrier_evpvar_table[wi1], barrier_equip_table[wi1], barrier_level_table[wi1]);
	}
	FreeCSV();
	// Set up the stack table too.
	for (ch = 0;ch < 0x09;ch++)
	{
		if (ch != 0x02)
			stackable_table[ch] = 10;
	}
	stackable_table[0x10] = 99;

}

//载入武器参数
void LoadWeaponParam()
{
	unsigned ch, wi1, wi2;

	LoadCSV("param\\weaponpmt.ini");
	/*这是文件参考参数
	0x000000, Saber, 177, -1, -1, 0, 255, 5, 7, 0, 0, 60, 0, 0, 0, -1, 0, 10, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0
	0x000100, Saber, 177, 0, 0, 0, 255, 40, 55, 30, 0, 0, 0, 0, 35, 0, 0, 30, 0, 0, 1, 2, -1, -1, 1, 1, -1, -1, 0, 0, 0, 0
	*/
	for (ch = 0;ch < csv_lines;ch++)
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

//载入魔法科技参数
void LoadTechParam()
{
	unsigned ch, ch2;

	LoadCSV("param\\tech.ini");
	/*用于参考
	Foie,15,20,0,15,0,0,30,30,30,0,30,20
	*/
	if (csv_lines != 19)
	{
		printf("科技 tech.ini 文件CSV内容已损坏.\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	for (ch = 0;ch < 19;ch++) // For technique
	{
		for (ch2 = 0;ch2 < 12;ch2++) // For class
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

//载入商店数据2
void LoadShopData2()
{
	FILE* fp;
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

//舰船服务器参数回调
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == MYWM_NOTIFYICON)
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			switch (wParam)
			{
			case 100:
				if (program_hidden)
				{
					program_hidden = 0;
					reveal_window;
				}
				else
				{
					program_hidden = 1;
					ShowWindow(consoleHwnd, SW_HIDE);
				}
				return TRUE;
				break;
			}
			break;
		}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//舰船服务器主体
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
	int wserror1;
	int wserror2;
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

	wcscat((wchar_t*)&dp[0], L"Tethealla 舰船服务器 版本 ");
	wcscat((wchar_t*)&dp[0], SERVER_VERSION);
	wcscat((wchar_t*)&dp[0], L" 作者 Sodaboy 编译 Sancaros");
	wcscat((wchar_t*)&dp[0], L" 当前舰船: ");
	wcscat((wchar_t*)&dp[0], SHIP_NAME);
	SetConsoleTitle((wchar_t*)&dp[0]);
	/*
	strcat(&dp[0], "Tethealla 舰船服务器 版本 ");
	strcat(&dp[0], SERVER_VERSION);
	strcat(&dp[0], " 作者 Sodaboy 编译 Sancaros");
	strcat(&dp[0], " 当前舰船: ");
	strcat(&dp[0], SHIP_NAME);
	SetConsoleTitle(&dp[0]);*/

	wprintf(L"\n特提塞拉 舰船服务器 版本 %Ls  版权作者 (C) 2008  Terry Chatman Jr.\n", SERVER_VERSION);
	printf("\n编译 Sancaros. 2020.12\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf("这个程序绝对没有保证: 详情参见说明\n");
	printf("请参阅GPL-3.0.TXT中的第15节\n");
	printf("这是免费软件,欢迎您重新发布\n");
	printf("在某些条件下,详见GPL-3.0.TXT.\n");
	printf("\n\n");
	char* localLanguage = setlocale(LC_ALL, "");
	if (localLanguage == NULL)
	{
		printf("获取本地语言类型失败\n");
	}
	setlocale(LC_CTYPE, localLanguage);
	if (setlocale(LC_CTYPE, "") == NULL)
	{ /*设置为本地环境变量定义的locale*/
		fprintf(stderr, "无法设置本地语言\n 已改为UT8编码语言");
		setlocale(LC_CTYPE, "utf8");
	}
	printf("本地语言类型为 %s\n", localLanguage);
	/*
	for (ch=0;ch<5;ch++)
	{
	printf (".");
	Sleep (200);
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
	printf("设置文件载入确认!\n\n");

	printf("准备载入语言文件...\n");

	load_language_file();

	printf("语言文件确认!\n\n");

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

	for (ch = 1;ch < 200;ch++)
		tnlxp[ch] = tnlxp[ch - 1] + tnlxp[ch];

	printf("加载战斗参数文件...\n\n");
	LoadBattleParam(&ep1battle_off[0], "param\\BattleParamEntry.dat", 374, 0x8fef1ffe);
	LoadBattleParam(&ep1battle[0], "param\\BattleParamEntry_on.dat", 374, 0xb8a2d950);
	LoadBattleParam(&ep2battle_off[0], "param\\BattleParamEntry_lab.dat", 374, 0x3dc217f5);
	LoadBattleParam(&ep2battle[0], "param\\BattleParamEntry_lab_on.dat", 374, 0x4d4059cf);
	LoadBattleParam(&ep4battle_off[0], "param\\BattleParamEntry_ep4.dat", 332, 0x50841167);
	LoadBattleParam(&ep4battle[0], "param\\BattleParamEntry_ep4_on.dat", 332, 0x42bf9716);
	/*
	for (ch = 0;ch < 374;ch++)
		if (ep2battle_off[ch].HP)
		{
			ep2battle_off[ch].XP = (ep2battle_off[ch].XP * 130) / 100; // 30% boost to EXP
			ep2battle[ch].XP = (ep2battle[ch].XP * 130) / 100;
		}*/
	for (ch = 0; ch < 374; ch++) {
		if (ep1battle_off[ch].HP)
		{
			ep1battle_off[ch].EVP = (ep1battle_off[ch].EVP * 120) / 100; // 20% 几率提升 EVP
			ep1battle[ch].EVP = (ep1battle[ch].EVP * 120) / 100;
			ep1battle_off[ch].LCK = (ep1battle_off[ch].LCK * 150) / 100; // 50% 几率提升 LCK
			ep1battle[ch].LCK = (ep1battle[ch].LCK * 150) / 100;
		}
		if (ep2battle_off[ch].HP)
		{
			ep2battle_off[ch].XP = (ep2battle_off[ch].XP * 140) / 100; // 40% 几率提升 EXP
			ep2battle[ch].XP = (ep2battle[ch].XP * 140) / 100;
			ep2battle_off[ch].EVP = (ep2battle_off[ch].EVP * 120) / 100; // 20% 几率提升 EVP
			ep2battle[ch].EVP = (ep2battle[ch].EVP * 120) / 100;
			ep2battle_off[ch].LCK = (ep2battle_off[ch].LCK * 150) / 100; // 50% 几率提升 LCK
			ep2battle[ch].LCK = (ep2battle[ch].LCK * 150) / 100;
		}
	}

	for (ch = 0; ch < 332; ch++) {
		if (ep4battle_off[ch].HP)
		{
			ep4battle_off[ch].XP = (ep4battle_off[ch].XP * 180) / 100; // 80% 几率提升 EXP
			ep4battle[ch].XP = (ep4battle[ch].XP * 180) / 100;
			ep4battle_off[ch].EVP = (ep4battle_off[ch].EVP * 120) / 100; // 20% 几率提升 EVP
			ep4battle[ch].EVP = (ep4battle[ch].EVP * 120) / 100;
			ep4battle_off[ch].LCK = (ep4battle_off[ch].LCK * 150) / 100; // 50% 几率提升 LCK
			ep4battle[ch].LCK = (ep4battle[ch].LCK * 150) / 100;
		}
	}
	printf("\n.. 完成!\n\n建立通用表... \n\n");
	printf("武器掉率: %03f%%\n", (float)WEAPON_DROP_RATE / 1000);
	printf("盔甲掉率: %03f%%\n", (float)ARMOR_DROP_RATE / 1000);
	printf("玛古掉率: %03f%%\n", (float)MAG_DROP_RATE / 1000);
	printf("工具掉率: %03f%%\n", (float)TOOL_DROP_RATE / 1000);
	printf("美赛塔掉率: %03f%%\n", (float)MESETA_DROP_RATE / 1000);
	printf("经验倍率: %u%%\n\n", EXPERIENCE_RATE * 100);

	printf("\n稀有掉率...\n");
	printf("稀有怪物掉率: %d\n", rare_mob_mult);
	printf("稀有箱子掉率: %d\n", rare_box_mult);
	printf("全船稀有掉率: %d\n\n", global_rare_mult);

	ch = 0;
	while (ch < 100000)
	{
		for (ch2 = 0;ch2 < 5;ch2++)
		{
			common_counters[ch2]++;
			if ((common_counters[ch2] >= common_rates[ch2]) && (ch < 100000))
			{
				common_table[ch++] = (unsigned char)ch2;
				common_counters[ch2] = 0;
			}
		}
	}

	printf(".. 完成!\n\n");

	printf("正在加载物品参数 param\\ItemPT.gsl...\n");
	fp = fopen("param\\ItemPT.gsl", "rb");
	if (!fp)
	{
		printf("缺少 ItemPT.gsl 文件\n");
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}
	fseek(fp, 0x3000, SEEK_SET);
	//EP1 2不变 EP4尝试用单独掉落 EP1 2的挑战模式采用EP1 2一样的试试看
	// 载入 EP1 ItemPT数据
	printf("正在解析 章节 I ItemPT数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2 < 4;ch2++) // For each difficulty
	{
		for (ch = 0;ch < 10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);

			ptd.enemy_dar[44] = 100; // Dragon
			ptd.enemy_dar[45] = 100; // De Rol Le
			ptd.enemy_dar[46] = 100; // Vol Opt
			ptd.enemy_dar[47] = 100; // Falz

			for (ch3 = 0;ch3 < 10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}

			for (ch3 = 0;ch3 < 0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}

			ptd.enemy_meseta[47][0] = ptd.enemy_meseta[46][0] + 400 + (100 * ch2); // Give Falz some meseta
			ptd.enemy_meseta[47][1] = ptd.enemy_meseta[46][1] + 400 + (100 * ch2);

			for (ch3 = 0;ch3 < 23;ch3++)
			{
				for (ch4 = 0;ch4 < 6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}

			for (ch3 = 0;ch3 < 28;ch3++)
			{
				for (ch4 = 0;ch4 < 10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}

			memcpy(&pt_tables_ep1[ch][ch2], &ptd, sizeof(PTDATA));

			// Set up the weapon drop table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3 < 4096))
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
				for (ch4 = 0;ch4 < 5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
					{
						slots_ep1[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5 = 0;ch5 < 4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							power_patterns_ep1[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5 = 0;ch5 < 6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							percent_patterns_ep1[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							tool_drops_ep1[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							attachment_ep1[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3 < 4096))
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

	// 载入 EP2 ItemPT数据
	printf("正在解析 章节 II ItemPT数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2 < 4;ch2++) // For each difficulty
	{
		for (ch = 0;ch < 10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);

			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow

			for (ch3 = 0;ch3 < 10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}

			for (ch3 = 0;ch3 < 0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}

			ptd.enemy_meseta[78][0] = ptd.enemy_meseta[77][0] + 400 + (100 * ch2); // Give Flow some meseta
			ptd.enemy_meseta[78][1] = ptd.enemy_meseta[77][1] + 400 + (100 * ch2);

			for (ch3 = 0;ch3 < 23;ch3++)
			{
				for (ch4 = 0;ch4 < 6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}

			for (ch3 = 0;ch3 < 28;ch3++)
			{
				for (ch4 = 0;ch4 < 10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}

			memcpy(&pt_tables_ep2[ch][ch2], &ptd, sizeof(PTDATA));

			// Set up the weapon drop table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3 < 4096))
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
				for (ch4 = 0;ch4 < 5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
					{
						slots_ep2[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}

			// Set up the power patterns

			for (ch5 = 0;ch5 < 4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							power_patterns_ep2[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the percent patterns

			for (ch5 = 0;ch5 < 6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							percent_patterns_ep2[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}

			// Set up the tool table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							tool_drops_ep2[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the attachment table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							attachment_ep2[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}


			// Set up the technique table

			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3 < 4096))
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
	
	// 载入 EP4 ItemPT数据
	printf("正在解析 章节 IV ItemPT数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2 < 4;ch2++) // For each difficulty
	{
		for (ch = 0;ch < 10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);

			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow

			for (ch3 = 0;ch3 < 10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}

			for (ch3 = 0;ch3 < 0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}
			for (ch3 = 0;ch3 < 23;ch3++)
			{
				for (ch4 = 0;ch4 < 6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}
			for (ch3 = 0;ch3 < 28;ch3++)
			{
				for (ch4 = 0;ch4 < 10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}
			memcpy(&pt_tables_ep4[ch][ch2], &ptd, sizeof(PTDATA));
			// Set up the weapon drop table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3 < 4096))
						{
							wep_rank = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if (wep_rank >= 0)
							{
								weapon_drops_ep4[ch][ch2][ch5][ch3++] = (ch4 + 1) + ((unsigned char)wep_rank << 8);
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
				for (ch4 = 0;ch4 < 5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
					{
						slots_ep4[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}
			// Set up the power patterns
			for (ch5 = 0;ch5 < 4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							power_patterns_ep4[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the percent patterns
			for (ch5 = 0;ch5 < 6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							percent_patterns_ep4[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the tool table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							tool_drops_ep4[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the attachment table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							attachment_ep4[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the technique table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3 < 4096))
							{
								tech_drops_ep4[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}
	
	// 载入 EP1 挑战 ItemPT数据
	printf("正在解析 章节 I 挑战 ItemPT数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2 < 4;ch2++) // For each difficulty
	{
		for (ch = 0;ch < 10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);
			ptd.enemy_dar[44] = 100; // Dragon
			ptd.enemy_dar[45] = 100; // De Rol Le
			ptd.enemy_dar[46] = 100; // Vol Opt
			ptd.enemy_dar[47] = 100; // Falz
			for (ch3 = 0;ch3 < 10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}
			for (ch3 = 0;ch3 < 0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}
			ptd.enemy_meseta[47][0] = ptd.enemy_meseta[46][0] + 400 + (100 * ch2); // Give Flow some meseta
			ptd.enemy_meseta[47][1] = ptd.enemy_meseta[46][1] + 400 + (100 * ch2);
			for (ch3 = 0;ch3 < 23;ch3++)
			{
				for (ch4 = 0;ch4 < 6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}
			for (ch3 = 0;ch3 < 28;ch3++)
			{
				for (ch4 = 0;ch4 < 10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}
			memcpy(&pt_tables_ep1c[ch][ch2], &ptd, sizeof(PTDATA));
			// Set up the weapon drop table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3 < 4096))
						{
							wep_rank = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if (wep_rank >= 0)
							{
								weapon_drops_ep1c[ch][ch2][ch5][ch3++] = (ch4 + 1) + ((unsigned char)wep_rank << 8);
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
				for (ch4 = 0;ch4 < 5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
					{
						slots_ep1c[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}
			// Set up the power patterns
			for (ch5 = 0;ch5 < 4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							power_patterns_ep1c[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the percent patterns
			for (ch5 = 0;ch5 < 6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							percent_patterns_ep1c[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the tool table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							tool_drops_ep1c[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the attachment table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							attachment_ep1c[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}
			// Set up the technique table
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3 < 4096))
							{
								tech_drops_ep1c[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}
	
	// 载入 EP2 挑战 ItemPT数据
	printf("正在解析 章节 II 挑战 ItemPT数据... (需要一会会哦...)\n");
	for (ch2 = 0;ch2 < 4;ch2++) // For each difficulty
	{
		for (ch = 0;ch < 10;ch++) // For each ID
		{
			fread(&ptd, 1, sizeof(PTDATA), fp);
			//这里没完成
			ptd.enemy_dar[73] = 100; // Barba Ray
			ptd.enemy_dar[76] = 100; // Gol Dragon
			ptd.enemy_dar[77] = 100; // Gar Gryphon
			ptd.enemy_dar[78] = 100; // Olga Flow
			for (ch3 = 0;ch3 < 10;ch3++)
			{
				ptd.box_meseta[ch3][0] = swapendian(ptd.box_meseta[ch3][0]);
				ptd.box_meseta[ch3][1] = swapendian(ptd.box_meseta[ch3][1]);
			}
			for (ch3 = 0;ch3 < 0x64;ch3++)
			{
				ptd.enemy_meseta[ch3][0] = swapendian(ptd.enemy_meseta[ch3][0]);
				ptd.enemy_meseta[ch3][1] = swapendian(ptd.enemy_meseta[ch3][1]);
			}
			ptd.enemy_meseta[78][0] = ptd.enemy_meseta[77][0] + 400 + (100 * ch2); // Give Flow some meseta
			ptd.enemy_meseta[78][1] = ptd.enemy_meseta[77][1] + 400 + (100 * ch2);
			for (ch3 = 0;ch3 < 23;ch3++)
			{
				for (ch4 = 0;ch4 < 6;ch4++)
					ptd.percent_pattern[ch3][ch4] = swapendian(ptd.percent_pattern[ch3][ch4]);
			}
			for (ch3 = 0;ch3 < 28;ch3++)
			{
				for (ch4 = 0;ch4 < 10;ch4++)
				{
					if (ch3 == 23)
						ptd.tool_frequency[ch3][ch4] = 0;
					else
						ptd.tool_frequency[ch3][ch4] = swapendian(ptd.tool_frequency[ch3][ch4]);
				}
			}
			memcpy(&pt_tables_ep2c[ch][ch2], &ptd, sizeof(PTDATA));
			// 设置武器掉落表
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 12;ch4++)
					{
						wep_counters[ch4] += ptd.weapon_ratio[ch4];
						if ((wep_counters[ch4] >= 0xFF) && (ch3 < 4096))
						{
							wep_rank = ptd.weapon_minrank[ch4];
							wep_rank += ptd.area_pattern[ch5];
							if (wep_rank >= 0)
							{
								weapon_drops_ep2c[ch][ch2][ch5][ch3++] = (ch4 + 1) + ((unsigned char)wep_rank << 8);
								wep_counters[ch4] = 0;
							}
						}
					}
				}
			}
			// 设置插槽位表
			memset(&wep_counters[0], 0, 4 * 24);
			ch3 = 0;
			while (ch3 < 4096)
			{
				for (ch4 = 0;ch4 < 5;ch4++)
				{
					wep_counters[ch4] += ptd.slot_ranking[ch4];
					if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
					{
						slots_ep2c[ch][ch2][ch3++] = ch4;
						wep_counters[ch4] = 0;
					}
				}
			}
			// 设置强度百分比
			for (ch5 = 0;ch5 < 4;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 9;ch4++)
					{
						wep_counters[ch4] += ptd.power_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							power_patterns_ep2c[ch][ch2][ch5][ch3++] = ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// 设置属性百分比
			for (ch5 = 0;ch5 < 6;ch5++)
			{
				memset(&wep_counters[0], 0, 4 * 24);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 23;ch4++)
					{
						wep_counters[ch4] += ptd.percent_pattern[ch4][ch5];
						if ((wep_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							percent_patterns_ep2c[ch][ch2][ch5][ch3++] = (char)ch4;
							wep_counters[ch4] = 0;
						}
					}
				}
			}
			// 设置工具表
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tool_counters[0], 0, 4 * 28);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 28;ch4++)
					{
						tool_counters[ch4] += ptd.tool_frequency[ch4][ch5];
						if ((tool_counters[ch4] >= 0x2710) && (ch3 < 4096))
						{
							tool_drops_ep2c[ch][ch2][ch5][ch3++] = ch4;
							tool_counters[ch4] = 0;
						}
					}
				}
			}
			// 设置插件表
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 6;ch4++)
					{
						tech_counters[ch4] += ptd.percent_attachment[ch4][ch5];
						if ((tech_counters[ch4] >= 0x64) && (ch3 < 4096))
						{
							attachment_ep2c[ch][ch2][ch5][ch3++] = ch4;
							tech_counters[ch4] = 0;
						}
					}
				}
			}
			// 设置魔法科技表
			for (ch5 = 0;ch5 < 10;ch5++)
			{
				memset(&tech_counters[0], 0, 4 * 19);
				ch3 = 0;
				while (ch3 < 4096)
				{
					for (ch4 = 0;ch4 < 19;ch4++)
					{
						if (ptd.tech_levels[ch4][ch5 * 2] >= 0)
						{
							tech_counters[ch4] += ptd.tech_frequency[ch4][ch5];
							if ((tech_counters[ch4] >= 0xFF) && (ch3 < 4096))
							{
								tech_drops_ep2c[ch][ch2][ch5][ch3++] = ch4;
								tech_counters[ch4] = 0;
							}
						}
					}
				}
			}
		}
	}
	//! Structures inside the ItemPT.gsl archive .rel files.
	//!
	//! ItemPT.gsl contains:
	//!
	//! ItemPT{episode}{challenge}{difficulty}.rel
	//! episode = ["" => 1, "l" => 2, "e" => 4]
	//! challenge = ["c" => true, "" => false]
	//! difficulty = ["n", "h", "v", "u"]
	//!
	//! Each file has a `ProbTable` entry for one Section ID in the following
	//! order:
	//!
	//! * Viridia (0)
	//! * Greenill (1)
	//! * Skyly (2)
	//! * Bluefull (3)
	//! * Purplenum (4)
	//! * Pinkal (5)
	//! * Redria (6)
	//! * Oran (7)
	//! * Yellowboze (8)
	//! * Whitill (9)
	//!
	//! Note that episode 4 data was never actually procured, so specially doctored
	//! ItemPT.gsl files have hand-made Ep4 data. Sometimes they don't have proper
	//! headers, so we have to make assumptions about where they are in the file.
	//!
	//! V3 (GC, BB, Xbox?) probability tables are Big Endian even on BB.
	fclose(fp);
	printf("\n.. 完成!\n\n");
	printf("正在加载等级参数 PlyLevelTbl.bin ... ");
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
	//LoadQuests("quest\\challenge.ini", 10);
	LoadQuests("quest\\ep1challenge.ini", 10); //尝试修改任务载入模式 sancaros
	LoadQuests("quest\\ep2challenge.ini", 11);

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

	for (ch = 0;ch < 200;ch++)
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
	for (ch = 0;ch < serverBlocks;ch++)
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
	for (ch = 0;ch < serverMaxConnections;ch++)
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
	wc.lpszClassName = L"Sancaros";
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wc))
	{
		printf("注册 Class 文件失败 .\n");
		exit(1);
	}

	hwndWindow = CreateWindow(L"Sancaros", L"hidden window", WS_MINIMIZE, 1, 1, 1, 1,
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
	wcscat(&nid.szTip[0], L"Tethealla 舰船服务器 ");
	wcscat(&nid.szTip[0], (wchar_t*)SERVER_VERSION);
	wcscat(&nid.szTip[0], L" - 双击以显示/隐藏");
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

		for (ch = 0;ch < serverNumConnections;ch++)
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


		// Read from logon server (if connected) 从登录服务器读取（如果已连接） 

		if (logon->sockfd >= 0)
		{
			if ((unsigned)servertime - logon->last_ping > 60)
			{
				printf("登录服务器ping超时.  正在尝试在 %u 秒内重新连接...\n", LOGIN_RECONNECT_SECONDS);
				initialize_logon();
			}
			else
			{
				// If there is packet data
				// (not sure what function loads data from the client into the logon struct)
				if (logon->packetdata)
				{
					// I think this checks if there are any unread packets
					// No, i think it loads the packet needed to be read
					// NO - its loading the size of the packet
					logon_this_packet = *(unsigned*)&logon->packet[logon->packetread];
					// Here is where it actually loads the packet (copies into a buffer)
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
				printf("正在重新连接到登录服务器...\n"); //sancaros
				reconnect_logon();
			}
		}


		// Listen for block connections

		for (ch = 0;ch < serverBlocks;ch++)
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
					if ((workConnect->plySockfd = tcp_accept(ship_sockfd, (struct sockaddr*)&listen_in, &listen_length)) > 0)
					{
						if (!blockConnections)
						{
							workConnect->connection_index = ch;
							serverConnectionList[serverNumConnections++] = ch;
							memcpy(&workConnect->IP_Address[0], inet_ntoa(listen_in.sin_addr), 16);
							*(unsigned*)&workConnect->ipaddr = *(unsigned*)&listen_in.sin_addr;
							printf("舰舱收到来自 %s:%u 的登船请求\n", workConnect->IP_Address, listen_in.sin_port);
							printf("玩家统计: %u\n", serverNumConnections); //新增玩家统计文件 测试用

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
						//WriteLog("debug客户端断点13");
					}
				}
			}

			for (ch = 0;ch < serverBlocks;ch++)
			{
				if (FD_ISSET(block_sockfd[ch], &ReadFDs))
				{
					// Someone's attempting to connect to the block server.
					ch2 = free_connection();
					if (ch2 != 0xFFFF)
					{
						listen_length = sizeof(listen_in);
						workConnect = connections[ch2];
						if ((workConnect->plySockfd = tcp_accept(block_sockfd[ch], (struct sockaddr*)&listen_in, &listen_length)) > 0)
						{
							if (!blockConnections)
							{
								workConnect->connection_index = ch2;
								serverConnectionList[serverNumConnections++] = ch2;
								memcpy(&workConnect->IP_Address[0], inet_ntoa(listen_in.sin_addr), 16);
								printf("舰舱已接收来自 %s:%u 的登船请求\n", inet_ntoa(listen_in.sin_addr), listen_in.sin_port);
								*(unsigned*)&workConnect->ipaddr = *(unsigned*)&listen_in.sin_addr;
								printf("玩家统计: %u\n", serverNumConnections);
								ShipSend0E(logon);
								start_encryption(workConnect);
								/* Doin' block process... */
								workConnect->block = ch + 1;
								blocks[workConnect->block - 1]->count++;
							}
							else
								initialize_connection(workConnect);
							//WriteLog("debug客户端断点14");
						}
					}
				}
			}


			// Process client connections 输出客户端连接获取数据

			for (ch = 0;ch < serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				workConnect = connections[connectNum];

				if (workConnect->plySockfd >= 0)
				{
					if (FD_ISSET(workConnect->plySockfd, &WriteFDs))
					{
						// Write shit.写得就像屎一样

						bytes_sent = send(workConnect->plySockfd, &workConnect->sndbuf[workConnect->sndwritten],
							workConnect->snddata - workConnect->sndwritten, 0);

						if (bytes_sent == SOCKET_ERROR)
						{/*
							wserror1 = WSAGetLastError();
							wserror2 = GetLastError();
							printf("\n无法发送数据至客户端...\n");
							printf("\n套接字错误信息如下 %u.\n", wserror1);
							printf("\n套接字错误信息如下 %u.\n", wserror2);
							*/
							initialize_connection(workConnect);
							//WriteLog("debug客户端断点15");
						}
						else
						{
							workConnect->toBytesSec += bytes_sent;
							workConnect->sndwritten += bytes_sent;
						}

						if (workConnect->sndwritten == workConnect->snddata)
							workConnect->sndwritten = workConnect->snddata = 0;
					}

					// Disconnect those violators of the law...切断那些违法者的联系 也就是说 数据为空 违规了

					if (workConnect->todc)
						initialize_connection(workConnect);
					//WriteLog("debug客户端断点16");

					if (FD_ISSET(workConnect->plySockfd, &ReadFDs))
					{
						pkt_len = recv(workConnect->plySockfd, &tmprcv[0], TCP_BUFFER_SIZE - 1, 0);
						// Read shit.
						if (pkt_len <= 0)
						{/*
							wserror1 = WSAGetLastError();
							wserror2 = GetLastError();
							printf("\n无法从客户端读取数据...\n");
							printf("\n套接字错误信息如下 %u.\n", wserror1);
							printf("\n套接字错误信息如下 %u.\n", wserror2);
							*/
							initialize_connection(workConnect);
							//WriteLog("debug客户端断点17");
						}
						else
						{
							workConnect->fromBytesSec += (unsigned)pkt_len;
							// Work with it.

							for (pkt_c = 0;pkt_c < pkt_len;pkt_c++)
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
										//WriteLog("debug客户端断点18");
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
										(workConnect->toBytesSec > 150000)
										)
									{
										printf("%u 由于可能的DDOS攻击而断开连接. (p/s: %u, tb/s: %u, fb/s: %u)\n", workConnect->guildcard, workConnect->packetsSec, workConnect->toBytesSec, workConnect->fromBytesSec);
										initialize_connection(workConnect);
										//WriteLog("debug客户端断点19");
										break;
									}
									else
									{
										switch (workConnect->block)
										{
										case 0x00:
											// Ship Server 舰船服务器
											ShipProcessPacket(workConnect);
											break;
										default:
											// Block server 舰仓服务器
											BlockProcessPacket(workConnect);
											break;
										}
									}
									workConnect->rcvread = 0;
								}
							}
						}
					}

					if (FD_ISSET(workConnect->plySockfd, &ExceptFDs)) // Exception?这里指的是发生意外如何处理
						initialize_connection(workConnect);
					//WriteLog("debug客户端断点20");

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
						wserror1 = WSAGetLastError();
						wserror2 = GetLastError();
						printf("\n无法将数据发送到登录服务器...\n");
						printf("\n套接字错误信息如下 %u.\n", wserror1);
						printf("\n套接字错误信息如下 %u.\n", wserror2);
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
						wserror1 = WSAGetLastError();
						wserror2 = GetLastError();
						printf("\n无法读取来自登录服务器的数据...\n");
						printf("\n套接字错误信息如下 %u.\n", wserror1);
						printf("\n套接字错误信息如下 %u.\n", wserror2);
						initialize_logon();
						printf("与登录服务器的连接中断...\n");
						printf("将在 %u 秒后重连...\n", LOGIN_RECONNECT_SECONDS);
					}
					else
					{
						// Work with it.
						for (pkt_c = 0;pkt_c < pkt_len;pkt_c++)
						{
							logon->rcvbuf[logon->rcvread++] = tmprcv[pkt_c];

							if (logon->rcvread == 4)
							{
								/* Read out how much data we're expecting this packet. */
								logon->expect = *(unsigned*)&logon->rcvbuf[0];

								if (logon->expect > TCP_BUFFER_SIZE)
								{
									printf("从登录服务器接收太多数据.\n正在断开连接,并将在 %u 秒后重新连接...\n", LOGIN_RECONNECT_SECONDS);
									initialize_logon();
								}
							}

							if ((logon->rcvread == logon->expect) && (logon->expect != 0))
							{
								decompressShipPacket(logon, &logon->decryptbuf[0], &logon->rcvbuf[0]);

								logon->expect = *(unsigned*)&logon->decryptbuf[0];

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

int tcp_accept(int sockfd, struct sockaddr* client_addr, int* addr_len)
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
	memset((void*)&sa, 0, sizeof(sa));

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error 如果发生sockket错误*/
	if (fd < 0)
		debug_perror("无法创建端口");
	else
	{

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr(dest_addr);
		sa.sin_port = htons((unsigned short)port);

		if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
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
	memset((void*)&sa, 0, sizeof(sa));

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Error */
	if (fd < 0) {
		debug_perror("无法创建端口");
		debug_perror("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(1);
	}

	sa.sin_family = AF_INET;
	memcpy((void*)&sa.sin_addr, (void*)&ip, sizeof(struct in_addr));
	sa.sin_port = htons((unsigned short)port);

	/* Reuse port */

	rcSockopt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&turn_on_option_flag, sizeof(turn_on_option_flag));

	/* bind() the socket to the interface */
	if (bind(fd, (struct sockaddr*)&sa, sizeof(struct sockaddr)) < 0) {
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
void debug_perror(char* msg) {
	debug("%s : %s\n", msg, strerror(errno));
}
/*****************************************************************************/
void debug(char* fmt, ...)
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

static void pso_crypt_init_key_bb(unsigned char* data)
{
	unsigned x;
	for (x = 0; x < 48; x += 3)
	{
		data[x] ^= 0x19;
		data[x + 1] ^= 0x16;
		data[x + 2] ^= 0x18;
	}
}


void pso_crypt_decrypt_bb(PSO_CRYPT* pcry, unsigned char* data, unsigned
	length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long*)&data[edx];
		ebx = ebx ^ pcry->tbl[5];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[4];
		ebp ^= *(unsigned long*)&data[edx + 4];
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
		*(unsigned long*)&data[edx] = ebp;
		*(unsigned long*)&data[edx + 4] = ebx;
		edx = edx + 8;
	}
}


void pso_crypt_encrypt_bb(PSO_CRYPT* pcry, unsigned char* data, unsigned
	length)
{
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

	edx = 0;
	ecx = 0;
	eax = 0;
	while (edx < length)
	{
		ebx = *(unsigned long*)&data[edx];
		ebx = ebx ^ pcry->tbl[0];
		ebp = ((pcry->tbl[(ebx >> 0x18) + 0x12] + pcry->tbl[((ebx >> 0x10) & 0xff) + 0x112])
			^ pcry->tbl[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->tbl[(ebx & 0xff) + 0x312];
		ebp = ebp ^ pcry->tbl[1];
		ebp ^= *(unsigned long*)&data[edx + 4];
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
		*(unsigned long*)&data[edx] = ebp;
		*(unsigned long*)&data[edx + 4] = ebx;
		edx = edx + 8;
	}
}

void encryptcopy(BANANA* client, const unsigned char* src, unsigned size)
{
	unsigned char* dest;

	// Bad pointer check...错误的指针检查
	if (((unsigned)client < (unsigned)connections[0]) ||
		((unsigned)client > (unsigned)connections[serverMaxConnections - 1]))
		return;

	// This is to avoid a TCP buffer overflow i think
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


void pso_crypt_table_init_bb(PSO_CRYPT* pcry, const unsigned char* salt)
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

	for (ecx = 0;ecx < 0x12;ecx++)
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

unsigned RleEncode(unsigned char* src, unsigned char* dest, unsigned src_size)
{
	unsigned char currChar, prevChar;             /* current and previous characters当前和以前的角色 */
	unsigned short count;                /* number of characters in a run运行中的角色数 */
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

void RleDecode(unsigned char* src, unsigned char* dest, unsigned src_size)
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

void prepare_key(unsigned char* keydata, unsigned len, struct rc4_key* key)
{
	unsigned index1, index2, counter;
	unsigned char* state;

	state = key->state;

	for (counter = 0; counter < 256; counter++)
		state[counter] = (unsigned char)counter;

	key->x = key->y = index1 = index2 = 0;

	for (counter = 0; counter < 256; counter++) {
		index2 = (keydata[index1] + state[counter] + index2) & 255;

		/* swap 转换 */
		state[counter] ^= state[index2];
		state[index2] ^= state[counter];
		state[counter] ^= state[index2];

		index1 = (index1 + 1) % len;
	}
}

/* 可逆加密, 将编码缓冲区更新密钥 */

void rc4(unsigned char* buffer, unsigned len, struct rc4_key* key)
{
	unsigned x, y, xorIndex, counter;
	unsigned char* state;

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
			*(unsigned*)dest = src_size;
			// Compress packet using RLE, storing at offset 0x08 of new packet. 使用RLE压缩数据包,在新数据包的偏移量0x08处存储.
			//
			// result = size of RLE compressed data + a DWORD for the original packet size.
			result = RleEncode(src, dest + 4, src_size) + 4;
			// Encrypt with RC4
			rc4(dest, result, &ship->sc_key);
			// Increase result by the size of a DWORD for the final ship packet size.
			result += 4;
			// Copy it to the front of the packet.把它复制到数据包的前面
			*(unsigned*)&ship->sndbuf[ship->snddata] = result;
			ship->snddata += (int)result;
		}
	}
}

//解压舰船数据包
void decompressShipPacket(ORANGE* ship, unsigned char* dest, unsigned char* src)
{
	unsigned src_size, dest_size;
	unsigned char* srccpy;

	if (ship->crypt_on)
	{
		src_size = *(unsigned*)src;
		src_size -= 8;
		src += 4;
		srccpy = src;
		// Decrypt RC4
		rc4(src, src_size + 4, &ship->cs_key);
		// The first four bytes of the src should now contain the expected uncompressed data size.
		dest_size = *(unsigned*)srccpy;
		// Increase expected size by 4 before inserting into the destination buffer.  (To take account for the packet
		// size DWORD...)
		dest_size += 4;
		*(unsigned*)dest = dest_size;
		// Decompress the data...
		RleDecode(srccpy + 4, dest + 4, src_size);
	}
	else
	{
		src_size = *(unsigned*)src;
		memcpy(dest + 4, src + 4, src_size);
		src_size += 4;
		*(unsigned*)dest = src_size;
	}
}
