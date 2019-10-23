#include "utility.h"
#include <stdio.h>
#include "Packet.h"

int Networking::StartupWSA(int version, WSADATA& dataToStore)
{
	int startupResult = WSAStartup(static_cast<WORD>(version), &dataToStore);
	if (startupResult != 0)
	{
		printf("[WINSOCK2] WSAStartup failed with code: %d\n", startupResult);
		return 1;
	}
	return 0;
}

int Networking::SetupOverlapEventHandle(OVERLAPPED& overlapToSetup)
{
	SecureZeroMemory((PVOID)& overlapToSetup, sizeof(WSAOVERLAPPED));
	overlapToSetup.hEvent = WSACreateEvent();
	if (overlapToSetup.hEvent == WSA_INVALID_EVENT) {
		printf("[WINSOCK2] WSACreateEvent failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int Networking::SetupSocket(SOCKET& targetSocket, int addressFamily, int sockType, connectionProtocol protocol, DWORD Flags)
{
	targetSocket = WSASocketW(addressFamily, sockType, static_cast<int>(protocol), nullptr, 0, Flags);
	if (targetSocket == INVALID_SOCKET)
	{
		printf("[SETUP] Socket setup failed: %d!\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int Networking::SetSocketBroadcast(SOCKET& targetSocket)
{
	if (setsockopt(targetSocket, SOL_SOCKET, SO_BROADCAST, "1", sizeof(char)) == SOCKET_ERROR)
	{
		printf("[WINSOCK2] Unable to set socket to broadcast: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int Networking::SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue)
{
	if (setsockopt(targetSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)& timeoutValue, sizeof(DWORD)) == SOCKET_ERROR)
	{
		printf("[WINSOCK2] Unable to set receive timeout for this socket: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

int Networking::SetSocketReuseAble(SOCKET& targetSocket, int optionValue)
{
	if (setsockopt(targetSocket, SOL_SOCKET, SO_REUSEADDR, (const char*) &optionValue, sizeof optionValue) == SOCKET_ERROR)
	{
		printf("[WINSOCK2] setsockopt failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

void Networking::SetupSockAddrinfo(SOCKADDR_IN& endpoint, int addressFamily, u_short port, bool broadcast)
{
	endpoint.sin_family = static_cast<ADDRESS_FAMILY>(addressFamily);
	endpoint.sin_port = port;
	if (broadcast) { endpoint.sin_addr.S_un.S_addr = INADDR_BROADCAST; }
	else { endpoint.sin_addr.S_un.S_addr = htonl(INADDR_ANY); }
}

int Networking::BindSocket(SOCKET& targetSocket, SOCKADDR_IN& endpoint)
{
	if (bind(targetSocket, (SOCKADDR*)& endpoint, (int)sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		printf("[WINSOCK2] Unable to bind socket to port %d (%d)!\n", endpoint.sin_port, WSAGetLastError());
		return 1;
	}
	return 0;
}

std::string Networking::GetAddressToString(sockaddr_in* sockaddr)
{
	char s[INET6_ADDRSTRLEN];
	inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, s, sizeof s);
	std::string returnString(s);

	return returnString;
}

int Networking::GetAddressFromString(sockaddr_in* sockaddr, std::string& addressToConvert)
{
	int returnValue = 0;
	if (sockaddr != nullptr)
	{
		inet_pton(sockaddr->sin_family, addressToConvert.c_str(), &returnValue);
	}
	return returnValue;
}

int Networking::GetPortToNumber(sockaddr_in* sockaddr)
{
	int number = 0;
	if (sockaddr != nullptr)
	{
		u_short temp = ntohs(sockaddr->sin_port);
		number = static_cast<int>(temp);
	}
	return number;
}


int Networking::SendPacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& recvSockAdd, Packet& packetToSend, DWORD flags)
{
	int error = 0;
	auto bufferPointer = (char*)packetToSend.GetData();
	WSABUF bufferInfo;
	bufferInfo.len = static_cast<ULONG>(packetToSend.GetDataSize());
	bufferInfo.buf = bufferPointer;
	DWORD bytesSend = 0;

	int result = WSASendTo(targetSocket, &bufferInfo, 1, &bytesSend, flags,
		(SOCKADDR*)& recvSockAdd, sizeof(SOCKADDR_IN), &overlappedHandler, nullptr);

	//Check for sending errors
	if ((result == SOCKET_ERROR) && (WSA_IO_PENDING != (error = WSAGetLastError()))) {
		printf("[WINSOCK2] WSASendTo failed with error: %d\n", error);
		WSACloseEvent(overlappedHandler.hEvent);
		closesocket(targetSocket);
		WSACleanup();
		return 0;
	}

	//Check for event errors
	result = WSAWaitForMultipleEvents(1, &overlappedHandler.hEvent, TRUE, INFINITE, TRUE);
	if (result == WSA_WAIT_FAILED) {
		printf("[WINSOCK2] WSAWaitForMultipleEvents failed with error: %d\n",
			WSAGetLastError());
		return 0;
	}

	result = WSAGetOverlappedResult(targetSocket, &overlappedHandler, &bytesSend, 0, &flags);
	if (result == 0)
	{
		printf("[WINSOCK2] WSASendTo failed with error: %d\n", WSAGetLastError());
		return 0;
	}

	return bytesSend;
}

int Networking::ReceivePacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& senderSockAdd, Packet& packetToFill, DWORD flags)
{
	int error = 0;
	char receiveBuffer[defaultBufferSize];
	WSABUF bufferInfo{ defaultBufferSize, receiveBuffer };
	int socksize = sizeof(sockaddr_in);
	DWORD bytesReceived = 0;

	int receiveCount = WSARecvFrom(targetSocket, &bufferInfo, 1, &bytesReceived, &flags, (SOCKADDR*)& senderSockAdd, &socksize, &overlappedHandler, nullptr);

	if (receiveCount != 0)
	{
		error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			wprintf(L"[WINSOCK2] WSARecvFrom failed with error: %ld\n", error);
			WSACloseEvent(overlappedHandler.hEvent);
			closesocket(targetSocket);
			WSACleanup();
			return 0;
		}
		receiveCount = WSAWaitForMultipleEvents(1, &overlappedHandler.hEvent, TRUE, INFINITE, TRUE);
		if (receiveCount == WSA_WAIT_FAILED) {
			wprintf(L"[WINSOCK2] WSAWaitForMultipleEvents failed with error: %d\n", WSAGetLastError());
			return 0;
		}
		receiveCount = WSAGetOverlappedResult(targetSocket, &overlappedHandler, &bytesReceived, FALSE, &flags);
		if (receiveCount == 0)
		{
			wprintf(L"[WINSOCK2] WSArecvFrom failed with error: %d\n", WSAGetLastError());
			return 0;
		}
		auto data = static_cast<char*>(malloc(bytesReceived));
		std::copy_n(&receiveBuffer[0], bytesReceived, data);
		packetToFill.Append(data, bytesReceived);
	}
	return bytesReceived;
}
