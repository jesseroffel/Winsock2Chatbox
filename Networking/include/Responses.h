#pragma once
namespace Networking
{
	class Packet;
	class Client;

	class Responses 
	{
	public:
		ChatState HandlePacket(Packet&& packet);
		ChatState HandleChatpacket(Packet&& packet);

		void SetClientReference(Client* client = nullptr) { if (client != nullptr) { clientReference = client; } }
	private:
		Client* clientReference = nullptr;
		ChatState CheckHTTPURequest(Packet&& packet);
		ChatState IdentifiedSuccessfully(Packet&& packet);
		ChatState ClientAcknowledged(Packet&& packet);
		void AcknowledgeComplete();

		ChatState ChatNameIdentification(Packet&& packet);
		ChatState ChatMessage(Packet&& packet);
	};



}