#include "../../pch.h"
#include "UserInterface.h"

#include "../Cheat/Settings.h"

#include "../../vendors/imgui/imgui.h"
#include "../../vendors/imgui/imgui_custom.hpp"
#include "../../vendors/imgui/imgui_impl_win32.h"
#include "../../vendors/imgui/imgui_impl_dx11.h"
#include "../../vendors/imgui/fonts.hpp"
#include "../../vendors/imgui/img.hpp"

#include <d3d11.h>
#include <D3DX11tex.h>
#include <dwmapi.h>


static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
static bool						g_wndMinimized = false;

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern IMGUI_IMPL_API long long ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}

		g_wndMinimized = (wParam == SIZE_MINIMIZED);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
 
void UserInterface::Display()
{
	auto globals = Instance<Globals>::Get();

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC , WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "#w", NULL };
	::RegisterClassEx(&wc);

	globals->m_Hwnd = CreateWindowExA(0L, wc.lpszClassName, "", WS_POPUP, ((GetSystemMetrics(SM_CXSCREEN) - m_Width) / 2.f), ((GetSystemMetrics(SM_CYSCREEN) - m_Height) / 2.f), m_Width, m_Height, NULL, NULL, wc.hInstance, NULL);

	MARGINS margins = { -1 };
	DwmExtendFrameIntoClientArea(globals->m_Hwnd, &margins); //make transparent

	if (!CreateDeviceD3D(globals->m_Hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return;
	}

	::ShowWindow(globals->m_Hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(globals->m_Hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	ImGuiContext& g = *GImGui;
	auto style = &ImGui::GetStyle();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(globals->m_Hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 0.f);

	ID3D11ShaderResourceView* Image = nullptr;
	D3DX11_IMAGE_LOAD_INFO info;
	ID3DX11ThreadPump* pump{ nullptr };
	D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, imgs::rawData, sizeof(imgs::rawData), &info,
		pump, &Image, 0);

	ID3D11ShaderResourceView* lilImage = nullptr;
	D3DX11_IMAGE_LOAD_INFO info2;
	ID3DX11ThreadPump* pump2{ nullptr };
	D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, imgs::logoRaw, sizeof(imgs::logoRaw), &info2,
		pump2, &lilImage, 0);


	/**/
	ImFont* oliviasans = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::oliviasans_compressed_data, fonts::oliviasans_compressed_size, 20.f);
	static const ImWchar icons_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
	ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryCompressedTTF(fonts::materialicons_compressed_data, fonts::materialicons_compressed_size, 20.f, &icons_config, icons_ranges);

	ImFont* iconFont = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::materialicons_compressed_data, fonts::materialicons_compressed_size, 30.f, NULL, icons_ranges);

	ImFont* reforma = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::reforma_compressed_data, fonts::reforma_compressed_size, 15.f);
	ImFont* opensan = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::opensans_med_compressed_data, fonts::opensans_med_compressed_size, 22.5f);
	ImFont* medopensan = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::opensans_med_compressed_data, fonts::opensans_med_compressed_size, 20.f);
	ImFont* lilopensan = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::opensans_med_compressed_data, fonts::opensans_med_compressed_size, 15.f);
	ImFont* bigopensan = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::opensans_med_compressed_data, fonts::opensans_med_compressed_size, 32.5f);

	ImFont* iconFont2 = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::icons_compressed_data, fonts::icons_compressed_size, 25.f);
	ImFont* hugeIcons = io.Fonts->AddFontFromMemoryCompressedTTF(fonts::icons_compressed_data, fonts::icons_compressed_size, 50.f);

	int menuMovementX = 0;
	int menuMovementY = 0;
	int currentTab = 0;
	int currentModule = 0;
	bool bTitleHovered = false;
	bool bDraging = false;
	bool bMinimized = false;

	while (!Instance<Settings>::Get()->m_Destruct)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				Instance<Settings>::Get()->m_Destruct = true;
			}
		}

		if (g_wndMinimized)
		{
			Sleep(1);
			continue;
		}
		else {
			bMinimized = false;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static uint32_t cnt = 0;
		static float freq = .01f;

		if (globals->Gui.m_Rainbow) {
			globals->Gui.m_AccentColor[0] = (std::sin(freq * cnt + 0) * 127 + 128) / 255.f;
			globals->Gui.m_AccentColor[1] = (std::sin(freq * cnt + 2) * 127 + 128) / 255.f;
			globals->Gui.m_AccentColor[2] = (std::sin(freq * cnt + 4) * 127 + 128) / 255.f;

			if (cnt++ >= (uint32_t)-1)
				cnt = 0;
		}
		/**/

		if (Instance<Settings>::Get()->m_Destruct)
			break;

		/**/

		ImGui::SetNextWindowPos({ 0.F, 0.f });
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

			ImGui::SetNextWindowSize({ m_Width, m_Height });
			ImGui::Begin("##mainWindow", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{

			}
			ImGui::End();
			
			/**/
			if (!bDraging )
				bDraging = bTitleHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left);

			if (bDraging && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
				bDraging = false;

			if (ImGui::IsMouseClicked(0))
			{
				POINT CursorPosition;
				RECT MenuPosition;

				GetCursorPos(&CursorPosition);
				GetWindowRect(globals->m_Hwnd, &MenuPosition);

				menuMovementX = CursorPosition.x - MenuPosition.left;
				menuMovementY = CursorPosition.y - MenuPosition.top;
			}

			if (bDraging)
			{
				POINT cursor_position;

				GetCursorPos(&cursor_position);
				SetWindowPos(globals->m_Hwnd, nullptr, cursor_position.x - menuMovementX, cursor_position.y - menuMovementY, 0, 0, SWP_NOSIZE);
			}

			ImGui::PopStyleVar(2);
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync

		if (bMinimized) {
			ShowWindow(globals->m_Hwnd, SW_MINIMIZE);
		}
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(globals->m_Hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}