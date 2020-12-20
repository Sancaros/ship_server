#pragma once
void BroadcastToAll(unsigned short *mes, BANANA* client)
{
	unsigned short xEE_Len;
	unsigned short *pd;
	unsigned short *cname;
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
		for (ch = 0;ch<serverNumConnections;ch++)
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
		// Global announcement
		if ((logon) && (logon->sockfd >= 0))
		{
			// Send the announcement to the logon server.
			logon->encryptbuf[0x00] = 0x12;
			logon->encryptbuf[0x01] = 0x00;
			*(unsigned *)&logon->encryptbuf[0x02] = client->guildcard;
			memcpy(&logon->encryptbuf[0x06], &PacketData[sizeof(PacketEE)], xEE_Len - sizeof(PacketEE));
			compressShipPacket(logon, &logon->encryptbuf[0x00], 6 + xEE_Len - sizeof(PacketEE));
		}
	}
	client->announce = 0;
}

void GlobalBroadcast(unsigned short *mes)
{
	unsigned short xEE_Len;
	unsigned short *pd;
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
	for (ch = 0;ch<serverNumConnections;ch++)
	{
		connectNum = serverConnectionList[ch];
		if (connections[connectNum]->guildcard)
		{
			cipher_ptr = &connections[connectNum]->server_cipher;
			encryptcopy(connections[connectNum], &PacketData[0], xEE_Len);
		}
	}
}