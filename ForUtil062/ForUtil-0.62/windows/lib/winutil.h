/*****
* winutil.h: winutil library public header file  
*
* Hicks Version 	NA
* This file Version	$Revision: 1.7 $
*
* Creation date:	16-08-1995
* Last modification: 	$Date: 1996/08/27 19:26:19 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*
* Based on winutil.lib for Hicks.
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/winutil/RCS/winutil.h,v 1.7 1996/08/27 19:26:19 koen Exp koen $
*****/
/*****
* $Log: winutil.h,v $
* Revision 1.7  1996/08/27 19:26:19  koen
* Added adjust_lb_width macro and changed proto's for 3d stuff to remove 
* dependency on externally declared HINSTANCE hInst.
*
*****/

#ifndef __winutil_h_
#define __winutil_h_

/* standard needed includes */
#include <stdio.h>
#include <windows.h>

/*****
* These macros will always yield the proper result whether we are compiling
* for win16 or win32. It's important to notice that the functions that
* use these macros _must_ define wParam and lParam in their function
* headers!
*****/
#ifdef WIN32
#define GET_COMMAND_ID		LOWORD(wParam)
#define GET_COMMAND_CMD		HIWORD(wParam)
#define GET_COMMAND_HWND	lParam
#else /* win16 */
#define GET_COMMAND_ID		wParam
#define GET_COMMAND_CMD		HIWORD(lParam)
#define GET_COMMAND_HWND	LOWORD(lParam)
#endif /* WIN32 */

#ifndef IDHELP
#define IDHELP 998
#endif /* IDHELP */

#define MI(ID) 		MAKEINTRESOURCE(ID)
#define SET_PROC_ADDRESS		1
#define RELEASE_PROC_ADDRESS	0

/*****
* Function Prototypes from winutil.c
*****/
/* initialise and release ctl3d lib support */
extern int  init3d(HINSTANCE hInst);
extern void free3d(HINSTANCE hInst);

/* add ctl3d effects to the main window of an application */
extern BOOL CALLBACK add_ctl3d_effects(HWND hwnd, LPARAM lParam);
extern void subclass_main_app_window(HWND hParent, HINSTANCE hInst);

/* display a modal dialog */
extern int DisplayDialog(HWND hDlg, HINSTANCE hInst, int DialogId, 
	FARPROC DialogProc);

/* retrieve the path of the executable hInst and return it in szFileName */
extern void GetExePath(HINSTANCE hInst, char* szFileName);

/* return to id of the checked radio button between idFirst and idLast */
extern int IsRadioButtonChecked(HWND hDlg, int idFirst, int idLast);

/*****
* Function Prototypes from file.c
*****/
/* open a file */
extern FILE *open_file(char *filename);

/* create a file */
extern FILE *create_file(char *filename);

/* display a file save as fileselection dialog. */
extern char *display_save_dialog(HWND hDlg, char *initial_name, char *filter);

/* display a file open fileselection dialog. */
extern char *display_open_dialog(HWND hDlg, char *initial_name, char *filter);

/*****
* Function Prototypes from nerror.c
*****/
/* Supported dialog types */
#define R_WARNING	0	/* warning dialog */
#define R_ERROR		1	/* error dialog */
#define R_QUESTION	2	/* question dialog */
#define R_MESSAGE	3	/* message dialog */

/* supported button types */
#define RB_OK1		0	/* OK */
#define RB_OK2		1	/* OK, CANCEL */
#define RB_YES2		2	/* YES, NO */
#define RB_YES3		3	/* YES, NO, CANCEL */
#define RB_RETRY2	4	/* ABORT, RETRY */
#define RB_RETRY3	5	/* ABORT, RETRY, IGNORE */

/***** 
* Display an error dialog.
* Invoke ErrHandler as:
* ErrHandler(hwnd, <dialog type>, <buttons>, <message>, <variable message args>)
* HWND can be NULL.
*****/
extern int ErrHandler(HWND, int, int, char*, ...);

/***** 
* adjust the width of horizontal listbox scrollbar 
* hwndLB is the window handle to a listbox
* hdcLB is the handle to a HDC of a listbox
* tm is a handle to a TEXTMETRIX of a listbox
* szBuf is the text to add to a listbox
*****/
#define adjust_lb_width(hwndLB, hdcLB, tm, szBuf) \
{ \
	WORD __lb_width; DWORD __lb_data; \
	__lb_width = (WORD)SendMessage(hwndLB, LB_GETHORIZONTALEXTENT, 0, 0L); \
	__lb_data = GetTextExtent(hdcLB, (LPSTR) szBuf, \
		(WORD)(strlen(szBuf)*1.25)) + tm.tmAveCharWidth; \
	if ((WORD)(LOWORD(__lb_data)) > __lb_width) \
		SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, \
			LOWORD(__lb_data),0L); \
}

/* Don't add anything after this endif! */
#endif /* __winutil_h_ */
