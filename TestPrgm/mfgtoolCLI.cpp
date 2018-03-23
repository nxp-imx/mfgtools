/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <cstring>
#include <fstream>
#include <string>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include "MfgToolLib_Export.h"
INSTANCE_HANDLE lib;
int TERM_WIDTH = 80;
int BAR_WIDTH = 0.5 * TERM_WIDTH;
unsigned long opIDs[4] = {0, 0, 0, 0};
bool finished[4] = {true, true, true, true};
double iperc[4] = {0, 0, 0, 0};
bool bDoExit = true;
int ports = 0;
int boards = 0;

std::string curCommand[4] = {"Starting", "Starting", "Starting", "Starting"}; 
std::string oldCommand[4] = {"", "", "", ""};
std::string trim(std::string str)
{
	size_t first = str.find_first_not_of(' ');
	size_t last = str.find_last_not_of(' ');
	if (last > 0.8 * (BAR_WIDTH))
		last = 0.8 * (BAR_WIDTH);
	return str.substr(first, (last-first+1));
}
void exitLibrary()
{
	int ret;
	std::cout << std::endl << std::endl;
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
void updateUI(OPERATE_RESULT* puiInfo)
{
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	TERM_WIDTH = w.ws_col;
	BAR_WIDTH = TERM_WIDTH * 0.5 - (10 * boards) ;

	int pval = -1;
	
	for (int i = 0; i < ports; i++)
	{
		if (opIDs[i] == puiInfo->OperationID)
		{
			pval = i;
		}
	}

	if (pval == -1)
	{
		for (int i = 0; i < ports; i++)
		{
			if (opIDs[i] == 0)
			{
				opIDs[i] = puiInfo->OperationID;
				pval = boards;
				finished[boards] = false;
				boards++;
				return;
			}
		}
	}
	std::string cmdInfo = std::string((char*)puiInfo->cmdInfo);
	if (!cmdInfo.empty())
	{
		cmdInfo = trim(cmdInfo);
		oldCommand[pval] = std::string(curCommand[pval]);
		curCommand[pval] = std::string(cmdInfo);
	}

	if (bDoExit)
	{
		finished[pval] = puiInfo->bFinished;

		bool bExit = true;
		for (int b = 0; b < boards; b++)
		{
			if (finished[b])
			{
				if (opIDs[b] != -1)
				{
					int ret = MfgLib_StopOperation(opIDs[b], lib);
					if(ret != MFGLIB_ERROR_SUCCESS)
					{
						printf(_T("no brakes Could not stop operation\n"));
						return -1;
					}
					opIDs[b] = -1;
				}
			}
			else
			{
				bExit = false;
			}
		}
		if (bExit)
			exitLibrary();
	}


	for (int b = 0; b < boards; b++)
	{
		std::cout << "\r";
		printf("%c[2K", 27);
		if (oldCommand[b].compare(curCommand[b]) != 0)
		{
			std::cout << " " << b + 1 << " " << std::setw(10) << "Done!" << " [";
			for (int i = 0; i < BAR_WIDTH; i++)
			{
				std::cout << "=";
			}
			std::cout << ">] " << std::setw(0.8 * (BAR_WIDTH)) << oldCommand[b] << std::endl;
		}
		else
		{
			float percentage = (float) puiInfo->DoneWithinCommand / puiInfo->TotalWithinCommand;
			if (percentage != percentage)
			{
				percentage = 0;
			}
			if (b == pval)
			{
				iperc[b] = percentage;
				if (iperc[b] < 0)
					iperc[b] = 0;
				if (iperc[b] > 1)
					iperc[b] = 1;
			}
			std::cout << " " << b + 1 << " " << std::setw(9) << (int) (100 * iperc[b]) << "% [";
			int bars = iperc[b] * (BAR_WIDTH);
			for (int i = 0; i < bars; i ++)
			{
				std::cout << "=";
			}
			std::cout << ">";
			for (int i = 0; i < (BAR_WIDTH) - bars; i++)
			{
				std::cout << " ";
			}
			std::cout << "] " << std::setw(0.8 * (BAR_WIDTH)) << oldCommand[b] << std::endl;
		}
	}
	for (int b = 0; b < boards; b++)
	{
		printf("\x1B[1F");
	}
	std::cout.flush();
	return;
}
int main (int argc,char* argv[]){
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	TERM_WIDTH = w.ws_col;
	BAR_WIDTH = 0.8 * TERM_WIDTH;

	std::string newpath = "./";
	std::string mylist = "SabreSD";
	std::string myprofile = "Linux";

	std::map<CString, CString> m_uclKeywords;
	std::map<CString, CString>::const_iterator it;


	std::ifstream file("cfg.ini");
	if (file)
	{
		std::string str;
		int state = 0;
		while (std::getline(file, str))
		{
			size_t locBreak = str.find(" ");
			int strip = 3;
			if (locBreak == std::string::npos)
			{
				locBreak = str.find("=");
				strip = 1;
			}
			if (str.find("=") != std::string::npos)
			{
				switch (state)
				{
					case 0:
						myprofile = std::string(str.substr(locBreak + strip, str.size() - locBreak));
						break;
					case 1:
						break;
					case 2:
						mylist = std::string(str.substr(locBreak + strip, str.size() - locBreak));
						break;
					case 3:
						m_uclKeywords[str.substr(0, locBreak)] = str.substr(locBreak + strip, str.size() - locBreak);
						break;
				}
			}
			if (str.compare("[profiles]") == 0)
				state = 0;
			else if (str.compare("[platform]") == 0)
				state = 1;
			else if (str.compare("[LIST]") == 0)
				state = 2;
			else if (str.compare("[variable]") == 0)
				state = 3;
		}
	}

	std::ifstream ufile("UICfg.ini");
	if (ufile)
	{
		std::string str;
		while (std::getline(ufile, str))
		{
			size_t locBreak = str.find(" ");
			int strip = 3;
			if (locBreak == std::string::npos)
			{
				locBreak = str.find("=");
				strip = 1;
			}
			if (str.find("=") != std::string::npos)
			{
				ports = std::stoi(str.substr(locBreak + strip, str.size() - locBreak));
			}
		}
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
		else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--profile") == 0)
		{
			myprofile = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--ports") == 0)
		{
			ports = atoi(argv[i+1]);
			i++;
		}
		else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--profilepath") == 0)
		{
			newpath = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0)
		{
			mylist = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--noexit") == 0)
		{
			bDoExit = false;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			std::cout << "Usage: [program] [arguments] [settings]=[values]" << std::endl;
			std::cout << std::setw(6) << "-s" << "  --setting      " << "Specify any UCL keywords." << std::endl;
			std::cout << std::setw(6) << "-p" << "  --ports        " << "Specify the number of boards connected." << std::endl;
			std::cout << std::setw(6) << "-o" << "  --profilepath  " << "Specify path to Profiles directory." << std::endl;
			std::cout << std::setw(6) << "-c" << "  --profile      " << "Specify the profile to use." << std::endl;
			std::cout << std::setw(6) << "-l" << "  --list         " << "Specify command list." << std::endl;
			std::cout << std::setw(6) << "-x" << "  --noexit       " << "Wait for further connections upon finishing all operations." << std::endl;
			std::cout << std::setw(6) << "-h" << "  --help         " << "Display this help information." << std::endl;
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
		MfgLib_SetUCLKeyWord(key.GetBuffer(), value.GetBuffer());
	}


	ret = MfgLib_SetProfilePath(lib, (BYTE_t *) newpath.c_str());
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("Set Profile path failed\n"));
		return -1;
	}

	//set profile and list
	ret = MfgLib_SetProfileName(lib,(BYTE_t *) myprofile.c_str());
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("Set Profile name failed\n"));
		return -1;
	}
	ret = MfgLib_SetListName(lib, (BYTE_t *) mylist.c_str());
	if(ret != MFGLIB_ERROR_SUCCESS)
	{
		printf(_T("Set List name failed\n"));
		return -1;
	}
	ret = MfgLib_SetMaxBoardNumber(lib, ports);
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

	for (int i = 0; i < ports; i++)
	{
		ret=MfgLib_StartOperation(lib,infoOp.pOperationInfo[i].OperationID);
		if(ret!=MFGLIB_ERROR_SUCCESS){
			printf("start op Failed code# %d \n",ret);
			return -1;
		}
	}

	ret=MfgLib_RegisterCallbackFunction(lib, OperateResult, updateUI);

	while(true)
	{
		sleep(3);
	}

}
