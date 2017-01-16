// SimpleHttpServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#define TEST

#ifdef TEST
#include "Test.h"
#else
#include "Core.h"
#endif


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef TEST
	return Test::TestAll();
#else
	Core* core = Core::getInstance();
	core->start();
	return 0;
#endif
}