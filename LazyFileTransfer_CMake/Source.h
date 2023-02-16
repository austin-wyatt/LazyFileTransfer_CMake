#pragma once

#include <iostream>

#include <string.h>
#include "public.h"
#include <vector>
#include <fstream>
#include <filesystem>

#define _sendData(data) &data, sizeof(data)

using namespace std;


struct SourceFileInfo
{
	char* DestFilePath;
	int DestFilePathLen;

	char* SourceFilePath;
};

class Source
{
public:
	Source() {}

	Source(SOCKET destSocket)
	{
		ConnectedDest = destSocket;
	}

	SOCKET ConnectedDest;

	vector<SourceFileInfo*> FilesToCopy = vector<SourceFileInfo*>();

	void SendData(void* data, int len);
	void BeginDataTransfer();
	void EndDataTransfer();

	void CloseConnection();

	void AddFile(char* sourcePath, char* destinationPath);
	void SendFiles();

private:
	void sendFileInfo();
	void sendFileData();

	void sendIOBuffer()
	{
		send(this->ConnectedDest, _ioBuffer, *(int*)(_ioBuffer)+sizeof(int), 0);
	}

	template <typename T>
	void fillIOBuffer(T data)
	{
		*(int*)(_ioBuffer) = sizeof(data);
		*(T*)(_ioBuffer + sizeof(int)) = data;
	}

	static const int _ioBufferLen = 8 * 1024;
	char _ioBuffer[_ioBufferLen];
};
