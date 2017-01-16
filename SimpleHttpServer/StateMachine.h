#pragma once

#include <map>
#include <sstream>
#include <functional>

template <typename E> class StateMachine
{
public:
	StateMachine() {};
	~StateMachine() {};
	void StateMachine<E>::addStep(E start, std::function<E(E start, std::stringstream& currentStream)> handler) {
		stateHandlerMap[start] = handler;
	}
	E StateMachine<E>::produce(std::string buffer) {
		if (currentStream.eof()) { //!!! If the stream is over, must reset it
			currentStream.clear();
		}
		currentStream << buffer;
		for (auto next = stateHandlerMap[currentState](currentState, currentStream); next != currentState; ) {
			currentState = next;
			next = stateHandlerMap[currentState](currentState, currentStream);
		}
		return currentState;
	}
protected:
	E currentState = E(0);
	std::map<E, std::function<E(E start, std::stringstream& currentStream)>> stateHandlerMap;
	std::stringstream currentStream;
};

