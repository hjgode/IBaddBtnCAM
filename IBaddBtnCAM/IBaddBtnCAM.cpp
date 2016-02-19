// IBaddBtnCAM.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "IBaddBtnCAM.h"

#define WM_STOP WM_USER+1

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWnd=NULL;

//colors
unsigned long colorRed=RGB(255,0,0);
//======================================================================
extern "C" __declspec(dllimport) DWORD SetProcPermissions(DWORD);
extern "C" __declspec(dllimport) DWORD GetCurrentPermissions(void);
DWORD orgPermissions;

#define HGAP     6 //100 //200                     // Width of floating wnd
#define MARGIN   6
HBITMAP m_Bitmap;
TCHAR* exeName=L"\\Program Files\\Intermec Browser\\FullScreenCameraCNx.exe";

RECT rectScreen;

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//user messages
WORD WM_DOCKDOWN =	WM_USER + 1;
WORD WM_DOCKUP	 =	WM_USER + 2;

//=============================
//register custom messages
void register_User_Msgs(){
	WM_DOCKDOWN = RegisterWindowMessage(L"WM_DOCKDOWN");
	WM_DOCKUP = RegisterWindowMessage(L"WM_DOCKUP");
}

//=============================
//a thread to watch window for changes
HANDLE hThreadWatch=NULL;
DWORD  idThreadWatch;
HANDLE evStopThread=NULL;					//for named event
TCHAR* strEventStopThread=L"stopThread";	//name of event

DWORD watchThread(PVOID lpVoid){
	DEBUGMSG(1, (L"watchThread started\n"));
	HWND hwndMain = (HWND)lpVoid;
	HANDLE waitHandle=evStopThread;
	BOOL stopMe=FALSE;
	DWORD dwWaitResult;
	static WORD wState=-1;
	while(!stopMe){
		dwWaitResult = WaitForSingleObject(waitHandle, 1000);
		switch(dwWaitResult){
			case WAIT_OBJECT_0:
				stopMe=TRUE;
				break;
			case WAIT_TIMEOUT:
				//check window
				HWND hwndTest=FindWindow(L"IntermecBrowser", NULL);
				if(hwndTest){
					RECT rect;
					//undocked WatchThread: rect = 0/0, 480/800
					//docked   WatchThread: rect = 0/38, 480/732
					GetWindowRect(hwndTest, &rect);
					DEBUGMSG(1, (L"WatchThread: rect = %i/%i, %i/%i\n",rect.left, rect.top, rect.right, rect.bottom));
					WORD msg=WM_DOCKDOWN;
					if(rect.bottom==rectScreen.bottom)
						msg=WM_DOCKDOWN;
					else
						msg=WM_DOCKUP;
					if(msg!=wState){
						if(g_hWnd){
							PostMessage(hwndMain, msg, 0, 0);
						}
					}
					wState=msg;
				}
				else{

					//no IntermecBrowser, then EXIT
					ShowWindow(hwndMain, SW_MINIMIZE);
					//SendMessage(hwndMain, WM_STOP, 0, 0);
					SendMessage(hwndMain, WM_QUIT, 0, 0);
					wState=-1;
				}
				break;
		}
	}//while
	DEBUGMSG(1, (L"watchThread ended\n"));
	return 0;
}
void startThread(HWND hwnd){
	g_hWnd=hwnd;
	register_User_Msgs();
	if(!evStopThread)
		evStopThread=CreateEvent(NULL, FALSE, FALSE, strEventStopThread);
	hThreadWatch=CreateThread(NULL, 0, watchThread, hwnd, 0, &idThreadWatch);
}

void stopThread(){
	if(evStopThread)
		SetEvent(evStopThread);
	Sleep(1000);
}


//----------------------------------------------------------------------
/// startExe will start external app
void startExe(){
	PROCESS_INFORMATION pi;
	int iRet=CreateProcess(exeName, L"", NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);
	if(iRet!=0){
			DEBUGMSG(1, (L"Started: %s:pid=0x%08x\n", exeName, pi.dwProcessId));
			CloseHandle(pi.hProcess);
	}
	else{
		DEBUGMSG(1, (L"Could not start %s, %i\n", exeName, iRet));
	}
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	HWND hwndIB=NULL;

	while( (hwndIB=FindWindow(L"IntermecBrowser",NULL))==NULL){
		DEBUGMSG(1, (L"sleep\n"));
		Sleep(3000);
	}

	DEBUGMSG(1, (L"InitInstance...\n"));
	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	//orgPermissions = GetCurrentPermissions();
	//DWORD ProcPermission = SetProcPermissions(0xFFFFFFFF);

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IBADDBTNCAM));

	DEBUGMSG(1, (L"Message Loop...\n"));
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	//SetProcPermissions(orgPermissions);

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IBADDBTNCAM));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
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
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the device specific controls such as CAPEDIT and SIPPREF.
    //SHInitExtraControls();

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_IBADDBTNCAM, szWindowClass, MAX_LOADSTRING);

    //If it is already running, then focus on the window, and exit
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

	//need to know the screen size
	rectScreen.right=GetSystemMetrics(SM_CXSCREEN);
	rectScreen.bottom=GetSystemMetrics(SM_CYSCREEN);
	rectScreen.left=0; rectScreen.top=0;

	HWND hWndDock=FindWindow(L"Dock", L"Dock");
	if(hWndDock==NULL)
		return FALSE;
	HWND hWndDockParent=GetParent(hWndDock);	//window we attach to, Main Window will not work!
	
	//size
	HWND hwndButton4=FindWindow(L"Button4", L"Button4");	
	if(hwndButton4==NULL)
		return FALSE;
	
	//get button size
	RECT rcSize;
	GetClientRect(hwndButton4, &rcSize);
	int w=rcSize.right-rcSize.left;
	int h=rcSize.bottom-rcSize.top;

	DWORD dwBorderY = GetSystemMetrics(SM_CYBORDER);
	DWORD dwBorderX = GetSystemMetrics(SM_CXBORDER);

	//get position
	RECT rcPos;
	GetWindowRect(hwndButton4,&rcPos);

	hWnd = CreateWindowEx(
		0, //WS_EX_TOPMOST | WS_EX_ABOVESTARTUP, 
		szWindowClass, 
		NULL /* szTitle */, 
		WS_VISIBLE, //| WS_EX_ABOVESTARTUP, //WS_VISIBLE , // | WS_CHILD| WS_EX_TOOLWINDOW  | WS_POPUP | WS_NONAVDONEBUTTON, 
		rcPos.left + w + HGAP + MARGIN, 
		rcPos.top - dwBorderX/2,// + MARGIN,
		w ,// + dwBorderY, 
		h ,// + dwBorderX, 
		hWndDockParent, // NULL, 
		NULL, 
		hInstance, 
		NULL);

    if (!hWnd)
    {
        return FALSE;
    }

	g_hWnd=hWnd;

	//when we get here the Intermec Browser should be loaded, so show the cam symbol
    ShowWindow(hWnd, SW_SHOWNORMAL);// nCmdShow);
    UpdateWindow(hWnd);

	LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
	DEBUGMSG(1, (L"GetWindowLong GWL_SYTLE=%08x\n", lStyle));
	//DWORD dwLeft = readReg();
	//SetWindowPos(hWnd, HWND_TOPMOST, dwLeft, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

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
    HDC hdc, memdc;
	RECT rt;
	static HBRUSH hbrRed;

    static SHACTIVATEINFO s_sai;
	if(message==WM_DOCKDOWN){
		//hide me
		DEBUGMSG(1, (L"WM_DOCKDOWN"));
		ShowWindow(hWnd, SW_HIDE);
	}
	else if(message==WM_DOCKUP){
		//show me
		DEBUGMSG(1, (L"WM_DOCKUP"));
		ShowWindow(hWnd, SW_SHOWNORMAL);
		UpdateWindow(hWnd);
	}

    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_OK:
                    SendMessage (hWnd, WM_CLOSE, 0, 0);				
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
			g_hWnd=hWnd;
			startThread(hWnd);

			hbrRed = CreateSolidBrush(colorRed);

            break;
		case WM_LBUTTONUP:
			startExe();
			break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
            GetClientRect(hWnd, &rt);
			FillRect(hdc, &rt, hbrRed);

			m_Bitmap=LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
			memdc = CreateCompatibleDC(hdc);
			// Select the new bitmap
            SelectObject(memdc, m_Bitmap);
			// Copy the bits from the memory DC into the current dc
			BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, memdc, 0, 0, SRCCOPY);
			DeleteDC(memdc);
			DeleteObject(m_Bitmap);

            EndPaint(hWnd, &ps);
            break;
		case WM_STOP:
			DEBUGMSG(1, (L"WM_STOP"));
			stopThread();
			break;
		case WM_CLOSE:
			DEBUGMSG(1, (L"WM_CLOSE"));
			DestroyWindow(hWnd);
			break;
        case WM_DESTROY:
			DEBUGMSG(1, (L"WM_DESTROY"));
			stopThread();
			Sleep(1000);
			PostQuitMessage(0);
            break;
		case WM_QUIT:
			DEBUGMSG(1, (L"WM_QUIT"));
			stopThread();
			Sleep(1000);
			DestroyWindow(hWnd); //issues a WM_DESTROY and ends the app
			break;
        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            {
                // Create a Done button and size it.  
                SHINITDLGINFO shidi;
                shidi.dwMask = SHIDIM_FLAGS;
                shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
                shidi.hDlg = hDlg;
                SHInitDialog(&shidi);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}
