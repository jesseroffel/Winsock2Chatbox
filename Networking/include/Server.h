#pragma once

#include "utility.h"
#include <stdio.h>


namespace networking
{
	
	class Server
	{
	public:
		Server() {};
		~Server() = default;

		int Initialize();
		int SetupAddrInfo(addrinfo addrInfoHints, PCSTR port);
		//void SetSocketStructure(sockaddr_in& targetSockAddr, std::string& ipAddress, u_short port, bool localhost);
		//int SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue);
		//int BindSocket(SOCKET& targetSocket, sockaddr& socketSettings);


	private:
		SOCKET SsdpSocket = INVALID_SOCKET;		// to search
		SOCKET listenSocket = INVALID_SOCKET;	// to chat

		addrinfo* addrresult = nullptr;

		addrinfo addrinfoSettings;
		sockaddr_in sockaddrSettings;
		
		hostent* localhost = nullptr;
		PCSTR servicePort = "27015";
	};
}