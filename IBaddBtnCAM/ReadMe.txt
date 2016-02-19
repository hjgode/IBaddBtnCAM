========================================================================
    WIN32 APPLICATION : IBaddBtnCAM Project Overview
========================================================================

AppWizard has created this IBaddBtnCAM application for you.  

Add a button to Enterprise Browser (EB) to launch a camera application


#SOLUTION
FINALLY GOING with injectDLL that watches for the start of IntermecBrowser an then launches
IBaddCAMbtn.

When IntermecBrowser ends, IBaddCAMbtn will be terminated by IBinjectDLL.dll on DLL_DETACH.

#Installation
copy IBinjectDLL.dll to \windows
copy IBaddCAMbtn to \windows

add the injectDLL registry

REGEDIT4
[HKEY_LOCAL_MACHINE\System\Kernel]
"InjectDLL"=hex(7):\
      5C,77,69,6E,64,6F,77,73,5C,49,42,69,6E,6A,65,63,74,44,4C,4C,2E,64,6C,6C,\
      00,00

(the hex is a multisz saying "\Windows\IBinjectDLL.dll" without quotes)
 
========================================================================
#NOTES
## analysis

EB uses several windows

desktop--+--"IntermecBrowser"
         |   +--"Button" "ToggleButton"
         |   +--"DISPLAYCLASS"
         |       +--"PIEHTML"
         |           +html elements...
         +--"Dock" "Dock" (just hides the windows menu bar), disabled window!
         +--"Button1"
         +--"Button2"         
         +--"Button3"         
         +--"Button4"         
         +--"Button5"     
         +--"TopBar"

hwnd		procID		class					title					pos/size				state
0x7c08e370	0x166f6afe	('intermecbrowser.exe')	'IntermecBrowser'	''	0;38/480;732 (480x694)	[visible]
0x7c08f0d0	0x166f6afe	('intermecbrowser.exe')	'Button5'	'Button5'	350;737/405;795 (55x58)	[visible]
0x7c08ef80	0x166f6afe	('intermecbrowser.exe')	'Button4'	'Button4'	213;737/268;795 (55x58)	[visible]
0x7c08ee30	0x166f6afe	('intermecbrowser.exe')	'Button3'	'Button3'	144;737/199;795 (55x58)	[visible]
0x7c08ece0	0x166f6afe	('intermecbrowser.exe')	'Button2'	'Button2'	75;737/130;795 (55x58)	[visible]
0x7c08eb40	0x166f6afe	('intermecbrowser.exe')	'Button1'	'Button1'	6;737/61;795 (55x58)	[visible]
0x7c08e9f0	0x166f6afe	('intermecbrowser.exe')	'Dock'	'Dock'	0;732/480;800 (480x68)	[visible]
0x7c08e4a0	0x166f6afe	('intermecbrowser.exe')	'TopBar'	'TopBar'	0;0/480;38 (480x38)	[visible]
0x7c08fc50	0x166f6afe	('intermecbrowser.exe')	'OLEAUT32'	''	0;0/0;0 (0x0)	[hidden]
0x7c08e8a0	0x166f6afe	('intermecbrowser.exe')	'COMPIMEUI'	'COMPIMEUI'	0;0/0;0 (0x0)	[hidden]
0x7c08e790	0x166f6afe	('intermecbrowser.exe')	'Ime'	'Default Ime'	0;0/1;1 (1x1)	[hidden]
    
the idea is to show a button like window with EB like Button1 to Button5 are displayed

"Dock" window can not be used as it is a disabled window

Subclassing a 'foreign' windows is not possible. Either use an injectDLL or simply some threads that watches 
the state of the IntermecBrowser window.

##watching the "Dock"
on 'down', WM_WINDOWPOSCHANGED with lParam=0x256bC71C (WINDOWPOS struct)
on 'up', WM_WINDOWPOSCHANGED with lParam=0x256bc52c (WINDOWPOS struct)

#############################################################################
//======================================================================
// 
// Static variables for task bar subclassing 
// 
static LRESULT CALLBACK TaskWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); //CallBAck for TaskBar Hook
static HWND g_hWndTask = NULL; 
static WNDPROC g_fnProcTask = NULL; 

//----------------------------------------------------------------------
//HOOK Into TaskBar WndProc
//
BOOL HookWindow(TCHAR* winClass, TCHAR* winTitle) 
{ 
	BOOL bRet=FALSE;
	// 
	// Already hooked? 
	// 
	if(g_fnProcTask) 
		return FALSE; 
	g_hWndTask = FindWindow(winClass, winTitle); 
	if(g_hWndTask) { 
		g_fnProcTask = (WNDPROC)GetWindowLong(g_hWndTask, GWL_WNDPROC); 
		SetLastError(0);
		LONG oldWndProc = SetWindowLong(g_hWndTask, GWL_WNDPROC, (LONG)TaskWndProc); 
		DWORD dwErr=GetLastError();
		if(oldWndProc==0 && dwErr!=0){
			DEBUGMSG(1, (L"Windows hook failed: %i\n", dwErr));
		}
		else if(oldWndProc!=0 && dwErr==0){
			DEBUGMSG(1, (L"Windows hook OK fn=0x%08x, res=0x%08x\n", oldWndProc, dwErr));
			bRet=TRUE;
		}
	} 
	return bRet; 
} 

//----------------------------------------------------------------------
BOOL UnhookWindow() 
{ 
	// 
	// Already freed? 
	// 
	if(!g_fnProcTask) 
		return FALSE; 
	SetWindowLong(g_hWndTask, GWL_WNDPROC, (LONG)g_fnProcTask); 
	g_fnProcTask = NULL; 
	return TRUE; 
} 
//----------------------------------------------------------------------
// TaskWndProc // 
// Handles the messages to filter // 
LRESULT TaskWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	DEBUGMSG(1, (L"TaskWndProc: msg=0x%08x, wParam=0x%08x, lParam=0x%08x\n", msg, wParam, lParam));
	switch(msg){
		case WM_WINDOWPOSCHANGED:
			DEBUGMSG(1, (L"TaskWndProc: WM_WINDOWPOSCHANGED hi=%i, lo=%i\n", HIWORD(lParam), LOWORD(lParam)));
			break;
		default:
			break;
	}
	//if(msg == WM_LBUTTONUP) { 
	//	RECT rc; 
	//	POINT pt; 
	//	rc.left = 240 - 26; 
	//	rc.top = 0; 
	//	rc.bottom = 26; 
	//	rc.right = 240; 
	//	pt.x = LOWORD(lParam); 
	//	pt.y = HIWORD(lParam); 
	//	if(PtInRect(&rc, pt)) { 
	//		//PostMessage(g_hWnd, WM_CLOSE, 0, 0); 
	//		//simply do nothing
	//		return true;
	//		//return CallWindowProc( g_fnProcTask, hWnd, WM_MOUSEMOVE, 0, MAKELPARAM(200, 0)); 
	//	} 
	//} 
	return CallWindowProc(g_fnProcTask, hWnd, msg, wParam, lParam); 
}
/////////////////////////////////////////////////////////////////////////////s