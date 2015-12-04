// cvt.h - include file for text conversion utility
// version 2.3 - 08/23/93

// structure sizes
#define HEADSIZE			256

// structure for global data
typedef struct {
#ifdef __MSDOS__
	FILE *fi;					// (INTERNAL USE ONLY) input file
	FILE *fo;					// (INTERNAL USE ONLY) output file, if fileout=1
#endif
#ifdef applec
	short fi;					// (INTERNAL USE ONLY) input file
	short fo;					// (INTERNAL USE ONLY) output file, if fileout=1
#endif
	char *outp;					// buffer pointer
	short ftype;				// (RETURNED) file type
	short outcnt;				// (RETURNED) # of chars in output buffer
	short maxcnt;				// size of buffer
	short overflow;				// (RETURNED) 1=more text than buffer
	short usetabs;				// 1=use tab char, 0=substitute spaces
	short tabwidth;				// # of spaces to substitute for tabs
	short testonly;				// 1=no conversion performed
	short fileout;				// 1=output to text file
#ifdef __MSDOS__
	char iname[64];				// input file name
	char oname[64];				// (RETURNED) output file name (if fileout=1)
#endif
#ifdef applec
	FSSpec iname;				// input file name
	FSSpec oname;				// (RETURNED) output file name (if fileout=1)
#endif
	char hbuf[HEADSIZE];		// (INTERNAL USE ONLY) buffer for header
} CVTPARMS;

// error codes
enum {
	ERR_None,					// no error
	ERR_NoInput,				// input file missing
	ERR_UnknownType,			// can't identify file type
	ERR_UnsupportedType,		// can identify type, but can't convert it
	ERR_TextNotFound,			// can't find text in file
	ERR_UnexpectedEOF,			// Unexpected EOF during text processing
	ERR_OutputErr				// error writing output file (if fileout=1)
};

// file type IDs
enum {
	CVT_ASCII,					// straight ASCII file
	CVT_WP51,					// WordPerfect 5.1
	CVT_WPMAC,					// WordPerfect Mac
	CVT_MSWRITE,				// MS Write - Windows
	CVT_WORDDOS,				// MS Word for DOS
	CVT_WORDWIN,				// MS Word for Windows 1.0 or 2.0
	CVT_WORDMAC3,				// MS Word for Macintosh 3.x
	CVT_WORDMAC4,				// MS Word for Macintosh 4.x or 5.x
	CVT_AMIPRO,					// Lotus Ami Pro 2.0
	CVT_RTF,					// RTF
	CVT_RFT,					// DCA-RFT
	CVT_FFT,					// DCA-FFT
	CVT_DW4,					// DisplayWrite4
	CVT_POSTSCR					// PostScript
};

// function prototype
short ConvertText(CVTPARMS *glob);

