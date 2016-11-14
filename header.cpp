/* 
isDcc
(c) 1998 Andrew de Quincey
adq@tardis.ed.ac.uk
See README.TXT for copying/distribution/modification details.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "ishield.h"
#include "util.h"
#include "common.h"
const char *ISHIELD_STR = "Copyright (c) 1990-2002 InstallShield Software Corp. All Rights Reserved.";

static FunctionPrototype** parseFunctions(int fd, int numFuncs, int fileVersion);
static SysVar** parseSysVars(int fd, int numVar);
static std::vector<DataType*> parseUserTypes(int fd, int numTypes);
static VarType parseType(int fd, int type);

void parseHeader(int fd, ISData* isData)
{
	int tmp;
	int filePos;
	int i;
	unsigned short v0;
	char unknown_buffer[104-80];
	unsigned int offsets[5];

	// the CRC ??
	isData->fileVersion = 12;

	isData->crc = get4Byte(fd);
	v0 = get2Byte(fd);

	tmp = strlen(ISHIELD_STR)+1;
	isData->infoString = (char*) mallocX(tmp);
	filterLF(getString(fd, isData->infoString, tmp));

	// --------------------------------------------------------------------------
	// print out interesting (ahem) header stuff

	if ( strncmp(isData->infoString,ISHIELD_STR,strlen(ISHIELD_STR)) != 0 )
		error("File is not an installshield file! (Bad Magic)\n");

	getLength(fd, unknown_buffer, 104-80);

	//String Bytes, Functions Def, Data Types 
	getLength(fd, (char *)offsets, 5*sizeof(unsigned int));


	// --------------------------------------------------------------------------
	// next comes the string bytes. I'm not ACTUALLY quite sure what these
	// do, but they're definitely to do with string initialisation
	// we'll just skip over them for now, but keep a note of the length for
	// later
	// there are 0x0002's for each system string variable
	// there are 0xffff's for each user string variable, PLUS 5 EXTRA ones!!!
	// (the 5 extra ones are a bug in version 1 of the files, I think)
	tmp = get2Byte(fd);

	_lseek(fd,offsets[2],0);

	isData->stringSysVarsCount = 0;
	isData->stringUserVarsCount = 0;
	isData->numberSysVarsCount = 0;
	isData->numberUserVarsCount = 0;
	isData->endCodeSegmentOffset = offsets[4];
	if ( isData->endCodeSegmentOffset == 0 )
	{
		long cur_pos = _tell(fd);
		isData->endCodeSegmentOffset = _lseek(fd,0,SEEK_END);
		_lseek(fd,cur_pos,SEEK_SET);
	}

	/*// process string system variables
	isData->stringSysVarsCount = get2Byte(fd);
	isData->stringSysVars = parseSysVars(fd, isData->stringSysVarsCount);

	// work out number of user string variables
	// this equals (stringStuffLength - stringSysVarsCount) - 5
	// why 5 has to be subtracted, i don't know, but it does
	if (isData->fileVersion == 2)
		isData->stringUserVarsCount = (tmp - isData->stringSysVarsCount)-5;
	else
		isData->stringUserVarsCount = tmp - isData->stringSysVarsCount;

	// --------------------------------------------------------------------------
	// get number of user number variables
	// Needs 0xb subtracted from it in file version 1... bug??

	// get the number of user vars
	tmp = get2Byte(fd);

	// process numerical system variables
	isData->numberSysVarsCount = get2Byte(fd);
	isData->numberSysVars = parseSysVars(fd, isData->numberSysVarsCount);

	// set the number of user number vars
	if (isData->fileVersion == 2)
		isData->numberUserVarsCount = tmp - 0xb;
	else
		isData->numberUserVarsCount = tmp - isData->numberSysVarsCount;
	*/

	// --------------------------------------------------------------------------
	// do user defined types
	int dataTypesCount = get2Byte(fd);
	isData->dataTypes = parseUserTypes(fd, dataTypesCount);


	// --------------------------------------------------------------------------
	// process user function headers.
	isData->functionPrototypesCount= get2Byte(fd);
	isData->functionPrototypes = parseFunctions(fd, 
		isData->functionPrototypesCount, 
		isData->fileVersion);

	// --------------------------------------------------------------------------
	// process user function bodies.
	isData->functionBodiesCount= isData->functionPrototypesCount;
	isData->functionBodies= (FunctionBody**) mallocX(sizeof(FunctionBody*) * isData->functionBodiesCount);

	// --------------------------------------------------------------------------
	// get number of labels & make up their memory space
	// added +1 onto labelsCount since it seems label0 IS valid....
	// graaa!
	isData->labelsCount = get2Byte(fd) ;
	isData->labels = (Label**) mallocX(sizeof(Label*) * isData->labelsCount);
	for(i=0; i< isData->labelsCount; i++)
	{
		isData->labels[i] = (Label*) mallocX(sizeof(Label));
		isData->labels[i]->usage=0;
		isData->labels[i]->position=-1;
		isData->labels[i]->passed = 0;
		isData->labels[i]->file_position=get4Byte(fd);
	}
	filePos = _tell(fd);

	for(i=0;i<isData->functionPrototypesCount;i++)
	{
		if ( isData->functionPrototypes[i]->label != 0xffff )
		{
			isData->functionPrototypes[i]->label = isData->labels[isData->functionPrototypes[i]->label]->file_position;
		}
	}
	// --------------------------------------------------------------------------
	// make up initial memory for the codelines
	isData->codeLines = (CodeLine **) mallocX(sizeof(CodeLine*));
	isData->codeLinesMax = 1;
	isData->codeLinesCount = 0;


	// make up space for variable usage counters
	isData->stringUserVars = (int*) mallocX(sizeof(int) * isData->stringUserVarsCount);
	for(i=0; i < isData->stringUserVarsCount; i++)
		isData->stringUserVars[i] = 0;
	isData->numberUserVars = (int*) mallocX(sizeof(int) * isData->numberUserVarsCount);
	for(i=0; i < isData->numberUserVarsCount; i++)
		isData->numberUserVars[i] = 0;


	// get the compiler version string
	isData->compilerVersion = 12;

	// --------------------------------------------------------------------------
	// work out the actual end of file - could be some crap there

	// move to EOF
	filePos = _tell(fd);
	_lseek(fd, 0, 2);

	// move back to where we were
	isData->eofPos = _tell(fd);
	_lseek(fd, filePos, 0);
}




static FunctionPrototype** parseFunctions(int fd, 
	int numFuncs, int fileVersion)
{
	int i, j, tmp;
	FunctionPrototype** tmpFuncs;
	FunctionPrototype* tmpFunc;
	char buffer[512];
	int paramCounts[10];
	int strParam, numParam;
	int dll_name,fct_name;

	// setup memory
	tmpFuncs = (FunctionPrototype**) mallocX((sizeof(FunctionPrototype*) * numFuncs));

	// read all functions
	for(i=0; i < numFuncs ; i++)
	{
		// make & store structure
		tmpFunc = (FunctionPrototype*) mallocX(sizeof(FunctionPrototype));
		tmpFunc->functionBody = NULL;
		tmpFuncs[i] = tmpFunc;

		// perform appropriate processing depending on FN type
		tmpFunc->type = get1Byte(fd);
		tmpFunc->returnType = get1Byte(fd);
		//DLL NAME
		dll_name = get2Byte(fd);
		if ( dll_name != 0 )
		{
			// get the dll name
			getString(fd, buffer, dll_name);
			buffer[dll_name]='.';
			dll_name++;
		}
		fct_name = get2Byte(fd);
		if ( fct_name != 0 )
		{
			getString(fd, buffer+dll_name, fct_name);
			tmp = strlen(buffer);
			tmpFunc->name = (char*) mallocX(tmp+1);
			strcpy_s(tmpFunc->name, tmp + 1, buffer);
		}
		else
		{
			sprintf_s(buffer, "function%i", i);
			tmpFunc->name = (char*) mallocX(strlen(buffer)+1);
			strcpy_s(tmpFunc->name, strlen(buffer) + 1, buffer);
		}
		tmpFunc->label = get2Byte(fd); //Line ?

		tmpFunc->paramsCount = get2Byte(fd);
		

		// get memory for parameter list
		tmpFunc->params = (int*) mallocX(sizeof(int) * tmpFunc->paramsCount);

		// setup count of string & number params (and locals)
		tmpFunc->paramStringsCount =0;
		tmpFunc->paramNumbersCount =0;
		tmpFunc->localParamSize =0;

		// extract all the parameter types	
		for(j =0; j< tmpFunc->paramsCount; j++)
		{
			// get param type
			tmpFunc->params[j] = parseType(fd, get1Byte(fd));

			// is it BYREF?
			if (get1Byte(fd) & 2)
				tmpFunc->params[j] |= 0x80000000;

			if (((tmpFunc->params[j]) & 0xffffff) == TYPE_STRING)
				tmpFunc->paramStringsCount++;
			else
				tmpFunc->paramNumbersCount++;
		}


		// get memory for parameter names
		tmpFunc->paramStringsNames = (char**) mallocX(sizeof(char*) * tmpFunc->paramStringsCount);
		tmpFunc->paramNumbersNames = (char**) mallocX(sizeof(char*) * tmpFunc->paramNumbersCount);

		// make up the names
		for(j=0; j< 8; j++)
			paramCounts[j] =0;
		strParam=0;
		numParam=0;
		for(j=0; j< tmpFunc->paramsCount; j++)
		{
			switch(tmpFunc->params[j] & 0xffffff)
			{
			case TYPE_STRING:
				sprintf_s(buffer, "pString%i", paramCounts[0]++);
				break;

			case TYPE_CHAR:
				sprintf_s(buffer, "pChar%i", paramCounts[1]++);
				break;

			case TYPE_LONG:
				sprintf_s(buffer, "pLong%i", paramCounts[2]++);
				break;

			case TYPE_INT:
				sprintf_s(buffer, "pInt%i", paramCounts[3]++);
				break;

			case TYPE_NUMBER:
				sprintf_s(buffer, "pNumber%i", paramCounts[4]++);
				break;

			case TYPE_LIST:
				sprintf_s(buffer, "pList%i", paramCounts[5]++);
				break;

			case TYPE_BOOL:
				sprintf_s(buffer, "pBool%i", paramCounts[6]++);
				break;

			case TYPE_HWND:
				sprintf_s(buffer, "pHwnd%i", paramCounts[7]++);
				break;

			case TYPE_CONSTANT:
				sprintf_s(buffer, "pConstant%i", paramCounts[7]++);
				break;

			case TYPE_UNDEF1:
				sprintf_s(buffer, "pUndef1%i", paramCounts[7]++);
				break;
			case TYPE_UNDEF2:
				sprintf_s(buffer, "pUndef2%i", paramCounts[7]++);
				break;
			case TYPE_UNDEF3:
				sprintf_s(buffer, "pUndef3%i", paramCounts[7]++);
				break;
			case TYPE_UNDEF4:
				sprintf_s(buffer, "pUndef4%i", paramCounts[7]++);
				break;
			case TYPE_UNDEF5:
				sprintf_s(buffer, "pUndef5%i", paramCounts[7]++);
				break;

			default:
				error("Unknown parameter type\n");
			}

			// put the name in the correct list of parameters
			if ((tmpFunc->params[j] & 0xffffff) == TYPE_STRING)
			{	
				tmpFunc->paramStringsNames[strParam] = (char*) mallocX(strlen(buffer) +1);
				strcpy_s(tmpFunc->paramStringsNames[strParam], strlen(buffer) + 1, buffer);
				strParam++;
			}
			else
			{	
				tmpFunc->paramNumbersNames[numParam] = (char*) mallocX(strlen(buffer) +1);
				strcpy_s(tmpFunc->paramNumbersNames[numParam], strlen(buffer) + 1, buffer);
				numParam++;
			}
		}
		// dunno which body this prototype should be paired with
		tmpFunc->functionBody = NULL;
	}

	return(tmpFuncs);
}


static SysVar** parseSysVars(int fd, int numVars)
{
	int nameLen;
	int i;
	SysVar** tmpVars;
	SysVar* tmpVar;

	// setup memory
	tmpVars = (SysVar**) mallocX(sizeof(SysVar*) * numVars);

	// read all strings
	for(i=0; i < numVars; i++)
	{
		// make & store structure
		tmpVar = (SysVar*) mallocX(sizeof(SysVar));
		tmpVars[i] = tmpVar;

		// ignore string number in file
		get2Byte(fd);

		// get name
		nameLen = get2Byte(fd);
		tmpVar->name = (char*) mallocX(nameLen+1);
		getString(fd, tmpVar->name, nameLen);
	}

	return(tmpVars);
}


static std::vector<DataType*> parseUserTypes(int fd, int numTypes)
{
	std::vector<DataType*> tmpVars;

	// setup memory
	tmpVars.resize(numTypes);

	for(int i=0; i< numTypes; i++)
	{
		// setup memory for this type
		DataType* tmpVar = new DataType();
		tmpVars[i] = tmpVar;
		tmpVar->name = NULL;
		/*// skip two bytes
		get2Byte(fd);

		// get name
		tmp = get2Byte(fd);
		tmpVar->name = (char*) mallocX(tmp + 1);
		getString(fd, tmpVar->name, tmp);*/

		// get number of members & setup memory
		unsigned int entriesCount = get2Byte(fd);
		tmpVar->entries.resize(entriesCount);
		TypeEntry* tmpEntry = tmpVar->entries.data();
		// process them
		for(unsigned int j=0; j < entriesCount; j++, ++tmpEntry)
		{
			// get the data type of entry
			tmpEntry->type = parseType(fd, get1Byte(fd));

			// get the size of it
			tmpEntry->size = get2Byte(fd);

			// get name of entry
			int tmp = get2Byte(fd);
			tmpEntry->name = (char*) mallocX(tmp+1);
			getString(fd, tmpEntry->name, tmp);
		}
	}

	return(tmpVars);
}


static VarType parseType(int fd, int type) {
	switch(type) {

	case TYPE_STRING:
		return(TYPE_STRING);
	case 1:
		return(TYPE_CHAR);
	case TYPE_LONG:
		return(TYPE_LONG);
	case 3:
		return(TYPE_INT);
	case TYPE_NUMBER:
		return(TYPE_NUMBER);
	case 5:
		return(TYPE_LIST);
	case 6:
		return(TYPE_BOOL);
	case 7:
		return(TYPE_HWND);
	case 8:
		return(TYPE_UNDEF1);
	case 9:
		return(TYPE_CONSTANT);
	case 10:
		return (TYPE_UNDEF2);
	case TYPE_UNDEF3:
		return (TYPE_UNDEF3);
	case TYPE_UNDEF4:
		return (TYPE_UNDEF4);
	case TYPE_UNDEF5:
		return TYPE_UNDEF5;
	default:
		assert(false);
	}

	error("Unknown type (0x%x) at 0x%x\n", type, _tell(fd));
	return VarType(0);
} // parseType
