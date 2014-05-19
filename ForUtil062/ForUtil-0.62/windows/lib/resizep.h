/*****
* resizeP.h: private header file for resize.c
*
* Hicks Version         1.00
* This file Version     $Revision: 2.2 $
*
* Creation date:        07-06-1995
* Last modification:    $Date: 1996/08/27 19:24:49 $
*
* Author:               Koen
* (C)1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/winutil/RCS/resizep.h,v 2.2 1996/08/27 19:24:49 koen Exp koen $
*****/
/*****
* ChangeLog 
* $Log: resizep.h,v $
* Revision 2.2  1996/08/27 19:24:49  koen
* Added admin struct for multiple window management.
*
* Revision 2.1  1996/05/23  16:25:19  koen
* new default rcs branch
*
* Revision 1.6  1996/05/10  08:05:32  koen
* changed rcs header.
*
* Revision 1.5  1996/04/19  16:38:13  koen
* ?
*
* Revision 1.4  1995/10/18  01:27:22  koen
* ?
*
* Revision 1.3  1995/08/17  22:09:17  koen
* added rcs Log directive
*
*****/ 

#ifndef _resize_p_h_
#define _resize_p_h_

#include "windows\lib\resize.h"

/* Resize defines */
#define NO_SIZE		-1
#define V_SIZE		2
#define H_SIZE		4
#define ALL_SIZE	8
#define H_PARTIAL	16

/* Stick defines */
#define STICK_NONE	-1
#define STICK_LEFT	2
#define STICK_RIGHT	4
#define STICK_TOP	8
#define STICK_BOTTOM	16

/***** 
* The most important datatype in this file: contains resizing information
* about a window.
*****/
typedef struct tagRESIZE{
	float pheight;	/* height of gadget in % of root window heigth	*/
	float pwidth;	/* width of gadget in % of root window width	*/
	int height;	/* height of gadget				*/
	int width;	/* width of gadget				*/
	float left;	/* x-coordinate percentage upper-left corner	*/
	float top;	/* y-coordinate percentage upper-left corner	*/
	float right;	/* x-coordinate percentage lower-right corner	*/
	float bottom;	/* y-coordinate percentage lower-right corner	*/
}RESIZE;

/* Struct containing info for a control */
typedef struct _WinInfo{
	HGLOBAL hglb;		/* memory handle for this gadget	*/
	int Id;			/* Id for this gadget			*/
	HWND Gadget;		/* window handle for this gadget	*/
	RECT rc;		/* size info for this gadget		*/
	int GADresize;		/* is resizable(V_SIZE,H_SIZE,ALL_SIZE)?*/
	BOOL GADmove;		/* is moveable?				*/
	int StickWhere;		/* Where to stick the gadget		*/
	int topOffset;		/* Offset for Sticktop gadgets		*/
	int bottomOffset;	/* Offset for Stickbottom gadgets	*/
	RESIZE resize;		/* Relative % to root window dimensions	*/
	struct _WinInfo *next;  /* next item in list			*/
}WinInfo;

/*****
* This datatype allows resize to manage a number of windows, each with it's
* own resize information.
*****/
typedef struct _resize_admin_struct{
	HWND owner;		/* owning window */
	WinInfo *WinList;	/* resize information for this window */
	RECT thisRect;		/* current size */
	RECT prevRect;		/* previous size */
	BOOL busy;		/* true when this record is used */
	BOOL has_subs;		/* true if this HWND has sub-parents */
}resize_admin_struct;

/* Don't add anything after this endif! */
#endif /* _resize_p_h_ */
