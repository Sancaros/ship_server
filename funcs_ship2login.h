#pragma once
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
	for (ch = 0;ch<128;ch++)
		logon->key_change[ch] = -1;
	*(unsigned *)&logon->_ip.s_addr = *(unsigned *)&loginIP[0];
}

void reconnect_logon()
{
	// Just in case this is called because of an error in communication with the logon server.
	logon->sockfd = tcp_sock_connect(inet_ntoa(logon->_ip), 3455);
	if (logon->sockfd >= 0)
	{
		printf("�����ɹ���������½������!\n");
		logon->last_ping = (unsigned)time(NULL);
	}
	else
	{
		printf("����ʧ��.���� %u �������...\n", LOGIN_RECONNECT_SECONDS);
		logon_tick = 0;
	}
}