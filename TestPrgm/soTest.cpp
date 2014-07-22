#include <stdio.h>
#include "MfgToolLib_Export.h"

int main (int argc,char * argv[]){
    INSTANCE_HANDLE lib;

    foo();
    if(MfgLib_Initialize()!=MFGLIB_ERROR_SUCCESS){
	printf("Failed to initialize MfgLib");
	return -1;
    }
    if (MfgLib_CreateInstanceHandle(&lib)!=MFGLIB_ERROR_SUCCESS){
	printf("CreateInstanceHandle failed");
	return -1;
    }
    if( MfgLib_DestoryInstanceHandle(lib)!=MFGLIB_ERROR_SUCCESS){
	printf("DestroyInstanceHandle failed");
        return -1;
    }
    if(MfgLib_Uninitialize()!=MFGLIB_ERROR_SUCCESS){
	printf("Uninitialize failed");
        return -1;
    }
    return 0;

}
