#pragma once

namespace Networking
{
	enum class NetCommands
	{
		Invalid = 600,
		IdentifyBroadcast,
		IdentifySuccessful,
		IdentifyFailure,
		Acknowledge,
		AcknowledgeComplete,
		ChatCommand,
		LostConnection,
		Disconnect
	};

	enum class ChatCommands
	{
		Invalid = 700,
		NameIdentify,
		Chatmessage,
		HeartbeatCheck,
		HeartbeatPulse
	};
}