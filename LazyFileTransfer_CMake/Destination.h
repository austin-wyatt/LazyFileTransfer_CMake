#pragma once

#include <iostream>
#include <string.h>
#include "public.h"
#include <vector>
#include <fstream>
#include <filesystem>

using namespace std;

class Destination
{
public:
	Destination()
	{
		ZeroMemory(_readBuffer, _readBufferLen);
	}

	Destination(SOCKET socket)
	{
		TransferSocket = socket;

		ZeroMemory(_readBuffer, _readBufferLen);
	}

	SOCKET TransferSocket = INVALID_SOCKET;

	SOCKET ConnectedSocket = INVALID_SOCKET;
	sockaddr_in ConnectedSocketAddr;

	void Listen();

private:
	//Reads bulk data transfers and routes them where they need to go
	void startDataLoop();

	//Takes the current byte index into the read buffer and extracts the passed file data info
	void processFileDataInfo(int byteIndex, int totalBytesRecieved);

	/*Create a file at the path specified by FilePath for each file info objectand append an amount
	of bytes specified by FileSize read from the connected socket into the created file*/
	void processFileData(vector<FileInfo*>* fileInfo);

	//Replace any occurrences of WRONG_DELIMITER with FILE_DELIMITER
	void resolveFilePathInconsistencies(FileInfo* fileInfo);

	static const int _readBufferLen = 64 * 1024;
	char _readBuffer[_readBufferLen];
};
