#include "Base64.hpp"
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#define UNKNOW 0
#define ENCODE 1
#define DECODE 2
#define CONFIG "Base64_Config.ini"


int main(int argc, char *argv[])
{
	//打开配置文件
	HANDLE hConfigFile = CreateFileA(CONFIG, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hConfigFile == INVALID_HANDLE_VALUE)
	{
		puts("配置文件打开失败!");
		return -1;
	}

	//初始化类
	Base64 b64;
	if (b64.GetLastError() != Base64::ErrorCode::CLASS_NO_ERROR)
	{
		puts(b64.GetErrorReason(b64.GetLastError()));
		return -1;
	}

	//打开成功
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		DWORD dwReadBytes;

		unsigned char ucBase64Code[64];
		if (!ReadFile(hConfigFile, ucBase64Code, 64, &dwReadBytes, NULL) || dwReadBytes != 64)
		{
			CloseHandle(hConfigFile);
			puts("文件读取失败!");
			return -1;
		}
		//设置加密串
		if (!b64.SetBaseCode(ucBase64Code))
		{
			CloseHandle(hConfigFile);
			puts(b64.GetErrorReason(b64.GetLastError()));
			return -1;
		}

		//读取直到下一行
		char cReadChar;
		do
		{
			if (!ReadFile(hConfigFile, &cReadChar, 1, &dwReadBytes, NULL) || dwReadBytes != 1)
			{
				CloseHandle(hConfigFile);
				puts("文件读取失败!");
				return -1;
			}
		} while (cReadChar != '\n');//循环读取直到换行

		//读取到填充字符
		unsigned char ucFullCode;
		if (!ReadFile(hConfigFile, &ucFullCode, 1, &dwReadBytes, NULL) || dwReadBytes != 1)
		{
			CloseHandle(hConfigFile);
			puts("文件读取失败!");
			return -1;
		}
		//设置填充字符
		if (!b64.SetFullCode(ucFullCode))
		{
			CloseHandle(hConfigFile);
			puts(b64.GetErrorReason(b64.GetLastError()));
			return -1;
		}
	}
	else//打开失败创建新的文件(不常见情况)
	{
		DWORD dwWriteBytes;

		if (!WriteFile(hConfigFile, b64.GetBaseCode(), 64, &dwWriteBytes, NULL) || dwWriteBytes != 64)//写入加密串
		{
			CloseHandle(hConfigFile);
			puts("文件写入失败!");
			return -1;
		}

		unsigned char cTemp[3] = {'\r','\n',b64.GetFullCode()};//CRLF换行和填充字符

		if (!WriteFile(hConfigFile, cTemp, 3, &dwWriteBytes, NULL) || dwWriteBytes != 3)//写入换行和填充字符
		{
			CloseHandle(hConfigFile);
			puts("文件写入失败!");
			return -1;
		}
	}
	CloseHandle(hConfigFile);

	//命令行参数判断
	if (argc != 4)
	{
		puts("请使用:\nBase64 [-OE|-E] or [-OD|D] [待转换文件名] [转换结果文件名]\n");
		system("pause");
		return -1;
	}

	if (argv[1][0] != '-' && argv[1][0] != '/')
	{
		puts("请使用:\nBase64 [-OE|-E] or [-OD|-D] [待转换文件名] [转换结果文件名]\n");
		return -1;
	}

	//命令行参数解析
	int iCode = UNKNOW;
	bool bOverwrite = false;
	char *pArgv = &argv[1][1];
	while (*pArgv != '\0')
	{
		switch (toupper(*pArgv))
		{
		case 'E':
			if (iCode != UNKNOW)
			{
				puts("冲突或重复的命令行选项!\n");
				return -1;
			}
			iCode = ENCODE;
			break;
		case 'D':
			if (iCode != UNKNOW)
			{
				puts("冲突或重复的命令行选项!\n");
				return -1;
			}
			iCode = DECODE;
			break;
		case 'O':
			if (bOverwrite == true)
			{
				puts("重复的命令行选项!\n");
				return -1;
			}
			bOverwrite = true;
			break;
		case '?':
			puts("请使用:\nBase64 [-OE|-E] or [-OD|-D] [待转换文件名] [转换结果文件名]\n");
			return -1;
			break;
		default:
			puts("参数错误!");
			puts("请使用:\nBase64 [-OE|-E] or [-OD|-D] [待转换文件名] [转换结果文件名]\n");
			return -1;
			break;
		}

		++pArgv;
	}


	//打开输出文件
	HANDLE hWriteFile = NULL;
	hWriteFile = CreateFileA(argv[3], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, bOverwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hWriteFile == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_EXISTS)
		{
			puts("输出文件已存在!\n如果要覆盖文件,请使用-O选项");
		}
		else
		{
			puts("输出文件打开失败!");
		}
		return -1;
	}


	//打开输入文件并映射
	HANDLE hReadFile = CreateFileA(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReadFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hWriteFile);
		puts("输入文件打开失败!");
		return -1;
	}

	//获得文件大小
	LARGE_INTEGER liFileSize = {0};
	if (!GetFileSizeEx(hReadFile, &liFileSize))
	{
		CloseHandle(hReadFile);
		CloseHandle(hWriteFile);
		puts("文件大小获取失败!");
		return -1;
	}

	//判断文件为空
	if (liFileSize.QuadPart == 0)
	{
		CloseHandle(hReadFile);
		CloseHandle(hWriteFile);
		puts("文件为空!");
		return -1;
	}

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW(hReadFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hFileMapping)
	{
		CloseHandle(hReadFile);
		CloseHandle(hWriteFile);
		puts("文件映射对象创建失败!");
		return -1;
	}
	CloseHandle(hReadFile);//关闭输入文件

	//映射文件到内存
	LPVOID lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!lpReadMem)
	{
		CloseHandle(hWriteFile);
		CloseHandle(hFileMapping);
		puts("映射文件到内存失败!");
		return -1;
	}
	CloseHandle(hFileMapping);//关闭文件映射对象


	//加密解密
	ULONGLONG ullStartTime, ullDuringOperation;//计时器变量

	ULONGLONG ullDataSize = 0;
	PUCHAR pucData = NULL;
	if (iCode == ENCODE)
	{
		//计算大小
		ullDataSize = b64.GetEnCodeSize(NULL, liFileSize.QuadPart);
		pucData = (PUCHAR)malloc(ullDataSize);

		//计时并加密
		ullStartTime = GetTickCount64();
		bool bOperationSuccess = b64.EnCode(lpReadMem, liFileSize.QuadPart, pucData, ullDataSize);
		ullDuringOperation = GetTickCount64() - ullStartTime;

		if (!bOperationSuccess)
		{
			CloseHandle(hWriteFile);
			UnmapViewOfFile(lpReadMem);
			puts(b64.GetErrorReason(b64.GetLastError()));
			return -1;
		}

		printf("加密成功,用时:%lld毫秒(约为%.2lf秒)\n正在写入文件\n", ullDuringOperation, (long double)ullDuringOperation / (long double)1000.00);
	}
	else if (iCode == DECODE)
	{
		//计算大小
		ullDataSize = b64.GetDeCodeSize((Base64::PCUCHAR)lpReadMem, liFileSize.QuadPart);
		pucData = (PUCHAR)malloc(ullDataSize);

		//计时并解密
		ullStartTime = GetTickCount64();
		bool bOperationSuccess = b64.DeCode((Base64::PCUCHAR)lpReadMem, liFileSize.QuadPart, pucData, ullDataSize);
		ullDuringOperation = GetTickCount64() - ullStartTime;

		if (!bOperationSuccess)
		{
			CloseHandle(hWriteFile);
			UnmapViewOfFile(lpReadMem);
			puts(b64.GetErrorReason(b64.GetLastError()));
			return -1;
		}

		printf("解密成功,用时:%lld毫秒(约为%.2lf秒)\n正在写入文件\n", ullDuringOperation, (long double)ullDuringOperation / (long double)1000.00);
	}

	//写入文件
	DWORD dwWriteByte;
	DWORD dwWriteDataByte;

	ullStartTime = GetTickCount64();
	while (ullDataSize != 0)//数据过大则分多次写入
	{
		if (ullDataSize >= MAXDWORD)//数据大于单次最大写入字节则写入最大写入字节
		{
			dwWriteDataByte = MAXDWORD;
		}
		else//直到数据小于最大写入字节时写入剩余数据
		{
			dwWriteDataByte = (DWORD)ullDataSize;
		}

		if (!WriteFile(hWriteFile, pucData, dwWriteDataByte, &dwWriteByte, NULL) || dwWriteByte != dwWriteDataByte)
		{
			free(pucData);
			pucData = NULL;

			CloseHandle(hWriteFile);
			UnmapViewOfFile(lpReadMem);
			puts("文件写入失败!");
			return -1;
		}

		ullDataSize -= dwWriteDataByte;
	}
	ullDuringOperation = GetTickCount64() - ullStartTime;
	printf("文件写入成功,用时:%lld毫秒(约为%.2lf秒)\n", ullDuringOperation, (long double)ullDuringOperation / (long double)1000.00);

	free(pucData);
	pucData = NULL;

	//成功返回
	CloseHandle(hWriteFile);
	UnmapViewOfFile(lpReadMem);
	return 0;
}