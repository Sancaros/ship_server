#pragma once
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
		printf("�޷�������־�ļ� ship.log\n");
	}

	fprintf(fp, "[%u��%02u��%02u��, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
	fclose(fp);

	printf("[%u��%02u��%02u��, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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
		printf("�޷�д��GM������־ gm.log\n");
	}

	fprintf(fp, "[%u��%02u��%02u��, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
	fclose(fp);

	printf("[%u��%02u��%02u��, %02u:%02u] %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, text);
}