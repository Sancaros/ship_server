/* function defintions 功能定义 */
extern void	mt_bestseed(void);
extern void	mt_seed(void);	/* Choose seed from random input 从随机输入中选择种子. */
extern unsigned long mt_lrand(void);	/* Generate 32-bit random value  生成32位随机值  */

char* Unicode_to_ASCII(unsigned short* ucs);
void WriteLog(char *fmt, ...);
void WriteGM(char *fmt, ...);
void ShipSend04(unsigned char command, BANANA* client, ORANGE* ship);
void ShipSend0E(ORANGE* ship);
void Send01(const wchar_t *text, BANANA* client, int line);
void ShowArrows(BANANA* client, int to_all);
unsigned char* MakePacketEA15(BANANA* client);
void SendToLobby(LOBBY* l, unsigned max_send, unsigned char* src, unsigned short size, unsigned nosend);
void removeClientFromLobby(BANANA* client);

void debug(char *fmt, ...);
void debug_perror(char * msg);
void tcp_listen(int sockfd);
int tcp_accept(int sockfd, struct sockaddr *client_addr, int *addr_len);
int tcp_sock_connect(char* dest_addr, int port);
int tcp_sock_open(struct in_addr ip, int port);

void encryptcopy(BANANA* client, const unsigned char* src, unsigned size);
void decryptcopy(unsigned char* dest, const unsigned char* src, unsigned size);

void prepare_key(unsigned char *keydata, unsigned len, struct rc4_key *key);
void compressShipPacket(ORANGE* ship, unsigned char* src, unsigned long src_size);
void decompressShipPacket(ORANGE* ship, unsigned char* dest, unsigned char* src);
int qflag(unsigned char* flag_data, unsigned flag, unsigned difficulty);