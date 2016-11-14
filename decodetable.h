/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#ifndef DECODETABLE_H
#define DECODETABLE_H

#include "common.h"
#include "decodeOps.h"

static char* rawDecodeTable[][4] = {

				 {(char*) 0x0000, "??", (char*) OP_UNKNOWN1, (char*) (*decodeOp)},

				 {(char*) 0x0001, NULL, NULL, (char*) (*doGoto)}, //*
			     {(char*) 0x0002, "abort", NULL, (char*) (*doAbort)},
			     {(char*) 0x0003, "exit", NULL, (char*) (*doExit)},
			     {(char*) 0x0004, "if", (char*)OP_IF, (char*) (*decodeOp)}, //*
			     {(char*) 0x0005, NULL, NULL, (char*) (*doGoto2)},

			     // arithmetic operations
 			     {(char*) 0x0006, "=", (char*)OP_EQUATE, (char*) (*equate)}, // varA = varB  (= Assign()) //*
			     {(char*) 0x0007, "+", (char*) OP_PLUS, (char*) (*decodeOp)}, //*
				 {(char*) 0x0008, "%", (char*) OP_MOD, (char*) (*decodeOp)}, //*

				 {(char*) 0x0009, "<", (char*) OP_LESSTHAN, (char*) (*decodeOp)}, //Comparisons
				 {(char*) 0x000A, ">", (char*) OP_GREATERTHAN , (char*) (*decodeOp)}, //*
				 {(char*) 0x000B, "<=", (char*)OP_LESSTHANEQUAL , (char*) (*decodeOp)}, //*
				 {(char*) 0x000C, ">=", (char*)OP_GREATERTHANEQUAL , (char*) (*decodeOp)}, //*
				 {(char*) 0x000D, "=", (char*)OP_EQUAL , (char*) (*decodeOp)}, //*
				 {(char*) 0x000E, "!=", (char*) OP_NOTEQUAL, (char*) (*decodeOp)}, //*

			     {(char*) 0x000f, "-", (char*) OP_MINUS, (char*) (*decodeOp)},    // X = Y - Z //*
			     {(char*) 0x0010, "*", (char*) OP_MULT, (char*) (*decodeOp)}, //*
			     {(char*) 0x0011, "/", (char*) OP_DIV, (char*) (*decodeOp)}, //*
			     
			     // binary operations
			     {(char*) 0x0012, "&", (char*) OP_BITAND, (char*) (*decodeOp)}, //*
			     {(char*) 0x0013, "|", (char*) OP_BITOR, (char*) (*decodeOp)}, //*
			     {(char*) 0x0014, "^", (char*) OP_BITEOR, (char*) (*decodeOp)}, //*
			     {(char*) 0x0015, "~", (char*) OP_BITNOT, (char*) (*decodeOp)}, //*

			     // shift operators
			     {(char*) 0x0016, "<<", (char*) OP_SHIFTL, (char*) (*decodeOp)}, //*
			     {(char*) 0x0017, ">>", (char*) OP_SHIFTR, (char*) (*decodeOp)}, //*

			     // logical operators
				 {(char*) 0x0018, "||", (char*) OP_LOGICOR, (char*) (*decodeOp)},//*
			     {(char*) 0x0019, "&&", (char*) OP_LOGICAND, (char*) (*decodeOp)},//*
			     

				 //Member operator				 
				 {(char*) 0x001a, "&", (char*) OP_ADDRESS_OF, (char*) (*decodeOp)}, //Address Operator
				 {(char*) 0x001b, "*", (char*) OP_INDIRECTION, (char*) (*decodeOp)}, //Indirection op
				 {(char*) 0x001c, "->", (char*) OP_POINTER, (char*) (*decodeOp)}, //Structure pointer op
				 {(char*) 0x001d, "[]", (char*) OP_STRBW, (char*) (*decodeOp)}, 
				 {(char*) 0x001e, "[]", (char*) OP_STRBR, (char*) (*decodeOp)},
				 {(char*) 0x001f, "??", (char*) OP_UNKNOWN1, (char*) (*decodeOp)},
				 {(char*) 0x0020, "CallDllFx", (char*) OP_UNKNOWN1, (char*) (*doCall)},

			     // function stuff
			     {(char*) 0x0021, NULL, NULL, (char*) (*doCall)},
			     {(char*) 0x0022, NULL, NULL, (char*) (*functionStart)}, //*
			     {(char*) 0x0023, NULL, NULL, (char*) (*funcReturn)}, //*
			     {(char*) 0x0024, NULL, NULL, (char*) (*funcReturn)},
				 {(char*) 0x0025, NULL, NULL, (char*) (*funcReturn)},
			     {(char*) 0x0026, NULL, NULL, (char*) (*functionEnd)}, //*
				 {(char*) 0x0027, NULL, NULL, (char*) (*funcReturn)}, //*				 

				 {(char*) 0x0028, "StrLengthChars", (char*) OP_STRLC, (char*) (*decodeOp)}, 
				 {(char*) 0x0029, "StrSub", (char*) OP_STRSUB, (char*) (*decodeOp)}, 
				 {(char*) 0x002a, "StrFind", (char*) OP_STRFIND, (char*) (*decodeOp)}, 
				 {(char*) 0x002B, "StrCompare", (char*) OP_STRCOMP, (char*) (*decodeOp)},
				 {(char*) 0x002C, "StrToNum", (char*) OP_STRTONUM, (char*) (*decodeOp)},
				 {(char*) 0x002D, "NumToStr", (char*) OP_NUMTOSTR, (char*) (*decodeOp)}, 
				 {(char*) 0x002e, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x002f, "Handler", (char*) OP_HANDLER, (char*) (*decodeOp)}, 
				 {(char*) 0x0030, "ExecuteHandler", (char*) OP_EXEHANDLER, (char*) (*decodeOp)}, 
				 {(char*) 0x0031, "Resize", (char*) OP_RESIZE, (char*) (*decodeOp)}, 				 
				 {(char*) 0x0032, "SizeOf", (char*) OP_SIZEOF, (char*) (*decodeOp)}, 
				 {(char*) 0x0033, "=", (char*) OP_STRUCT_MEMBER, (char*) (*decodeOp)}, //Member operator				 
 				 {(char*) 0x0034, "set", (char*)OP_EQUATE, (char*) (*decodeOp)}, //Set object 
				 {(char*) 0x0035, "read", (char*) OP_STRUCT_MEMBER1, (char*) (*decodeOp)}, //Member operator
				 {(char*) 0x0036, "try", (char*)OP_TRY, (char*) (*decodeOp)}, //*
				 {(char*) 0x0037, "catch", (char*)OP_CATCH, (char*) (*decodeOp)}, //*
				 {(char*) 0x0038, "endcatch", (char*)OP_ENDCATCH, (char*) (*decodeOp)}, //*
				 {(char*) 0x0039, "UseDll", (char*) OP_USEDLL, (char*) (*decodeOp)}, 
				 {(char*) 0x003A, "UnUseDll", (char*) OP_UNUSEDLL, (char*) (*decodeOp)}, 
				 {(char*) 0x003B, "??", (char*) OP_SPECIAL1, (char*) (*decodeOp)}, 
				 {(char*) 0x003c, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)}, 
				 {(char*) 0x003D, ".", (char*) OP_STRUCT_MEMBER2, (char*) (*decodeOp)}, //Member operator
				 {(char*) 0x003e, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 //Probably not used after that...
				 {(char*) 0x003f, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0040, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0041, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0042, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0043, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0044, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0045, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0046, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0047, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0048, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0049, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004A, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004b, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004c, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004d, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004e, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x004f, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0050, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0051, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0052, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0053, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0054, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0055, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0056, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0057, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0058, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0059, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005a, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005b, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005c, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005d, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005e, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x005f, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0060, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0061, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 {(char*) 0x0062, "??", (char*) OP_UNKNOWN2, (char*) (*decodeOp)},
				 
			     {(char*) 0x0129, NULL, NULL, NULL}                // WHILE/SWITCH => ignore it

};

static int rawDecodeTableSize = sizeof(rawDecodeTable) / (sizeof(char*)*4);

typedef void (*decodeFunc)(int, int, int, ISData*);

typedef struct
{
  char* text;
  union
  {
    int numParams;
    int operationNumber;
  } info;
  decodeFunc decodeFunction0;
  decodeFunc decodeFunction1;
} DecodeEntry;

static DecodeEntry** decodeTable;

#define DECODETABLESIZE 512

#endif
