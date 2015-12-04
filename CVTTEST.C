// cvttest.c - text conversion utility - test program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <time.h>
#include "cvt.h"

void error(char *s)
{
	printf("%s\n", s);
	exit(3);
}

#define MAXBUFFER			32767

void main(short argc, char *argv[])
{
	char *p, *arg;
	short err;
	CVTPARMS glob;
	clock_t tbef, taft;
	char oname[64];			// output file name

	if (argc < 2)
		exit(1);
// init globals
	glob.iname[0] = oname[0] = 0;
	glob.usetabs = 0;
	glob.tabwidth = 1;
	glob.testonly = 0;
	glob.fileout = 0;
// parse command line
	while (--argc)
	{
		arg = *(++argv);
		if (arg[0] == '-' || arg[0] == '/')
		{
			switch (tolower(arg[1]))
			{
				case 'f':		// output file ?
					glob.fileout = 1;
					break;
				case 't':		// tab expand ?
					glob.tabwidth = atoi(arg+2);
					if (glob.tabwidth == 0)
						glob.usetabs = 1;
					break;
				case 'y':		// test only ?
					glob.testonly = 1;
					break;
			}
		}
		else
			strcpy(glob.iname, arg);
	}
// handle blank file names
	if (glob.iname[0] == 0)
		error("You must specify a source file");
	if (!glob.fileout && oname[0] == 0)
	{
		strcpy(oname, glob.iname);
		if ((p = strrchr(oname, '.')) != 0)
			*p = 0;
		strcat(oname, ".TXT");
	}
	if ((glob.outp = malloc(MAXBUFFER)) == 0)
		error("No memory for buffer");
	glob.maxcnt = MAXBUFFER;
	tbef = clock();
	err = ConvertText(&glob);
	taft = clock();
	switch (glob.ftype)
	{
		case CVT_ASCII:
			printf("ASCII - ");
			break;
		case CVT_WP51:
			printf("WordPerfect 5.1 - ");
			break;
		case CVT_WPMAC:
			printf("WordPerfect Mac - ");
			break;
		case CVT_MSWRITE:
			printf("Windows Write - ");
			break;
		case CVT_WORDDOS:
			printf("Word For DOS - ");
			break;
		case CVT_WORDWIN:
			printf("Word For Windows - ");
			break;
		case CVT_WORDMAC3:
			printf("Word For Mac 3.x - ");
			break;
		case CVT_WORDMAC4:
			printf("Word For Mac 4.x/5.x - ");
			break;
		case CVT_AMIPRO:
			printf("AMI Pro - ");
			break;
		case CVT_RTF:
			printf("RTF 1.0 - ");
			break;
		case CVT_RFT:
			printf("IBM DCA/RFT - ");
			break;
		case CVT_FFT:
			printf("IBM FFT - ");
			break;
		case CVT_DW4:
			printf("DisplayWrite - ");
			break;
		case CVT_POSTSCR:
			printf("PostScript - ");
			break;
	}
	switch (err)
	{
	// error returns
		case ERR_None:
			break;
		case ERR_NoInput:
			error("cannot open input file");
		case ERR_UnknownType:
			error("Sorry, Unknown format");
		case ERR_UnsupportedType:
			error("Not yet supported");
		case ERR_TextNotFound:
			error("Unable to locate text in file");
		case ERR_UnexpectedEOF:
			error("Unexpected end of file");
	}
// check if truncated
	if (glob.overflow)
	{
		if (glob.fileout)
			printf("Lots of chars converted\n");
		else
			printf("file truncated\n");
	}
	else
		printf("%d chars converted\n", glob.outcnt);
	printf("Took %ld ticks (%ld secs)\n", taft - tbef,
		((taft - tbef) * 10 / 182));
	if (!glob.testonly && !glob.fileout)
	{
		FILE *fo;					// output file
		
		if ((fo = fopen(oname, "wt")) == 0)
			error("cannot open output file");
		fwrite(glob.outp, 1, glob.outcnt, fo);
		if (fclose(fo) != 0)
			error("Error closing output file - disk may be full");
	// delete output file on error
		else if (err != ERR_None)
			unlink(oname);
	}
	exit(0);
}

