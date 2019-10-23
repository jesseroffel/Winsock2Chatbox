#include "Server.h"

int networking::Server::Initialize()
{
	WSADATA wsaData;
	int iResult = 0;

	//Initialize Winsock2
	iResult = WSAStartup(0x202, &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with code: %d\n", iResult);
		return iResult;
	}

	if (networking::SetupAddrInfo(addrinfoSettings))
	{
		printf("Unable to get addrinfo");
		WSACleanup();
		return 1;
	}

	if (networking::SetupSocket(SsdpSocket, connectionProtocol::UDP))
	{
		printf("Unable to create sockets!");
		WSACleanup();
		return 1;
	}

	if (networking::SetSocketTimeout(SsdpSocket, 0x1))
	{
		closesocket(SsdpSocket);
		WSACleanup();
		return 1;
	}

	if (networking::BindSocket(SsdpSocket, 20175, true))
	{
		closesocket(SsdpSocket);
		WSACleanup();
		return 1;
	}

	printf("Server setup completed, waiting for messages...");
	return 0;
}

int networking::Server::SetupAddrInfo(addrinfo addrInfoHints, PCSTR port)
{
	ZeroMemory(&addrInfoHints, sizeof(addrInfoHints));
	addrInfoHints.ai_family = AF_UNSPEC;
	addrInfoHints.ai_socktype = SOCK_STREAM;
	addrInfoHints.ai_protocol = IPPROTO_UDP;
	addrInfoHints.ai_flags = AI_PASSIVE;

	int iResult = getaddrinfo(nullptr, port, &addrInfoHints, &addrresult);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	return 0;
}
//
//void networking::Server::SetSocketStructure(sockaddr_in& targetSockAddr, std::string& ipAddress, u_short port, bool localhost)
//{
//	targetSockAddr.sin_family = AF_INET;		// IPV4
//}
//
//int networking::Server::SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue)
//{
//	return 0;
//}
//
//int networking::Server::BindSocket(SOCKET& targetSocket, sockaddr& socketSettings)
//{
//	return 0;
//}
