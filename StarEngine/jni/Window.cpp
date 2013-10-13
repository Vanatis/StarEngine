#ifdef _WIN32

#include "Window.h"
#include <string>
#include "SceneManaging/SceneManager.h"
#include "AssetManaging/TextureManager.h"
#include "Logger.h"
#include "TimeManager.h"
#include "Helpers/Helpers.h"
#include "Helpers/Filepath.h"
#include "Input/XMLContainer.h"
#include "Input/XMLFileParser.h"
#include "GraphicsManager.h"
#include "StarEngine.h"

#pragma comment(lib, "opengl32.lib")

namespace star
{
	const Window::psip Window::CLASS_STYLES[] = {
			psip(_T("CS_DBLCLKS"), CS_DBLCLKS),
			psip(_T("CS_HREDRAW"), CS_HREDRAW),
			psip(_T("CS_NOCLOSE"), CS_NOCLOSE),
			psip(_T("CS_PARENTDC"), CS_PARENTDC),
			psip(_T("CS_VREDRAW"), CS_VREDRAW)
		};

	const Window::psip Window::WINDOW_STYLES[] = {
			psip(_T("WS_BORDER"), WS_BORDER),
			psip(_T("WS_CAPTION"), WS_CAPTION),
			psip(_T("WS_CHILD"), WS_CHILD),
			psip(_T("WS_CHILDWINDOW"), WS_CHILDWINDOW),
			psip(_T("WS_CLIPCHILDREN"), WS_CLIPCHILDREN),
			psip(_T("WS_CLIPSIBLINGS"), WS_CLIPSIBLINGS),
			psip(_T("WS_DISABLED"), WS_DISABLED),
			psip(_T("WS_DLGFRAME"), WS_DLGFRAME),
			psip(_T("WS_GROUP"), WS_GROUP),
			psip(_T("WS_HSCROLL"), WS_HSCROLL),
			psip(_T("WS_ICONIC"), WS_ICONIC),
			psip(_T("WS_MAXIMIZE"), WS_MAXIMIZE),
			psip(_T("WS_MAXIMIZEBOX"), WS_MAXIMIZEBOX),
			psip(_T("WS_MINIMIZE"), WS_MINIMIZE),
			psip(_T("WS_MINIMIZEBOX"), WS_MINIMIZEBOX),
			psip(_T("WS_OVERLAPPED"), WS_OVERLAPPED),
			psip(_T("WS_OVERLAPPEDWINDOW"), WS_OVERLAPPEDWINDOW),
			psip(_T("WS_POPUP"), WS_POPUP),
			psip(_T("WS_POPUPWINDOW"), WS_POPUPWINDOW),
			psip(_T("WS_SIZEBOX"), WS_SIZEBOX),
			psip(_T("WS_SYSMENU"), WS_SYSMENU),
			psip(_T("WS_TABSTOP"), WS_TABSTOP),
			psip(_T("WS_THICKFRAME"), WS_THICKFRAME),
			psip(_T("WS_TILED"), WS_TILED),
			psip(_T("WS_TILEDWINDOW"), WS_TILEDWINDOW),
			psip(_T("WS_VISIBLE"), WS_VISIBLE),
			psip(_T("WS_VSCROLL"), WS_VSCROLL),
			psip(_T("WS_SYSMENU"), WS_SYSMENU)
		};

	Window *Window::m_pInstance = nullptr;

	Window * Window::GetInstance()
	{
		if(m_pInstance == nullptr)
		{
			m_pInstance = new Window();
		}
		return m_pInstance;
	}

	void Window::Initialize(HINSTANCE instance, BaseGame * pBaseGame)
	{
		ASSERT(!m_IsInitialized, _T("Engine is already initialized!"));
		if(!m_IsInitialized)
		{
			mGamePtr = pBaseGame;
			Logger::GetInstance()->Initialize();

			m_IsInitialized = true;
			XMLContainer winManifest;
			XMLFileParser manifestParser(EMPTY_STRING, _T("Win32Manifest.xml"));
			manifestParser.Read(winManifest);

			//set console position
			HWND consoleHandle = GetConsoleHWND();
			if(consoleHandle != NULL)
			{
				auto consoleAttributes = winManifest[_T("console")]->GetAttributes();
				int x = string_cast<int>(consoleAttributes[_T("x")]);
				int y = string_cast<int>(consoleAttributes[_T("y")]);
				int width = string_cast<int>(consoleAttributes[_T("width")]);
				int height = string_cast<int>(consoleAttributes[_T("height")]);
				MoveWindow(consoleHandle, x, y, width, height, true);
			}

			mContext.mTimeManager = mTimeManager;

			WNDCLASSEX wndClass;
			wndClass.cbSize = sizeof(WNDCLASSEX);

			auto assets_settings = winManifest[_T("assets")]->GetAttributes();
			mAssetsRoot = assets_settings[_T("root")];
			Filepath::SetAssetsRoot(mAssetsRoot);

			wndClass.style = 0;
			auto class_map = winManifest[_T("class_styles")];
			auto class_style_it = class_map->lower_bound(_T("style"));
			auto class_style_end_it = class_map->upper_bound(_T("style"));
			do
			{
				wndClass.style |= CastStringToClassStyle(class_style_it->second->GetValue());
				++class_style_it;
			}
			while(class_style_it != class_style_end_it);

			wndClass.lpfnWndProc = wEventsProc;
			wndClass.cbClsExtra = 0;
			wndClass.cbWndExtra = 0;
			wndClass.hInstance = instance;
			wndClass.hIcon = NULL;
			wndClass.hCursor = NULL;
			wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			wndClass.lpszMenuName = NULL;
			wndClass.lpszClassName = winManifest[_T("class")]->GetValue().c_str();
			wndClass.hIconSm = NULL;

			uint32 windowStyles(0);
			auto window_map = winManifest[_T("window_styles")];
			auto win_style_it = window_map->lower_bound(_T("style"));
			auto win_style_end_it = window_map->upper_bound(_T("style"));
			do
			{
				windowStyles |= CastStringToWinStyle(win_style_it->second->GetValue());
				++win_style_it;
			}
			while(win_style_it != win_style_end_it);

	#pragma warning ( disable : 4800 )
			ASSERT(RegisterClassEx(&wndClass), _T("Couldn't register the Windows Class!"));
	#pragma warning ( default : 4800 )

			m_CanGoFullScreen =
				string_cast<bool>(winManifest[_T("allow_fullscreen")]->GetValue());
			m_ManipulateWindowResolution = 
				string_cast<bool>(winManifest[_T("allow_fullscreen")]->GetAttributes()[_T("manipulate_window_resolution")]);
			m_UpdateGameWhenInactive = 
				string_cast<bool>(winManifest[_T("update_game_when_inactive")]->GetValue());

			auto & resolution = winManifest[_T("resolution")]->GetAttributes();
			auto & position = winManifest[_T("position")]->GetAttributes();

			int position_x = string_cast<int>(position[_T("x")]);
			int position_y = string_cast<int>(position[_T("y")]);
			int position_width = string_cast<int>(resolution[_T("width")]);
			int position_height = string_cast<int>(resolution[_T("height")]);

			if(position_x == -1)
			{
				position_x = int(GetSystemMetrics(SM_CXSCREEN)) / 2;
				position_x -= position_width / 2;
			}

			if(position_y == -1)
			{
				position_y = int(GetSystemMetrics(SM_CYSCREEN)) / 2;
				position_y -= position_height / 2;
			}

			tstring windowsTitle = winManifest[_T("title")]->GetValue();
			StarEngine::GetInstance()->SetGameTitle(windowsTitle);

			mHandle = CreateWindowA(	string_cast<std::string>(winManifest[_T("class")]->GetValue()).c_str(),
									string_cast<std::string>(windowsTitle).c_str(),
									windowStyles,
									position_x,
									position_y,
									position_width,
									position_height,
									NULL, NULL, instance, NULL);

			ASSERT(mHandle != NULL, _T("Couldn't create the window."));

			ShowWindow(mHandle, SW_SHOWNORMAL);
			UpdateWindow(mHandle);

					PIXELFORMATDESCRIPTOR pixelFormatDesc = {
				sizeof(PIXELFORMATDESCRIPTOR),
				1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
				PFD_TYPE_RGBA,
				32,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				24, 0, 0,
				PFD_MAIN_PLANE,
				0,
				0, 0, 0
			};
	
			mHDC = GetDC(mHandle); // Gets the display context
			ASSERT(mHDC != NULL, _T("Couldn't create the Display Context!"));

			int pixelFormat = ChoosePixelFormat(mHDC, &pixelFormatDesc); // Chooses the pixel format
			ASSERT(pixelFormat != 0, _T("Invalid pixel format!"));

			// Sets the pixel format
			ASSERT(SetPixelFormat(mHDC, pixelFormat, &pixelFormatDesc) != 0, _T("Couldn't set the pixel format!"));

			HGLRC hglrc = wglCreateContext(mHDC); // Creates the rendering context
			ASSERT(hglrc != NULL, _T("Couldn't create the rendering context!"));

			// Attaches the rendering context
			ASSERT(wglMakeCurrent(mHDC, hglrc) != 0, _T("Action couldn't be completed!"));

			MSG msg ={};

			auto cursor = winManifest[_T("cursor")];
			auto cursor_settings = cursor->GetAttributes();

			bool hide_cursor = string_cast<bool>(cursor_settings[_T("hidden")]);
			ShowCursor(!hide_cursor);

			auto clip_settings = cursor->at(_T("clip"))->GetAttributes();
			mClipRect.left = string_cast<long>(clip_settings[_T("x")]);
			if(mClipRect.left != -1)
			{
				RECT windowRect;
				GetWindowRect(mHandle, &windowRect);

				mClipRect.left += windowRect.left;
				long width = string_cast<long>(clip_settings[_T("width")]);
				if(width == -1)
				{
					mClipRect.right = windowRect.right;
				}
				else
				{
					mClipRect.right = mClipRect.left + width;
				}
				mClipRect.top = string_cast<long>(clip_settings[_T("y")]);
				mClipRect.top += windowRect.top;
				long height = string_cast<long>(clip_settings[_T("height")]);
				if(height == -1)
				{
					mClipRect.bottom = windowRect.bottom;
				}
				else
				{
					mClipRect.bottom = mClipRect.top + height;
				}

				CalculateRect(mClipRect);

				ClipCursor(&mClipRect);
			}
	
			mGamePtr->Initialize(position_width,position_height);

			// Main message loop:
			while(msg.message != WM_QUIT)
			{
				bool monitor_started(false);
				if(m_IsActive)
				{
					mTimeManager->StartMonitoring();
					monitor_started = true;
				}

				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if(m_IsActive) // We've processed all pending Win32 messages, and can now do a rendering update.
				{
					mGamePtr->Draw();
		
				}
				SwapBuffers(Window::mHDC); // Swaps display buffers

				if(m_IsActive)
				{
					mGamePtr->Update(mContext);
					SetWindowsTitle();
				}

				if(monitor_started)
				{
					mTimeManager->StopMonitoring();
				}
			}

			mGamePtr->End();
		}
	}

	Window::Window()
		: m_IsInitialized(false)
		, m_IsFullScreen(false)
		, m_CanGoFullScreen(false)
		, m_WindowMoved(false)
		, m_IsActive(true)
		, m_UpdateGameWhenInactive(false)
		, m_ManipulateWindowResolution(false)
		, m_SavedWindowState()
		, mGamePtr(nullptr)
		, mTimeManager(new TimeManager())
		, mHandle()
		, mOGLContext()
		, mHDC()
		, mClipRect()
		, mAssetsRoot()
	{
	}

	void Window::CalculateRect(RECT & rect)
	{
		rect.top += 25;
		rect.left += 5;
		rect.right -= 5;
		rect.bottom -= 5;
	}

	void Window::SetClipRect(const RECT & rect)
	{
		mClipRect = rect;
	}

	bool Window::IsCursorClipped() const
	{
		return mClipRect.top != -1;
	}

	void Window::ToggleFullScreen(HWND hWnd)
	{
		SetFullScreen(hWnd, !m_IsFullScreen);
	}

	void Window::SetFullScreen(HWND hWnd, bool fullscreen)
	{
		if(!m_IsFullScreen)
		{
			m_SavedWindowState.Maximized = IsZoomed(hWnd);
			if(m_SavedWindowState.Maximized)
			{
				// window can't be maximized in fullscreen modus
				SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
			}
			m_SavedWindowState.Style = GetWindowLong(hWnd, GWL_STYLE);
			m_SavedWindowState.ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
			GetWindowRect(hWnd, &m_SavedWindowState.WinRect);
		}

		m_IsFullScreen = fullscreen;
		auto hdc = GetDC(hWnd);

		WindowInactiveUpdate(false);

		if (m_IsFullScreen) 
		{
			DEVMODE fullscreenSettings;

			int screenWidth = GetDeviceCaps(hdc, HORZRES);
			int screenHeight = GetDeviceCaps(hdc, VERTRES);

			if(m_ManipulateWindowResolution)
			{
				screenWidth = m_SavedWindowState.WinRect.right - m_SavedWindowState.WinRect.left;
				screenHeight = m_SavedWindowState.WinRect.bottom - m_SavedWindowState.WinRect.top;
			}

			EnumDisplaySettings(NULL, 0, &fullscreenSettings);
			fullscreenSettings.dmPelsWidth        = screenWidth;
			fullscreenSettings.dmPelsHeight       = screenHeight;
			fullscreenSettings.dmBitsPerPel       = GetDeviceCaps(hdc, BITSPIXEL);
			fullscreenSettings.dmDisplayFrequency = GetDeviceCaps(hdc, VREFRESH);
			fullscreenSettings.dmFields           = DM_PELSWIDTH |
													DM_PELSHEIGHT |
													DM_BITSPERPEL |
													DM_DISPLAYFREQUENCY;

			SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
			SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);
			bool isChangeSuccessful = ChangeDisplaySettings(&fullscreenSettings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
			ASSERT(isChangeSuccessful, _T("Couldn't put the screen into fullscreen mode..."));
			ShowWindow(hWnd, SW_MAXIMIZE);
		}
		else
		{
			SetWindowLongPtr(hWnd, GWL_EXSTYLE, m_SavedWindowState.ExStyle);
			SetWindowLongPtr(hWnd, GWL_STYLE, m_SavedWindowState.Style);
			bool isChangeSuccessful = ChangeDisplaySettings(NULL, CDS_RESET) == DISP_CHANGE_SUCCESSFUL;
			ASSERT(isChangeSuccessful, _T("Couldn't put the screen into windowed mode..."));
			SetWindowPos(hWnd, HWND_NOTOPMOST, 
				m_SavedWindowState.WinRect.left,
				m_SavedWindowState.WinRect.top,
				m_SavedWindowState.WinRect.right - m_SavedWindowState.WinRect.left,
				m_SavedWindowState.WinRect.bottom - m_SavedWindowState.WinRect.top,
				SWP_SHOWWINDOW);
			ShowWindow(hWnd, SW_RESTORE);

			if (m_SavedWindowState.Maximized)
			{
				SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
			}
		}
	}

	void Window::UpdateClippingIfNeeded()
	{
		if(m_WindowMoved)
		{
			WindowInactiveUpdate(false);
			m_WindowMoved = false;
		}
	}

	void Window::SetWindowMoved()
	{
		m_WindowMoved = true;
	}

	void Window::SetWindowActive(bool active)
	{
		m_IsActive = active;
	}

	void Window::SetWindowsTitle() const
	{
		if (StarEngine::GetInstance()->HasTitleUpdated())
		{
			SetWindowText(mHandle, 
				(StarEngine::GetInstance()->m_Title + StarEngine::GetInstance()->m_SubTitle).c_str());
			StarEngine::GetInstance()->ResetTitleUpdateMark();
		}
	}

	HWND Window::GetConsoleHWND()
	{
		#define MY_BUFSIZE 1024 // Buffer size for console window titles.
		HWND hwndFound;         // This is what is returned to the caller.
		tchar pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
											// WindowTitle.
		tchar pszOldWindowTitle[MY_BUFSIZE]; // Contains original
											// WindowTitle.
    
		// Fetch current window title.
		GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

		// Format a "unique" NewWindowTitle.
		wsprintf(pszNewWindowTitle,_T("%d/%d"),
					GetTickCount(),
					GetCurrentProcessId());

		// Change current window title.
		SetConsoleTitle(pszNewWindowTitle);

		// Ensure window title has been updated.
		Sleep(40);

		// Look for NewWindowTitle.
		hwndFound=FindWindow(NULL, pszNewWindowTitle);

		// Restore original window title.
		SetConsoleTitle(pszOldWindowTitle);

		return(hwndFound);
	}

	LRESULT CALLBACK Window::wEventsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_DESTROY:
				PostQuitMessage(0);
				return 1;
			case WM_KILLFOCUS:
				Window::GetInstance()->WindowInactiveUpdate(true);
				break;
			case WM_SETFOCUS:
				Window::GetInstance()->WindowInactiveUpdate(false);
				break;
			case WM_LBUTTONUP:
				Window::GetInstance()->UpdateClippingIfNeeded();
				break;
			case WM_ACTIVATE:
				{
					bool active = !(LOWORD(wParam) == WA_INACTIVE);
					Window::GetInstance()->WindowInactiveUpdate(!active);
					Window::GetInstance()->SetWindowActive(
						Window::GetInstance()->UpdateGameWhenInactive() || active
						);
					if(!Window::GetInstance()->UpdateGameWhenInactive())
					{
						if(active)
						{
							StarEngine::GetInstance()->SetActive();
						}
						else
						{
							StarEngine::GetInstance()->SetInactive();
						}
					}
				}
				break;
			case WM_MOVE:
				UpdateWindowClipping(hWnd);
				Window::GetInstance()->SetWindowMoved();
				Window::GetInstance()->WindowInactiveUpdate(true);
				break;
			case WM_SYSCOMMAND:
				if (Window::GetInstance()->CanGoFullScreen()
					&& SC_KEYMENU == (wParam & 0xFFF0)
					&& GetKeyState(VK_RETURN))
				{
					//alt - enter => FullScreen
					Window::GetInstance()->ToggleFullScreen(hWnd);
					return 1;
				}
				break;
			case WM_SIZE:
				UpdateWindowClipping(hWnd);
				break;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	Window::~Window(void)
	{
	}

	void Window::WindowInactiveUpdate(bool inactive)
	{
		if(m_IsFullScreen || (inactive && mClipRect.left != -1))
		{
			ClipCursor(NULL);
		}
		else if(mClipRect.left != -1)
		{
			SetActiveWindow(mHandle);
			ClipCursor(NULL);
			ClipCursor(&mClipRect);
		}
	}

	const tstring & Window::GetAssetRoot() const
	{
		return mAssetsRoot;
	}

	const HDC & Window::GetHDC() const
	{
		return mHDC;
	}

	const HWND & Window::GetHandle() const
	{
		return mHandle;
	}

	int Window::CastStringToClassStyle(const tstring & style)
	{
		for ( int32 i = 0 ; i < CLASS_STYLES_COUNT ; ++i )
		{
			if(style == CLASS_STYLES[i].first)
			{
				return CLASS_STYLES[i].second;
			}
		}
		ASSERT(false, _T("Invalid class style found in Win32Manifest.xml"));
		return NULL;
	}

	int Window::CastStringToWinStyle(const tstring & style)
	{
		for ( int32 i = 0 ; i < WINDOW_STYLES_COUNT ; ++i )
		{
			if(style == WINDOW_STYLES[i].first)
			{
				return WINDOW_STYLES[i].second;
			}
		}
		ASSERT(false, _T("Invalid window style found in Win32Manifest.xml"));
		return NULL;
	}

	void UpdateWindowClipping(HWND hWnd)
	{
		RECT winRect;
		GetWindowRect(hWnd, &winRect);
		GraphicsManager::GetInstance()->SetWindowDimensions(
			winRect.right - winRect.left, 
			winRect.bottom - winRect.top);
		if(Window::GetInstance()->IsCursorClipped()
					&& Window::GetInstance()->IsInitialized())
		{
			Window::GetInstance()->CalculateRect(winRect);
			Window::GetInstance()->SetClipRect(winRect);
		}
	}
}

#endif