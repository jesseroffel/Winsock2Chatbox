#pragma once

#include "utility.h"
#include <stdio.h>
#include <vector>
#include <atomic>
#include "Packet.h"
#include "Responses.h"
#include "ChatboxStates.h"
#include "Netcommands.h"

namespace Networking
{
	class Client
	{
	public:
		bool parnerSetupReady = false;
		bool heartBeatCheck = false;
		bool quitApplication = false;

		Client();
		~Client() = default;

		//Initialize functions
		ChatState InitializeWSA();
		ChatState InitializeSearchSocket();
		ChatState SetupOrJoinChat();
		ChatState SetupLocalChatbox();
		ChatState JoinChatbox();
		
		// Handling states
		ChatState VerifyJoiningClient();
		ChatState EstablishConnectionWithHost();
		ChatState SendIdentifyLocalNetwork();

		int HandleChatNetwork();
		void HandleChatInput();
		void Close();


		//Packet Wrapper
		ChatState Send(Packet packet, sockaddr_in& address);
		ChatState Receive(WORD timeout, sockaddr_in& address);
		//Packet info setting
		void SetNetCommandReceived(NetCommands command) { lastNetCommand = command; }
		void SetChatCommandReceived(ChatCommands command) { lastChatCommand = command; }

		void SetParnerSetupReady(bool state) { parnerSetupReady = state; }
		bool SetConnectionPartnerIP(std::string& connectionIP);
		const std::string& GetConnectionPartnerIp() { return connection.partnerIp; }
		void SetChatCompanionFound(bool state) { suitableChatCompanionFound = state; }
		void SetHeartbeatReceived(bool state) { heartbeatReceived = state; }
		
		void SendChatUsername();
		void SetChatParnerName(std::string& nameToSet) { connection.partnerName = nameToSet; }
		void SetChatParnerIp(std::string& ipToSet) { connection.partnerIp = ipToSet; }
		void SendHeartbeatCheck();
		void SendHeartbeatResponse();
		void DisconnectUser();
		void LostConnection();
		void ChatProblemOccurred();
		void AskForNewConnection();
		std::string& GetChatParnetName();

	private:
		Responses responseHandler{};
		connected_user connection{};
		SOCKET ssdpListenSocket = INVALID_SOCKET;		// to search // be found
		DWORD totalEvents = 0;

		WSAEVENT wsaEvents[WSA_MAXIMUM_WAIT_EVENTS];
		DWORD wsaEventCount = 0;
		WSAOVERLAPPED ssdrOverlapped{};
		WSAOVERLAPPED sendingOverlapped{};
		WSAOVERLAPPED receivingOverlapped{};
		WSADATA wsaReturn{};

		const std::string ssdpRequestString = 
			"M-SEARCH * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"ST: sampo:chat\r\n"
			"MAX: \"ssdp:discover\"\r\n"
			"MX: 3\r\n\r\n";
		const std::string googleIp = "8.8.8.8";


		const int standardBufferSize = 0xFFFF;
		const int backupSeekPort = 3553;
		const int clientSize = sizeof(sockaddr_in);
		const WORD setupTimeout = 0x1000;
		WORD chatTimeout = 0x2710;
		const u_short googlePort = 53;
		const u_short chatPort = 29640;

		//Received states
		ChatState lastChatState = ChatState::NotSet;
		ChatState lastSendState = ChatState::NotSet;
		ChatState lastReceiveState = ChatState::NotSet;
		NetCommands lastNetCommand = NetCommands::Invalid;
		ChatCommands lastChatCommand = ChatCommands::Invalid;

		//Information related to connect to chat with
		bool chatboxSetup = false;
		bool suitableChatCompanionFound = false;
		bool receivedPossibleConnected = false;
		bool heartbeatReceived = false;

		//Multi-threading
		std::atomic<bool> ReadInput = false;

		int SetChatTimeout(int value);

		SOCKET* GetCurrentActiveSocket();
		
	};
}