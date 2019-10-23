#include "Client.h"
#include "Netcommands.h"
#include <iostream>


Networking::Client::Client()
	
{

}

int Networking::Client::InitializeWSA()
{
	if (Networking::StartupWSA(0x202, wsaReturn)) { return 1; }
	if (Networking::SetupOverlapEventHandle(overlapped)) { return 1; }

	return 0;
}

int Networking::Client::InitializeSearchSocket()
{
	if (Networking::SetupSocket(ssdpListenSocket, AF_INET, SOCK_STREAM, connectionProtocol::UDP)) { return 1; }

	if (Networking::SetSocketTimeout(ssdpListenSocket, 0xBB8)) { return 1; }

	if (Networking::SetSocketReuseAble(ssdpListenSocket, 1)) { return 1; }

	if (Networking::SetSocketBroadcast(ssdpListenSocket)) { return 1; }

	Networking::SetupSockAddrinfo(ssdpListenAddr, AF_INET, 1900, true);

	responseHandler.SetClientReference(this);
	return 0;
}

int Networking::Client::SetupChatbox()
{
	char name[25];
	printf("[Chat] Please enter your username as identification.\n");
	std::cin.getline(name, 25);
	userName = name;

	while (!chatboxSetup)
	{
		if (this->SearchForLocalChatbox() != 0)
		{
			printf("[Chat] Available chatbox found!\n");
			chatboxSetup = true;
		}
		else
		{
			printf("[Chat] No available chatbox found.\n");
			if (SetupLocalChatbox())
			{
				chatState = ChatState::FailedToSetup;
				WSACleanup();
			}
			chatboxSetup = true;
		}
	}
	if (chatState == ChatState::FailedToSetup || chatState == ChatState::NotSet)
	{
		return 0;
	}

	chatListenAddr.sin_port = htons(static_cast<u_short>(chatPortUsed));

	printf("[Chat] You're connected! Type to send a message.\n");
	//No longer needed to keep this up to date.
	closesocket(ssdpListenSocket);
	return 1;
}

int Networking::Client::SearchForLocalChatbox()
{
	printf("[Network] Sending ping to nearby open chatbox...\n");

	Packet identifyPacket;
	auto netCommand = static_cast<unsigned int>(Netcommands::Identify);
	identifyPacket << netCommand;
	identifyPacket << ssdpRequestString;

	if (Networking::SendPacket(ssdpListenSocket, overlapped, ssdpListenAddr, identifyPacket) != 0) { return 1; }

	auto* buffer = (char*)malloc(standardBufferSize);
	Packet identifyResponse;
	if (Networking::ReceivePacket(ssdpListenSocket, overlapped, ssdpListenAddr, identifyResponse) != 0)
	{
		responseHandler.HandlePacket(std::move(identifyResponse));
		if (receivedPossibleConnected)
		{
			//Setup port
			if (!TryToSetupTheChatConnection())
			{
				printf("[Network] Couldn't complete chat setup..\n");
				return 1;
			}

			Packet acknowledgePacket;
			netCommand = static_cast<unsigned int>(Netcommands::Acknowledge);
			std::string connectIp = Networking::GetAddressToString(&ssdpListenAddr);
			chatPortUsed = recPort;
			auto portToSend = static_cast<unsigned int>(chatPortUsed);
			acknowledgePacket << netCommand;
			acknowledgePacket << connectIp;
			acknowledgePacket << portToSend;
			if (Networking::SendPacket(ssdpListenSocket, overlapped, ssdpListenAddr, acknowledgePacket) != 0)
			{
				printf("[Network] Waiting 10 seconds for the host to setup a chat..\n");
				if (Networking::SetSocketTimeout(ssdpListenSocket, chatTimeout))
				{
					printf("[Network] Unable to wait, closing connection..\n");
					return 0;
				}
				Packet acknowledgeResponse;
				if (Networking::ReceivePacket(ssdpListenSocket, overlapped, ssdpListenAddr, acknowledgeResponse) != 0)
				{
					responseHandler.HandlePacket(std::move(acknowledgeResponse));
					// RESPOND TO CHAT SuccessfullySetup OR FailedToSetup
					if (TryToSetupTheChatConnection())
					{
						printf("[Network] Client set up correctly to connect with chat partner.\n");
						SetChatState(ChatState::SuccessfullySetup);
						BindChatPort();
						return 1;
					}
					else
					{
						printf("[Network] Client wasn't able to start a chat connection.\n");
						SetChatState(ChatState::FailedToSetup);
						return 0;
					}
				}
				else
				{
					printf("[Network] Other client didn't respond to setup a connection..\n");
					return 0;
				}
			}
		}
	}
	free(buffer);
	return 0;
}

int Networking::Client::SetupLocalChatbox()
{
	if (Networking::SetupSocket(ssdpListenSocket, AF_INET, SOCK_STREAM, connectionProtocol::UDP))
	{
		return 1;
	}

	if (Networking::SetSocketTimeout(ssdpListenSocket, 0xBB8))
	{
		closesocket(ssdpListenSocket);
		WSACleanup();
		return 1;
	}

	if (Networking::SetSocketReuseAble(ssdpListenSocket, 1))
	{
		closesocket(ssdpListenSocket);
		WSACleanup();
		return 1;
	}


	Networking::SetupSockAddrinfo(ssdpListenAddr, AF_INET, 1900, false);

	if (Networking::BindSocket(ssdpListenSocket, ssdpListenAddr))
	{
		closesocket(ssdpListenSocket);
		WSACleanup();
		return 1;
	}

	//target for incoming
	SOCKADDR_IN ssdpAddr;
	ssdpAddr.sin_family = AF_INET;
	ssdpAddr.sin_port = htons(1900);
	ssdpAddr.sin_addr.s_addr = INADDR_BROADCAST;

	auto* buffer = (char*)malloc(standardBufferSize);

	printf("[Network] Listening to other requests...\n");
	bool suitableChatCompanionFound = false;
	bool waitingForAsk = false;
	hostingChatbox = true;
	chatPortUsed = hostPort;
	chatTimeout = chatTimeout - 0x400;
	while (!suitableChatCompanionFound)
	{
		if (!waitingForAsk)
		{
			Packet waitForSuitableClientPacket{};
			if (Networking::ReceivePacket(ssdpListenSocket, overlapped, ssdpListenAddr, waitForSuitableClientPacket) != 0)
			{
				responseHandler.HandlePacket(std::move(waitForSuitableClientPacket));
				if (identifiedSuccessfully)
				{
					const std::string responseString = "HTTP/1.1 200 OK\r\nST: \"rakas:chat\"\r\n";

					const std::string connectIp = Networking::GetAddressToString(&ssdpListenAddr);
					if (!connectIp.empty())
					{
						Packet companionFoundReply;
						auto netCommand = static_cast<unsigned int>(Netcommands::IdentifySuccessful);
						companionFoundReply << netCommand;
						companionFoundReply << responseString;
						companionFoundReply << connectIp;
						if (Networking::SendPacket(ssdpListenSocket, overlapped, ssdpListenAddr, companionFoundReply) != 0)
						{
							waitingForAsk = true;
						}
					}
				}
			}
		}
		else
		{
			//HOST RECEIVING CLIENT IP
			Packet receivingAckPacket{};
			if (Networking::ReceivePacket(ssdpListenSocket, overlapped, ssdpListenAddr, receivingAckPacket) != 0)
			{
				responseHandler.HandlePacket(std::move(receivingAckPacket));
				SendChatSocketSetup();
				suitableChatCompanionFound = true;
			}
		}
	}
	free(buffer);
	return 0;
}

void Networking::Client::Close()
{
	shutdown(ssdpListenSocket, 2);
	shutdown(chatSocket, 2);
	closesocket(ssdpListenSocket);
	closesocket(chatSocket);
	if (WSACleanup() != 0)
	{
	}
	printf("[WINSOCK2] Unable to Cleanup: %i", WSAGetLastError());
}

int Networking::Client::SendChatSocketSetup()
{
	Packet setupPacket{};
	if (chatState == ChatState::FailedToSetup)
	{
		auto netCommand = static_cast<unsigned int>(Netcommands::AcknowledgeFailure);
		setupPacket << netCommand;
		Networking::SendPacket(ssdpListenSocket, overlapped, ssdpListenAddr, setupPacket);
		return 0;
	}
	if (chatState == ChatState::SuccessfullySetup)
	{
		auto netCommand = static_cast<unsigned int>(Netcommands::AcknowledgeSuccess);
		auto portUsed = static_cast<unsigned int>(chatPortUsed);
		setupPacket << netCommand;
		setupPacket << portUsed;
		Networking::SendPacket(ssdpListenSocket, overlapped, ssdpListenAddr, setupPacket);
		return 1;
	}
	return 0;
}

int Networking::Client::SetChatTimeout(int value)
{
	if (Networking::SetSocketTimeout(chatSocket, value))
	{
		closesocket(chatSocket);
		WSACleanup();
		return false;
	}
	return true;
}

int Networking::Client::BindChatPort()
{
	if (Networking::BindSocket(chatSocket, otherUser.address))
	{
		printf("[Network] Chat socket failed to setup correctly.\n");
		printf("[Network] Connected with: %s:%i\n", Networking::GetAddressToString(&otherUser.address).c_str(), Networking::GetPortToNumber(&otherUser.address));
		closesocket(chatSocket);
		WSACleanup();
		return false;
	}
	
	printf("[Network] Chat socket successfully set up.\n");
	printf("[Network] Connected with: %s:%i\n", Networking::GetAddressToString(&otherUser.address).c_str(), Networking::GetPortToNumber(&otherUser.address));
	return true;
}

bool Networking::Client::SetConnectionPartnerIP(std::string& connectionIP)
{
	otherUserIp = connectionIP;
	int returnValue = Networking::GetAddressFromString(&ssdpListenAddr, connectionIP);
	if (returnValue != 0)
	{
		otherUser.address.sin_addr.s_addr = returnValue;
		chatListenAddr.sin_addr.s_addr = returnValue;
		receivedPossibleConnected = true;
	}
	//otherUser.address.sin_addr.s_addr = inet_pton(connectionIP.c_str());
	return false;
}

bool Networking::Client::SetConnectionPartnerPort(u_short port)
{
	if (port > 0)
	{
		otherUser.address.sin_port = htons(port);
		return true;
	}
	return false;
}

bool Networking::Client::TryToSetupTheChatConnection()
{
	if (Networking::SetupSocket(chatSocket, AF_INET, SOCK_STREAM, connectionProtocol::UDP))
	{
		printf("[Network] Unable to create sockets!\n");
		WSACleanup();
		return false;
	}

	if (Networking::SetSocketTimeout(chatSocket, chatTimeout))
	{
		closesocket(chatSocket);
		WSACleanup();
		return false;
	}


	if (Networking::SetSocketReuseAble(chatSocket, 1))
	{
		closesocket(chatSocket);
		WSACleanup();
		return false;
	}
	if (otherUser.address.sin_addr.S_un.S_addr == 0)
	{
		closesocket(chatSocket);
		WSACleanup();
		return false;
	}
	else
	{
		otherUser.address.sin_family = AF_INET;
		chatListenAddr.sin_family = AF_INET;
	}

	return true;
}


void Networking::Client::SendChatUsername()
{
	auto netCommand = static_cast<unsigned int>(Netcommands::ChatCommand);
	auto chatCommand = static_cast<unsigned int>(ChatCommands::Chatmessage);
	Packet chatSend;
	chatSend << netCommand;
	chatSend << chatCommand;
	chatSend << userName;
	Networking::SendPacket(chatSocket, overlapped, otherUser.address, chatSend);
}

void Networking::Client::SetChatParnerName(std::string& nameToSet)
{
	otherUser.name = nameToSet;
}

int Networking::Client::SendHeartbeat()
{
	Packet namePacket;
	auto netCommand = static_cast<unsigned int>(Netcommands::ChatCommand);
	auto chatCommand = static_cast<unsigned int>(ChatCommands::HeartbeatPulse);
	namePacket << netCommand;
	namePacket << chatCommand;
	Networking::SendPacket(chatSocket, overlapped, otherUser.address, namePacket);
	return 1;
}

int Networking::Client::HeartbeatReceived()
{
	Packet namePacket;
	auto netCommand = static_cast<unsigned int>(Netcommands::ChatCommand);
	auto chatCommand = static_cast<unsigned int>(ChatCommands::HeartbeatCheck);
	namePacket << netCommand;
	namePacket << chatCommand;
	Networking::SendPacket(chatSocket, overlapped, otherUser.address, namePacket);

	if (Networking::SetSocketTimeout(chatSocket, 0xD00))
	{
		closesocket(chatSocket);
		WSACleanup();
		return 0;
	}
	Packet chatReceive{};
	if (Networking::ReceivePacket(chatSocket, overlapped, otherUser.address, chatReceive) != 0)
	{
		responseHandler.HandlePacket(std::move(chatReceive));
		return 1;
	}
	return 0;
}

void Networking::Client::DisconnectUser()
{
	printf("[Chat] The other user left the chat, disconnecting..\n");
	ReadInput = false;
	chatState = ChatState::ChatClosed;
}

void Networking::Client::LostConnection()
{
	printf("[Chat] Lost connection...\n");
	ReadInput = false;
	chatState = ChatState::ChatClosed;
}

std::string& Networking::Client::GetChatParnetName()
{
	return otherUser.name;
}

void Networking::Client::HandleChatInput()
{
	ReadInput = true;
	char textMessage[265];
	do
	{
		std::cin.getline(textMessage, 265);
		const std::string convertedMessage(textMessage);
		if (convertedMessage.length() != 0)
		{
			if (convertedMessage.front() != '.')
			{
				auto netCommand = static_cast<unsigned int>(Netcommands::ChatCommand);
				auto chatCommand = static_cast<unsigned int>(ChatCommands::Chatmessage);
				Packet chatSend;
				chatSend << netCommand;
				chatSend << chatCommand;
				chatSend << userName;
				chatSend << convertedMessage;
				Networking::SendPacket(chatSocket, overlapped, otherUser.address, chatSend);
			}
			else
			{
				if (strcmp(".disconnect", textMessage) == 0)
				{
					printf("[Chat] Disconnecting...\n");
					auto netCommand = static_cast<unsigned int>(Netcommands::Disconnect);
					Packet disconnectPacket;
					disconnectPacket << netCommand;
					Networking::SendPacket(chatSocket, overlapped, otherUser.address, disconnectPacket);
					ReadInput = false;
					chatState = ChatState::ChatClosed;
				}
				else
				{
					printf("[Chat] Unknown command: %s\n", convertedMessage.c_str());
				}
			}
		}
	} while (ReadInput);
}

int Networking::Client::Chat()
{
	auto* buffer = (char*)malloc(0xFFFF);

	while (chatState != ChatState::ChatClosed)
	{
		Packet chatReceive{};
		if (Networking::ReceivePacket(chatSocket, overlapped, otherUser.address, chatReceive) != 0)
		{
			responseHandler.HandlePacket(std::move(chatReceive));
		}
		else
		{
			if (!HeartbeatReceived())
			{
				LostConnection();
			}
			else
			{
				SetChatTimeout(chatTimeout);
			}
		}
	}
	free(buffer);


	return 0;
}
