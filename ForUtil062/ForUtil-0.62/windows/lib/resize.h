/*****
* resize.h: public header file for resize.c
*
* Hicks Version 	1.00
* This file Version	$Revision: 2.2 $
*
* Creation date:	07-06-1995
* Last modification: 	$Date: 1996/08/27 19:24:23 $
*
* Author:		Koen
* (C)1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/winutil/RCS/resize.h,v 2.2 1996/08/27 19:24:23 koen Exp koen $
*****/
/*****
* ChangeLog 
* $Log: resize.h,v $
* Revision 2.2  1996/08/27 19:24:23  koen
* Two new routine prototypes to support multiple window management.
*
* Revision 2.1  1996/05/23  16:25:19  koen
* new rcs default branch.
*
* Revision 1.7  1996/05/10  08:04:51  koen
* changed rcs header.
*
* Revision 1.6  1996/04/19  16:38:11  koen
* ?
*
* Revision 1.5  1995/08/17  22:09:09  koen
* added rcs Log directive
*
*****/ 

#ifndef _resize_h_
#define _resize_h_

/*****
* The routines in this header file provide a generic resizing feature to all 
* of your application windows. 
*
* Usage of these routines is given in the example listed below:
*
*	case WM_INITDIALOG:
*		RegisterResize(hDlg, hInst); [Register with resize]
*		...			[or call after a CreateWindow/Dialog]
*		return TRUE;
*	....
*	case WM_SIZE:
*		switch(wParam)
*		{
* 				[Call DoResize when window is maximized]
*			case SIZE_MAXIMIZED:	
* 				[Call DoResize when window is resized]
*			case SIZE_RESTORED:		
*				DoResize(hDlg, FALSE);
*				return(TRUE);
*			default:
*				break;
*		}
*		return FALSE;
*	...
*	case WM_CLOSE:
*		UnregisterResize(hDlg);	[Unregister from resize] 
*		DestroyWindow(hDlg);
*		return TRUE;
*
*****/

/*
* Treshold values for resize in display values 
* A resize is only done when the difference between the previous window
* size and the new window size is more than V_TRESHHOLD in vertical direction
* or more than H_TRESHHOLD in horizontal direction. This can be overridden 
* by forcing a resize with DoResize (see below).
*/
#define V_TRESHHOLD	10	/* minimum vertical resize change	*/
#define H_TRESHHOLD	10	/* minimum horizontal resize change	*/
#define STICK_TRESHOLD	5	/* maximum bottom/top stick deviation	*/

/* 
* Default support for up to 5 windows. If your application will use more than
* 5 windows with resize, increase this value to an appropriate number.
* Resize uses a lineair lookup to determine the first available slot or match
* in it's own administration records, so don't make this number too big.
*/
#define MAX_RESIZE_WINDOWS	5

/* Register a window with resize. */
extern void RegisterResize(HWND hParent, HINSTANCE hInst);

/* 
* Resize driver routine. When force == TRUE, resize will be force to do
* a resize, even if the size of the window hasn't been changed.
*/
extern void DoResize(HWND hParent, int force);

/* 
* Unregister a given window from resize. It is imperative that you
* use this routine, especially in a dialogbox routine which will be
* called a number of times. If you do not call this routine, resize will
* give up at a certain time telling you that it's internal administration
* table is full.
*/
void UnregisterResize(HWND hParent);

/* Don't add anything after this endif! */
#endif /* _resize_h_ */
