#pragma once
typedef struct st_chardata { //16进制 10进制
	unsigned short packetSize; // 0x00-0x01 0 - 1  Always set to 0x399C 修改为0x39A0 长度 1475 总字节长度
	unsigned short command; // 0x02-0x03 2 - 3 Always set to 0x00E7 指令占用231字节
	unsigned char flags[4]; // 0x04-0x07 4 - 7 定义职业的吧？
	unsigned char inventoryUse; // 0x08 8 人物槽位
	unsigned char HPuse; // 0x09 血量 9
	unsigned char TPuse; // 0x0A 魔法值 10
	unsigned char lang; // 0x0B 语言 11
	INVENTORY inventory[30]; // 0x0C-0x353 //30格背包 12 - 851
	unsigned short ATP[2]; // 0x354-0x355 852 - 853
	unsigned short MST[2]; // 0x356-0x357 854 - 855
	unsigned short EVP[2]; // 0x358-0x359 856 - 857
	unsigned short HP[2]; // 0x35A-0x35B 858 - 859
	unsigned short DFP[2]; // 0x35C-0x35D 860 - 861
	unsigned short TP[2]; // 0x35E-0x35F 862 - 863
	unsigned short LCK[2]; // 0x360-0x361 864 - 865
	unsigned short ATA[2]; // 0x362-0x363 866 - 867
	//unsigned char unknown[8]; // 0x364-0x36B 868 - 875  (Offset 0x360 has 0x0A value on Schthack's...) 偏移量0x360在Schthack上有0x0A值。 
	unsigned short level[373]; // 0x36C-0x36D; 876 - 877
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
	unsigned char unknown7[16]; // 0x4E4 - 0x4F3; 1252 - 1267
	unsigned char options[4]; // 0x4F4-0x4F7;
							  // Stored from ED 01 packet.
	unsigned char quest_data1[520]; // 0x4F8 - 0x6FF; 1272 - 1791
	unsigned bankUse; // 0x700 - 0x703 1792 - 1795
	unsigned bankMeseta; // 0x704 - 0x707; 1796 - 1799
	BANK_ITEM bankInventory[200]; // 0x708 - 0x19C7 1800 - 6599
	unsigned guildCard; // 0x19C8-0x19CB; 6600 - 6603
						// Stored from E8 06 packet.
	unsigned char name2[24]; // 0x19CC - 0x19E3; 6604 - 6627
	unsigned char unknown9[232]; // 0x19E4-0x1ACB; 6628 - 6859
	unsigned char reserved1;  // 0x1ACC; 6860 // Has value 0x01 on Schthack's
	unsigned char reserved2; // 0x1ACD; 6861 // Has value 0x01 on Schthack's
	unsigned char sectionID2; // 0x1ACE; 6862
	unsigned char _class2; // 0x1ACF; 6863
	unsigned char unknown10[4]; // 0x1AD0-0x1AD3; 6864 - 6867
	unsigned char symbol_chats[1248]; // 0x1AD4 - 0x1FB3 6868 - 8115
									  // Stored from ED 02 packet.
	unsigned char shortcuts[2624]; // 0x1FB4 - 0x29F3 8116 - 10739
								   // Stored from ED 03 packet.
	unsigned char autoreply[344]; // 0x29F4 - 0x2B4B; 10740 - 11083
	unsigned char GCBoard[172]; // 0x2B4C - 0x2BF7; 11084 - 11255
	unsigned char unknown12[200]; // 0x2BF8 - 0x2CBF; 11256 - 11455
	unsigned char challengeData[320]; // 0x2CC0 - 0x2DFF 11456 - 11775
	unsigned char unknown13[172]; // 0x2E00 - 0x2EAB; 11776 - 11947
	unsigned char unknown14[276]; // 0x2EAC - 0x2FBF; 11948 - 12223
								  // I don't know what this is, but split from unknown13 because this chunk is
								  // actually copied into the 0xE2 packet during login @ 0x08 
								  //我不知道这是什么，但是从unknown13开始拆分，因为这个块实际上是在登录期间复制到0xE2包中的 

	unsigned char keyConfigGlobal[364]; // 0x2FC0 - 0x312B  12224 - 12587
										// Copied into 0xE2 login packet @ 0x11C 复制到0xE2登录包@0x11C 
										// Stored from ED 04 packet.
	
	unsigned char joyConfigGlobal[56]; // 0x312C - 0x3163 12588 - 12643
									   // Copied into 0xE2 login packet @ 0x288
									   // Stored from ED 05 packet.
	
	unsigned guildCard2; // 0x3164 - 0x3167 12644 - 12647
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