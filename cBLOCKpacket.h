#pragma once
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
	_itoa(serverID, &tempName[0], 10);
	if (serverID < 10)
	{
		tempName[0] = 0x30;
		tempName[1] = 0x30 + serverID;
		tempName[2] = 0x00;
	}
	else
		_itoa(serverID, &tempName[0], 10);
	strcat(&tempName[0], ":");
	strcat(&tempName[0], (unsigned char*) &Ship_Name[0]);
	Packet07Data[0x32] = 0x08;
	Offset = 0x12;
	tn = &tempName[0];
	while (*tn != 0x00)
	{
		Packet07Data[Offset++] = *(tn++);
		Packet07Data[Offset++] = 0x00;
	}
	Offset = 0x36;
	for (ch = 0;ch<serverBlocks;ch++)
	{
		Packet07Data[Offset] = 0x12;
		BlockID = 0xEFFFFFFF - ch;
		*(unsigned *)&Packet07Data[Offset + 2] = BlockID;
		memcpy(&Packet07Data[Offset + 0x08], &blockString[0], 10);
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
	*(unsigned *)&Packet07Data[Offset + 2] = BlockID;
	memcpy(&Packet07Data[Offset + 0x08], &shipSelectString[0], 22);
	Offset += 0x2C;
	while (Offset % 8)
		Packet07Data[Offset++] = 0x00;
	*(unsigned short*)&Packet07Data[0x00] = (unsigned short)Offset;
	Packet07Size = Offset;
}