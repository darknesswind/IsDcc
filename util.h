/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/



#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <assert.h>

#ifdef MSC
typedef unsigned int uint;
typedef unsigned short ushort;
#endif
typedef unsigned char uchar;

extern void printIndent(int value);
extern char* filterLF(char* buffer);
extern uint get4Byte(int file);
extern ushort get2Byte(int file);
extern uchar get1Byte(int file);
extern char* getString(int file, char* buffer, int length);
extern char* getLength(int file, char* buffer, int length);
extern void error(char* fmt, ...);
extern void warning(char* fmt, ...);
extern char* escapeString(char* buffer);
extern void* mallocX(long size);
extern int readX(int fd, char* buffer, int len);

#endif

