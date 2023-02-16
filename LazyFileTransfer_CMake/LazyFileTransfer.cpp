#include <iostream>
#include <string.h>
#include "Destination.h"
#include "Source.h"
#include "public.h"
#include <thread>

using namespace std;

bool ResolveConnection(char** argv);

enum class TransferMode
{
    Source,
    Destination
};

TransferMode transferMode;

SOCKET transferSocket;

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        cout << "No parameters provided (-source [source info] or -destination)" << endl;
        return 1;
    }

    if (strcmp(argv[1], "-source") == 0 && argc >= 5)
    {
        transferMode = TransferMode::Source;
    }
    else if (strcmp(argv[1], "-destination") == 0 && argc == 3)
    {
        transferMode = TransferMode::Destination;
    }
    else
    {
        cout << "Incorrect parameters provided (-source [dest ip address] [fileSource] [fileDest] ... or -destination [dest ip address])" << endl;
        return 1;
    }

#ifdef _WIN32
    //initialize winsock
    WSADATA wsaData;

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"Error at WSAStartup()\n");
        WSACleanup();
        return 1;
    }
#endif

    transferSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ResolveConnection(argv))
    {
        if (transferMode == TransferMode::Destination)
        {
            Destination* dest = new Destination(transferSocket);

            dest->Listen();

            delete dest;
        }
        else if (transferMode == TransferMode::Source)
        {
            Source* source = new Source(transferSocket);

            for (int i = 3; i < argc - 1; i += 2)
            {
                source->AddFile(argv[i], argv[i + 1]);
            }

            source->SendFiles();

            //end connection 
            source->CloseConnection();
            delete source;
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

bool ResolveConnection(char** argv)
{
    sockaddr_in  addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TRANSFER_PORT_NUM);

    int result;

    switch (transferMode)
    {
    case TransferMode::Destination:
        inet_pton(AF_INET, argv[2], &addr.sin_addr.s_addr);

        result = bind(transferSocket, (SOCKADDR*)&addr, sizeof(addr));

        if (result == SOCKET_ERROR)
        {
            cout << "Socket connection failed" << endl;
            closesocket(transferSocket);
            return false;
        }

        cout << "Socket bound to port " << TRANSFER_PORT_NUM << endl;

        break;
    case TransferMode::Source:
        inet_pton(AF_INET, argv[2], &addr.sin_addr.s_addr);

        result = connect(transferSocket, (SOCKADDR*)&addr, sizeof(addr));
        if (result == SOCKET_ERROR)
        {
            cout << "Socket connection to destination " << argv[2] << " failed" << endl;
            closesocket(transferSocket);
            return false;
        }

        cout << "Socket connected to destination " << argv[2] << ":" << TRANSFER_PORT_NUM << endl;

        break;
    }


    return true;
}