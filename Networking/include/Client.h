#pragma once

#include "utility.h"
#include <stdio.h>
#include <vector>
#include <atomic>
#include "Packet.h"
#include "Responses.h"

namespace Networking
{
	class Client
	{
	public:
		Client();
		~Client() = default;

		int InitializeWSA();
		int InitializeSearchSocket();
		int SetupChatbox();
		int SearchForLocalChatbox();
		int SetupLocalChatbox();

		int Chat();
		void HandleChatInput();
		int BindChatPort();
		void Close();


		// Handling
		void SetIdentifiedSuccess(bool state) { identifiedSuccessfully = state; }
		bool SetConnectionPartnerIP(std::string& connectionIP);
		const std::string& GetConnectionPartnerIp() { return otherUserIp; }
		bool SetConnectionPartnerPort(u_short port);
		bool TryToSetupTheChatConnection();
		void SetChatState(ChatState newState) { chatState = newState; }

		void SendChatUsername();
		void SetChatParnerName(std::string& nameToSet);
		int SendHeartbeat();
		int HeartbeatReceived();
		void DisconnectUser();
		void LostConnection();
		std::string& GetChatParnetName();

	private:
		Responses responseHandler{};
		SOCKET ssdpListenSocket = INVALID_SOCKET;		// to search // be found
		SOCKET chatSocket = INVALID_SOCKET;				// to chat

		WSAOVERLAPPED overlapped{};
		sockaddr_in ssdpListenAddr{};
		sockaddr_in chatListenAddr{};
		WSADATA wsaReturn{};

		const std::string ssdpRequestString = 
			"M-SEARCH * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"ST: rakas:chat\r\n"
			"MAX: \"ssdp:discover\"\r\n"
			"MX: 3\r\n\r\n";

		int standardBufferSize = 0xFFFF;

		//Information related to connect to chat with
		bool chatboxSetup = false;
		bool identifiedSuccessfully = false;
		bool receivedPossibleConnected = false;
		ChatState chatState = ChatState::NotSet;

		connected_user otherUser{};
		std::string otherUserIp = "";
		std::string userName = "";
		const int hostPort = 6464;
		const int recPort = 6465;
		int chatPortUsed = 0;
		int chatTimeout = 0x2710;
		bool hostingChatbox = false;

		//Multi-threading
		std::atomic<bool> ReadInput = false;

		//int ReceivePackage(SOCKET& targetSocket, sockaddr_in& targetsockaddr, char* buffer);
		//int SendPackage(SOCKET& targetSocket, sockaddr_in& targetsockaddr, Packet& packet);

		int SendChatSocketSetup();
		int SetChatTimeout(int value);

		
	};
}