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

#define reveal_window \
	ShowWindow ( consoleHwnd, SW_NORMAL ); \
	SetForegroundWindow ( consoleHwnd ); \
	SetFocus ( consoleHwnd )

#define swapendian(x) ( ( x & 0xFF ) << 8 ) + ( x >> 8 )
#define FLOAT_PRECISION 0.00001

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
*/

#define SERVER_VERSION "0.148"
#define DEBUG 1 //Nova更新的代码 用于debug
#define USEADDR_ANY
#define LOGON_PORT 3455 //Nova更新的代码 用于log
#define TCP_BUFFER_SIZE 64000
#define PACKET_BUFFER_SIZE ( TCP_BUFFER_SIZE * 16 )

//#define LOG_60 日志

#define SHIP_COMPILED_MAX_CONNECTIONS 900
#define SHIP_COMPILED_MAX_GAMES 75
#define LOGIN_RECONNECT_SECONDS 15
#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define MAX_SAVED_LOBBIES 20
#define MAX_SAVED_ITEMS 3000
#define MAX_GCSEND 2000
#define ALL_ARE_GM 0
#define PRS_BUFFER 262144

#define SEND_PACKET_03 0x00
#define RECEIVE_PACKET_93 0x0A
#define MAX_SENDCHECK 0x0B

// Our Character Classes 角色职业

#define CLASS_HUMAR 0x00
#define CLASS_HUNEWEARL 0x01
#define CLASS_HUCAST 0x02
#define CLASS_RAMAR 0x03
#define CLASS_RACAST 0x04
#define CLASS_RACASEAL 0x05
#define CLASS_FOMARL 0x06
#define CLASS_FONEWM 0x07
#define CLASS_FONEWEARL 0x08
#define CLASS_HUCASEAL 0x09
#define CLASS_FOMAR 0x0A
#define CLASS_RAMARL 0x0B
#define CLASS_MAX 0x0C

// Class equip_flags 装备分类

#define HUNTER_FLAG	1   // Bit 1 人类
#define RANGER_FLAG	2   // Bit 2 
#define FORCE_FLAG	4   // Bit 3
#define HUMAN_FLAG	8   // Bit 4
#define	DROID_FLAG	16  // Bit 5
#define	NEWMAN_FLAG	32  // Bit 6
#define	MALE_FLAG	64  // Bit 7
#define	FEMALE_FLAG	128 // Bit 8

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <tchar.h>

#include "resource.h"
#include "pso_crypt.h"
#include "bbtable.h"
#include "localgms.h"
#include "prs.cpp"
#include "def_map.h" // Map file name definitions 地图文件
#include "def_block.h" // Blocked packet definitions 舰仓数据
#include "def_packets.h" // Pre-made packet definitions 预加载数据
#include "def_structs.h" // Various structure definitions 各种结构定义
#include "def_tables.h" // Various pre-made table definitions 各种预制表定义

const unsigned char Message03[] = { "Tethealla 舰船服务器 v.148" };

/* function defintions 功能定义 */
#include "def_functions.h"

/* variables 变量*/

struct timeval select_timeout = {
	0,
	5000
};

FILE* debugfile;
unsigned global_rare_mult = 1; //Nova更新的代码

// Random drop rates 随机掉落几率

unsigned rare_box_mult; //Nova更新的代码
unsigned rare_mob_drop_mult; //Nova更新的代码
unsigned WEAPON_DROP_RATE,
ARMOR_DROP_RATE,
MAG_DROP_RATE,
TOOL_DROP_RATE,
MESETA_DROP_RATE,
EXPERIENCE_RATE;
unsigned common_rates[5] = { 0 };

// Rare monster appearance rates 稀有怪物出现几率

unsigned rare_mob_mult; //Nova更新的代码
unsigned hildebear_rate,
rappy_rate,
lily_rate,
slime_rate,
merissa_rate,
pazuzu_rate,
dorphon_rate,
kondrieu_rate = 0;

unsigned common_counters[5] = { 0 };

unsigned char common_table[100000];

unsigned char PacketA0Data[0x4000] = { 0 };
unsigned char Packet07Data[0x4000] = { 0 };
unsigned short Packet07Size = 0;
unsigned char PacketData[TCP_BUFFER_SIZE];
unsigned char PacketData2[TCP_BUFFER_SIZE]; // Sometimes we need two... 就是需要俩
unsigned char tmprcv[PACKET_BUFFER_SIZE];

/* Populated by load_config_file(): 标识配置文件参数*/

unsigned char serverIP[4] = { 0 };
int autoIP = 0;
unsigned char loginIP[4];
unsigned short serverPort;
unsigned short serverMaxConnections;
int ship_support_extnpc = 0;
unsigned serverNumConnections = 0;
unsigned blockConnections = 0;
unsigned blockTick = 0;
unsigned serverConnectionList[SHIP_COMPILED_MAX_CONNECTIONS];
unsigned short serverBlocks;
unsigned char shipEvent;
unsigned serverID = 0;
time_t servertime;
unsigned normalName = 0xFFFFFFFF;
unsigned globalName = 0xFF1D94F7;
unsigned localName = 0xFFB0C4DE;

unsigned short ship_banmasks[5000][4] = { 0 }; // IP address ban masks IP地址封禁掩码
BANDATA ship_bandata[5000];
unsigned num_masks = 0;
unsigned num_bans = 0;

/* Common tables 常用数据表 */

PTDATA pt_tables_ep1[10][4];
PTDATA pt_tables_ep2[10][4];

// Episode I parsed PT data 章节 1 数据解析

unsigned short weapon_drops_ep1[10][4][10][4096];
unsigned char slots_ep1[10][4][4096];
unsigned char tech_drops_ep1[10][4][10][4096];
unsigned char tool_drops_ep1[10][4][10][4096];
unsigned char power_patterns_ep1[10][4][4][4096];
char percent_patterns_ep1[10][4][6][4096];
unsigned char attachment_ep1[10][4][10][4096];

// Episode II parsed PT data 章节 2 数据解析

unsigned short weapon_drops_ep2[10][4][10][4096];
unsigned char slots_ep2[10][4][4096];
unsigned char tech_drops_ep2[10][4][10][4096];
unsigned char tool_drops_ep2[10][4][10][4096];
unsigned char power_patterns_ep2[10][4][4][4096];
char percent_patterns_ep2[10][4][6][4096];
unsigned char attachment_ep2[10][4][10][4096];


/* Rare tables 稀有类型数据表*/

unsigned rt_tables_ep1[0x200 * 10 * 4] = { 0 };
unsigned rt_tables_ep2[0x200 * 10 * 4] = { 0 };
unsigned rt_tables_ep4[0x200 * 10 * 4] = { 0 };

unsigned char startingData[12 * 14];
playerLevel playerLevelData[12][200];

fd_set ReadFDs, WriteFDs, ExceptFDs;

saveLobby savedlobbies[MAX_SAVED_LOBBIES];
unsigned char dp[TCP_BUFFER_SIZE * 4];
unsigned ship_ignore_list[300] = { 0 };
unsigned ship_ignore_count = 0;
unsigned ship_gcsend_list[MAX_GCSEND * 3] = { 0 };
unsigned ship_gcsend_count = 0;
char* Ship_Name[255];
SHIPLIST shipdata[200];
BLOCK* blocks[10];
QUEST quests[512];
QUEST_MENU quest_menus[12];
unsigned* quest_allow = 0; // the "allow" list for the 0x60CA command...
unsigned quest_numallows;
unsigned numQuests = 0;
unsigned questsMemory = 0;
char* languageExts[10];
char* languageNames[10];
wchar_t* languageMessages[10][256]; //尝试解决文本问题
unsigned numLanguages = 0;
unsigned totalShips = 0;
BATTLEPARAM ep1battle[374];
BATTLEPARAM ep2battle[374];
BATTLEPARAM ep4battle[332];
BATTLEPARAM ep1battle_off[374];
BATTLEPARAM ep2battle_off[374];
BATTLEPARAM ep4battle_off[332];
unsigned battle_count;
SHOP shops[7000];
unsigned shop_checksum;
unsigned shopidx[200];
unsigned ship_index;
unsigned char ship_key[128];

// New leet parameter tables!!!!111oneoneoneeleven

unsigned char armor_equip_table[256] = { 0 };
unsigned char barrier_equip_table[256] = { 0 };
unsigned char armor_level_table[256] = { 0 };
unsigned char barrier_level_table[256] = { 0 };
unsigned char armor_dfpvar_table[256] = { 0 };
unsigned char barrier_dfpvar_table[256] = { 0 };
unsigned char armor_evpvar_table[256] = { 0 };
unsigned char barrier_evpvar_table[256] = { 0 };
unsigned char weapon_equip_table[256][256] = { 0 };
unsigned short weapon_atpmax_table[256][256] = { 0 };
unsigned char grind_table[256][256] = { 0 };
unsigned char special_table[256][256] = { 0 };
unsigned char stackable_table[256] = { 0 };
unsigned equip_prices[2][13][24][80] = { 0 };
char max_tech_level[19][12];

PSO_CRYPT* cipher_ptr;

#define MYWM_NOTIFYICON (WM_USER+2)
int program_hidden = 1;
HWND consoleHwnd;

#include "funcs_string.h"
#include "funcs_files.h"

ORANGE logon_structure;
BANANA * connections[SHIP_COMPILED_MAX_CONNECTIONS];
ORANGE * logon_connecion;
BANANA * workConnect;
ORANGE * logon;
unsigned logon_tick = 0;
unsigned logon_ready = 0;

const char serverName[] = { "P\0s\0o\0B\0B\0C\0h\0i\0n\0a\0" };
const char shipSelectString[] = { "S\0h\0i\0p\0 \0S\0e\0l\0e\0c\0t\0" };
const char blockString[] = { "B\0L\0O\0C\0K\0" };

//数据连接
#include "funcs_connect.h"

//舰船数据包
#include "funcs_cBLOCKpacket.h"

//日志代码
#include "funcs_logon.h"

//信息发送代码
#include "funcs_send.h"

//地图数据代码
#include "funcs_mapdata.h"

//任务读取代码
#include "funcs_quest.h"

//公告信息代码
#include "funcs_broadcast.h"






//sancaros 不太明白这段的意义 是否是关乎公会的代码
unsigned char* MakePacketEA15(BANANA* client)
{
	sprintf(&PacketData[0x00], "\x64\x08\xEA\x15\x01");
	memset(&PacketData[0x05], 0, 3);
	*(unsigned *)&PacketData[0x08] = client->guildcard;
	*(unsigned *)&PacketData[0x0C] = client->character.teamID;
	PacketData[0x18] = (unsigned char)client->character.privilegeLevel;
	memcpy(&PacketData[0x1C], &client->character.teamName[0], 28);
	sprintf(&PacketData[0x38], "\x84\x6C\x98");
	*(unsigned *)&PacketData[0x3C] = client->guildcard;
	PacketData[0x40] = client->clientID;
	memcpy(&PacketData[0x44], &client->character.name[0], 24);
	memcpy(&PacketData[0x64], &client->character.teamFlag[0], 0x800);
	return   &PacketData[0];
}

#include "funcs_item.h"



//全部移至send


//大厅名称信息
const char lobbyString[] = { "L\0o\0b\0b\0y\0 \0" };

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
		for (ch = 0x1C;ch<0x5C;ch += 2)
		{
			ship->key_change[ch2 + (ship->decryptbuf[ch] % 4)] = ship->decryptbuf[ch + 1];
			ch2 += 4;
		}
		prepare_key(&ship->user_key[0], 32, &ship->cs_key);
		prepare_key(&ship->user_key[0], 32, &ship->sc_key);
		ship->crypt_on = 1;
		memcpy(&ship->encryptbuf[0x00], &ship->decryptbuf[0x04], 0x28);
		memcpy(&ship->encryptbuf[0x00], &ShipPacket00[0x00], 0x10); // Yep! :)
		ship->encryptbuf[0x00] = 1;
		memcpy(&ship->encryptbuf[0x28], &Ship_Name[0], 12);
		*(unsigned *)&ship->encryptbuf[0x34] = serverNumConnections;
		*(unsigned *)&ship->encryptbuf[0x38] = *(unsigned *)&serverIP[0];
		*(unsigned short*)&ship->encryptbuf[0x3C] = (unsigned short)serverPort;
		*(unsigned *)&ship->encryptbuf[0x3E] = shop_checksum;
		*(unsigned *)&ship->encryptbuf[0x42] = ship_index;
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
			serverID = *(unsigned *)&ship->decryptbuf[0x06];
			if (serverIP[0] == 0x00)
			{
				*(unsigned *)&serverIP[0] = *(unsigned *)&ship->decryptbuf[0x0A];
				printf("更新IP地址为 %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
			}
			serverID++;
			if (serverID != 0xFFFFFFFF)
			{
				printf("舰船已成功注册到登录服务器!!! 舰船 ID: %u\n", serverID);
				printf("构建舰仓列表数据包...\n\n");
				ConstructBlockPacket();
				printf("正在载入任务物品奖励...\n");
				quest_numallows = *(unsigned *)&ship->decryptbuf[0x0E];
				if (quest_allow)
					free(quest_allow);
				quest_allow = malloc(quest_numallows * 4);
				memcpy(quest_allow, &ship->decryptbuf[0x12], quest_numallows * 4);
				printf("任务补助物品数量: %u\n\n", quest_numallows);
				normalName = *(unsigned *)&ship->decryptbuf[0x12 + (quest_numallows * 4)];
				localName = *(unsigned *)&ship->decryptbuf[0x16 + (quest_numallows * 4)];
				globalName = *(unsigned *)&ship->decryptbuf[0x1A + (quest_numallows * 4)];
				memcpy(&ship->user_key[0], &ship_key[0], 128); // 1024-bit key

															   // Change keys

				for (ch2 = 0;ch2<128;ch2++)
					if (ship->key_change[ch2] != -1)
						ship->user_key[ch2] = (unsigned char)ship->key_change[ch2]; // update the key

				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->cs_key);
				prepare_key(&ship->user_key[0], sizeof(ship->user_key), &ship->sc_key);
				memset(&ship->encryptbuf[0x00], 0, 8);
				ship->encryptbuf[0x00] = 0x0F;
				ship->encryptbuf[0x01] = 0x00;
				printf("从服务器请求掉落数据表...\n");
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

			guildcard = *(unsigned *)&ship->decryptbuf[0x06];
			sockfd = *(int *)&ship->decryptbuf[0x0C];

			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->plySockfd == sockfd) && (connections[connectNum]->guildcard == guildcard))
				{
					client = connections[connectNum];
					client->gotchardata = 1;
					memcpy(&client->character.packetSize, &ship->decryptbuf[0x10], sizeof(CHARDATA));

					/* Set up copies of the banks 建立银行的副本 */

					memcpy(&client->char_bank, &client->character.bankUse, sizeof(BANK));
					memcpy(&client->common_bank, &ship->decryptbuf[0x10 + sizeof(CHARDATA)], sizeof(BANK));

					cipher_ptr = &client->server_cipher;
					if (client->isgm == 1)
						*(unsigned *)&client->character.nameColorBlue = globalName;
					else
						if (isLocalGM(client->guildcard))
							*(unsigned *)&client->character.nameColorBlue = localName;
						else
							*(unsigned *)&client->character.nameColorBlue = normalName;

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

					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
					{
						if (client->character.inventory[ch2].in_use)
							FixItem(&client->character.inventory[ch2].item);
					}

					// Fix up equipped weapon, armor, shield, and mag equipment information 修复武器,装甲,盾,还有玛古装备信息

					eq_weapon = 0;
					eq_armor = 0;
					eq_shield = 0;
					eq_mag = 0;

					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
						{
							// Unequip all weapons when there is more than one equipped.  当装备了多个武器时,取消所装备武器
							if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
								(client->character.inventory[ch2].flags & 0x08))
								client->character.inventory[ch2].flags &= ~(0x08);
						}

					}

					if (eq_armor > 1)
					{
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
						{
							// Unequip all mags when there is more than one equipped. 当装备了多个玛古时，取消装备所有玛古。 
							if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
								(client->character.inventory[ch2].flags & 0x08))
								client->character.inventory[ch2].flags &= ~(0x08);
						}
					}

					for (ch2 = 0;ch2<client->character.bankUse;ch2++)
						FixItem((ITEM*)&client->character.bankInventory[ch2]);

					baseATP = *(unsigned short*)&startingData[(client->character._class * 14)];
					baseMST = *(unsigned short*)&startingData[(client->character._class * 14) + 2];
					baseEVP = *(unsigned short*)&startingData[(client->character._class * 14) + 4];
					baseHP = *(unsigned short*)&startingData[(client->character._class * 14) + 6];
					baseDFP = *(unsigned short*)&startingData[(client->character._class * 14) + 8];
					baseATA = *(unsigned short*)&startingData[(client->character._class * 14) + 10];

					for (ch2 = 0;ch2<client->character.level;ch2++)
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

					//client->character.lang = 0x00; 语言？

					cd = (unsigned char*)&client->character.packetSize;

					cd[(8 * 28) + 0x0F] = client->matuse[0];
					cd[(9 * 28) + 0x0F] = client->matuse[1];
					cd[(10 * 28) + 0x0F] = client->matuse[2];
					cd[(11 * 28) + 0x0F] = client->matuse[3];
					cd[(12 * 28) + 0x0F] = client->matuse[4];

					encryptcopy(client, (unsigned char*)&client->character.packetSize, sizeof(CHARDATA));
					client->preferred_lobby = 0xFF;

					cd[(8 * 28) + 0x0F] = 0x00; // Clear this stuff out to not mess up our item procedures. 把这些东西清理干净，以免弄乱我们的物品处理程序。 
					cd[(9 * 28) + 0x0F] = 0x00;
					cd[(10 * 28) + 0x0F] = 0x00;
					cd[(11 * 28) + 0x0F] = 0x00;
					cd[(12 * 28) + 0x0F] = 0x00;

					for (ch2 = 0;ch2<MAX_SAVED_LOBBIES;ch2++)
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
						WriteGM("GM %u (%s) 已连接", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
					else
						WriteLog("用户 %u (%s) 已连接", client->guildcard, Unicode_to_ASCII((unsigned short*)&client->character.name[4]));
					break;
				}
			}
		}
		break;
		case 0x03:
		{
			unsigned guildcard;
			BANANA* client;

			guildcard = *(unsigned *)&ship->decryptbuf[0x06];

			for (ch = 0;ch<serverNumConnections;ch++)
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
		break;
	case 0x06:
		// Reserved 这里又做了功能保留 sancaros
		break;
	case 0x07:
		// Card database full.卡片数据库已满
		gcn = *(unsigned *)&ship->decryptbuf[0x06];

		for (ch = 0;ch<serverNumConnections;ch++)
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
			gcn = *(unsigned *)&ship->decryptbuf[0x06];
			for (ch = 0;ch<serverNumConnections;ch++)
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
			unsigned char *n;
			unsigned char *c;
			unsigned short blockPort;

			gcn = *(unsigned *)&ship->decryptbuf[0x06];
			client_gcn = *(unsigned *)&ship->decryptbuf[0x0A];

			// requesting ship ID @ 0x0E 请求ship ID号

			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard == gcn) && (connections[connectNum]->lobbyNum))
				{
					if (connections[connectNum]->lobbyNum < 0x10)
						for (ch2 = 0;ch2<MAX_SAVED_LOBBIES;ch2++)
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
					*(unsigned *)&ship->encryptbuf[0x02] = serverID;
					*(unsigned *)&ship->encryptbuf[0x06] = *(unsigned *)&ship->decryptbuf[0x0E];
					// 0x10 = 41 result packet
					memset(&ship->encryptbuf[0x0A], 0, 0x136);
					ship->encryptbuf[0x10] = 0x30;
					ship->encryptbuf[0x11] = 0x01;
					ship->encryptbuf[0x12] = 0x41;
					ship->encryptbuf[0x1A] = 0x01;
					*(unsigned *)&ship->encryptbuf[0x1C] = client_gcn;
					*(unsigned *)&ship->encryptbuf[0x20] = gcn;
					ship->encryptbuf[0x24] = 0x10;
					ship->encryptbuf[0x26] = 0x19;
					*(unsigned *)&ship->encryptbuf[0x2C] = *(unsigned *)&serverIP[0];
					blockPort = serverPort + connections[connectNum]->block;
					*(unsigned short *)&ship->encryptbuf[0x30] = (unsigned short)blockPort;
					memcpy(&ship->encryptbuf[0x34], &lobbyString[0], 12);
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
			gcn = *(unsigned *)&ship->decryptbuf[0x20];

			// requesting ship ID @ 0x0E 获取SHIP ID号

			for (ch = 0;ch<serverNumConnections;ch++)
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
			gcn = *(unsigned *)&ship->decryptbuf[0x36];

			// requesting ship ID @ 0x0E获取SHIP ID号

			for (ch = 0;ch<serverNumConnections;ch++)
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
			gcn = *(unsigned *)&ship->decryptbuf[0x07];
			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->guildcard == gcn)
				{
					client = connections[connectNum];
					switch (CreateResult)
					{
					case 0x00:
						// All good!!!
						client->character.teamID = *(unsigned *)&ship->decryptbuf[0x823];
						memcpy(&client->character.teamFlag[0], &ship->decryptbuf[0x0B], 0x800);
						client->character.privilegeLevel = 0x40;
						client->character.unknown15 = 0x00986C84; // ??这段不懂
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

			teamid = *(unsigned *)&ship->decryptbuf[0x07];

			for (ch = 0;ch<serverNumConnections;ch++)
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

			teamid = *(unsigned *)&ship->decryptbuf[0x07];

			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if ((connections[connectNum]->guildcard != 0) && (connections[connectNum]->character.teamID == teamid))
				{
					tClient = connections[connectNum];
					memset(&tClient->character.guildCard2, 0, 2108);
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

			size = *(unsigned *)&ship->decryptbuf[0x00];
			size -= 10;

			teamid = *(unsigned *)&ship->decryptbuf[0x06];

			for (ch = 0;ch<serverNumConnections;ch++)
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

			gcn = *(unsigned *)&ship->decryptbuf[0x0A];
			size = *(unsigned short*)&ship->decryptbuf[0x0E];

			for (ch = 0;ch<serverNumConnections;ch++)
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
		// Reserved
		break;
	case 0x0B:
		// Player authentication result from the logon server.
		gcn = *(unsigned *)&ship->decryptbuf[0x06];
		if (ship->decryptbuf[0x05] == 0)
		{
			BANANA* client;

			// Finish up the logon process here. 在这里完成登录过程 

			for (ch = 0;ch<serverNumConnections;ch++)
			{
				connectNum = serverConnectionList[ch];
				if (connections[connectNum]->temp_guildcard == gcn)
				{
					client = connections[connectNum];
					client->slotnum = ship->decryptbuf[0x0A];
					client->isgm = ship->decryptbuf[0x0B];
					memcpy(&client->encryptbuf[0], &PacketE6[0], sizeof(PacketE6));
					*(unsigned *)&client->encryptbuf[0x10] = gcn;
					client->guildcard = gcn;
					*(unsigned *)&client->encryptbuf[0x14] = *(unsigned*)&ship->decryptbuf[0x0C];
					*(long long *)&client->encryptbuf[0x38] = *(long long*)&ship->decryptbuf[0x10];
					if (client->decryptbuf[0x16] < 0x05)
					{
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
							// Request E7 information from server...
							Send83(client); // Lobby data
							ShipSend04(0x00, client, logon);
						}
					}
					break;
				}
			}
		}
		else
		{
			// Deny connection here.
			for (ch = 0;ch<serverNumConnections;ch++)
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

			// Retrieved ship list data.  Send to client...

			sockfd = *(int *)&ship->decryptbuf[0x06];
			pOffset = *(unsigned short *)&ship->decryptbuf[0x0A];
			memcpy(&PacketA0Data[0x00], &ship->decryptbuf[0x0A], pOffset);
			pOffset += 0x0A;

			totalShips = 0;

			for (ch = 0;ch<PacketA0Data[0x04];ch++)
			{
				shipdata[ch].shipID = *(unsigned *)&ship->decryptbuf[pOffset];
				pOffset += 4;
				*(unsigned *)&shipdata[ch].ipaddr[0] = *(unsigned *)&ship->decryptbuf[pOffset];
				pOffset += 4;
				shipdata[ch].port = *(unsigned short *)&ship->decryptbuf[pOffset];
				pOffset += 2;
				totalShips++;
			}

			for (ch = 0;ch<serverNumConnections;ch++)
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
			printf("从登录服务器接收掉落数据表...\n");
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
		*(unsigned *)&ship->encryptbuf[0x00] = *(unsigned *)&ship->decryptbuf[0x04];
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x04);
		break;
	case 0x10:
		// Monster appearance rates
		printf("\n从服务器收到稀有怪物出现倍率...\n");
		for (ch = 0;ch<8;ch++)
		{
			mob_rate = *(unsigned *)&ship->decryptbuf[0x06 + (ch * 4)];
			mob_calc = (long long)mob_rate * 0xFFFFFFFF / 100000;
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
		// Ping received
		ship->last_ping = (unsigned)servertime;
		*(unsigned *)&ship->encryptbuf[0x00] = *(unsigned *)&ship->decryptbuf[0x04];
		compressShipPacket(ship, &ship->encryptbuf[0x00], 0x04);
		break;
	case 0x12:
		// Global announce
		gcn = *(unsigned *)&ship->decryptbuf[0x06];
		GlobalBroadcast((unsigned short*)&ship->decryptbuf[0x0A]);
		WriteGM("GM %u 发布全服公告: %s", gcn, Unicode_to_ASCII((unsigned short*)&ship->decryptbuf[0x0A]));
		break;
	default:
		// Unknown
		break;
	}
}

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


void FeedMag(unsigned magid, unsigned itemid, BANANA* client)
{
	int found_mag = -1;
	int found_item = -1;
	unsigned ch, ch2, mt_index;
	int EvolutionClass = 0;
	MAG* m;
	unsigned short* ft;
	short mIQ, mDefense, mPower, mDex, mMind;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == magid)
		{
			// Found mag
			if ((client->character.inventory[ch].item.data[0] == 0x02) &&
				(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
			{
				found_mag = ch;
				m = (MAG*)&client->character.inventory[ch].item.data[0];
				for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
				{
					if (client->character.inventory[ch2].item.itemid == itemid)
					{
						// Found item to feed
						if ((client->character.inventory[ch2].item.data[0] == 0x03) &&
							(client->character.inventory[ch2].item.data[1]  < 0x07) &&
							(client->character.inventory[ch2].item.data[1] != 0x02) &&
							(client->character.inventory[ch2].item.data[5] >  0x00))
						{
							found_item = ch2;
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

	//缺失 Sancaros
	if ((found_mag == -1) || (found_item == -1))
	{
		//Send1A(L"Could not find mag to feed or item to feed said mag.", client, 76);
		//client->todc = 1;
	}
	else
	{
		DeleteItemFromClient(itemid, 1, 0, client);

		// Rescan to update Mag pointer (if changed due to clean up)
		for (ch = 0;ch<client->character.inventoryUse;ch++)
		{
			if (client->character.inventory[ch].item.itemid == magid)
			{
				// Found mag (again)
				if ((client->character.inventory[ch].item.data[0] == 0x02) &&
					(client->character.inventory[ch].item.data[1] <= Mag_Agastya))
				{
					found_mag = ch;
					m = (MAG*)&client->character.inventory[ch].item.data[0];
					break;
				}
			}
		}

		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (m->mtype)
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
		m->synchro += ft[mt_index];
		if (m->synchro < 0)
			m->synchro = 0;
		if (m->synchro > 120)
			m->synchro = 120;
		mIQ = m->IQ;
		mIQ += ft[mt_index + 1];
		if (mIQ < 0)
			mIQ = 0;
		if (mIQ > 200)
			mIQ = 200;
		m->IQ = (unsigned char)mIQ;

		// Add Defense

		mDefense = m->defense % 100;
		mDefense += ft[mt_index + 2];

		if (mDefense < 0)
			mDefense = 0;

		if (mDefense >= 100)
		{
			if (m->level == 200)
				mDefense = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->defense = ((m->defense / 100) * 100) + mDefense;
			CheckMagEvolution(m, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			m->defense = ((m->defense / 100) * 100) + mDefense;

		// Add Power

		mPower = m->power % 100;
		mPower += ft[mt_index + 3];

		if (mPower < 0)
			mPower = 0;

		if (mPower >= 100)
		{
			if (m->level == 200)
				mPower = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->power = ((m->power / 100) * 100) + mPower;
			CheckMagEvolution(m, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			m->power = ((m->power / 100) * 100) + mPower;

		// Add Dex

		mDex = m->dex % 100;
		mDex += ft[mt_index + 4];

		if (mDex < 0)
			mDex = 0;

		if (mDex >= 100)
		{
			if (m->level == 200)
				mDex = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->dex = ((m->dex / 100) * 100) + mDex;
			CheckMagEvolution(m, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			m->dex = ((m->dex / 100) * 100) + mDex;

		// Add Mind

		mMind = m->mind % 100;
		mMind += ft[mt_index + 5];

		if (mMind < 0)
			mMind = 0;

		if (mMind >= 100)
		{
			if (m->level == 200)
				mMind = 99; // Don't go above level 200
			else
				m->level++; // Level up!
			m->mind = ((m->mind / 100) * 100) + mMind;
			CheckMagEvolution(m, client->character.sectionID, client->character._class, EvolutionClass);
		}
		else
			m->mind = ((m->mind / 100) * 100) + mMind;
	}
}

void CheckMaxGrind(INVENTORY_ITEM* i)
{
	if (i->item.data[3] > grind_table[i->item.data[1]][i->item.data[2]])
		i->item.data[3] = grind_table[i->item.data[1]][i->item.data[2]];
}


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

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;

			// Copy item before deletion (needed for consumables)
			memcpy(&i, &client->character.inventory[ch], sizeof(INVENTORY_ITEM));

			// Unwrap mag
			if ((i.item.data[0] == 0x02) && (i.item.data2[2] & 0x40))
			{
				client->character.inventory[ch].item.data2[2] &= ~(0x40);
				break;
			}

			// Unwrap item
			if ((i.item.data[0] != 0x02) && (i.item.data[4] & 0x40))
			{
				client->character.inventory[ch].item.data[4] &= ~(0x40);
				break;
			}

			if (i.item.data[0] == 0x03) // Delete consumable item right away
				DeleteItemFromClient(itemid, 1, 0, client);

			break;
		}
	}

	/*if (!found_item)
	{//缺失 Sancaros
	Send1A(L"Could not find item to \"use\".", client, 77);
	//client->todc = 1;
	}
	else*/
	//{
	// Setting the eq variables here should fix problem with ADD SLOT and such.
	eq_wep = eq_armor = eq_shield = eq_mag = -1;

	for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
			// Heaven Punisher used...
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
					Send1A(L"You can't learn that technique.", client, 78);
					client->todc = 1;
				}
				else
					client->character.techniques[i.item.data[4]] = i.item.data[2]; // Learn technique
			}
			break;
		case 0x0A:
			if (eq_wep != -1)
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
				for (ch2 = 0;ch2<5;ch2++)
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
				Send1A(L"Attempt to exceed material usage limit.", client, 79);
				client->todc = 1;
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
				// Merges
				client->character.inventory[eq_shield].item.data[2] = 0x3A + (i.item.data[2] - 0x16);
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
				*(unsigned *)&add_item.item.data[0] = new_item;
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
	//}
}

int check_equip(unsigned char eq_flags, unsigned char cl_flags)
{
	int eqOK = 1;
	unsigned ch;

	for (ch = 0;ch<8;ch++)
	{
		if ((cl_flags & (1 << ch)) && (!(eq_flags & (1 << ch))))
		{
			eqOK = 0;
			break;
		}
	}
	return eqOK;
}

void EquipItem(unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item, found_slot;
	unsigned slot[4];

	found_item = 0;
	found_slot = 0;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
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
					Send1A(L"\"God/Equip\" is disallowed.", client, 80);
					client->todc = 1;
				}
				if (!client->todc)
				{
					// De-equip any other weapon on character. (Prevent stacking)
					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
						Send1A(L"\"God/Equip\" is disallowed.", client, 80);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other armor and equipped slot items.
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
						Send1A(L"\"God/Equip\" is disallowed.", client, 80);
						client->todc = 1;
					}
					if (!client->todc)
					{
						// Remove any other barrier
						for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
					for (ch2 = 0;ch2<4;ch2++)
						slot[ch2] = 0;
					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
					for (ch2 = 0;ch2<4;ch2++)
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
						client->character.inventory[ch].flags &= ~(0x08);
						Send1A(L"There are no free slots on your armor.  Equip unit failed.", client, 81);
						client->todc = 1;
					}
					break;
				}
				break;
			case 0x02:
				// Remove equipped mag
				for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			if (!client->todc)  // Finally, equip item
				client->character.inventory[ch].flags |= 0x08;
			break;
		}
	}
	/*if (!found_item)
	{//缺失 Sancaros
	Send1A(L"Could not find item to equip.", client, 82);
	client->todc = 1;
	}*/
}

void DeequipItem(unsigned itemid, BANANA* client)
{
	unsigned ch, ch2, found_item = 0;

	for (ch = 0;ch<client->character.inventoryUse;ch++)
	{
		if (client->character.inventory[ch].item.itemid == itemid)
		{
			found_item = 1;
			client->character.inventory[ch].flags &= ~(0x08);
			switch (client->character.inventory[ch].item.data[0])
			{
			case 0x00:
				// Remove any other weapon (in case of a glitch... or stacking)
				for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x00) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			case 0x01:
				switch (client->character.inventory[ch].item.data[1])
				{
				case 0x01:
					// Remove any other armor (stacking?) and equipped slot items.
					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
					for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
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
				for (ch2 = 0;ch2<client->character.inventoryUse;ch2++)
					if ((client->character.inventory[ch2].item.data[0] == 0x02) &&
						(client->character.inventory[ch2].flags & 0x08))
						client->character.inventory[ch2].flags &= ~(0x08);
				break;
			}
			break;
		}
	}
	/*if (!found_item)
	{
	Send1A(L"Could not find item to unequip.", client, 83);
	client->todc = 1;
	}*/
}

unsigned GetShopPrice(INVENTORY_ITEM* ci)
{
	unsigned compare_item, ch;
	int percent_add, price;
	unsigned char variation;
	float percent_calc;
	float price_calc;

	price = 10;

	/*	printf ("Raw item data for this item is:\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n%02x%02x%02x%02x\r\n",
	ci->item.data[0], ci->item.data[1], ci->item.data[2], ci->item.data[3],
	ci->item.data[4], ci->item.data[5], ci->item.data[6], ci->item.data[7],
	ci->item.data[8], ci->item.data[9], ci->item.data[10], ci->item.data[11],
	ci->item.data[12], ci->item.data[13], ci->item.data[14], ci->item.data[15] ); */

	switch (ci->item.data[0])
	{
	case 0x00: // Weapons
		if (ci->item.data[4] & 0x80)
			price = 1; // Untekked = 1 meseta
		else
		{
			if ((ci->item.data[1] < 0x0D) && (ci->item.data[2] < 0x05))
			{
				if ((ci->item.data[1] > 0x09) && (ci->item.data[2] > 0x03)) // Canes, Rods, Wands become rare faster
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
		case 0x01: // Armor
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
		case 0x02: // Shield
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
		case 0x03: // Units
			if (ci->item.data[2] < 0x40)
				price = unit_prices[ci->item.data[2]];
			break;
		}
		break;
	case 0x03:
		// Tool
		if (ci->item.data[1] == 0x02) // Technique
		{
			if (ci->item.data[4] < 0x13)
				price = ((int)(ci->item.data[2] + 1) * tech_prices[ci->item.data[4]]) / 100L;
		}
		else
		{
			compare_item = 0;
			memcpy(&compare_item, &ci->item.data[0], 3);
			for (ch = 0;ch<(sizeof(tool_prices) / 4);ch += 2)
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
		*(unsigned *)&PacketBF[0x0C] = tnlxp[target_level - 1] - client->character.XP;
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

	// Add the mag bonus to the 0x30 packet
	for (ch = 0;ch<client->character.inventoryUse;ch++)
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


void AddExp(unsigned XP, BANANA* client)
{
	MAG* m;
	unsigned short levelup, ch, finalDFP, finalATP, finalATA, finalMST;

	if (!client->lobby)
		return;

	client->character.XP += XP;

	PacketBF[0x0A] = (unsigned char)client->clientID;
	*(unsigned *)&PacketBF[0x0C] = XP;
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

		// Add the mag bonus to the 0x30 packet
		for (ch = 0;ch<client->character.inventoryUse;ch++)
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
		for (ch = 0;ch<(ship_gcsend_count * 3);ch += 3)
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
		for (ch = 0;ch<(MAX_GCSEND * 3);ch += 3)
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

	for (ch = 0;ch<(ship_gcsend_count * 3);ch += 3)
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
		for (ch = 0;ch<(ship_gcsend_count * 3);ch += 3)
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

	for (ch = 0;ch<num_bans;ch++)
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
				WriteLog("Could not open bandata.dat for writing!!");
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
		for (ch = 0;ch<num_bans;ch++)
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
			WriteLog("Could not open bandata.dat for writing!!");
	}
	return banned;
}

int stfu(unsigned gc_num)
{
	int result = 0;
	unsigned ch;

	for (ch = 0;ch<ship_ignore_count;ch++)
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

	for (ch = 0;ch<ship_ignore_count;ch++)
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
			ignored = 0; // Can't ignore with a full list...
			SendB0(L"Ship ignore list is full.", client, 1);
		}
	}
	else
	{
		ch2 = 0;
		for (ch = 0;ch<ship_ignore_count;ch++)
		{
			if ((ship_ignore_list[ch] != 0) && (ch != ch2))
				ship_ignore_list[ch2++] = ship_ignore_list[ch];
		}
		ship_ignore_count = ch2;
	}
	return ignored;
}

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
		debug("Client sent a 0x60 packet whose sizecheck != size.\n");
		debug("Command: %02X | Size: %04X | Sizecheck(%02x): %04x\n", client->decryptbuf[0x08],
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
			debug("No size check information for 0x60 lobby packet %02x", client->decryptbuf[0x08]);
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
			// Symbol chat (throttle for spam)
			dont_send = 1;
			if ((((unsigned)servertime - client->command_cooldown[0x07]) >= 1) && (!stfu(client->guildcard)))
			{
				client->command_cooldown[0x07] = (unsigned)servertime;
				if (client->lobbyNum < 0x10)
					max_send = 12;
				else
					max_send = 4;
				for (ch = 0;ch<max_send;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						ignored = 0;
						lClient = l->client[ch];
						for (ch2 = 0;ch2<lClient->ignore_count;ch2++)
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
				// Player hit a monster
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
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			break;
		case 0x20:
			// Remember client's position.
			l->floor[client->clientID] = client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *)&client->decryptbuf[0x18];
			l->clientx[client->clientID] = *(unsigned *)&client->decryptbuf[0x10];
			break;
		case 0x25:
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned *)&client->decryptbuf[0x0C];
				EquipItem(itemid, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x26:
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned *)&client->decryptbuf[0x0C];
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
			// Use item
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				itemid = *(unsigned *)&client->decryptbuf[0x0C];
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
			// Mag feeding
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				magid = *(unsigned *)&client->decryptbuf[0x0C];
				itemid = *(unsigned *)&client->decryptbuf[0x10];
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
				itemid = *(unsigned *)&client->decryptbuf[0x0C];
				count = *(unsigned *)&client->decryptbuf[0x10];
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
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				// Client dropping complete item
				itemid = *(unsigned*)&client->decryptbuf[0x10];
				DeleteItemFromClient(itemid, 0, 1, client);
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0x3E:
		case 0x3F:
			l->clientx[client->clientID] = *(unsigned *)&client->decryptbuf[0x14];
			l->clienty[client->clientID] = *(unsigned *)&client->decryptbuf[0x1C];
			break;
		case 0x40:
		case 0x42:
			l->clientx[client->clientID] = *(unsigned *)&client->decryptbuf[0x0C];
			l->clienty[client->clientID] = *(unsigned *)&client->decryptbuf[0x10];
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
			// Decrease mag sync on death
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				client->dead = 1;
				for (ch = 0;ch<client->character.inventoryUse;ch++)
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
			// Telepipe check
			if ((client->lobbyNum < 0x10) || (client->decryptbuf[0x0E] > 0x11))
			{
				Send1A(L"Incorrect telepipe.", client, 88);
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0x74:
			// W/S (throttle for spam)
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
					for (ch = 0;ch<client->decryptbuf[0x0C];ch++)
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
					for (ch = client->decryptbuf[0x0C];ch<8;ch++)
						*(unsigned short*)&client->decryptbuf[0x10 + (ch * 2)] = ws_data;

					if (ws_ok)
					{
						if (client->lobbyNum < 0x10)
							max_send = 12;
						else
							max_send = 4;

						for (ch = 0;ch<max_send;ch++)
						{
							if ((l->slot_use[ch]) && (l->client[ch]))
							{
								ignored = 0;
								lClient = l->client[ch];
								for (ch2 = 0;ch2<lClient->ignore_count;ch2++)
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
			// Set player flag

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
			// Client selling item
			if ((client->lobbyNum > 0x0F) && (l->floor[client->clientID] == 0))
			{
				itemid = *(unsigned *)&client->decryptbuf[0x0C];
				for (ch = 0;ch<client->character.inventoryUse;ch++)
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
			// Client setting coordinates for stack drop
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				client->drop_area = *(unsigned *)&client->decryptbuf[0x0C];
				client->drop_coords = *(long long*)&client->decryptbuf[0x10];
				client->drop_item = *(unsigned *)&client->decryptbuf[0x18];
			}
			else
			{
				dont_send = 1;
				if (client->lobbyNum < 0x10)
					client->todc = 1;
			}
			break;
		case 0xC4:
			// Inventory sort
			if (client->lobbyNum > 0x0F)
			{
				dont_send = 1;
				SortClientItems(client);
			}
			else
				client->todc = 1;
			break;
		case 0xC5:
			// Visiting hospital
			if (client->lobbyNum > 0x0F)
				DeleteMesetaFromClient(10, 0, client);
			else
				client->todc = 1;
			break;
		case 0xC6:
			// Steal Exp
			if (client->lobbyNum > 0x0F)
			{
				unsigned exp_percent = 0;
				unsigned exp_to_add;
				unsigned char special = 0;

				mid = *(unsigned short*)&client->decryptbuf[0x0C];
				mid &= 0xFFF;
				if (mid < 0xB50)
				{
					for (ch = 0;ch<client->character.inventoryUse;ch++)
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
			// Charge action
			if (client->lobbyNum > 0x0F)
			{
				int meseta;

				meseta = *(int *)&client->decryptbuf[0x0C];
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

						XP = l->mapData[mid].exp * EXPERIENCE_RATE;

						FILE * fp;
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
								SendEE(L"Client/server data synchronization error.  Please reinstall your client and all patches.", client, 135);
								client->todc = 1;
							}
						}

						//debug ("mid death: %u  base: %u, skin: %u, reserved11: %f, exp: %u", mid, l->mapData[mid].base, l->mapData[mid].skin, l->mapData[mid].reserved11, XP);

						if (client->decryptbuf[0x10] != 1) // Not the last player who hit?
							XP = (XP * 77) / 100L;

						if (client->character.level < 199)
							AddExp(XP, client);

						// Increase kill counters for SJS, Lame d'Argent, Limiter and Swordsman Lore

						for (ch = 0;ch<client->character.inventoryUse;ch++)
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
			// Exchange item for team points
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
				for (ch = 0;ch<4;ch++)
				{
					if ((l->slot_use[ch]) && (l->client[ch]))
					{
						lClient = l->client[ch];
						switch (lClient->mode)
						{
						case 0x01:
						case 0x02:
							// Copy character backup
							if (lClient->character_backup)
								memcpy(&lClient->character, lClient->character_backup, sizeof(lClient->character));
							if (lClient->mode == 0x02)
							{
								for (ch2 = 0;ch2<lClient->character.inventoryUse;ch2++)
								{
									if (lClient->character.inventory[ch2].item.data[0] == 0x02)
										lClient->character.inventory[ch2].in_use = 0;
								}
								CleanUpInventory(lClient);
								lClient->character.meseta = 0;
							}
							break;
						case 0x03:
							// Wipe items and reset level.
							for (ch2 = 0;ch2<30;ch2++)
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
							// Unknown mode?
							break;
						}
					}
				}
				// Reset boxes and monsters...
				memset(&l->boxHit, 0, 0xB50); // Reset box and monster data
				memset(&l->monsterData, 0, sizeof(l->monsterData));
			}
			break;
		case 0xD2:
			// Gallon seems to write to this area...
			dont_send = 1;
			if (client->lobbyNum > 0x0F)
			{
				unsigned qofs;

				qofs = *(unsigned *)&client->decryptbuf[0x0C];
				if (qofs < 23)
				{
					qofs *= 4;
					*(unsigned *)&client->character.quest_data2[qofs] = *(unsigned*)&client->decryptbuf[0x10];
					memcpy(&client->encryptbuf[0x00], &client->decryptbuf[0x00], 0x14);
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->decryptbuf[0x00], 0x14);
				}
			}
			else
			{
				dont_send = 1;
				client->todc = 1;
			}
			break;
		case 0xD5:
			// Exchange an item
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
			// Trade PDs for an item from Hopkins' dad
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
					DeleteFromInventory(&work_item, 0xFF, client); // Delete all Photon Drops
					if (!client->todc)
					{
						memcpy(&work_item.item.data[0], &client->decryptbuf[0x0C], 12);
						*(unsigned *)&work_item.item.data2[0] = *(unsigned *)&client->decryptbuf[0x18];
						work_item.item.itemid = l->playerItemID[client->clientID];
						l->playerItemID[client->clientID]++;
						AddToInventory(&work_item, 1, 0, client);
						memset(&client->encryptbuf[0x00], 0, 0x0C);
						// I guess this is a sort of action confirmed by server thing...
						// Also starts an animation and sound... with the wrong values, the camera pans weirdly...
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
			// Add attribute to S-rank weapon (not implemented yet)
			break;
		case 0xD9:
			// Momoka Item Exchange
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
					if (*(unsigned *)&client->character.inventory[ci].item.data[0] == compare_item)
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
					compare_item = *(unsigned *)&client->decryptbuf[0x20];
					for (ci = 0; ci < quest_numallows; ci++)
					{
						if (compare_item == quest_allow[ci])
						{
							*(unsigned *)&add_item.item.data[0] = *(unsigned *)&client->decryptbuf[0x20];
							break;
						}
					}
					if (*(unsigned *)&add_item.item.data[0] == 0)
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
						*(unsigned *)&client->encryptbuf[0x10] = itemid;
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
						*(unsigned *)&client->encryptbuf[0x0C] = itemid;
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
			// Upgrade Photon of weapon
			if ((client->lobbyNum > 0x0F) && (client->decryptbuf[0x0A] == client->clientID))
			{
				INVENTORY_ITEM work_item, work_item2;
				unsigned ci, ai,
					compare_itemid = 0, compare_item1 = 0, compare_item2 = 0, num_attribs = 0;
				char attrib_add;

				memcpy(&compare_item1, &client->decryptbuf[0x0C], 3);
				compare_itemid = *(unsigned *)&client->decryptbuf[0x20];
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
							*(unsigned *)&client->encryptbuf[0x0C] = work_item2.item.itemid;
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
		case 0xDE:
			// Good Luck
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
					if (*(unsigned *)&client->character.inventory[ci].item.data[0] == compare_item)
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
					for (ci = 0;ci<8;ci++)
						client->encryptbuf[0x0C + (ci << 2)] = (mt_lrand() % (sizeof(good_luck) >> 2)) + 1;
					cipher_ptr = &client->server_cipher;
					encryptcopy(client, &client->encryptbuf[0x00], 0x2C);
				}
				else
				{
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					*(unsigned *)&add_item.item.data[0] = good_luck[mt_lrand() % (sizeof(good_luck) >> 2)];
					DeleteItemFromClient(itemid, 1, 0, client);
					memset(&client->encryptbuf[0x00], 0, 0x18);
					client->encryptbuf[0x00] = 0x18;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDB;
					client->encryptbuf[0x09] = 0x06;
					client->encryptbuf[0x0C] = 0x01;
					*(unsigned *)&client->encryptbuf[0x10] = itemid;
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
					*(unsigned *)&client->encryptbuf[0x0C] = itemid;
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
					for (ci = 0;ci<8;ci++)
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

			for (ch = 0;ch<client->character.inventoryUse;ch++)
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
					*(unsigned *)&client->encryptbuf[0x10] = pt_itemid;
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
			/* Temporary
			debug ("0x60 from %u:", client->guildcard);
			display_packet (&client->decryptbuf[0x00], size);
			WriteLog ("0x60 from %u\n%s\n\n:", client->guildcard, (char*) &dp[0]);
			*/
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

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];
	else
		ptd = &pt_tables_ep1[sid][l->difficulty];

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

			if (l->episode == 0x02)
				ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
			else
				ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

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

		for (ch = 0;ch<max_percent;ch++)
		{
			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			if ((do_area) && (!did_area[do_area]))
			{
				did_area[do_area] = 1;
				i->item.data[6 + (num_percents * 2)] = (unsigned char)do_area;
				if (l->episode == 0x02)
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				else
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
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
			if (l->episode == 0x02)
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096];
			else
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];
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
			if (l->episode == 0x02)
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
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
		*(unsigned *)&i->item.data2[0] = meseta;
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

	if (l->episode == 0x02)
		ptd = &pt_tables_ep2[sid][l->difficulty];
	else
		ptd = &pt_tables_ep1[sid][l->difficulty];

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

		if (l->episode == 0x02)
			ch2 = weapon_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
		else
			ch2 = weapon_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];

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

		for (ch = 0;ch<3;ch++)
		{
			if (l->episode == 0x02)
				do_area = attachment_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				do_area = attachment_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
			if ((do_area) && (!did_area[do_area]))
			{
				did_area[do_area] = 1;
				i->item.data[6 + (num_percents * 2)] = (unsigned char)do_area;
				if (l->episode == 0x02)
					percent = percent_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				else
					percent = percent_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
				percent -= 2;
				percent *= 5;
				(char)i->item.data[6 + (num_percents * 2) + 1] = percent;
				num_percents++;
			}
		}

		// Add a grind

		if (l->episode == 0x02)
			ch = power_patterns_ep2[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];
		else
			ch = power_patterns_ep1[sid][l->difficulty][ptd->area_pattern[area]][mt_lrand() % 4096];

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
			if (l->episode == 0x02)
				i->item.data[5] = slots_ep2[sid][l->difficulty][mt_lrand() % 4096];
			else
				i->item.data[5] = slots_ep1[sid][l->difficulty][mt_lrand() % 4096];

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
		if (l->episode == 0x02)
			*(unsigned *)&i->item.data[0] = tool_remap[tool_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096]];
		else
			*(unsigned *)&i->item.data[0] = tool_remap[tool_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096]];
		if (i->item.data[1] == 0x02) // Technique
		{
			if (l->episode == 0x02)
				i->item.data[4] = tech_drops_ep2[sid][l->difficulty][area][mt_lrand() % 4096];
			else
				i->item.data[4] = tech_drops_ep1[sid][l->difficulty][area][mt_lrand() % 4096];
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
		*(unsigned *)&i->item.data2[0] = meseta;
		break;
	default:
		break;
	}
	i->item.itemid = l->itemID++;
}

void Send62(BANANA* client)
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
		// Send guild card
		if ((size == 0x0C) && (t < maxt))
		{
			if ((l->slot_use[t]) && (l->client[t]))
			{
				lClient = l->client[t];
				PrepGuildCard(client->guildcard, lClient->guildcard);
				memset(&PacketData[0], 0, 0x114);
				sprintf(&PacketData[0x00], "\x14\x01\x60");
				PacketData[0x03] = 0x00;
				PacketData[0x04] = 0x00;
				PacketData[0x08] = 0x06;
				PacketData[0x09] = 0x43;
				*(unsigned *)&PacketData[0x0C] = client->guildcard;
				memcpy(&PacketData[0x10], &client->character.name[0], 24);
				memcpy(&PacketData[0x60], &client->character.guildcard_text[0], 176);
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
			itemid = *(unsigned *)&client->decryptbuf[0x0C];
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
				*(unsigned *)&PacketData[0x10] = itemid;
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
					if (l->episode == 0x02)
						ptd = &pt_tables_ep2[client->character.sectionID][l->difficulty];
					else
						ptd = &pt_tables_ep1[client->character.sectionID][l->difficulty];

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
									*(unsigned *)&l->gameItem[itemNum].item.data2[0] = meseta;
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
								*(unsigned *)&PacketData[0x0C] = *(unsigned *)&client->decryptbuf[0x0C];
								memcpy(&PacketData[0x10], &client->decryptbuf[0x10], 10);
								memcpy(&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12);
								*(unsigned *)&PacketData[0x28] = l->gameItem[itemNum].item.itemid;
								*(unsigned *)&PacketData[0x2C] = *(unsigned *)&l->gameItem[itemNum].item.data2[0];
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
	case 0x6F:
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

					//debug ("quest loaded: %i", l->quest_loaded);

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
							if (((mb->flag2 - FLOAT_PRECISION) < (float) 1.00000) &&
								((mb->flag2 + FLOAT_PRECISION) > (float) 1.00000))
							{
								// Fixed item alert!!!

								box_rare = 1;
								itemNum = free_game_item(l);
								if (((mb->flag3 - FLOAT_PRECISION) < (float) 1.00000) &&
									((mb->flag3 + FLOAT_PRECISION) > (float) 1.00000))
								{
									// Fully fixed!

									*(unsigned *)&l->gameItem[itemNum].item.data[0] = *(unsigned *)&mb->drop[0];

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
											if (l->gameItem[itemNum].item.data[0] <  0x02)
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

						for (ch = 0;ch<30;ch++)
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
						*(unsigned *)&PacketData[0x0C] = *(unsigned *)&client->decryptbuf[0x0C];
						memcpy(&PacketData[0x10], &client->decryptbuf[0x10], 10);
						memcpy(&PacketData[0x1C], &l->gameItem[itemNum].item.data[0], 12);
						*(unsigned *)&PacketData[0x28] = l->gameItem[itemNum].item.itemid;
						*(unsigned *)&PacketData[0x2C] = *(unsigned *)&l->gameItem[itemNum].item.data2[0];
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
		// Chair info
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
				client->doneshop[client->decryptbuf[0x0C]] = shopidx[client->character.level] + (333 * ((unsigned)client->decryptbuf[0x0C])) + (mt_lrand() % 333);
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
							l->playerItemID[client->clientID] = *(unsigned *)&client->decryptbuf[0x0C];
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

			compare_item = *(unsigned *)&client->decryptbuf[0x0C];

			for (ch = 0;ch<client->character.inventoryUse;ch++)
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
		// Client accepting tekked version of weapon.
		if ((client->lobbyNum > 0x0F) && (client->tekked.item.itemid))
		{
			unsigned ch2;

			for (ch = 0;ch<4;ch++)
			{
				if ((l->slot_use[ch]) && (l->client[ch]))
				{
					for (ch2 = 0;ch2<l->client[ch]->character.inventoryUse;ch2++)
						if (l->client[ch]->character.inventory[ch2].item.itemid == client->tekked.item.itemid)
						{
							Send1A(L"Item duplication attempt!", client, 94);
							client->todc = 1;
							break;
						}
				}
			}

			for (ch = 0;ch<l->gameItemCount;l++)
			{
				itemNum = l->gameItemList[ch];
				if (l->gameItem[itemNum].item.itemid == client->tekked.item.itemid)
				{
					// Added to the game's inventory by the client...
					// Delete it and avoid duping...
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
		// Client accessing bank
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

				for (ch = 0;ch<client->character.bankUse;ch++)
					client->character.bankInventory[ch].itemid = l->bankItemID[client->clientID]++;
				memset(&client->encryptbuf[0x00], 0, 0x34);
				client->encryptbuf[0x02] = 0x6C;
				client->encryptbuf[0x08] = 0xBC;
				bank_size = 0x18 * (client->character.bankUse + 1);
				*(unsigned *)&client->encryptbuf[0x0C] = bank_size;
				bank_size += 4;
				*(unsigned short *)&client->encryptbuf[0x00] = (unsigned short)bank_size;
				bank_use = mt_lrand();
				*(unsigned *)&client->encryptbuf[0x10] = bank_use;
				bank_use = client->character.bankUse;
				*(unsigned *)&client->encryptbuf[0x14] = bank_use;
				*(unsigned *)&client->encryptbuf[0x18] = client->character.bankMeseta;
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
					itemid = *(unsigned *)&client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *)&client->decryptbuf[0x10];

						if (client->character.meseta >= meseta)
						{
							client->character.bankMeseta += meseta;
							client->character.meseta -= meseta;
							if (client->character.bankMeseta > 999999)
								client->character.bankMeseta = 999999;
						}
						else
						{
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
					itemid = *(unsigned *)&client->decryptbuf[0x0C];
					if (itemid == 0xFFFFFFFF)
					{
						meseta = *(unsigned *)&client->decryptbuf[0x10];
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

				gcn = *(unsigned *)&client->decryptbuf[0x0C];
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
				meseta = *(int *)&client->decryptbuf[0x0C];
				if (meseta < 0)
				{
					meseta = -meseta;
					client->character.meseta -= (unsigned)meseta;
				}
				else
				{
					memset(&add_item, 0, sizeof(INVENTORY_ITEM));
					add_item.item.data[0] = 0x04;
					*(unsigned *)&add_item.item.data2[0] = *(unsigned *)&client->decryptbuf[0x0C];
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
					WriteLog("用户 %u 试图领取任务奖励 %08x ,但该项目不在允许列表中.", client->guildcard, compare_item);
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
			wrap_id = *(unsigned *)&client->decryptbuf[0x18];

			for (ch = 0;ch<client->character.inventoryUse;ch++)
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
	case 0xE0:
		if (client->lobbyNum > 0x0F)
		{
			if ((l->oneperson) && (l->quest_loaded) && (l->drops_disabled) && (!l->questE0))
			{
				unsigned bp, bpl, new_item;

				if (client->decryptbuf[0x0D] > 0x03)
					bpl = 1;
				else
					bpl = l->difficulty + 1;

				for (bp = 0;bp<bpl;bp++)
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
					*(unsigned *)&client->encryptbuf[0x18] = new_item;
					*(unsigned *)&client->encryptbuf[0x24] = l->itemID;
					itemNum = free_game_item(l);
					if (l->gameItemCount < MAX_SAVED_ITEMS)
						l->gameItemList[l->gameItemCount++] = itemNum;
					memset(&l->gameItem[itemNum], 0, sizeof(GAME_ITEM));
					*(unsigned *)&l->gameItem[itemNum].item.data[0] = new_item;
					if (new_item == 0x04)
					{
						new_item = pt_tables_ep1[client->character.sectionID][l->difficulty].enemy_meseta[0x2E][0];
						new_item += mt_lrand() % 100;
						*(unsigned *)&client->encryptbuf[0x28] = new_item;
						*(unsigned *)&l->gameItem[itemNum].item.data2[0] = new_item;
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
	default:
		if (client->lobbyNum > 0x0F)
		{
			WriteLog("62 指令 \"%02x\" 未被游戏进行接收处理. (数据如下)", client->decryptbuf[0x08]);
			packet_to_text(&client->decryptbuf[0x00], size);
			WriteLog("%s", &dp[0]);
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
						*(unsigned *)&client->decryptbuf[0x7C] = client->guildcard;
						// Check techniques...
						if (!(client->equip_flags & DROID_FLAG))
						{
							for (ch = 0;ch<19;ch++)
							{
								if ((char)client->decryptbuf[0xC4 + ch] > max_tech_level[ch][client->character._class])
								{
									(char)client->character.techniques[ch] = -1; // Unlearn broken technique.
									client->todc = 1;
								}
							}
							if (client->todc)
								Send1A(L"Technique data check failed.\n\nSome techniques have been unlearned.", client, 98);
						}
						memcpy(&client->decryptbuf[0xC4], &client->character.techniques, 20);
						// Could check character structure here
						memcpy(&client->decryptbuf[0xD8], &client->character.gcString, 104);
						// Prevent crashing with NPC skins...
						if (client->character.skinFlag)
							memset(&client->decryptbuf[0x110], 0, 10);
						// Could check stats here
						memcpy(&client->decryptbuf[0x148], &client->character.ATP, 36);
						// Could check inventory here
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
							dont_send = 0; // It's cool to send them as long as this user is bursting and we're the leader.
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


void Send01(const wchar_t *text, BANANA* client, int line)
{
	if (line>-1 && strlen(languageMessages[client->character.lang][line]))
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
	for (ch = 0;ch<strlen(text);ch++)
	{
		PacketData[mesgOfs++] = text[ch];
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


void WriteLog(char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[MAX_GM_MESG_LEN];
	SYSTEMTIME rawtime;

	FILE *fp;

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


void WriteGM(char *fmt, ...)
{
#define MAX_GM_MESG_LEN 4096

	va_list args;
	char text[MAX_GM_MESG_LEN];
	SYSTEMTIME rawtime;

	FILE *fp;

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


char character_file[255];

#include "funcs_command.h"

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