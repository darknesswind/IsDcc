/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "util.h"

void printArgs(ISData* isData, CodeLine* c, int function);
void printArg(ISData* isData, Parameter* p, int function);
void printLine(ISData* isData, CodeLine* code, int indent, int function);

extern char version[];

void output(int fd, ISData* isData, int indent) {
  int i;
  int j;
  int strParam, numParam;


  printf_s("// Generated with isDcc v%s\n", version);
  printf_s("// (c) 1998 Andrew de Quincey\n");
  printf_s("// isDcc v2.00 (c) 2000 Mr. Smith\n");
  printf_s("// isDcc v2.10 (c) 2001 Mr. Smith\n");
  printf_s("// isDcc v3.10, (c) 2001 Mr Won't tell:) \n"); //version3.1
  printf_s("// original file version = %d\n", isData->compilerVersion);
  printf_s("\n\n");

  // the declare section
  printf_s("declare\n");

  // string vars
  if (isData->stringUserVarsCount) {
    printIndent(indent);
    printf_s("// ------------- STRING VARIABLES --------------\n");
  }
  for(i=0; i < isData->stringUserVarsCount; i++)
    if (isData->stringUserVars[i]) {
      printIndent(indent);
      printf_s("string string%i;\n", i);
    }
  if (isData->stringUserVarsCount)
    printf_s("\n");


  // number vars
  if (isData->numberUserVarsCount)
  {
    printIndent(indent);
    printf_s("// ------------- NUMBER VARIABLES --------------\n");
  }
  for(i=0; i < isData->numberUserVarsCount; i++)
    if (isData->numberUserVars[i])
    {
      printIndent(indent);
      printf_s("number number%i;\n", i);
    }
  if (isData->numberUserVarsCount)
    printf_s("\n");
  
  // datatypes
  if (isData->dataTypes.size())
  {
    printIndent(indent);
    printf_s("// ------------- DATA TYPES --------------\n");
  }
  for(i=0; i < isData->dataTypes.size(); i++)
  {
    printIndent(indent);
    printf_s("typedef %s\n", isData->dataTypes[i]->name);
    printIndent(indent);
    printf_s("begin\n");

    for(j=0; (unsigned int)j< isData->dataTypes[i]->entries.size(); j++)
    {
      printIndent(indent+1); 
      printf_s("%s ", types[isData->dataTypes[i]->entries[j].type]);
      printf_s("%s", isData->dataTypes[i]->entries[j].name);
      
      if (isData->dataTypes[i]->entries[j].type == 0)
	    printf_s("[%i]", isData->dataTypes[i]->entries[j].size);
      printf_s(";\n");
    }

    printIndent(indent);
    printf_s("end;\n");
    
    if (i < (isData->dataTypes.size() -1))
      printf_s("\n");
  }
  if (isData->dataTypes.size())
    printf_s("\n");

  // functions declarations
  if (isData->functionPrototypesCount)
  {
    printIndent(indent);
    printf_s("// ------------- FUNCTION PROTOTYPES --------------\n");
  }
  for(i=0; i< isData->functionPrototypesCount; i++)
  {
    // if we don't actually know which function body
    // is associated, then skip the function
    if ((isData->functionPrototypes[i]->functionBody == NULL) &&
		(isData->functionPrototypes[i]->type == FUNCTION_USER))
      continue;
    
    strParam=0;
    numParam=0;
    printIndent(indent);
    printf_s("%d prototype %s(", i, isData->functionPrototypes[i]->name);
    
    for(j=0; j< isData->functionPrototypes[i]->paramsCount; j++)
    {
      if (isData->functionPrototypes[i]->params[j] & 0x80000000)
	printf_s("BYREF ");

      printf_s("%s", types[isData->functionPrototypes[i]->params[j] & 0xffffff]);
      
      if (j < (isData->functionPrototypes[i]->paramsCount -1))
	printf_s(", ");
    }

    printf_s("); ");
	if ( isData->functionPrototypes[i]->label != 0xFFFF )
		printf_s("starting at %06X",isData->functionPrototypes[i]->label);
	printf_s("\n");
  }


  // functions
  printf_s("\n");
  printIndent(indent);
  printf_s("// ------------- FUNCTION DEFS --------------\n");
  for(i=0; i < isData->functionPrototypesCount; i++)
  {
    // not a user function => no body
    if (isData->functionPrototypes[i]->type == FUNCTION_DLL)
      continue;

    // function never called => skip it
    if ((isData->functionPrototypes[i]->functionBody == NULL))
      continue;

    // print function header
    printIndent(indent);
    printf_s("// ------------- FUNCTION %s (%d) --------------\n", isData->functionPrototypes[i]->name,i);
    printIndent(indent);
    printf_s("function %s(", isData->functionPrototypes[i]->name);
    
    strParam=0;
    numParam=0;
    for(j=0; j< isData->functionPrototypes[i]->paramsCount; j++)
    {
      switch((isData->functionPrototypes[i]->params[j]) & 0xffffff)
      {
      case TYPE_STRING:
	      printf_s("%s", isData->functionPrototypes[i]->paramStringsNames[strParam++]);
	      break;
	
	// a number
      default:
	      printf_s("%s", isData->functionPrototypes[i]->paramNumbersNames[numParam++]);
      }
      
      if (j < (isData->functionPrototypes[i]->paramsCount -1))
	      printf_s(", ");
    }
    printf_s(")\n");

	/*
    // print local variables
    for(j=0; j< isData->functionPrototypes[i]->localNumbersCount; j++)
    {
      printIndent(indent+1);
      printf_s("number lNumber%i;\n", j);
    }
    for(j=0; j< isData->functionPrototypes[i]->localStringsCount; j++)
    {
      printIndent(indent+1);
      printf_s("string lString%i;\n", j);
    }*/

    // print function body
    printIndent(indent);
    printf_s("begin\n");
    for(j=0; j < isData->functionPrototypes[i]->functionBody->codeLinesCount; j++)
    {
      // skip any NULLed lines
      if (isData->functionPrototypes[i]->functionBody->codeLines[j] == NULL) continue;
      printLine(isData, isData->functionPrototypes[i]->functionBody->codeLines[j], indent+1, i);
    }

    // if last thing in function is a label, then stick a return in	
    // (can't have a label as the last thing in a function)
    if ((isData->functionPrototypes[i]->functionBody->codeLinesCount) &&
	(isData->functionPrototypes[i]->functionBody->codeLines[j-1]->type == LABEL))
    {
      printIndent(indent+1);
      printf_s("return;\n");
    }

    // end of function
    printIndent(indent);
    printf_s("end;\n");
    
    if (i < (isData->functionPrototypesCount -1))
      printf_s("\n\n");
  }
}   



void printArgs(ISData* isData, CodeLine* c, int function)
{
  int i;
  
  for(i=0; i< c->paramsCount; i++)
  {
    printArg(isData, c->params[i], function);
    
    if (i < (c->paramsCount-1))
      printf_s(", ");
  }
}


void printArg(ISData* isData, Parameter* p, int function)
{
  switch(p->type)
  {
  case PARAM_SYSTEMSTRINGVARIABLE:
    printf_s("%s", isData->stringSysVars[p->data.variableNumber]->name);
    break;

  case PARAM_USERSTRINGVARIABLE:
    printf_s("string%i", p->data.variableNumber);
    break;

  case PARAM_FNPARAMSTRINGVARIABLE:
    if (function == -1)
      error("Unexpected function parameter found (not in a function)\n");

    if (p->data.variableNumber >= isData->functionPrototypes[function]->paramStringsCount)     
	error("Invalid function parameter found\n");
      
    printf_s("%s", isData->functionPrototypes[function]->paramStringsNames[p->data.variableNumber]);
    break;

  case PARAM_FNLOCALSTRINGVARIABLE:
    if (function == -1)
      error("Unexpected function local found (not in a function)\n");

    printf_s("lString%i", p->data.variableNumber);
    break;

  case PARAM_SYSTEMNUMBERVARIABLE:
	  if ( isData->numberSysVarsCount!= 0 )
    	printf_s("%s", isData->numberSysVars[p->data.variableNumber]->name);
	  else
		  printf_s("%d", p->data.variableNumber);
    break;

  case PARAM_USERNUMBERVARIABLE:
    printf_s("number%i", p->data.variableNumber);
    break;

  case PARAM_FNPARAMNUMBERVARIABLE:
    if (function == -1)
      error("Unexpected function parameter found (not in a function)\n");

    if (p->data.variableNumber >= isData->functionPrototypes[function]->paramNumbersCount)     
	error("Invalid function parameter found\n");
      
    printf_s("%s", isData->functionPrototypes[function]->paramNumbersNames[p->data.variableNumber]);
    break;

  case PARAM_FNLOCALNUMBERVARIABLE:
    if (function == -1)
      error("Unexpected function local found (not in a function)\n");

    printf_s("lNumber%i", p->data.variableNumber);
    break;

  case PARAM_STRINGCONST:
    printf_s("\"%s\"", p->data.string);
    break;
      
  case PARAM_LONGCONST:
    if (p->data.intVal < 256)
      printf_s("%i", p->data.intVal);
    else
      printf_s("0x%x", p->data.intVal);
    break;

  case PARAM_DATATYPENUM:
    printf_s("%s", isData->dataTypes[p->data.intVal]->name);
	if ( isData->dataTypes[p->data.intVal]->name == NULL )
	{
		printf_s(",%d",p->data.intVal);
	}
    break;

  default:
    error("poo\n");
  }	
}


void printLine(ISData* isData, CodeLine* code, int indent, int function){
  int i;
  struct LabelRef *lRefP;
  int previous_label = 0;
  for(i=0;i<isData->labelsCount;i++)
  {
	  if (( isData->labels[i]->file_position < code->offset )&&
		  ( isData->labels[i]->passed == 1))
	  {
		  previous_label = i;
	  }

	  if (( isData->labels[i]->file_position < code->offset )&&
		  ( isData->labels[i]->passed == 0))
	  {
		  previous_label = i;
		  isData->labels[i]->passed = 1;
		  printf_s("Label%d:\n",i,isData->labels[i]->file_position);
	  }
	  if ( isData->labels[i]->file_position == code->offset )
	  {
		  previous_label = i;
		  isData->labels[i]->passed = 1;
		  printf_s("Label%d:\n",i);
		  return;
	  }
	  if ( isData->labels[i]->file_position >= code->offset )
		  break;
  }

  if (argnum==2 && code->type != LABEL) 
    printf_s("%06X:%04X: ",code->offset,code->opcode); //version2

  switch(code->type){
  case FUNCTION:
    printIndent(indent);	
    if (isData->fileVersion == 5)
      printf_s("%s(", code->name);
    else  // fix for installshield < 5
    {
      if ((code->functionNumber != -1) &&
	  (isData->functionPrototypes[code->functionNumber]->type == FUNCTION_DLL) &&
      	  strchr(code->name, '.'))
	      printf_s("%s(", strchr(code->name, '.') +1);
      else
	      printf_s("%s(", code->name);

	  	printf_s("begin\n");
    }
     
    printArgs(isData, code, function);
    printf_s(");\n");
    break;

  case OPERATION:
    printIndent(indent);
	if ( code->destination) 
	{
		if ( code->operationNumber == OP_STRBR && code->paramsCount == 2)
		{
			printArg(isData, code->destination, function);
			printf_s("= ");
			printArg(isData, code->params[0], function);
			printf_s("[");
			printArg(isData, code->params[1], function);
			printf_s("]");
		}
		else if ( code->operationNumber == OP_STRBW )
		{
			printArg(isData, code->destination, function);
			printf_s("[");
			printArg(isData, code->params[0], function);
			printf_s("]");
			printf_s("= ");
			printArg(isData, code->params[1], function);
		}
		else if( ( code->operationNumber == OP_STRLC)||
				 ( code->operationNumber == OP_SPECIAL1)||
				 ( code->operationNumber == OP_SIZEOF ))
		{
			printf_s("number0 = %s(",code->string);
			printArg(isData, code->destination, function);
			for(i=0;i<code->paramsCount;i++)
			{
				printf_s(",");
				printArg(isData, code->params[i], function);
			}
			printf_s(" )");
		}
		else
		if (( code->operationNumber == OP_USEDLL )||
			( code->operationNumber == OP_RESIZE )||
			( code->operationNumber == OP_EXEHANDLER)||
			( code->operationNumber == OP_HANDLER)||
			( code->operationNumber == OP_UNUSEDLL ))
		{
			printf_s("%s (", code->string);
			printArg(isData, code->destination, function);
			for(i=0;i<code->paramsCount;i++)
			{
				printf_s(",");
				printArg(isData, code->params[i], function);
			}
			printf_s(" )");
		}
		else if (( code->operationNumber == OP_STRTONUM)||
			( code->operationNumber == OP_STRCOMP) ||
			( code->operationNumber == OP_STRSUB) ||
			( code->operationNumber == OP_STRFIND) )
		{
			printArg(isData, code->destination, function);

			printf_s(" = %s (",code->string);
			printArg(isData, code->params[0], function);			
			for(i=1;i<code->paramsCount;i++)
			{
				printf_s(",");
				printArg(isData, code->params[i], function);
			}
			printf_s(" )");
		}
		else if ( code->operationNumber == OP_NUMTOSTR)
		{
			printf_s("NumToStr(");
			printArg(isData, code->destination, function);
			printf_s(",");
			printArg(isData, code->params[0], function);			
			printf_s(" )");
		}
		else if ( code->operationNumber == OP_STRUCT_MEMBER1 )
		{
			if ( code->destination != NULL && code->paramsCount >= 1 )
			{
				if ( code->paramsCount == 2 )
				{
					printf_s(" number0 = ");
					printArg(isData, code->params[1], function);
					printf_s(".");
					printArg(isData, code->destination, function);
					printArg(isData, code->params[0], function);
				}
				else
				{
					printf_s(" number0 = %s (", code->string);
					printArg(isData, code->destination, function);
					printf_s(".");
					printArg(isData, code->params[0], function);				
					for(i=1;i<code->paramsCount;i++)
					{
						printf_s(",");
						printArg(isData, code->params[i], function);
					}
					printf_s(" )");
				}
			}
			else
			{
				printf_s("wrong code");
			}
		}
		else if ( code->operationNumber == OP_STRUCT_MEMBER )
		{			
			if ( code->destination != NULL && code->paramsCount >= 2 )
			{
				printArg(isData, code->params[1], function);				
				printf_s(".");
				printArg(isData, code->destination, function);
				printArg(isData, code->params[0], function);
				printf_s("=");
				for(i=2;i<code->paramsCount;i++)
				{
					printf_s(",");
					printArg(isData, code->params[i], function);
				}

			}
			else
			{
				printf_s("wrong code");
			}
		}
		else if ( code->operationNumber == OP_IF )
		{
			printf_s("%s ", code->string);
			if ( code->paramsCount == 1 )
			{
				printArg(isData, code->params[0], function);
				printf_s(" == false then goto label%d ",code->destination->data.intVal+previous_label);
				//printArg(isData, code->destination, function);				
			}
		}
		else
		{
	    	printArg(isData, code->destination, function);
	    	printf_s(" = ");
		    
		    switch(code->operationNumber)
		    {		      
		    case OP_EQUATE:
				if ( code->paramsCount != 0 )
		      		printArg(isData, code->params[0], function);
			break;
			
		    default:
				{
					if ( code->paramsCount == 2 )
					{
						printArg(isData, code->params[0], function);
						printf_s(" %s ", code->string);
						printArg(isData, code->params[1], function);
					}
					else
					{
						printf_s(" %s ", code->string);
						for(i=0;i<code->paramsCount;i++)
						{
				      		printArg(isData, code->params[i], function);
							if ( i != code->paramsCount-1)
								printf_s(" , ");
						}
					}
				}
		      break;
		    }
		}
	}
	else
		printf_s(" %s ", code->string);
    printf_s(";\n");
    break;
    
  case EXIT:
    printIndent(indent);
    printf_s("exit;\n");
    break;
    
  case ABORT:
    printIndent(indent);
    printf_s("abort;\n");
    break;
    
  case LABEL:
    //version2.1 Modified to print cross references
    if (isData->labels[code->labelNumber]->usage){
      printf_s("\nlabel%i: //Ref: ", code->labelNumber);
      lRefP = isData->labels[code->labelNumber]->lRefPointer;
      for (i=0; i<isData->labels[code->labelNumber]->usage; i++){
        printf_s("%06X  ",lRefP->offset);
        lRefP = lRefP->lRefPointer;
      }
      printf_s("\n");
    }
    break;
    
  case GOTO:
    printIndent(indent);
    printf_s("goto label%i;\n", code->destLabel+previous_label);
    break;
    
  case IFSTATEMENT:
    printIndent(indent);
    printf_s("if (");
    printArg(isData, code->params[0], function);
    printf_s(" %s ", operations[code->comparisonType]);
    printArg(isData, code->params[1], function);
    printf_s(") then\n");
    printIndent(indent+1);
    if (argnum==2) printf_s("             "); //version2
    printf_s("goto label%i;\n", code->destLabel);
    printIndent(indent);
    if (argnum==2) printf_s("             "); //version2
    printf_s("endif;\n");
    break;
    
  case HANDLER:
    printIndent(indent);
    printf_s("Handler(");
    printArg(isData, code->params[0], function);
    printf_s(", label%i", code->params[1]->data.intVal);
    printf_s(");\n");
      break;
      
  case RETURN:
    printIndent(indent);
	printf_s("return;\n");
    break;
  case FCTEND:
    printIndent(indent);
    printf_s("end;\n");
    break;
    
  case CALL:
    printIndent(indent);
	if ( code->opcode == 0x21 )
	{
    	printf_s("call ");
		i = code->params[0]->data.intVal;
		if ( i < isData->functionPrototypesCount )
		{
			printf_s("%s",isData->functionPrototypes[i]->name);
		}
		else
		{
			printf_s("label%i", code->params[0]->data.intVal);
		}
	}
	else
	{
		printf_s("callDll ");
		i = code->params[0]->data.intVal;
		if ( i < isData->functionPrototypesCount )
		{
			printf_s("%s",isData->functionPrototypes[i]->name);
		}
		else
		{
			printf_s("label%i", code->params[0]->data.intVal);
		}
	}
    
	printf_s("(");
	for(i=1;i<code->paramsCount;i++)
    {      
      printArg(isData, code->params[i],function);
	  if ( i != code->paramsCount-1) 
      	printf_s(",");
    }
	printf_s(")");
    printf_s(";\n");
    break;

  case FUNCRETURN:
    printIndent(indent);

	if ( code->opcode == 0x27 )
		printf_s("freeLocalVariable();\n");
	else
	{
	    printf_s("return");
	    if (code->paramsCount == 1)
	    {
	      printf_s("(");
	      printArg(isData, code->params[0],function);
	      printf_s(")");
	    }
	    printf_s(";\n");
	}
    break;

  default:
    error("unknown code type %i\n", code->type);
  }
}











