#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include "Client.h"
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

int main()
{
	Networking::Client client;
	if (client.InitializeWSA())
	{
		printf("[Setup] Unable to setup the chat.\n");
		return 1;
	}
	if (client.InitializeSearchSocket())
	{
		client.Close();
		return 1;
	}

	if (client.SetupChatbox())
	{
		unsigned int threadsAvailable = 0;
		threadsAvailable = std::thread::hardware_concurrency();
		if (threadsAvailable < 2)
		{
			printf("[Thread] Amount of threads not supported: %u", threadsAvailable);
			return 1;
		}
		else
		{
			std::thread inputThread(&Networking::Client::HandleChatInput, &client);
			client.Chat();
			if (inputThread.joinable())
			{
				inputThread.join();
			}
			client.Close();
		}
		
		return 0;
	}

	client.Close();
	return 0;
}