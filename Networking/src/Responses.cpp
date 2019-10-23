#include "Responses.h"
#include "Packet.h"
#include "Netcommands.h"
#include "Client.h"

namespace Networking
{
	bool Responses::HandlePacket(Packet&& packet)
	{
		unsigned int rawNetcommand = 0;
		packet >> rawNetcommand;

		auto netcommand = static_cast<Netcommands>(rawNetcommand);

		switch (netcommand)
		{
		case Netcommands::Identify:
		{
			return IdentifiedCorrectly(std::move(packet));
		}
		case Netcommands::IdentifySuccessful:
		{
			return IdentifiedSuccessfully(std::move(packet));
		}
		case Netcommands::Acknowledge:
		{
			return AcknowledgeClient(std::move(packet));
		}
		case Netcommands::AcknowledgeSuccess:
		{
			return AcknowledgeClientSuccess(std::move(packet));
		}
		case Netcommands::AcknowledgeFailure:
		{
			return AcknowledgeClientFailure(std::move(packet));
		}
		case Netcommands::ChatCommand:
		{
			return HandleChatpacket(std::move(packet));
		}
		case Netcommands::Disconnect:
		{
			clientReference->DisconnectUser();
			return true;
		}
		default:
		{
			printf("[Network] Received unknown packet\n\n");
			return false;
		}
		}
	}

	bool Responses::HandleChatpacket(Packet&& packet)
	{
		unsigned int rawChatCommand = 0;
		packet >> rawChatCommand;

		auto chatCommand = static_cast<ChatCommands>(rawChatCommand);
		switch (chatCommand)
		{
		case ChatCommands::NameIdentify:
		{
			return ChatNameIdentification(std::move(packet));
		}
		case ChatCommands::Chatmessage:
		{
			return ChatMessage(std::move(packet));
		}
		case ChatCommands::HeartbeatCheck:
		{
			printf("[Chat] Received Heartbeat request!\n");
			return clientReference->SendHeartbeat();
		}
		case ChatCommands::HeartbeatPulse:
		{
			printf("[Chat] Received Heartbeat response!\n");
			return true;
		}
		default:
		{
			printf("[Chat] Unknown chat command received!\n");
		}
		}
		return false;
	}

	bool Responses::IdentifiedCorrectly(Packet&& packet)
	{
		std::string checkHttpu = "";
		const int totalLength = 96;
		
		packet >> checkHttpu;

		if (!checkHttpu.empty())
		{
			if (checkHttpu.length() == totalLength)
			{
				printf("[Network] Received HTTPU request\n");
				//printf("\n%s", checkHttpu.c_str());
				clientReference->SetIdentifiedSuccess(true);
				return true;
			}
		}
		return false;
	}

	bool Responses::IdentifiedSuccessfully(Packet&& packet)
	{
		std::string responseString = "";
		std::string ipToConnectWith = "";
		packet >> responseString;
		if (responseString.length() == 35)
		{
			packet >> ipToConnectWith;
			printf("[Network] Found available chatbox with ip: %s!\n", ipToConnectWith.c_str());
			//printf("\n%s\n", responseString.c_str());

			clientReference->SetConnectionPartnerIP(ipToConnectWith);
			return true;
		}
		return false;
	}

	bool Responses::AcknowledgeClient(Packet&& packet)
	{
		std::string ipToConnectWith = "";
		unsigned int chatPort = 0;
		packet >> ipToConnectWith;
		packet >> chatPort;
		auto correctPort = static_cast<int>(chatPort);
		if (!ipToConnectWith.empty() && chatPort != 0)
		{
			clientReference->SetConnectionPartnerIP(ipToConnectWith);
			clientReference->SetConnectionPartnerPort(static_cast<u_short>(correctPort));
			if (clientReference->TryToSetupTheChatConnection())
			{
				clientReference->BindChatPort();
				clientReference->SetChatState(ChatState::SuccessfullySetup);
				return true;
			}
			clientReference->SetChatState(ChatState::FailedToSetup);
			return false;
		}
		return true;
	}

	bool Responses::AcknowledgeClientSuccess(Packet&& packet)
	{
		unsigned int tempPortGetter;
		packet >> tempPortGetter;
		u_short portToUse = 0;
		portToUse = static_cast<u_short>(tempPortGetter);
		printf("[Network] %s:%i is ready for chat!\n", clientReference->GetConnectionPartnerIp().c_str(), portToUse);
		return clientReference->SetConnectionPartnerPort(portToUse);
	}

	bool Responses::AcknowledgeClientFailure(Packet&& packet)
	{
		packet.Clear();
		printf("[Network] %s wasn't able to start a chat connection successfully.\n", clientReference->GetConnectionPartnerIp().c_str());
		return true;
	}

	bool Responses::ChatNameIdentification(Packet&& packet)
	{
		std::string name;
		packet >> name;
		printf("[Chat] Other client identified as: %s\n", name.c_str());
		clientReference->SetChatParnerName(name);
		return true;
	}

	bool Responses::ChatMessage(Packet&& packet)
	{
		std::string username = "";
		std::string messageReceived = "";
		packet >> username;
		packet >> messageReceived;
		if (!username.empty() || !messageReceived.empty())
		{
			printf("[%s]: %s\n", username.c_str(), messageReceived.c_str());
			return true;
		}
		return false;
	}

}
