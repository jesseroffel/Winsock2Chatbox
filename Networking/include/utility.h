#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

namespace Networking
{
	const int defaultBufferSize = 1024;
	class Packet;
	enum class connectionProtocol
	{
		UNSET = 0,
		TCP = 6,
		UDP = 17,
		UCMPV6 = 58
	};

	enum class ChatState
	{
		NotSet = 900,
		FailedToSetup,
		SuccessfullySetup,
		NameIdentifyPhase,
		ChatPhase,
		ChatClosed
	};

	struct connected_user
	{
		SOCKADDR_IN address{};
		std::string name = "UndentifiedUser";
	};

	int StartupWSA(int version, WSADATA& dataToStore);
	int SetupOverlapEventHandle(OVERLAPPED& overlapToSetup);
	int SetupSocket(SOCKET& targetSocket, int addressFamily, int sockType, connectionProtocol protocol, DWORD flags = 0);
	int SetSocketBroadcast(SOCKET& targetSocket);
	int SetSocketTimeout(SOCKET& targetSocket, DWORD timeoutValue);
	int SetSocketReuseAble(SOCKET& targetSocket, int optionValue);
	void SetupSockAddrinfo(SOCKADDR_IN& endpoint, int addressFamily, u_short port, bool broadcast);
	int BindSocket(SOCKET& targetSocket, SOCKADDR_IN& endpoint);

	std::string GetAddressToString(sockaddr_in* sockaddr);
	int GetAddressFromString(sockaddr_in* sockaddr, std::string& addressToConvert);
	int GetPortToNumber(sockaddr_in* sockaddr);

	int SendPacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& recvSockAdd, Packet& packetToSend, DWORD flags = 0);
	int ReceivePacket(SOCKET& targetSocket, WSAOVERLAPPED& overlappedHandler, sockaddr_in& senderSockAdd, Packet& packetToFill, DWORD flags = 0);
}