#include<crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#include"Application/Application.hpp"
#include "dxgidebug.h"

/*
	Made by:
	Pontus Hellman
	Simon Törnblom
	Yousra Jlali
*/


int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Application::Get()->Run();
	Application::Get()->Shutdown();

	IDXGIDebug* dxgiControler;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiControler)))) {
		dxgiControler->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}

	return 0;
}