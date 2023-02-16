#include "Destination.h"

void Destination::Listen()
{
	//only allow one connection
	listen(this->TransferSocket, 1);

	sockaddr_in sourceAddr;
	//int addrSize = sizeof(sourceAddr);
	socklen_t addrSize = sizeof(sourceAddr);

	while (true)
	{
		std::cout << "Listening on port " << TRANSFER_PORT_NUM << std::endl;

		this->ConnectedSocket = accept(this->TransferSocket, (sockaddr*)&sourceAddr, &addrSize);
		this->ConnectedSocketAddr = sourceAddr;

		if (this->ConnectedSocket != INVALID_SOCKET)
		{
			std::cout << "Connection opened" << std::endl;
		}
		else
		{
			std::cout << "Connection failed" << std::endl;
		}

		startDataLoop();
	}
}

void Destination::startDataLoop()
{
	int byteIndex = 0;
	int bytesReceived = 0;

	int totalBytesReceived = 0;
	long long controlData = 0;

	bool reading = false;

	int packetSize = 0;

	while (this->ConnectedSocket != INVALID_SOCKET)
	{
		bytesReceived = recv(this->ConnectedSocket, _readBuffer + totalBytesReceived, _readBufferLen - totalBytesReceived, 0);

		totalBytesReceived += bytesReceived;

		//0 bytes recieved or the error value, -1, indicates a closed connection
		if (bytesReceived <= 0)
		{
			std::cout << "Connection closed" << std::endl;
			closesocket(this->ConnectedSocket);
			this->ConnectedSocket = INVALID_SOCKET;
			return;
		}
		else if (!reading && totalBytesReceived >= (sizeof(int) + sizeof(long long)))
		{
			packetSize = *(int*)(_readBuffer + byteIndex);
			byteIndex += sizeof(int);

			controlData = *(long long*)(_readBuffer + byteIndex);

			if (controlData == TRANSFER_BEGIN)
			{
				reading = true;
				byteIndex += sizeof(long long);
			}
			else
			{
				std::cout << "Data transfer must begin with TRANSFER_BEGIN header" << std::endl;
				closesocket(this->ConnectedSocket);
				this->ConnectedSocket = INVALID_SOCKET;
				return;
			}
		}

		if (totalBytesReceived >= sizeof(long long))
		{
			controlData = *(long long*)(_readBuffer + totalBytesReceived - sizeof(long long));

			if (controlData == TRANSFER_END)
			{
				reading = false;
				totalBytesReceived -= sizeof(long long) + sizeof(int);
			}
		}

		//This socket will only accept an initial data transfer if it begins with a 
		//TRANSFER_BEGIN code and ends with a TRANSFER_END code. (Note that all data
		//must be prefixed with an integer containing the size of the following packet)
		//
		//Here we want to loop until recieving both the TRANSFER_BEGIN and TRANSFER_END call
		//so that the entire read is in the read buffer and easy to access.
		//(Note that an initial transmission of more than 64KB would overflow the read buffer)
		if (reading || totalBytesReceived < (sizeof(int) + sizeof(long long)))
			continue;


		while (byteIndex < totalBytesReceived)
		{
			packetSize = *(int*)(_readBuffer + byteIndex);
			byteIndex += sizeof(int);

			//If the client ever indicates a packet of size 0 or less, end the connection
			if (packetSize <= 0)
			{
				std::cout << "Connection closed" << std::endl;
				closesocket(this->ConnectedSocket);
				this->ConnectedSocket = INVALID_SOCKET;
			}
			else
			{
				long long data = *(long long*)(_readBuffer + byteIndex);

				//More options could be included here but for now only file transfer is supported.
				//If file transfer is not selected the data will simply be printed to the console.
				if (data == FILE_INFO_HEADER)
				{
					processFileDataInfo(byteIndex + packetSize, totalBytesReceived);
					std::cout << "File data processed" << endl;

					totalBytesReceived = 0;
					byteIndex = 0;
				}
				else
				{
					std::cout << "data: " << _readBuffer + byteIndex << std::endl;
				}
			}

			byteIndex += packetSize;
		}
	}
}

void Destination::processFileDataInfo(int byteIndex, int totalbytesReceived)
{
	vector<FileInfo*> filesToProcess = vector<FileInfo*>();

	FileInfo* currFileInfo = nullptr;

	long long currentSection = 0;

	int newLength;
	char* tempCharBuf;

	int packetSize;

	//Process the contents of the read buffer that we gathered earlier.
	while (byteIndex < totalbytesReceived)
	{
		packetSize = *(int*)(_readBuffer + byteIndex);
		//The byteIndex indicates the offset into the byte array that is the read buffer.
		//Since this is a simple application, a small 4 byte header containing the size of the
		//following packet is all that is necessary. For a more complicated application, a larger
		//header containing more features such as packet type and optional flag values could be used.
		byteIndex += sizeof(int);

		if (packetSize == sizeof(long long))
		{
			currentSection = *(long long*)(_readBuffer + byteIndex);

			switch (currentSection)
			{
			case FILE_INFO_FILE_BEGIN:
				currFileInfo = new FileInfo();
				break;
			case FILE_INFO_FILE_COMPLETE:
				resolveFilePathInconsistencies(currFileInfo);

				filesToProcess.push_back(currFileInfo);
				break;
			case FILE_INFO_HEADER_COMPLETE:
				processFileData(&filesToProcess);
				return;
			}
		}
		else
		{
			switch (currentSection)
			{
			case FILE_INFO_FILEPATH:
				//Create a new string large enough to hold the passed information
				newLength = currFileInfo->FilePathLen + packetSize;

				//Ensure we have the space to include the null terminator
				tempCharBuf = new char[newLength + 1];

				if (currFileInfo->FilePathLen != 0)
				{
					//Account for the case that the file path needed to be split into two packets for
					//whatever reason that may have occurred
					memcpy(tempCharBuf, currFileInfo->FilePath, currFileInfo->FilePathLen);
				}

				memcpy(tempCharBuf + currFileInfo->FilePathLen, (_readBuffer + byteIndex), packetSize);

				delete[] currFileInfo->FilePath;

				currFileInfo->FilePath = tempCharBuf;
				currFileInfo->FilePathLen = newLength;
				tempCharBuf[newLength] = '\0';
				break;
			}
		}

		byteIndex += packetSize;
	}
}

void Destination::processFileData(vector<FileInfo*>* fileInfo)
{
	long long data = DEST_REQ_DATA;
	//Let the source know that we are prepared to accept file data
	send(this->ConnectedSocket, (char*)&data, sizeof(long long), 0);

	FileInfo* curr;

	ofstream file;

	int currentBytesReceived = 0;
	int fileBytesRecieved = 0;
	int readBufferOffset = 0;

	int packetSize = 0;

	int totalBytesRecieved = 0;

	for (int i = 0; i < fileInfo->size(); i++)
	{
		curr = fileInfo->at(i);

		string* fullFilePath = new string(curr->FilePath);
		int delimOffset = fullFilePath->find_last_of(FILE_DELIMTER);

		if (delimOffset > 0)
		{
			filesystem::path tempPath = filesystem::path(fullFilePath->substr(0, delimOffset));

			//Check if the directory we are trying to place the file in doesn't exist.
			//If it doesn't, attempt to create each directory in the tree 
			//(CreateDirectoryA will fail if the directory already exists)
			if (!filesystem::is_directory(tempPath))
			{
				delimOffset = 0;
				while (delimOffset != -1)
				{
					if (delimOffset != 0)
					{
#ifdef _WIN32
						CreateDirectoryA(fullFilePath->substr(0, delimOffset).c_str(), NULL);
#elif __linux__
						mkdir(fullFilePath->substr(0, delimOffset).c_str(), S_IRWXU | S_IRWXO);
#endif
					}

					delimOffset = fullFilePath->find(FILE_DELIMTER, delimOffset + 1);
				}
			}
		}

		delete fullFilePath;



		file.open(curr->FilePath, fstream::binary | fstream::trunc | fstream::in);

		long long controlVal = 0;

		bool success = true;

		while (controlVal != FILE_INFO_EOF)
		{
			int bytesReceived = recv(this->ConnectedSocket, _readBuffer + currentBytesReceived, _readBufferLen - currentBytesReceived, 0);

			currentBytesReceived += bytesReceived;

			totalBytesRecieved += bytesReceived;

			//Act on bytes in the buffer and ensure that we have more than just a packet header to work with
			while (readBufferOffset < currentBytesReceived && ((currentBytesReceived - readBufferOffset) > sizeof(int)))
			{
				packetSize = *(int*)(_readBuffer + readBufferOffset);
				readBufferOffset += sizeof(int);

				if (packetSize == sizeof(long long))
				{
					//Take action if our packet size is a 8 bytes and matches one of the constant control values.
					//This does introduce an astronomically rare case where the file data being sent has 8 bytes remaining
					//and coincidentally happens to match one of these control values. The solution to this would of course 
					//be a larger header with some delineation for control value vs data value. Instead of doing that though
					//I will say that it is rare enough that I'm not worried about it.
					controlVal = *(long long*)(_readBuffer + readBufferOffset);
					if (controlVal == FILE_INFO_EOF)
					{
						readBufferOffset += sizeof(long long);
						break;
					}
					else if (controlVal == FILE_INFO_FILE_CANCELED)
					{
						readBufferOffset += sizeof(long long);
						success = false;
						controlVal = FILE_INFO_EOF;
						break;
					}
				}

				if (currentBytesReceived - readBufferOffset < packetSize)
				{
					//If we didn't recieve the full packet, go back and read from the socket again.
					//To that end, ensure we reinclude the packet header we just read to avoid reading from
					//the packet itself on the next pass and receiving junk data.
					readBufferOffset -= sizeof(int);
					break;
				}
				else
				{
					//Write the packet to the file and increment the buffer offset if all is well.
					file.write(_readBuffer + readBufferOffset, packetSize);
					readBufferOffset += packetSize;
				}
			}

			if (readBufferOffset > 0)
			{
				//Shift the buffer offset back to 0 if we have data in our buffer after attempting a write.
				//This clears up room for more incoming packets.
				currentBytesReceived -= readBufferOffset;
				//memmove is preferred over memcpy here due to the source and destination potentially being overlapping memory.
				memmove(_readBuffer, _readBuffer + readBufferOffset, currentBytesReceived);
				readBufferOffset = 0;
			}
		}

		if (currentBytesReceived > 0)
		{
			//Go ahead and do a final memory move after finishing the file to make sure our 
			//data starts at buffer position 0
			currentBytesReceived -= readBufferOffset;
			memmove(_readBuffer, _readBuffer + readBufferOffset, currentBytesReceived);
		}
		else
		{
			currentBytesReceived = 0;
		}

		readBufferOffset = 0;

		if (success)
		{
			cout << "File " << curr->FilePath << " copied successfully" << endl;
		}
		else
		{
			cout << "File " << curr->FilePath << " copy unsuccessful" << endl;
		}

		file.close();

		//Clean up the FileInfo object
		delete curr;
	}
}

void Destination::resolveFilePathInconsistencies(FileInfo* fileInfo)
{
	for (int i = 0; i < fileInfo->FilePathLen; i++) 
	{
		if (fileInfo->FilePath[i] == WRONG_DELIMITER)
			fileInfo->FilePath[i] = FILE_DELIMTER;
	}
}