#pragma once

#include <winsock2.h>

#include <vector>
#include <iostream>
#include <stdio.h>
#include <cmath>

// http://www.binarytides.com/udp-socket-programming-in-winsock/
#include <stdio.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <xnamath.h>
#include <string>     // std::string, std::stof


#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define SERVER "127.0.0.1"  //ip address of udp server
#define BUFLEN 512  //Max length of buffer
#define PORT 6768   //The port on which to listen for incoming data


// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class EyeTracking
{
public:

	int SoketID;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	char UDPbuf[BUFLEN];

	std::atomic_bool bIsUDPThreadRunning;
	std::thread UDPThread;
	int ThreadDelayMS = 0;

	float deltaPackageTime = 0.0f;
	float lastPackage = 0.0f;

	std::vector<float> LeftEye = { 0.f, 0.f , 0.f };
	std::vector<float> RightEye = { 0.f, 0.f , 0.f };
	std::vector<float> MiddleEye = { 0.f, 0.f , 0.f };


	// Constructor with vectors
	EyeTracking()
	{
		SoketID = 0;
		bIsUDPThreadRunning = false;
	}

	void RunCamerasUDPThread()
	{
		//start communication
		while (bIsUDPThreadRunning == true)
		{
			//clear the buffer by filling null, it might have previously received data
			memset(UDPbuf, '\0', BUFLEN);
			//try to receive some data, this is a blocking call
			if (recvfrom(SoketID, UDPbuf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR)
			{
				printf("recvfrom() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			ParseUDPString(UDPbuf);

			//std::this_thread::sleep_for(std::chrono::milliseconds(ThreadDelayMS));
		}
	}

	//  Listen Cameras UDP packages
	void ListenCamerasUDP()
	{
		WSADATA wsa;

		//Initialise winsock
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("Failed. Error Code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Initialised.\n");

		//create socket
		if ((SoketID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
		{
			printf("socket() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//setup address structure
		memset((char *)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(PORT);
		si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

		if (bind(SoketID, (SOCKADDR *)&si_other, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		{
			printf("socket() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		if (bIsUDPThreadRunning == false)
		{
			bIsUDPThreadRunning = true;
			UDPThread = std::thread([this]() { RunCamerasUDPThread(); });
		}
	}

	void CloseCamerasUDP()
	{

		if (bIsUDPThreadRunning)
		{
			bIsUDPThreadRunning = false;
			UDPThread.join();

			if (SoketID)
			{
				closesocket(SoketID);
				WSACleanup();
			}
		}
	}

	std::vector<float> ConvertCoordToVector(const std::string& EyeString)
	{
		std::string TempString = "";
		std::vector<float> PositionVector = { 0.f, 0.f , 0.f };

		int i;
		bool IsXCoord = true;

		for (i = 0; i <= EyeString.length(); i++)
		{
			if (*(EyeString.c_str() + i) == ',')
			{
				if (IsXCoord)
				{
					PositionVector[0] = std::stof(TempString);
				}
				else
				{
					PositionVector[1] = std::stof(TempString);
				}

				// Reset string
				TempString = "";

				IsXCoord = false;

				// skip this iteration
				continue;
			}

			// last coordinate
			if (EyeString.length() == i)
			{
				PositionVector[2] = std::stof(TempString);
				break;
			}
			TempString += *(EyeString.c_str() + i);
		}

		for (int i = 0; i < PositionVector.size(); i++)
		{
			PositionVector[i] /= 10.f;
		}

		return PositionVector;
	}

	void ParseUDPString(const char* Buffer)
	{
		std::string BufferString(Buffer);

		std::string PositionLeftStartString = "<PositionLeft>";
		std::string PositionLeftStartEnd = "</PositionLeft>";
		int PositionLeftStart = BufferString.find(PositionLeftStartString) + PositionLeftStartString.length();
		int PositionLeftEnd = BufferString.find(PositionLeftStartEnd);
		int PositionLeftCount = PositionLeftEnd - PositionLeftStart;

		std::string PositionRightStartEnd = "</PositionRight>";
		std::string PositionRightStartString = "<PositionRight>";
		int PositionRightStart = BufferString.find(PositionRightStartString) + PositionRightStartString.length();
		int PositionRightEnd = BufferString.find(PositionRightStartEnd);
		int PositionRightCount = PositionRightEnd - PositionRightStart;

		std::string PositionLeftString = BufferString.substr(PositionLeftStart, PositionLeftCount);
		std::string PositionRightString = BufferString.substr(PositionRightStart, PositionRightCount);


		LeftEye = ConvertCoordToVector(PositionLeftString);
		RightEye = ConvertCoordToVector(PositionRightString);
		for (int i = 0; i < MiddleEye.size(); i++)
		{
			MiddleEye[i] = (LeftEye[i] + RightEye[i]) / 2.f;
		}
	}
};