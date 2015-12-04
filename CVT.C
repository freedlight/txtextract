// cvt.c - text conversion utility
// version 2.5 - 12/7/93
// original in MASM by Jay Munro

#include <StdIO.h>
#include <StdLib.h>
#include <String.h>
#include <CType.h>
#ifdef applec
#include <Strings.h>
#include <Files.h>
#include <Memory.h>
#include <Errors.h>
#include <Events.h>
#endif
#include "cvt.h"

// these defines are used in cvthead.h - must come first
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

#include "cvthead.h"

// 0=no buffering, 1=use buffers
#ifdef applec
#define	USERBUFF		1
#else
#define	USERBUFF		0
#endif
#define	OSFRIEND		50	/* how multitask-friendly we are */

//------------------------------------------------------------------------
//		generic utility subroutines
//------------------------------------------------------------------------

void swaplong(DWORD *val)
{
	register BYTE a;
	typedef union {
		DWORD s;
		BYTE b[4];
	} canon_long;
	register canon_long *x = (canon_long *)val;

	a = x->b[0];
	x->b[0] = x->b[3];
	x->b[3] = a;
	a = x->b[1];
	x->b[1] = x->b[2];
	x->b[2] = a;
}

void swapword(WORD *val)
{
	register BYTE a;
	typedef union {
		WORD s;
		BYTE b[2];
	} canon_word;
	register canon_word *x = (canon_word *)val;

	a = x->b[0];
	x->b[0] = x->b[1];
	x->b[1] = a;
}

static char *wrbuf = NULL;
#if USERBUFF
static short wrbufsiz = 32767;
static short wroffs = 0;
#endif

// returns 0 for OK, 1 for error
short PutInBuffer(CVTPARMS *glob, short ch)
{
#ifdef applec
	OSErr err;
	long wrlen;
#endif

	if (glob->testonly)
	{
		glob->outcnt++;
		return 0;
	}
	if (glob->fileout)
	{
#if USERBUFF	//----------------------------------------------
	// allocate a buffer for writing
		while (wrbuf == NULL)
		{
#ifdef __MSDOS__
			wrbuf = malloc(wrbufsiz);
#endif
#ifdef applec
			wrbuf = (char *)NewPtr(wrbufsiz);
#endif
			if (wrbuf == NULL)
			{
				wrbufsiz /= 2;
				if (wrbufsiz < 256)
					return 1;
			}
		}
	// if necessary, empty the buffer
		if (wroffs >= wrbufsiz || ch == -1)
		{
#ifdef __MSDOS__
			if (fwrite(wrbuf, 1, wroffs, glob->fo) <= 0)
#endif
#ifdef applec
			EventRecord ev;

			wrlen = wroffs;
		// non-preemptive multitasking friendly
			WaitNextEvent(0, &ev, OSFRIEND, NULL);
			if ((err = FSWrite(glob->fo, &wrlen, wrbuf)) != noErr)
#endif
				return 1;
			wroffs = 0;
		}
	// write character to next offset ptr
		wrbuf[wroffs++] = ch;
#else			//-------------------------------------------------
		if (ch > 0)
		{
#ifdef __MSDOS__
			putc(ch, glob->fo);
#endif
#ifdef applec
			char wbuf[4];

			wbuf[0] = ch;
			wrlen = 1;
			FSWrite(glob->fo, &wrlen, wbuf);
#endif
		}
#endif		//-------------------------------------------------
	}
	if (ch > 0)
	{
		if (glob->outcnt < glob->maxcnt)
			glob->outp[glob->outcnt++] = ch;
		else
			glob->overflow = 1;
	}
	return 0;
}

short WriteIt(CVTPARMS *glob, short ch)
{
	short err = 0;

// special parsing for tabs
	if (ch == '\t')
	{
		short i;

		if (glob->usetabs)
			err = PutInBuffer(glob, '\t');
		else
			for (i=0; i<glob->tabwidth; i++)
				err = PutInBuffer(glob, ' ');
	}
// special parsing for end-of-line
	else if (ch == '\n')
	{
#ifdef __MSDOS__
		err = PutInBuffer(glob, '\r');
#endif
		err = PutInBuffer(glob, '\n');
	}
	else if (ch == '\r')
#ifdef __MSDOS__
		err = PutInBuffer(glob, '\r');
#else
		;
#endif
	else
		err = PutInBuffer(glob, ch);
	return err;
}

static char *rdbuf = NULL;
#if USERBUFF
static short rdbufsiz = 32767;
static short rdoffs = 32767;
#endif

// return 1 at EOF or error, 0 if OK
#pragma warn -par
short ReadIt(CVTPARMS *glob, BYTE *ch)
{
#ifdef applec
	OSErr err;
#endif

#if USERBUFF		//-------------------------------------------
	long rdlen;

// allocate a buffer for reading
	while (rdbuf == NULL)
	{
#ifdef __MSDOS__
		rdbuf = malloc(rdbufsiz);
#endif
#ifdef applec
		rdbuf = (char *)NewPtr(rdbufsiz);
#endif
		if (rdbuf == NULL)
		{
			rdbufsiz /= 2;
			if (rdbufsiz < 256)
				return 1;
		}
	}
// if necessary, fill the buffer
	if (rdoffs >= rdbufsiz)
	{
#ifdef __MSDOS__
		rdlen = rdbufsiz;
		if ((rdlen = fread(rdbuf, 1, rdlen, glob->fi)) <= 0)
#endif
#ifdef applec
		EventRecord ev;

		rdlen = rdbufsiz;
	// non-preemptive multitasking friendly
		WaitNextEvent(0, &ev, OSFRIEND, NULL);
		err = FSRead(glob->fi, &rdlen, rdbuf);
		if (err == eofErr)
			err = noErr;
		if (err != noErr || rdlen <= 0)
#endif
			return 1;
		if (rdlen < rdbufsiz)
			rdbufsiz = rdlen;
		rdoffs = 0;		// be sure we have a correct offset ptr
	}
// read character from next offset ptr
	*ch = rdbuf[rdoffs++];
#else				//----------------------------------------------
#ifdef __MSDOS__
	*ch = fgetc(glob->fi);
	if (feof(glob->fi))
		return 1;
#endif
#ifdef applec
	char rbuf[4];

	rdlen = 1;
	if ((err = FSRead(glob->fi, &rdlen, rbuf)) != noErr)
		return 1;
	*ch = rbuf[0];
#endif
#endif			//----------------------------------------------
	return 0;
}
#pragma warn .par

void BackRead(CVTPARMS *glob)
{
#if USERBUFF	//----------------------------------------------
	if (rdoffs)
		rdoffs--;
	else
	{
		long rdlen = rdbufsiz;

#ifdef __MSDOS__
		fseek(glob->fi, -rdbufsiz, SEEK_CUR);
		fread(rdbuf, 1, rdlen, glob->fi);
#endif
#ifdef applec
		SetFPos(glob->fi, fsFromMark, -rdbufsiz);
		FSRead(glob->fi, &rdlen, rdbuf);
#endif
		rdoffs = rdbufsiz - 1;
	}
#else				//----------------------------------------------
#ifdef __MSDOS__
	fseek(glob->fi, -1, SEEK_CUR);
#endif
#ifdef applec
	SetFPos(glob->fi, fsFromMark, -1);
#endif
#endif			//----------------------------------------------
}

void SeekData(CVTPARMS *glob, long pos)
{
#ifdef __MSDOS__
	fseek(glob->fi, pos, SEEK_SET);
#endif
#ifdef applec
	SetFPos(glob->fi, fsFromStart, pos);
#endif
}

//------------------------------------------------------------------------
//		conversion subroutines
//------------------------------------------------------------------------

// rules -
//		only new lines are passed
//		carriage returns are ignored
//		tabs may be expanded to spaces, depending on config setting

void StdConvert(CVTPARMS *glob, BYTE ch)
{
	if (ch == '\n' || ch == '\t')
		WriteIt(glob, ch);
#ifdef __MSDOS__
	else if (ch == '\r')
		WriteIt(glob, ch);
#endif
	else if (ch < 0x20 || ch > 0xFE)
		;
	else
		WriteIt(glob, ch);
}

void MSWordConvert(CVTPARMS *glob, BYTE ch)
{
	if (ch == 11)		// hard return
		WriteIt(glob, '\n');
	else if (ch == 15)		// em dash?
		WriteIt(glob, '-');
	else if (ch == 2)			// print current date
		;
	else if (ch == 3)			// print current time
		;
	else
		StdConvert(glob, ch);
}

// WordPerfect 5.1 conversion routine
short ConvertWordPerf(CVTPARMS *glob)
{
	BYTE ch, chx;
	WPerf51 *head = (WPerf51 *)glob->hbuf;

#ifdef applec
	swaplong((DWORD *)&(head->docstart));
#endif
	SeekData(glob, head->docstart);		// go to start of text
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			break;
		if (ch == 0xA9 || ch == 0xAA || ch == 0xAB)	// hard hyphen
			WriteIt(glob, '-');
		else if (ch == 0xA0)		// hard space
			WriteIt(glob, ' ');
		else if (ch == 0xC1)		// code for tabs
		{
			while (1)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if ((ch & 0x60) == 0)
					WriteIt(glob, '\t');
				if (ch == 0xC1)
					break;
			}
		}
		else if (ch >= 0xC0 && ch <= 0xCF)	// single byte command?
		{
			while (1)
			{
				if (ReadIt(glob, &chx))
					return ERR_UnexpectedEOF;
				if (chx == ch)
					break;
			}
		}
		else if (ch >= 0xD0)		// multibyte command?
		{
			short cnt;

			if (ch == 0xDC)			// column ?
			{
				if (ReadIt(glob, &chx))
					return ERR_UnexpectedEOF;
				if (chx == 0)					// service 0?
					WriteIt(glob, '\n');
			}
			else if (ch == 0xD4)	// soft or hard page ?
			{
				if (ReadIt(glob, &chx))
					return ERR_UnexpectedEOF;
				if (chx == 0)					// service 0?
					WriteIt(glob, 12);			// page feed
			}
			if (ReadIt(glob, &ch))
				return ERR_UnexpectedEOF;
			cnt = ch + 2;
			while (--cnt)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
			}
		}
		else if (ch == '\r')
			WriteIt(glob, '\n');
		else
			StdConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// WordPerfect Mac conversion routine
short ConvertWPMac(CVTPARMS *glob)
{
	BYTE ch, chx;
	WPerf51 *head = (WPerf51 *)glob->hbuf;

#ifdef applec
	swaplong((DWORD *)&(head->docstart));
#endif
	SeekData(glob, head->docstart);		// go to start of text
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			break;
		if (ch == 0x96)	// hard hyphen
			WriteIt(glob, '-');
		else if (ch == 0xA0)
			WriteIt(glob, ' ');
		else if (ch == 0xC1)		// code for tabs
		{
			while (1)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if ((ch & 0x60) == 0)
					WriteIt(glob, '\t');
				if (ch == 0xC1)
					break;
			}
		}
		else if (ch >= 0xC0 && ch <= 0xE0)	// single byte command?
		{
			if (ch == 0xDC)	// soft return ?
				WriteIt(glob, '\n');
			while (1)
			{
				if (ReadIt(glob, &chx))
					return ERR_UnexpectedEOF;
				if (chx == ch)
					break;
			}
		}
		else if (ch == '\r')
			WriteIt(glob, '\n');
		else
			StdConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// Word for Dos conversion routine
short ConvertWordDos(CVTPARMS *glob)
{
#define WORDDOS_TEXTBEG	128
	BYTE ch;
	long ch_left;
	MSWordDOS *head = (MSWordDOS *)glob->hbuf;

#ifdef applec
	swaplong((DWORD *)&(head->textend));
#endif
	ch_left = head->textend - WORDDOS_TEXTBEG;
	SeekData(glob, WORDDOS_TEXTBEG);		// skip header
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (--ch_left == 0L)
			break;
		MSWordConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// Word for Windows conversion routine
short ConvertWordWin(CVTPARMS *glob)
{
	BYTE ch;
	long ch_left;
	MSWordWin *head = (MSWordWin *)glob->hbuf;

#ifdef applec
	swaplong((DWORD *)&(head->textbeg));
	swaplong((DWORD *)&(head->textend));
#endif
	ch_left = head->textend - head->textbeg;
	SeekData(glob, head->textbeg);		// seek to start of file
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (--ch_left == 0L)
			break;
		if (ch == 0x92)			// apostrophe
			WriteIt(glob, '\'');
		else if (ch == 0x97)		// em dash
			WriteIt(glob, '-');
		else if (ch == 147 || ch == 148)	// quotes
			WriteIt(glob, '"');
		else if (ch == 7)			// columns
			;
		else if (ch == 19)		// start of field
		{
			short fldcnt = 1;
	
			do {
				ch_left--;
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if (ch == 21)
					fldcnt--;
				if (ch == 19)
					fldcnt++;
			} while (fldcnt);
		}
		else
			MSWordConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// Write conversion routine
short ConvertMSWrite(CVTPARMS *glob)
{
#define MSWRITE_TEXTBEG	128
	BYTE ch;
	long ch_left;
	MSWordDOS *head = (MSWordDOS *)glob->hbuf;

#ifdef applec
	swaplong((DWORD *)&(head->textend));
#endif
	ch_left = head->textend - MSWRITE_TEXTBEG;
	SeekData(glob, MSWRITE_TEXTBEG);	// skip header
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (--ch_left <= 0L)
			break;
		if (ch == 0x92)			// apostrophe
			WriteIt(glob, '\'');
		else if (ch == 0x97)		// em dash?
			WriteIt(glob, '-');
		else
			MSWordConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// ASCII conversion (copy) routine
short ConvertAscii(CVTPARMS *glob)
{
	BYTE ch;

	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			break;
		StdConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// Word for Mac 3.x conversion routine
short ConvertWordMac3(CVTPARMS *glob)
{
	BYTE ch;
	long ch_left;
	MSWordMac3 *head = (MSWordMac3 *)glob->hbuf;

#ifdef __MSDOS__
	swaplong((DWORD *)&(head->textbeg));
	swaplong((DWORD *)&(head->textend));
#endif
	ch_left = head->textend - head->textbeg;
	SeekData(glob, head->textbeg);		// seek to start of file
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (--ch_left == 0L)
			break;
		MSWordConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// Word for Mac 4.x conversion routine
short ConvertWordMac4(CVTPARMS *glob)
{
	BYTE ch;
	long ch_left;
	MSWordMac4 *head = (MSWordMac4 *)glob->hbuf;

#ifdef __MSDOS__
	swaplong((DWORD *)&(head->textbeg));
	swaplong((DWORD *)&(head->textend));
#endif
	ch_left = head->textend - head->textbeg;
	SeekData(glob, head->textbeg);		// seek to start of file
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (--ch_left == 0L)
			break;
		MSWordConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// scan for a char
// returns 1 if found, 0 on EOF
short RTFscan(CVTPARMS *glob, BYTE chs)
{
	BYTE ch;

	while (1)
	{
		if (ReadIt(glob, &ch))
			return 0;
		if (ch == chs)
		{
			BackRead(glob);
			break;
		}
	}
	return 1;
}

// RTF conversion routine
short ConvertRTF(CVTPARMS *glob)
{
	BYTE ch;
	short level, cmdcnt;
	char cmd[32];

// search for start-of-text tag
	SeekData(glob, 6);			// skip ID bytes
	if (!RTFscan(glob, '{'))
		return ERR_TextNotFound;
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			break;
		if (ch == '\r')			// most <0D>s in file are bogus
			;
		else if (ch == '{')		// skip command sections between { }
		{
			level = 1;
			while (level)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if (ch == '{')
					level++;
				if (ch == '}')
					level--;
			}
		}
		else if (ch == '\\')
		{
			cmdcnt = 0;
		// scan through command until terminator
		// we don't care about any numerics, so just parse them as
		// part of the command
			while (1)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if (ch == ' ')
					break;
				if (ch == '\\' || ch == '{')
				{
					BackRead(glob);
					break;
				}
				cmd[cmdcnt++] = ch;
			}
			cmd[cmdcnt] = 0;
		// see if the command is one we care about
		// if not, ignore it
			if (cmd[0] == '\\')
				WriteIt(glob, '\\');
			else if (cmd[0] == '{')
				WriteIt(glob, '{');
			else if (cmd[0] == '-' || cmd[0] == '_')
				WriteIt(glob, '-');
			else if (cmd[0] == '~')
				WriteIt(glob, ' ');
			else if (strcmp(cmd, "par") == 0)
				WriteIt(glob, '\n');
			else if (strcmp(cmd, "sect") == 0)
				WriteIt(glob, '\n');
			else if (strcmp(cmd, "page") == 0)
				WriteIt(glob, '\n');
			else if (strcmp(cmd, "line") == 0)
				WriteIt(glob, '\n');
			else if (strcmp(cmd, "column") == 0)
				WriteIt(glob, '\n');
			else if (strcmp(cmd, "tab") == 0)
				WriteIt(glob, '\t');
		}
		else if (ch == '}')
		{
			if (!RTFscan(glob, '{'))
				break;
		}
		else
			WriteIt(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

// AMI Pro conversion routine
short ConvertAmiPro(CVTPARMS *glob)
{
	BYTE ch;
	short phase = 0;
#define AMIEDOC   "[edoc]"

// search for start-of-text tag
	while (1)
	{
		if (ReadIt(glob, &ch))
			return ERR_TextNotFound;
		if (ch == AMIEDOC[phase])
		{
			phase++;
			if (phase >= strlen(AMIEDOC))
				break;
		}
		else
			phase = 0;
	}
	phase = 0;
	while (1)
	{
		if (!glob->fileout && glob->overflow)
			break;
		if (ReadIt(glob, &ch))
			return ERR_UnexpectedEOF;
		if (ch == '\r')				// filter spurious CR/LFs
		{
			if (phase == 1)
				WriteIt(glob, '\n');
			else
				phase = 1;
			ReadIt(glob, &ch);
			if (ch != '\n')
				BackRead(glob);
			continue;
		}
		phase = 0;
		if (ch == '>')					// terminating > for text section
			break;
		else if (ch == '<')				// flush commands between <>
		{
			ReadIt(glob, &ch);
			if (ch == '<')				// << is really just <
				WriteIt(glob, ch);
			else if (ch == '[')			// <[ is really just [
				WriteIt(glob, ch);
			else if (ch == ';')			// <;x is really just x
			{
				ReadIt(glob, &ch);
				WriteIt(glob, ch);
			}
			while (1)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if (ch == '>')
					break;
			}
		}
		else if (ch == '@')			// some commands are between @@
		{
			while (1)
			{
				if (ReadIt(glob, &ch))
					return ERR_UnexpectedEOF;
				if (ch == '@')
					break;
			}
		}
		else
			StdConvert(glob, ch);
	}
	if (WriteIt(glob, -1))
		return ERR_OutputErr;
	return ERR_None;
}

//------------------------------------------------------------------------
//		parsing subroutines
//------------------------------------------------------------------------

// ASCII check consists of checking for non-printing chars
// in header (1st 256 bytes)
// fix 2.4 - only check headlen, in case file is < 256 bytes
//				only check up to headlen-1, in case hbuf[headlen] == 0x1A
short AsciiCheck(BYTE *hbuf, short headlen)
{
	short i;

	for (i=0; i<headlen-1; i++)
	{
		if (hbuf[i] == '\t' ||
			 hbuf[i] == '\r' ||
			 hbuf[i] == '\n')
			 	continue;
		if (hbuf[i] < ' ')
			return 0;
	}
	return 1;
}

typedef struct {
	short type;
	short len;
	unsigned char *code;
} TYPEINFO;

TYPEINFO t[] = {
	{ CVT_WP51,		4, "\xFF\x57\x50\x43" },
	{ CVT_WORDDOS,	2, "\x31\xBE" },
	{ CVT_WORDWIN,	2, "\xDB\xA5" },
	{ CVT_WORDWIN,	2, "\x9B\xA5" },
	{ CVT_WORDMAC3,	2, "\xFE\x34" },
	{ CVT_WORDMAC4,	2, "\xFE\x37" },
	{ CVT_AMIPRO,	5, "[ver]" },
	{ CVT_RTF,		6, "{\\rtf1" },
	{ CVT_RFT,		5, "\x00\x05\xE1\x03\x00" },
	{ CVT_FFT,		2, "\x2B\xD2" },
	{ CVT_DW4,		9, "\x80\x00\x00\x09\x20\x00\x4F\x7B\x4A" },
	{ CVT_POSTSCR,	4, "%!PS" },
};
#define MAXTYPE		(sizeof(t)/sizeof(t[0]))

// identifies file by header
short HeadCheck(char *hbuf, short headlen)
{
	short i;

	for (i=0; i<MAXTYPE; i++)
		if (memcmp(hbuf, t[i].code, t[i].len) == 0)
		{
			if (t[i].type == CVT_WORDDOS)
			{
				if (((MSWordDOS *)hbuf)->writeid == 0)
					return CVT_WORDDOS;
				return CVT_MSWRITE;
			}
			if (t[i].type == CVT_WP51)
			{
				if (((WPerf51 *)hbuf)->fieldtype == 0x2C)
					return CVT_WPMAC;
				if (((WPerf51 *)hbuf)->fieldtype == 0x0A)
					return CVT_WP51;
			// unknown - guess
				return CVT_WP51;
			}
			return t[i].type;
		}
	if (AsciiCheck((BYTE *)hbuf, headlen))
		return CVT_ASCII;
	return -1;
}

short ConvertText(CVTPARMS *glob)
{
	short err, headlen;
#if applec
	long flen;
#endif

#if __MSDOS__
	if ((glob->fi = fopen(glob->iname, "rb")) == 0)
#endif
#if applec
	if (FSpOpenDF(&glob->iname, fsRdPerm, &glob->fi) != noErr)
#endif
		return ERR_NoInput;
	glob->overflow = 0;
	glob->outcnt = 0;
#if __MSDOS__
	headlen = fread(glob->hbuf, 1, HEADSIZE, glob->fi);
#endif
#if applec
	flen = HEADSIZE;
	FSRead(glob->fi, &flen, glob->hbuf);
	headlen = flen;
#endif
	SeekData(glob, 0);
	if ((glob->ftype = HeadCheck(glob->hbuf, headlen)) == -1)
		return ERR_UnknownType;
	if (glob->fileout)
	{
#ifdef __MSDOS__
		char *p;

		strcpy(glob->oname, glob->iname);
		if ((p = strchr(glob->oname, '.')) != 0L)
			*p = 0;
		strcat(glob->oname, ".CVT");
		if ((glob->fo = fopen(glob->oname, "wt")) == 0)
#endif
#ifdef applec
		char *p, *nam;

		glob->oname = glob->iname;
		nam = &(glob->oname.name);
		p2cstr(nam);
		if ((p = strchr(nam, '.')) != 0L)
			*p = 0;
		strcat(nam, ".CVT");
		c2pstr(nam);
		FSpDelete(&glob->oname);
		if (FSpCreate(&glob->oname, 'MPS ', 'TEXT', -1) != noErr)
			return ERR_OutputErr;
		if (FSpOpenDF(&glob->oname, fsWrPerm, &glob->fo) != noErr)
#endif
			return ERR_OutputErr;
	}
	switch (glob->ftype)
	{
		case CVT_POSTSCR:
		case CVT_ASCII:
			err = ConvertAscii(glob);
			break;
		case CVT_WP51:
			err = ConvertWordPerf(glob);
			break;
		case CVT_WPMAC:
			err = ConvertWPMac(glob);
			break;
		case CVT_WORDDOS:
			err = ConvertWordDos(glob);
			break;
		case CVT_WORDWIN:
			err = ConvertWordWin(glob);
			break;
		case CVT_AMIPRO:
			err = ConvertAmiPro(glob);
			break;
		case CVT_MSWRITE:
			err = ConvertMSWrite(glob);
			break;
		case CVT_WORDMAC3:
			err = ConvertWordMac3(glob);
			break;
		case CVT_WORDMAC4:
			err = ConvertWordMac4(glob);
			break;
		case CVT_RTF:
			err = ConvertRTF(glob);
			break;
		case CVT_RFT:
		case CVT_FFT:
		case CVT_DW4:
			err = ERR_UnsupportedType;
			break;
	}
#ifdef __MSDOS__
	fclose(glob->fi);
	free(rdbuf);
	free(wrbuf);
#endif
#ifdef applec
	FSClose(glob->fi);
	DisposePtr((Ptr)rdbuf);
#endif
	rdbuf = NULL;
	if (glob->fileout)
	{
#if __MSDOS__
		putc(0x1A, glob->fo);		// DOS EOF mark
		if (fclose(glob->fo) != 0)
#endif
#ifdef applec
		if (FSClose(glob->fo) != noErr)
#endif
			return ERR_OutputErr;
	}
	return err;
}
