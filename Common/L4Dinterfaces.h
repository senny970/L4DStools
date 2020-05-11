#pragma once

#include "../Utils/FindPattern.h"
#include "../Common/SDKheaders.h"


//IClientMode *GetClientMode();
#define GetClientMode_SIG "\x8B\x0D\x00\x00\x00\x00\x8B\x01\x8B\x90\x00\x00\x00\x00\xFF\xD2\x8B\x04\x85\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xA1\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xB8\x00\x00\x00\x00"
#define GetClientMode_MASK "xx????xxxx????xxxxx????xxxxxxxxxx????xxxxxxxxxxxx????"

class L4DInterface
{
public:
	L4DInterface();
	~L4DInterface();
	CreateInterfaceFn clientFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("client.dll"), "CreateInterface");
	CreateInterfaceFn serverFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("server.dll"), "CreateInterface");
	CreateInterfaceFn materialFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("materialsystem.dll"), "CreateInterface");
	CreateInterfaceFn datacacheFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("datacache.dll"), "CreateInterface");
	CreateInterfaceFn stdRenderFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("studiorender.dll"), "CreateInterface");
	CreateInterfaceFn vguimatsurfaceFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("vguimatsurface.dll"), "CreateInterface");
	CreateInterfaceFn steamclientFactory = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("steamclient.dll"), "CreateInterface");

	IClientMode* GetClientModeInterface();
	IClientMode* IclientMode = NULL;
	IClientEntityList* entList = NULL;	
	void PrintInterfaces();
private:
};

L4DInterface::L4DInterface()
{

	//IclientMode = GetClientModeInterface();
	entList = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
}

L4DInterface::~L4DInterface()
{
}

IClientMode* L4DInterface::GetClientModeInterface() {
		IClientMode* pClmode = NULL;
		typedef IClientMode* (*FuncType)();
		FuncType ClMode;

		ClMode = (FuncType)FindPattern("client.dll", GetClientMode_SIG, GetClientMode_MASK);
		if (ClMode)
			pClmode = ClMode();		
		else
			pClmode = NULL;
		return pClmode;
}

void L4DInterface::PrintInterfaces() {
	Msg("\nInterfaces:\n");
	Msg("IClientMode: %x\n", IclientMode);
	Msg("IClientEntityList: %x\n", entList);

	Msg("\n");
}

L4DInterface* pL4Dinterface = NULL;