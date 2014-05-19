static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winutil/RCS/winutil.c,v 1.6 1996/08/27 19:25:20 koen Exp koen $";
/*****
* winutil.c: Standard windows utilities.  
*
* Common Version 	1.00
* This file Version	$Revision: 1.6 $
*
* Creation date:	01-05-1995
* Last modification: 	$Date: 1996/08/27 19:25:20 $
*
* Author:		Koen
* (C)1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: winutil.c,v $
* Revision 1.6  1996/08/27 19:25:20  koen
* Removed dependency on external declared hInst.
*
* Revision 1.5  1996/05/23  03:20:53  koen
* Added the IsRadioButtonChecked routine.
* correct a small bug in DisplayDialog: if this returns FALSE, the
* parent window will be placed on top of the Z-order.
*
* Revision 1.4  1996/05/10  08:09:53  koen
* Changed some comments.
*
* Revision 1.3  1995/11/29  06:33:11  koen
* Checked for win32 portability.
*
* Revision 1.2  1995/10/17  22:58:15  koen
* removed CLOSEPROC and exit calls upon failure. Made h3d an internal variable.
* Moved the GetExePath procedure from getstr.c in here
*
* Revision 1.1  1995/08/18  19:56:20  koen
* Initial revision
*
*****/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <ctl3d.h>

#include "windows\lib\winutil.h"

/*****
* Local declarations
*****/
typedef BOOL (WINAPI *ctl3dproc)(HANDLE);
static HINSTANCE h3d;	/* handle to ctl3dv2.dll */

/*****
* Modal dialogbox display driver routine 
*****/ 
int DisplayDialog(HWND hDlg, HINSTANCE hInst, int DialogId, FARPROC DialogProc)
{
	FARPROC lpfnTmpDlgProc;
	int result;

	lpfnTmpDlgProc=MakeProcInstance(DialogProc, hInst);
	result = DialogBox(hInst, MI(DialogId), hDlg, (DLGPROC)lpfnTmpDlgProc);
	FreeProcInstance(lpfnTmpDlgProc);
	return(result);
}

/*****
* ctl3d Init routine.
*****/
int init3d(HINSTANCE hInst)
{
	#ifdef WIN32
	static char f[] = "ctl3d32.dll";
	#else
	static char f[] = "ctl3dv2.dll";
	#endif

	ctl3dproc p;

	if(h3d)
		return 0;
	if ((h3d = LoadLibrary(f)) == 0)
		return 0;
	if ((p = (ctl3dproc)GetProcAddress(h3d, "Ctl3dRegister")) == 0)
		return 0;
	if (!(*p)(hInst))
		return 0;
	if ((p = (ctl3dproc)GetProcAddress(h3d, "Ctl3dAutoSubclass")) == 0)
		return 0;
	if (!(*p)(hInst))
		return 0;
	return 1;
}

/*****
* ctl3d Exit routine.
*****/
void free3d(HINSTANCE hInst)
{
	ctl3dproc p;

	if (h3d == 0)
		return;
	if ((p = (ctl3dproc)GetProcAddress(h3d, "Ctl3dUnregister")) == 0)
		return;
	(*p)(hInst);

	FreeLibrary(h3d);
}

/*****
* Retrieve the full path name for the given HINSTANCE
*****/
void GetExePath(HINSTANCE hInst, char* szFileName)
{
	char *  pcFileName;
	int     nFileNameLen;
	nFileNameLen = GetModuleFileName(hInst, szFileName, 256);
	pcFileName = szFileName + nFileNameLen;
	while (pcFileName > szFileName)
	{
		if (*pcFileName == '\\' || *pcFileName == ':')
		{
			*(++pcFileName) = '\0';
			break;
		}
		nFileNameLen--;
		pcFileName--;
	}
	return;
}

/*****
* Procedure which checks what checkbutton is activated in a row of 
* checkbuttons.
*****/
int
IsRadioButtonChecked(HWND hDlg, int idFirst, int idLast)
{
	register int i;
	for(i = idFirst; i <= idLast && IsDlgButtonChecked(hDlg,i) <= 0; i++);
	return(i);
}

/*****
* These two routines fix the ctl3d look for a main window made of a dialog.
* Since ctl3d only subclasses dialogs automatically, we must subclass
* each control in our main _WINDOW_ manually to achieve a 3D-look.
*
* subclass_main_app_window will perform an enumeration of all
* controls in the main window. This enumeration calls add_ctl3d_effects
* with the window handle of a control. add_ctl3d_effects then registers
* the given window (=control) with ctl3d.
*****/
#pragma argsused
BOOL CALLBACK
add_ctl3d_effects(HWND hwnd, LPARAM lParam)
{
	Ctl3dSubclassCtl(hwnd);
	return(TRUE);
}

void
subclass_main_app_window(HWND hParent, HINSTANCE hInst)
{
	WNDENUMPROC enumProc;

	enumProc = (WNDENUMPROC)MakeProcInstance((FARPROC)add_ctl3d_effects, 
		hInst);
	EnumChildWindows(hParent, enumProc, 0l);
	FreeProcInstance((FARPROC)enumProc);
}
