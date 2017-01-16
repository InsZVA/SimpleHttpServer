#include "stdafx.h"
#include "HttpModule.h"

#define SUCCESS_RESPONSE "<html><body>Success</body></html>"
#define FAIL_RESPONSE "<html><body>Fail</body></html>"

HttpModule::HttpModule()
{
}


HttpModule::~HttpModule()
{
}

void HttpModule::Serve(IOCPModule* module, IOCPModule::PER_IO_CONTEXT* ioContext, IOCPModule::PER_SOCKET_CONTEXT* socketContext) {
	HttpRequest *request;
	if (socketContext->request) {

		request = socketContext->request;
		if (socketContext->request->responsed) {
			delete request;
			request = new HttpRequest(ioContext->wsaBuf.buf, ioContext->overlapped.InternalHigh);
			socketContext->request = request;
		} else 
			request->parse(ioContext->wsaBuf.buf, ioContext->overlapped.InternalHigh);
	}
	else {
		request = new HttpRequest(ioContext->wsaBuf.buf, ioContext->overlapped.InternalHigh);
		socketContext->request = request;
	}
	if (!request->isFinished()) return;
	std::string method = request->getMethod();
	if (method == "GET") {
		auto url = request->getURL();
		if (url == "/index.html") {
			SendFile(module, ioContext, socketContext, request, "./www/index.html", "text/html");
		}
		else if (url == "/plain.txt") {
			SendFile(module, ioContext, socketContext, request, "./www/plain.txt", "text/plain");
		}
		else if (url == "/img.html") {
			SendFile(module, ioContext, socketContext, request, "./www/img.html", "text/html");
		}
		else if (url == "/1.png") {
			SendFile(module, ioContext, socketContext, request, "./www/1.png", "image/png");
		}
		else {
			std::string notFound = request->getVersion() + " " + "404 Not Found\r\n";
			socketContext->postSend(module, notFound.c_str(), notFound.length());
		}
	}
	else if (method == "POST") {
		std::map<std::string, std::vector<std::string>> formData;
		FormDataDecode(formData, request->getBody());
		std::ostringstream out;
		out << request->getVersion() << " 200 OK\r\n" << "Content-Type: text/html\r\n" << "Server: InsHttpBeta 17.01\r\n";
		if (formData["login"].size() > 0 && formData["login"][0] == "3140104024" &&
			formData["pass"].size() > 0 && formData["pass"][0] == "4024") {
			out << "Content-Length:" << sizeof(SUCCESS_RESPONSE) - 1 << "\r\n\r\n" << SUCCESS_RESPONSE;
		}
		else {
			out << "Content-Length:" << sizeof(FAIL_RESPONSE) - 1 << "\r\n\r\n" << FAIL_RESPONSE;
		}
		auto outStr = out.str();
		socketContext->postSend(module, outStr.c_str(), outStr.length());
	}
	socketContext->request->responsed = true;
}

void HttpModule::SendFile(IOCPModule* module, IOCPModule::PER_IO_CONTEXT* ioContext, IOCPModule::PER_SOCKET_CONTEXT* socketContext,
	HttpRequest* request, std::string filePath, std::string contentType) {

	HANDLE fp = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE,	FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fp == NULL)
	{
		std::string notFound = request->getVersion() + " " + "404 Not Found\r\n";
		socketContext->postSend(module, notFound.c_str(), notFound.length());
		return;
	}

	DWORD dwBytesInBlock = GetFileSize(fp, NULL);

	HANDLE hFileMapping = CreateFileMapping(fp, NULL, PAGE_READWRITE, (DWORD)(dwBytesInBlock >> 16), (DWORD)(dwBytesInBlock & 0x0000FFFF), NULL);

	int dwError = GetLastError();
	CloseHandle(fp);

	__int64 qwFileOffset = 0;

	LPVOID pbFile = (LPVOID)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, (DWORD)(qwFileOffset >> 32), (DWORD)(qwFileOffset & 0xFFFFFFFF),
		dwBytesInBlock);


	std::ostringstream out;
	out << request->getVersion() << " 200 OK\r\nContent-Length:" << dwBytesInBlock << "\r\n"
		<< "Server:" << "InsHttpBeta 17.01\r\n"
		<< "Content-Type:" << contentType << "\r\n" << "\r\n";
	auto outStr = out.str();
	char* buff = (char*)malloc(outStr.length() + dwBytesInBlock); //TODO: Optimize
	memcpy(buff, outStr.c_str(), outStr.length());
	memcpy(buff + outStr.length(), pbFile, dwBytesInBlock);

	UnmapViewOfFile(pbFile);
	CloseHandle(hFileMapping);

	socketContext->postSend(module, buff, outStr.length() + dwBytesInBlock);
}

// TODO: support url-encode/url-decode
void HttpModule::FormDataDecode(std::map<std::string, std::vector<std::string>>& formData, std::string source) {
	std::istringstream in(source);
	std::string currentKey;
	std::string currentValue;
	bool parsingKey = true;
	char c = in.get();
	for (; !in.eof(); c = in.get()) {
		if (parsingKey) {
			if (c == '=') {
				parsingKey = false;
				currentValue = "";
				continue;
			}
			currentKey += c;
		}
		else {
			if (c == '&') {
				if (formData[currentKey].size() == 0) {
					formData[currentKey] = std::vector < std::string > {currentValue};
				}
				else {
					auto v = formData[currentKey];
					v.push_back(currentValue);
					formData[currentKey] = v;
				}
				currentKey = "";
				parsingKey = true;
				continue;
			}
			currentValue += c;
		}
	}
	if (formData[currentKey].size() == 0) {
		formData[currentKey] = std::vector < std::string > {currentValue};
	}
	else {
		auto v = formData[currentKey];
		v.push_back(currentValue);
		formData[currentKey] = v;
	}
}