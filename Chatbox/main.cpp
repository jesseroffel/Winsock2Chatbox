#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include "Client.h"
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

int main()
{
	//Check whether it's possible to run the chat which requires a second thread to be used to poll incoming messages while typing.
	unsigned int threadsAvailable = 0;
	threadsAvailable = std::thread::hardware_concurrency();
	if (threadsAvailable < 2)
	{
		printf("[System] Chat setup failed because of not enough threads available, minimum required: 2. Amount of threads not supported: %u", threadsAvailable);
		return 1;
	}

	//Setup WSA
	Networking::Client client;
	if (client.InitializeWSA() != Networking::ChatState::WSASetupSuccess)
	{
		printf("[System] WSA Initialization failed: unable to setup the chat.\n");
		return 1;
	}

	//Setup Local network search socket, if this fails then this means the chat can't function at all.
	if (client.InitializeSearchSocket() != Networking::ChatState::WSASetupSocketSuccess)
	{
		printf("[System] Socket Initialization failed: unable to setup the chat.\n");
		client.Close();
		return 1;
	}

	//Chat loop
	while (!client.quitApplication)
	{
		if (client.SetupOrJoinChat() != Networking::ChatState::SuccessfullySetup)
		{
			continue;
		}

		if (client.parnerSetupReady)
		{
			std::thread inputThread(&Networking::Client::HandleChatInput, &client);
			client.HandleChatNetwork();
			if (inputThread.joinable()) { inputThread.join(); }

			client.AskForNewConnection();
		}
	}

	//Cleanup
	client.Close();
	return 0; 

}