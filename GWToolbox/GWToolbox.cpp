#include "GWToolbox.h"
#include "../include/OSHGui/OSHGui.hpp"
#include "../include/OSHGui/Drawing/Direct3D9/Direct3D9Renderer.hpp"
#include "../include/OSHGui/Drawing/Theme.hpp"
#include "../include/OSHGui/Input/WindowsMessage.hpp"

#include <string>

using namespace OSHGui::Drawing;
using namespace OSHGui::Input;

GWToolbox* GWToolbox::instance = NULL;

namespace{
	LPD3DXFONT dbgFont = NULL;
	RECT Type1Rect = { 50, 50, 300, 14 };
	GWAPI::GWAPIMgr * mgr;
	GWAPI::DirectXMgr * dx;

	HHOOK oshinputhook;
	WindowsMessage input;
}

LRESULT CALLBACK MessageHook(int code, WPARAM wParam, LPARAM lParam) {

	// do nothing if application is not initialized or we got a message that we shouldn't handle
	if (!Application::InstancePtr()->HasBeenInitialized()
		|| (lParam & 0x80000000)
		|| (lParam & 0x40000000)
		|| (code != HC_ACTION)) {
		return CallNextHookEx(NULL, code, wParam, lParam);
	}

	LPMSG msg = (LPMSG)lParam;

	switch (msg->message) {
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (input.ProcessMessage(msg)) {
			//LOG("consumed mouse event %d\n",msg->message);
			return TRUE;
		}
		break;

	// send keyboard messages to gw, osh and toolbox (does osh need it?)
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_IME_CHAR:
		//LOG("processing keyboard event %d, key %u\n", msg->message, msg->wParam);
		input.ProcessMessage(msg);
		GWToolbox::getInstance()->hotkeyMgr->processMessage(msg);
		break;
	}

	return CallNextHookEx(NULL, code, wParam, lParam);
}


void create_gui(IDirect3DDevice9* pDevice) {
	Application::Initialize(std::unique_ptr<Direct3D9Renderer>(new Direct3D9Renderer(pDevice)));

	Application * app = Application::InstancePtr();

	string path = GWToolbox::getInstance()->config->getPathA("DefaultTheme.txt");
	try {
		Theme theme;
		theme.Load(path);
		app->SetTheme(theme);
		LOG("Loaded theme file %s\n", path.c_str());
	} catch (Misc::InvalidThemeException e) {
		ERR("WARNING Could not load theme file %s\n", path.c_str());
	}
	
	auto font = FontManager::LoadFont("Arial", 8.0f, false); //Arial, 8PT, no anti-aliasing
	app->SetDefaultFont(font);
	app->SetCursorEnabled(false);

	auto form = std::make_shared<Form>();
	form->SetText("GWToolbox++");
	form->SetSize(100, 300);
	
	Label * pcons = new Label();
	pcons->SetText("Pcons");
	pcons->SetBounds(0, 0, 100, 30);
	pcons->GetClickEvent() += ClickEventHandler([pcons](Control*) {
		LOG("Clicked on pcons!\n");
	});
	
	form->AddControl(pcons);

	app->Run(form);
	app->Enable();

	//app->RegisterHotkey(Hotkey(Key::Insert, [] {
	//	Application::InstancePtr()->Toggle();
	//}));

	GWToolbox * tb = GWToolbox::getInstance();
	tb->pcons->buildUI();
	tb->builds->buildUI();
	tb->hotkeys->buildUI();

	oshinputhook = SetWindowsHookEx(WH_GETMESSAGE, MessageHook, NULL, GetCurrentThreadId());
}

// All rendering done here.
static HRESULT WINAPI endScene(IDirect3DDevice9* pDevice) {
	static bool init = false;
	if (!init) {
		init = true;
		create_gui(pDevice);
	}

	auto &renderer = Application::Instance().GetRenderer();

	renderer.BeginRendering();

	Application::Instance().Render();

	renderer.EndRendering();

	if (!dbgFont)
		D3DXCreateFontA(pDevice, 14, 0, FW_BOLD, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &dbgFont);


	//dbgFont->DrawTextA(NULL, "AHJHJHJHJHJHJHJHJHJ", strlen("AHJHJHJHJHJHJHJHJHJ") + 1, &Type1Rect, DT_NOCLIP, 0xFF00FF00);

	return  dx->GetEndsceneReturn()(pDevice);
}

static HRESULT WINAPI resetScene(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {

	// pre-reset here.

	if (dbgFont) dbgFont->OnLostDevice();
	HRESULT result = dx->GetResetReturn()(pDevice, pPresentationParameters);
	if (result == D3D_OK){
		if (dbgFont) dbgFont->OnResetDevice();// post-reset here.
	}

	return result;
}


void GWToolbox::exec() {
	mgr = GWAPI::GWAPIMgr::GetInstance();
	dx = mgr->DirectX;
	

	pcons->loadIni();
	builds->loadIni();
	hotkeys->loadIni();

	dx->CreateRenderHooks(endScene, resetScene);
	
	input.SetKeyboardInputEnabled(true);
	input.SetMouseInputEnabled(true);

	m_Active = true;

	Application * app = Application::InstancePtr();
	while (true) { // main loop
		if (app->HasBeenInitialized()) {
			pcons->mainRoutine();
			builds->mainRoutine();
			hotkeys->mainRoutine();
		}
		Sleep(10);

		if (GetAsyncKeyState(VK_END) & 1)
			destroy();
	}
}

void GWToolbox::destroy()
{
	
	delete pcons;
	delete builds;
	delete hotkeys;

	config->save();
	delete config;
	delete hotkeyMgr;

	UnhookWindowsHookEx(oshinputhook);
	GWAPI::GWAPIMgr::Destruct();
	m_Active = false;
	ExitThread(EXIT_SUCCESS);
}

// what is this for?
bool GWToolbox::isActive() {
	return m_Active;
}


void GWToolbox::threadEntry(HMODULE mod) {
	if (instance) return;

	GWAPI::GWAPIMgr::Initialize();

	instance = new GWToolbox(mod);
	instance->exec();
}