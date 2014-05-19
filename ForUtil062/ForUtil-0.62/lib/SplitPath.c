#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/lib/RCS/SplitPath.c,v $";
#endif
/*****
* SplitPath.c : splits a filename in a fully qualified path and name
*	component.
*
* This file Version	$Revision: 1.7 $
*
* Creation date:	Mon Feb  5 02:44:15 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:19:10 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		Mark Edel with modifications by koen
*****/
/*****
* ChangeLog 
* $Log: SplitPath.c,v $
* Revision 1.7  1996/08/27 19:19:10  koen
* Added the get_filename_part routine
*
* Revision 1.6  1996/08/07 21:17:27  koen
* Corrected a bug in normalizePathname for MSDOS.
*
* Revision 1.5  1996/08/02 14:52:42  koen
* Replaced forLibP.h with forutil.h
*
* Revision 1.4  1996/07/30 02:02:27  koen
* Added the get_filename_part routine.
*
* Revision 1.3  1996/07/16 09:17:53  koen
* corrected copyright
*
* Revision 1.2  1996/05/06 00:36:25  koen
* Adapted for MSDOS: now using generic slashes. Specific code for getting the 
* correct path name under msdos.
*
* Revision 1.1  1996/02/05 01:45:29  koen
* Initial revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <limits.h> 
#include <ctype.h>

#include "forutil.h"

/*****************Private functions prototypes*************************/

static int normalizePathname(char *pathname);
static int compressPathname(char *pathname);
static char *nextSlash(char *ptr);
static char *prevSlash(char *ptr);
static int compareThruSlash(char *string1, char *string2);
static void copyThruSlash(char **toString, char **fromString);

/*
* return only the filename part of a file specification
*/
void
get_filename_part(char *fullname, char *filename)
{
	int fullLen = strlen(fullname);
	int i, pathLen, fileLen;
	/* find the last slash */
    for (i=fullLen-1; i>=0; i--) {
    	if (fullname[i] == SLASH)
	    break;
    }
    /* move chars before / (or ] or :) into pathname,& after into filename */
    pathLen = i + 1;
    fileLen = fullLen - pathLen;
    strncpy(filename, &fullname[pathLen], fileLen);
    filename[fileLen] = 0;
    return;
}

/* 
* Decompose a Unix file name into a file name and a path
* This function originally comes from Nedit. 
* Used with kind permission of Mark Edel (Author of Nedit).  
*/ 

int ParseFilename(char *fullname, char *filename, char *pathname)
{
    int fullLen = strlen(fullname);
    int i, pathLen, fileLen;
    /* find the last slash */
    for (i=fullLen-1; i>=0; i--) {
    	if (fullname[i] == SLASH)
	    break;
    }
    /* move chars before / (or ] or :) into pathname,& after into filename */
    pathLen = i + 1;
    fileLen = fullLen - pathLen;
    strncpy(pathname, fullname, pathLen);
    pathname[pathLen] = 0;
    strncpy(filename, &fullname[pathLen], fileLen);
    filename[fileLen] = 0;
    return(normalizePathname(pathname));
}

static int normalizePathname(char *pathname)
{
    static char oldPathname[MAXPATHLEN], *wd;

    /* if this is a relative pathname, prepend current directory */
#ifndef __MSDOS__
    if (pathname[0] != SLASH)
    { 	/* make a copy of pathname to work from */
		strcpy(oldPathname, pathname);
		/* get the working directory */
		wd = getcwd(NULL, MAXPATHLEN);
		/* prepend it to the path */
		strcpy(pathname, wd);
		strcat(pathname, CHAR_SLASH);
		strcat(pathname, oldPathname);
    }
    /* compress out .. and . */
    return compressPathname(pathname);
#else
	/* relative directories in msdos can start with a . or a drive spec. */
	if (pathname[0] == '.' ||
    	(isascii(pathname[0]) && pathname[1] == ':' && pathname[2] != '\\'))
    {
		/* make a copy of pathname to work from */
		strcpy(oldPathname, pathname);
		/*
		* OK, fuzzy msdos, if the second char is a :, the first two chars
        * specify another drive. To get the current wd on that drive,
        * chdir to that drive, and get the working directory on that drive,
        * else just get the current directory.
		*/
		if(pathname[1] == ':')
		{
			char *curr_dir, *wd2_dir;
			int curr_disk, new_disk;
            /* let wd point right past the [drive:] stuff */
            wd = pathname + 2;
            strcpy(oldPathname, wd);
            wd = NULL;
			/* get current drive & directory */
			curr_dir = getcwd(NULL, MAXPATHLEN);
			curr_disk = getdisk();
			new_disk = toupper(pathname[0]) - 65;
			/* change disk */
			setdisk(new_disk);
			/* get current directory on this drive */
			wd2_dir = getcwd(NULL, MAXPATHLEN);
			/*
			*  set & get current directory. This ensures that a path spec.
			* a la d:..\..\foo will be resolved in a fully qualified path.
			*/
			chdir(pathname);
			wd = getcwd(NULL, MAXPATHLEN);
			/* change back to original dir on this drive */
			chdir(wd2_dir);
			/* change back to original disk & path */
			setdisk(curr_disk);
			chdir(curr_dir);
			/* remove the trailing slash */
			if(wd[strlen(wd)-1] == SLASH)
				wd[strlen(wd)-1] = '\0';
			/* free the chars getcwd returns */
			free(curr_dir);
			free(wd2_dir);
		}
		else
			wd = getcwd(NULL, MAXPATHLEN);
		/* prepend it to the path */
		strcpy(pathname, wd);
		strcat(pathname, CHAR_SLASH);
		strcat(pathname, oldPathname);
    }
    /* compress out .. and . */
    return compressPathname(pathname);
#endif
}


static int compressPathname(char *pathname)
{
    char *inPtr, *outPtr;

    /* compress out . and .. */
    inPtr = &pathname[1];		/* start after initial / */
    outPtr = &pathname[1];
    while (1) {
	/* if the next component is "../", remove previous component */
	if (compareThruSlash(inPtr, ".."CHAR_SLASH)) {
	    /* error if already at beginning of string */
	    if (outPtr == &pathname[1])
	        return 0;
	    /* back up outPtr to remove last path name component */
	    outPtr = prevSlash(outPtr);
	    inPtr = nextSlash(inPtr);
	} else if (compareThruSlash(inPtr, "."CHAR_SLASH)) {
	    /* don't copy the component if it's the redundant "./" */
	    inPtr = nextSlash(inPtr);
	} else {
	    /* copy the component to outPtr */
	    copyThruSlash(&outPtr, &inPtr);
	}
	if (inPtr == NULL) {
	    return 1;
	}
    }
}

static char *nextSlash(char *ptr)
{
    for(; *ptr!=SLASH; ptr++) {
    	if (*ptr == '\0')
	    return NULL;
    }
    return ptr + 1;
}

static char *prevSlash(char *ptr)
{
    for(ptr -= 2; *ptr!=SLASH; ptr--);
    return ptr + 1;
}

static int compareThruSlash(char *string1, char *string2)
{
    while (1) {
    	if (*string1 != *string2)
	    return 0;
	if (*string1 =='\0' || *string1==SLASH)
	    return 1;
	string1++;
	string2++;
    }
}

static void copyThruSlash(char **toString, char **fromString)
{
    char *to = *toString;
    char *from = *fromString;
    
    while (1) {
        *to = *from;
        if (*from =='\0') {
            *fromString = NULL;
            return;
        }
	if (*from==SLASH) {
	    *toString = to + 1;
	    *fromString = from + 1;
	    return;
	}
	from++;
	to++;
    }
}

#ifdef TEST
int main(int argc, char **argv)
{
	static char path[256], file[14];

	fprintf(stderr, "Testing d:\\usr\\bcc\\include\\stdio.h\n");
	ParseFilename("d:\\usr\\bcc\\include\\stdio.h", file,path);
	fprintf(stderr, "path: %s\nfile: %s\n", path, file);

	fprintf(stderr, "Testing ..\\autoconf.h\n");
	ParseFilename("..\\autoconf.h", file,path);
	fprintf(stderr, "path: %s\nfile: %s\n", path, file);

	fprintf(stderr, "Testing d:..\\foo.txt\n");
	ParseFilename("d:..\\foo.txt", file,path);
	fprintf(stderr, "path: %s\nfile: %s\n", path, file);

	fprintf(stderr, "Testing d:..\\..\\foo.txt\n");
	ParseFilename("d:..\\..\\foo.txt", file,path);
	fprintf(stderr, "path: %s\nfile: %s\n", path, file);
	return(0);
}
#endif
