#include "stdafx.h"
#include "HttpRequest.h"


HttpRequest::HttpRequest(char* buf, unsigned int len) // TODO: maybe bug
{
	auto requestMethodHandler = [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		char c = currentStream.get();
		while (!currentStream.eof()) {
			if (c == ' ') return requestURL;
			method.put(c);
			c = currentStream.get();
		}
		return requestMethod;
	};

	httpRequestStateM.addStep(requestMethod, requestMethodHandler);

	httpRequestStateM.addStep(requestURL, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		char c = currentStream.get();
		while (!currentStream.eof()) {
			if (c == ' ') return requestVersion;
			url.put(c);
			c = currentStream.get();
		}
		return requestURL;
	});

	httpRequestStateM.addStep(requestVersion, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		char c = currentStream.get();
		while (!currentStream.eof()) {
			if (c == '\r') {
				c = currentStream.get();
				if (currentStream.eof()) {
					currentStream.putback('\r');
					return requestVersion;
				}
				if (c == '\n')
					return headerKey;
				else
				{
					currentStream.putback(c);
					c = '\r';
				}
			}
			version.put(c);
			c = currentStream.get();
		}
		return requestVersion;
	});

	httpRequestStateM.addStep(headerKey, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		char c = currentStream.get();
		while (!currentStream.eof()) {
			if (c == ':') {
				currentValue = "";
				return headerValue;
			}
			currentKey += c;
			c = currentStream.get();
		}
		return headerKey;
	});

	httpRequestStateM.addStep(headerValue, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		char c = currentStream.get();
		while (!currentStream.eof()) {
			if (c == '\r') {
				c = currentStream.get();
				if (currentStream.eof()) {
					currentStream.putback('\r');
					return headerValue;
				}
				if (c == '\n') {
					c = currentStream.get();
					if (currentStream.eof()) {
						header[currentKey] = currentValue;
						return headerValue;
					}
					if (c == '\r') {
						c = currentStream.get();
						if (currentStream.eof()) {
							currentStream.putback('\r');
							return headerValue;
						}
						if (c == '\n') {
							header[currentKey] = currentValue;
							return body;
						}
						currentStream.putback(c);
						currentStream.putback('\r');
						currentKey == "";
						return headerKey;
					}
					currentStream.putback(c);
					header[currentKey] = currentValue;
					currentKey = "";
					return headerKey;
				}
				else
				{
					currentStream.putback(c);
					c = '\r';
				}
			}
			currentValue += c;
			c = currentStream.get();
		}
		return headerValue;
	});

	httpRequestStateM.addStep(body, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		prepareHeader();
		char c = currentStream.get();
		while (!currentStream.eof() && c != 0) { // c == 0 -> wait for next request(body)
			bodyStream.put(c);
			contentReceived++;
			if (contentReceived >= contentLength && contentLength != 0) {
				finished = true;
				return finish;
			}
			c = currentStream.get();
		}
		if (contentLength != 0)
			return body;
		else
		{
			finished = true;
			return finish;
		}
	});

	httpRequestStateM.addStep(finish, [this](httpRequestState start, std::stringstream& currentStream) -> httpRequestState {
		return finish;
	});

	finished = parse(buf, len);
}


HttpRequest::~HttpRequest()
{
}

bool HttpRequest::parse(char* buf, unsigned int len) {
	std::lock_guard<std::mutex> lock(mutex);
	if (finished) return true;
	std::string buffer(buf, len);
	httpRequestStateM.produce(buffer);
	return finished;
}

void HttpRequest::prepareHeader() {
	if (headerPrepared) return;
	for (auto& h : header) {
		/*
		std::string upper;
		for (auto c : h.first) {
			if (c >= 'a' && c <= 'z')
				upper += c + 'A' - 'a';
			else
				upper += c;
		}
		header[upper] = h.second;
		*/
		if (h.first == "Content-Length") {
			contentLength = std::atoi(h.second.c_str());
		}
	}
	headerPrepared = true;
}

std::string HttpRequest::getMethod() {
	return method.str();
}

std::string HttpRequest::getURL() {
	return url.str();
}

std::string HttpRequest::getBody() {
	return bodyStream.str();
}

std::string HttpRequest::getVersion() {
	return version.str();
}

bool HttpRequest::isFinished() {
	return finished;
}

std::map<std::string, std::string>& HttpRequest::getHeader() {
	return header;
}