%{
#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winfflow/RCS/winfflow.l,v 1.8 1996/08/27 19:32:25 koen Exp koen $";
#endif
/*****
* winfflow.l : lexical flowgraph routines for Fortran77, MS Windows version
*
* This file Version	$Revision: 1.8 $
*
* Creation date:	Mon Feb  5 02:26:23 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:32:25 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*
* Heavily modified version of fflow.l
*****/
/*****
* ChangeLog
* $Log: winfflow.l,v $
* Revision 1.8  1996/08/27 19:32:25  koen
* Updated scanner routines. Added a graphical display of scanning progress.
*
*****/
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <windowsx.h>

#include "forutil.h"
#include "windows\lib\winutil.h"
#include "windows\src\winfflow.h"
#include "windows\src\resource.h"

#define DONOTHING	0
#define ADDFUNC		1
#define ADDCALL		2

/* Bookkeeping data for flowgraph data */
typedef struct _global_flow_data{
	flowInfo *head;
	flowInfo *current;
	flowInfo *call_head;
	flowInfo *current_call;
}global_flow_data;

/**** Explicitly Exported Functions ****/
BOOL CALLBACK _export ProgressProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam);

/**** Global Variables ****/
HWND progress_dialog;			/* scan progress window handle */

/**** Private Variables ****/
static global_flow_data global_flow;
static int action, Type;
static char curr_path[512], curr_file[128];
flowInfo *flow;				/* flowgraph list */
DLGPROC old_draw;			/* original drawing procedure */
static HWND Ruler;			/* The progress indicator */
static HBRUSH red_brush;	/* red brush for progress dialog */
static int num_calls;
static unsigned long box_length, curr_line_num;

/**** Private Functions ****/
static void add_new_func(char *name, int callline, char *path, char *file);
static void add_new_call(char *name, int callline);

inline void getParent(flowInfo *list, flowInfo *call);
inline flowInfo *new_entry(char *name, int callline, char *path, char *file);

static void display_progress_box(int state);

flowInfo *sortAll(flowInfo *list);

/* When memory allocation fails, display an error message and exit */
#define MEMERROR(len) \
	{ \
		ErrHandler(hWndMain, R_ERROR, RB_OK1, "Fatal Error: " \
			"GlobalAlloc failed for %i bytes", (int)(len));\
		exit(2);\
	}
%}

routine  ^([ \t]{6,})subroutine[ \t]
program  ^([ \t]{6,})program[ \t]
function ^([ \t]{6,})[ [:alpha:]\*[:digit:]]*function[ \t]
call	 [^('][ \t]{1,}call[ ]*
name [^\(\)'\n ]*[a-zA-Z0-9_]+
nl \n

%option caseless yylineno
%x FNAME

%%
^[*cC].* ; /* eat up comments entirely */
{program}	{
			action = ADDFUNC;
			Type   = PROGRAM;
			BEGIN FNAME;
		}
{function}	{
			action = ADDFUNC;
			Type   = FUNCTION;
			BEGIN FNAME;
		}
{routine}	{
			action = ADDFUNC;
			Type   = SUBROUTINE;
			BEGIN FNAME;
		}
{call}		{
			action = ADDCALL;
			Type   = UNKNOWN;
			BEGIN FNAME;
		}
<FNAME>{name}?	{
		/*
		* Depending on the action taken,
		* add a new function to the flowgraph list or
		* add an entry for a call made in the current
		* function
		*/
			InvalidateRect(Ruler, NULL, FALSE);
			if(action == ADDFUNC)
				add_new_func(yytext, yylineno, curr_path,
					curr_file);
			else
			{
				num_calls++;
				add_new_call(yytext, yylineno);
			}
			BEGIN 0;
		}
{nl}	; /* ignore newlines */	
.	 	; /* nothing for all other remaining chars */
%%

/*****
* Draw the ruler which displays how far the scanner is.
*****/
static void DrawPercentage(HWND hDraw, unsigned long percent)
{
	HDC hdc;
	HBRUSH hBrush;
	PAINTSTRUCT ps;
	RECT rc;
	int len;

	hdc = BeginPaint(hDraw, &ps);
	GetClientRect(hDraw, &rc);	/* max. size of area to paint */
	SetMapMode(hdc, MM_TEXT);
	len = rc.left;
	if(box_length != 0)
	{
		/* compute length of area to paint red */
		len = rc.left + (int)(rc.right * ((float)percent/(float)box_length));
		hBrush = SelectObject(hdc, red_brush);
		Rectangle(hdc, rc.left, rc.top, len, rc.bottom);
		SelectObject(hdc, hBrush);
	}
	else
	{
		HBRUSH white_brush;
		/* draw a white initial rectangle */
		white_brush= CreateSolidBrush(RGB(255,255,255));
		hBrush = SelectObject(hdc, white_brush);
		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, hBrush);
		DeleteObject(white_brush);
	}
	EndPaint(hDraw, &ps);
}

/*****
* Drawing procedure for the progress indicator
****/
BOOL CALLBACK _export DrawProc(HWND hDraw, UINT message, UINT wParam,
	LONG lParam)
{
	switch(message)
	{
		case WM_PAINT:
			DrawPercentage(hDraw, curr_line_num + yylineno);
			break;
	}
	return ((int)(CallWindowProc(old_draw, hDraw, message, wParam, lParam)));
}

/*****
* Window procedure for the progress dialog box
*****/
#pragma argsused
BOOL CALLBACK _export ProgressProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam)
{
	WNDPROC new_draw;
	switch(message)
	{
		case WM_INITDIALOG:
			/* create a red brush for progress bar painting */
			red_brush  = CreateSolidBrush(RGB(255,0,0));
			/* Initial strings */
			SetDlgItemText(hDlg, IDC_GENERATING1, "");
			SetDlgItemText(hDlg, IDC_GENERATING2, "");
			Ruler = GetDlgItem(hDlg, IDC_GENERATING2);
			/* use subclassing so we can paint the progress bar */
			new_draw = (WNDPROC)MakeProcInstance((FARPROC)DrawProc,
				hInst);
			old_draw = (DLGPROC)GetWindowLong(Ruler, GWL_WNDPROC);
			SetWindowLong(Ruler, GWL_WNDPROC, (LONG)new_draw);
			return TRUE;
		case WM_CLOSE:
			DeleteObject(red_brush);
			DestroyWindow(hDlg);
			progress_dialog = NULL;
			return TRUE;
		case WM_COMMAND:
			/* abort when the stop button is pressed */
			if(GET_COMMAND_ID == IDCANCEL)
			{
				yylex_abort = TRUE;
				return TRUE;
			}
			break;
	}
	return FALSE;
}

/*****
* display the progress dialogbox
* state is 1 for creation, 0 for destruction
*****/
static void
display_progress_box(int state)
{
	static DLGPROC lpfnProgressProc;

	if(state == SET_PROC_ADDRESS)
	{
		/* initialise the progress dialog box */
		lpfnProgressProc=(DLGPROC)MakeProcInstance((FARPROC) ProgressProc,
			hInst);
		progress_dialog = CreateDialog(hInst,
			MAKEINTRESOURCE(WINFFLOW_GENERATING_DIALOG), hWndMain,
			(DLGPROC)lpfnProgressProc);
	}
	else
		if(state == RELEASE_PROC_ADDRESS && lpfnProgressProc)
			FreeProcInstance(lpfnProgressProc);
}


/****
* Free the memory occupied by the given flowInfo list
****/
void
free_flowgraph(flowInfo *list)
{
	flowInfo *routines_left, *calls_left, *calls;

	while( list != NULL)
	{
		routines_left = list->next;
		calls = list->calls;
		while( calls != NULL)
		{
			calls_left = calls->next;
			GlobalFreePtr(calls->name);
			GlobalUnlock(calls->hglb);
			GlobalFree(calls->hglb);
			calls = calls_left;
		}
		GlobalFreePtr(list->name);
//		GlobalFreePtr(list->file);
//		GlobalFreePtr(list->path);
		GlobalUnlock(list->hglb);
		GlobalFree(list->hglb);
		list = routines_left;
	}
	list = NULL;
}

/*****
* Create a new flowgraph entry
* Args:
* name: name of (program,subroutine,function)
* callline: line where a call was made -or- where name was defined
* path: pathname of file
* file: name of file
*****/
static flowInfo
*new_entry(char *name, int callline, char *path, char *file)
{
	static flowInfo *list = NULL;
	HGLOBAL hglb = 0;

	hglb = GlobalAlloc(GPTR, sizeof(flowInfo));
	list = (flowInfo*)GlobalLock(hglb);
	if(list == NULL)
		MEMERROR(sizeof(flowInfo));
	list->hglb = hglb;
	/*
	* Due to our setup which attaches calls to a particular routine in a
	* given file, calls do not have a file nor a path associated with them,
	* only programs, subroutines and functions have.
	*/
	if(action != ADDCALL)
	{
		int alloc_len = (strlen(name)+strlen(file)+strlen(path)+4)*sizeof(char);
		/* allocate 1 block for routine name, path and file */
		list->name = GlobalAllocPtr(GPTR, alloc_len);
		if(list->name == NULL)
			MEMERROR(alloc_len);
		strcpy(list->name,name);
		list->file = list->name + (strlen(name)+2)*sizeof(char);
		strcpy(list->file, file);
		list->path = list->file + (strlen(file)+2)*sizeof(char);
		strcpy(list->path, path);
	}
	else
	{
		list->name = GlobalAllocPtr(GPTR, (strlen(name)+1)*sizeof(char));
		if(list->name == NULL)
			MEMERROR((strlen(name)+1)*sizeof(char));
		strcpy(list->name,name);
	}
	list->Type = Type;
	list->defline = callline;
	list->recursive = 0;
	list->called = 0;
	list->next = NULL;
	list->calls = NULL;
	list->parent = NULL;
	return(list);
}

static void
add_new_func(char *name, int callline, char *path, char *file)
{
	static flowInfo *new_func;

	new_func = new_entry(name, callline, path, file);

	/* Data initialisation */
	if(global_flow.head == NULL)
	{
		global_flow.head = new_func;
		global_flow.current = new_func;
		global_flow.call_head = NULL;
		global_flow.current_call = NULL;
	}
	else
	{
		global_flow.current->next = new_func;
		global_flow.current = new_func;
		global_flow.call_head = NULL;
		global_flow.current_call = NULL;
	}
}

static void
add_new_call(char *name, int callline)
{
	static flowInfo *new_call;

	/* user forgot a program statement. Add one for him. */
	if(!global_flow.head)
	{
		char prog_name[NAME_MAX];

		memset(prog_name, '\0', NAME_MAX);
		action = ADDFUNC;
		Type   = PROGRAM;
		/* program name by default filename part */
		if(strstr(curr_file, "."))
			strncpy(prog_name, curr_file, strstr(curr_file, ".") - curr_file);
		else
			strncpy(prog_name, curr_file, NAME_MAX);
		/* add it */
		add_new_func(prog_name, 1, curr_path, curr_file, 0);
		/* reset flags */
		action = ADDCALL; 
		Type   = UNKNOWN;
	}

	new_call = new_entry(name, callline, NULL, NULL);

	/* for data initialisation */
	if(global_flow.call_head == NULL)
	{
		global_flow.call_head = new_call;
		global_flow.current_call = new_call;
		global_flow.current->calls = new_call;
	}
	else
	{
		global_flow.current_call->next = new_call;
		global_flow.current_call = new_call;
	}
}

/*****
* Find in which file a function call is defined. If a routine is
* found which corresponds with the name of the call, the parent field
* of the flowInfo for the current call is set to the routine found.
*
* Very expensive (due strcmpi), but I don't see any other way...
*****/
static void
getParent(flowInfo *list, flowInfo *call)
{
	flowInfo *tmp;
	for(tmp = list; tmp != NULL; tmp = tmp->next)
	{
		/* match only if names and length are equal */
		if(!(strcmpi(tmp->name, call->name)))
			break;
	}
	if(tmp == NULL)
		return;
	call->parent = tmp;
	call->Type = KNOWN;
	/* This one has been used, mark it */
	tmp->called++;
}

/*****
* Figure out where the calls made in each function are defined
*****/
static flowInfo
*sortAll(flowInfo *list)
{
	flowInfo *tmp, *tmp1;

	for(tmp = list; tmp != NULL; tmp = tmp->next)
	{
		for(tmp1 = tmp->calls; tmp1 != NULL; tmp1 = tmp1->next)
		{
			getParent(list, tmp1);
			/*
			* sanity check: if the parent of the current routine
			* or function is the routine/function itself, we are
			* recursing on ourselves..
			*/
			if(tmp1->parent == tmp)
			{
				tmp->recursive = 1;
				ErrHandler(hWndMain, R_ERROR, RB_OK1,
					"ERROR: function "
					"%s recurses into itself\n"
					"line %i in file %s%s.\n", tmp->name,
					tmp1->defline, tmp->path, tmp->file);
				return(list);
			}
		}
	}
	return(list);
}

/*****
* Count the number of lines in file.
* Needed to set the correct length in the scan progress dialog
*****/
unsigned long
count_lines(FILE *file)
{
	register unsigned long line_num = 0;
	char input[128];
	while((fgets(input, 128,file)))
		line_num++;
	return(line_num);
}

/*****
* main for fflow
* No need to check if we have to scan files (num_files == 0) since
* that is the responsibility of the caller.
*****/
void
generate_flowgraph(void)
{
	int i;
	FILE *file;
	static char msg_buf[128];
	HCURSOR cursor_save;

	/* set global flow to NULL. */
	global_flow.head = NULL;
	global_flow.current = NULL;
	global_flow.call_head = NULL;
	global_flow.current_call = NULL;

	/* display progress dialog */
	box_length = 0;
	curr_line_num = 0;
	display_progress_box(SET_PROC_ADDRESS);
	InvalidateRect(Ruler, NULL, FALSE);
	SetDlgItemText(progress_dialog,IDC_GENERATING1,"Initializing Scanner");

	/* display hourglass cursor while initializing the scanner */
	/* figure out max. length of progress bar */
	box_length = 0;
	cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT));
	for(i = 0; i < num_files; i++)
	{
		if((file = fopen(file_list[i], "r"))!=NULL)
		{
			box_length += count_lines(file);
            fclose(file);
		}
	}
	SetCursor(cursor_save);

	/* unmatched output to /dev/null */
	yyout = fopen(YYOUT_DEVICE, "w");

	/* scan the file list */
	for(i = 0; i < num_files; i++)
	{
		file = fopen(file_list[i], "r");
		if(!file)
		{
			ErrHandler(hWndMain, R_WARNING, RB_OK1, "Error opening "
				"file %s: %s", file_list[i], ErrorString());
			continue;
		}
		/* split filename into a file and path component */
		ParseFilename(file_list[i], curr_file, curr_path);
		yyin = file;

		sprintf(msg_buf, "%s", curr_file);
		SetDlgItemText(progress_dialog, IDC_GENERATING1, msg_buf);

		action = DONOTHING;
		yylineno = 1;
		/* scan the file */
		yylex();
		/*
		* flush the buffer of flex so we won't have
		* any trailing stuff from the previous file
		*/
		YY_FLUSH_BUFFER;
		fclose(yyin);
		/* check if user aborted the scanner */
		if(yylex_abort)
		{
			if((ErrHandler(hWndMain, R_QUESTION, RB_YES2, "Do you "
				"wish to abort the scan process?")) == IDYES)
				break;
		}
		curr_line_num += yylineno;
	}
	/* close output device */
	fclose(yyout);

	SetDlgItemText(progress_dialog, IDC_GENERATING1, "Sorting...");

	flow = global_flow.head;

	flow = sortAll(flow);

	/* close progress dialog */
	SendMessage(progress_dialog, WM_CLOSE, 0, 0l);
	display_progress_box(RELEASE_PROC_ADDRESS);
}
