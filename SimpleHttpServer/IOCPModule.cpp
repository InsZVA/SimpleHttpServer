#include "stdafx.h"
#include "IOCPModule.h"
#include <thread>

//#define DEBUG

IOCPModule* IOCPModule::instance = nullptr;
IOCPModule::EVENT_HANDLER IOCPModule::recvHandler =
[](PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext) {
#ifdef DEBUG
	std::cout << "Recv:" << ioContext->overlapped.InternalHigh << std::endl;
	for (auto i = 0; i < ioContext->overlapped.InternalHigh; i++) {
		printf("%x ", ioContext->szBuffer[i]);
	}
#endif
	//std::cout << std::endl;
	if (!socketContext->postSend(EXAMPLE_RESPONSE, sizeof(EXAMPLE_RESPONSE)) && WSAGetLastError() != 997) {
#ifdef DEBUG
		std::cout << "Error Code:" << WSAGetLastError() << std::endl;
#endif
	}
};

IOCPModule::EVENT_HANDLER IOCPModule::sendHandler =
[](PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext) {
	socketContext->close();
	IOCPModule::freeIoContext(ioContext);
};


IOCPModule* IOCPModule::getInstance() {
	if (instance == nullptr) 
		instance = new IOCPModule();
	return instance;
}

IOCPModule::IOCPModule()
{
}


IOCPModule::~IOCPModule()
{
	WSACleanup();
}

bool IOCPModule::initialize() {
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); // Num of CPU
	WSADATA wsaData;
	auto ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != NULL) return false;

	struct sockaddr_in serverAddress;
	listener = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listener == INVALID_SOCKET) return false;

	ZeroMemory((char*)&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
	serverAddress.sin_port = htons(LISTEN_PORT);

	if (SOCKET_ERROR == bind(listener, (struct sockaddr*)&serverAddress, sizeof(serverAddress)))
		return false;
	if (SOCKET_ERROR == listen(listener, SOMAXCONN))
		return false;
	if(CreateIoCompletionPort((HANDLE)listener, iocp, 0, 0) == NULL) return false;

	// get AcceptEx process pointer
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx),
		&acceptEx, sizeof(acceptEx), &dwBytes, NULL, NULL);

	// get GetAcceptExSockAddrs process pointer
	GUID guidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockAddrs, sizeof(guidGetAcceptExSockAddrs),
		&getAcceptexSockAddrs, sizeof(getAcceptexSockAddrs), &dwBytes, NULL, NULL);

	// Post a acceptex request;
	for (auto i = 0; i < PRE_ACCEPTEX_REQUESTS; i++) {
		PER_IO_CONTEXT *ioContext = allocIoContext();
		if (ioContext == NULL) return false;
		if (acceptEx(listener, ioContext->socketAccept, ioContext->wsaBuf.buf, ioContext->wsaBuf.len - 2 * (sizeof(sockaddr_in) + 16),
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, 0, (LPOVERLAPPED)&ioContext->overlapped) == FALSE && 
			WSAGetLastError() != 997 && WSAGetLastError() != 0) {
#ifdef DEBUG
			std::cout << "Error code:" << WSAGetLastError() << std::endl;
#endif
			closesocket(ioContext->socketAccept);
			freeIoContext(ioContext);
			return false;
		}
		/*if (CreateIoCompletionPort((HANDLE)ioContext->socketAccept, iocp, 0, 0) == NULL) {
			closesocket(ioContext->socketAccept);
			freeIoContext(ioContext);
			return false;
		}*/
	}
	return true;
}

// allocate a io context for accpetex
// this function first create a socket for future
IOCPModule::PER_IO_CONTEXT* IOCPModule::allocIoContext() {
	SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (acceptSocket == INVALID_SOCKET) return NULL;

	return allocIoContext(acceptSocket, accept, NULL);
}

// allocate a io context for wsarecv/wsasend
// this function dosn't create new socket
IOCPModule::PER_IO_CONTEXT* IOCPModule::allocIoContext(SOCKET acceptSocket, operation opType = recv, EVENT_HANDLER ev = recvHandler) {
	PER_IO_CONTEXT *ioContext = (PER_IO_CONTEXT*)malloc(sizeof(PER_IO_CONTEXT)); // TODO: Optimize using cache
	ZeroMemory(ioContext, sizeof(PER_IO_CONTEXT));
	ioContext->opType = opType;
	ioContext->socketAccept = acceptSocket;
	ioContext->wsaBuf.len = MAX_BUFFER_LEN;
	ioContext->wsaBuf.buf = ioContext->szBuffer;
	ioContext->ev = ev ? ev : recvHandler;
	return ioContext;
}

IOCPModule::PER_SOCKET_CONTEXT* IOCPModule::allocSocketContext() {
	auto ret = (PER_SOCKET_CONTEXT*)malloc(sizeof(PER_SOCKET_CONTEXT));
	ZeroMemory(ret, sizeof(PER_SOCKET_CONTEXT));
	ret->arrayLen = 0;
	ret->arrayCap = 8;
	ret->arrayIoContext = (PER_IO_CONTEXT**)malloc(sizeof(PER_IO_CONTEXT) * ret->arrayCap);
	return ret;
}

bool IOCPModule::freeSocketContext(PER_SOCKET_CONTEXT* context) {
	free(context);
	return true;
}

bool IOCPModule::freeIoContext(PER_IO_CONTEXT* context) {
	free(context);
	return true;
}

bool IOCPModule::eventLoop() {
	
	DWORD nBytes;
	PER_SOCKET_CONTEXT* socketContext;
	LPOVERLAPPED lpoverlapped;
	if (GetQueuedCompletionStatus(iocp, &nBytes, (PULONG_PTR)&socketContext, &lpoverlapped, 1000) == FALSE) {
		return true;
	};
	PER_IO_CONTEXT* context = CONTAINING_RECORD(lpoverlapped, PER_IO_CONTEXT, overlapped);

	switch (context->opType) {
	case accept:
		return doAcceptEx(context);
	case recv:
		if (socketContext == NULL) { 
#ifdef DEBUF
			std::cout << "why socketContext == NULL? some recv requests error?" << std::endl;
#endif
			return false;
		}
		return doRecv(context, socketContext);
	case send:
#ifdef DEBUF
		std::cout << "sended" << std::endl;
#endif
		// TODO: optimize
		context->ev(context, socketContext);
		freeIoContext(context);
		return true;
	default:
		return false;
	}
	
	return true;
}

// post a acceptex request to iocp
// this function will create a new socket for future
bool IOCPModule::postAcceptEx(PER_IO_CONTEXT* context) {
	SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (acceptSocket == INVALID_SOCKET) return false;
	context->opType = accept;
	context->socketAccept = acceptSocket;
	if (acceptEx(listener, context->socketAccept, context->wsaBuf.buf, context->wsaBuf.len - 2 * (sizeof(sockaddr_in) + 16),
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, 0, (LPOVERLAPPED)&context->overlapped) == FALSE &&
		WSAGetLastError() != 997 && WSAGetLastError() != 0) {
		closesocket(context->socketAccept);
		return false;
	}
	return true;
}

bool IOCPModule::doAcceptEx(PER_IO_CONTEXT* context) {
	// get client addr
	SOCKADDR_IN *clientAddr = NULL, *localAddr = NULL;
	int remoteLen  = sizeof(SOCKADDR_IN);
	int localLen = remoteLen;
	getAcceptexSockAddrs(context->wsaBuf.buf, context->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&localAddr, &localLen, (LPSOCKADDR*)&clientAddr, &remoteLen);
#ifdef DEBUG
	std::cout << "Client:  " << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port) << "   has been accepted." << std::endl;
#endif
	SOCKET recvSocket = context->socketAccept;

	// allocate new ioContext and socketContext
	PER_IO_CONTEXT* recvIoContext = allocIoContext(recvSocket);
	if (recvIoContext == NULL) return false;
	PER_SOCKET_CONTEXT* recvSocketContext = allocSocketContext();
	if (recvSocketContext == NULL) {
		freeIoContext(recvIoContext);
		return false;
	}
	memcpy(&recvSocketContext->clientAddr, clientAddr, sizeof(SOCKADDR_IN));
	recvSocketContext->socket = recvSocket;

	// bind new request to iocp
	if (CreateIoCompletionPort((HANDLE)recvIoContext->socketAccept, iocp, (ULONG_PTR)&recvSocketContext, 0) == NULL) {
		closesocket(recvIoContext->socketAccept);
		freeIoContext(recvIoContext);
		freeSocketContext(recvSocketContext);
		return false;
	}

	// call recv handler
	recvIoContext->ev(context, recvSocketContext);

	// post next accept request to listener
	if (postAcceptEx(context) == false) {
		freeIoContext(context);
		return false;
	}

	// post a recv request to iocp
	memcpy(&recvSocketContext->clientAddr, clientAddr, sizeof(SOCKADDR_IN));
	recvSocketContext->socket = recvSocket;
	if (postRecv(recvIoContext, recvSocketContext) == false) {
		freeIoContext(recvIoContext);
		freeSocketContext(recvSocketContext);
		return false;
	}
	return true;
}

bool IOCPModule::doRecv(PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext) {
	ioContext->ev(ioContext, socketContext); 

	// post a recv request to iocp
	if (postRecv(ioContext, socketContext) == false) {
		freeIoContext(ioContext);
		freeSocketContext(socketContext);
		return false;
	}

	return true;
}

bool IOCPModule::postRecv(PER_IO_CONTEXT* ioContext, PER_SOCKET_CONTEXT* socketContext) {
	return ioContext->postRecv();
}