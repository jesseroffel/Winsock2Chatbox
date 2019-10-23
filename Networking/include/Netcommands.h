#pragma once

namespace Networking
{
	enum class Netcommands
	{
		Identify = 600,
		IdentifySuccessful,
		IdentifyFailure,
		Acknowledge,
		AcknowledgeSuccess,
		AcknowledgeFailure,
		ChatCommand,
		Disconnect
	};

	enum class ChatCommands
	{
		NameIdentify,
		Chatmessage,
		HeartbeatCheck,
		HeartbeatPulse
	};
}