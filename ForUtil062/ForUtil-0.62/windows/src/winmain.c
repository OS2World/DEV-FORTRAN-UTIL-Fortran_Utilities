static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winfflow/RCS/winmain.c,v 1.2 1996/08/27 19:31:31 koen Exp koen $";
/*****
* winmain.c: WinMain and related for winfflow.
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	08-08-1996
* Last modification: 	$Date: 1996/08/27 19:31:31 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Log: winmain.c,v $
* Revision 1.2  1996/08/27 19:31:31  koen
* Added keyboard interfacing and About dialog procedure.
*
* Revision 1.1  1996/08/18 18:35:12  koen
* Initial Revision
*
*****/
/* include this before anything else */
#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <ctl3d.h>

#ifdef WIN32
#include <bwcc.h>
#endif

#include "forutil.h"
#include "windows\lib\resize.h"		/* dialog resizing routines */
#include "windows\lib\winutil.h"
#include "windows\src\winfflow.h"	/* contains the flowInfo struct */
#include "windows\src\resource.h"

/**** Global Variables ****/
HINSTANCE hInst;			/* application instance handle */
HWND hWndMain;				/* main dialog window handle. */
int num_files = 0;			/* total no. of files selected */
char **file_list = NULL;		/* list of files */

/**** Explicitly Exported Functions ****/
long FAR PASCAL _export WndProc(HWND hwnd, UINT message,
	UINT wParam, LONG lParam);
BOOL FAR PASCAL _export	AboutProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam);
BOOL CALLBACK _export count_children(HWND hDlg, LPARAM lParam);
LRESULT CALLBACK _export KeyboardProc(int code, WPARAM wParam, LPARAM lParam);

/**** External declared variables ****/
extern int yylex_abort;			/* flex abort flag */

/**** Private Variables ****/
static HBRUSH background_brush;
static BOOL block_resize;		/* resize blocking flag */
static int file_sel_changed;		/* file selection has changed */
static int num_children, curr_child;
static HHOOK myhookproc;		/* keyboard hook procedure handle */

typedef struct {
	HWND hwnd;
	short active;
}window_admin;				/* list of child windows */

static window_admin *window_list;	/* list of controls with WS_TABSTOP */

/**** Private Functions ****/
static void CloseSetup(void);
static void get_fortran_files(HWND hDlg);
static void get_selected_file(HWND hDlg, int command_id);
static void destroy_filelist(void);
static void get_child_windows(HWND hParent, HINSTANCE hInst);
static void fill_file_listbox(HWND hDlg, char *files, int offset,
	char *initial_dir);
static int get_files_to_scan(HWND hDlg);

/*****
* WinMain for our application.
*****/
#pragma argsused
int PASCAL
WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine,
	int nCmdShow)
{
	static char szAppName [] = "winfflow" ;
	HWND hwnd ;
	MSG  msg;
	WNDCLASS wndclass ;
	HANDLE hAccel;

	/* Register exit function */
	atexit(CloseSetup);

	if (!hPrevInstance)
	{
		wndclass.style		= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wndclass.lpfnWndProc	= WndProc ;
		wndclass.cbClsExtra	= 0 ;
		wndclass.cbWndExtra	= DLGWINDOWEXTRA ;
		wndclass.hInstance	= hInstance ;
		wndclass.hIcon		= LoadIcon (hInstance,
					MAKEINTRESOURCE(WINFFLOW_ICON_1));
		wndclass.hCursor	= LoadCursor (NULL, IDC_ARROW) ;
		wndclass.hbrBackground  = NULL;
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName  = szAppName ;
		RegisterClass(&wndclass);
	}

	/* save instance handle for use by other functions */
	hInst = hInstance;

	/*
	* Accelerators needed to provide a keyboard interface to the
	* main window
	*/
	hAccel = LoadAccelerators(hInstance, szAppName);

	/* load settings from ini file */
	load_settings();

	/* initialise ctl3d */
	init3d(hInst);

	/*
	* block resize. Otherwise CreateDialog will call DoResize with
	* an empty window.
	*/
	block_resize = TRUE;
	/* Create our main window. */
	hwnd = CreateDialog (hInstance, szAppName, 0, NULL) ;

	/* Show it */
	ShowWindow (hwnd, nCmdShow) ;

	/*
	* add ctl3d effects to the main window. This will subclass the
	* childs controls only. Setting the background color is done below.
	*/
	subclass_main_app_window(hwnd, hInstance);

	/*
	* Now for some background color initialisation.
	* Since the hbrBackground field of our wndclass is zero, we need to
	* respond to any WM_ERASEBKGND messages. But when the window comes up
	* we do not have a background brush yet so our window will be 
	* transparant!
	* So, in order to get the correct background color (which should be 
	* the one * provided by ctl3d, we send a RedrawWindow twice.
	* The first one will trigger a WM_CTLCOLOR message for each child 
	* control in the window. The background brush used is stored in 
	* background_brush.
	* The WM_ERASEBKGND message does nothing in response to the first 
	* message. When we sent the second message, we have a background brush 
	* available so the background will be painted with the correct color.
	* We need the RDW_ALLCHILDREN flag again since we invalidate the entire
	* window region. Not doing this will cause all child controls not to be
	* repainted. 
	*/
	RedrawWindow(hwnd, NULL, NULL,
		RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | 
		RDW_INTERNALPAINT);
	RedrawWindow(hwnd, NULL, NULL,
		RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | 
		RDW_INTERNALPAINT);

	/* save window handle for later use */
	hWndMain = hwnd;

	/* make list of child windows needed for keyboard interface */
	get_child_windows(hwnd, hInst);

	/* Release resize blocking flag */
	block_resize = FALSE;
	/* Register with resize and force a display */
	RegisterResize(hwnd, hInst);
	DoResize(hwnd, TRUE);

	/* load any saved files */
	load_ini_file_data(hwnd);

	SetFocus(window_list[0].hwnd);	/* set initial focus */

	/* GetMessage returns 0 ONLY when WM_QUIT is passed */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if((output_dialog == 0 ||
			!IsDialogMessage(output_dialog, &msg)) &&
			(progress_dialog == 0 ||
			!IsDialogMessage(progress_dialog, &msg)))
		{
			if(!TranslateAccelerator(hwnd, hAccel, &msg))
			{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
			}
		}
	}
	return msg.wParam ;
}

/*****
* Clear the current list of files and destroy any associated
* flowgraph data with it.
*****/
static void
destroy_filelist(void)
{
	int i;
	for(i = 0 ; i < num_files; i++)
		GlobalFreePtr(file_list[i]);
	if(num_files)
		GlobalFreePtr(file_list);
	free_flowgraph(flow);
	flow = NULL;
	file_list == NULL;
	num_files = 0;
	file_sel_changed = 1;
}

/*****
* perform closeup actions
* Release memory occupied by the filelist
* unattach ctl3d support
*****/
static void
CloseSetup(void)
{
	/* release keyboard hook */
	UnhookWindowsHookEx(myhookproc);
	/* release memory occupied by window list */
	free(window_list);
	/* release memory */
	destroy_filelist();
	/* release ctl3d */
	free3d(hInst);
}

/*****
* Retrieve all items from the file listbox in the main window.
* This routine discards any preloaded list of files.
* As a consequence of that, it also discards any flowgraph generated from 
* that filelist.
*
* Returns 0 if there are no files else returns 1.
*****/
static int
get_files_to_scan(HWND hDlg)
{
	int i;
	char szBuf[MAXPATHLEN];
	HWND hwndList = GetDlgItem(hDlg, IDC_MAIN_LISTBOX);

	if(num_files != 0 && !file_sel_changed)
		return(1);
	/* clear previous list */
	destroy_filelist();

	/* get no of items in the list */
	num_files = (int)SendMessage(hwndList, LB_GETCOUNT, 0, 0L);

	/* return if no files in list */
	if(num_files == 0)
		return(0);

	/* allocate memory for this list */
	file_list = (char**)GlobalAllocPtr(GPTR, (num_files+1)*sizeof(char*));
	if(file_list == NULL)
	{
		ErrHandler(hDlg, R_ERROR, RB_OK1, "Out of memory: Failed to "
			"allocate %i bytes for filelist",
			num_files*sizeof(char*));
		exit(4);
	}
	/* get the items in the list */
	for(i = 0; i < num_files; i++)
	{
		SendMessage(hwndList, LB_GETTEXT, i, (LONG)(LPSTR)szBuf);
		file_list[i]=(char*)GlobalAllocPtr(GPTR,
			(strlen(szBuf)+1)*sizeof(char));
		if(file_list[i] == NULL)
		{
			ErrHandler(hDlg, R_ERROR, RB_OK1, "Out of memory: "
				"Failed to allocate %i bytes for file %i",
				(strlen(szBuf)+1)*sizeof(char), szBuf);
			exit(4);
		}
		strcpy(file_list[i], szBuf);
	}
	file_sel_changed = 0;
	return(1);
}

/*****
* fill the file listbox in the main window
* files is a list of files preceded with the directory name. Filenames
* are separated by spaces.
* offset indicates at which offset in ``files'' the filenames start.
*****/
static void
fill_file_listbox(HWND hDlg, char *files, int offset, char *initial_dir)
{
	int file_len;
	char file[NAME_MAX], full_name[MAXPATHLEN];
	char *chPtr, *curr_file_ptr;
	HWND hwnd_files;
	HDC hdc_files;
	TEXTMETRIC tm_files;

	/* needed to adjust width of horizontal scrollbar */
	hwnd_files = GetDlgItem(hDlg, IDC_MAIN_LISTBOX);
	hdc_files = GetDC(hwnd_files);
	GetTextMetrics(hdc_files, &tm_files);

	/* selection has changed */
	file_sel_changed = 1;

	/*
	* The filenames in files start at offset. Offset contains the index
	* in the files array where the files start
	*/
	curr_file_ptr = &files[offset];

	/* add all files */
	while(1)
	{
		/* get the name of a file */
		for(chPtr = curr_file_ptr, file_len = 0;
			*chPtr != '\0' && *chPtr != ' '; chPtr++, file_len++);

		/* sanity check */
		if(file_len == 0)
			break;

		/* copy the file name and null-terminate */
		strncpy(file, curr_file_ptr, file_len);
		file[file_len] = '\0';

		/* make it lower case */
		downcase(file);
		/* copy path + filename */
		sprintf(full_name, "%s\\%s", initial_dir, file);

		/* check if this file is already in the list, if so, skip it */
		if(SendMessage(hwnd_files, LB_FINDSTRINGEXACT, -1,
			(LPARAM)((LPSTR)full_name)) == LB_ERR)
		{
			/* adjust scrollbar width */
			adjust_lb_width(hwnd_files, hdc_files, tm_files, 
				full_name);

			/* add this filename to the listbox */
			SendMessage(hwnd_files, LB_ADDSTRING, 0, 
				(LPARAM)full_name);

			/* sanity check */
			if(chPtr == '\0')
				break;
		}
		/* adjust ptr for next file name in files array */
		curr_file_ptr = chPtr + 1;
	}
	ReleaseDC(hwnd_files, hdc_files);
	return;
}

/*****
* Display a file selection dialog box to allow the
* user to select files he wishes to scan
*****/
static void
get_fortran_files(HWND hDlg)
{
	OPENFILENAME ofn;
	char szDirName[128];
	static char szFile[1024];
	UINT  i, cbString;
	char  chReplace;	/* string separator for szFilter */
	char  szFilter[256];
	i = 0;

	/* clear szFile buffer */
	memset(szFile, '\0', 1024);

	/* initial directory appearing in the fileselectiondialog. */
	strcpy(szDirName, initial_dir);

	/*
	* Load string needed for the possible filters used by the fileselection
	* dialog.
	*/
	memset(szFilter, '\0', sizeof(szFilter));
	if ((cbString = LoadString(hInst, IDS_FILTERSTRING, szFilter,
		sizeof(szFilter))) == 0)
	{
		ErrHandler(hDlg, R_ERROR, RB_OK1, "Internal Error: failed to "
			"get resource");
		return;
	}
	/* retrieve filter wildcard */
	chReplace = szFilter[cbString - 1];
	for (i = 0; szFilter[i] != '\0'; i++)
	{
		if (szFilter[i] == chReplace)
		szFilter[i] = '\0';
	}

	/* Set all structure members to zero. */
	memset(&ofn, 0, sizeof(OPENFILENAME));
	/* Fill appropriate fields */
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	/* filter string */
	ofn.lpstrFilter = szFilter;
	/* default filter to use */
	ofn.nFilterIndex = 1;
	/* this string will contain selected files upon return */
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = sizeof(szFile);
	/* initial directory to use */
	ofn.lpstrInitialDir = szDirName;
	/* title for this fileselection dialog box */
	ofn.lpstrTitle = "Select Files to Scan";
	/* fileselection dialog configuration flags */
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT;

	/* display fileselection dialog box */
	if(GetOpenFileName(&ofn))
	{
		/* zero out initial_dir */
		memset(initial_dir, '\0', sizeof(initial_dir));
		/* save original dir.*/
		strncpy(initial_dir, ofn.lpstrFile, ofn.nFileOffset);
		/* cut off a trailing slash */
		if(initial_dir[strlen(initial_dir) - 1] == '\\')
			initial_dir[strlen(initial_dir) - 1] = '\0';
		/* remove trailing spaces, if any */
		stripspaces(initial_dir);
		/* make it lower case */
		downcase(initial_dir);
		/* file the file listbox in the main window */
		fill_file_listbox(hDlg, ofn.lpstrFile, ofn.nFileOffset,
			initial_dir);
	}
}

/****
* Retrieve the currently selected file and display it in the configured
* editor.
****/
static void
get_selected_file(HWND hDlg, int command_id)
{
	int index;
	char szBuf[MAXPATHLEN];

	/* get currently selected item */
	index = (int)SendDlgItemMessage(hDlg, IDC_MAIN_LISTBOX,
		LB_GETCURSEL, 0, 0L);
	if(index == LB_ERR && command_id == IDC_MAIN_EDIT)
	{
		ErrHandler(hDlg, R_WARNING, RB_OK1, "Nothing selected. Please "
			"select a file from the list");
		return;
	}

	/* empty listbox */
	if(index == LB_ERR)
		return;

	/* get name of selected file */
	SendDlgItemMessage(hDlg, IDC_MAIN_LISTBOX, LB_GETTEXT, index, 
		(LONG)(LPSTR)szBuf);
	switch(command_id)
	{
		case IDC_MAIN_EDIT:
		case IDC_MAIN_LISTBOX:
			edit_file(szBuf, 0);
			break;
		case IDC_MAIN_REMOVE:
			file_sel_changed = 1;
			SendDlgItemMessage(hDlg, IDC_MAIN_LISTBOX,
				LB_DELETESTRING, index, 0L);
	}
}

/*****
* Main window dialog box procedure
*****/
long FAR PASCAL _export
WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HBRUSH hbrush;

	switch (message)
	{
		case WM_CREATE:
			file_sel_changed = 0;
			/* attach keyboard interface */
			if((myhookproc = SetWindowsHookEx(WH_KEYBOARD,
				(HOOKPROC)KeyboardProc, hInst,
				GetCurrentTask())) == NULL)
				ErrHandler(NULL, R_ERROR, RB_OK1, "Winfflow "
					"Error: failed to set keyboard hook.\n"
					"Keyboard interface not installed.");
			return FALSE;
		/* following four cases ctl3d specific */
		case WM_SYSCOLORCHANGE:
			Ctl3dColorChange();
			return FALSE;
		case WM_SETTEXT:
		case WM_NCPAINT:
		case WM_NCACTIVATE:
			SetWindowLong(hwnd, DWL_MSGRESULT,
				Ctl3dDlgFramePaint(hwnd, message, wParam, 
				lParam));
			return TRUE;
		/*
		* paint window background with a brush provided by the response
		* to the WM_CTLCOLOR message
		*/
		case WM_ERASEBKGND:
		{
			RECT lprc;
			HDC hdc = (HDC)wParam;

			if(hwnd != hWndMain)
				return TRUE;
			if(!background_brush)
				return TRUE;
			UnrealizeObject(background_brush);
			GetClientRect(hwnd, &lprc);
			SetBrushOrg(hdc, 0, 0);
			SelectObject(hdc, background_brush);
			FillRect(hdc, &lprc, background_brush);
			return TRUE;
		}
		case WM_SIZE:
			switch(wParam)
			{
				case SIZE_MAXIMIZED:
				case SIZE_RESTORED:
					if(!block_resize)
						DoResize(hwnd, 0);
					return FALSE;
				default:
					break;
			}
			break;
		case WM_COMMAND:
		switch(GET_COMMAND_ID)
		{
			case IDC_MAIN_ABOUT:
				DisplayDialog(hwnd, hInst,WINFFLOW_ABOUT_DIALOG,
					AboutProc);
				break;
			/* add files to the listbox */
			case IDC_MAIN_ADD:
				get_fortran_files(hwnd);
				break;

			/* double click will edit selected file */
			case IDC_MAIN_LISTBOX:
				if(GET_COMMAND_CMD == LBN_DBLCLK)
					get_selected_file(hwnd, 
						IDC_MAIN_LISTBOX);
				break;

			/* remove/edit selected file */
			case IDC_MAIN_EDIT:
			case IDC_MAIN_REMOVE:
				get_selected_file(hwnd, GET_COMMAND_ID);
				break;
			case IDC_MAIN_REMOVE_ALL:
				SendDlgItemMessage(hwnd, IDC_MAIN_LISTBOX, 
					LB_RESETCONTENT, 0, 0L);
				destroy_filelist();
				break;
			/* settings */
			case IDC_MAIN_SETTINGS:
				DisplayDialog(hwnd, hInst,
					WINFFLOW_SETTINGS_DIALOG,
					(FARPROC)SettingsProc);
				break;

			/* scan the files */
			case IDOK:
				/*
				* scan the files when the selection has changed,
				* there is no current flowgraph or when the 
				* previous scan has been aborted
				*/
				if(file_sel_changed || flow == NULL ||
					yylex_abort)
				{
					/* pick up files */
					if(!get_files_to_scan(hwnd))
						break;
					generate_flowgraph();
				}
				/* hide main window */
				ShowWindow(hWndMain, SW_HIDE);
				/* display selector dialog box */
				DisplayDialog(hwnd, hInst,
					WINFFLOW_OUTPUTSELECTOR_DIALOG,
					(FARPROC)SelectorProc);
				/* display main window again */
				ShowWindow(hWndMain, SW_RESTORE);
				break;
			case IDCANCEL:
				/* save the filelist */
				save_ini_file_data(hwnd);
				SendMessage(hWndMain, WM_CLOSE, 0, 0L);
				break;
			case IDHELP:
				WinHelp(hWndMain, "winfflow.hlp", 
					HELP_CONTEXT, WINFFLOW_MAIN_DIALOG);
				break;
		} return(FALSE);
		case WM_SYSCOMMAND:
			if(GET_COMMAND_ID == SC_CLOSE)	/* Get the close item */
			{
				SendMessage(hWndMain, WM_CLOSE, 0, 0L);
				return FALSE;
			}
			break;
		case WM_CLOSE:
			UnregisterResize(hwnd);
			DestroyWindow(hwnd);
			return FALSE;
		case WM_DESTROY:
			PostQuitMessage (0) ;
			return FALSE;
/* Ctl3d specific code */
#ifdef WIN32
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
#else
		case WM_CTLCOLOR:
#endif
			hbrush = Ctl3dCtlColorEx(message, wParam, lParam);
			if (hbrush != (HBRUSH) FALSE)
			{
				/* save this brush */
				background_brush = hbrush;
				return hbrush;
			}
			/* fall through */
		default:
			break;
	 }
	 return (DefWindowProc (hwnd, message, wParam, lParam));
}

/*****
* This routine is called by EnumChildWindows in the get_child_windows to
* build a list of windows with the WS_TABSTOP style
* This routine is called twice: once with lParam == 0 to count the number
* of windows and once again to add all windows to a list.
*****/
BOOL CALLBACK _export 
count_children(HWND hDlg, LPARAM lParam)
{
	long class_style;

	class_style = GetWindowLong(hDlg, GWL_STYLE);
	if(class_style & WS_TABSTOP)
	{
		if(lParam)
		{
			/* save window handles */
			window_list[num_children].hwnd = hDlg;
			window_list[num_children].active = 0;
		}
		num_children++;
	}
	return(TRUE);
}

/*****
* This routine will make a list of all childwindows of the hParent
* window.
*****/
static void
get_child_windows(HWND hParent, HINSTANCE hInst)
{
	WNDENUMPROC enumProc;

	/* Create procedure address for the enumeration routine. */
	enumProc = (WNDENUMPROC)MakeProcInstance((FARPROC)count_children,
		hInst);
	/* count no of windows with WS_TABSTOP defined */
	num_children = 0;
	EnumChildWindows(hParent, enumProc, 0L);
	/* allocate memory for it */
	window_list = (window_admin*)malloc(num_children*sizeof(window_admin));
	num_children = 0;
	/* build window list */
	EnumChildWindows(hParent, enumProc, 1L);

	FreeProcInstance((FARPROC)enumProc);
}

/*****
* Keyboard interface handler. This routine provides the <TAB> and
* <SHIFT>+<TAB> key sequence to tab between windows that are tabbable.
* This is a critical piece of code since it will be called for every
* key that is hit by the user.
*****/
LRESULT CALLBACK _export KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	/* 
	* only enter when the tab key has been hit _and_ the key is back
	* up again
	*/
	if(0x80000000L & lParam)
	{
		switch(GET_COMMAND_ID)
		{
			case VK_TAB:
				/*
				* Only walk to the next/previous window if the current
				* window is the window we think it is. If that is not
				* the case, another window had the focus and we restore
				* the focus to where we think it should be. Would give
				* nasty results otherwise.
				*/
				if(GetFocus() == window_list[curr_child].hwnd)
				{
					/* walk back when shift is pressed */
					if(GetKeyState(VK_SHIFT) < 0)
						curr_child = (--curr_child == -1 ?
							num_children-1 : curr_child);
					else
						curr_child = (++curr_child == num_children ?
							0 : curr_child);
				}
				/* Set to focus to this window */
				SetFocus(window_list[curr_child].hwnd);
				break;
			case VK_SPACE:
				if(GetFocus() == GetDlgItem(hWndMain, IDC_MAIN_LISTBOX))
					PostMessage(hWndMain, WM_COMMAND, IDC_MAIN_EDIT, 0L);
				SetFocus(window_list[curr_child].hwnd);
				break;
		}
	}
	/*
	* When code < 0, Windows wants us to call the next item in the
	* keyboard chain.
	*/
	return((code<0 ? CallNextHookEx(myhookproc, code, wParam, lParam):0));
}

/*****
* About dialog box procedure.
*****/
#pragma argsused
BOOL FAR PASCAL _export
AboutProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
	switch(message)
	{
		case WM_COMMAND:
			switch(GET_COMMAND_ID)
			{
				case IDOK:
					EndDialog(hDlg, 0);
					return(TRUE);
			}
	}
	return(FALSE);
}

