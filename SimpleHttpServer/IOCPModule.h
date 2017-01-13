#pragma once

#include "Module.h"
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <functional>
#include <iostream>
#include "Mempool.h"

#pragma comment (lib, "Ws2_32.lib")

//#define DEBUG

#define MAX_BUFFER_LEN 1500 // Maybe MTU?
#define PER_SOCKET_MAX_IO_CONTEXT_NUM 16
#define EXAMPLE_RESPONSE "HTTP/1.1 200 OK\r\nServer:TestC\r\nContent-Type:text/html\r\nContent-Length:5\r\n\r\nHello\r\n\0"

class IOCPModule : Module
{
public:
	// TODO: add thread local mempool
	IOCPModule();
	~IOCPModule();
	static IOCPModule* getInstance();
	virtual bool initialize();
	bool eventLoop();

	enum operation { accept, recv, send };

	struct _PER_IO_CONTEXT;
	struct _PER_SOCKET_CONTEXT;
	typedef void(*EVENT_HANDLER)(IOCPModule::_PER_IO_CONTEXT*, IOCPModule::_PER_SOCKET_CONTEXT*);

	typedef struct _PER_IO_CONTEXT {
		OVERLAPPED overlapped;
		SOCKET socketAccept;
		WSABUF wsaBuf;
		char szBuffer[MAX_BUFFER_LEN];
		operation opType;
		IOCPModule::EVENT_HANDLER ev;

		bool postRecv() {
			if (opType != recv) return false;
			return SOCKET_ERROR != WSARecv(socketAccept, &wsaBuf, 1, 0, 0, &overlapped, NULL) || WSAGetLastError() != 997;
		}
	} PER_IO_CONTEXT;

	typedef struct _PER_SOCKET_CONTEXT {
		SOCKET socket;
		SOCKADDR_IN clientAddr;
		PER_IO_CONTEXT** arrayIoContext;
		int arrayLen;
		int arrayCap;

	private: 
		bool insertIoContext(PER_IO_CONTEXT* ioContext) {
			return true; // for test
			if (arrayLen < arrayCap) {
				arrayIoContext[arrayLen] = ioContext;
				arrayLen++;
			}
			else {
				arrayCap <<= 1;
				arrayIoContext = (PER_IO_CONTEXT**)realloc(arrayIoContext, arrayCap); // TODO: Optimize
				if (arrayIoContext)
					return insertIoContext(ioContext);
				else
					return false;
			}
			return true;
		}

	public:
		bool postSend(const char* buff, int len) {
			PER_IO_CONTEXT* ioContext = IOCPModule::allocIoContext(this->socket, send, IOCPModule::sendHandler);
			memcpy(ioContext->szBuffer, buff, len); // TODO: optimize
			ioContext->wsaBuf.len = len;
			if (!insertIoContext(ioContext))
			{
				IOCPModule::freeIoContext(ioContext);
				return false;
			}
			
			// TODO:len > MTU?
			if (SOCKET_ERROR == WSASend(this->socket, &ioContext->wsaBuf, 1, NULL, 0, &ioContext->overlapped, NULL) &&
				WSAGetLastError() != 997) return false;
		}

		bool close() {
			// TODO: Cancel all IO requests
			// TODO: disconEx
			closesocket(socket);
			return true;
		}
	} PER_SOCKET_CONTEXT;



private:
	static IOCPModule* instance;

	const int LISTEN_PORT = 4024;
	const int PRE_ACCEPTEX_REQUESTS = 8;

	HANDLE iocp = NULL;
	SOCKET listener = NULL;
	LPFN_ACCEPTEX acceptEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS getAcceptexSockAddrs = NULL;

	// Note:
	// ioContext could be returned form acceptex
	// to get the socket, please access socketContext
	static EVENT_HANDLER recvHandler;
	// Note:
	// 
	static EVENT_HANDLER sendHandler;

	bool postAcceptEx(PER_IO_CONTEXT* context);
	bool postRecv(PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext);
	bool doAcceptEx(PER_IO_CONTEXT* context);
	bool doRecv(PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext);

	static PER_IO_CONTEXT* allocIoContext();
	static PER_IO_CONTEXT* allocIoContext(SOCKET acceptSocket, operation opType, EVENT_HANDLER ev);
	static PER_SOCKET_CONTEXT* allocSocketContext();
	static bool freeIoContext(PER_IO_CONTEXT* context);
	static bool freeSocketContext(PER_SOCKET_CONTEXT* context);
};

