/* variables 变量*/
struct timeval select_timeout = {
	0,
	5000
};

FILE* debugfile;
unsigned global_rare_mult = 1; //Nova更新的代码 //12.22
							   // Random drop rates 随机掉落几率
unsigned rare_box_mult; //Nova更新的代码 //12.22
unsigned rare_mob_drop_mult; //Nova更新的代码 //12.22
unsigned WEAPON_DROP_RATE,
ARMOR_DROP_RATE,
MAG_DROP_RATE,
TOOL_DROP_RATE,
MESETA_DROP_RATE,
EXPERIENCE_RATE;
unsigned common_rates[5] = { 0 };

// Rare monster appearance rates 稀有怪物出现几率

unsigned rare_mob_mult; //Nova更新的代码 //12.22
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
char Ship_Name[255]; //舰船名称没完成
					 //wchar_t Ship_Name[255]; //舰船名称没完成
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
wchar_t* languageMessages[10][256];
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

