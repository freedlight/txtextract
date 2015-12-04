
arc:
	backpack textract

it: cvttest.exe
	rm *.bak

cvttest.exe: cvttest.obj cvt.obj
	bcc -ml -v cvttest.obj cvt.obj

cvt.obj: cvt.c
	bcc -c -ml -v cvt.c

cvttest.obj: cvttest.c
	bcc -c -ml -v cvttest.c

