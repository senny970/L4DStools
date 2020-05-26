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
bool bEntityIspector = false;
bool bDemoWindow = false;
#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)
#define ES	0
typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
LRESULT CALLBACK MsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }
tEndScene oEndScene;

WNDPROC oWndProc;

const char* szConvertType(int iType)
{
	switch (iType)
	{
	case DPT_Int:
		return "int";
	case DPT_Float:
		return "float";
	case DPT_Vector:
		return "Vector";
	case DPT_VectorXY:
		return "Vector2D";
	case DPT_String:
		return "char*";
	case DPT_Array:
		return "Array";
	case DPT_DataTable:
		return "DataTable";
	case DPT_Int64:
		return "__int64";
	}

	return "Unknown";
}

int data_int = 0;
float data_float = 0.0f;
Vector data_vector = Vector(0, 0, 0);

void DumpNetVar(RecvTable* pTable, int iLevel = 1, IClientEntity *entity = NULL)
{
	for (int i = 0; i < pTable->m_nProps; ++i)
	{
		RecvProp Prop = pTable->m_pProps[i];
		int iLevel4 = iLevel * 4;

		if (Prop.m_pDataTable)
		{
			if (Prop.m_pDataTable->m_nProps <= 0)
				continue;

			ImGui::Text("%*sclass %s // %s 0x%X\n%*s{\n", iLevel4, "\t", Prop.m_pVarName, Prop.m_pDataTable->m_pNetTableName, Prop.m_Offset, iLevel4, "\t");

			DumpNetVar(Prop.m_pDataTable, iLevel + 1, entity);

			ImGui::Text("%*s};\n\n", iLevel4, "\t");
		}
		else
			if (strcmp(szConvertType(Prop.m_RecvType), "int") == 0) {
				data_int = *(int*)((char*)entity + Prop.m_Offset);
				ImGui::Text("%*s%s %s; // 0x%X : %d\n", iLevel4, "\t", szConvertType(Prop.m_RecvType), Prop.m_pVarName, Prop.m_Offset, data_int);				
				continue;
			}

			if (strcmp(szConvertType(Prop.m_RecvType), "float") == 0) {
				data_float = *(float*)((char*)entity + Prop.m_Offset);
				ImGui::Text("%*s%s %s; // 0x%X : %.2f\n", iLevel4, "\t", szConvertType(Prop.m_RecvType), Prop.m_pVarName, Prop.m_Offset, data_float);
				continue;
			}

			if (strcmp(szConvertType(Prop.m_RecvType), "Vector") == 0) {
				data_vector = *(Vector*)((char*)entity + Prop.m_Offset);
				ImGui::Text("%*s%s %s; // 0x%X : (%f, %f, %f)\n", iLevel4, "\t", szConvertType(Prop.m_RecvType), Prop.m_pVarName, Prop.m_Offset, data_vector.x, data_vector.y, data_vector.z);
				continue;
			}

			ImGui::Text( "%*s%s %s; // 0x%X\n", iLevel4, "\t", szConvertType(Prop.m_RecvType), Prop.m_pVarName, Prop.m_Offset);
	}
}

/*void NetVars::Dump()
{
	bool bFirst = true;
	for (ClientClass* pClass = g_pClient->GetAllClasses(); pClass; pClass = pClass->m_pNext)
	{
		fprintf_s(pFile, "%sclass %s // %s\n{\n", (bFirst ? "" : "\n"), pClass->m_pNetworkName, pClass->m_pRecvTable->m_pNetTableName);

		DumpNetVar(pFile, pClass->m_pRecvTable);

		fprintf_s(pFile, "};\n");

		bFirst = false;
	}
}*/

//old
/*void DumpTable(RecvTable* table, int depth) {
	char* pre;
	char* string;

	ImGui::Text(table->m_pNetTableName);

		for (int i = 0; i < table->m_nProps; i++) {
			RecvProp* prop = &table->m_pProps[i];
			if (!prop)
				continue;

			if (strstr(prop->m_pVarName, "baseclass") || strstr(prop->m_pVarName, "0") || strstr(prop->m_pVarName, "1") || strstr(prop->m_pVarName, "2"))
				continue;

			//ImGui::Text(prop->m_pVarName);

			ImGui::Text("\t%s [0x%x]", prop->m_pVarName, prop->m_Offset);

			if (prop->m_pDataTable)
				DumpTable(prop->m_pDataTable, depth + 1);
		}
}*/

void EntityInspector(bool *show) {
		ImVec2 start_window_pos(5, 5);
		ImVec2 windows_size(200, 400);
		char ent_caption_count[32];
		int ent_count = 0;
		static int selected = -1;
		static bool b_isDebug = false;
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar;
		ImGuiWindowFlags window_flags2 = ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;
		const ImGuiID child_entitys = ImGui::GetID((void*)(intptr_t)0);
		const ImGuiID child_properties = ImGui::GetID((void*)(intptr_t)1);
		static RecvTable* pTable = NULL;
		static char filter[128] = "";

		ImGui::Begin("Entity inspector",show);
		//ImGui::SetWindowSize(windows_size);
		//ImGui::SetWindowPos(start_window_pos);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::Checkbox("Debug line", &b_isDebug);
		ImGui::BeginChild(child_entitys, ImVec2(200, 350), true, window_flags);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Entitys"))
			{
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		for (int i = 0; i < g_pL4Dinterface->entList->GetMaxEntities(); i++) {
			IClientEntity* entity = g_pL4Dinterface->entList->GetClientEntity(i);
			if (entity) {
				ent_count++;
				sprintf(ent_caption_count, "Entitys: %d", ent_count);
				char* network_name = entity->GetClientClass()->m_pNetworkName;

				if (strlen(filter) > 0) {
					if (strstr(network_name, filter) == 0) {
						continue;
					}
				}

				char buf[32];
				sprintf(buf, "%d %s", i, network_name);				
				if (ImGui::Selectable(buf, selected == i)) {
					selected = i;
					pTable = entity->GetClientClass()->m_pRecvTable;
				}
			}
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild(child_properties, ImVec2(400, 350), true, window_flags2);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Properties"))
			{
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		//properties
		if (selected >= 0) {
			IClientEntity* ent = g_pL4Dinterface->entList->GetClientEntity(selected);
			if (ent) {
				//ImGui::Text("%d %s", selected, ent->GetClientClass()->m_pNetworkName);
				//DumpTable(pTable, 0);
				DumpNetVar(pTable, 0, ent);
			}
		}

		ImGui::EndChild();
		ImGui::Text("Entity filter:");
		ImGui::InputTextWithHint("", "Entity name", filter, IM_ARRAYSIZE(filter));
		ImGui::Text("Entitys: %d", ent_count);
		ImGui::Text("Selected: %d", selected);
		ImGui::PopStyleVar();
		ImGui::End();		
}

static void MainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Entity inspector"))
		{			
			bEntityIspector = !bEntityIspector;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Demo"))
		{	
			bDemoWindow = !bDemoWindow;
			/*if (ImGui::MenuItem("Demo")) {
				bDemoWindow = !bDemoWindow;
			}*/
			//ImGui::Separator();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

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

	//iPanel->SetMouseInputEnabled(1, false);
	if (active) {
		//ImGui::ShowDemoWindow(&bDemoWindow);		
		//EntityInspector(&bEntityIspector);
		//EntityInspector(&active);
		MainMenuBar();
		if(bDemoWindow)
			ImGui::ShowDemoWindow(&bDemoWindow);
		if (bEntityIspector)
			EntityInspector(&bEntityIspector);
	}

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