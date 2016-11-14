/* 
isDcc
(c) 1998 Andrew de Quincey
adq@tardis.ed.ac.uk
See README.TXT for copying/distribution/modification details.
*/

#include <string.h>
#include <stdio.h>
#include <io.h>

#include "common.h"
#include "decodeOps.h"
#include "decodetable.h"
#include "util.h"


static int labelCounter;
static int instructionCounter;
int argnum = 0;

static void addCodeLine(ISData* isData, int inCounter, CodeLine* codeLine);
static int parseArg(int fd, Parameter* param, ISData* isData, int inCounter, int updateUsage);
static int parseOther(int fd, void* buffer);
static int parseComparison(int comparison);
static void decodePass0(int fd, ISData* isData);
static void decodePass1(ISData* isData);
static void decodePass2(ISData* isData);

void initDecode(void)
{
	int i;
	int pos;

	// get memory & wipe decodeTable
	decodeTable = (DecodeEntry**)  mallocX(sizeof(DecodeEntry*)*DECODETABLESIZE);
	for(i =0; i < DECODETABLESIZE; i++)
		decodeTable[i] = NULL;

	for(i=0; i< rawDecodeTableSize; i++)
	{
		// check op isn't out of range
		if ((pos = (int) rawDecodeTable[i][0]) > DECODETABLESIZE)
			error("\n!!!!Decode table entry out of range! (%i >= %i)\n", pos, DECODETABLESIZE);

		// check we don't have duplicates    
		if (decodeTable[pos] != NULL)
			error("Duplicate entries for opcode (0x%x) in decode table!\n", pos);

		// make new memspace for entry
		decodeTable[pos] = (DecodeEntry*) malloc(sizeof(DecodeEntry));

		// add in the useful stuff
		decodeTable[pos]->text = (char*) rawDecodeTable[i][1];
		decodeTable[pos]->info.numParams = (int) rawDecodeTable[i][2];
		decodeTable[pos]->decodeFunction0 = (decodeFunc) rawDecodeTable[i][3];
	}
	/*
	for(i=0; i< DECODETABLESIZE; i++)
	{
	if (decodeTable[i])
	continue;

	printf("%x\n", i);
	}
	exit(1);
	*/
	// zero the label & instruction counters
	// it seems labels do after all start with 1 (at least in version 5)
	labelCounter=1;     //version2.1
	instructionCounter=0;
}


void decode(int fd, ISData* isData)
{
	// pass 0 - read the code in from the file
	decodePass0(fd, isData);

	// pass 1 - work out function prototype/body pairings & fix local variable counts
	// The problem is that the order in which the function bodies appear in the
	// .ins file does not correspond to the function numbering (1,2,..). A connection
	// between function prototypes (headers) and bodies can only be made, because
	// each function has also a label associated with it (in the function call i.e.
	// function102() ) and I know in which body this label is.
	decodePass1(isData);

	// pass 2 - fix local/parameter variable indicators
	decodePass2(isData);
}


// pass 1 - work out function prototype/body pairings, fix local variable counts,
static void decodePass1(ISData* isData)
{
	int i;
	int j;
	int fn;

	// do the main body of the code
	for(i=0; i< isData->codeLinesCount; i++)
	{
		// ah ha! a function call. we can update
		if ((isData->codeLines[i]->type == FUNCTION) &&
			(isData->codeLines[i]->functionNumber >= 0) &&
			(isData->functionPrototypes[isData->codeLines[i]->functionNumber]->type == FUNCTION_USER))
		{
			// get the function number
			fn = isData->codeLines[i]->functionNumber;

			// skip if we've already done this one
			if (isData->functionPrototypes[fn]->functionBody != NULL) continue;

			// check we've not got an invalid function label (i.e. the label is
			// present in the main code, NOT a function
			if (isData->labels[isData->codeLines[i]->destLabel]->functionNumber == -1)
				error("Invalid function label\n");

			// update function body pointer
			isData->functionPrototypes[fn]->functionBody = 
				isData->functionBodies[isData->labels[isData->codeLines[i]->destLabel]->functionNumber];

			// update local string vars counts
			isData->functionPrototypes[fn]->localParamSize = isData->functionPrototypes[fn]->functionBody->localParamSize;
		}
	}

	// do the functions
	for(i=0; i< isData->functionBodiesCount; i++)
	{
		for(j=0; j< isData->functionBodies[i]->codeLinesCount; j++)
		{
			// ah ha! a function call. we can update
			if ((isData->functionBodies[i]->codeLines[j]->type == FUNCTION) &&
				(isData->functionBodies[i]->codeLines[j]->functionNumber >= 0) &&
				(isData->functionPrototypes[isData->functionBodies[i]->codeLines[j]->functionNumber]->type == FUNCTION_USER))
			{
				// get the function number
				fn = isData->functionBodies[i]->codeLines[j]->functionNumber;

				// skip if we've already done this one	
				if (isData->functionPrototypes[fn]->functionBody != NULL)
					continue;

				// update function body pointer
				isData->functionPrototypes[fn]->functionBody = 
					isData->functionBodies[isData->labels[isData->functionBodies[i]->codeLines[j]->destLabel]->functionNumber];

				isData->functionPrototypes[fn]->localParamSize =
					isData->functionPrototypes[fn]->functionBody->localParamSize;

				/*
				// update local string vars counts
				isData->functionPrototypes[fn]->localStringsCount =
				isData->functionPrototypes[fn]->functionBody->localStringsCount -
				isData->functionPrototypes[fn]->paramStringsCount;
				isData->functionPrototypes[fn]->localStrings = 
				isData->functionPrototypes[fn]->functionBody->localStrings + 
				(sizeof(int) * isData->functionPrototypes[fn]->paramStringsCount);

				// update local number vars counts
				isData->functionPrototypes[fn]->localNumbersCount =
				isData->functionPrototypes[fn]->functionBody->localNumbersCount -
				isData->functionPrototypes[fn]->paramNumbersCount;
				isData->functionPrototypes[fn]->localNumbers = 
				isData->functionPrototypes[fn]->functionBody->localNumbers + 
				(sizeof(int) * isData->functionPrototypes[fn]->paramNumbersCount); */
			}
		}
	}
}


// pass 2 - fix local/parameter variable indicators
static void decodePass2(ISData* isData)
{
	int i;
	int j;
	int k;

	for(i=0; i< isData->functionPrototypesCount; i++)
	{
		// function not actually used
		if (isData->functionPrototypes[i]->functionBody == NULL)
			continue;

		for(j=0; j< isData->functionPrototypes[i]->functionBody->codeLinesCount; j++)
		{      
			for(k=0; k< isData->functionPrototypes[i]->functionBody->codeLines[j]->paramsCount; k++)
			{
				//	printf("\t\tP%i\n", isData->functionPrototypes[i]->functionBody->codeLines[j]->paramsCount);

				if (isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->type == PARAM_FNLOCALSTRINGVARIABLE)
				{
					// this is actually a parameter variable
					if (isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->data.variableNumber <
						isData->functionPrototypes[i]->paramStringsCount)
					{
						isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->type = PARAM_FNPARAMSTRINGVARIABLE;
					}

					// actually a local variable
					else
						isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->data.variableNumber -=
						isData->functionPrototypes[i]->paramStringsCount;
				}

				if (isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->type == PARAM_FNLOCALNUMBERVARIABLE)
				{
					// this is actually a parameter variable
					if (isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->data.variableNumber <
						isData->functionPrototypes[i]->paramNumbersCount)
					{
						isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->type = PARAM_FNPARAMNUMBERVARIABLE;
					}

					// actually a local variable
					else
						isData->functionPrototypes[i]->functionBody->codeLines[j]->params[k]->data.variableNumber -=
						isData->functionPrototypes[i]->paramNumbersCount;
				}
			}

			// fix the destination
			if (isData->functionPrototypes[i]->functionBody->codeLines[j]->destination != NULL)
			{
				if (isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->type == PARAM_FNLOCALSTRINGVARIABLE)
				{
					// this is actually a parameter variable
					if (isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->data.variableNumber <
						isData->functionPrototypes[i]->paramStringsCount)
					{
						isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->type = PARAM_FNPARAMSTRINGVARIABLE;
					}

					// actually a local variable
					else
						isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->data.variableNumber -=
						isData->functionPrototypes[i]->paramStringsCount;
				}

				if (isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->type == PARAM_FNLOCALNUMBERVARIABLE)
				{
					// this is actually a parameter variable
					if (isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->data.variableNumber <
						isData->functionPrototypes[i]->paramNumbersCount)
					{
						isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->type = PARAM_FNPARAMNUMBERVARIABLE;
					}

					// actually a local variable
					else
						isData->functionPrototypes[i]->functionBody->codeLines[j]->destination->data.variableNumber -=
						isData->functionPrototypes[i]->paramNumbersCount;
				}
			}
		} 
	}
}




// pass 0 => decode stuff from the file
void decodePass0(int fd, ISData* isData)
{
	int opcode;
	int i,j;
	int expectFunctionStart;
	int curFunction,currFuncFileOffset;
	int pos;
	int last_label = 0;

	// decode the functions/ subroutines
	curFunction = 0;
	while(1)
	{
		// need to check this as well, 'cos you don't always have an _EWQ section
		// this needs to be checked FIRST, otherwise we'll interfere with opcode
		// reading
		
		pos = _tell(fd);
		currFuncFileOffset = pos;
		if (pos >= isData->eofPos)
		{
			isData->functionBodiesCount = curFunction;
			return;
		}
		opcode = get2Byte(fd); //First 2bytes are some size.

		// check for EOF (read next opcode, move back & check it's not 0x455f)
		opcode = get2Byte(fd);
		_lseek(fd, -2, 1);

		// check we've not got too many functions
		if (curFunction == isData->functionBodiesCount)
			error("Too many functions found!\n");

		// make up space for the codelines in this function
		isData->functionBodies[curFunction] = (FunctionBody*) mallocX(sizeof(FunctionBody));
		isData->functionBodies[curFunction]->codeLines = (CodeLine**) mallocX(sizeof(CodeLine*));
		isData->functionBodies[curFunction]->codeLinesCount = 0;
		isData->functionBodies[curFunction]->codeLinesMax = 1;

		// zero instruction counter, so label offsets will be relative to the
		// start of the function they are in
		instructionCounter=0;

		//    printf("------------\n");	

		// right, process the code for real now
		expectFunctionStart=1;
		while(1)
		{
			// need to check this as well, 'cos you don't always have an _EWQ section
			// this needs to be checked FIRST, otherwise we'll interfere with opcode
			// reading
			pos = _tell(fd);

			if (pos >= isData->eofPos)
			{
				isData->functionBodiesCount = curFunction;
				return;
			}
			  for(i=last_label;i<isData->labelsCount;i++)
			  {
				  if ( isData->labels[i]->file_position == pos )
				  {
					  last_label = i+1;
					  opcode = get2Byte(fd);
				  }
				  if ( isData->labels[i]->file_position >= pos )
				  {
					  last_label = i;
					  break;
				  }
			  }

			// get opcode
			opcode = get2Byte(fd);

			// hit a function start here? => end of function
			if ((opcode == 0x0022) && (!expectFunctionStart))
			{
				_lseek(fd, -2, 1);
				break;
			}

			// hit a function start here? => ignore it
			if ((opcode == 0x0022) && expectFunctionStart)
			{
				for(j=0;j<isData->functionPrototypesCount;j++)
				{
					if ( isData->functionPrototypes[j]->label == currFuncFileOffset  )
					{						
						isData->functionPrototypes[j]->functionBody = isData->functionBodies[curFunction];
						break;
					}
				}
				expectFunctionStart=0;
			}

			//      printf("%x -- %x\n", _tell(fd), opcode);

			// check opcode in range & in table
			if ((opcode > DECODETABLESIZE) || (opcode < 0))
				error("\n!!!!Opcode out of range(0x%x) at (0x%x)\n", opcode, _tell(fd));
			if (decodeTable[opcode] == NULL)
				error("\n!!!!Unknown opcode (0x%x) at (0x%x)\n", opcode, _tell(fd));
			
			// run the function if there is one
			if (decodeTable[opcode]->decodeFunction0 != NULL)
				(*decodeTable[opcode]->decodeFunction0)(fd, opcode, curFunction, isData);
			if ( opcode == 0x0026 )
				break;			
		}

		// keep function indicator up to date
		curFunction++;

		pos = _tell(fd);
		if ( pos >= isData->endCodeSegmentOffset )
			break;
	}
	isData->functionBodiesCount = curFunction;
} 




// ----------------------------------------------------------------------------
// function for parsing labels
static void label(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = LABEL;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->labelNumber = labelCounter;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	// record where this label is (relative to the main code, or the
	// current function start, since instructionCounter is reset at the
	// start of each new function
	isData->labels[labelCounter]->position = instructionCounter;
	isData->labels[labelCounter]->functionNumber = inCounter;

	// just dump the no. of instructions to next label
	get2Byte(fd);

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;

	// move onto the next label
	labelCounter++;
}


// ----------------------------------------------------------------------------
// function for equating things
static void equate(int fd, int opcode, int inCounter, ISData* isData)
{
	int argType,nbArg;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = OPERATION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->operationNumber = OP_EQUATE;
	codeLine->paramsCount = 1;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*));
	strcpy_s(codeLine->string, "=");

	codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));

	nbArg = get2Byte(fd);
	if ( nbArg == 1 )
	{		
		codeLine->paramsCount = 0;
		codeLine->destination->type = PARAM_SYSTEMNUMBERVARIABLE;
		codeLine->destination->data.variableNumber = get2Byte(fd);
	}
	else
	{
		if ( nbArg != 2 ) //!! What is that?
		{
			codeLine->paramsCount = 0;
			_lseek(fd,-2,1); //Skip that one.
			return;
		}

		if ( nbArg != 2 )
			error("\n!!!!Unexpected variable count type at (0x%x) in strCopy\n", _tell(fd));
		

		// get the destination
		switch(parseArg(fd, codeLine->destination, isData, inCounter, 1))
		{
		case PARAM_SYSTEMSTRINGVARIABLE:
		case PARAM_USERSTRINGVARIABLE:
		case PARAM_SYSTEMNUMBERVARIABLE:
		case PARAM_USERNUMBERVARIABLE:
		case PARAM_FNPARAMNUMBERVARIABLE:
		case PARAM_FNLOCALNUMBERVARIABLE:
		case PARAM_FNPARAMSTRINGVARIABLE:
		case PARAM_FNLOCALSTRINGVARIABLE:
			break;

		default:
			error("\n!!!!Unexpected variable type at (0x%x) in strCopy\n", 
				_tell(fd));
		}


		//  printf("YY%i\n", isData->codeLines[0]->type);


		// get the source
		codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
		switch(argType = parseArg(fd, codeLine->params[0], isData, inCounter, 1))
		{
		case PARAM_SYSTEMSTRINGVARIABLE:
		case PARAM_USERSTRINGVARIABLE:
		case PARAM_SYSTEMNUMBERVARIABLE:
		case PARAM_USERNUMBERVARIABLE:
		case PARAM_FNPARAMNUMBERVARIABLE:
		case PARAM_FNLOCALNUMBERVARIABLE:
		case PARAM_FNPARAMSTRINGVARIABLE:
		case PARAM_FNLOCALSTRINGVARIABLE:
		case PARAM_STRINGCONST:
		case PARAM_LONGCONST:
			break;

		default:
			printf("%i\n", argType);
			error("\n!!!!Unexpected variable type at (0x%x) in strCopy\n", 
				_tell(fd));
		}
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}


// ----------------------------------------------------------------------------
// function for an if statement
static void ifStmt(int fd, int opcode, int inCounter, ISData* isData)
{
	char buffer[512];
	int unknown;
	int label;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = IFSTATEMENT;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = get2Byte(fd);
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) * 2);
	codeLine->destination = NULL;


	// get jump destination
	if (parseOther(fd, (void*) buffer) != OTHER_LABEL)
		error("\n!!!!Unexpected variable type at (0x%x) in ifStmt\n", _tell(fd));

	// removed -1 since label0 can exist
	label = *((int*) buffer);
	codeLine->destLabel = label;


	/*
	// build up a cross reference list. version2.1
	makeCrossRef(label, codeLine->offset, isData);

	// update label's usage count
	// removed -1 since label0 can exist
	isData->labels[label]->usage++;
	*/
	// parse comparison
	if ((codeLine->comparisonType = parseComparison(get1Byte(fd))) == OP_UNKNOWN)
		error("\n!!!!Unknown comparison type at (0x%x) in compare\n", _tell(fd));

	// get first operand 
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[0], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in ifStmt\n", 
			_tell(fd));
	}

	// get second operand 
	codeLine->params[1] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[1], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in ifStmt\n", 
			_tell(fd));
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



// ----------------------------------------------------------------------------
// function for a comparison
static void comparison(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));
	Parameter tmpParam;

	// setup start of structure
	codeLine->type = OPERATION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = 2;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) * 2);


	// get the destination
	codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->destination, isData, inCounter, 1))
	{
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in strCopy\n", 
			_tell(fd));
	}


	// get first operand 
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[0], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// get the comparison type
	if (parseArg(fd, &tmpParam, isData, inCounter, 0) != PARAM_LONGCONST)
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", _tell(fd));
	if ((codeLine->operationNumber = parseComparison(tmpParam.data.intVal)) == OP_UNKNOWN)
		error("\n!!!!Unknown comparison type at (0x%x) in compare\n", _tell(fd));
	strcpy_s(codeLine->string, operations[codeLine->operationNumber]);

	// get second operand 
	codeLine->params[1] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[1], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



// ----------------------------------------------------------------------------
// function for a generic operation decode
static void decodeOp(int fd, int opcode, int inCounter, ISData* isData)
{
	int arg,i,val;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));


	// setup start of structure
	codeLine->type = OPERATION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = get2Byte(fd)-1;
	codeLine->operationNumber = decodeTable[opcode]->info.operationNumber;
	strcpy_s(codeLine->string, operations[decodeTable[opcode]->info.operationNumber]);

	if ( codeLine->operationNumber == OP_POINTER )
	{
		_lseek(fd,-2,1);
		codeLine->paramsCount = 1;
		codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));

		codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) *codeLine->paramsCount);
		codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
		codeLine->params[0]->data.intVal = get2Byte(fd);
		codeLine->params[0]->type = PARAM_DATATYPENUM;
		codeLine->params[0]->byref = 0;
		arg = parseArg(fd, codeLine->destination, isData, inCounter, 1);
	}
	else if (( codeLine->operationNumber == OP_UNKNOWN )||
			 //( opcode == 0x2b)||
			 ( opcode == 0x62)) // !!!
	{
		_lseek(fd,-2,1);
		codeLine->paramsCount = 0;
		codeLine->destination = 0;
	}
	else if ( ( opcode == 0x0004 && codeLine->paramsCount > 1 ) ||
		 ( ((opcode >= 0x0007 && opcode < 0x0020) || (opcode == 0x2a) || (opcode == 0x2b)) && codeLine->paramsCount > 2 ))
	{
		_lseek(fd,-2,1);
		codeLine->paramsCount = 0;
		codeLine->destination = 0;
	}
	else 
	{
		if ( opcode == 0x0004 && codeLine->paramsCount == 1 )
		{
			val = get2Byte(fd);
			if ( val == 0 )
			{
				codeLine->paramsCount = 0;
				codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));
				codeLine->destination->data.intVal = val;
				codeLine->destination->type = PARAM_DATATYPENUM;
				codeLine->destination->byref = 0;
				goto output_line;
			}
			else
				_lseek(fd,-2,1);
		}
		if ( codeLine->paramsCount > -1 )
		{
			// get the destination
			codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));
			arg = parseArg(fd, codeLine->destination, isData, inCounter, 1);


			codeLine->params = NULL;
			if ( codeLine->paramsCount > 0 )
			{
				codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) *codeLine->paramsCount);

				for(i=0;i<codeLine->paramsCount;i++)
				{
					// get first operand 
					codeLine->params[i] = (Parameter*) mallocX(sizeof(Parameter));
					arg = parseArg(fd, codeLine->params[i], isData, inCounter, 1);
				}
			}
		}
		else
		{
			codeLine->paramsCount = 0;
			codeLine->destination = NULL;
			codeLine->params = NULL;
		}
	}
output_line:
	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);
	// next instruction
	instructionCounter++;
}


// ----------------------------------------------------------------------------
// function for a single variable operation
static void singleVarOp(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = OPERATION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = 2;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*)*2);
	codeLine->operationNumber = decodeTable[opcode]->info.operationNumber;
	strcpy_s(codeLine->string, operations[decodeTable[opcode]->info.operationNumber]);


	// get the destination
	codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->destination, isData, inCounter, 1))
	{
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in strCopy\n", 
			_tell(fd));
	}

	// copy destination into 1st param
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter)); 
	memcpy(codeLine->params[0], codeLine->destination, sizeof(Parameter));

	// get operand 
	codeLine->params[1] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[1], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}


// ----------------------------------------------------------------------------
// function for a NOT operation
static void notOp(int fd, int opcode, int inCounter, ISData* isData)
{
	char buffer[512];
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = OPERATION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = 1;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*));
	codeLine->operationNumber = decodeTable[opcode]->info.operationNumber;
	strcpy_s(codeLine->string, operations[decodeTable[opcode]->info.operationNumber]);


	// get the destination
	codeLine->destination = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->destination, isData, inCounter, 1))
	{
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in strCopy\n", 
			_tell(fd));
	}


	// ignore this. it is a copy of the other operand 
	// (it seems to put this in, even though BIT NOT takes only one operand!)
	parseArg(fd, (Parameter*) buffer, isData, inCounter, 0);


	// get operand 
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter)); 
	switch(parseArg(fd, codeLine->params[0], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



// ----------------------------------------------------------------------------
// function for the AskOptions(...) dialogue
static void askOptions(int fd, int opcode, int inCounter, ISData* isData)
{
	int numPairs;
	char buffer[512];
	int argType;
	int count;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = FUNCTION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->functionNumber = -1;
	codeLine->destination = NULL;

	// copy function's name into it
	codeLine->name = (char*) mallocX(strlen(decodeTable[opcode]->text)+1);
	strcpy_s(codeLine->name, strlen(decodeTable[opcode]->text) + 1, decodeTable[opcode]->text);

	// get the number of pairs
	if ((argType = parseOther(fd, (void*) buffer)) != OTHER_NUMPARAMS)
		error("\n!!!!Unexpected variable type at (0x%x) in askOptions\n", 
		_tell(fd));
	numPairs = *((int*) buffer);
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) * (2 + (numPairs * 2)));
	codeLine->paramsCount = 2 + (numPairs * 2);

	// first two args definitely there
	// get first operand 
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter)); 
	switch(parseArg(fd, codeLine->params[0], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// get second operand  (string)
	codeLine->params[1] = (Parameter*) mallocX(sizeof(Parameter)); 
	switch(parseArg(fd, codeLine->params[1], isData, inCounter, 1))
	{
	case PARAM_STRINGCONST:
	case PARAM_SYSTEMSTRINGVARIABLE:
	case PARAM_USERSTRINGVARIABLE:
	case PARAM_FNPARAMSTRINGVARIABLE:
	case PARAM_FNLOCALSTRINGVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	// extract (variable number of) args
	for(count =0; count < numPairs; count++)
	{
		// get first operand (string)
		codeLine->params[2+(count*2)] = (Parameter*) mallocX(sizeof(Parameter)); 
		switch(parseArg(fd, codeLine->params[2 + (count*2)], isData, inCounter, 1))
		{
		case PARAM_STRINGCONST:
		case PARAM_SYSTEMSTRINGVARIABLE:
		case PARAM_USERSTRINGVARIABLE:
		case PARAM_FNPARAMSTRINGVARIABLE:
		case PARAM_FNLOCALSTRINGVARIABLE:
			break;

		default:
			error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
				_tell(fd));
		}

		// get second operand (number)	
		codeLine->params[2+1+(count*2)] = (Parameter*) mallocX(sizeof(Parameter)); 
		switch(parseArg(fd, codeLine->params[2 + 1 +(count*2)], isData, inCounter, 1))
		{
		case PARAM_LONGCONST:
		case PARAM_SYSTEMNUMBERVARIABLE:
		case PARAM_USERNUMBERVARIABLE:
		case PARAM_FNPARAMNUMBERVARIABLE:
		case PARAM_FNLOCALNUMBERVARIABLE:
			break;

		default:
			error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
				_tell(fd));
		}
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}    



// ----------------------------------------------------------------------------
// function for goto
static void doGoto(int fd, int opcode, int inCounter, ISData* isData)
{
	int argType;
	char buffer[512];
	CodeLine* codeLine;

	codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = GOTO;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	argType = get1Byte(fd);
	if ( argType == 0 )
	{
		get1Byte(fd);
		codeLine->destLabel = 0; //get2Byte(fd);
	}
	else 
	{
		_lseek(fd,-1,1);

		if ( opcode != 1 )
		{
			// get the destination
			if ((argType = parseOther(fd, (void*) buffer)) != OTHER_LABEL)
				error("\n!!!!Unexpected variable type at (0x%x) in askOptions\n", 
				_tell(fd));
			// removed -1 since label0 can exist
			codeLine->destLabel = *((int*) buffer);
		}
		else
			codeLine->destLabel = 0;
	}

	// build up a cross reference list. version2.1
	//makeCrossRef(codeLine->destLabel, codeLine->offset, isData);

	// update it's usage count
	//isData->labels[codeLine->destLabel]->usage++;

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}

static void doGoto2(int fd, int opcode, int inCounter, ISData* isData)
{
	int argType;
	char buffer[512];
	CodeLine* codeLine;

	codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = GOTO;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	argType = get2Byte(fd);
	if ( argType == 1 )
	{
		argType = parseOther(fd, (void*) buffer);
		if (( argType != OTHER_LABEL) &&
			( argType != OTHER_NUMPARAMS))			
			error("\n!!!!Unexpected variable type at (0x%x) in askOptions\n", 
			_tell(fd));
		// removed -1 since label0 can exist
		codeLine->destLabel = *((int*) buffer);
		if ( argType != OTHER_NUMPARAMS )
		{
			argType = get2Byte(fd);
			if ( argType == 0x0005 )
				_lseek(fd,-2,1);
		}
	}
	else 
	{
		_lseek(fd,-2,1);
		codeLine->destLabel = 0;
	}

	// build up a cross reference list. version2.1
	//makeCrossRef(codeLine->destLabel, codeLine->offset, isData);

	// update it's usage count
	//isData->labels[codeLine->destLabel]->usage++;

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}




// ----------------------------------------------------------------------------
// a function start
static void functionStart(int fd, int opcode, int inCounter, ISData* isData)
{
	int i;
	int numLocalParamSize =0;

	// check we're expecting a function
	//if (inCounter < 0)
	//  error("Got unexpected function start at (0x%x)\n", _tell(fd));

	// OK, done the function, now do it's variable declarations

	// this tells us the length of the function parameter/variable
	// definition. We can just skip it since we need to parse this anyway
	//There seems to be always 2bytes after the 0x2200 command for nothing
	get2Byte(fd);

	// this tells us the number of local strings
	i = get1Byte(fd);
	if ( i != 7 )
		error("Got unexpected start type (0x%x) \n",_tell(fd));

	numLocalParamSize = get4Byte(fd);

	// get number of local strings (incl params)
	isData->functionBodies[inCounter]->localParamSize = (int*)numLocalParamSize;

	// dunno which prototype matches this function yet
	isData->functionBodies[inCounter]->prototype = -1;

	// we don't add this in as a line of code, since it isn't necessary
}


// ----------------------------------------------------------------------------
// a function end
static void functionEnd(int fd, int opcode, int inCounter, ISData* isData)
{

	int nb0,nb1,nb2,nb3,nb4;
	// I used not to do anything here, until someone pointed out that
	// this can occur in the middle of a function, necessitating this
	// extra bit of code to slap a RETURN in.
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	//these are indices
	nb0 = get2Byte(fd); 
	nb1 = get2Byte(fd); //Skip some data here.
	nb2 = get2Byte(fd);
	_lseek(fd,nb2*4,1);
	nb3 = get2Byte(fd);
	nb4 = get2Byte(fd);
	_lseek(fd,nb4*4,1);


	// setup start of structure
	codeLine->type = FCTEND;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}


// ----------------------------------------------------------------------------
// a function return function
static void funcReturn(int fd, int opcode, int inCounter, ISData* isData)
{
	int argType,i;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = FUNCRETURN;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->destination = NULL;

	if ( opcode == 0x27 )
	{
		codeLine->paramsCount =0;
		get2Byte(fd);
	}
	else if ( opcode == 0x25 )
	{
		codeLine->paramsCount =0;
	}
	else /*if ((opcode != 0xb7) && (get2Byte(fd) != 0xb7)) 	// not sure what this does. seems to be always 0xb7
	{
		codeLine->paramsCount =0;
		_lseek(fd, -2, 1);
	}
	else*/
	{
		codeLine->paramsCount = get2Byte(fd);
		if ( codeLine->paramsCount > 0xa )
		{
			_lseek(fd,-2,1);
			codeLine->paramsCount = 0;
		}
		else if ( codeLine->paramsCount != 0 )
		{
			codeLine->params = (Parameter**) mallocX(sizeof(Parameter*)*codeLine->paramsCount);
			for(i=0;i<codeLine->paramsCount;i++)
			{
				codeLine->params[i] = (Parameter*) mallocX(sizeof(Parameter)); 
				argType = parseArg(fd, codeLine->params[i], isData, inCounter, 1);
			}
		}
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;  
}


static void doExit(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));
	int param,i;
	
	// setup start of structure
	codeLine->type = EXIT;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	param = get2Byte(fd); //Skip a bit here.
	if ( param > 1 )
	{
		_lseek(fd,-2,1);
	}
	else
	{
		for(i=0;i<param;i++)
			get2Byte(fd);
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}


static void doAbort(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));
	int param;

	// setup start of structure
	codeLine->type = ABORT;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount =0;
	codeLine->destination = NULL;

	param = get2Byte(fd); //Skip a bit here.
	if ( param != 0 )
	{
		_lseek(fd,-2,1);
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}


static void doHandler(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = HANDLER;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = 2;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*)*2);
	codeLine->destination = NULL;

	// get first operand 
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[0], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
	case PARAM_SYSTEMNUMBERVARIABLE:
	case PARAM_USERNUMBERVARIABLE:
	case PARAM_FNPARAMNUMBERVARIABLE:
	case PARAM_FNLOCALNUMBERVARIABLE:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}


	// get first operand 
	codeLine->params[1] = (Parameter*) mallocX(sizeof(Parameter));
	switch(parseArg(fd, codeLine->params[1], isData, inCounter, 1))
	{
	case PARAM_LONGCONST:
		break;

	default:
		error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
			_tell(fd));
	}

	/*
	NOT ANYMORE... Apparently label 0 CAN exist in the file
	// my labels start at 0
	(codeLine->params[1]->data.intVal--);
	*/

	// GRAAAA. If you pass -1 to Handler(), it wipes the current
	// handler. Need to check for this, and not try and update
	// the labelcount ('cos it'll crash)
	if (codeLine->params[1]->data.intVal != -1) {
		// build up a cross reference list. version2.1
		//makeCrossRef(codeLine->params[1]->data.intVal, codeLine->offset, isData);

		// update label's usage count
		isData->labels[codeLine->params[1]->data.intVal]->usage++;
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



static void doCall(int fd, int opcode, int inCounter, ISData* isData)
{
	char buffer[256];
	int pos,dest_fct,nb_params,i;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = CALL;
	codeLine->offset = _tell(fd)-2;  //version2

	dest_fct = get2Byte(fd);
	nb_params = get2Byte(fd);

	codeLine->opcode = opcode;
	codeLine->paramsCount = nb_params+1;
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*)*(nb_params+1));
	codeLine->destination = NULL;

	// can have a label or an int here
	pos = _tell(fd);
	codeLine->params[0] = (Parameter*) mallocX(sizeof(Parameter));
	codeLine->params[0]->type = PARAM_LONGCONST;
	codeLine->params[0]->data.intVal = dest_fct;

	for(i=1;i<=nb_params;i++)
  	{
	  codeLine->params[i] = (Parameter*) mallocX(sizeof(Parameter));

	  parseArg(fd, codeLine->params[i], isData, inCounter, 1);
  	}

	/*
	NOT ANYMORE.. APPARENTLY label0 CAN exist in the file
	// my labels start at 0
	(codeLine->params[0]->data.intVal)--;
	*/

	// build up a cross reference list. version2.1
	//makeCrossRef(codeLine->params[0]->data.intVal, codeLine->offset, isData);

	// update it's usage count
	isData->labels[codeLine->params[0]->data.intVal]->usage++;

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



static void doReturn(int fd, int opcode, int inCounter, ISData* isData)
{
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = RETURN;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->paramsCount = 0;
	codeLine->destination = NULL;

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}



static void sPrintf(int fd, int opcode, int inCounter, ISData* isData)
{
	int numParams;
	char buffer[512];
	int argType;
	int count;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = FUNCTION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->functionNumber = -1;
	codeLine->destination = NULL;

	// copy function's name into it
	codeLine->name = (char*) mallocX(strlen(decodeTable[opcode]->text) + 1);
	strcpy_s(codeLine->name, strlen(decodeTable[opcode]->text) + 1, decodeTable[opcode]->text);

	// get the number of params
	if ((argType = parseOther(fd, (void*) buffer)) != OTHER_NUMPARAMS)
		error("\n!!!!Unexpected variable type at (0x%x) in askOptions\n", 
		_tell(fd));
	numParams = *((int*) buffer);
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) * numParams);
	codeLine->paramsCount = numParams;

	// extract (variable number of) args
	for(count =0; count < numParams; count++)
	{
		// get operand
		codeLine->params[count] = (Parameter*) mallocX(sizeof(Parameter));
		switch(parseArg(fd, codeLine->params[count], isData, inCounter, 1))
		{
		case PARAM_STRINGCONST:
		case PARAM_SYSTEMSTRINGVARIABLE:
		case PARAM_USERSTRINGVARIABLE:
		case PARAM_LONGCONST:
		case PARAM_SYSTEMNUMBERVARIABLE:
		case PARAM_USERNUMBERVARIABLE:
		case PARAM_FNPARAMNUMBERVARIABLE:
		case PARAM_FNLOCALNUMBERVARIABLE:
		case PARAM_FNPARAMSTRINGVARIABLE:
		case PARAM_FNLOCALSTRINGVARIABLE:
			break;

		default:
			error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
				_tell(fd));
		}
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}




static void sPrintfBox(int fd, int opcode, int inCounter, ISData* isData)
{
	int numParams;
	char buffer[512];
	int argType;
	int count;
	CodeLine* codeLine = (CodeLine*) mallocX(sizeof(CodeLine));

	// setup start of structure
	codeLine->type = FUNCTION;
	codeLine->offset = _tell(fd)-2;  //version2
	codeLine->opcode = opcode;
	codeLine->functionNumber = -1;
	codeLine->destination = NULL;

	// copy function's name into it
	codeLine->name = (char*) mallocX(strlen(decodeTable[opcode]->text)+1);
	strcpy_s(codeLine->name, strlen(decodeTable[opcode]->text) + 1, decodeTable[opcode]->text);

	// get the number of params
	if ((argType = parseOther(fd, (void*) buffer)) != OTHER_NUMPARAMS)
		error("\n!!!!Unexpected variable type at (0x%x) in askOptions\n", 
		_tell(fd));
	numParams = *((int*) buffer);
	codeLine->params = (Parameter**) mallocX(sizeof(Parameter*) * numParams);
	codeLine->paramsCount = numParams;

	// extract (variable number of) args
	for(count =0; count < numParams; count++)
	{
		// get operand
		codeLine->params[count] = (Parameter*) mallocX(sizeof(Parameter));
		switch(parseArg(fd, codeLine->params[count], isData, inCounter, 1))
		{
		case PARAM_STRINGCONST:
		case PARAM_SYSTEMSTRINGVARIABLE:
		case PARAM_USERSTRINGVARIABLE:
		case PARAM_LONGCONST:
		case PARAM_SYSTEMNUMBERVARIABLE:
		case PARAM_USERNUMBERVARIABLE:
		case PARAM_FNPARAMNUMBERVARIABLE:
		case PARAM_FNLOCALNUMBERVARIABLE:
		case PARAM_FNPARAMSTRINGVARIABLE:
		case PARAM_FNLOCALSTRINGVARIABLE:
			break;

		default:
			error("\n!!!!Unexpected variable type at (0x%x) in compare\n", 
				_tell(fd));
		}
	}

	// add it into the appropriate list of codelines
	addCodeLine(isData, inCounter, codeLine);

	// next instruction
	instructionCounter++;
}





// ----------------------------------------------------------------------------
// copy a line of code from the file into the correct place in the structur
static void addCodeLine(ISData* isData, int inCounter, CodeLine* codeLine)
{
	CodeLine** tmpCodeLines;

	// add into main code
	if (inCounter == -1)
	{
		if (isData->codeLinesCount >= isData->codeLinesMax)
		{
			tmpCodeLines = (CodeLine**) malloc(sizeof(CodeLine*) * isData->codeLinesMax*2);
			memcpy(tmpCodeLines, isData->codeLines, sizeof(CodeLine*) * isData->codeLinesMax);
			free(isData->codeLines);
			isData->codeLines = tmpCodeLines;
			isData->codeLinesMax *=2;
		}

		isData->codeLines[isData->codeLinesCount++] = codeLine;
	}

	// add into a function
	if (inCounter >= 0)
	{
		if (isData->functionBodies[inCounter]->codeLinesCount >= isData->functionBodies[inCounter]->codeLinesMax)
		{
			tmpCodeLines =(CodeLine**) malloc(sizeof(CodeLine*) * isData->functionBodies[inCounter]->codeLinesMax*2);
			memcpy(tmpCodeLines, 
				isData->functionBodies[inCounter]->codeLines, 
				sizeof(CodeLine*) * isData->functionBodies[inCounter]->codeLinesMax);
			free(isData->functionBodies[inCounter]->codeLines);
			isData->functionBodies[inCounter]->codeLines = tmpCodeLines;
			isData->functionBodies[inCounter]->codeLinesMax *=2;
		}

		isData->functionBodies[inCounter]->codeLines[isData->functionBodies[inCounter]->codeLinesCount++] = codeLine;
	}

	//printLine(isData,codeLine,0,inCounter);
}



static int parseArg(int fd, Parameter* param, ISData* isData, int inCounter, int updateUsage)
{
	int argType;
	int tmp;
	char buffer[1024];

	switch(argType = get1Byte(fd))
	{
	case 0:
		tmp = get1Byte(fd);
		param->type = PARAM_LONGCONST;
		param->data.intVal = tmp;

		return(PARAM_LONGCONST);

	case 0x6: // static string
		param->type = PARAM_STRINGCONST;

		tmp = get2Byte(fd);
		if ( tmp > 4096 )
		{
			__debugbreak();
		}
		getString(fd, buffer, tmp);
		escapeString(buffer);

		param->data.string = (char*) mallocX(strlen(buffer)+1);

		strcpy_s(param->data.string, strlen(buffer) + 1, buffer);

		return(PARAM_STRINGCONST);

	case 0x7: // static long

		param->type = PARAM_LONGCONST;
		param->data.intVal = get4Byte(fd);
		
		return(PARAM_LONGCONST);

	case 0x5: // local variable
	case 0x4: // local variable
		
		tmp = get2Byte(fd);

		// maybe a function local string variable?
		if ((0xff9b - tmp) < 0x8000)
		{
			param->type = (argType == 4 ? PARAM_FNLOCALSTRINGVARIABLE : PARAM_FNLOCALNUMBERVARIABLE);
			param->data.variableNumber = 0xff9b - tmp;
			
			return(param->type);
		}

		// a global user variable
		param->type = (argType == 4 ? PARAM_FNLOCALSTRINGVARIABLE : PARAM_FNLOCALNUMBERVARIABLE);
		param->data.variableNumber = tmp;
		///if (updateUsage)
		//	(isData->stringUserVars[param->data.variableNumber])++;
		
		return(param->type);

	case 0x8: // destination number variable
		tmp = get2Byte(fd);

		// maybe a function variable?
		if ((0xff9b - tmp) < 0x8000)
		{
			param->type = PARAM_FNLOCALNUMBERVARIABLE;
			param->data.variableNumber = 0xff9b - tmp;
			
			return(PARAM_FNLOCALNUMBERVARIABLE);
		}

		// a global system variable
		if ((tmp >=0) && (tmp < isData->numberSysVarsCount))
		{
			param->type = PARAM_SYSTEMNUMBERVARIABLE;
			param->data.variableNumber = tmp;
			
			return(PARAM_SYSTEMNUMBERVARIABLE);
		}


		// a global user variable
		param->type = PARAM_USERNUMBERVARIABLE;
		param->data.variableNumber = tmp;
		//if (updateUsage)	
		//	(isData->numberUserVars[param->data.variableNumber])++;
		return(PARAM_USERNUMBERVARIABLE);

		error("Out-of-range number variable (0x%x) encountered at 0x%x\n",
			tmp, _tell(fd));

	default:
		return(PARAM_UNKNOWN);
	}
}  

static int parseOther(int fd, void* buffer)
{
	int argType;

	*((int*) buffer) =0;
	switch(argType = get1Byte(fd))
	{
	case 0x0: // number of variable pairs following		
		*((int*) buffer) = (get1Byte(fd) << 8);
		return(OTHER_NUMPARAMS);

	case 0x80: // function number
		*((int*) buffer) = get2Byte(fd);
		return(OTHER_USERFUNCTION);

	case 0x70: // label
		*((int*) buffer) = get2Byte(fd);
		return(OTHER_LABEL);

	case 0x7: // label
		*((int*) buffer) = get4Byte(fd);
		return(OTHER_LABEL);

	default:
		return(OTHER_UNKNOWN);
	}
}


static int parseComparison(int comparison)
{
	switch(comparison & 0x0f)
	{
	case 1:    
		return(OP_LESSTHAN);
		break;

	case 2:
		return(OP_GREATERTHAN);
		break;

	case 3:
		return(OP_LESSTHANEQUAL);
		break;

	case 4:
		return(OP_GREATERTHANEQUAL);
		break;

	case 5:
		return(OP_EQUAL);
		break;

	case 6:
		return(OP_NOTEQUAL);
		break;

	default:
		return(OP_UNKNOWN);
	}
}


// -------------------------------------------------------------------
//version2.1  Building cross references for labels
void makeCrossRef(int labelNr, long offset, ISData *isData){
	int i;
	struct LabelRef *lRefnew, *lRefold;

	lRefnew = (struct LabelRef*) mallocX(sizeof(struct LabelRef));
	lRefnew->offset = offset;
	lRefnew->lRefPointer = NULL;
	if (isData->labels[labelNr]->usage == 0)
		isData->labels[labelNr]->lRefPointer = lRefnew;
	else {
		lRefold = isData->labels[labelNr]->lRefPointer;
		for (i=1; i<isData->labels[labelNr]->usage; i++)
			lRefold = lRefold->lRefPointer;
		lRefold->lRefPointer = lRefnew;
	}

} // makeCrossRef
