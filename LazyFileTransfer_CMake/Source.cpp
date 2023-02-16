#include "Source.h"

void Source::SendData(void* data, int len)
{
	send(this->ConnectedDest, (char*)&len, sizeof(int), 0);
	send(this->ConnectedDest, (char*)data, len, 0);
}

void Source::BeginDataTransfer()
{
	fillIOBuffer(TRANSFER_BEGIN);
	sendIOBuffer();
}

void Source::EndDataTransfer()
{
	fillIOBuffer(TRANSFER_END);
	sendIOBuffer();
}

void Source::CloseConnection()
{
	if (this->ConnectedDest != INVALID_SOCKET)
	{
		SendData(nullptr, 0);
		closesocket(this->ConnectedDest);
		this->ConnectedDest = INVALID_SOCKET;
	}
}


void Source::SendFiles()
{
	sendFileInfo();
}

void Source::sendFileInfo()
{
	BeginDataTransfer();

	long long data;

	fillIOBuffer(FILE_INFO_HEADER);
	sendIOBuffer();

	for (int i = 0; i < FilesToCopy.size(); i++)
	{
		fillIOBuffer(FILE_INFO_FILE_BEGIN);
		sendIOBuffer();

		fillIOBuffer(FILE_INFO_FILEPATH);
		sendIOBuffer();

		fillIOBuffer(FILE_INFO_FILEPATH);
		sendIOBuffer();

		SendData(FilesToCopy.at(i)->DestFilePath, FilesToCopy.at(i)->DestFilePathLen);

		fillIOBuffer(FILE_INFO_FILE_COMPLETE);
		sendIOBuffer();
	}

	fillIOBuffer(FILE_INFO_HEADER_COMPLETE);
	sendIOBuffer();

	EndDataTransfer();

	sendFileData();
}

void Source::sendFileData()
{
	int bytesReceived = recv(this->ConnectedDest, _ioBuffer, _ioBufferLen, 0);

	ifstream file;
	streampos size;
	streampos currPos;

	long long data = 0;

	const int BLOCK_SIZE = 4 * 1024;

	if (bytesReceived == sizeof(long long) && *(long long*)_ioBuffer == DEST_REQ_DATA)
	{
		SourceFileInfo* currFile;

		for (int i = 0; i < FilesToCopy.size(); i++)
		{
			currFile = FilesToCopy.at(i);

			file.open(currFile->SourceFilePath, fstream::binary | fstream::out | fstream::ate);

			size = file.tellg();
			currPos = 0;

			file.seekg(0);

			int bytesToRead = 0;

			if (file.is_open())
			{
				while (currPos < size)
				{
					bytesToRead = size - currPos;
					bytesToRead = bytesToRead > BLOCK_SIZE ? BLOCK_SIZE : bytesToRead;

					file.read(_ioBuffer + 4, bytesToRead);
					*(int*)(_ioBuffer) = bytesToRead;
					sendIOBuffer();

					currPos += bytesToRead;
				}

				fillIOBuffer(FILE_INFO_EOF);
				sendIOBuffer();
				file.close();

				cout << "File " << currFile->SourceFilePath << " sent" << endl;
			}
			else
			{
				cout << "File " << currFile->SourceFilePath << " could not be opened" << endl;

				fillIOBuffer(FILE_INFO_FILE_CANCELED);
				sendIOBuffer();
			}

			delete currFile;
		}
	}
}

void Source::AddFile(char* sourcePath, char* destinationPath)
{
	filesystem::path* tempPath = new filesystem::path(sourcePath);

	if (filesystem::is_directory(*tempPath))
	{
		auto directoryIterator = filesystem::directory_iterator(*tempPath);

		//If our source path is a directory, iterate through each entry of that directory and 
		//recursively call AddFile on it. 
		for (auto const& dir_entry : directoryIterator)
		{
			//entryPath is stack allocated but this shouldn't be an issue as we reduce its memory footprint 
			//substantially before recursing. When doing this we also pass a new destinationPath to preserve
			//the file structure.
			string entryPath = dir_entry.path().string();

			//Create the source path C string for this entry. This memory will be cleared when the SourceFileInfo
			//object is deleted after sending the file information to the destination socket.
			char* newSourcePath = new char[entryPath.size() + 1];
			memcpy(newSourcePath, entryPath.c_str(), entryPath.size());
			newSourcePath[entryPath.size()] = '\0';

			//Build the new destination path string to preserve folder structure
			string* newDestPathStr = new string(destinationPath);
			entryPath = entryPath.substr(entryPath.find_last_of(FILE_DELIMTER));
			newDestPathStr->append(entryPath);

			//Create the destination path C string for this entry.
			char* newDestPath = new char[newDestPathStr->size() + 1];
			memcpy(newDestPath, newDestPathStr->c_str(), newDestPathStr->size());
			newDestPath[newDestPathStr->size()] = '\0';

			delete newDestPathStr;

			//Recursively call AddFile to either add this file to the list of files to send or 
			//iterate its directory contents
			AddFile(newSourcePath, newDestPath);
		}
	}
	else
	{
		SourceFileInfo* info = new SourceFileInfo();

		info->SourceFilePath = sourcePath;
		info->DestFilePath = destinationPath;

		int i = 0;
		for (i = 0; *(destinationPath + i) != '\0'; i++)
		{
			info->DestFilePathLen = i + 1;
		}

		FilesToCopy.push_back(info);
	}

	delete tempPath;
}
