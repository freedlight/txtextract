// header structures for text convert utility

typedef struct {
	long	idcode;			// ID code
	long	docstart;		// offset to start of document
	char	prodtype;		// product type
	char	fieldtype;		// product specific variable
	char	majorver;		// major of product
	char	minorver;		// minor version of product (0=WP5.0, 1=5.1)
	short	encrypt;			// 0 = not encrypted
	short	spare;			// usually 0
} WPerf51;

typedef struct {
	WORD	idcode;			// ID code - 0xBE31
	short	doctype;			// Document type
	WORD	toolid;			// ID code - 0xAB00
	short	incrsave;		// Pointer to incremental save
	short	charfmt;			// Pointer to Char formatting
	short	piecetbl;		// Pointer to piece table
	short	cres;
	long	textend;			// File char pos of file end
	short	parafmt;			// Location of para formatting
	short	foottbl;			// Location of footnote table
	short	bookmk;			// Bookmarks and sequence info
	short	secttbl;			// Section table
	short	bufftbl;			// Buffer table
	short	summary;			// Summary information
	char	stylpath[66];	// Style sheet path
	short	writeid;			// For Windows Write use
	char	printer[8];		// Printer name
	short	pastend;			// One past end of document
	char	docprop[8];		// Document properties
	char	version;			// word version
	char	autosave;
	short	pagetbl;			// Word 5.0 page table
	short	bookmac;			// Mac of Bkmk stuff
	short	filename;		// Only used for Asv files
	short	headtbl;			// Running head table
	short	codepage;
} MSWordDOS;

typedef struct {
	WORD	idcode;			// ID code - 0xDBA5
	short	fibver;			// fib version written
	short	prodver;
	short	lid;
	short	next;
	short	complex;
	short	encrypt;
	long	fibback;
	short	spare[3];		// reserved
	long	textbeg;			// offset to start of text
	long	textend;			// offset to end of text
	// ... more stuff
} MSWordWin;

typedef struct {
	WORD	idcode;			// ID code - 0x34FE
	short unk1[2];
	long  unk2[2];
	long	textend;			// offset to end of text   (Moto format)
	long  unk3;
	short unk4;
	long	textbeg;			// offset to start of text (Moto format)
	// ... more stuff
} MSWordMac3;

typedef struct {
	WORD	idcode;			// ID code - 0x37FE
	short unk1[2];
	long  unk2[3];
	short unk3;
	long	textbeg;			// offset to start of text (Moto format)
	long	textend;			// offset to end of text   (Moto format)
	// ... more stuff
} MSWordMac4;
