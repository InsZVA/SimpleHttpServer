#pragma once
#include "IOCPModule.h"
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#define    WIN32_LEAN_AND_MEAN
#include <windows.h>

class HttpModule
{
public:
	HttpModule();
	~HttpModule();

	static void Serve(IOCPModule* module, IOCPModule::PER_IO_CONTEXT* ioContext, IOCPModule::PER_SOCKET_CONTEXT* socketContext);
	static void SendFile(IOCPModule* module, IOCPModule::PER_IO_CONTEXT* ioContext, IOCPModule::PER_SOCKET_CONTEXT* socketContext,
		HttpRequest* request, std::string filePath, std::string contentType);
	static void FormDataDecode(std::map<std::string, std::vector<std::string>>& formData, std::string source);
};

