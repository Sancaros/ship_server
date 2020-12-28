
#define reveal_window \
	ShowWindow ( consoleHwnd, SW_NORMAL ); \
	SetForegroundWindow ( consoleHwnd ); \
	SetFocus ( consoleHwnd )

#define swapendian(x) ( ( x & 0xFF ) << 8 ) + ( x >> 8 )
#define FLOAT_PRECISION 0.00001



#define SERVER_VERSION L"0.148"
#define SHIP_NAME L"未完成识别代码"//用于服务器窗口显示舰船名称

#define USEADDR_ANY
#define LOGON_PORT 3455 //Nova更新的代码 用于log12.22
#define TCP_BUFFER_SIZE 64000
#define PACKET_BUFFER_SIZE ( TCP_BUFFER_SIZE * 16 )

#define LOG_60 //日志开启12.22

#define SHIP_COMPILED_MAX_CONNECTIONS 900
#define SHIP_COMPILED_MAX_GAMES 75
#define LOGIN_RECONNECT_SECONDS 15
#define MAX_SIMULTANEOUS_CONNECTIONS 6
#define MAX_SAVED_LOBBIES 20
#define MAX_SAVED_ITEMS 3000
#define MAX_GCSEND 2000
#define ALL_ARE_GM 0
#define PRS_BUFFER 262144

#define SEND_PACKET_03 0x00 // Done
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

#define HUNTER_FLAG	1   // Bit 1 猎人
#define RANGER_FLAG	2   // Bit 2 枪手
#define FORCE_FLAG	4   // Bit 3 法师
#define HUMAN_FLAG	8   // Bit 4 人类
#define	DROID_FLAG	16  // Bit 5 机器人
#define	NEWMAN_FLAG	32  // Bit 6 新人类
#define	MALE_FLAG	64  // Bit 7 男人
#define	FEMALE_FLAG	128 // Bit 8 女人