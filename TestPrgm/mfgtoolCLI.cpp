#include <stdio.h>
#include <string.h>
#include <cstring>
#include <fstream>
#include <string>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include "MfgToolLib_Export.h"
int TERM_WIDTH = 80;
int TEXT_WIDTH = 0.4 * TERM_WIDTH;
int BAR_WIDTH = 0.5 * TERM_WIDTH;

//char curCommand[TEXT_WIDTH] = "Starting";
//char oldCommand[TEXT_WIDTH];
std::string curCommand = std::string("Starting");
std::string oldCommand = std::string("");
std::string trim(std::string str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	if (last > TEXT_WIDTH)
		last = TEXT_WIDTH - 1;
	return str.substr(first, (last-first+1));
}
void updateUI(OPERATE_RESULT* puiInfo)
{
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	TERM_WIDTH = w.ws_col;
	TEXT_WIDTH = 0.4 * TERM_WIDTH;
	BAR_WIDTH = 0.4 * TERM_WIDTH;
	std::string cmdInfo = std::string(puiInfo->cmdInfo);
	if (!cmdInfo.empty())
	{
		cmdInfo = trim(cmdInfo);
		oldCommand = std::string(curCommand);
		curCommand = std::string(cmdInfo);
	}
	if (oldCommand.compare(curCommand) != 0)
	{
		std::cout << "\r" << std::setw(TEXT_WIDTH) << oldCommand << " [";
		for (int i = 0; i < BAR_WIDTH; i++)
		{
			std::cout << "=";
		}
		std::cout << ">] Done!" << std::endl;
	}
	else
	{
		std::cout << "\r" << std::setw(TEXT_WIDTH) << oldCommand << " [";
		float percentage = (float) puiInfo->DoneWithinCommand / puiInfo->TotalWithinCommand;
		int bars = percentage * (float) BAR_WIDTH;
		for (int i = 0; i < bars; i ++)
		{
			std::cout << "=";
		}
		std::cout << ">";
		for (int i = 0; i < BAR_WIDTH - bars; i++)
		{
			std::cout << " ";
		}
		std::cout << "]";
	}
	std::cout.flush();
	return;
}
int main (int argc,char* argv[]){
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	TERM_WIDTH = w.ws_col;
	TEXT_WIDTH = 0.4 * TERM_WIDTH;
	BAR_WIDTH = 0.4 * TERM_WIDTH;
	INSTANCE_HANDLE lib;

	char * newpath;
	int hasnewpath = 0;


	std::map<CString, CString> m_uclKeywords;
	std::map<CString, CString>::const_iterator it;


	std::ifstream file("cfg.ini");
	std::string str;
	int state = 0;
	while (std::getline(file, str))
	{
		if (state == 3)
		{
			size_t locBreak = str.find(" ");
			int strip = 3;
			if (locBreak == std::string::npos)
			{
				locBreak = str.find("=");
				strip = 1;
			}
			m_uclKeywords[str.substr(0, locBreak)] = str.substr(locBreak + strip, str.size() - locBreak);
		}
		if (str.compare("[variable]") == 0)
			state = 3;
	}


	printf("Your Options:\n");

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--setting") == 0)
		{
			CString* argString = new CString(argv[i + 1]);
			int loc = argString->find('=', 0);
			if (loc == std::string::npos)
			{
				m_uclKeywords[*argString] = argv[i+2];
				std::cout << argString << ": " << argv[i+2] << std::endl;
				i+=2;
			}
			else
			{
				m_uclKeywords[argString->substr(0, loc)] = argString->substr(loc + 1, argString->size() - loc);
				std::cout << argString->substr(0, loc) << ": " << argString->substr(loc + 1, argString->size() - loc) << std::endl;
				i++;
			}
			delete argString;
		}
		else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--profile") == 0)
		{
			hasnewpath = 1;
			newpath = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			printf("Usage: [program] [arguments] [settings]=[values]\n");
			printf("  -s  --setting		Specify any UCL keywords.\n");
			printf("	-p	--profile		Specify path to Profiles directory.\n");
			printf("  -h  --help      Display this help information.\n");
			exit(EXIT_SUCCESS);
		}
	}

	OPERATIONS_INFORMATION infoOp;
	int ret=MfgLib_Initialize();
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("Failed to initialize MfgLib\n");
		return -1;
	}
	ret=MfgLib_CreateInstanceHandle(&lib);
	if (ret!=MFGLIB_ERROR_SUCCESS){
		printf("CreateInstanceHandle failed\n");
		return -1;
	}



	for ( it=m_uclKeywords.begin(); it!=m_uclKeywords.end(); ++it )
	{
		CString key = it->first;
		CString value = it->second;
		MfgLib_SetUCLKeyWord((char*)key.GetBuffer(),(char*) value.GetBuffer());
	}


	if (hasnewpath == 1)
		ret = MfgLib_SetProfilePath(lib, (BYTE_t *) newpath);

	//set profile and list
	ret = MfgLib_SetProfileName(lib,(BYTE_t *) _T("Linux"));
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("Set Profile name failed\n"));
		return -1;
	}
	ret = MfgLib_SetListName(lib, (BYTE_t *) _T("SDCard"));
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("Set List name failed\n"));
		return -1;
	}
	ret = MfgLib_SetMaxBoardNumber(lib,4);// hard coded for test
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("The specified board number[%d] is invalid, should be 1~4\n"), 4);
		return -1;
	}


	OPERATION_INFOMATION *pOpInformation = NULL;
	pOpInformation = new OPERATION_INFOMATION[1];// has index of how many concurrent boards
	if(NULL == pOpInformation)
	{
		printf(_T("Lack of Memory!!!."));
		return -1;
	}
	infoOp.pOperationInfo = pOpInformation;


	printf("Ready to flash.\n");

	ret=MfgLib_InitializeOperation(lib);
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("init op Failed code# %d \n",ret);
		return -1;
	}
	ret= MfgLib_GetOperationInformation(lib,&infoOp);
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("Get op info  Failed code# %d \n",ret);
		return -1;
	}     

	ret=MfgLib_StartOperation(lib,infoOp.pOperationInfo[0].OperationID);
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("start op Failed code# %d \n",ret);
		return -1;
	}

	ret=MfgLib_RegisterCallbackFunction(lib, OperateResult, updateUI);

	ret=MfgLib_UninitializeOperation(lib);
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("init op Failed %d\n",ret);
		return -1;
	} 



	ret=MfgLib_DestoryInstanceHandle(lib);
	if( ret!=MFGLIB_ERROR_SUCCESS){
		printf("DestroyInstanceHandle failed  %d \n",ret);
		return -1;
	}
	ret=MfgLib_Uninitialize();
	if(ret!=MFGLIB_ERROR_SUCCESS){
		printf("Uninitialize failed  %d  \n",ret);
		return -1;
	}
	return 0;
}
