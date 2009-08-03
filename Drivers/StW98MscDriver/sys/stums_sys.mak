!IF "$(CFG)" == ""
CFG=Stums_sys - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Stums_sys - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Stums_sys - Win32 Release" && "$(CFG)" != "Stums_sys - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Stums_sys.mak" CFG="Stums_sys - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Stums_sys - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Stums_sys - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP="$(MSVC60)\VC98\Bin\cl.exe"
RSC="$(W98DDK)\bin\win98\rc.exe"
LINK32="$(MSVC60)\VC98\Bin\link.exe"
LIB="$(MSVC60)\VC98\Bin\lib.exe"

!IF  "$(CFG)" == "Stums_sys - Win32 Release"
OUTDIR=.\Release
INTDIR=.\Release
BINDIR=.\..\..\..\Release\Drivers

ALL : "$(BINDIR)\Stums.sys" "$(BINDIR)\Stums.cat" "$(BINDIR)\Stums.inf"


CLEAN :
	-@erase /q "$(OUTDIR)\*.*"
	-rmdir  "$(OUTDIR)"
	-@erase  "$(BINDIR)\Stums.sys"
	-@erase  "$(BINDIR)\Stums.inf"
	-@erase  "$(BINDIR)\Stums.cat"


"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(BINDIR)" :
    if not exist "$(BINDIR)/$(NULL)" mkdir "$(BINDIR)"

CPP_PROJ=-nologo -I.. -I"$(W98DDK)\inc" -I"$(W98DDK)\inc\win98" -I"$(MSVC60)\VC98\include" -D_X86_=1 -Di386=1  -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO=1    -DNDEBUG -D_DLL=1 -D_IDWBUILD    /c /Zel /Zp8 /Gy -cbstring /W3 /Gz  /QIfdiv- /QIf  /Gi- /Gm- /GX- /GR- /GF   /Oxs /Oy   -FI"$(W98DDK)\inc\win98\warning.h" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Stums.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c

LIB_OBJS= \
	"$(INTDIR)\umsspnp.obj" \
	"$(INTDIR)\cbi.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\bulkonly.obj" \
	"$(INTDIR)\umssdbg.obj" \
	"$(INTDIR)\umss.obj" \
	"$(INTDIR)\umsspwr.obj" \
	"$(INTDIR)\ios.obj" \
	"$(INTDIR)\umss.res"

LIB_PROJ=-out:"$(INTDIR)\STUMS.lib"	-debugtype:cv -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 -nodefaultlib -machine:i386 -def:STUMS.def $(LIB_OBJS)

RSC_PROJ=-l 409  -fo $(INTDIR)\umss.tmp -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400 -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO=1 -DNDEBUG -D_DLL=1 -D_IDWBUILD -I"$(W98DDK)\inc" -I"$(W98DDK)\src\usb\inc" -I"$(W98DDK)\src\wdm\usb\inc" -I"$(W98DDK)\src\block\inc" -I..\
	
"$(INTDIR)\STUMS.exp" : $(LIB_OBJS)
	$(LIB) $(LIB_PROJ)

LINK32_FLAGS=ntoskrnl.lib hal.lib usbd.lib /base:0x10000 /version:4.00 /entry:DriverEntry@8 /incremental:no /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Stums.sys" /libpath:"$(W98DDK)\lib\i386\free" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -FORCE:MULTIPLE -RELEASE -FULLBUILD -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 -MERGE:.rdata=.text -optidata -driver -align:0x20 -osversion:4.00 -subsystem:native,4.00 -debug:NONE -debugtype:MAP,COFF -map -PDB:NONE
LINK32_OBJS= \
	"$(INTDIR)\STUMS.exp" \
	"$(INTDIR)\umsspnp.obj" \
	"$(INTDIR)\cbi.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\bulkonly.obj" \
	"$(INTDIR)\umssdbg.obj" \
	"$(INTDIR)\umss.obj" \
	"$(INTDIR)\umsspwr.obj" \
	"$(INTDIR)\ios.obj" \
	"$(INTDIR)\umss.res"

"$(OUTDIR)\Stums.sys" : $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

"$(BINDIR)\Stums.sys" : "$(BINDIR)" "$(OUTDIR)\Stums.sys"
	copy "$(OUTDIR)\Stums.sys" "$(BINDIR)"

"$(OUTDIR)\Stums.cat" :
	"$(MSVC70)\common7\tools\bin\makecat" -v <<Stums.cdf
[CatalogHeader]
Name=Stums.cat
ResultDir=$(OUTDIR)
PublicVersion=5.131.3668.0

[CatalogFiles]
Stums.sys=$(OUTDIR)\Stums.sys
<<

"$(BINDIR)\Stums.cat" : "$(BINDIR)" "$(OUTDIR)\Stums.cat"
	copy "$(OUTDIR)\Stums.cat" "$(BINDIR)"

$(BINDIR)\Stums.inf : ".\..\Stums.inf"
	copy ".\..\Stums.inf" "$(BINDIR)"

!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
BINDIR=.\..\..\..\Release\Drivers\Debug

ALL : "$(BINDIR)\Stums.sys" "$(BINDIR)\Stums.bsc" "$(BINDIR)\Stums.nms" "$(BINDIR)\Stums.cat" "$(BINDIR)\Stums.inf"


CLEAN :
	-@erase /q "$(OUTDIR)\*.*"
	-@rmdir  "$(OUTDIR)"
	-@erase  "$(BINDIR)\Stums.sys"
	-@erase  "$(BINDIR)\Stums.inf"
	-@erase  "$(BINDIR)\Stums.cat"
	-@erase  "$(BINDIR)\Stums.bsc"
	-@erase  "$(BINDIR)\Stums.nms"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(BINDIR)" :
    if not exist "$(BINDIR)/$(NULL)" mkdir "$(BINDIR)"

CPP_PROJ=-nologo -I.. -I"$(W98DDK)\inc" -I"$(W98DDK)\inc\win98" -I"$(MSVC60)\VC98\include" -D_X86_=1 -Di386=1  -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO=1    -DDEBUG -D_DLL=1 -D_IDWBUILD    /c /Zel /Z7 /Zp8 /Gy -cbstring /W3 /Gz  /QIfdiv- /QIf  /Gi- /Gm- /GX- /GR- /GF   /Oxs /Oi   -FI"$(W98DDK)\inc\win98\warning.h" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Stums.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c

LIB_OBJS= \
	"$(INTDIR)\umsspnp.obj" \
	"$(INTDIR)\cbi.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\bulkonly.obj" \
	"$(INTDIR)\umssdbg.obj" \
	"$(INTDIR)\umss.obj" \
	"$(INTDIR)\umsspwr.obj" \
	"$(INTDIR)\ios.obj" \
	"$(INTDIR)\umss.res"

LIB_PROJ=-out:"$(INTDIR)\STUMS.lib"	-debugtype:cv -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 -nodefaultlib -machine:i386 -def:STUMS.def $(LIB_OBJS)

RSC_PROJ=/l 0x409 /fo"$(INTDIR)\umss.res" /d "_DEBUG" /d WIN32_LEAN_AND_MEAN=1

"$(INTDIR)\STUMS.exp" : $(LIB_OBJS)
	$(LIB) $(LIB_PROJ)

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Stums.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cbi.sbr" \
	"$(INTDIR)\globals.sbr" \
	"$(INTDIR)\bulkonly.sbr" \
	"$(INTDIR)\umssdbg.sbr" \
	"$(INTDIR)\umss.sbr" \
	"$(INTDIR)\umsspnp.sbr" \
	"$(INTDIR)\umsspwr.sbr" \
	"$(INTDIR)\ios.sbr"

"$(OUTDIR)\Stums.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

"$(BINDIR)\Stums.bsc" : "$(BINDIR)" "$(OUTDIR)\Stums.bsc"
	copy "$(OUTDIR)\Stums.bsc" "$(BINDIR)"

LINK32_FLAGS=ntoskrnl.lib hal.lib usbd.lib /nologo /base:"0x10000" /version:5.0 /stack:0x40000,0x1000 /entry:"DriverEntry" /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Stums.pdb" /map:"$(INTDIR)\Stums.map" /debug /debugtype:both /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Stums.sys" /pdbtype:sept /libpath:"$(W98DDK)\lib\i386\checked" /libpath:"$(W98DDK)\lib\i386\free" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -FORCE:MULTIPLE -RELEASE -FULLBUILD -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 -MERGE:.rdata=.text -optidata -driver -align:0x20 -osversion:5.00 -subsystem:native,1.10 -driver:WDM -debug:FULL 
LINK32_OBJS= \
	"$(INTDIR)\STUMS.exp" \
	"$(INTDIR)\umsspnp.obj" \
	"$(INTDIR)\cbi.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\bulkonly.obj" \
	"$(INTDIR)\umssdbg.obj" \
	"$(INTDIR)\umss.obj" \
	"$(INTDIR)\umsspwr.obj" \
	"$(INTDIR)\ios.obj" \
	"$(INTDIR)\umss.res"

"$(OUTDIR)\Stums.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

"$(BINDIR)\Stums.sys" : "$(BINDIR)" "$(OUTDIR)\Stums.sys"
	copy "$(OUTDIR)\Stums.sys" "$(BINDIR)"

"$(OUTDIR)\Stums.nms" : "$(OUTDIR)\Stums.sys"
	$(SOFTICE)\nmsym /trans:source;package;always $(OUTDIR)\Stums.sys 
	
"$(BINDIR)\Stums.nms" : "$(BINDIR)" "$(OUTDIR)\Stums.nms"
	copy "$(OUTDIR)\Stums.nms" "$(BINDIR)"

"$(OUTDIR)\Stums.cat" :
	"$(MSVC70)\common7\tools\bin\makecat" -v <<Stums.cdf
[CatalogHeader]
Name=StUms.cat
ResultDir=$(OUTDIR)
PublicVersion=5.131.3668.0

[CatalogFiles]
StUms.sys=$(OUTDIR)\StUms.sys
<<

"$(BINDIR)\Stums.cat" : "$(BINDIR)" "$(OUTDIR)\Stums.cat"
	copy "$(OUTDIR)\Stums.cat" "$(BINDIR)"

$(BINDIR)\Stums.inf : ".\..\Stums.inf"
	copy ".\..\Stums.inf" "$(BINDIR)"

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

!IF "$(CFG)" == "Stums_sys - Win32 Release" || "$(CFG)" == "Stums_sys - Win32 Debug"

SOURCE=.\umsspnp.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\umsspnp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\umsspnp.obj"	"$(INTDIR)\umsspnp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\umssdbg.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\umssdbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\umssdbg.obj"	"$(INTDIR)\umssdbg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\umss.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\umss.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\umss.obj"	"$(INTDIR)\umss.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\umss.rc

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\umss.res" : $(SOURCE) "$(INTDIR)" ../../../Customization/version.h
#	$(RSC) $(RSC_PROJ) $(SOURCE)
	$(RSC) /fo "$(INTDIR)\umss.res" -I../../../Customization -I"$(W98DDK)\inc" -I"$(W98DDK)\inc\win98" -I"$(MSVC60)\VC98\include" $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\umss.res" : $(SOURCE) "$(INTDIR)" ../../../Customization/version.h
	$(RSC) /fo "$(INTDIR)\umss.res" -I../../../Customization -I"$(W98DDK)\inc" -I"$(W98DDK)\inc\win98" -I"$(MSVC60)\VC98\include" $(SOURCE)


!ENDIF 

SOURCE=.\umsspwr.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\umsspwr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\umsspwr.obj"	"$(INTDIR)\umsspwr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ios.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\ios.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\ios.obj"	"$(INTDIR)\ios.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\bulkonly.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\bulkonly.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\bulkonly.obj"	"$(INTDIR)\bulkonly.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\cbi.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\cbi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\cbi.obj"	"$(INTDIR)\cbi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\globals.c

!IF  "$(CFG)" == "Stums_sys - Win32 Release"


"$(INTDIR)\globals.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Stums_sys - Win32 Debug"


"$(INTDIR)\globals.obj"	"$(INTDIR)\globals.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

