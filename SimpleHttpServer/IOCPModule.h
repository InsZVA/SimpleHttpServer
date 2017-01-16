#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <functional>
#include <iostream>
#include "Mempool.h"
#include "HttpRequest.h"
#include <atomic>
#include <thread>
#include <queue>

#pragma comment (lib, "Ws2_32.lib")
//#define DEBUG

#define MAX_BUFFER_LEN 1500 // Maybe MTU?
#define PER_SOCKET_MAX_IO_CONTEXT_NUM 16
#define EXAMPLE_RESPONSE "HTTP/1.1 200 OK\r\nServer:TestC\r\nContent-Type:text/html\r\nContent-Length:5\r\n\r\nHello\r\n\0"

class IOCPModule
{
public:
	// TODO: add thread local mempool
	IOCPModule() = delete;
	IOCPModule(Mempool *mempool);
	~IOCPModule();

	static bool initialize();
	bool eventLoop();


	enum operation { accept, recv, send, disconnect };

	struct _PER_IO_CONTEXT;
	struct _PER_SOCKET_CONTEXT;
	typedef void(*EVENT_HANDLER)(IOCPModule* module, IOCPModule::_PER_IO_CONTEXT*, IOCPModule::_PER_SOCKET_CONTEXT*);

	// If the send buffer is smaller than 1500
	// Allocate in a struct. Otherwise allocate in heap
	typedef struct _PER_IO_CONTEXT {
		OVERLAPPED overlapped;
		SOCKET socketAccept;
		operation opType;
		IOCPModule::EVENT_HANDLER ev;
		WSABUF wsaBuf;
		// WARING: Internel use, DO NOT USE outside
		// To read the buffer, please read wsabuf
		// Because sometimes this struct was changed from LARGE_SEND_CONTEXT
		char szBuffer[MAX_BUFFER_LEN];

	public:bool postRecv(IOCPModule*, IOCPModule::_PER_IO_CONTEXT*, IOCPModule::_PER_SOCKET_CONTEXT*);
	} PER_IO_CONTEXT;

	// If the send buffer is smaller than 1500
	// Allocate in a struct. Otherwise allocate in heap
	typedef struct _LARGE_SEND_CONTEXT {
		OVERLAPPED overlapped;
		SOCKET socketAccept;
		operation opType;
		IOCPModule::EVENT_HANDLER ev;
		WSABUF wsaBuf;
	} LARGE_SEND_CONTEXT;

	typedef struct _PER_SOCKET_CONTEXT {
		SOCKET socket;
		SOCKADDR_IN clientAddr;
		PER_IO_CONTEXT** arrayIoContext;
		int arrayLen;
		int arrayCap;
		HttpRequest* request;
	private: 
		bool insertIoContext(PER_IO_CONTEXT* ioContext);
	public:
		bool postSend(IOCPModule*, const char* buff, int len);
		bool close(IOCPModule*);
	} PER_SOCKET_CONTEXT;

	PER_IO_CONTEXT* allocIoContext();
	PER_IO_CONTEXT* allocIoContext(SOCKET acceptSocket, operation opType, EVENT_HANDLER ev);
	LARGE_SEND_CONTEXT* allocSendContext(SOCKET acceptSocket, unsigned int size, EVENT_HANDLER ev);
	PER_SOCKET_CONTEXT* allocSocketContext();
	bool freeIoContext(PER_IO_CONTEXT* context);
	bool freeSocketContext(PER_SOCKET_CONTEXT* context);
	bool freeSendContext(LARGE_SEND_CONTEXT* context);
private:
	static std::atomic_int *startNum;

	static const int LISTEN_PORT = 4024;
	static const int PRE_ACCEPTEX_REQUESTS = 8;

	static HANDLE iocp;
	static SOCKET listener;
	static LPFN_ACCEPTEX acceptEx;
	static LPFN_GETACCEPTEXSOCKADDRS getAcceptexSockAddrs;
	static LPFN_DISCONNECTEX disconnectEx;

	std::queue<SOCKET> reusedSocketQueue;

	Mempool *mempool = NULL;

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

	// This four functions push closed socket in a queue
	// For future usage with AcceptEx
	bool postDisconnect(SOCKET s);
	bool doDisconnect(SOCKET s);
	SOCKET getReusedSocket();
	bool freeReusedSocket(SOCKET);

	typedef struct _DISCONNECT_CONTEXT {
		OVERLAPPED overlapped;
		SOCKET socket;
		operation opType;
	} DISCONNECT_CONTEXT;
};

