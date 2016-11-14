/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#ifndef ISHIELD_H
#define ISHIELD_H



#define ISHIELD_MAGIC 0x000cc9b8
#define ISHIELD_MAGIC_v2_20_905 0x1234001c
#define ISHIELD_MAGIC_v3_00_065 0x12340016
#define ISHIELD_MAGIC_v5_00_000 0x00010050



#define ISHIELD2_CHAR 0x00050001
#define ISHIELD2_SHORT 0x00050002
#define ISHIELD2_INT 0x00050003
#define ISHIELD2_LONG 0x00050004

#define ISHIELD2_BOOL 0x00050006
#define ISHIELD2_HWND 0x00050007

#define ISHIELD2_STRING 0x00040000



#define ISHIELD3_SHORT 0x00050002
#define ISHIELD3_HWND_INT2 0x00050003
#define ISHIELD3_LONG_NUMBER 0x00050004

#define ISHIELD3_STRUCTP 0x00050007
#define ISHIELD3_HWND_INT 0x00030003

#define ISHIELD3_STRING 0x00040000
#define ISHIELD3_LIST_POINTER 0x00050005



#define ISHIELD3_NORETURN 0x0000
#define ISHIELD3_SHORT_RET 0x0002
#define ISHIELD3_HWND_INT_RET 0x0003
#define ISHIELD3_LONG_NUMBER_RET 0x0004
#define ISHIELD3_BOOL_RET 0x0006
#define ISHIELD3_STRUCTP_RET 0x0007


#define IS2_LONG 0
#define IS2_STRING 1
#define IS2_BOOL 2
#define IS2_CHAR 3
#define IS2_HWND 4
#define IS2_INT 5
#define IS2_SHORT 6

#define IS3_LONG_NUMBER 0
#define IS3_SHORT 1
#define IS3_STRUCTP 2
#define IS3_STRUCTP_RET 3
#define IS3_HWND_INT 4
#define IS3_HWND_INT2 5
#define IS3_STRING 6
#define IS3_BOOL_RET 7
#define IS3_LIST_POINTER 8
#define IS3_HWND_INT_RET 9
#define IS3_LONG_NUMBER_RET 10
#define IS3_NORETURN -1
#define IS3_SHORT_RET 11

static char* is2TypeTable[] = {"long", "string", "bool", "char", "hwnd", "int",
			       "short", "hwnd/int", "&struct"};

static char* is3TypeTable[] = {"long/number", "short", "&struct", "&struct",
			       "hwnd/int", "hwnd/int", "string", "bool",
			       "list/pointer", "hwnd/int", "long/number",
			       "short"};




#define IS_STRING 0
#define IS_CHAR 1
#define IS_SHORT 2
#define IS_INT 3
#define IS_LONG 4
#define IS_POINTER 5
#define IS_BOOL 6
#define IS_HWND 7




static char* isDataTypes[] = {"string", "char", "short", "int", "long",
			      "pointer", "bool", "hwnd"};

#endif

