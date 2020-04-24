#pragma once

namespace Networking
{
	enum class ChatState
	{
		//Setup WSA/Sockets
		WSAStartupFailed = 900,
		WSAOverlapEventFailed,
		WSASetupSuccess,
		WSASetSocketSetupFailed,
		WSASetSocketTimeoutFailed,
		WSASetSocketReusableFailed,
		WSASetSocketBroadcastFailed,
		WSAConnectToSocketFailed,
		WSABindSocketFailed,
		WSAListenToSocketFailed,
		WSASetupSocketSuccess,
		WSAAcceptSuccess,
		WSAAcceptFailed,


		//Packets
		WSAWaitForEventsFailed,
		WSAResetEventFailed,
		WSAOverlappedResultFailed,

		//ReceivingPackets
		WSAReceiveFromFailed,
		WSAReceiveTimeout,
		WSAReceivedNoBytes,
		WSAReceivedInvalidPacket,

		//SendingPackets
		WSASendToFailed,
		WSASendPacketSuccess,

		//Network Responses
		LocalIdentifyValidHTTPU,
		LocalIdentifiedFailed,
		LocalIdentifyReceivedSuccess,
		LocalIdentifyReceivedFailed,
		LocalIdentifyTimeout,
		LocalAvailableChatFound,
		LocalNoAvailableChats,
		LocalIdentifyNetworkSendFailed,

		//Acknowledgements
		ClientAcknowledged,
		ClientAcknowledgeSuccess,
		ClientAcknowledgeFailed,


		//Connection Status
		ChatCommandReceived,
		FailedChatCommand,
		SendHeartbeat,
		HeartbeatReceived,
		ExitApplication,
		NotSet,
		FailedToSetup,
		SuccessfullySetup,
		ChatClosed

	};
}