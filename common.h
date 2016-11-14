/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <vector>

enum VarType : unsigned int
{
	TYPE_STRING = 0,
	TYPE_CHAR = 1,
	TYPE_LONG = 2,
	TYPE_INT = 3,
	TYPE_NUMBER = 4,
	TYPE_LIST = 5,
	TYPE_BOOL = 6,
	TYPE_HWND = 7,
	TYPE_UNDEF1 = 8,
	TYPE_CONSTANT = 9,
	TYPE_UNDEF2 = 10,
	TYPE_UNDEF3 = 11,
	TYPE_UNDEF4 = 12,
	TYPE_AUTOSTRING = 13,
};

typedef struct
{
  int type;
  int byref;
  
  union
  {
    int intVal;
    char* string;
    int variableNumber;
    int userFunction;
    int label;
  } data;
} Parameter;

typedef struct CodeLine CodeLine;
struct CodeLine
{
  int type;
  long offset;  // version2 by Mr. Smith
  int opcode;
  char* name;
  
  Parameter* destination;
  Parameter** params;
  int paramsCount;

  CodeLine** subCodeLines;
  int subCodeLinesCount;

  int functionNumber;

  char string[32];
  int operationNumber;
  int destLabel;
  int comparisonType;
  int labelNumber;

};

#define FUNCTION 0
#define OPERATION 1
#define IFSTATEMENT 2
#define COMPARISON 3
#define LABEL 4
#define GOTO 5
#define EXIT 6
#define ABORT 7
#define FUNCRETURN 8
#define HANDLER 9
#define CALL 10
#define RETURN 11
#define FCTEND 12


#define OP_UNKNOWN 0
#define OP_EQUATE 1
#define OP_LESSTHAN 2
#define OP_GREATERTHAN 3
#define OP_LESSTHANEQUAL 4
#define OP_GREATERTHANEQUAL 5
#define OP_EQUAL 6
#define OP_NOTEQUAL 7
#define OP_PLUS 8
#define OP_MINUS 9
#define OP_MULT 10
#define OP_DIV 11
#define OP_BITAND 12
#define OP_BITOR 13
#define OP_BITEOR 14
#define OP_BITNOT 15
#define OP_SHIFTL 16
#define OP_SHIFTR 17
#define OP_LOGICAND 18
#define OP_LOGICOR 19
#define OP_STRCAT 20
#define OP_PATHCAT 21
#define OP_MOD 22
#define OP_LOGICNOT 23
#define OP_STRUCT_MEMBER 24
#define OP_POINTER 25
#define OP_INDIRECTION 26
#define OP_ADDRESS_OF 27
#define OP_IF 28
#define OP_UNKNOWN1 29
#define OP_UNKNOWN2 30
#define OP_TRY 31
#define OP_CATCH 32 
#define OP_ENDCATCH 33
#define OP_STRUCT_MEMBER1 34
#define OP_STRUCT_MEMBER2 35
#define OP_USEDLL 36
#define OP_UNUSEDLL 37
#define OP_STRTONUM 38
#define OP_NUMTOSTR 39
#define OP_STRCOMP 40
#define OP_STRFIND 41
#define OP_STRSUB 42
#define OP_STRLC  43
#define OP_STRBR  44
#define OP_STRBW  45
#define OP_RESIZE 46
#define OP_SIZEOF 47
#define OP_EXEHANDLER 48
#define OP_HANDLER 49
#define OP_SPECIAL1 50

static char* operations[] = {NULL, "=", "<", ">", "<=", ">=", "==", "!=",
			     "+", "-", "*", "/", "&", "|", "^", "~", "<<",
			     ">>", "&&", "||", "+", "^", "%","!",".","->","*","&","if","??","??",
				"try","catch","endcatch","",".","UseDll","UnUseDll","StrToNum",
				"NumToStr","StrCompare","StrFind","StrSub","StrLengthChars","","",
				"Resize","SizeOf","ExecuteHandler","Handler","Set2Handler??"};  //version2.1

typedef struct {
  char* name;
} SysVar;


//version2.1
struct LabelRef {
  long offset;
  struct LabelRef *lRefPointer;
};

typedef struct {
  int usage;
  int position;
  int file_position;
  int functionNumber;
  int passed;
  struct LabelRef *lRefPointer; //version2.1
} Label;


typedef struct{
  CodeLine** codeLines;
  int codeLinesCount;
  int codeLinesMax;

  int* localParamSize;
  
  int prototype;
} FunctionBody;


typedef struct
{
  int type;
  char *name;
  int label;
  int returnType;
  FunctionBody* functionBody;
  


  // top bit set => byref
  int* params;
  int paramsCount;

  char** paramStringsNames;
  int paramStringsCount;

  char** paramNumbersNames;
  int paramNumbersCount;

  int* localParamSize;
} FunctionPrototype;



#define EVENT_HANDLER 0xA
#define FUNCTION_USER 2
#define FUNCTION_DLL 1

typedef struct
{
	VarType type;
	unsigned int size;
	char* name;
} TypeEntry;

typedef struct
{
  char* name;
  std::vector<TypeEntry> entries;
} DataType;

typedef struct
{

  unsigned long versionMagic;
  unsigned char fileVersion;
  unsigned long crc;
  char* infoString;
  unsigned char compilerVersion;
  long eofPos;

  FunctionPrototype** functionPrototypes;
  int functionPrototypesCount;

  FunctionBody** functionBodies;
  int functionBodiesCount;

  SysVar** stringSysVars;
  int stringSysVarsCount;

  SysVar** numberSysVars;
  int numberSysVarsCount;

  int* stringUserVars;
  int stringUserVarsCount;

  int* numberUserVars;
  int numberUserVarsCount;
  int endCodeSegmentOffset;

  std::vector<DataType*> dataTypes;

  CodeLine** codeLines;
  int codeLinesCount;
  int codeLinesMax;

  Label** labels;
  int labelsCount;
} ISData;



#define PARAM_UNKNOWN 0
#define PARAM_STRINGCONST 1
#define PARAM_LONGCONST 2
#define PARAM_SYSTEMSTRINGVARIABLE 3
#define PARAM_USERSTRINGVARIABLE 4
#define PARAM_SYSTEMNUMBERVARIABLE 5
#define PARAM_USERNUMBERVARIABLE 6
#define PARAM_FNPARAMSTRINGVARIABLE 7
#define PARAM_FNPARAMNUMBERVARIABLE 8
#define PARAM_FNLOCALSTRINGVARIABLE 9
#define PARAM_FNLOCALNUMBERVARIABLE 10
#define PARAM_LABEL 11
#define PARAM_DATATYPENUM 12

#define OTHER_UNKNOWN 0
#define OTHER_USERFUNCTION 1
#define OTHER_LABEL 2
#define OTHER_NUMPARAMS 3


/*
BOOL*
CHAR*
HWND*
INT*
LIST*
LONG (number)*
LPSTR (pointer)
Number*
Object
Pointer
Short (Number)
String *
Variant
Void
WString
*/

static char* types[] = {"fixstr", "char", "long", "int", "number", 
			"LIST", "BOOL", "HWND","UNDEF1","CONSTANT","UNDEF2",
			"UNDEF3", "UNDEF4", "autostr" };

extern int argnum; //version2

#endif





