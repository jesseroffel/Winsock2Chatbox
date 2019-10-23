#pragma once

namespace Networking
{
	class Packet;
	class Client;
	class Responses
	{
	public:
		bool HandlePacket(Packet&& packet);
		bool HandleChatpacket(Packet&& packet);

		void SetClientReference(Client* client = nullptr) { if (client != nullptr) { clientReference = client; } }
	private:
		Client* clientReference = nullptr;
		bool IdentifiedCorrectly(Packet&& packet);
		bool IdentifiedSuccessfully(Packet&& packet);
		bool AcknowledgeClient(Packet&& packet);
		bool AcknowledgeClientSuccess(Packet&& packet);
		bool AcknowledgeClientFailure(Packet&& packet);
		bool ChatNameIdentification(Packet&& packet);
		bool ChatMessage(Packet&& packet);
	};



}