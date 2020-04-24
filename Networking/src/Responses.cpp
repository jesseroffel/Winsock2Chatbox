#include "Packet.h"
#include "Netcommands.h"
#include "Client.h"
#include "Responses.h"

namespace Networking
{
	ChatState Responses::HandlePacket(Packet&& packet)
	{
		unsigned int rawNetcommand = 0;
		packet >> rawNetcommand;

		NetCommands netcommand = static_cast<NetCommands>(rawNetcommand);
		clientReference->SetNetCommandReceived(netcommand);

		switch (netcommand)
		{
			case NetCommands::IdentifyBroadcast:
			{
				return CheckHTTPURequest(std::move(packet));
			}
			case NetCommands::IdentifySuccessful:
			{
				return IdentifiedSuccessfully(std::move(packet));
			}
			case NetCommands::IdentifyFailure:
			{
				return ChatState::LocalIdentifyReceivedFailed;
			}
			case NetCommands::Acknowledge:
			{
				return ClientAcknowledged(std::move(packet));
			}
			case NetCommands::AcknowledgeComplete:
			{
				return ChatState::ClientAcknowledgeSuccess;
			}
			case NetCommands::ChatCommand:
			{
				return HandleChatpacket(std::move(packet));
			}
			case NetCommands::Disconnect:
			case NetCommands::LostConnection:
			{
				clientReference->DisconnectUser();
				return ChatState::ExitApplication;
			}
		}

		//printf("[Network] Received unknown packet, netcommand: %u\n", rawNetcommand);
		return ChatState::WSAReceivedInvalidPacket;
	}

	ChatState Responses::HandleChatpacket(Packet&& packet)
	{
		unsigned int rawChatCommand = 0;
		packet >> rawChatCommand;

		auto chatCommand = static_cast<ChatCommands>(rawChatCommand);
		clientReference->SetChatCommandReceived(chatCommand);

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
			//printf("[System] Received Heartbeat request!\n");
			clientReference->SendHeartbeatResponse();
			return ChatState::SendHeartbeat;
		}
		case ChatCommands::HeartbeatPulse:
		{
			return ChatState::HeartbeatReceived;
		}
		default:
		{
			printf("[System] Unknown chat command received!\n");
		}
		}
		return ChatState::FailedChatCommand;
	}

	ChatState Responses::CheckHTTPURequest(Packet&& packet)
	{
		std::string checkHttpu{};
		const int totalLength = 96;
		
		packet >> checkHttpu;

		if (!checkHttpu.empty())
		{
			if (checkHttpu.length() == totalLength)
			{
				return ChatState::LocalIdentifyValidHTTPU;
			}
		}
		return ChatState::LocalIdentifiedFailed;
	}

	ChatState Responses::IdentifiedSuccessfully(Packet&& packet)
	{
		// Host sending IP+Name to Client
		std::string clientIp{}, clientName{};
		packet >> clientIp;
		packet >> clientName;
		if (clientIp.empty() || clientName.empty())
		{
			return ChatState::LocalIdentifiedFailed;
		}

		clientReference->SetChatParnerIp(clientIp);
		clientReference->SetChatParnerName(clientName);
		return ChatState::LocalIdentifyReceivedSuccess;
	}

	ChatState Responses::ClientAcknowledged(Packet&& packet)
	{
		// Client sending IP+Name to Host
		std::string clientIp{}, clientName{};
		packet >> clientIp;
		packet >> clientName;
		if (clientIp.empty() || clientName.empty())
		{
			return ChatState::LocalIdentifiedFailed;
		}

		clientReference->SetChatParnerIp(clientIp);
		clientReference->SetChatParnerName(clientName);
		return ChatState::ClientAcknowledged;
	}


	ChatState Responses::ChatNameIdentification(Packet&& packet)
	{
		std::string newChatName{};
		packet >> newChatName;
		if (newChatName.empty())
		{
			return ChatState::FailedChatCommand;
		}
		clientReference->SetChatParnerName(newChatName);
		printf("[System] Partner changed the username: as: %s\n", newChatName.c_str());

		return ChatState::ChatCommandReceived;
	}

	ChatState Responses::ChatMessage(Packet&& packet)
	{
		std::string messageReceived{};
		packet >> messageReceived;
		if (!messageReceived.empty())
		{
			printf("[%s]: %s\n", clientReference->GetChatParnetName().c_str(), messageReceived.c_str());
			return ChatState::ChatCommandReceived;
		}
		return ChatState::FailedChatCommand;
	}

}
