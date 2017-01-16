#pragma once

#include "StateMachine.h"
#include <string>
#include <map>
#include <functional>
#include <mutex>

class HttpRequest
{
public:
	HttpRequest(char* buff, unsigned int len);
	~HttpRequest();
	bool parse(char* buff, unsigned int len);
	std::string getMethod();
	std::string getURL();
	std::string getBody();
	std::string getVersion();
	bool isFinished();
	bool responsed = false;
	std::map<std::string, std::string>& getHeader();
private:
	bool finished = false;
	std::ostringstream method;
	std::ostringstream url;
	std::ostringstream version;
	std::map<std::string, std::string> header;
	std::ostringstream bodyStream;
	std::string currentKey;
	std::string currentValue;
	std::mutex mutex;
	int contentLength = 0;
	int contentReceived = 0;
	bool headerPrepared = false;
	enum httpRequestState {requestMethod, requestURL, requestVersion, headerKey, headerValue, body, finish};
	StateMachine<httpRequestState> httpRequestStateM;

	void prepareHeader();
};

