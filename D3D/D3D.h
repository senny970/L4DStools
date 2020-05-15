
#pragma comment ( lib , "d3d9.lib" )
#pragma comment(lib, "detours.lib")

#include <Windows.h>
#include <iostream>
#include <d3d9.h>
#include <dinput.h>
#include <tchar.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "..\Detours\detours.h"
//#include "Utils/vmthook.h"
#define DIRECTINPUT_VERSION 0x0800

//0B99AE65 | 68 80 FF A5 0B | push shaderapidx9.BA5FF80       | BA5FF80:" ERROR: failed to create vertex decl for vertex format %x! You'll probably see messed-up mesh rendering - to diagnose, build shaderapidx9.dll in debug.\n"
//0B99AE3B | E8 80 11 FE FF | call <shaderapidx9.sub_B97BFC0> | Dx9Device
//#define SIG "\xB8\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x56\x8B\xF1\xE8"
//#define MASK "x????xxxxxxxxxxxxxxx"


/*IDirect3DDevice9* GetD3DDevice() {
	IDirect3DDevice9* d3dDevice = NULL;
	typedef IDirect3DDevice9* (*FuncType)(void);
	FuncType Dx9Device;
	Dx9Device = (FuncType)FindPattern("shaderapidx9.dll", SIG, MASK);

	if (Dx9Device) {
		ConColorMsg(Color(0, 255, 0, 255), "---------------D3D Init---------------\n");
		ConColorMsg(Color(0, 255, 0, 255), "Dx9Device() found at: %x\n", Dx9Device);
		ConColorMsg(Color(0, 255, 0, 255), "IDirect3DDevice9* found at: %x\n", Dx9Device());
		d3dDevice = *(IDirect3DDevice9**)Dx9Device();
		return d3dDevice;
	}
	else {
		ConColorMsg(Color(255, 0, 0, 255), "D3D Init Filed!\n");
		return NULL;
	}
}*/


bool active = false;
#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)
#define ES	0
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
LRESULT CALLBACK MsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }
tEndScene oEndScene;

WNDPROC oWndProc;

HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	/*D3DRECT BarRect = { 100, 100, 200, 200 };
	pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(1, 1, 1, 1), 0.0f, 0); // was black because its upp to 255 and not 0 to 1 :')*/
	static bool init = true;
	if (init)
	{
		init = false;
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplWin32_Init(FindWindowA("Valve001", NULL));
		ImGui_ImplDX9_Init(pDevice);
		ImGui::StyleColorsDark();
	}
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	//

	/*imgui kullanma alaný*/
	if (active)
		ImGui::ShowDemoWindow(&active);

	//mebu son
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

void DX_Init(DWORD* table)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX),CS_CLASSDC,MsgProc,0L,0L,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,"DX",NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	LPDIRECT3DDEVICE9 pd3dDevice;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice);
	DWORD* pVTable = (DWORD*)pd3dDevice;
	pVTable = (DWORD*)pVTable[0];
	table[ES] = pVTable[42];
	DestroyWindow(hWnd);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_KEYUP && wParam == VK_DELETE)
		active = !active;

	if (active && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void ImGuiSetup() {
	HWND  window = FindWindowA("Valve001", NULL);
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
	DWORD VTable[3] = { 0 };
	DX_Init(VTable);
	HOOK(EndScene, VTable[ES]);
}