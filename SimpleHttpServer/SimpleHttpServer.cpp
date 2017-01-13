// SimpleHttpServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Core.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Core* core = Core::getInstance();
	core->start();
	return 0;
}