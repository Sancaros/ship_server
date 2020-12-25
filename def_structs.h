#define NO_ALIGN __declspec(align(1))

//typedef struct NO_ALIGN st_ptdata //这是游戏原始数据结构
typedef struct st_ptdata
{
	unsigned char weapon_ratio[12]; // high = 0x0D
	char weapon_minrank[12];
	unsigned char weapon_reserved[12]; // ??
	unsigned char power_pattern[9][4];
	unsigned short percent_pattern[23][6];
	unsigned char area_pattern[30];
	unsigned char percent_attachment[6][10];
	unsigned char element_ranking[10];
	unsigned char element_probability[10];
	unsigned char armor_ranking[5];
	unsigned char slot_ranking[5];
	unsigned char unit_level[10];
	unsigned short tool_frequency[28][10];
	unsigned char tech_frequency[19][10];
	char tech_levels[19][20];
	unsigned char enemy_dar[100];
	unsigned short enemy_meseta[100][2];
	char enemy_drop[100];
	unsigned short box_meseta[10][2];
	unsigned char reserved[0x1000 - 0x8C8];
} PTDATA;

/* Ban Structure 封禁结构 */

typedef struct st_bandata
{
	unsigned guildcard;
	unsigned type; // 1 = account, 2 = ipaddr, 3 = hwinfo 1=账户 2=ip地址 3=地域封禁
	unsigned ipaddr;
	long long hwinfo;
} BANDATA;


/* Saved Lobby Structure 保存的房间结构*/

typedef struct st_saveLobby {
	unsigned guildcard; //采用的是人物唯一ID来识别
	unsigned short lobby;
} saveLobby;


/* Weapon pmt structure 武器参数结构*/

//typedef struct NO_ALIGN st_weappmt
typedef struct st_weappmt
{
	// Starts @ 0x4348
	unsigned index;
	short model;
	short skin;
	short unknown1;
	short unknown2;
	unsigned short equippable;
	short atpmin;
	short atpmax;
	short atpreq;
	short mstreq;
	short atareq;
	short mstadd;
	unsigned char grind;
	unsigned char photon_color;
	unsigned char special_type;
	unsigned char ataadd;
	unsigned char unknown4[14];
} weappmt;


/* Armor pmt structure 装备参数结构*/

//typedef struct NO_ALIGN st_armorpmt
typedef struct st_armorpmt
{
	// Starts @ 0x40 with barriers (Barrier and armor share the same structure...)
	// Armors start @ 0x14f0
	unsigned index;
	short model;
	short skin;
	short u1;
	short u2;
	short dfp;
	short evp;
	short u3;
	unsigned short equippable;
	unsigned char level;
	unsigned char efr;
	unsigned char eth;
	unsigned char eic;
	unsigned char edk;
	unsigned char elt;
	unsigned char dfp_var;
	unsigned char evp_var;
	short u4;
	short u5;
} armorpmt;


/* Battle parameter structure 战斗参数结构*/

//typedef struct NO_ALIGN st_battleparam {
typedef struct st_battleparam {
	unsigned short ATP;
	unsigned short MST;
	unsigned short EVP;
	unsigned short HP;
	unsigned short DFP;
	unsigned short ATA;
	unsigned short LCK;
	unsigned short ESP;
	unsigned reserved1;
	unsigned reserved2;
	unsigned reserved3;
	unsigned XP;
	unsigned reserved4;
} BATTLEPARAM;


/* Character Data Structure 角色基础数据结构*/

//typedef struct NO_ALIGN st_playerLevel {
typedef struct st_playerLevel {
	unsigned char ATP;
	unsigned char MST;
	unsigned char EVP;
	unsigned char HP;
	unsigned char DFP;
	unsigned char ATA;
	unsigned char LCK;
	unsigned char TP;
	unsigned XP;
} playerLevel;


/* Mag Structure 玛古结构*/

//typedef struct NO_ALIGN st_mag
typedef struct st_mag
{
	unsigned char two; // "02" =P
	unsigned char mtype;
	unsigned char level;
	unsigned char blasts;
	short defense;
	short power;
	short dex;
	short mind;
	unsigned itemid;
	char synchro;
	unsigned char IQ;
	unsigned char PBflags;
	unsigned char color;
} MAG;


/* Item Structure (Without Flags 无标记) 物品结构*/

//typedef struct NO_ALIGN st_item {
typedef struct st_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare 标准的参数
	unsigned itemid; // player item id
	unsigned char data2[4]; // $setitem4 (mag use only)
} ITEM;


/* Bank Item Structure 银行仓库物品结构*/

//typedef struct NO_ALIGN st_bank_item {
typedef struct st_bank_item {
	unsigned char data[12]; // the standard $setitem1 - $setitem3 fare 标准的参数结构
	unsigned itemid; // player item id 玩家物品id
	unsigned char data2[4]; // $setitem4 (mag use only) 这是给玛古使用的空间
	unsigned bank_count; // Why? 银行统计？
} BANK_ITEM;

/* Bank Structure 银行仓库结构*/

//typedef struct NO_ALIGN st_bank {
typedef struct st_bank {
	unsigned bankUse;
	unsigned bankMeseta;
	BANK_ITEM bankInventory[200];
} BANK;


/* Item Structure (Includes Flags) 物品结构*/

//typedef struct NO_ALIGN st_inventory_item {
typedef struct st_inventory_item {
	unsigned char in_use; // 0x01 = item slot in use, 0xFF00 = unused
	unsigned char reserved[3]; //这里在login服务器没有定义
	unsigned flags; // 8 = equipped
	ITEM item;
} INVENTORY_ITEM;


/* Game Inventory Item Structure 游戏背包物品结构*/

//typedef struct NO_ALIGN st_game_item {
typedef struct st_game_item {
	unsigned gm_flag; // reserved
	ITEM item;
} GAME_ITEM;


/* Main Character Structure 主要角色结构*/
typedef struct st_chardata { //16进制 10进制
	unsigned short packetSize; // 0x00-0x01 0 - 1  Always set to 0x399C 修改为0x39A0 长度 14752 总字节长度
	unsigned short command; // 0x02-0x03 2 - 3 Always set to 0x00E7 指令占用231字节
	unsigned char flags[4]; // 0x04-0x07 4 - 7 定义职业的吧？
	unsigned char inventoryUse; // 0x08 8 人物槽位
	unsigned char HPmat; // 0x09 血量 9
	unsigned char TPmat; // 0x0A 魔法值 10
	unsigned char lang; // 0x0B 语言 11
	INVENTORY_ITEM inventory[30]; // 0x0C-0x353 //30格背包 12 - 851
	unsigned short ATP; // 0x354-0x355 852 - 853
	unsigned short MST; // 0x356-0x357 854 - 855
	unsigned short EVP; // 0x358-0x359 856 - 857
	unsigned short HP; // 0x35A-0x35B 858 - 859
	unsigned short DFP; // 0x35C-0x35D 860 - 861
	unsigned short ATA; // 0x35E-0x35F 862 - 863
	unsigned short LCK; // 0x360-0x361 864 - 865
	//unsigned char unknown[10]; // 0x362-0x36B 866 - 867


	unsigned char option_flags[10]; // 0x362-0x36B 866 - 875

	//unsigned short TP; // 0x35E-0x35F 862 - 863
	//unsigned short LCK; // 0x360-0x361 864 - 865
	//unsigned short ATA; // 0x362-0x363 866 - 867

	//unsigned char unknown[8]; // 0x364-0x36B 868 - 875  (Offset 0x360 has 0x0A value on Schthack's...) 偏移量0x360在Schthack上有0x0A值。 
	
	unsigned short level; // 0x36C-0x36D; 876 - 877



	unsigned short unknown2; // 0x36E-0x36F; 878 - 879
	unsigned XP; // 0x370-0x373 880 - 883
	unsigned meseta; // 0x374-0x377; 884 - 887
	char gcString[10]; // 0x378-0x381; 888 - 897
	unsigned char unknown3[14]; // 0x382-0x38F; 898 - 911  // Same as E5 unknown2 和E5指令的 未知函数 2 一样
	unsigned char nameColorBlue; // 0x390; 912 
	unsigned char nameColorGreen; // 0x391; 913
	unsigned char nameColorRed; // 0x392; 914
	unsigned char nameColorTransparency; // 0x393; 915
	unsigned short skinID; // 0x394-0x395; 916 - 917
	unsigned char unknown4[18]; // 0x396-0x3A7 918 - 935
	unsigned char sectionID; // 0x3A8; 936
	unsigned char _class; // 0x3A9; 937
	unsigned char skinFlag; // 0x3AA; 938
	unsigned char unknown5[5]; // 0x3AB-0x3AF; 939 - 943  // Same as E5 unknown4. 和E5指令的 未知函数 4 一样
	unsigned short costume; // 0x3B0 - 0x3B1; 944 - 945
	unsigned short skin; // 0x3B2 - 0x3B3; 946 - 947
	unsigned short face; // 0x3B4 - 0x3B5; 948 - 949
	unsigned short head; // 0x3B6 - 0x3B7; 950 - 951
	unsigned short hair; // 0x3B8 - 0x3B9; 952 - 953
	unsigned short hairColorRed; // 0x3BA-0x3BB; 954 - 959
	unsigned short hairColorBlue; // 0x3BC-0x3BD; 960 - 961
	unsigned short hairColorGreen; // 0x3BE-0x3BF; 958 - 959
	unsigned proportionX; // 0x3C0-0x3C3; 960 - 963
	unsigned proportionY; // 0x3C4-0x3C7; 964 - 967
	unsigned char name[24]; // 0x3C8-0x3DF; 968 - 991
	unsigned playTime; // 0x3E0 - 0x3E3 992 - 995
	unsigned char unknown6[4]; // 0x3E4 - 0x3E7; 996 - 999
	unsigned char keyConfig[232]; // 0x3E8 - 0x4CF; 1000 - 1231
								  // Stored from ED 07 packet.  从ED 07包中存储。 

	unsigned char techniques[20]; // 0x4D0 - 0x4E3; 1232 - 1251
	unsigned char name3[16]; // 0x4E4 - 0x4F3; 1252 - 1267
							 //unsigned char unknown7[16]; // 0x4E4 - 0x4F3; 1252 - 1267
	unsigned char options[4]; // 0x4F4-0x4F7; 1268 - 1271
							  // Stored from ED 01 packet.
	unsigned char quest_data1[520]; // 0x4F8 - 0x6FF; 1272 - 1791
	unsigned bankUse; // 0x700 - 0x703 1792 - 1795
	unsigned bankMeseta; // 0x704 - 0x707; 1796 - 1799
	BANK_ITEM bankInventory[200]; // 0x708 - 0x19C7 1800 - 6599
	unsigned guildCard; // 0x19C8-0x19CB; 6600 - 6603
						// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3; 6604 - 6627
	unsigned char unknown9[56]; // 0x19E4-0x1ACB; 6628 - 6859
	unsigned char guildcard_text[176]; // 0x1A1C - 0x1ACB
	unsigned char reserved1;  // 0x1ACC; 6860 // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; 6861 // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE; 6862
	unsigned char _class2; // 0x1ACF; 6863


	unsigned char unknown10[4]; // 0x1AD0-0x1AD3; 6864 - 6867


	unsigned char symbol_chats[1248]; // 0x1AD4 - 0x1FB3 6868 - 8115
									  // Stored from ED 02 packet.
	unsigned char shortcuts[2624]; // 0x1FB4 - 0x29F3 8116 - 10739
								   // Stored from ED 03 packet.
	unsigned char autoReply[344]; // 0x29F4 - 0x2B4B; 10740 - 11083
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7; 11084 - 11255
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF; 11256 - 11455
	unsigned char challengeData[320]; // 0x2CC0 - 0x2DFF 11456 - 11775

									  //unsigned char unknown13[172]; // 0x2E00 - 0x2EAB; 11776 - 11947 分解为三种数据
	unsigned char techConfig[40]; // 0x2E00 - 0x2E27 11776 - 11815
	unsigned char unknown13[44]; // 0x2E28-0x2E4F 11816 - 11859
	unsigned char battleData[88]; // 0x2E50 - 0x2EAB (Quest data 2 任务数据2) 11856 - 11947
	//unsigned char unknown13[44]; // 0x2E28-0x2E53 11816 - 11860
	//unsigned char quest_data2[88]; // 0x2E55 - 0x2EAB (Quest data 2 任务数据2) 11861 - 11947

	unsigned char unknown14[276]; // 0x2EAC - 0x2FBF; 11948 - 12223
								  // I don't know what this is, but split from unknown13 because this chunk is
								  // actually copied into the 0xE2 packet during login @ 0x08 
								  //我不知道这是什么，但是从unknown13开始拆分，因为这个块实际上是在登录期间复制到0xE2包中的 
	/*S服务端中的关于 276字节的定义
	uint8_t unk1[0x0114];
	struct {
		uint32_t guildcard;
		uint16_t name[0x18]; //24
		uint16_t team[0x10]; //16
		uint16_t desc[0x58]; //88
		uint8_t reserved1;
		uint8_t language;
		uint8_t section;
		uint8_t ch_class;
	} blocked[29];
	*/
	unsigned char keyConfigGlobal[364]; // 0x2FC0 - 0x312B  12224 - 12587
										// Copied into 0xE2 login packet @ 0x11C 复制到0xE2登录包@0x11C 
										// Stored from ED 04 packet.

	unsigned char joyConfigGlobal[56]; // 0x312C - 0x3163 12588 - 12643
									   // Copied into 0xE2 login packet @ 0x288
									   // Stored from ED 05 packet.

	unsigned serial_number; // 0x3164 - 0x3167 12644 - 12647
						 //(From here on copied into 0xE2 login packet @ 0x2C0...)

	unsigned teamID; // 0x3168 - 0x316B 12648 - 12651
	unsigned char teamInformation[8]; // 0x316C - 0x3173 12652 - 12659
									  //(usually blank...)
	unsigned short privilegeLevel; // 0x3174 - 0x3175 12660 - 12661
	unsigned short reserved3; // 0x3176 - 0x3177 12662 - 12663
	unsigned char teamName[28]; // 0x3178 - 0x3193 12664 - 12691
	unsigned unknown15; // 0x3194 - 0x3197 12692 - 12695
	unsigned char teamFlag[2048]; // 0x3198 - 0x3997 12696 - 14743
	unsigned char teamRewards[8]; // 0x3998 - 0x39A0 14744 - 14752
} CHARDATA;

/*
//typedef struct NO_ALIGN st_chardata {
typedef struct st_chardata {
	unsigned short packetSize; // 0x00-0x01  // Always set to 0x399C 修改为0x39A0 长度 1475 总字节长度
	unsigned short command; // 0x02-0x03 // // Always set to 0x00E7
	unsigned char flags[4]; // 0x04-0x07
	unsigned char inventoryUse; // 0x08
	unsigned char HPmat; // 0x09
	unsigned char TPmat; // 0x0A
	unsigned char lang; // 0x0B
	INVENTORY_ITEM inventory[30]; // 0x0C-0x353
	unsigned short ATP; // 0x354-0x355
	unsigned short MST; // 0x356-0x357
	unsigned short EVP; // 0x358-0x359
	unsigned short HP; // 0x35A-0x35B
	unsigned short DFP; // 0x35C-0x35D
	unsigned short TP; // 0x35E-0x35F
	unsigned short LCK; // 0x360-0x361
	unsigned short ATA; // 0x362-0x363
	unsigned char unknown[8]; // 0x364-0x36B  (Offset 0x360 has 0x0A value on Schthack's...)
	unsigned short level; // 0x36C-0x36D;
	unsigned short unknown2; // 0x36E-0x36F;
	unsigned XP; // 0x370-0x373
	unsigned meseta; // 0x374-0x377;
	char gcString[10]; // 0x378-0x381;
	unsigned char unknown3[14]; // 0x382-0x38F;  // Same as E5 unknown2
	unsigned char nameColorBlue; // 0x390;
	unsigned char nameColorGreen; // 0x391;
	unsigned char nameColorRed; // 0x392;
	unsigned char nameColorTransparency; // 0x393;
	unsigned short skinID; // 0x394-0x395;
	unsigned char unknown4[18]; // 0x396-0x3A7
	unsigned char sectionID; // 0x3A8;
	unsigned char _class; // 0x3A9;
	unsigned char skinFlag; // 0x3AA;
	unsigned char unknown5[5]; // 0x3AB-0x3AF;  // Same as E5 unknown4.
	unsigned short costume; // 0x3B0 - 0x3B1;
	unsigned short skin; // 0x3B2 - 0x3B3;
	unsigned short face; // 0x3B4 - 0x3B5;
	unsigned short head; // 0x3B6 - 0x3B7;
	unsigned short hair; // 0x3B8 - 0x3B9;
	unsigned short hairColorRed; // 0x3BA-0x3BB;
	unsigned short hairColorBlue; // 0x3BC-0x3BD;
	unsigned short hairColorGreen; // 0x3BE-0x3BF;
	unsigned proportionX; // 0x3C0-0x3C3;
	unsigned proportionY; // 0x3C4-0x3C7;
	unsigned char name[24]; // 0x3C8-0x3DF;
	unsigned playTime; // 0x3E0 - 0x3E3;
	unsigned char unknown6[4];  // 0x3E4 - 0x3E7
	unsigned char keyConfig[232]; // 0x3E8 - 0x4CF;
								  // Stored from ED 07 packet.
	unsigned char techniques[20]; // 0x4D0 - 0x4E3;
	unsigned char unknown7[16]; // 0x4E4 - 0x4F3;
	unsigned char options[4]; // 0x4F4-0x4F7;
							  // Stored from ED 01 packet.
	unsigned reserved4; // not sure
	unsigned char quest_data1[512]; // 0x4FC - 0x6FB; (Quest data 1)
	unsigned reserved5;
	unsigned bankUse; // 0x700 - 0x703
	unsigned bankMeseta; // 0x704 - 0x707;
	BANK_ITEM bankInventory[200]; // 0x708 - 0x19C7
	unsigned guildCard; // 0x19C8-0x19CB;
						// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3;
	unsigned char unknown9[56]; // 0x19E4-0x1A1B;
	unsigned char guildcard_text[176]; // 0x1A1C - 0x1ACB
	unsigned char reserved1;  // 0x1ACC; // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE;
	unsigned char _class2; // 0x1ACF;
	unsigned char unknown10[4]; // 0x1AD0-0x1AD3;
	unsigned char symbol_chats[1248];	// 0x1AD4 - 0x1FB3
										// Stored from ED 02 packet.
	unsigned char shortcuts[2624];	// 0x1FB4 - 0x29F3
									// Stored from ED 03 packet.
	unsigned char autoReply[344]; // 0x29F4 - 0x2B4B;
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7;
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF;
	unsigned char challengeData[320]; // 0x2CC0 - 0X2DFF
	unsigned char techConfig[40]; // 0x2E00 - 0x2E27
	unsigned char unknown13[40]; // 0x2E28-0x2E4F
	unsigned char quest_data2[92]; // 0x2E50 - 0x2EAB (Quest data 2)
	unsigned char unknown14[276]; // 0x2EAC - 0x2FBF; // I don't know what this is, but split from unknown13 because this chunk is
								  // actually copied into the 0xE2 packet during login @ 0x08
	unsigned char keyConfigGlobal[364]; // 0x2FC0 - 0x312B  // Copied into 0xE2 login packet @ 0x11C
										// Stored from ED 04 packet.
	unsigned char joyConfigGlobal[56]; // 0x312C - 0x3163 // Copied into 0xE2 login packet @ 0x288
									   // Stored from ED 05 packet.
	unsigned guildCard2; // 0x3164 - 0x3167 (From here on copied into 0xE2 login packet @ 0x2C0...)
	unsigned teamID; // 0x3168 - 0x316B
	unsigned char teamInformation[8]; // 0x316C - 0x3173 (usually blank...)
	unsigned short privilegeLevel; // 0x3174 - 0x3175
	unsigned short reserved3; // 0x3176 - 0x3177
	unsigned char teamName[28]; // 0x3178 - 0x3193
	unsigned unknown15; // 0x3194 - 0x3197
	unsigned char teamFlag[2048]; // 0x3198 - 0x3997
	unsigned char teamRewards[8]; // 0x3998 - 0x39A0
} CHARDATA;
*/


/* Connected Client Structure */

typedef struct st_banana {
	int plySockfd;
	int block;
	unsigned char rcvbuf[TCP_BUFFER_SIZE];
	unsigned short rcvread;
	unsigned short expect;
	unsigned char decryptbuf[TCP_BUFFER_SIZE]; // Used when decrypting packets from the client...从客户端解密数据包时使用
	unsigned char sndbuf[TCP_BUFFER_SIZE];
	unsigned char encryptbuf[TCP_BUFFER_SIZE]; // Used when making packets to send to the client...在制作要发送给客户端的数据包时使用
	unsigned char packet[TCP_BUFFER_SIZE];
	int snddata,
		sndwritten;
	int crypt_on;
	PSO_CRYPT server_cipher, client_cipher;
	CHARDATA character;
	unsigned char equip_flags;
	unsigned matuse[5];
	int mode; // Usually set to 0, but changes during challenge and battle play 只有在挑战模式和对战模式才会发生改变
	void* character_backup; // regular character copied here during challenge and battle 只有在挑战模式和对战模式才会发生改变
	int gotchardata; //获取角色数据的结构定义
	unsigned guildcard;
	unsigned temp_guildcard;
	long long hwinfo;
	int isgm;
	int slotnum;
	unsigned response;		// Last time client responded...
	unsigned lastTick;		// The last second
	unsigned toBytesSec;	// How many bytes per second the server sends to the client
	unsigned fromBytesSec;	// How many bytes per second the server receives from the client
	unsigned packetsSec;	// How many packets per second the server receives from the client
	unsigned char sendCheck[MAX_SENDCHECK + 2];
	unsigned char preferred_lobby;
	unsigned short lobbyNum;
	unsigned char clientID;
	int bursting;
	int teamaccept;
	int masterxfer;
	int todc;
	unsigned dc_time;
	unsigned char IP_Address[16]; // Text version
	unsigned char ipaddr[4]; // Binary version
	unsigned connected;
	unsigned savetime;
	unsigned connection_index;
	unsigned drop_area;
	long long drop_coords;
	unsigned drop_item;
	int released;
	unsigned char releaseIP[4];
	unsigned short releasePort;
	int sending_quest;
	unsigned qpos;
	int hasquest;
	int doneshop[3];
	int dead;
	int lobbyOK;
	unsigned ignore_list[100];
	unsigned ignore_count;
	INVENTORY_ITEM tekked;
	unsigned team_info_flag, team_info_request;
	unsigned command_cooldown[256];
	unsigned team_cooldown[32];
	int bankType;
	int bankAccess;
	BANK common_bank;
	BANK char_bank;
	void* lobby;
	int announce;
	int debugged;
} BANANA;


/* Quest Details Structure */

typedef struct st_qdetails {
	unsigned short qname[32];
	unsigned short qsummary[128];
	unsigned short qdetails[256];
	unsigned char* qdata;
	unsigned qsize;
} QDETAILS;

/* Loaded Quest Structure */

typedef struct st_quest {
	QDETAILS* ql[10];  // Supporting 10 languages
	unsigned char* mapdata;
	unsigned max_objects;
	unsigned char* objectdata;
} QUEST;


/* Assembled Quest Menu Structure */

typedef struct st_questmenu {
	unsigned num_categories;
	unsigned char c_names[10][256];
	unsigned char c_desc[10][256];
	unsigned quest_counts[10];
	unsigned quest_indexes[10][32];
} QUEST_MENU;


/* a RC4 expanded key session */

const unsigned char RC4publicKey[32] = {
	103, 196, 247, 176, 71, 167, 89, 233, 200, 100, 044, 209, 190, 231, 83, 42,
	6, 95, 151, 28, 140, 243, 130, 61, 107, 234, 243, 172, 77, 24, 229, 156
};

struct rc4_key {
	unsigned char state[256];
	unsigned x, y;
};


/* Connected Logon Server Structure */

typedef struct st_orange {
	int sockfd;
	struct in_addr _ip;
	unsigned char rcvbuf[TCP_BUFFER_SIZE];
	unsigned long rcvread;
	unsigned long expect;
	unsigned char decryptbuf[TCP_BUFFER_SIZE];
	unsigned char sndbuf[PACKET_BUFFER_SIZE];
	unsigned char encryptbuf[TCP_BUFFER_SIZE];
	int snddata, sndwritten;
	unsigned char packet[PACKET_BUFFER_SIZE];
	unsigned long packetdata;
	unsigned long packetread;
	int crypt_on;
	unsigned char user_key[128];
	int key_change[128];
	struct rc4_key cs_key;
	struct rc4_key sc_key;
	unsigned last_ping;
} ORANGE;


/* Ship List Structure (Assembled from Logon Packet) */

typedef struct st_shiplist {
	unsigned shipID;
	unsigned char ipaddr[4];
	unsigned short port;
} SHIPLIST;


/* Shop Item Structure */

//typedef struct NO_ALIGN st_shopitem {
typedef struct st_shopitem {
	unsigned char data[12];
	unsigned reserved3;
	unsigned price;
} SHOP_ITEM;


/* Shop Structure */

//typedef struct NO_ALIGN st_shop {
typedef struct st_shop {
	unsigned short packet_length;
	unsigned short command;
	unsigned flags;
	unsigned reserved;
	unsigned char type;
	unsigned char num_items;
	unsigned short reserved2;
	SHOP_ITEM item[0x18];
	unsigned char reserved4[16];
} SHOP;


/* Map Monster Structure */

//typedef struct NO_ALIGN st_mapmonster {
typedef struct st_mapmonster {
	unsigned base;	// 4
	unsigned reserved[11]; // 44
	float reserved11; // 4
	float reserved12; // 4
	unsigned reserved13; // 4
	unsigned exp; // 4
	unsigned skin; // 4
	unsigned rt_index;  // 4
} MAP_MONSTER;


/* Map box structure */

typedef struct st_mapbox {
	float flag1;
	float flag2;
	float flag3;
	unsigned char drop[8];
} MAP_BOX;


/* Internal Monster Structure */

typedef struct st_monster {
	short HP;
	unsigned dead[4];
	unsigned drop;
} MONSTER;


/* Lobby Structure */

typedef struct st_lobby {
	unsigned char floor[12];
	unsigned clientx[12];
	unsigned clienty[12];
	unsigned char arrow_color[12];
	unsigned lobbyCount;
	MONSTER monsterData[0xB50];
	MAP_MONSTER mapData[0xB50]; // For figuring out which monsters go where, etc.
	MAP_BOX objData[0xB50]; // Box drop information
	unsigned mapIndex;
	unsigned objIndex;
	unsigned rareIndex;
	unsigned char rareData[0x20];
	unsigned char boxHit[0xB50];
	GAME_ITEM gameItem[MAX_SAVED_ITEMS]; // Game Item Data
	unsigned gameItemList[MAX_SAVED_ITEMS]; // Game Item Link List
	unsigned gameItemCount;
	unsigned itemID;
	unsigned playerItemID[4];
	int questE0; // Server already dropped BP reward?
	int drops_disabled; // Basically checks if someone exchanged a photon crystal
	unsigned bankItemID[4];
	unsigned leader;
	unsigned char sectionID;
	unsigned gamePlayerCount; // This number increases as people join and depart the game...
	unsigned gamePlayerID[4]; // Keep track for leader purposes...
	unsigned char gameName[30];
	unsigned char gamePassword[32];
	unsigned char gameMap[128];
	unsigned char gameMonster[0x04];
	unsigned char episode;
	unsigned char difficulty;
	unsigned char battle;
	unsigned char challenge;
	unsigned char oneperson;
	unsigned short battle_level;
	int meseta_boost;
	int quest_in_progress;
	int quest_loaded;
	int inpquest;
	unsigned start_time;
	int in_use;
	int redbox;
	int slot_use[12];
	BANANA* client[12];
	BATTLEPARAM* bptable;
} LOBBY;


/* Block Structure */

typedef struct st_block {
	LOBBY lobbies[16 + SHIP_COMPILED_MAX_GAMES];
	unsigned count; // keep track of how many people are on this block
} BLOCK;