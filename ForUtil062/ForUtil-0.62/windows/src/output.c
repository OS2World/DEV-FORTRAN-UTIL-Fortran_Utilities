static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winfflow/RCS/output.c,v 1.2 1996/08/27 19:30:54 koen Exp koen $";
/*****
* output.c: Window procedures for the output dialog. MS-Windows version
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	08-08-1996
* Last modification: 	$Date: 1996/08/27 19:30:54 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Log: output.c,v $
* Revision 1.2  1996/08/27 19:30:54  koen
* Added support for routine references, dynamic updating, flowgraph 
* generation abort procedure.
*
* Revision 1.1  1996/08/18 18:34:16  koen
* Initial Revision
*
*****/
#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#include <windows.h>

#include "forutil.h"
#include "windows\lib\resize.h"
#include "windows\lib\winutil.h"
#include "windows\src\resource.h"
#include "windows\src\winfflow.h"	/* contains the flowInfo struct */

/**** Global Variables ****/
HWND output_dialog;		/* flowgraph window handle */
BOOL collapse_consecutive;	/* collapse calls to the same routine */
BOOL hide_unresolved;		/* don't show unresolved calls */
BOOL hide_resolved;		/* don't show resolved calls */

/**** Explicitly exported functions ****/
BOOL CALLBACK _export output_count_children(HWND hDlg, LPARAM lParam);
LRESULT CALLBACK _export OutputKeyboardProc(int code, WPARAM wParam, 
	LPARAM lParam);

/**** Private Variables ****/
static HDC hdcLB;		/* HDC for output listbox */
static TEXTMETRIC tm;		/* current font data */
static HWND hwndLB;		/* output listbox window handle */

static unsigned long num_calls_known;	/* current no. of resolved calls */
static unsigned long num_calls_unknown;	/* current no. of unresolved calls */
static char *current_func;		/* current routine */
static FILE *save_file;			/* handle to save_file */
static BOOL save_to_file;		/* True when saving to file */
static BOOL flow_abort;			/* flowgraph generation abort flag */
static jmp_buf jumper;			/* jumpstate for listbox overflow */

/* See winmain.c for information about the things below */
static int num_children, curr_child;
static HHOOK output_hookproc; 		/* keyboard hook procedure handle */

static HWND *window_list;		/* list of windows with WS_TABSTOP */

/**** Private Functions *****/
static void edit_flowgraph_line(HWND hDlg);
static void print_flow_calls_in_window(flowInfo *calls, int depth);
static void display_flowgraph(flowInfo *routine);
static void save_flowgraph(FILE *file);
static int add_string_to_output(char *string);
static void get_child_windows(HWND hParent, HINSTANCE hInst);

/*****
* Retrieve the currently selected line from the output dialog.
* This line is scanned to pick up the filename part of that output line.
* When a valid filename is found, an editor is started with the file selected.
*****/
static void
edit_flowgraph_line(HWND hDlg)
{
	int index, line;
	char szBuf[256], file[MAXPATHLEN];
	char *current_chPtr, *chPtr;

   	memset(szBuf, '\0', MAXPATHLEN);

	/* Get index of selected line. If OK, we get the line itself */
	index = (int)SendDlgItemMessage(hDlg, IDC_OUTPUT_LISTBOX,
		LB_GETCURSEL, 0, 0L);

	if((int)index == LB_ERR)
	{
		ErrHandler(hDlg, R_WARNING, RB_OK1, "Nothing selected. Please "
			"select an item from the list");
		return;
	}

	/* pick up text */
	SendDlgItemMessage(hDlg, IDC_OUTPUT_LISTBOX, LB_GETTEXT, index, 
		(LONG)(LPSTR)szBuf);

	/* if this is a ``repeated'' line, get the previous line. */
	if((!strstr(szBuf, "(")) && (!strstr(szBuf, ")")))
		SendDlgItemMessage(hDlg, IDC_OUTPUT_LISTBOX,
			LB_GETTEXT, index-1, (LONG)(LPSTR)szBuf);
	/*
	* Filter filename part out of the line retrieved.
	* The format of this line for a resolved call reads:
	*	 <text> (filename, linenum)
	* The format of this line for a UNresolved call reads:
	*	<text> (<unknown>)
	* The format of this line when collapse_consecutive == 1 reads:
	*	<text> repeated <num> times
	*/
	for(chPtr = &szBuf[0]; *chPtr != '\0' && *chPtr != '(' ; chPtr++);

	if(*chPtr == '\0')
		return;

	/* move past ``('' */
	current_chPtr = chPtr+1;
	/* get terminating ``)'' of ``,'' if a linenumber is present */
	for(index = 0; *chPtr != ',' && *chPtr != ')'; chPtr++, index++);
	/* copy filename */
	strncpy(file, current_chPtr, index);
	file[index-1] = '\0';

	/* return if we have an unknown file */
	if(!strcmp(file, UNDEF))
		return;

	/* pick up line number */
	for(index = 0; !isdigit(*chPtr); chPtr++, index++);
	current_chPtr = chPtr;
	for(index = 0; *chPtr != ')'; chPtr++, index++);
	current_chPtr[index] = '\0';
	line = atoi(current_chPtr);

	/* call the editor with this file and (possible) line number */
	edit_file(file, line);
}

/*****
* The flowgraph output window procedure
*****/
#pragma argsused
BOOL CALLBACK _export OutputProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam)
{
	static WINDOWPLACEMENT curr_pos; /* save dialog position and size */

	switch(message)
	{
		case WM_INITDIALOG:
			/* register with resize */
			RegisterResize(hDlg, hInst);

			/*
			* Save window handle, HDC and textmetrics for the
			* output listbox. Needed for scrollbar width adjustment.
			*/
			hwndLB = GetDlgItem(hDlg, IDC_OUTPUT_LISTBOX);
			hdcLB = GetDC(hwndLB);
			GetTextMetrics(hdcLB, &tm);

			/* default values for text fields */
			SendDlgItemMessage(hDlg, IDC_OUTPUT_LISTBOX,
				LB_RESETCONTENT, 0, 0L);
			SetDlgItemText(hDlg, IDC_OUTPUT_FILENAME, UNDEF);
			SetDlgItemText(hDlg, IDC_OUTPUT_ROUTINE_NAME, UNDEF);
			SetDlgItemText(hDlg, IDC_OUTPUT_ROUTINE_TYPE,
				"Subroutine name:");
			SetDlgItemInt (hDlg, IDC_OUTPUT_TOTAL_CALLS, 0, FALSE);
			SetDlgItemInt (hDlg, IDC_OUTPUT_RESOLVED, 0, FALSE);
			SetDlgItemInt (hDlg, IDC_OUTPUT_UNRESOLVED, 0, FALSE);
			/* initialise keyboard interface */
			num_children = 0;
			curr_child = 0;
			get_child_windows(hDlg, hInst);
			if((output_hookproc = SetWindowsHookEx(WH_KEYBOARD,
				(HOOKPROC)OutputKeyboardProc, hInst,
				GetCurrentTask())) == NULL)
				ErrHandler(NULL, R_ERROR, RB_OK1, "Winfflow "
					"Output: failed to set keyboard hook.\n"
					"Keyboard interface not installed.");

			/* Close button is Abort when generating a flowgraph */
			SetDlgItemText(hDlg, IDOK, "Abort");
			/* listbox has initial focus */
			SetFocus(GetDlgItem(hDlg, IDC_OUTPUT_LISTBOX));
			if(curr_pos.length)
				SetWindowPlacement(hDlg, &curr_pos);
			return(0);
		case WM_MOVE:
			curr_pos.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hDlg, &curr_pos);
			return(TRUE);
		/* save window state and perform resizing */
		case WM_SIZE:
			switch(wParam)
			{
				case SIZE_RESTORED:
					curr_pos.length=sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hDlg, &curr_pos);
					/* fall through */
				case SIZE_MAXIMIZED:
					DoResize(hDlg, 0);
					return(TRUE);
				default:
					break;
			}
			break;
		case WM_COMMAND:
			switch(GET_COMMAND_ID)
			{
				/* close window */
				case IDOK:
				{
					char szBuf[10];
					memset(szBuf, '\0', 10);
					GetDlgItemText(hDlg, IDOK, szBuf, 10);
					/* Check if this is Cancel or not */
					if(!strcmpi(szBuf, "Abort"))
						flow_abort = TRUE;
					else
						SendMessage(hDlg,WM_CLOSE,0,0L);
					return(TRUE);
				}
				/* edit selection */
				case IDC_OUTPUT_EDIT:
					edit_flowgraph_line(hDlg);
					return(TRUE);

				/* save to file */
				case IDC_OUTPUT_SAVE:
				{
					char *save_filename;
					char szFilter[128];
					char filename[14];

					if (!LoadString(hInst,IDS_FILTERSTRING1,
						szFilter, sizeof(szFilter)))
					{
						ErrHandler(hDlg,R_ERROR,RB_OK1,
							"Internal Error: failed"
							" to retrieve a needed"
							" string resource");
						return(TRUE);
					}
					/* default filename to use */
					strncpy(filename, current_func, 8);
					strcat(filename, ".out");
					/* get save filename */
					save_filename = display_save_dialog(output_dialog,
						filename, szFilter);
					if(save_filename == NULL)
						return(TRUE);
					/* return if unable to create file */
					if((save_file = create_file(save_filename)) == NULL)
						return(TRUE);
					save_file = fopen(save_filename, "w");
					save_flowgraph(save_file);
					fclose(save_file);
					return(TRUE);
				}
				/* edit when doubleclicked */
				case IDC_OUTPUT_LISTBOX:
					if(GET_COMMAND_CMD == LBN_DBLCLK)
						edit_flowgraph_line(hDlg);
					return(TRUE);
				case IDHELP:
					WinHelp(hWndMain, "winfflow.hlp",
						HELP_CONTEXT,
						WINFFLOW_OUTPUT_DIALOG);
				return (TRUE);
			}
			break;
		case WM_SYSCOMMAND:
			if(GET_COMMAND_ID == SC_CLOSE)	/* Get the close item */
			{
				SendMessage(hDlg, WM_CLOSE, 0, 0L);
				return(TRUE);
			}
			break;
		case WM_CLOSE:
			ReleaseDC(hwndLB, hdcLB);
			UnregisterResize(hDlg);
			/* release keyboard hook */
			free(window_list);
			UnhookWindowsHookEx(output_hookproc);
			DestroyWindow(hDlg);
			output_dialog = NULL;
			return(TRUE);
	}
	return(FALSE);
}

/*****
* display the flowgraph output window
*****/
void
display_output_box(void)
{
	DLGPROC lpfnOutputProc;

	/* initialise output dialog box */
	lpfnOutputProc=(DLGPROC)MakeProcInstance((FARPROC) OutputProc,
		hInst);
	output_dialog = CreateDialog(hInst,
		MAKEINTRESOURCE(WINFFLOW_OUTPUT_DIALOG), hWndMain,
		(DLGPROC)lpfnOutputProc);
}

/*****
* Save the flowgraph in the listbox to the file save_file.
* This file expects a FILE pointer to the save_file, so it's
* the caller's responsibility to provide this handle.
*****/
static void
save_flowgraph(FILE *file)
{
	int i,j;
	char szBuf[256];

	/* get no of elements in the list */
	j = (int)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

	/* save all elements to the file */
	for(i = 0; i < j; i++)
	{
		SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)((LPSTR)szBuf));
		fprintf(file, "%s\n", szBuf);
	}
}

/*****
* This routine adds a string to the output window.
* If the listbox is full, this routine tells the user so and
* asks him if he wants to save the flowgraph to a file.
* If the user responds positive, a file dialog is popped up
* and the user enters a filename. The flowgraph will then
* be saved to this file.
* If he answers no, this routine terminates the flowgraph generation
* and keeps the output dialog open.
* If he answers cancel, this routine terminates the flowgraph generation
* and pops down the output dialog.
* Flowgraph termination is done by doing a longjmp.
*****/
static int
add_string_to_output(char *string)
{
	short ret_val = 0;

	/* do this when saving to file */
	if(save_to_file)
	{
		if(!fprintf(save_file, "%s\n", string))
		{
			ErrHandler(output_dialog, R_ERROR, RB_OK1,
				"Write failed, disk full?");
			/* return, keep dialog up */
			return(1);
		}
		return(0);
	}

	/* adjust horizontal scrollbar width */
	adjust_lb_width(hwndLB, hdcLB, tm, string);
	/* add string */
	ret_val = (short)SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)string);

	if(ret_val < 0)
	{
		char *save_filename;
		ret_val = (short)ErrHandler(output_dialog, R_QUESTION, RB_YES3,
			"The generated flowgraph for %s will be too"
			" large to fit entirely into the listbox.\n"
			"Do you wish to save it to file?", current_func);
		switch(ret_val)
		{
			case IDYES:
			{
				char szFilter[128];
				char filename[14];
				memset(filename, '\0', 14);

				if(!LoadString(hInst,IDS_FILTERSTRING1,szFilter,
					sizeof(szFilter)))
				{
					ErrHandler(output_dialog,R_ERROR,RB_OK1,
						"Internal Error: failed to "
						"retrieve a needed string "
						"resource");
					/* terminate loop, output dialog up */
					return(1);
				}
				/* default filename to use */
				strncpy(filename, current_func, 8);
				strcat(filename, ".out");
				save_filename=display_save_dialog(output_dialog,
					filename, szFilter);

				if(save_filename == NULL)
					/* terminate loop, output dialog up */
					return(1);

				/* return if unable to create file */
				if((save_file=create_file(save_filename))==NULL)
					return(1);
				save_flowgraph(save_file);
				save_to_file = TRUE;
				return(0);
			}
			case IDNO:
				/* terminate loop, output dialog up */
				return(1);
			case IDCANCEL:
				/* terminate loop, output dialog down */
				return(2);
		}
	}
	return(0);
}

/*****
* Print the flowgraph to the given window. We follow up to depth levels
* deep. If this depth is reached, recursion might be happening. If this
* is the case, the program aborts with an appropriate message. The
* default value for depth is MAXDEPTH, defined to 64.
*
* This routine is recursive.
*****/
static void
print_flow_calls_in_window(flowInfo *calls, int depth)
{
	flowInfo *tmp = NULL;
	static int i;
	static char szBuf[256], depth_buf[2*MAXDEPTH+8];
	char string[10];
	MSG msg;

	/* cutoff flowgraph generation if the user has told us to do so */
	if(cutoff_depth && depth >= cutoff_depth)
		return;

	if(depth >= MAXDEPTH)
	{
		ErrHandler(output_dialog, R_ERROR, RB_OK1,
			"Error: I'm %i levels deep. This might indicate "
			"undetected recursion.\nCurrent function is %s in "
			"file %s\n",
			depth, calls->name, calls->file);
		/* terminate loop, output dialog up */
		longjmp(jumper,1);
		return;
	}
	/* No for loop here if we want to use the PeekMessage below. */
	tmp = calls;
	while(tmp)
	{	/* start of while */
		/* Having a private message loop allows the user to abort. */
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;	/* start next loop */
		}
		/* aborted by user, terminate loop, output dialog up */
		if(flow_abort)
			longjmp(jumper,1);
		/* reset string buffers */
		memset(szBuf, '\0', sizeof(szBuf));
		memset(depth_buf, '\0', MAXDEPTH);
		/* compose prefix string to add */
		sprintf(depth_buf, "%-5i", depth+1);
		for(i = 0 ; i < depth; i++)
			strcat(depth_buf," |");
		/* this is a resolved call */
		if(tmp->parent)
		{
			/* increment known counter */
			ultoa(++num_calls_known, string, 10);
			SetDlgItemText(output_dialog, IDC_OUTPUT_RESOLVED, 
				string);
			if(!hide_resolved)
			{
				sprintf(szBuf, "%s | -> %s, line %i (%s%s, "
					"line %i)", depth_buf, tmp->name,
					tmp->defline, tmp->parent->path,
					tmp->parent->file,tmp->parent->defline);
				/* add string to output, check return value. */
				i = add_string_to_output(szBuf);
				if(i != 0)
					longjmp(jumper, i);
			}
		}
		else
		{
			/* increment unknown counter */
			num_calls_unknown++;
			ultoa(num_calls_unknown, string, 10);
			SetDlgItemText(output_dialog, IDC_OUTPUT_UNRESOLVED, 
				string);
			if(!hide_unresolved)
			{
				sprintf(szBuf, "%s | -> %s, line %i ("
					UNDEF ")", depth_buf,
					tmp->name, tmp->defline);
				/* add string to output, check return value. */
				i = add_string_to_output(szBuf);
				if(i != 0)
					longjmp(jumper, i);
			}
		}
		/*
		* If this is true, skip past all routines with the
		* same name as the current routine.
		*/
		if(collapse_consecutive)
		{
			int num_collapsed = 0;
			/*
			* skip past these calls and count how many. When
			* this loop terminates, tmp will point to the
			* last call with this name. The outermost
			* for loop will adjust tmp to point to the call
			* after this one.
			*/
			for(; tmp->next != NULL &&
				!strcmp(tmp->name, tmp->next->name);
				tmp = tmp->next, num_collapsed++);
			sprintf(szBuf, "%s | -> %s repeated %i times",
				depth_buf, tmp->name, num_collapsed);
			if(num_collapsed)
			{
				/*
				* Add this string if the following is true:
				* 1. call has got a parent _and_ we are to 
				*    show resolved calls.
				* -or-
				* 2. call hasn't got a parent _and_ we are to 
				*    show unresolved calls.
				*/
				if((tmp->parent && !hide_resolved) ||
					(!tmp->parent && !hide_unresolved))
				{
					sprintf(szBuf, "%s | -> %s repeated %i "
						"times", depth_buf, tmp->name, 
						num_collapsed);
					i = add_string_to_output(szBuf);
					if(i != 0)
						longjmp(jumper, i);
				}
				/* increment appropriate counter */
				if(tmp->parent)
					num_calls_known += num_collapsed;
				else
					num_calls_unknown += num_collapsed;
			}
		}
		/* display current no of total calls */
		ultoa(num_calls_known+num_calls_unknown, string, 10);
		SetDlgItemText(output_dialog, IDC_OUTPUT_TOTAL_CALLS, string);
		/*
		* Try to resolve next call if current call has got a
		* parent (= is resolvabe), this parent is not recursive
		* and has got calls.
		*/
		if(tmp->parent && !tmp->parent->recursive &&
			tmp->parent->calls)
			print_flow_calls_in_window(tmp->parent->calls,
				depth+1);
		tmp = tmp->next;	/* increment pointer */
	}	/* end of while */
	return;
}

/*****
* Create & display a flowgraph for the given routine
*****/
static void
display_flowgraph(flowInfo *routine)
{
	static char msg_buf[256];
	char string[10];
	int value = 0;
	unsigned long file_size = 0;

	/* reset string buffers */
	memset(msg_buf, '\0', sizeof(msg_buf));

	/* clear any previous listings */
	SendMessage(hwndLB, LB_RESETCONTENT,0,0l);

	/* compose full filename */
	sprintf(msg_buf, "%s%s", routine->path, routine->file);
	/* get size of file */
	file_size = check_if_file_exists(msg_buf);
	/* compose string to add */
	sprintf(msg_buf, "%s%s (%lu bytes)", routine->path,
		routine->file, file_size);

	/* Set & clear texts in output dialog */
	/* Close button is Abort when generating a flowgraph */
	SetDlgItemText(output_dialog, IDOK, "Abort");
	SetDlgItemText(output_dialog, IDC_OUTPUT_FILENAME, msg_buf);
	SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_NAME, routine->name);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_TOTAL_CALLS, 0, FALSE);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_RESOLVED, 0, FALSE);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_UNRESOLVED, 0, FALSE);

	/* name of current routine. */
	current_func = routine->name;
	/* initialise counters */
	num_calls_known = num_calls_unknown = 0;

	/* no saving to file */
	save_to_file = FALSE;

	/* reset string buffers */
	memset(msg_buf, '\0', sizeof(msg_buf));

	/*
	* Compose first flowgraph line: routine type, name of routine, file
	* and line where this routine is defined.
	*/
	switch(routine->Type)
	{
		case PROGRAM:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Program name:");
			sprintf(msg_buf, "program %s (%s%s, line %i)",
				routine->name, routine->path, routine->file,
				routine->defline);
			break;
		case FUNCTION:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Function name:");
			sprintf(msg_buf, "function %s (%s%s, line %i)",
				routine->name, routine->path, routine->file,
				routine->defline);
			break;
		case SUBROUTINE:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Subroutine name:");
			sprintf(msg_buf, "subroutine %s (%s%s, line %i)",
				routine->name, routine->path, routine->file,
				routine->defline);
			break;
		default:	/* should not be reached */
			strcpy(msg_buf, "(unknown type)");
	}
	/* adjust horizontal scrollbar width */
	adjust_lb_width(hwndLB, hdcLB, tm, msg_buf);
	/* add text to output listbox */
	SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)msg_buf);

	/* reset flowgraph generation abort flag */
	flow_abort = FALSE;

	/* Jump point when flowgraph generation is terminated prematurely. */
	value = setjmp(jumper);

	/*
	* setjmp returns 0 on initialistion and returns the return value
	* from longjmp. 0 return value indicates normal termination.
	*/
	if(value == 0)
	{
		if(routine->calls)
			print_flow_calls_in_window(routine->calls, 0);
		else
			SendDlgItemMessage(output_dialog, IDC_OUTPUT_LISTBOX,
				LB_ADDSTRING, 0,
				(LPARAM)("contains no CALL statements"));
	}
	/* report final statistics */
	ultoa(num_calls_known+num_calls_unknown, string, 10);
	SetDlgItemText(output_dialog, IDC_OUTPUT_TOTAL_CALLS, string);
	ultoa(num_calls_known, string, 10);
	SetDlgItemText(output_dialog, IDC_OUTPUT_RESOLVED, string);
	ultoa(num_calls_unknown, string, 10);
	SetDlgItemText(output_dialog, IDC_OUTPUT_UNRESOLVED, string);
	if(flow_abort)
		ErrHandler(output_dialog, R_MESSAGE, RB_OK1, "Flowgraph "
			"Generation aborted.\nThe resulting flowgraph will "
			"be incomplete.");
	SetDlgItemText(output_dialog, IDOK, "&Close");

	/* cancel pressed, close output dialog */
	if(value == 2 && output_dialog)
		SendMessage(output_dialog, WM_CLOSE, 0, 0L);

	/* close output file (if any) */
	if(save_to_file)
		fclose(save_file);
	save_to_file = FALSE;
}

/*****
* generate a flow graph for all routines/functions in a given file
* Currently unused.
*****/
void
file_flow_graph(flowInfo *list, char *file)
{
	flowInfo *tmp, *start;

	/* figure out where to start the flowgraph */
	for(tmp=list; tmp != NULL && strcasecmp(file, tmp->file);
		tmp = tmp->next);

	if(tmp == NULL)
	{
		ErrHandler(output_dialog, R_ERROR, RB_OK1,
			"Could not find file %s anywhere, "
			"I'm stymied\n", file);
		return;
	}
	start = tmp;

	/* Display flowgraph for each subroutine/function found in this file */
	for(tmp = start; tmp != NULL;
		tmp = (tmp->next != NULL && !strcmpi(file, tmp->next->file) ?
			tmp->next : NULL))
		display_flowgraph(tmp);
}

/*****
* generate a flow graph for a given file
*****/
void
func_flow_graph(flowInfo *list, char *func)
{
	flowInfo *tmp;

	/* figure out where to start */
	for(tmp=list; tmp != NULL && strcasecmp(func, tmp->name);
		tmp = tmp->next);

	if(tmp == NULL)
	{
		ErrHandler(output_dialog, R_ERROR, RB_OK1,
			"Could not find routine or "
			"function %s anywhere, I'm stymied\n",
			func);
		return;
	}

	display_flowgraph(tmp);
}

/*****
* display all callers of subroutine func.
*****/
void
show_refs(flowInfo *list, flowInfo *func)
{
	flowInfo *tmp, *calls;
	static char msg_buf[256];
	char string[10];
	unsigned long file_size = 0;
	int num_refs = 0;

	SetDlgItemText(output_dialog, IDOK, "&Close");
	/* reset string buffers */
	memset(msg_buf, '\0', sizeof(msg_buf));
	/* clear any previous listings */
	SendMessage(hwndLB, LB_RESETCONTENT,0,0l);
	if(func)
	{
		/* compose full filename */
		sprintf(msg_buf, "%s%s", func->path, func->file);
		/* get size of file */
		file_size = check_if_file_exists(msg_buf);
		/* compose string to add */
		sprintf(msg_buf, "%s%s (%lu bytes)", func->path,
			func->file, file_size);
	}
	else
		sprintf(msg_buf, "<unknown>");

	/* Set & clear texts in output dialog */
	/* Close button is Abort when generating a flowgraph */
	SetDlgItemText(output_dialog, IDC_OUTPUT_FILENAME, msg_buf);
	SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_NAME, func->name);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_TOTAL_CALLS, 0, FALSE);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_RESOLVED, 0, FALSE);
	SetDlgItemInt (output_dialog, IDC_OUTPUT_UNRESOLVED, 0, FALSE);

	/* initialise counters */
	num_calls_known = num_calls_unknown = 0;

	/* no saving to file */
	save_to_file = FALSE;

	/* reset string buffers */
	memset(msg_buf, '\0', sizeof(msg_buf));

	/*
	* Compose first flowgraph line: routine type, name of routine, file
	* and line where this routine is defined.
	*/
	switch(func->Type)
	{
		case PROGRAM:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Program name:");
			if(func)
			sprintf(msg_buf, "program %s (%s%s, line %i)",
				func->name, func->path, func->file,
				func->defline);
			break;
		case FUNCTION:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Function name:");
			sprintf(msg_buf, "function %s (%s%s, line %i)",
				func->name, func->path, func->file,
				func->defline);
			break;
		case SUBROUTINE:
			SetDlgItemText(output_dialog, IDC_OUTPUT_ROUTINE_TYPE,
				"Subroutine name:");
			sprintf(msg_buf, "subroutine %s (%s%s, line %i)",
				func->name, func->path, func->file,
				func->defline);
			break;
		default:	/* should not be reached */
			strcpy(msg_buf, "(unknown type)");
	}
	/* adjust horizontal scrollbar width */
	adjust_lb_width(hwndLB, hdcLB, tm, msg_buf);
	/* add text to output listbox */
	SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)msg_buf);

	for(tmp = list; tmp!= NULL; tmp = tmp->next)
	{
		for(calls = tmp->calls; calls != NULL; calls = calls->next)
		{
			if(!strcmpi(func->name, calls->name))
			{
				/* reset string buffers */
				memset(msg_buf, '\0', sizeof(msg_buf));
				switch(tmp->Type)
				{
					case PROGRAM:
						sprintf(msg_buf, "program %s (%s%s, line %i)",
							tmp->name, tmp->path, tmp->file,
							calls->defline);
						break;
					case FUNCTION:
						sprintf(msg_buf, "function %s (%s%s, line %i)",
							tmp->name, tmp->path, tmp->file,
							calls->defline);
					break;
					case SUBROUTINE:
						sprintf(msg_buf, "subroutine %s (%s%s, line %i)",
							tmp->name, tmp->path, tmp->file,
							calls->defline);
						break;
					default:	/* should not be reached */
						break;
				}
				num_refs++;
				/* adjust horizontal scrollbar width */
				adjust_lb_width(hwndLB, hdcLB, tm, msg_buf);
				/* add text to output listbox */
				SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)msg_buf);
			}
		}
	}
	if(num_refs == 0)
	{
		sprintf(msg_buf, "not referenced by any subroutine");
		SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)msg_buf);
	}
	/* report final statistics */
	ultoa(num_refs, string, 10);
	SetDlgItemText(output_dialog, IDC_OUTPUT_TOTAL_CALLS, string);
}

/*****
* Output dialog keyboard interface routine.
* See comments at KeyboardProc in winmain.c
*****/
LRESULT CALLBACK _export
OutputKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if(wParam == VK_F1)
	   PostMessage(output_dialog, WM_COMMAND, IDHELP, 0L);
	else if(0x80000000L & lParam)
	{
		switch(GET_COMMAND_ID)
		{
			case VK_TAB:
				if(GetFocus() == window_list[curr_child])
				{
					/* walk back when shift is pressed */
					if(GetKeyState(VK_SHIFT) < 0)
						curr_child =
						(--curr_child == -1 ?
						num_children-1 : curr_child);
					else
						curr_child =
						(++curr_child == num_children ?
						0 : curr_child);
				}
				SetFocus(window_list[curr_child]);
				break;
			/*
			* Can't use RETURN. If used and the output
			* dialog is activated by pressing enter in the
			* output selector, this also get's called. And
			* thats dead wrong.
			*/
			case VK_SPACE:
				if(GetFocus() == hwndLB)
					PostMessage(output_dialog, WM_COMMAND, 
						IDC_OUTPUT_EDIT, 0L);
				SetFocus(window_list[curr_child]);
				break;
			case VK_F1:
			   PostMessage(output_dialog, WM_COMMAND, IDHELP, 0L);
               break;
		}
	}

	return((code < 0 ?
		CallNextHookEx(output_hookproc, code, wParam, lParam) : 0));
}

/*****
* Count no of windows with WS_TABSTOP defined.
* Called twice: when lParam == 0, count no of buttons, else add to
* window list.
*****/
BOOL CALLBACK _export
output_count_children(HWND hDlg, LPARAM lParam)
{
	long class_style;

	class_style = GetWindowLong(hDlg, GWL_STYLE);
	if(class_style & WS_TABSTOP)
	{
		if(lParam)
			window_list[num_children] = hDlg;
		num_children++;
	}
	return(TRUE);
}

/*****
* Enumerate all child windows of hParent and store them in the private
* array window_list.
*****/
static void
get_child_windows(HWND hParent, HINSTANCE hInst)
{
	WNDENUMPROC enumProc;

	/* Create procedure address for the enumeration routine. */
	enumProc = (WNDENUMPROC)MakeProcInstance((FARPROC)output_count_children,
		hInst);
	/* count no of windows with WS_TABSTOP defined */
	num_children = 0;
	EnumChildWindows(hParent, enumProc, 0L);
	/* allocate memory for it */
	window_list = (HWND*)malloc(num_children*sizeof(HWND));
	num_children = 0;
	/* build window list */
	EnumChildWindows(hParent, enumProc, 1L);

	FreeProcInstance((FARPROC)enumProc);
}

