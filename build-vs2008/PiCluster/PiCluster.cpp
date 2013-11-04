// PiCluster.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "PiCluster.h"
#include "winsock.h"
#include "Tests.h"

#define MAX_LOADSTRING 100
//#define ZOOM
//#define SECOND_SCREEN

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
InstrumentCluster gCluster;						// Our instrument cluster
GraphicsContextWin gFontDB;						// Context for generating font info

// Font databases are global
FontDatabaseFile*	gFontErasDemi18 = NULL;		// 18 point Eras Demi ITC

// Images
BMPImage*			gWaterTempImage = NULL;
BMPImage*			gFuelImage		= NULL;
BMPImage*			gCarTopView		= NULL;
BMPImage*			gLeftArrowImage	= NULL;
BMPImage*			gRightArrowImage= NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
bool LoadFontResource(FontDatabaseFile** ptr, char* filename);
bool LoadImageResource(BMPImage** ptr, char* filename);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PICLUSTER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PICLUSTER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PICLUSTER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PICLUSTER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

#if defined(ZOOM) && defined(SECOND_SCREEN)
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	1920 + 0, 0, 1920, 1000, NULL, NULL, hInstance, NULL);
#elif defined(ZOOM) && !defined(SECOND_SCREEN)
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 1920, 1000, NULL, NULL, hInstance, NULL);
#elif !defined(ZOOM) && defined(SECOND_SCREEN)
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      1920 + 296, 266, 1296, 538, NULL, NULL, hInstance, NULL);
#else
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      296, 266, 1296, 538, NULL, NULL, hInstance, NULL);
#endif

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // Run the Pi tests
   Tests tests;
   tests.RunAllTests();

  // while(1);

   LoadFontResource(&gFontErasDemi18, "Eras Demi ITC.bin");
   LoadImageResource(&gWaterTempImage, "..\\..\\bitmaps\\Water.bmp");
   LoadImageResource(&gFuelImage, "..\\..\\bitmaps\\Fuel.bmp");
   LoadImageResource(&gCarTopView, "..\\..\\bitmaps\\CamaroGrey.bmp");
   LoadImageResource(&gLeftArrowImage, "..\\..\\bitmaps\\LeftArrow.bmp");
   LoadImageResource(&gRightArrowImage, "..\\..\\bitmaps\\RightArrow.bmp");
   
   Rect rect(0, 0, 1280, 480);
   gCluster.Init(rect);
   gFontDB.CreateFontDatabase("Eras Demi ITC", 18);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static uint64_t lastUpdateTimeMs = GetTickCount64();
	static int frames = 0;
	static float fps = 0.0f;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		{
			RECT winRect;
			hdc = BeginPaint(hWnd, &ps);
			GraphicsContextWin* ctx = (GraphicsContextWin*)&gCluster.GetPrimarySurface().GetGraphicsContext();
			gCluster.Update();
			gCluster.Draw();

			Rect dirty = gCluster.mDirty.GetDirtyRect();
			winRect.left  = dirty.x;
			winRect.top   = dirty.y;
			winRect.right = dirty.x + dirty.w;
			winRect.bottom= dirty.y + dirty.h;
			if (ps.rcPaint.left < winRect.left)
				winRect.left = ps.rcPaint.left;
			if (ps.rcPaint.top < winRect.top)
				winRect.top = ps.rcPaint.top;
			if (ps.rcPaint.bottom > winRect.bottom)
				winRect.bottom = ps.rcPaint.bottom;
			if (ps.rcPaint.right > winRect.right)
				winRect.right= ps.rcPaint.right;

			// Copy the back buffer to the front buffer
			ctx->CopyBackToFront(dirty);

#ifdef ZOOM
			StretchBlt(hdc, 0, 0, 2560, 960, ctx->GetDC(), 0, 0, 640, 240, SRCCOPY);
#else
			BitBlt(hdc, winRect.left, winRect.top, winRect.right-winRect.left, winRect.bottom-winRect.top, ctx->GetDC(), winRect.left, winRect.top, SRCCOPY);
#endif
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			SelectObject(hdc, GetStockObject(WHITE_PEN));

			// Draw the region
			//Rectangle(hdc, winRect.left, winRect.top, winRect.right, winRect.bottom);
			EndPaint(hWnd, &ps);

			frames++;
			if ((GetTickCount64() - lastUpdateTimeMs) > 1000)
			{
				float interval = (GetTickCount64() - lastUpdateTimeMs) / 1000.0;
				fps = frames / interval;
				frames = 0;
				lastUpdateTimeMs = GetTickCount64();
				ctx->mProfileData.Snapshot(true);
			}

			hdc = GetDC(hWnd);
			HFONT font = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
									CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Courier New");
			
			SelectObject(hdc, font);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(200, 200, 200));
			int x = 10;
			int y = 10;
			int deltaY = 15;
			char text[256];
			sprintf_s(text, 256, "%2.1f FPS", fps);
			BitBlt(hdc, x, y, 200, 200, ctx->GetDC(), x, y, SRCCOPY);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "Update:         %lld", ctx->mProfileData.mUpdate.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "Draw:           %lld", ctx->mProfileData.mDraw.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "CopyToPrimary:  %lld", ctx->mProfileData.mCopyToPrimary.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "CopyBackToFront:%lld", ctx->mProfileData.mCopyBackToFront.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "DrawTrapezoid:  %lld", ctx->mProfileData.mDrawTrapezoid.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "DrawLine:       %lld", ctx->mProfileData.mDrawLine.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "DrawText:       %lld", ctx->mProfileData.mDrawText.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "FloodFill:      %lld", ctx->mProfileData.mFloodFill.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			sprintf_s(text, 256, "Region:         %lld", ctx->mProfileData.mRegion.GetSnapshot()/1000L);
			TextOutA(hdc, x, y, text, strlen(text));	y += deltaY;

			DeleteDC(hdc);
			Sleep(1);
			//::InvalidateRect(hWnd, &winRect, false);
			::InvalidateRect(hWnd, NULL, false);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool LoadFontResource(FontDatabaseFile** ptr, char* filename)
{
	FILE* fp = NULL;
	if (!fopen_s(&fp, filename, "rb"))
	{
		// Read the file size
		uint32_t fileSize;
		fread(&fileSize, 1, 4, fp);
		fileSize = fileSize;
		*ptr = (FontDatabaseFile*)malloc(fileSize);
		(*ptr)->fileSize = fileSize;
		fread(&((*ptr)->fontName), 1, fileSize - 4, fp);
	}
	fclose(fp);
	return fp ? true : false;
}

bool LoadImageResource(BMPImage** ptr, char* filename)
{
	FILE* fp = NULL;
	do
	{
		if (!fopen_s(&fp, filename, "rb"))
		{
			// Read the ID, make sure it looks like a BMP
			char id[3] = { 0 };
			fread(id, 1, 2, fp);
			if (id[0] != 'B' || id[1] != 'M')
			   break;
			uint32_t fileSize;
			fread(&fileSize, 1, 4, fp);
			fileSize = fileSize;
			*ptr = (BMPImage*)malloc(fileSize);
			(*ptr)->id[0] = 'B';
			(*ptr)->id[1] = 'M';
			(*ptr)->bmpSize = fileSize;
			// Windows pads the 2 bytes of ID, so add two bytes
			fread((uint8_t*)(*ptr) + 8, 1, fileSize - 6, fp);
		}
	} while (false);
	if (fp)
		fclose(fp);
	return fp ? true : false;
}









