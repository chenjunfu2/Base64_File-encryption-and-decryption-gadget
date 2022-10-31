#include "Base64.hpp"
#include <Windows.h>
#include <stdio.h>

#define UNKNOW 0
#define ENCODE 1
#define DECODE 2
#define CONFIG "Base64_Config.ini"


int main(int argc, char* argv[])
{
	//命令行参数判断
	if (argc != 4)
	{
		puts("请使用:\nBase64.exe [-E|-D] [待转换文件名] [转换结果文件名]\n");
		return -1;
	}

	int Code = UNKNOW;
	if (!lstrcmpiA(argv[1], "-E") || !lstrcmpiA(argv[1], "/E"))//encode
	{
		Code = ENCODE;
	}
	else if (!lstrcmpiA(argv[1], "-D") || !lstrcmpiA(argv[1], "/D"))//decode
	{
		Code = DECODE;
	}
	else if (!lstrcmpiA(argv[1], "-?") || !lstrcmpiA(argv[1], "/?"))
	{
		puts("请使用:\nBase64.exe [-E|-D] [待转换文件名] [转换结果文件名]\n");
	}
	else
	{
		puts("参数错误!\n请使用:\nBase64.exe [-E|-D] [待转换文件名] [转换结果文件名]\n");
	}


	//打开输出文件
	HANDLE hWriteFile = NULL;
	hWriteFile = CreateFileA(argv[3], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hWriteFile == INVALID_HANDLE_VALUE)
	{
		puts("输出文件打开失败!");
		return -1;
	}


	//打开输入文件并映射
	LPVOID lpReadMem = NULL;
	LARGE_INTEGER liFileSize = { 0 };
	{
		 HANDLE hFile = CreateFileA(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			puts("输入文件打开失败!");
			return -1;
		}
			
		if (!GetFileSizeEx(hFile, &liFileSize))//获得文件大小
		{
			CloseHandle(hFile);
			puts("未知错误!");
			return -1;
		}

		HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);//创建文件映射对象
		if (!hFileMapping)
		{
			CloseHandle(hFile);
			puts("未知错误!");
			return -1;
		}
		CloseHandle(hFile);

		lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);//映射文件到内存
		if (!lpReadMem)
		{
			CloseHandle(hFileMapping);
			puts("未知错误!");
			return -1;
		}
		CloseHandle(hFileMapping);
	}

	
	
	//打开配置文件读取配置并初始化类
	Base64 b64;
	{

		//打开配置文件
		HANDLE hConfigFile = CreateFileA(CONFIG, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hConfigFile == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hWriteFile);
			UnmapViewOfFile(lpReadMem);
			puts("配置文件打开失败!");
			return -1;
		}

		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			DWORD dwReadBytes;
			unsigned char cBase64Text[64];

			if (!ReadFile(hConfigFile, cBase64Text, 64, &dwReadBytes, NULL) || dwReadBytes != 64)
			{
				goto read_fail_return;
			}

			b64.SetBaseText(cBase64Text);

			unsigned char cTemp;
			do
			{
				if (!ReadFile(hConfigFile, &cTemp, 1, &dwReadBytes, NULL) || dwReadBytes != 1)
				{
					goto read_fail_return;
				}
			} while (cTemp != '\n');//循环读取直到换行

			//读取到填充字符
			if (!ReadFile(hConfigFile, &cTemp, 1, &dwReadBytes, NULL) || dwReadBytes != 1)
			{
				goto read_fail_return;
			}

			b64.SetFullCode(cTemp);
		}
		else//新创建的文件(不常见情况，后置以提高性能)
		{
			DWORD dwWriteBytes;

			if (!WriteFile(hConfigFile, b64.GetBaseText(), 64, &dwWriteBytes, NULL) || dwWriteBytes != 64)//写入加密串
			{
				goto write_fail_return;
			}

			unsigned char cTemp[3] = { '\r','\n' };//换行
			cTemp[2] = b64.GetFullCode();//填充字符

			if (!WriteFile(hConfigFile, cTemp, 3, &dwWriteBytes, NULL) || dwWriteBytes != 3)//写入换行和填充字符
			{
				goto write_fail_return;
			}
		}

		CloseHandle(hConfigFile);
	}




	//加密解密
	if (Code == ENCODE)
	{
		ULONGLONG ullEnCodeSize = b64.GetEnCodeSize(NULL, liFileSize.QuadPart);
		PUCHAR ucEnCode = (PUCHAR)malloc(ullEnCodeSize);

		//计时器变量
		ULONGLONG ullStartTime = GetTickCount64();
		b64.EnCode(lpReadMem, liFileSize.QuadPart, ucEnCode);
		ULONGLONG ullDuringOperation = GetTickCount64() - ullStartTime;

		printf("加密成功,用时:%lld毫秒(约为%.2lf秒)\n正在写入文件\n\n", ullDuringOperation, (long double)ullDuringOperation / (long double)1000.00);

		DWORD hWriteByte;
		if (!WriteFile(hWriteFile, ucEnCode, ullEnCodeSize, &hWriteByte, NULL) || hWriteByte != ullEnCodeSize)
		{
			free(ucEnCode);
			goto write_fail_return;
		}
		free(ucEnCode);
	}
	else if (Code == DECODE)
	{
		ULONGLONG ullDeCodeSize = b64.GetDeCodeSize((Base64::PCUCHAR)lpReadMem, liFileSize.QuadPart);
		PUCHAR ucDeCode = (PUCHAR)malloc(ullDeCodeSize);

		//计时器变量
		ULONGLONG ullStartTime = GetTickCount64();
		bool bOperationSuccess = b64.DeCode((Base64::PCUCHAR)lpReadMem, liFileSize.QuadPart, ucDeCode);
		ULONGLONG ullDuringOperation = GetTickCount64() - ullStartTime;

		if (!bOperationSuccess)
		{
			CloseHandle(hWriteFile);
			UnmapViewOfFile(lpReadMem);
			puts("文件内容有误,解密失败!");
			return -1;
		}
		
		printf("解密成功,用时:%lld毫秒(约为%.2lf秒)\n正在写入文件\n\n", ullDuringOperation, (long double)ullDuringOperation / (long double)1000.00);

		DWORD hWriteByte;
		if (!WriteFile(hWriteFile, ucDeCode, ullDeCodeSize, &hWriteByte, NULL) || hWriteByte != ullDeCodeSize)
		{
			free(ucDeCode);
			goto write_fail_return;
		}
		free(ucDeCode);
	}

	//结束清理
	CloseHandle(hWriteFile);
	UnmapViewOfFile(lpReadMem);
	puts("文件写入成功!");
	return 0;

	//失败处理
write_fail_return:
	CloseHandle(hWriteFile);
	UnmapViewOfFile(lpReadMem);
	puts("文件写入失败!");
	return -2;
read_fail_return:
	CloseHandle(hWriteFile);
	UnmapViewOfFile(lpReadMem);
	puts("文件读取失败!");
	return -3;
}
