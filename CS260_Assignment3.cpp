// Jonathan Bourim, 011/10/2020
#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <chrono>

#define TCP_PAYLOAD 4096
#define SLEEP_TIME 100

void WSAErrorExit(const char* err, const bool cleanup = true)
{
	std::cerr << err << ": " << WSAGetLastError() << std::endl;

	// Attempt to clean up if the error isn't on start/cleanup
	if (cleanup)
	{
		const int res = WSACleanup();

		if (res != 0) 
			WSAErrorExit("Error in cleaning up WSA");
	}
}

int CreateAddress(sockaddr_in& addr, char* ip, int port)
{
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	int result = 0;

	if (ip == nullptr)
	{
		addr.sin_addr.S_un.S_addr = INADDR_ANY;
	}
	else
	{
		result = inet_pton(AF_INET, ip, &addr.sin_addr);
	}

	return result;
}

int Listen(SOCKET sock, int backlog)
{
	int max = backlog;
	if (max < 1)
		max = SOMAXCONN;

	return listen(sock, max);
}

int CreateSocket(SOCKET& sock, int sockType)
{
	int result = 0;
	// Create socket
	SOCKET sock = socket(AF_INET, sockType, 0);
	if (sock == INVALID_SOCKET)
	{
		result = 1;
	}
	return result;
}


int main(int argc, char** argv)
{
	
	// Initialize WSA, error check
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		WSAErrorExit("Error in starting up WSA", false);
		return 1;
	}


	const int port = *reinterpret_cast<int*>(argv[1]);
	if (port < 0 || port > 65535)
	{
		WSAErrorExit("Received invalid port as program argument.\n Please provide a port between 0 and 65535.");
		return 1;
	}

	// Create listening socket
	SOCKET listenSocket = {};
	if (CreateSocket(listenSocket, SOCK_STREAM) != 0)
	{
		WSAErrorExit("Failed to create listening socket");
		return 1;
	}

	// Create socket address for receiving the address
	sockaddr_in address = { };
	if (CreateAddress(address, nullptr, port) != 0)
	{
		WSAErrorExit("Error in creating socket");
		return 1;
	}

	// Set as non-blocking socket
	unsigned long nonBlock = 1;
	result = ioctlsocket(listenSocket, FIONBIO, &nonBlock);
	if (result == SOCKET_ERROR)
	{
		WSAErrorExit("Error in attempting to set listening socket to non-blocking");
		return 1;
	}

	result = bind(listenSocket, (sockaddr*)&address, sizeof(sockaddr_in));
	if (result == SOCKET_ERROR)
	{
		WSAErrorExit("Error in attempting to bind listening socket");
		return 1;
	}

	while(true)
	{
		
	}


	// // Attempt to connect to the server
	// int error;
	// while(true)
	// {
	// 	result = connect(tcpSock, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr_in));
	// 	if (result == SOCKET_ERROR)
	// 	{
	// 		error = WSAGetLastError();
	// 		// Connected, we can continue application
	// 		if (error == WSAEISCONN)
	// 			break;
	//
	// 		// Not one of the expected errors to try again, fatal
	// 		if (!(error == WSAEINVAL || error == WSAEALREADY || error == WSAEWOULDBLOCK))
	// 		{
	// 			WSAErrorExit("Error connecting to the server", true);
	// 			return 1;
	// 		}
	// 	}
	// }

	// Send while we have data to send
	int dataSent = 0;
	int dataLen = static_cast<int>(strlen(argv[0]));
	char* buf = argv[0];
	do
	{
		result = send(tcpSock, buf, dataLen, 0);
		if (result == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			// Try again as we are getting blocked
			if (error == WSAEWOULDBLOCK)
				continue;

			WSAErrorExit("Error in sending data");
			return 1;
		}
		// Increase data sent and data pointer
		dataSent += result;
		buf += result;

	} while (dataSent < dataLen);
	// Shutdown sending
	shutdown(tcpSock, SD_SEND);


	// Create a buffer with payload in bytes
	char buffer[TCP_PAYLOAD] = { 0 };
	int remaining = TCP_PAYLOAD;
	char* bufPtr = buffer;
	
	// Listen for a response, then print it to the console when fully constructed.
	do
	{
		result = recv(tcpSock, bufPtr, remaining, 0);
		if (result == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			// Print period and sleep some time before trying again
			if (error == WSAEWOULDBLOCK)
			{
				std::cout << ". ";
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
				continue;
			}

			WSAErrorExit("Error in receiving data");
			return 1;
		}

		// Increment buffer pointer and decrease remaining size
		bufPtr += result;
		remaining -= result;
	} while (result != 0);

	std::cout << buffer << std::endl;
	
	// Close and cleanup
	result = closesocket(tcpSock);
	if (result == SOCKET_ERROR)
	{
		WSAErrorExit("Error in closing the socket");
		return 1;
	}

	result = WSACleanup();
	if (result != 0)
	{
		WSAErrorExit("Error in shutting down WSA", false);
		return 1;
	}

	return 0;
}
