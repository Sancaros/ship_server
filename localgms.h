//! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*!

Header for local GMs on Tethealla ships.

Written by Jon Wayne Parrott 8/18/08

Modified by Sodaboy for compatibility under
VC++ 2008 8/21/2008

This source is under no copyright.

*/
//! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


typedef struct st_localgm
{
	unsigned int guildcard;
	int level;
} LOCALGM;

LOCALGM localgms[100];
int localgmcount = -1;
int localgmrights[11];

int readLocalGMFile()
{
	FILE* gmfile;
	char inistring[512];
	int i;

	printf("正在载入本地 GM 列表和权限..\n");

	gmfile = fopen("localgms.ini", "r");

	if (gmfile != NULL)
	{
		printf("本地 GM 文件已载入, 解析中...\n");

		fgets(inistring, 512, gmfile);

		while (!feof(gmfile))
		{
			if (inistring[0] != '#' && inistring[0] != ';')
			{
				//! GC# and privledge level of local gms
				if (strstr(inistring, "[localgms]") != NULL)
				{
					while (fgets(inistring, 512, gmfile))
					{
						if (inistring[0] != '#' && inistring[0] != ';' && inistring[0] != '[')
						{

							localgmcount += 1;

							localgms[localgmcount].guildcard = atoi(strtok(inistring, ","));
							localgms[localgmcount].level = atoi(strtok(NULL, ","));

							printf("增加 GM # %u 并使其拥有权限等级 %u\n", localgms[localgmcount].guildcard, localgms[localgmcount].level);

						}
						if (inistring[0] == '[')
						{
							break;
						}
					}
				}
				else
					//! Rights
					if (strstr(inistring, "[gmrights]") != NULL)
					{
						while (fgets(inistring, 512, gmfile))
						{
							if (inistring[0] != '#' && inistring[0] != ';' && inistring[0] != '[')
							{

								int index = atoi(strtok(inistring, ","));

								localgmrights[index] = atoi(strtok(NULL, ","));

							}
							if (inistring[0] == '[')
							{
								break;
							}
						}
					}

					else
					{
						fgets(inistring, 512, gmfile);
					}
			}
			else
			{
				fgets(inistring, 512, gmfile);
			}
		}

		for (i = 0;i<10;i++)
		{
			if (localgmrights[i])
				printf(" 索引 %i: 权限 %i\n", i, localgmrights[i]);
		}

		fclose(gmfile);
		return 1;

	}
	else
	{
		printf("无法载入 localgms.ini 文件!");
		return 0;
	}

}

int getbit(int value, int bit)
{
	int mask = 1 << bit;

	if ((value&mask)>0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int playerHasRights(unsigned int guildcard, int command)
{
	int i;
	int rights = 0;

	//printf("Checking player %i's permission",guildcard);
	for (i = 0; i <= localgmcount; i++)
	{
		if (localgms[i].guildcard == guildcard)
		{
			rights = localgmrights[localgms[i].level];
			//printf("player has rights %i\n", rights);
		}
	}

	if (getbit(rights, command))
	{
		//printf("player authorized\n");
		return 1;
	}
	else
	{
		return 0;
	}
}

int isLocalGM(unsigned int guildcard)
{
	int i;
	for (i = 0; i <= localgmcount; i++)
	{
		if (localgms[i].guildcard == guildcard)
		{
			return 1;
		}
	}
	return 0;
}
