#include <stdio.h>
#include "MfgToolLib_Export.h"

int main (int argc,char * argv[]){
    INSTANCE_HANDLE lib;
    printf("hello\n");
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
    std::map<CString, CString> m_uclKeywords;
    std::map<CString, CString>::const_iterator it;
    m_uclKeywords["board"] = "sabresd";
    m_uclKeywords["mmc"] = "0";
    m_uclKeywords["sxuboot"]="17x17arm2";
    m_uclKeywords["sxdtb"]="17x17-arm2";
    for ( it=m_uclKeywords.begin(); it!=m_uclKeywords.end(); ++it )
    {
	CString key = it->first;
	CString value = it->second;
	MfgLib_SetUCLKeyWord((char*)key.GetBuffer(),(char*) value.GetBuffer());
    }



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


    printf(" this is a test at the top level\n");




    ret=MfgLib_InitializeOperation(lib);
    if(ret!=MFGLIB_ERROR_SUCCESS){
	printf("init op Failed code# %d \n",ret);
	return -1;
    } 
    while(1){
	sleep(3);
	printf("tick\n");
    }
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
