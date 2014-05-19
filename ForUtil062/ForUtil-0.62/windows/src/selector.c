static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winfflow/RCS/selector.c,v 1.2 1996/08/27 19:31:13 koen Exp koen $";
/*****
* selector.c: Window procedures for the selector dialog. MS-Windows version.
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	08-08-1996
* Last modification: 	$Date: 1996/08/27 19:31:13 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Log: selector.c,v $
* Revision 1.2  1996/08/27 19:31:13  koen
* Support for all new settings and reference browsing
*
* Revision 1.1  1996/08/18 18:34:28  koen
* Initial Revision
*
*****/
#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#ifdef WIN32
#include <bwcc.h>
#endif

#include "forutil.h"
#include "windows\lib\winutil.h"
#include "windows\src\winfflow.h"	/* contains the flowInfo struct */
#include "windows\src\resource.h"

/**** Explicitly Exported Function ****/
BOOL FAR PASCAL _export SelectorProc(HWND hDlg, UINT message, UINT wParam,
	LONG lParam);

/**** Private Variables ****/
static char curr_file[128];	/* currently selected file */
static HDC hdc_sel;		/* HDC of selector listbox */
static TEXTMETRIC tm_sel;	/* TEXTMETRIC of selector listbox */
static HWND hwnd_sel;		/* HWND handle of selector listbox */

/**** Private Functions ****/
static void fill_selector_listbox(HWND hDlg, int selector_type);
static void fill_selector_listbox_second_stage(int type, flowInfo *flow);
static void fill_selector_combobox(HWND hDlg);
static void get_and_display_selector(HWND hDlg, int selector, int type);

/*****
* Routine to fill the combobox containing the names of the files
* in the output selector dialog
******/
static void
fill_selector_combobox(HWND hDlg)
{
	flowInfo *tmp;
	static char szBuf[128];

	/* empty any current list */
	SendDlgItemMessage(hDlg, IDC_SELECTOR_COMBOBOX, CB_RESETCONTENT,0,0l);

	/* add an ``All Files'' to the listbox */
	SendDlgItemMessage(hDlg, IDC_SELECTOR_COMBOBOX, CB_ADDSTRING, 0,
		(LPARAM)DEFAULT_FILE);

	/* add all files to the listbox */
	for(tmp = flow; tmp != NULL ; tmp = tmp->next)
	{
		sprintf(szBuf, "%s", tmp->file);
		SendDlgItemMessage(hDlg, IDC_SELECTOR_COMBOBOX, CB_ADDSTRING, 0,
			(LPARAM)szBuf);
		/* skip all other entries in this file */
		for(; tmp->next != NULL && !strcasecmp(szBuf, tmp->next->file);
			tmp = tmp->next);
	}

	/* Select the default item */
	SendDlgItemMessage(hDlg, IDC_SELECTOR_COMBOBOX, CB_SETCURSEL, 0, 0L);
}

/*****
* Fill the listbox of the selector dialog.
* Type indicates what routines we should display.
*****/
static void
fill_selector_listbox(HWND hDlg, int selector_type)
{
	flowInfo *tmp;

	hwnd_sel = GetDlgItem(hDlg, IDC_SELECTOR_LISTBOX);
	hdc_sel = GetDC(hwnd_sel);
	GetTextMetrics(hdc_sel, &tm_sel);

	SendMessage(hwnd_sel, LB_RESETCONTENT, 0, 0L);

	/* We add all items to the listbox if ``All Files'' has been selected */
	if(!strcmp(curr_file, DEFAULT_FILE) ||
		selector_type == IDC_SELECTOR_SHOW_UNUSED)
	{
		for(tmp = flow; tmp != NULL ; tmp = tmp->next)
			fill_selector_listbox_second_stage(selector_type, tmp);

	}
	/* Only add items to the listbox for the current file */
	else
	{
		/* pick up the selected file */
		for( tmp = flow; tmp != NULL && strcmp(curr_file, tmp->file);
			tmp = tmp->next);

		/* sanity check */
		if(tmp == NULL)
		{
			ErrHandler(hDlg, R_ERROR, RB_OK1, "Could not locate "
				"selected file (%s)", curr_file);
			return;
		}

		/* found it, fill listbox for all entries in a file */
		while(1)
		{
			fill_selector_listbox_second_stage(selector_type, tmp);
			if(tmp->next==NULL||strcmp(curr_file,tmp->next->file))
				break;
			else
				tmp = tmp->next;
		}
	}
	ReleaseDC(hwnd_sel, hdc_sel);
}

/*****
* Add string to the listbox in the output selector dialog
*****/
static void
fill_selector_listbox_second_stage(int type, flowInfo *flow)
{
	static char szBuf[128];

	sprintf(szBuf, "%s", flow->name);

	/* adjust horizontal scrollbar width */
	adjust_lb_width(hwnd_sel, hdc_sel, tm_sel, szBuf);

	switch(type)
	{
		/* functions only */
		case IDC_SELECTOR_FUNCTION:
			if(flow->Type == FUNCTION || flow->Type == PROGRAM)
				SendMessage(hwnd_sel, LB_ADDSTRING, 0,
					(LPARAM)szBuf);
			break;
		/* subroutines only */
		case IDC_SELECTOR_SUBROUTINE:
			if(flow->Type == SUBROUTINE || flow->Type == PROGRAM)
				SendMessage(hwnd_sel, LB_ADDSTRING, 0,
					(LPARAM)szBuf);
			break;
		/* routines and functions */
		case IDC_SELECTOR_SUBROUTINE+IDC_SELECTOR_FUNCTION:
			SendMessage(hwnd_sel, LB_ADDSTRING, 0, (LPARAM)szBuf);
			break;
		/* show unused subroutines */
		case IDC_SELECTOR_SHOW_UNUSED:
			if(!flow->called && flow->Type == SUBROUTINE)
				SendMessage(hwnd_sel, LB_ADDSTRING, 0, 
					(LPARAM)szBuf);
			break;
		/* program only */
		default:
			if(flow->Type == PROGRAM)
				SendMessage(hwnd_sel, LB_ADDSTRING, 0,
					(LPARAM)szBuf);
			break;
	}
}

/*****
* Main flowgraph display & selection routine.
* This routine gets the current selection in the selector listbox
* and calls the appropriate flowgraph generation driver routine.
*
* Type indicates if the user wants a flowgraph (type = SELECTOR_VIEW)
* or browse the references (type = SELECTOR_BROWSE)
*****/
#pragma argsused
static void
get_and_display_selector(HWND hDlg, int selector, int type)
{
	DWORD index;
	static char szBuf[128];
	BOOL no_err = TRUE;	/* GetDlgItemInt error flag */
	flowInfo *tmp;

	/* get index in listbox */
	index = SendDlgItemMessage(hDlg, IDC_SELECTOR_LISTBOX, LB_GETCURSEL,
		0, 0L);
	if((int)index == LB_ERR)
	{
		ErrHandler(hDlg, R_WARNING, RB_OK1, "Nothing selected. Please "
			"select a routine from the list");
		return;
	}
	/* get cutoff depth */
	if(IsDlgButtonChecked(hDlg, IDC_SELECTOR_CUTOFF))
	{
		cutoff_depth = GetDlgItemInt(hDlg, IDC_SELECTOR_CUTOFF_DEPTH,
			&no_err, FALSE);
		if(!no_err)
			cutoff_depth = 0;
	}
	/* retrieve the item */
	SendDlgItemMessage(hDlg, IDC_SELECTOR_LISTBOX, LB_GETTEXT,
		(WPARAM)index, (LPARAM)((LPSTR) szBuf));

	/* check validity of routine if we must show subroutine references. */
	if(type == IDC_SELECTOR_BROWSE)
	{
		for(tmp = flow ; tmp != NULL && strcasecmp(szBuf, tmp->name);
			tmp = tmp->next);
		/* return if this is a function */
		if(tmp && tmp->Type == FUNCTION)
		{
			ErrHandler(NULL, R_MESSAGE, RB_OK1, "%s is a function. I can"
				" only show references to subroutines.", szBuf);
			return;
		}
		if(!tmp)
		{
			ErrHandler(NULL, R_ERROR, RB_OK1, "Internal Error: could not "
				"locate routine %s", szBuf);
			return;
		}
	}

	/* start up the output dialog */
	if(!output_dialog)
		display_output_box();
	else
	{
		/* already up, raise to the top of the window stack */
		BringWindowToTop(output_dialog);
		/* this call will trigger DoResize. */
		RedrawWindow(output_dialog, NULL, NULL, RDW_UPDATENOW);
	}

	/* reset any previous contents */
	SendMessage(GetDlgItem(output_dialog, IDC_OUTPUT_LISTBOX),
		LB_RESETCONTENT,0,0l);

	/* go and generate the flowgraph */
	if(type == IDC_SELECTOR_VIEW)
	{
		SetDlgItemText(output_dialog, IDC_OUTPUT_GROUPBOX,
			"Flowgraph");
		func_flow_graph(flow, szBuf);
	}
	else
	{
		SetDlgItemText(output_dialog, IDC_OUTPUT_GROUPBOX,
			"References");
		show_refs(flow, tmp);
	}
}

/*****
* Window procedure for the output selector dialog box
*****/
#pragma argsused
BOOL FAR PASCAL _export
SelectorProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
	static short curr_sel;
	short new_sel;
	static HWND hwndLB;
	static WINDOWPLACEMENT curr_pos; /* save dialog position and size */
	HMENU hMenu;

	switch(message)
	{
		case WM_INITDIALOG:
			/* Disable size and maximize system menu's */
			hMenu = GetSystemMenu(hDlg, FALSE);
			EnableMenuItem(hMenu, SC_SIZE, MF_DISABLED | MF_GRAYED);
			EnableMenuItem(hMenu, SC_MAXIMIZE, 
				MF_DISABLED | MF_GRAYED);

			/* Fill the file list box */
			fill_selector_combobox(hDlg);
			hwndLB = GetDlgItem(hDlg, IDC_SELECTOR_LISTBOX);

			/* all files selected by default */
			strcpy(curr_file, DEFAULT_FILE);

			/* we select both routines and functions by default */
			curr_sel=IDC_SELECTOR_FUNCTION+IDC_SELECTOR_SUBROUTINE;
			hide_resolved = 0;
			fill_selector_listbox(hDlg, curr_sel);
			CheckDlgButton(hDlg, IDC_SELECTOR_FUNCTION, 1);
			CheckDlgButton(hDlg, IDC_SELECTOR_SUBROUTINE, 1);
			SetDlgItemInt(hDlg, IDC_SELECTOR_CUTOFF_DEPTH,
				cutoff_depth, FALSE);
			/* should we enable cutting of flowgraph generation ? */
			CheckDlgButton(hDlg, IDC_SELECTOR_CUTOFF,
				(cutoff_depth != 0 ? 1 : 0));
			if(cutoff_depth)
				/* set cutoff depth */
				SetDlgItemInt(hDlg, IDC_SELECTOR_CUTOFF_DEPTH,
					cutoff_depth,FALSE);
			else
				/* display edit box */
				EnableWindow(GetDlgItem(hDlg,
					IDC_SELECTOR_CUTOFF_DEPTH),FALSE);
			CheckDlgButton(hDlg, IDC_SELECTOR_COLLAPSE,
				collapse_consecutive);
			CheckDlgButton(hDlg, IDC_SELECTOR_HIDE,
				hide_unresolved);
			SendMessage(hDlg,DM_SETDEFID,(WPARAM)IDC_DEFAULT_BUTTON,
				0L);
			if(curr_pos.length)
				SetWindowPlacement(hDlg, &curr_pos);
			return TRUE;
		case WM_MOVE:
			curr_pos.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hDlg, &curr_pos);
			return(TRUE);
		/* save window state */
		case WM_SIZE:
			if(GET_COMMAND_CMD == SIZE_RESTORED)
			{
				curr_pos.length=sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(hDlg, &curr_pos);
				return(TRUE);
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
			if(output_dialog)
				SendMessage(output_dialog,
					WM_CLOSE, 0, 0L);
			EndDialog(hDlg,0);
			return(TRUE);
		case WM_COMMAND:
		switch(GET_COMMAND_ID)
		{
			/* select what to display in the listbox */
			case IDC_SELECTOR_FUNCTION:
			case IDC_SELECTOR_SUBROUTINE:
				new_sel = 0;
				if(IsDlgButtonChecked(hDlg,
					IDC_SELECTOR_FUNCTION))
					new_sel += IDC_SELECTOR_FUNCTION;
				if(IsDlgButtonChecked(hDlg,
					IDC_SELECTOR_SUBROUTINE))
					new_sel += IDC_SELECTOR_SUBROUTINE;
				if(curr_sel != new_sel)
				{
					curr_sel = new_sel;
					fill_selector_listbox(hDlg, curr_sel);
				}
				return TRUE;
			case IDC_SELECTOR_SHOW_UNUSED:
				if(IsDlgButtonChecked(hDlg,
					IDC_SELECTOR_SHOW_UNUSED))
				{
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_FUNCTION), FALSE);
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_SUBROUTINE),FALSE);
					/* Select the default file */
					SendDlgItemMessage(hDlg,
						IDC_SELECTOR_COMBOBOX,
						CB_SETCURSEL, 0, 0L);
					strcpy(curr_file, DEFAULT_FILE);
					/* show all unused routines. */
					fill_selector_listbox(hDlg,
						IDC_SELECTOR_SHOW_UNUSED);
				}
				else
				{
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_FUNCTION), TRUE);
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_SUBROUTINE), TRUE);
					/* trigger a re-display */
					fill_selector_listbox(hDlg, curr_sel);
				}
				return TRUE;
			case IDC_SELECTOR_CUTOFF:
				if(IsDlgButtonChecked(hDlg,
					IDC_SELECTOR_CUTOFF))
				{
					/* enable edit box */
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_CUTOFF_DEPTH),
						TRUE);
					/* set value */
					SetDlgItemInt(hDlg,
						IDC_SELECTOR_CUTOFF_DEPTH,
						(cutoff_depth != 0 ?
						cutoff_depth : maxdepth),
						FALSE);
				}
				else
				{
					/* disable edit box */
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_CUTOFF_DEPTH),
						FALSE);
					cutoff_depth = 0;
				}
				return TRUE;

			/* check state of collapse_consecutive */
			case IDC_SELECTOR_COLLAPSE:
				collapse_consecutive = 0;
				if(IsDlgButtonChecked(hDlg,
					IDC_SELECTOR_COLLAPSE))
					collapse_consecutive = 1;
				if(dynamic_update && output_dialog)
					get_and_display_selector(hDlg,curr_sel,
						IDC_SELECTOR_VIEW);
				return TRUE;

			/* check state of hide_unresolved */
			case IDC_SELECTOR_HIDE:
				hide_unresolved = 0;
				if(IsDlgButtonChecked(hDlg, IDC_SELECTOR_HIDE))
					hide_unresolved = 1;
				if(dynamic_update && output_dialog)
					get_and_display_selector(hDlg,curr_sel,
						IDC_SELECTOR_VIEW);
				return TRUE;
			case IDC_SELECTOR_HIDE_KNOWN:
				hide_resolved = 0;
				if(IsDlgButtonChecked(hDlg, 
					IDC_SELECTOR_HIDE_KNOWN))
					hide_resolved = 1;
				if(dynamic_update && output_dialog)
					get_and_display_selector(hDlg,curr_sel,
						IDC_SELECTOR_VIEW);
				return TRUE;
			/* show flowgraph for the selected routine */
			case IDC_SELECTOR_VIEW:
				get_and_display_selector(hDlg, curr_sel,
					IDC_SELECTOR_VIEW);
				return TRUE;
			/* Browse refences for the selected routine */
			case IDC_SELECTOR_BROWSE:
				get_and_display_selector(hDlg, curr_sel,
					IDC_SELECTOR_BROWSE);
				return TRUE;

			/* double click on an item in the list. */
			case IDC_SELECTOR_LISTBOX:
			if(GET_COMMAND_CMD == LBN_DBLCLK)
			{
				get_and_display_selector(hDlg,
					curr_sel, IDC_SELECTOR_VIEW);
				return TRUE;
			}
			break;

			/* change of selection */
			case IDC_SELECTOR_COMBOBOX:
				if(GET_COMMAND_CMD == CBN_SELCHANGE)
				{
					DWORD index;
					hide_resolved = 0;
					CheckDlgButton(hDlg, IDC_SELECTOR_SHOW_UNUSED,FALSE);
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_FUNCTION), TRUE);
					EnableWindow(GetDlgItem(hDlg,
						IDC_SELECTOR_SUBROUTINE), TRUE);
					index = SendDlgItemMessage(hDlg,
						IDC_SELECTOR_COMBOBOX,
						CB_GETCURSEL, 0, 0L);
					SendDlgItemMessage(hDlg,
						IDC_SELECTOR_COMBOBOX,
						CB_GETLBTEXT, (WPARAM)index,
						(LPARAM)((LPSTR) curr_file));
					fill_selector_listbox(hDlg, curr_sel);
					return TRUE;
				}
				break;
			case IDOK:
				SendMessage(hDlg, WM_CLOSE, 0, 0L);
				return(TRUE);
			case IDHELP:
				WinHelp(hWndMain, "winfflow.hlp",
					HELP_CONTEXT,
					WINFFLOW_OUTPUTSELECTOR_DIALOG);
				return (TRUE);
			case IDC_DEFAULT_BUTTON:
				if(GetFocus() == hwndLB)
				{
					get_and_display_selector(hDlg,curr_sel,
						IDC_SELECTOR_VIEW);
					return(TRUE);
				}
				else
					PostMessage(hDlg,WM_NEXTDLGCTL,0,FALSE);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}
