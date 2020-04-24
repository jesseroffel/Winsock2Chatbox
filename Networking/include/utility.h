#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <vector>
#include "ChatboxStates.h"

namespace Networking
{
	const int defaultBufferSize = 1024;
	class Packet;
	class Responses;

	enum class ConnectionProtocolUsed
	{
		UNSET = 0,
		TCP = 6,
		UDP = 17,
		UCMPV6 = 58
	};

	struct connected_user
	{
		SOCKADDR_IN listenAddress{};
		//Can be set through SOCKADDR_IN convertion or directly through packets.
		std::string userIp = "";
		std::string name = "Unknown";
		std::string partnerIp = "";
		std::string partnerName = "Undentified Connection";
	};

	bool StartupWSA(int version, WSADATA& dataToStore);
	bool SetupOverlapEventHandle(OVERLAPPED& overlapToSetup);
	bool SetupSocket(SOCKET& targetSocket, int addressFamily, int sockType, ConnectionProtocolUsed protocol, DWORD flags = 0);
	bool SetSocketBroadcast(SOCKET& targetSocket, bool value);
	bool SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue);
	bool SetSocketReuseAble(SOCKET& targetSocket, int optionValue);
	void SetupSockAddrinfo(SOCKADDR_IN& endpoint, int addressFamily, std::string ipaddress, u_short port, bool broadcast);
	bool BindSocket(SOCKET& targetSocket, SOCKADDR_IN& endpoint);
	bool ListenToSocket(SOCKET& targetSocket, int amountConnectionsAccepted);
	bool ConnectToSocket(SOCKET& targetSocket, sockaddr_in& connectToAddress);
	bool AcceptSocket(SOCKET& listenSocket, sockaddr_in& endpoint, int sizeOfEndpoint);
	bool CloseSocket(SOCKET& targetSocket);

	std::string GetAddressToString(sockaddr_in* sockaddr);
	int GetAddressFromString(sockaddr_in* sockaddr, std::string& addressToConvert);
	int GetPortToNumber(sockaddr_in* sockaddr);

	std::string AttemptToFetchLocalNetworkAddress(SOCKET& targetSocket, sockaddr_in& connectToAddress);
	ChatState SendPacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& recvSockAdd, Packet& packetToSend, DWORD flags = 0);
	ChatState ReceivePacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, Responses& responseHandler, sockaddr_in& recvSockAdd, DWORD responseTimeOut, DWORD flags = 0);

	int CALLBACK AcceptOrRejectIncomingConnection(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS pQos, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData);
}