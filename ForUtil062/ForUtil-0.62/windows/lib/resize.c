static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winutil/RCS/resize.c,v 2.2 1996/08/27 19:23:53 koen Exp koen $";
/*****
* resize.c: resize functions.
*              
* Hicks Version 	1.00
* This file Version	$Revision: 2.2 $
*
* Creation date:	07-06-1995
* Last modification: 	$Date: 1996/08/27 19:23:53 $
*
* Author:		Koen 
* (C)1995 Ripley Software Development
* All Rights Reserved
*
* Although this file and the related header files are written by 
* Ripley Software Development, you may distribute these files
* under the terms of the GNU General Public License. If you don't have
* a copy of the GPL, please contact me and I'll mail it to you.
*****/
/*****
* ChangeLog 
* $Log: resize.c,v $
* Revision 2.2  1996/08/27 19:23:53  koen
* Added support for multiple window management.
*
* Revision 2.1  1996/05/23  16:46:43  koen
* RCS reorganization
*
* Revision 2.1  1996/05/23  03:04:38  koen
* Removed a few lines of obsolete code (between never-defined defines).
*
* Revision 1.10  1996/04/19  16:42:25  koen
* Fixed a bug in UpdateGadgets()
* Partial code clean up + comments added.
*
* Revision 1.9  1995/12/04  03:50:19  koen
* Added the WinList declaration since resize is standalone and WinList is
* declared extern in ResizeP.h
*
* Revision 1.8  1995/08/17  21:57:58  koen
* Added rcs Log directive
*
*****/

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* local includes */
#include "windows\lib\winutil.h"
#include "windows\lib\resizeP.h"

static resize_admin_struct resize_admin[MAX_RESIZE_WINDOWS];
static int has_subs;

/*****
* retrieve the class of a window and return true if we think it's a
* resizeable one.
* Fully resizable controls are groupbox, listbox, edit, combobox
* Horizontal resizable controls are buttons and statics except groupboxes
*****/
int get_window_class(HWND hDlg)
{
	char class_name[256];
	long class_style;

	/* First get the Class of this window */
	if(GetClassName(hDlg, class_name, 256) != 0)
	{
		/* See which class it is */
		if(strcmpi(class_name, "button") == 0)
		{
			/*
			* Get the style button. Groupboxes are fully resizable,
			* all other buttons are considered horizontal 
			* resizable only
			*/
			class_style = GetWindowLong(hDlg, GWL_STYLE);
			return( class_style & BS_GROUPBOX ? ALL_SIZE : H_SIZE);
		}
		else
		{
			/*
			* If the class is edit, listbox or combobox, it's also 
			* resizeable. Shades can be resized without problems, 
			* they only stretch horizontally
			*/
			if (strcmpi(class_name, "listbox") == 0 ||
				strcmpi(class_name, "combobox") == 0 ||
				strcmpi(class_name, "borshade") == 0 ||
				strcmpi(class_name, "edit") == 0 )
				return(ALL_SIZE);
			else
				if(strcmpi(class_name, "static") == 0)
					return(H_SIZE);
				else
					/* listbox part of a combobox */
					if(!strcmpi(class_name,"combolbox"))
					{
						has_subs = TRUE;
						return(H_PARTIAL);
					}
					else /* not resizeable */
						return(NO_SIZE);
		}
	}
	else /* shouldn't get here anyway */
		return(FALSE);
}

/*****
* Add the given window info the given WinInfo list
*****/
WinInfo *save_wininfo(WinInfo *Item, int Id, HWND hDlg, RECT rc)
{
	if(Item == NULL)
	{
		/* allocate memory. Stupid windows usage */
		HGLOBAL hglb;
		hglb = GlobalAlloc(GPTR, sizeof(WinInfo));
		Item = (WinInfo*)GlobalLock(hglb);
		Item->hglb = hglb;

		/* save the id */
		Item->Id = Id;

		/* Save the window handle */
		Item->Gadget = hDlg;

		/* save the RECT info */
		Item->rc = rc;

		/* All windows are always moveable */
		Item->GADmove = TRUE;

		/* GADresize depends on the class of this window */
		Item->GADresize = get_window_class(hDlg);
		Item->next = NULL;
	}
	else
		Item->next = save_wininfo(Item->next, Id, hDlg, rc);
	return(Item);
}

/*****
* Called by EnumChildWindows. Saves the Id, HWND and RECT info
* for hDlg.
*****/
#pragma argsused
BOOL CALLBACK build_wininfo_list(HWND hDlg, LPARAM lParam)
{
	RECT rc, rcClient, rcParent;
	resize_admin_struct *curr_window = (resize_admin_struct*)(lParam);
	int Id;

	/* Get Id of this window */
	Id = GetDlgCtrlID(hDlg);

	/* Get RECT for this window */
	GetWindowRect(hDlg, &rcClient);
	GetWindowRect(GetParent(hDlg), &rcParent);

	/* Store coordinates relative to parent RECT */
	rc.left  = rcClient.left - rcParent.left;
	rc.right = rcClient.right - rcParent.left;
	rc.top   = rcClient.top - rcParent.top - GetSystemMetrics(SM_CYCAPTION);
	rc.bottom= rcClient.bottom-rcParent.top-GetSystemMetrics(SM_CYCAPTION);

	/* store this information */
	curr_window->WinList = save_wininfo(curr_window->WinList, Id, hDlg, rc);

	return(TRUE);
}

/*****
* Update the GadgetList given. Calculates the relative percentages
* of child controls relatively to the dimensions of the main dialog
* dimensions (rc).
*****/
void validate_wininfo(resize_admin_struct *curr_window, HWND hDlg)
{
	WinInfo *tmp, *List;
	static float width, height;
	RECT rc;
	int topMost, bottomMost;

	/* Calculate width and height of main window */
	GetClientRect(hDlg, &rc);
	width = rc.right;
	height = rc.bottom;

	if(width == 0 || height == 0)
		return;	/* don't do a thing then */

	List = curr_window->WinList;

	if(List == NULL)
    	return;
	/*
	* Add CYCAPTION to the Childs of a child window
	* Subtract the height of the caption for controls without childs.
	*/
	for (tmp = List; tmp != NULL; tmp = tmp->next)
		if (GetParent(tmp->Gadget) != hDlg)
		{
			has_subs = TRUE;
			tmp->rc.top    += GetSystemMetrics(SM_CYCAPTION);
			tmp->rc.bottom += GetSystemMetrics(SM_CYCAPTION);
		}
		else
		{
//			tmp->rc.top    -= GetSystemMetrics(SM_CYCAPTION);
//			tmp->rc.bottom -= GetSystemMetrics(SM_CYCAPTION);
		}
	/*
	* Get topmost and bottommost windows 
	* (= windows closest to top and bottom of root window)
	*/
	topMost = List->rc.top ;//- rc.top;
	bottomMost = List->rc.bottom; // - rc.top;

	for (tmp = List; tmp != NULL ; tmp = tmp->next)
	{
		/* Check top and bottom of this Gadget */
		if(tmp->rc.top <= topMost && GetParent(tmp->Gadget) == hDlg)
			topMost = tmp->rc.top;
		if(tmp->rc.bottom >= bottomMost && GetParent(tmp->Gadget)==hDlg)
			bottomMost = tmp->rc.bottom;
	}
	for (tmp = List; tmp != NULL; tmp = tmp->next)
	{
		/*
		* First see if we must stick it to the top or bottom.
		* If so, set the fields to true and calculate the offsets
		* All windows which fall with five (5) points of the top
		* or bottommost window will also be considered as a top or
		* bottommost windows.
		*/
		GetWindowRect(GetParent(tmp->Gadget), &rc);
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
		if(GetParent(tmp->Gadget) == hDlg)
		{
			if(abs(tmp->rc.top - topMost) <= STICK_TRESHOLD)
			{
				tmp->StickWhere = STICK_TOP;
				tmp->topOffset = tmp->rc.top;
			} else 
			if(abs(tmp->rc.bottom - bottomMost) <= STICK_TRESHOLD)
			{
				tmp->StickWhere = STICK_BOTTOM;
				tmp->bottomOffset = rc.bottom -
					(tmp->rc.top + rc.top);
			}
		}
/* Now calculate the relative percentages */
		tmp->resize.pheight= ((float)(tmp->rc.bottom -
			tmp->rc.top)/height);
		tmp->resize.pwidth = ((float)(tmp->rc.right -
			tmp->rc.left)/width);
		tmp->resize.height = tmp->rc.bottom - tmp->rc.top;
		tmp->resize.width  = tmp->rc.right - tmp->rc.left;
		tmp->resize.left   = ((float)tmp->rc.left/width);
		tmp->resize.top    = ((float)tmp->rc.top/height);
		tmp->resize.right  = ((float)tmp->rc.right/width);
		tmp->resize.bottom = ((float)tmp->rc.bottom/height);
	}
}

/*****
* Resize function
*
* There are two versions of this routine: one that handles the resizing
* of windows with sub-parents in them (such as comboboxes) and one
* for windows without sub-parents.
*
* The following applies for this routine only.
* When resizing or moving a control, its new coordinates must be given
* relatively to the upper left corner of the parent of this control.
* A combobox is a control made up by two other controls: a edit control and a
* listbox control. If we have to resize any of those two, the GetParent
* returns the handle to the Combobox, _NOT_ the main dialog of our application.
*****/
void ResizeSubs(HWND hDlg, resize_admin_struct *curr_window, int force)
{
	int nh, nw, oh, ow, width, height, Pwidth, Pheight;
	int ln_cnt, distance, cySize;
	HDC hdc;
	TEXTMETRIC tm;
	int espace = 0;
	RECT ParentRc, Newrc, Oldrc;
	WinInfo *tmp, *WinList;
	HWND parent;

	Newrc = curr_window->thisRect;
	Oldrc = curr_window->prevRect;
	WinList = curr_window->WinList;

	/* current dimensions of root window */
	nh = Newrc.bottom - Newrc.top;
	nw = Newrc.right - Newrc.left;
	/* previous dimensions of root window */
	oh = Oldrc.bottom - Oldrc.top;
	ow = Oldrc.right - Oldrc.left;

	/*
	* return if the resize is less than the treshhold values or if
	* we are not forced to do this resize
	*/
	if(!force && abs(nh-oh) <=V_TRESHHOLD && abs(nw-ow) <=H_TRESHHOLD)
		return;
	/*
	* Else go figure it out.
	* First hide the window. Comment this out to see your dialog break down
	* and build up again.
	*/
#ifndef VIRUS_LIKE
	ShowWindow(hDlg, SW_HIDE);
#endif
	/* 
	* Calculate the extra space provided by windows sticking to the 
	* bottom of a window. Windows sticking to the bottom of a window
	* are truly stick to the bottom of that window: their offset
	* to the bottom is never changed. These windows can only resize
	* upwards or stretch horizontaly. As a result of that, we have some
	* extra vertical space available which we can use to distribute
	* the controls a little bit more even.
	*/
	for (tmp = WinList; tmp != NULL && tmp->StickWhere != STICK_BOTTOM;
		tmp = tmp->next);
	if (tmp != NULL)
		espace = nh - tmp->bottomOffset - (int)(tmp->resize.top*nh);

	/* Walk the WinInfo list and move/resize the items in the list */
	for (tmp = WinList; tmp != NULL; tmp = tmp->next)
	{
		parent = GetParent(tmp->Gadget);
		GetWindowRect(parent, &ParentRc);
		Pwidth = ParentRc.right - ParentRc.left;
		Pheight = ParentRc.bottom - ParentRc.top;

		/* Calculate the new window position */
		if(tmp->GADmove)
		{
			/* left position is always the percentage. */
			if(parent == hDlg)
				tmp->rc.left = (int)(tmp->resize.left*nw);
			else
				tmp->rc.left =(int)(tmp->resize.left*(Pwidth));
			switch(tmp->StickWhere)
			{
				case STICK_TOP:
					tmp->rc.top  = tmp->topOffset;
					break;
				case STICK_BOTTOM:
					tmp->rc.top  = nh - tmp->bottomOffset;
					break;
				default:
					tmp->rc.top  = (int)(tmp->resize.top *
						(parent == hDlg ? 
						(nh + espace):Pheight));
			}
		}
		/* If it is not moveable, use the old positions */
		else
		{
			tmp->rc.left = (int)(tmp->resize.left *ow);
			tmp->rc.top  = (int)(tmp->resize.top *oh);
		}
		/* Calculate new window size */
		switch(tmp->GADresize)
		{
		case ALL_SIZE:
			width = (int)(tmp->resize.pwidth * 
				(parent == hDlg ? nw : Pwidth));
			height = (int)(tmp->resize.pheight *
				(parent == hDlg ? (nh+espace):Pheight));
			break;
		case H_SIZE:
			/* use new width of root window */
			width = (int)(tmp->resize.pwidth * 
				(parent == hDlg ? nw : Pwidth));
			height= tmp->resize.height;
			break;
		case V_SIZE:
			height = (int)(tmp->resize.pheight *
				(parent == hDlg ? (nh+espace):Pheight));
			width = tmp->resize.width;
			break;
		case NO_SIZE:
			width = tmp->rc.right - tmp->rc.left;
			height= tmp->rc.bottom - tmp->rc.top;
			break;
		case H_PARTIAL:
			/* use new width of root window */
			width = (int)(tmp->resize.pwidth * 
				(parent == hDlg ? nw : Pwidth));
			if(parent != hDlg)
			{
				/*
				* Get the distance from the bottom of the child
				* to the bottom of the parent. The size of the 
				* parent is already known (Pheight). The size 
				* of the child has already been calculated 
				* (tmp->rc.top).
				*/
				distance = Pheight  - tmp->rc.top;
				hdc = GetDC(tmp->Gadget);
				GetTextMetrics(hdc, &tm);
				cySize=tm.tmHeight-tm.tmExternalLeading;
				ReleaseDC(tmp->Gadget, hdc);
				for(ln_cnt=0;ln_cnt*cySize<=distance;ln_cnt++)
				{
					tmp->rc.bottom = tmp->rc.top +
						ln_cnt * cySize ;
					height = ln_cnt * cySize;
				}
			}
		}
		/* Store the new right & bottom values */
		tmp->rc.right = tmp->rc.left+width;
		tmp->rc.bottom= tmp->rc.top+height;
		/* Move the gadget to it's new position */
		MoveWindow(tmp->Gadget, tmp->rc.left, tmp->rc.top, width, 
			height, TRUE);
	}
	/* Now show it again */
	ShowWindow(hDlg, SW_SHOW);
	return;
}

/*****
* Resize function for windows without sub-parents
*****/
void ResizeWindow(HWND hDlg, resize_admin_struct *curr_window, int force)
{
	int nh, nw, oh, ow, width, height;
	int espace = 0;
	RECT Newrc, Oldrc;
	WinInfo *tmp, *WinList;

	Newrc = curr_window->thisRect;
	Oldrc = curr_window->prevRect;
	WinList = curr_window->WinList;

	/* current dimensions of root window */
	nh = Newrc.bottom - Newrc.top;
	nw = Newrc.right - Newrc.left;
	/* previous dimensions of root window */
	oh = Oldrc.bottom - Oldrc.top;
	ow = Oldrc.right - Oldrc.left;

	/*
	* return if the resize is less than the treshhold values or if
	* we are not forced to do this resize
	*/
	if(!force && abs(nh-oh) <=V_TRESHHOLD && abs(nw-ow) <=H_TRESHHOLD)
		return;
	/*
	* Else go figure it out.
	* First hide the window. Comment this out to see your dialog break down
	* and build up again.
	*/
	ShowWindow(hDlg, SW_HIDE);

	/* 
	* Calculate the extra space provided by windows sticking to the 
	* bottom of a window. 
	*/
	for (tmp = WinList; tmp != NULL && tmp->StickWhere != STICK_BOTTOM;
		tmp = tmp->next);
	if (tmp != NULL)
		espace = nh - tmp->bottomOffset - (int)(tmp->resize.top*nh);

	/* Walk the WinInfo list and move/resize the items in the list */
	for (tmp = WinList; tmp != NULL; tmp = tmp->next)
	{
		/* Calculate new window position */
		if(tmp->GADmove)
		{
			/* left position is always the percentage. */
			tmp->rc.left = (int)(tmp->resize.left*nw);
			switch(tmp->StickWhere)
			{
				case STICK_TOP:
					tmp->rc.top  = tmp->topOffset;
					break;
				case STICK_BOTTOM:
					tmp->rc.top  = nh - tmp->bottomOffset;
					break;
				default:
					tmp->rc.top  = (int)(tmp->resize.top *
						(nh + espace));
			}
		}
		/* If it is not moveable, use the old positions */
		else
		{
			tmp->rc.left = (int)(tmp->resize.left *ow);
			tmp->rc.top  = (int)(tmp->resize.top *oh);
		}
		/* Calculate new window size */
		switch(tmp->GADresize)
		{
		case ALL_SIZE:
			width = (int)(tmp->resize.pwidth * nw);
			height = (int)(tmp->resize.pheight * (nh+espace));
			break;
		case H_SIZE:
			/* use new width of root window */
			width = (int)(tmp->resize.pwidth * nw);
			height= tmp->resize.height;
			break;
		case V_SIZE:
			height = (int)(tmp->resize.pheight * (nh+espace));
			width = tmp->resize.width;
			break;
		case NO_SIZE:
			width = tmp->rc.right - tmp->rc.left;
			height= tmp->rc.bottom - tmp->rc.top;
			break;
		}
		/* Store the new right & bottom values */
		tmp->rc.right = tmp->rc.left+width;
		tmp->rc.bottom= tmp->rc.top+height;
		/* Move the gadget to it's new position */
		MoveWindow(tmp->Gadget, tmp->rc.left, tmp->rc.top, width, 
			height, TRUE);
	}
	/* Now show it again */
	ShowWindow(hDlg, SW_SHOW);
	return;
}

/*****
* Initialise resize stuff.
* In order to get resizing working, we need a list of all child windows
* in our main dialog window (=the window the user sees).
* We do this by using EnumChildWindows.
* EnumChildWindows walks through all child windows of the given window
* (=hParent) and for each child it finds it calls a procedure (=enumProc)
* with the window handle for this child.
* enumProc stores this handle (together with some other data) in a linked
* list. This list is then used by the resize function.
* validate_wininfo ensures that all child window coordinates are made
* relative to the upper-left corner of our main dialog window.
*****/
void RegisterResize(HWND hParent, HINSTANCE hInst)
{
	WNDENUMPROC enumProc;
	int i;
	resize_admin_struct *curr_window;

	has_subs = FALSE;

	/* 
	* Get the first available record. We do not check if information
	* for the given parent is already in here, it's the callers 
	* responsibility to unregister himself from resize when a parent
	* dialog is closed 
	*/
	for(i = 0 ; i < MAX_RESIZE_WINDOWS && resize_admin[i].busy; i++);

	/* out of resources */
	if(i == MAX_RESIZE_WINDOWS)
	{
		ErrHandler(hParent, R_WARNING, RB_OK1, 
			"RegisterResize Failed: administration table full\n"
			"A maximum of %i windows can be registered.", 
			MAX_RESIZE_WINDOWS);
		return;
	}

	curr_window = &resize_admin[i];
	/* initialise current window information */
	curr_window->busy = TRUE;
	curr_window->WinList = NULL;
	curr_window->owner = hParent;
	GetWindowRect(hParent, &(curr_window->prevRect));

	/* Create procedure address for the enumeration routine. */
	enumProc = (WNDENUMPROC)MakeProcInstance((FARPROC)build_wininfo_list, 
		hInst);
	/* 
	* The current window information pointer is passed as a param to
	* the enumeration process 
	*/
	EnumChildWindows(hParent, enumProc, (LPARAM)(curr_window));

	FreeProcInstance((FARPROC)enumProc);

	/* get some more information on the child window positions */
	validate_wininfo(curr_window, hParent);

	if(has_subs == TRUE)
		curr_window->has_subs = TRUE;
}

/*****
* Resize driver routine. Stores the coordinates of the current window
* and calls the actual resize function. If force = TRUE, always resize
* Else use the threshold values defined in resize.h
*****/
void DoResize(HWND hParent, int force)
{
	int i;
	resize_admin_struct *curr_window;

	for(i=0;i<MAX_RESIZE_WINDOWS && resize_admin[i].owner!=hParent;i++);

	/* parent not found -> RegisterResize not used. */
	if(i == MAX_RESIZE_WINDOWS)
	{
		ErrHandler(hParent, R_WARNING, RB_OK1, 
			"DoResize Failed: window has not been "
			"registered with RegisterResize");
		return;
	}

	curr_window = &resize_admin[i];

	if(!curr_window->busy)
	{
		ErrHandler(hParent, R_WARNING, RB_OK1, 
			"DoResize Failed: window not marked as busy.");
		return;
	}

	GetWindowRect(hParent, &(curr_window->thisRect));
	if(curr_window->has_subs)
		ResizeSubs(hParent, curr_window, force);
	else
		ResizeWindow(hParent, curr_window, force);

	/* save new size for later use. */
	GetWindowRect(hParent, &(curr_window->prevRect));
}

/* unregister this window from resize */
void UnregisterResize(HWND hParent)
{
	WinInfo *tmp, *WinList;
	int i;

	for(i=0;i<MAX_RESIZE_WINDOWS && resize_admin[i].owner!=hParent;i++);

	/* parent not found -> RegisterResize not used. */
	if(i == MAX_RESIZE_WINDOWS)
	{
		ErrHandler(hParent, R_WARNING, RB_OK1, 
			"UnregisterResize Failed: window has not been "
			"registered with RegisterResize");
		return;
	}

	WinList = resize_admin[i].WinList;

	while(WinList != NULL)
	{
		tmp = WinList->next;
		GlobalUnlock(WinList->hglb);
		GlobalFree(WinList->hglb);
		WinList = tmp;
	}
	resize_admin[i].busy = FALSE;		/* mark as free */
	resize_admin[i].has_subs = FALSE;	/* mark as free */
	resize_admin[i].owner = NULL;		/* unattach from parent */
	resize_admin[i].WinList = NULL;		/* invalidate window info */
}
