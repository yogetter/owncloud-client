/**
 * Copyright (c) 2000-2013 Liferay, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#include "CommunicationSocket.h"
#include "UtilConstants.h"
#include "StringUtil.h"

#include <WinSock2.h>
#include <Ws2def.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>

#include <fstream> 

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <future>


// shared stuff:
std::deque<std::packaged_task<void()>> tasks;
std::mutex tasks_mutex;
std::atomic<bool> is_running;

#define BUFSIZE 1024

using namespace std;

#define DEFAULT_BUFLEN 4096

CommunicationSocket::CommunicationSocket(int port)
	: _port(port), _clientSocket(INVALID_SOCKET)
{
}

CommunicationSocket::~CommunicationSocket()
{
	Close();
}

bool CommunicationSocket::Close()
{
	WSACleanup();
	bool closed = (closesocket(_clientSocket) == 0);
	shutdown(_clientSocket, SD_BOTH);
	_clientSocket = INVALID_SOCKET;
	return closed;
}

void  CommunicationSocket::PersistantConnect()
{
	is_running = true;
	auto funcConnect = []() { 
		bool connected = connect();
		while (!connected){
			connected = connect();
		}
	};

	auto funcSync = []() {
		while (is_running) {
				{
					std::unique_lock<std::mutex> lock(tasks_mutex);
					while (!tasks.empty()) {
						auto task(std::move(tasks.front()));
						tasks.pop_front();

						lock.unlock();
						task();
						lock.lock();
					}
				}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	};
	
	std::thread fs(funcSync);

	auto func = [](decltype(funcConnect) func) {
		std::packaged_task<void()> task(func);
		std::future<void> result = task.get_future();

		{
			std::lock_guard<std::mutex> lock(tasks_mutex);
			tasks.push_back(std::move(task));
		}

		// wait on result
		result.get();
	};

	auto result = std::async(std::launch::async, func);


	is_running = false;
	fs.join();
}

bool CommunicationSocket::Connect()
{
	WSADATA wsaData;

	HRESULT iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != NO_ERROR) {
		int error = WSAGetLastError();
	}


	_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_clientSocket == INVALID_SOCKET) {
		//int error = WSAGetLastError();
		Close();
		return false;
	}

	struct sockaddr_in clientService;

	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(PLUG_IN_SOCKET_ADDRESS);
	clientService.sin_port = htons(_port);

	iResult = connect(_clientSocket, (SOCKADDR*)&clientService, sizeof(clientService));
	DWORD timeout = 500; // ms
	setsockopt(_clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(DWORD));

	if (iResult == SOCKET_ERROR) {
		//int error = WSAGetLastError();
		Close();
		return false;
	}
	return true;
}

bool CommunicationSocket::SendMsg(const wchar_t* message)
{
	const char* utf8_msg = StringUtil::toUtf8(message);
	size_t result = send(_clientSocket, utf8_msg, (int)strlen(utf8_msg), 0);
	delete[] utf8_msg;

	if (result == SOCKET_ERROR) {
		//int error = WSAGetLastError();
		closesocket(_clientSocket);
		return false;
	}

	return true;

}

bool CommunicationSocket::ReadLine(wstring* response)
{
	if (!response) {
		return false;
	}

	vector<char> resp_utf8;
	char buffer;
	while (true) {
		int bytesRead = recv(_clientSocket, &buffer, 1, 0);
		if (bytesRead <= 0) {
			response = 0;
			return false;
		}

		if (buffer == '\n')	{
			resp_utf8.push_back(0);
			*response = StringUtil::toUtf16(&resp_utf8[0], resp_utf8.size());
			return true;
		} else {
			resp_utf8.push_back(buffer);
		}
	}
}
