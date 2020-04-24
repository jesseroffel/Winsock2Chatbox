#include <stdio.h>
#include "utility.h"
#include <ws2tcpip.h>
#include "Packet.h"
#include "Responses.h"



namespace Networking
{
	bool StartupWSA(int version, WSADATA& dataToStore)
	{
		int startupResult = WSAStartup(static_cast<WORD>(version), &dataToStore);
		if (startupResult != 0)
		{
			printf("[System] WSAStartup failed with code: %d\n", startupResult);
			return false;
		}
		return true;
	}

	bool SetupOverlapEventHandle(OVERLAPPED& overlapToSetup)
	{
		SecureZeroMemory((PVOID)&overlapToSetup, sizeof(WSAOVERLAPPED));
		overlapToSetup.hEvent = WSACreateEvent();
		if (overlapToSetup.hEvent == WSA_INVALID_EVENT) {
			printf("[System] WSACreateEvent failed with error: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SetupSocket(SOCKET& targetSocket, int addressFamily, int sockType, ConnectionProtocolUsed protocol, DWORD Flags)
	{
		targetSocket = WSASocketW(addressFamily, sockType, static_cast<int>(protocol), nullptr, 0, Flags);
		if (targetSocket == INVALID_SOCKET)
		{
			printf("[System] Socket setup failed: %d!\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SetSocketBroadcast(SOCKET& targetSocket, bool value)
	{
		const char* mode = value ? "1" : "0";
		if (setsockopt(targetSocket, SOL_SOCKET, SO_BROADCAST, mode, sizeof(char)) == SOCKET_ERROR)
		{
			printf("[System] Unable to set socket to broadcast: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue)
	{
		if (setsockopt(targetSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutValue, sizeof(DWORD)) == SOCKET_ERROR)
		{
			printf("[System] Unable to set receive timeout for this socket: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SetSocketReuseAble(SOCKET& targetSocket, int optionValue)
	{
		if (setsockopt(targetSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optionValue, sizeof optionValue) == SOCKET_ERROR)
		{
			printf("[System] setsockopt failed with error: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	void SetupSockAddrinfo(SOCKADDR_IN& endpoint, int addressFamily, std::string ipaddress, u_short port, bool broadcast)
	{
		endpoint.sin_family = static_cast<ADDRESS_FAMILY>(addressFamily);
		endpoint.sin_port = htons(port);
		//hostent* thisHost = gethostbyname("");
		//char* ip = inet_ntoa(*(struct in_addr*) * thisHost->h_addr_list);
		//endpoint.sin_addr.s_addr = inet_addr(ip);
		if (ipaddress.empty())
		{
			if (broadcast) { endpoint.sin_addr.S_un.S_addr = INADDR_BROADCAST; }
			else { endpoint.sin_addr.S_un.S_addr = htonl(INADDR_ANY); }
		}
		else
		{
			int convertedIp = 0;
			;
			if (inet_pton(endpoint.sin_family, ipaddress.c_str(), &convertedIp) != 1)
			{
				printf("[System] Unable to convert %d !\n", WSAGetLastError());
			}
			else
			{
				endpoint.sin_addr.s_addr = convertedIp;
			}
			
		}
	}

	bool BindSocket(SOCKET& targetSocket, SOCKADDR_IN& endpoint)
	{
		if (bind(targetSocket, (SOCKADDR*)&endpoint, (int)sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		{
			printf("[System] Unable to bind socket %d!\n", WSAGetLastError());
			CloseSocket(targetSocket);
			return false;
		}
		return true;
	}

	bool ListenToSocket(SOCKET& targetSocket, int amountConnectionsAccepted)
	{
		if (listen(targetSocket, amountConnectionsAccepted) == SOCKET_ERROR)
		{
			printf("[System] Unable to listen to socket, error: %d\n",  WSAGetLastError());
			return false;
		}
		return true;
	}

	bool ConnectToSocket(SOCKET& targetSocket, sockaddr_in& connectToAddress)
	{
		if (WSAConnect(targetSocket, (SOCKADDR*)&connectToAddress, sizeof(connectToAddress), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR)
		{
			printf("[System] Unable to connect to socket %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool AcceptSocket(SOCKET& listenSocket, sockaddr_in& endpoint, int sizeOfEndpoint)
	{
		listenSocket = WSAAccept(listenSocket, (SOCKADDR*)&endpoint, &sizeOfEndpoint, AcceptOrRejectIncomingConnection, 0);
		if (listenSocket == INVALID_SOCKET)
		{
			CloseSocket(listenSocket);
			return false;
		}
		return true;
	}

	bool CloseSocket(SOCKET& targetSocket)
	{
		if (closesocket(targetSocket) == SOCKET_ERROR)
		{
			printf("[System] Failed to close socket, error: %d!\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	std::string GetAddressToString(sockaddr_in* sockaddr)
	{
		char s[INET6_ADDRSTRLEN];
		inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, s, sizeof s);
		std::string returnString(s);

		return returnString;
	}

	int GetAddressFromString(sockaddr_in* sockaddr, std::string& addressToConvert)
	{
		int returnValue = 0;
		if (sockaddr != nullptr)
		{
			inet_pton(sockaddr->sin_family, addressToConvert.c_str(), &returnValue);
		}
		return returnValue;
	}

	int GetPortToNumber(sockaddr_in* sockaddr)
	{
		int number = 0;
		if (sockaddr != nullptr)
		{
			u_short temp = ntohs(sockaddr->sin_port);
			number = static_cast<int>(temp);
		}
		return number;
	}


	std::string AttemptToFetchLocalNetworkAddress(SOCKET& targetSocket, sockaddr_in& connectToAddress)
	{
		if (WSAConnect(targetSocket, (SOCKADDR*)&connectToAddress, sizeof(connectToAddress), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR)
		{
			printf("[System] Unable to connect to socket %d\n", WSAGetLastError());
			return std::string{};
		}


		sockaddr_in localIpAddr;
		socklen_t addrlen = sizeof(localIpAddr);
		if (getsockname(targetSocket, reinterpret_cast<SOCKADDR*>(&localIpAddr), &addrlen) == -1) {
			CloseSocket(targetSocket);
			return std::string{};
		}

		char buf[INET_ADDRSTRLEN];
		std::string ip{};
		if (inet_ntop(AF_INET, &localIpAddr.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) {
			CloseSocket(targetSocket);
			return std::string{};
		}

		CloseSocket(targetSocket);
		return std::string{buf};
	}

	ChatState SendPacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& recvSockAdd, Packet& packetToSend, DWORD flags)
	{
		int error = 0;
		auto bufferPointer = (char*)packetToSend.GetData();
		WSABUF bufferInfo;
		bufferInfo.len = static_cast<ULONG>(packetToSend.GetDataSize());
		bufferInfo.buf = bufferPointer;
		DWORD bytesSend = 0;

		int result = WSASendTo(targetSocket, &bufferInfo, 1, &bytesSend, flags,
			(SOCKADDR*)&recvSockAdd, sizeof(SOCKADDR_IN), &overlappedHandler, nullptr);

		//Check for sending errors
		if ((result == SOCKET_ERROR) && (WSA_IO_PENDING != (error = WSAGetLastError()))) {
			printf("[System] WSASendTo failed with error: %d\n", error);
			return ChatState::WSASendToFailed;
		}

		//Check for event errors
		result = WSAWaitForMultipleEvents(1, &overlappedHandler.hEvent, TRUE, INFINITE, TRUE);
		if (result == WSA_WAIT_FAILED) {
			printf("[System] WSAWaitForMultipleEvents failed with error: %d\n", WSAGetLastError());
			return ChatState::WSAWaitForEventsFailed;
		}

		//Reset event
		result = WSAResetEvent(overlappedHandler.hEvent);
		if (result == 0)
		{
			wprintf(L"[System] WSAResetEvent failed with error = %d\n", WSAGetLastError());
		}

		result = WSAGetOverlappedResult(targetSocket, &overlappedHandler, &bytesSend, 0, &flags);
		if (result == 0)
		{
			printf("[System] WSASendTo failed with error: %d\n", WSAGetLastError());
			return ChatState::WSAOverlappedResultFailed;
		}

		return ChatState::WSASendPacketSuccess;
	}

	ChatState ReceivePacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, Responses& responseHandler, sockaddr_in& recvSockAdd, DWORD responseTimeOut, DWORD flags)
	{
		int error = 0;
		char receiveBuffer[defaultBufferSize];
		WSABUF bufferInfo{ defaultBufferSize, receiveBuffer };
		DWORD bytesReceived = 0;
		int socksize = sizeof(sockaddr_in);

		if (responseTimeOut == 0) {
			responseTimeOut = WSA_INFINITE;
		}

		int result = WSARecvFrom(
			targetSocket, &bufferInfo, 1, &bytesReceived, &flags,
			(SOCKADDR*)&recvSockAdd, &socksize, &overlappedHandler, nullptr
		);

		if ((result == SOCKET_ERROR) && (WSA_IO_PENDING != (error = WSAGetLastError()))) {
			wprintf(L"[System] WSARecvFrom failed with error: %ld\n", error);
			return ChatState::WSAReceiveFromFailed;
		}

		//Wait for overlapped I/O
		result = WSAWaitForMultipleEvents(1, &overlappedHandler.hEvent, FALSE, responseTimeOut, FALSE);
		if (result == WSA_WAIT_FAILED) {
			wprintf(L"[System] WSAWaitForMultipleEvents failed with error: %d\n", WSAGetLastError());
			return ChatState::WSAWaitForEventsFailed;
		}

		//Timeout hit
		if (result == WSA_WAIT_TIMEOUT)
		{
			return ChatState::WSAReceiveTimeout;
		}

		//Reset event
		result = WSAResetEvent(overlappedHandler.hEvent);
		if (result == 0)
		{
			wprintf(L"[System] WSAResetEvent failed with error = %d\n", WSAGetLastError());
			return ChatState::WSAReceiveFromFailed;
		}

		//Get actual result
		result = WSAGetOverlappedResult(targetSocket, &overlappedHandler, &bytesReceived, 0, &flags);
		if (result == 0)
		{
			wprintf(L"[System] WSArecvFrom failed with error: %d\n", WSAGetLastError());
			return ChatState::WSAReceiveFromFailed;
		}

		if (bytesReceived <= 0)
		{
			return ChatState::WSAReceivedNoBytes;
		}

		//Read packet and return state
		Packet packetToFill{};
		auto data = static_cast<char*>(malloc(bytesReceived));
		std::copy_n(&receiveBuffer[0], bytesReceived, data);
		packetToFill.Append(data, bytesReceived);
		free(data);
		return responseHandler.HandlePacket(std::move(packetToFill));
	}

	int AcceptOrRejectIncomingConnection(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData)
	{
		UNREFERENCED_PARAMETER(dwCallbackData);
		UNREFERENCED_PARAMETER(g);
		UNREFERENCED_PARAMETER(lpGQOS);
		UNREFERENCED_PARAMETER(lpSQOS);
		UNREFERENCED_PARAMETER(lpCalleeData);
		UNREFERENCED_PARAMETER(lpCallerData);
		if ((lpCallerId->len > 0) && (lpCalleeId->len > 0))
		{
// 			Packet packetToFill{};
// 			auto data = static_cast<char*>(malloc(lpCallerData->len));
// 			std::copy_n(lpCallerData->buf, lpCallerData->len, data);
// 			packetToFill.Append(data, lpCallerData->len);
// 			//ChatState state = responseHandler.HandlePacket(std::move(packetToFill));
// 			free(data);
			return CF_ACCEPT;
// 			if (state == ChatState::LocalIdentifiedSuccess)
// 			{
// 			}
		}
		return CF_REJECT;
	}
}

