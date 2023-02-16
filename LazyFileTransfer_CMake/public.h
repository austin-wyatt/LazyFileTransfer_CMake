#pragma once

const int TRANSFER_PORT_NUM = 27500;

const long long DEST_REQ_DATA = 0x25;

const long long TRANSFER_BEGIN = 0xFFFFFFFFFFFF0;
const long long TRANSFER_END = 0xFFFFFFFFFFFF1;

const long long FILE_INFO_HEADER = 0xFFFFFFFF0;
const long long FILE_INFO_HEADER_COMPLETE = 0xFFFFFFFFF;

const long long FILE_INFO_FILE_BEGIN = 0xFFFFFFFF1;
const long long FILE_INFO_FILEPATH = 0xFFFFFFFF2;
const long long FILE_INFO_FILE_LENGTH = 0xFFFFFFFF3;
const long long FILE_INFO_FILE_COMPLETE = 0xFFFFFFFF4;

const long long FILE_INFO_FILE_CANCELED = 0xFFFFFFFFD;
const long long FILE_INFO_EOF = 0xFFFFFFFFE;


#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#define FILE_DELIMTER '\\'
#define WRONG_DELIMITER '/'
#endif

#ifdef __linux__
#define FILE_DELIMTER '/'
#define WRONG_DELIMITER '\\'

#define SOCKET int
#define SOCKADDR sockaddr
#define closesocket close

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))
#define ZeroMemory RtlZeroMemory
#endif


class FileInfo
{
public:

	~FileInfo()
	{
		delete[] FilePath;
	}

	char* FilePath = nullptr;
	int FilePathLen = 0;
};
