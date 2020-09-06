// dear imgui: standalone example application for DirectX 9
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
#define START_WITH_VISU 0
#define REGISTER_HOOKS 1
#define REGISTER_MOUSE_HOOK 1
#define REGISTER_EVENT_HOOK 1
#define REGISTER_KEYBOARD_HOOK 1
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // [Win32] Don't implement default clipboard handler. Won't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc.
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // [Win32] Don't implement default IME handler. Won't use and link with ImmGetContext/ImmSetCompositionWindow.
//#define IMGUI_DISABLE_WIN32_FUNCTIONS                     // [Win32] Won't use and link with any Win32 function.
//#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS             // Don't implement ImFormatString/ImFormatStringV so you can implement them yourself if you don't want to link with vsnprintf.
//#define IMGUI_DISABLE_MATH_FUNCTIONS                      // Don't implement ImFabs/ImSqrt/ImPow/ImFmod/ImCos/ImSin/ImAcos/ImAtan2 wrapper so you can implement them yourself. Declare your prototypes in imconfig.h.
//#define IMGUI_DISABLE_DEFAULT_ALLOCATORS                  // Don't implement default allocators calling malloc()/free() to avoid linking with them. You will need to call ImGui::SetAllocatorFunctions().
#include "imgui.h"
#include <thread>
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "GL/glew.h"
#include "GL/wglew.h"

#include <tchar.h>

#include <mutex>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <set>
#include <cassert>
#include <unordered_set>

#include "resource.h"

#include "render_stats.hpp"
#include "keyboard.hpp"
#include "Mouse.hpp"
#include "Settings.hpp"
#include "NotifyIcon.hpp"
#include "Common.hpp"
#include "TimeInfo.hpp"
#include "file.hpp"
#include "Logs.hpp"
#include "Screen.hpp"
#include "Event.hpp"

#include "psapi.h"

constexpr auto Mail_Name = "\\\\.\\Mailslot\\Mes Touches";
constexpr auto IDM_EXIT = 100;
constexpr auto WM_NOTIFY_MSG = WM_APP + 1;
constexpr auto Quit_Request = WM_APP + 2;
UINT Mail_Arrived_Msg = WM_NULL;
// Data
static LPDIRECT3DDEVICE9		g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS	g_d3dpp;

void window_process() noexcept;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void event_queue_process() noexcept;

LRESULT CALLBACK display_hook();
LRESULT CALLBACK keyboard_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept;
LRESULT CALLBACK mouse_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept;
LRESULT CALLBACK event_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept;

std::optional<std::string> get_last_error_message() noexcept;
std::optional<HGLRC> create_gl_context(HWND handle_window) noexcept;
void destroy_gl_context(HGLRC gl_context) noexcept;

struct Mail_Message {
	int n_code;
	WPARAM w_param;
	LPARAM l_param;
};
void make_mail() noexcept;
std::optional<Mail_Message> read_mail() noexcept;

// It's shared data between the hook process and the windows process.
struct SharedData {
	std::mutex mut_keyboard_state;
	std::optional<KeyboardState> keyboard_state;

	std::mutex mut_mouse_state;
	std::optional<MouseState> mouse_state;

	std::mutex mut_event_state;
	std::optional<EventState> event_state;

	std::atomic<HWND> hook_window = nullptr;
	std::atomic<HWND> visu_window = nullptr;
	Settings settings;

	HANDLE mail_slot = INVALID_HANDLE_VALUE;
} shared;

constexpr auto hook_class_name = "Hook MT";
constexpr auto visu_class_name = "Visu MT";
constexpr auto window_title = "Mes Touches";

Logs logs;

struct EventQueueCache {
	std::mutex mutex;
	std::condition_variable wait_var;
	bool event_received{ false };

	std::vector<KeyEntry> keyboard;
	std::vector<ClickEntry> click;
	std::vector<Display> display;
	std::vector<AppUsage> app_usages;
} event_queue_cache;

void toggle_fullscren(HWND hwnd) {
	static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	if (dwStyle & WS_OVERLAPPEDWINDOW) {
		MONITORINFO mi = { sizeof(mi) };
		if (
			GetWindowPlacement(hwnd, &g_wpPrev) &&
			GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)
			) {
			SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(
				hwnd, HWND_TOP,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED
			);
		}
	}
	else {
		SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(hwnd, &g_wpPrev);
		SetWindowPos(
			hwnd,
			NULL,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);
	}
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (msg == Mail_Arrived_Msg) if (auto res = read_mail(); res) {
		event_hook(res->n_code, res->w_param, res->l_param);
	}

	switch (msg)
	{
	case WM_NOTIFY_MSG: {
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			if (!shared.visu_window) {
				std::thread{ window_process }.detach();
			}
			else {
				SetForegroundWindow(shared.visu_window);
			}
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			POINT pt;
			GetCursorPos(&pt);
			auto menu = CreatePopupMenu();
			InsertMenu(
				menu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT, TEXT("Quit")
			);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(
				menu,
				GetSystemMetrics(SM_MENUDROPALIGNMENT) | TPM_LEFTBUTTON,
				pt.x,
				pt.y,
				0,
				hWnd,
				NULL
			);
			DestroyMenu(menu);
			break;
		}
	} break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
	case Quit_Request:
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDM_EXIT:
				PostQuitMessage(0);
				return 0;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool visu_windows_ended = false;
LRESULT WINAPI WndProc_visu(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_KEYDOWN: {
		if (wParam == VK_F11) toggle_fullscren(hWnd);
		break;
	}
	case WM_SIZE:
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case Quit_Request:
		if (shared.hook_window) {
			PostMessage(shared.hook_window, Quit_Request, NULL, NULL);
		}
	case WM_DESTROY:
		visu_windows_ended = true;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifdef DEBUG_CONSOLE
int main() {
#else
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	auto time_start = get_milliseconds_epoch();

	std::filesystem::create_directories(get_app_data_path());
	
	shared.settings.copy_system();
	shared.keyboard_state =
		KeyboardState::load_from_file(get_app_data_path() / Default_Keyboard_Path);
	shared.mouse_state =
		MouseState::load_from_file(get_app_data_path() / MouseState::Default_Path, true);
	shared.event_state =
		EventState::load_from_file(get_app_data_path() / EventState::Default_Path);

	defer{
		if (shared.keyboard_state) {
			(void)shared.keyboard_state->save_to_file(get_app_data_path() / Default_Keyboard_Path);
		}

		if (shared.mouse_state){
			(void)shared.mouse_state->save_to_file(get_app_data_path() / MouseState::Default_Path);
		}

		if (shared.event_state){
			(void)shared.event_state->save_to_file(get_app_data_path() / EventState::Default_Path);
		}
	};

	// Create application window
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0L,
		0L,
		GetModuleHandle(nullptr),
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		hook_class_name,
		nullptr
	};

	wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = wc.hIcon;

	RegisterClassEx(&wc);
	defer{ UnregisterClass(hook_class_name, wc.hInstance); };
	HWND hwnd = CreateWindow(
		hook_class_name,
		window_title,
		WS_OVERLAPPED,
		400,
		300,
		0,
		0,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);
	shared.hook_window = hwnd;

	// so now we are after the creation of the koow window
	// but before its registration as a hook so it's the perfect time
	// to start the event_queue process.
	std::thread{ event_queue_process }.detach();

	defer{ DestroyWindow(hwnd); };
	defer{ shared.hook_window = nullptr; };

#if REGISTER_HOOKS
#if REGISTER_KEYBOARD_HOOK
	SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, NULL);
#endif
#if REGISTER_MOUSE_HOOK
	SetWindowsHookEx(WH_MOUSE_LL, mouse_hook, NULL, NULL);
#endif
#if REGISTER_EVENT_HOOK
	__declspec(dllimport) extern bool install_hook();
	__declspec(dllimport) extern bool uninstall_hook();

	Mail_Arrived_Msg = RegisterWindowMessage(Mail_Name);
	make_mail();
	install_hook();
	defer { uninstall_hook(); };
#endif
#endif

	NotifyIcon sys_tray_icon;
	sys_tray_icon.set_icon(wc.hIcon);
	sys_tray_icon.set_tooltip("You can open me");
	sys_tray_icon.set_window(hwnd);
	sys_tray_icon.set_message(WM_NOTIFY_MSG);
	sys_tray_icon.add();
	sys_tray_icon.show();
	defer{ sys_tray_icon.remove(); };

	LogEntry entry;
	entry.message = "Started in: " + std::to_string(get_milliseconds_epoch() - time_start) + "ms.";
	entry.tag.push_back("PERF");
	logs.lock_and_write(entry);

#if START_WITH_VISU
	std::thread{ window_process }.detach();
#endif

	MSG msg{};
	while (GetMessage(&msg, NULL, 0U, 0U)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void window_process() noexcept {
	LogWindow log_window;
	MouseWindow mou_window;
	SettingsWindow set_window;
	KeyboardWindow key_window;
	EventWindow eve_window;
	visu_windows_ended = false;
	// Create application window
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc_visu,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL,
		NULL,
		NULL,
		NULL,
		visu_class_name,
		NULL
	};
	wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = wc.hIcon;

	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(
		visu_class_name,
		window_title,
		WS_OVERLAPPEDWINDOW,
		100,
		75,
		1280,
		720,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);
	shared.visu_window = hwnd;

	defer{
		UnregisterClass(visu_class_name, wc.hInstance);
		DestroyWindow(hwnd);
		shared.visu_window = nullptr;
	};


	auto gl_context = *create_gl_context(hwnd);
	defer{ destroy_gl_context(gl_context); };

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplOpenGL3_Init();

	auto dc_window = GetDC(hwnd);
	if (!dc_window) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return;
	}
	
	defer{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	};

	// Main loop
	MSG msg = {};
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	auto delta_clock = get_microseconds_epoch();
	while (msg.message != WM_QUIT) {
		auto dt = (get_microseconds_epoch() - delta_clock) / 1'000'000.0;
		delta_clock = get_microseconds_epoch();

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (visu_windows_ended) break;
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		glClearColor(0.5f, 0.5f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);
		
		ImGui::DockSpace(ImGui::GetID("dock"));

		ImGui::Begin("Debug");
		ImGui::Text("%f", 1.f / (float)dt);
		ImGui::End();


		{
			std::lock_guard guard{ shared.mut_keyboard_state };
			key_window.render(shared.keyboard_state);
		} {
			std::lock_guard guard{ shared.mut_mouse_state };
			mou_window.render(shared.mouse_state);
		} {
			auto t = std::lock_guard{ shared.mut_event_state };
			eve_window.render(shared.event_state);
		}
		set_window.render(shared.settings);
		log_window.render(logs);

		if (ImGui::BeginPopup("Error Prompt")) {
			defer{ ImGui::EndPopup(); };

			ImGui::Text("The last action ended with an error see the logs for more details.");
		}

		// update
		if (set_window.show_log) {
			log_window.open = true;
			set_window.show_log = false;
		}
		if (set_window.reset_keyboard_state && shared.keyboard_state) {
			std::lock_guard guard{ shared.mut_keyboard_state };
			if (!shared.keyboard_state->reset_everything()) {
				ImGui::OpenPopup("Error Prompt");
			}
			set_window.reset_keyboard_state = false;
		}
		if (set_window.reset_mouse_state && shared.mouse_state) {
			std::lock_guard guard{ shared.mut_mouse_state };
			if (!shared.mouse_state->reset_everything()) {
				ImGui::OpenPopup("Error Prompt");
			}
			set_window.reset_mouse_state = false;
		}
		if (set_window.quit) {
			PostMessage(shared.visu_window, Quit_Request, 0, 0);
			// set_window.quit = false. We don't reset it because a quit is authoritative.
			// we will spam messages until we quit.
		}
		if (set_window.install) {

		}

		if (key_window.reset) {
			auto t = std::lock_guard{ shared.mut_keyboard_state };
			if (!shared.keyboard_state) shared.keyboard_state = KeyboardState{};
			if (!shared.keyboard_state->reset_everything()) {
				ImGui::OpenPopup("Error Prompt");
			}
			key_window.reset = false;
		}
		if (key_window.save) {
			auto full_path = get_app_data_path() / Default_Keyboard_Path;
			auto t = std::lock_guard{ shared.mut_keyboard_state };
			if (!shared.keyboard_state->save_to_file(full_path)) {
				ImGui::OpenPopup("Error Prompt");
			}
			key_window.save = false;
		}
		if (key_window.reload) {
			auto full_path = get_app_data_path() / Default_Keyboard_Path;
			auto t = std::lock_guard{ shared.mut_keyboard_state };
			auto opt = KeyboardState::load_from_file(full_path);
			if (opt) shared.keyboard_state = *opt;
			else ImGui::OpenPopup("Error Prompt");
			key_window.reload = false;
		}

		if (mou_window.reset) {
			auto t = std::lock_guard{ shared.mut_mouse_state };
			if (!shared.mouse_state) shared.mouse_state = MouseState{};
			if (!shared.mouse_state->reset_everything()) {
				ImGui::OpenPopup("Error Prompt");
			}
			mou_window.reset = false;
		}
		if (mou_window.save) {
			auto full_path = get_app_data_path() / MouseState::Default_Path;
			auto t = std::lock_guard{ shared.mut_mouse_state };
			if (!shared.mouse_state->save_to_file(full_path)) {
				ImGui::OpenPopup("Error Prompt");
			}
			mou_window.save = false;
		}
		if (mou_window.reload) {
			auto full_path = get_app_data_path() / MouseState::Default_Path;
			auto t = std::lock_guard{ shared.mut_mouse_state };
			auto opt = MouseState::load_from_file(full_path, mou_window.strict);
			if (opt) shared.mouse_state = *opt;
			else ImGui::OpenPopup("Error Prompt");
			mou_window.reload = false;
		}


		if (eve_window.reset) {
			auto t = std::lock_guard{ shared.mut_event_state };
			if (!shared.event_state) shared.event_state = EventState{};
			if (!shared.event_state->reset_everything()) {
				ImGui::OpenPopup("Error Prompt");
			}
			eve_window.reset = false;
		}
		if (eve_window.save) {
			auto full_path = get_app_data_path() / EventState::Default_Path;
			auto t = std::lock_guard{ shared.mut_event_state };
			if (!shared.event_state->save_to_file(full_path)) {
				ImGui::OpenPopup("Error Prompt");
			}
			eve_window.save = false;
		}
		if (eve_window.reload) {
			auto full_path = get_app_data_path() / EventState::Default_Path;
			auto t = std::lock_guard{ shared.mut_event_state };
			auto opt = EventState::load_from_file(full_path);
			if (opt) shared.event_state = *opt;
			else ImGui::OpenPopup("Error Prompt");
			eve_window.reload = false;
		}

		ImGui::End();
		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SwapBuffers(dc_window);
		//using namespace std::chrono_literals;
		//std::this_thread::sleep_for(16ms);
	}
}

LRESULT CALLBACK keyboard_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	thread_local std::vector<KeyEntry> key_entries_to_add;

	auto time_start = get_microseconds_epoch();
	defer{
		auto time_end = get_microseconds_epoch();
		auto dt = time_end - time_start;

		if (dt > 500) {
			LogEntry entry;
			entry.message = "Keyboard hook blocked for: " + std::to_string(dt) + "us.";
			entry.tag.push_back("PERF");
			logs.lock_and_write(entry);
		}
	};


	if (n_code < 0) return CallNextHookEx(NULL, n_code, w_param, l_param);

	switch (w_param) {
	case WM_KEYUP:
	case WM_SYSKEYUP: {
		auto& arg = *(KBDLLHOOKSTRUCT*)l_param;
		KeyEntry entry;
		entry.key_code = (uint8_t)arg.vkCode;
		entry.timestamp = get_seconds_epoch();

		key_entries_to_add.push_back(entry);
		break;
	}
	default:
		break;
	}

	if (!key_entries_to_add.empty() && event_queue_cache.mutex.try_lock()) {
		defer{
			event_queue_cache.mutex.unlock();
			event_queue_cache.wait_var.notify_all();
		};

		for (auto& x : key_entries_to_add) event_queue_cache.keyboard.push_back(x);
		key_entries_to_add.resize(0);
	}

	return CallNextHookEx(NULL, n_code, w_param, l_param);
}

LRESULT CALLBACK mouse_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	thread_local std::vector<ClickEntry> click_entries_to_add;

	auto time_start = get_microseconds_epoch();
	defer{
		auto time_end = get_microseconds_epoch();
		auto dt = time_end - time_start;

		if (dt > 500) {
			LogEntry entry;
			entry.message = "Mouse hook blocked for: " + std::to_string(dt) + "us.";
			entry.tag.push_back("PERF");
			logs.lock_and_write(entry);
		}
	};

	if (n_code < 0) return CallNextHookEx(NULL, n_code, w_param, l_param);


	switch (w_param) {
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_MOUSEWHEEL: {
		auto& arg = *(MSLLHOOKSTRUCT*)l_param;
		ClickEntry click;
		click.timestamp = get_seconds_epoch();
		switch (w_param) {
		case WM_LBUTTONUP:
			click.button_code = (uint8_t)MouseState::ButtonMap::Left;
			break;
		case WM_RBUTTONUP:
			click.button_code = (uint8_t)MouseState::ButtonMap::Right;
			break;
		case WM_MOUSEWHEEL:
			if ((short)(arg.mouseData >> (8 * sizeof(WORD))) > 0) {
				click.button_code = (uint8_t)MouseState::ButtonMap::Wheel_Up;
			}
			else {
				click.button_code = (uint8_t)MouseState::ButtonMap::Wheel_Down;
			}
			break;
		case WM_MBUTTONUP:
			click.button_code = (uint8_t)MouseState::ButtonMap::Wheel;
			break;
		case WM_XBUTTONUP:
			if ((arg.mouseData >> (8 * sizeof(WORD))) == XBUTTON1) {
				click.button_code = (uint8_t)MouseState::ButtonMap::Mouse_3;
			}
			else {
				click.button_code = (uint8_t)MouseState::ButtonMap::Mouse_4;
			}
			break;
		}

		click.x = arg.pt.x;
		click.y = arg.pt.y;
		click_entries_to_add.push_back(click);

		break;
	}
	default:
		break;
	}

	if (!click_entries_to_add.empty() && event_queue_cache.mutex.try_lock()) {
		defer{
			event_queue_cache.mutex.unlock();
			event_queue_cache.wait_var.notify_all();
		};

		for (auto& x : click_entries_to_add) event_queue_cache.click.push_back(x);

		click_entries_to_add.resize(0);
	}

	return CallNextHookEx(NULL, n_code, w_param, l_param);
}

LRESULT CALLBACK event_hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	thread_local std::unordered_map<HWND, std::uint64_t> opened;
	thread_local std::vector<AppUsage> entries_to_add;

	char big_buffer[1024];
	GetModuleFileNameA(NULL, big_buffer, sizeof(big_buffer));

	switch(n_code) {
		case HCBT_CREATEWND:{
			WCHAR wide_buffer[MAX_PATH] = {};
			DWORD proc_id = 0;
			GetWindowThreadProcessId((HWND)w_param, &proc_id);
			auto handle_process =
				OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, proc_id);

			GetModuleFileNameExW(handle_process, NULL, wide_buffer, MAX_PATH);
			opened[(HWND)w_param] = get_microseconds_epoch();
			break;
		}
		case HCBT_DESTROYWND: {
			if (opened.count((HWND)w_param) == 0) break;

			AppUsage use;
			use.timestamp_start = opened[(HWND)w_param];
			use.timestamp_end = get_microseconds_epoch();

			WCHAR wide_buffer[MAX_PATH] = {};
			GetWindowTextW((HWND)w_param, wide_buffer, MAX_PATH);

			auto size = WideCharToMultiByte(
				CP_UTF8, 0, wide_buffer, -1, nullptr, 0, NULL, NULL
			);
			if (size == 1) break; // If after that we don't know the name then there is no point to continue.

			use.doc_name = {};
			WideCharToMultiByte(
				CP_UTF8,
				0,
				wide_buffer,
				-1,
				use.doc_name.data(),
				use.doc_name.size(),
				NULL,
				NULL
			);


			DWORD proc_id = 0;
			GetWindowThreadProcessId((HWND)w_param, &proc_id);
			auto handle_process =
				OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, proc_id);

			GetModuleFileNameExW(handle_process, NULL, wide_buffer, MAX_PATH);
			size = WideCharToMultiByte(
				CP_UTF8, 0, wide_buffer, -1, nullptr, 0, NULL, NULL
			);
			if (size > 1) {
				WideCharToMultiByte(
					CP_UTF8,
					0,
					wide_buffer,
					-1,
					use.exe_name.data(),
					use.exe_name.size(),
					NULL,
					NULL
				);
			} else {
				auto default_string = "Internal";
				for (size_t i = 0; i < strlen(default_string); ++i) {
					use.exe_name[i] = default_string[i];
				}
			}

			entries_to_add.push_back(use);
			opened.erase((HWND)w_param);
			break;
		}
	};

	if (!entries_to_add.empty() && event_queue_cache.mutex.try_lock()) {
		defer {
			entries_to_add.clear();
			event_queue_cache.mutex.unlock();
			event_queue_cache.wait_var.notify_all();
		};

		for (auto& x : entries_to_add)
			event_queue_cache.app_usages.push_back(x);
	}

	return CallNextHookEx(NULL, n_code, w_param, l_param);
}

ClickEntry transform_click_to_canonical(ClickEntry x) noexcept;
void update_displays_from_click(MouseState& state, ClickEntry x) noexcept;

void event_queue_process() noexcept {
	while (shared.hook_window != nullptr) {
		std::unique_lock lk{ event_queue_cache.mutex };
		event_queue_cache.wait_var.wait(lk, [] {
			return
				!event_queue_cache.click.empty() ||
				!event_queue_cache.display.empty() ||
				!event_queue_cache.keyboard.empty() ||
				!event_queue_cache.app_usages.empty();
		});
		event_queue_cache.event_received = false;

		if (
			shared.mouse_state &&
			(!event_queue_cache.click.empty() || !event_queue_cache.display.empty())&&
			// Maybe we should be more aggresive and do a lock here instead ?
			shared.mut_mouse_state.try_lock()
		) {
			defer{ shared.mut_mouse_state.unlock(); };

			for (auto& x : event_queue_cache.click) {
				update_displays_from_click(*shared.mouse_state, x);
				shared.mouse_state->increment_button(transform_click_to_canonical(x));
			}
			event_queue_cache.click.clear();
			event_queue_cache.display.clear();
		}

		if (
			shared.keyboard_state &&
			!event_queue_cache.keyboard.empty() &&
			shared.mut_keyboard_state.try_lock()
		) {
			defer{ shared.mut_keyboard_state.unlock(); };
				
			for (auto x : event_queue_cache.keyboard) shared.keyboard_state->increment_key(x);
			event_queue_cache.keyboard.clear();
		}
		
		if (
			shared.event_state &&
			!event_queue_cache.app_usages.empty() &&
			shared.mut_event_state.try_lock()
		) {
			defer{ shared.mut_event_state.unlock(); };

			for (auto& x : event_queue_cache.app_usages)
				shared.event_state->register_event(x);

			event_queue_cache.app_usages.clear();
		}
	}
}

ClickEntry transform_click_to_canonical(ClickEntry x) noexcept {
	auto screens = get_all_screens();

	x.x += screens.main_x;
	x.y += screens.main_y;
	return x;
}

void update_displays_from_click(MouseState& state, ClickEntry x) noexcept {
#undef max
	constexpr auto MAX = std::numeric_limits<decltype(Display::timestamp_end)>::max();

	auto screen_is_display = [](Screen s, Display d) {
		return
			(memcmp(s.unique_hash_char, d.unique_hash_char, Display::Unique_Hash_Size) == 0) &&
			s.x == d.x && s.y == d.y && s.width == d.width && s.height == d.height;
	};

	auto screens = get_all_screens();

	std::unordered_set<Screen> screens_found;
	for (auto& d : state.display_entries) {
		// We are intersted only in the displays that are alive.
		if (d.timestamp_end != MAX) continue;

		bool found = false;
		for (auto& y : screens.screens) {
			if (screen_is_display(y, d)) {
				found = true;
				screens_found.insert(y);
				break;
			}
		}

		// If we can't find the registered display d in the actual screen set 'screens'
		// Then that mean that d has been disconnected and we should terminate it
		// taking x.timestamp as it's death time.
		if (!found) {
			d.timestamp_end = x.timestamp;
		}
	}

	// Now the screens in 'screens.screens' that are _not_ in (screens_found'
	// are screens that we see for the first time ever ! So we simply register them.
	for (auto& s : screens.screens) {
		if (screens_found.count(s) > 0) continue;

		Display d;
		d.x = s.x;
		d.y = s.y;
		memcpy_s(
			&d.unique_hash_char,
			Display::Unique_Hash_Size,
			&s.unique_hash_char,
			Display::Unique_Hash_Size
		);
		memset(d.custom_name, 0, Display::Custom_Name_Size);
		d.width = s.width;
		d.height = s.height;
		d.timestamp_start = x.timestamp;
		d.timestamp_end = MAX;

		state.display_entries.push_back(d);
	}
}

#define PROFILER_BEGIN_SEQ(x)
#define PROFILER_END_SEQ()
#define PROFILER_SEQ(x)

std::optional<HGLRC> create_gl_context(HWND handle_window) noexcept {
	PROFILER_BEGIN_SEQ("DC");
	auto dc = GetDC(handle_window);
	if (!dc) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	defer{ ReleaseDC(handle_window, dc); };

	PROFILER_SEQ("Pixel");
	PIXELFORMATDESCRIPTOR pixel_format{};
	pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixel_format.nVersion = 1;
	pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pixel_format.cColorBits = 32;
	pixel_format.cAlphaBits = 8;
	pixel_format.iLayerType = PFD_MAIN_PLANE;

	PROFILER_BEGIN_SEQ("choose");
	auto suggested_pixel_format = ChoosePixelFormat(dc, &pixel_format);
	if (!suggested_pixel_format) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	PROFILER_SEQ("describe");
	auto result = DescribePixelFormat(
		dc, suggested_pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format
	);
	if (!result) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	PROFILER_SEQ("set");
	if (!SetPixelFormat(dc, suggested_pixel_format, &pixel_format)) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	PROFILER_END_SEQ();

	PROFILER_SEQ("first context");
	auto gl_context = wglCreateContext(dc);
	if (!gl_context) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	if (!wglMakeCurrent(dc, gl_context)) {
		wglDeleteContext(gl_context);

		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	PROFILER_SEQ("glew");
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		printf("Can't init glew\n");
		return gl_context;
	}

	static int attribs[] = {
	#ifndef NDEBUG
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
	#endif
		0
	};

	PROFILER_SEQ("second context");
	auto gl = wglCreateContextAttribsARB(dc, nullptr, attribs);
	if (!gl) {
		auto err = glGetError();
		printf("%s", std::to_string(err).c_str());

		return gl_context;
	}

	if (!wglMakeCurrent(dc, gl)) {
		wglDeleteContext(gl);
		return gl_context;
	}


	PROFILER_SEQ("destroy");
	wglDeleteContext(gl_context);
	PROFILER_END_SEQ();
	return (HGLRC)gl;
}
void destroy_gl_context(HGLRC gl_context) noexcept {
	// >TODO error handling
	if (!wglDeleteContext(gl_context)) {
		printf("%s", get_last_error_message()->c_str());
	}
}

std::optional<std::string> get_last_error_message() noexcept {
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::nullopt; //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	auto flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;
	flags |= FORMAT_MESSAGE_FROM_SYSTEM;
	flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
	size_t size = FormatMessageA(
		flags, nullptr, errorMessageID, 0, (LPSTR)&messageBuffer, 0, nullptr
	);

	std::string message(messageBuffer, size);

	LocalFree(messageBuffer);
	return message;
}

void make_mail() noexcept {
	shared.mail_slot = CreateMailslot(Mail_Name, 0, MAILSLOT_WAIT_FOREVER, nullptr);

	if (shared.mail_slot == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Error creating mail slot\n");
	}
}

std::optional<Mail_Message> read_mail() noexcept {
	if (shared.mail_slot == INVALID_HANDLE_VALUE) return std::nullopt;
	DWORD cbMessage = 0;
	DWORD cMessage = 0;
	DWORD cbRead = 0; 
	BOOL fResult; 
	LPTSTR lpszBuffer; 
	TCHAR achID[80]; 
	DWORD cAllMessages; 
	OVERLAPPED ov;

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("SP_Event"));
	if (!hEvent) return std::nullopt;

	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.hEvent = hEvent;

	fResult = GetMailslotInfo(
		shared.mail_slot, // mailslot handle 
		nullptr,
	    &cbMessage,                   // size of next message 
	    &cMessage,                    // number of messages 
	    nullptr
	);              // no read time-out 

	if (!fResult) {
	    fprintf(stderr, "GetMailslotInfo failed with %d.\n", (int)GetLastError()); 
	    return std::nullopt;
	}

	if (cbMessage == MAILSLOT_NO_MESSAGE) {
	    return std::nullopt;
	} 

	cAllMessages = cMessage;

	// Allocate memory for the message.

	lpszBuffer = (LPTSTR)GlobalAlloc(GPTR, lstrlen((LPTSTR) achID) * sizeof(TCHAR) + cbMessage);

	if(!lpszBuffer) return std::nullopt;

	lpszBuffer[0] = '\0'; 

	fResult = ReadFile(shared.mail_slot, lpszBuffer, cbMessage, &cbRead, &ov);

	if (!fResult) {
	    fprintf(stderr, "ReadFile failed with %d.\n", (int)GetLastError());
	    GlobalFree((HGLOBAL) lpszBuffer);
	    return std::nullopt;
	}

	// Concatenate the message and the message-number string. 


	GlobalFree((HGLOBAL) lpszBuffer); 

	fResult = GetMailslotInfo(
		shared.mail_slot,              // mailslot handle 
	    (LPDWORD) NULL,               // no maximum message size 
	    &cbMessage,                   // size of next message 
	    &cMessage,                    // number of messages 
	    (LPDWORD) NULL                // no read time-out
	);

	if (!fResult) {
	    fprintf(stderr, "GetMailslotInfo failed (%d)\n", (int)GetLastError());
	    return std::nullopt;
	}

	CloseHandle(hEvent);

	return *reinterpret_cast<Mail_Message*>(lpszBuffer);
}