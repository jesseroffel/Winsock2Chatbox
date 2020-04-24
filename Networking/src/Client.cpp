#include "Client.h"
#include "Netcommands.h"
#include <iostream>

namespace Networking
{
	Client::Client() {}

	ChatState Client::InitializeWSA()
	{
		if (!StartupWSA(0x202, wsaReturn)) 
		{ 
			return ChatState::WSAStartupFailed; 
		}
		if (!SetupOverlapEventHandle(ssdrOverlapped)) 
		{ 
			return ChatState::WSAOverlapEventFailed; 
		}

		return ChatState::WSASetupSuccess;
	}

	ChatState Client::InitializeSearchSocket()
	{
		if (!SetupSocket(ssdpListenSocket, AF_INET, SOCK_STREAM, ConnectionProtocolUsed::TCP, WSA_FLAG_OVERLAPPED))
		{ 
			return ChatState::WSASetSocketSetupFailed; 
		}
		if (!SetSocketReuseAble(ssdpListenSocket, 1)) {
			return ChatState::WSASetSocketReusableFailed; 
		}

		//if (!SetSocketTimeout(ssdpListenSocket, 0xBB8)) { return ChatState::WSASetSocketTimeoutFailed; }
		//if (!SetSocketBroadcast(ssdpListenSocket, true)) { return ChatState::WSASetSocketBroadcastFailed; }

		SetupSockAddrinfo(connection.listenAddress, AF_INET, googleIp, googlePort, false);
		connection.userIp = AttemptToFetchLocalNetworkAddress(ssdpListenSocket, connection.listenAddress);
		if (connection.userIp.empty())
		{
			printf("[System] Unable to test network connection.\n");
			return ChatState::WSASetSocketSetupFailed;
		}

		responseHandler.SetClientReference(this);

		printf("[System] Please enter your username... (25 characters max)\n");
		char name[25];
		std::cin.getline(name, 25);
		connection.name = name;

		return ChatState::WSASetupSocketSuccess;
	}


	Networking::ChatState Client::SetupOrJoinChat()
	{
		bool HostingChatboxAnswer = false;
		std::string answer;
		unsigned char firstChar = 0;
		while (!HostingChatboxAnswer)
		{
			printf("[System] Would you like to host or join a chatbox? [H/J]\n");
			std::cin >> std::ws;	//Remove whitespace
			std::getline(std::cin, answer);
			if (answer.empty()) { continue; }
			firstChar = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(answer[0])));
			if (firstChar == 'H' || firstChar == 'J')
			{
				HostingChatboxAnswer = true;
			}
		}

		//Hosting chatbox
		if (firstChar == 'H')
		{
			ChatState hostingState = SetupLocalChatbox();
			if (hostingState != ChatState::WSAAcceptSuccess)
			{
				printf("[System] Something went wrong with setting up a connection... disconnected.\n");
				return hostingState;
			}
			ChatState verificationState = VerifyJoiningClient();
			if (verificationState != ChatState::LocalAvailableChatFound)
			{
				printf("[System] Something went wrong with establishing a connection... disconnected.\n");
				return hostingState;
			}

			//Host checks up on connected client earlier
			chatTimeout -= 0xBB8;
		}
		else // Joining chatbox
		{
			ChatState connectionState = JoinChatbox();
			if (connectionState != ChatState::WSAAcceptSuccess)
			{
				printf("[System] Something went wrong with setting up a connection... disconnected.\n");
				return connectionState;
			}

			ChatState establishState = EstablishConnectionWithHost();
			if (establishState != ChatState::LocalAvailableChatFound)
			{
				printf("[System] Something went wrong with establishing a connection... disconnected.\n");
				return establishState;
			}
		}

		//Switch to ChatSocket
		//ssdpListenSocket = ssdpListenSocket;
		
		if (!SetSocketTimeout(ssdpListenSocket, chatTimeout)) 
		{ 
			return ChatState::FailedToSetup; 
		}
		printf("[System] You're connected! Type to send a message.\n");
		parnerSetupReady = true;
		return ChatState::SuccessfullySetup;
	}

	ChatState Client::SetupLocalChatbox()
	{
		//target for incoming
		if (!SetupSocket(ssdpListenSocket, AF_INET, SOCK_STREAM, ConnectionProtocolUsed::TCP, WSA_FLAG_OVERLAPPED))
		{
			return ChatState::WSASetSocketSetupFailed;
		}
		// 		if (!SetSocketTimeout(ssdpListenSocket, 0xBB8)) 
		// 		{ 
		// 			return ChatState::WSASetSocketTimeoutFailed; 
		// 		}
				//if (ssdpUnavailable) { SetupSockAddrinfo(ssdpListenAddr, AF_INET, static_cast<u_short>(backupSeekPort), false); }
		SetupSockAddrinfo(connection.listenAddress, AF_INET, connection.userIp, chatPort, false);
		if (!BindSocket(ssdpListenSocket, connection.listenAddress))
		{
			return ChatState::WSABindSocketFailed;
		}
		if (!ListenToSocket(ssdpListenSocket, 1))
		{
			return ChatState::WSAListenToSocketFailed;
		}

		//Accepting connection(s)
		printf("[System] Chatbox is now waiting for a partner! You're available on: %s:%i\n", connection.userIp.c_str(), GetPortToNumber(&connection.listenAddress));
		if (!AcceptSocket(ssdpListenSocket, connection.listenAddress, clientSize))
		{
			return ChatState::WSAAcceptFailed;
		}

		return ChatState::WSAAcceptSuccess;
	}

	ChatState Client::JoinChatbox()
	{
		if (!SetupSocket(ssdpListenSocket, AF_INET, SOCK_STREAM, ConnectionProtocolUsed::TCP, WSA_FLAG_OVERLAPPED))
		{
			return ChatState::WSASetSocketSetupFailed;
		}
		if (!SetSocketTimeout(ssdpListenSocket, setupTimeout))
		{
			return ChatState::WSASetSocketTimeoutFailed;
		}
		//if (ssdpUnavailable) { SetupSockAddrinfo(ssdpListenAddr, AF_INET, static_cast<u_short>(backupSeekPort), false); }
		SetupSockAddrinfo(connection.listenAddress, AF_INET, connection.userIp, chatPort, false);
		if (!ConnectToSocket(ssdpListenSocket, connection.listenAddress))
		{
			//Failed to connect
			return ChatState::WSAConnectToSocketFailed;
		}
		return ChatState::WSAAcceptSuccess;
	}

	Networking::ChatState Client::VerifyJoiningClient()
	{
		//Wait for verification step
		if (!SetSocketTimeout(ssdpListenSocket, setupTimeout))
		{
			return ChatState::WSASetSocketTimeoutFailed;
		}
		ChatState respond = Receive(setupTimeout, connection.listenAddress);
		if (respond != ChatState::LocalIdentifyValidHTTPU)
		{
			printf("[System] Couldn't set up the identification with client..\n");
			return respond;
		}

		printf("[System] Received valid HTTPU request to connect...\n");
		Packet identifyPacket;
		auto netCommand = static_cast<unsigned int>(NetCommands::IdentifySuccessful);
		auto otherIp = static_cast<std::string>(connection.userIp);
		auto nameToSend = static_cast<std::string>(connection.name);
		identifyPacket << netCommand;
		identifyPacket << otherIp;
		identifyPacket << nameToSend;
 
		//Send NetCommands::IdentifySuccessful
 		auto sendAcknowledgeStatus = Send(identifyPacket, connection.listenAddress);
		if (sendAcknowledgeStatus != ChatState::WSASendPacketSuccess)
		{
			printf("[System] Couldn't send package to client to acknowledge..\n");
			return sendAcknowledgeStatus;
		}

		//Receive NetCommands::Acknowledge
		auto receiveAcknowledgeStatus = Receive(setupTimeout, connection.listenAddress);
		if (receiveAcknowledgeStatus != ChatState::ClientAcknowledged)
		{
			printf("[System] Couldn't set up the connection with client..\n");
			return receiveAcknowledgeStatus;
		}
		printf("[System] Incoming connection: %s from %s:%i\n", connection.partnerName.c_str(), connection.partnerIp.c_str(), GetPortToNumber(&connection.listenAddress));


		//Send NetCommand:AcknowledgeComplete and switch
		Packet acknowledgeCompletePacket;
		netCommand = static_cast<unsigned int>(NetCommands::AcknowledgeComplete);
		acknowledgeCompletePacket << netCommand;
		auto sendAcknowledgeCompleteStatus = Send(acknowledgeCompletePacket, connection.listenAddress);
		if (sendAcknowledgeCompleteStatus == ChatState::WSASendPacketSuccess)
		{
			return ChatState::LocalAvailableChatFound;
		}

		return ChatState::LocalNoAvailableChats;
	}

	Networking::ChatState Client::EstablishConnectionWithHost()
	{
		ChatState joinIdentifyPhase = SendIdentifyLocalNetwork();
		if (joinIdentifyPhase != ChatState::LocalIdentifyReceivedSuccess)
		{
			printf("[System] Couldn't set up the identification with host..\n");
			return joinIdentifyPhase;
		}

		printf("[System] Setting up connection with: %s on %s:%i\n", connection.partnerName.c_str(), connection.partnerIp.c_str(), GetPortToNumber(&connection.listenAddress));
		//return name to host as part of the acknowledgement
		Packet acknowledgePacket;
		auto netCommand = static_cast<unsigned int>(NetCommands::Acknowledge);
		auto otherIp = static_cast<std::string>(connection.userIp);
		auto nameToSend = static_cast<std::string>(connection.name);
		acknowledgePacket << netCommand;
		acknowledgePacket << otherIp;
		acknowledgePacket << nameToSend;
		
		//Send NetCommands::Acknowledge
		auto sendAcknowledgeStatus = Send(acknowledgePacket, connection.listenAddress);
		if (sendAcknowledgeStatus != ChatState::WSASendPacketSuccess)
		{
			printf("[System] Couldn't send package to host to acknowledge..\n");
			return sendAcknowledgeStatus;
		}

		//Receive NetCommands::AcknowledgeSuccess/AcknowledgeFailure
		auto receiveAcknowledgeStatus = Receive(setupTimeout, connection.listenAddress);
		if (receiveAcknowledgeStatus != ChatState::ClientAcknowledgeSuccess)
		{
			printf("[System] Couldn't set up the connection with host..\n");
		}
		return ChatState::LocalAvailableChatFound;
	}

	void Client::Close()
	{
		shutdown(ssdpListenSocket, 2);
		closesocket(ssdpListenSocket);
		if (WSACleanup() != 0)
		{
			printf("[Winsock] Unable to Cleanup: %i", WSAGetLastError());
		}
	}

	ChatState Client::Send(Packet packet, sockaddr_in& address)
	{
		lastSendState = SendPacket(ssdpListenSocket, ssdrOverlapped, address, packet);
		lastChatState = lastSendState;
		return lastChatState;
	}

	ChatState Client::Receive(WORD timeout, sockaddr_in& address)
	{
		lastReceiveState = ReceivePacket(ssdpListenSocket, ssdrOverlapped, responseHandler, address, timeout);
		lastChatState = lastReceiveState;
		return lastChatState;
	}

	int Client::SetChatTimeout(int value)
	{
		return SetSocketTimeout(ssdpListenSocket, value);
	}


	bool Client::SetConnectionPartnerIP(std::string& connectionIP)
	{
		connection.partnerIp = connectionIP;
		int returnValue = GetAddressFromString(&connection.listenAddress, connectionIP);
		if (returnValue != 0)
		{
			connection.listenAddress.sin_addr.s_addr = returnValue;
			receivedPossibleConnected = true;
		}
		return false;
	}

	ChatState Client::SendIdentifyLocalNetwork()
	{
		//Send local network identify message
		Packet identifyPacket;
		auto netCommand = static_cast<unsigned int>(NetCommands::IdentifyBroadcast);
		identifyPacket << netCommand;
		identifyPacket << ssdpRequestString;

		ChatState status = Send(identifyPacket, connection.listenAddress);
		if (status != ChatState::WSASendPacketSuccess) { return ChatState::LocalIdentifyNetworkSendFailed; }

		//Allocate buffer for possible response
		//auto* buffer = (char*)malloc(standardBufferSize);
		return Receive(setupTimeout, connection.listenAddress);
	}

	void Client::SendChatUsername()
	{
		Packet chatSend;
		auto netCommand = static_cast<unsigned int>(NetCommands::ChatCommand);
		auto chatCommand = static_cast<unsigned int>(ChatCommands::Chatmessage);
		chatSend << netCommand;
		chatSend << chatCommand;
		chatSend << connection.name;
		Send(chatSend, connection.listenAddress);
	}

	void Client::SendHeartbeatCheck()
	{
		Packet heartbeat;
		auto netCommand = static_cast<unsigned int>(NetCommands::ChatCommand);
		auto chatCommand = static_cast<unsigned int>(ChatCommands::HeartbeatCheck);
		heartbeat << netCommand;
		heartbeat << chatCommand;
		Send(heartbeat, connection.listenAddress);
	}

	void Client::SendHeartbeatResponse()
	{
		Packet heartbeat;
		auto netCommand = static_cast<unsigned int>(NetCommands::ChatCommand);
		auto chatCommand = static_cast<unsigned int>(ChatCommands::HeartbeatPulse);
		heartbeat << netCommand;
		heartbeat << chatCommand;
		Send(heartbeat, connection.listenAddress);
	}

	void Client::DisconnectUser()
	{
		printf("[System] %s left the chat, disconnecting...\n", connection.partnerName.c_str());
		ReadInput = false;
		lastChatState = ChatState::ChatClosed;
		closesocket(ssdpListenSocket);
	}

	void Client::LostConnection()
	{
		printf("[System] Lost connection with %s, disconnecting...\n", connection.partnerName.c_str());
		ReadInput = false;
		lastChatState = ChatState::ChatClosed;
		closesocket(ssdpListenSocket);
	}

	void Client::ChatProblemOccurred()
	{
		printf("[System] A problem occurred. Disconnecting..\n");
		ReadInput = false;
		lastChatState = ChatState::ChatClosed;
		closesocket(ssdpListenSocket);
	}

	void Client::AskForNewConnection()
	{
		bool HostingChatboxAnswer = false;
		std::string answer;
		unsigned char firstChar = 0;
		while (!HostingChatboxAnswer)
		{
			printf("[System] Would you like to host or join a chatbox? [Y/N]\n");
			std::cin >> std::ws;	//Remove whitespace
			std::getline(std::cin, answer);
			if (answer.empty()) { continue; }
			firstChar = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(answer[0])));
			if (firstChar == 'Y' || firstChar == 'N')
			{
				HostingChatboxAnswer = true;
			}
		}

		if (firstChar == 'N')
		{
			quitApplication = true;
		}

		parnerSetupReady = false;
		closesocket(ssdpListenSocket);
		//Setup Local network search socket, if this fails then this means the chat can't function at all.
		if (InitializeSearchSocket() != Networking::ChatState::WSASetupSocketSuccess)
		{
			printf("[System] Socket Initialization failed: unable to setup the chat.\n");
			Close();
			quitApplication = true;
		}
	}

	std::string& Client::GetChatParnetName()
	{
		return connection.partnerName;
	}

	void Client::HandleChatInput()
	{
		ReadInput = true;
		char textMessage[10000];

		do
		{
			std::cin.getline(textMessage, 10000);
			const std::string convertedMessage(textMessage);
			if (convertedMessage.length() == 0)
			{
				continue;
			}
			if (convertedMessage.front() != '/')
			{
				Packet chatSend;
				auto netCommand = static_cast<unsigned int>(NetCommands::ChatCommand);
				auto chatCommand = static_cast<unsigned int>(ChatCommands::Chatmessage);
				chatSend << netCommand;
				chatSend << chatCommand;
				chatSend << convertedMessage;
				ChatState sendState = Send(chatSend, connection.listenAddress);
				if (sendState != ChatState::WSASendPacketSuccess)
				{
					printf("[Chat] Unable to send message...\n");
				}
				continue;
			}

			//Read chatcommands
			if (strcmp("/disconnect", textMessage) == 0)
			{
				printf("[System] Disconnecting...\n");
				Packet disconnectPacket;
				auto netCommand = static_cast<unsigned int>(NetCommands::Disconnect);
				disconnectPacket << netCommand;
				Send(disconnectPacket, connection.listenAddress);
				ReadInput = false;
				lastChatState = ChatState::ChatClosed;
				continue;
			}
			printf("[System] Unknown command: %s\n", convertedMessage.c_str());
		} while (ReadInput);
	}

	int Client::HandleChatNetwork()
	{
		bool checkHeartbeat = false;
		while (lastChatState != ChatState::ChatClosed)
		{
			ChatState chatState = Receive(chatTimeout, connection.listenAddress);
			if (chatState == ChatState::WSAReceivedNoBytes || chatState == ChatState::WSAReceiveFromFailed)
			{
				ChatProblemOccurred();
				continue;
			}

			if (chatState == ChatState::WSAReceiveTimeout)
			{
				if (!checkHeartbeat)
				{
					checkHeartbeat = true;
					SendHeartbeatCheck();
					continue;
				}
				//Lost conneciton
				LostConnection();
				continue;
			}
			if ((chatState == ChatState::ChatCommandReceived || chatState == ChatState::HeartbeatReceived) && checkHeartbeat)
			{
				checkHeartbeat = false;
			}

		}

		//Close current chat and possibly ask for another one
		parnerSetupReady = false;
		return 0;
	}
}

