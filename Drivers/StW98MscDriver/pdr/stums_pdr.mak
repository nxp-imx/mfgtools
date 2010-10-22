!IF "$(CFG)" == ""
CFG=Stums_pdr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Stums_pdr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Stums_pdr - Win32 Release" && "$(CFG)" != "Stums_pdr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Stums_pdr.mak" CFG="Stums_pdr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Stums_pdr - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Stums_pdr - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

ASM="$(W98DDK)\bin\win98\ml.exe"
CPP="$(MSVC60)\VC98\Bin\cl.exe"
LINK32="$(MSVC60)\VC98\Bin\link.exe"

!IF  "$(CFG)" == "Stums_pdr - Win32 Release"
OUTDIR=.\Release
INTDIR=.\Release
BINDIR=.\..\..\..\Release\Drivers

ALL : "$(BINDIR)\StUmsPdr.pdr" "$(BINDIR)\StUmsPdr.cat"

CLEAN :
	-@erase /q "$(OUTDIR)\*.*"
	-rmdir  "$(OUTDIR)"
	-@erase  "$(BINDIR)\StUmsPdr.pdr"
	-@erase  "$(BINDIR)\StUmsPdr.cat"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(BINDIR)" :
    if not exist "$(BINDIR)/$(NULL)" mkdir "$(BINDIR)"

CPP_PROJ=/nologo -Zp -Gs -c -DIS_32 -Zl -Zi -Od -DDEBLEVEL=0 -Fd"$(OUTDIR)\\" -Fo"$(OUTDIR)\\" /D "IS_32" /I "$(W98DDK)\inc\win98" /I "$(W98DDK)\src\block\inc"
ASM_PROJ=/D "IS_32" /W2 /c /Cx /D "MASM6" /D "DEBLEVEL=0" /I "$(W98DDK)\inc\win98" /Fo "$(OUTDIR)\\"

LINK32_FLAGS=/debug:none /pdb:none /machine:I386 /DEF:umsspdr.def /LIBPATH:"$(W98DDK)\lib\win98" -ignore:4078 -ignore:4039 -ignore:4070 -VXD "$(W98DDK)\lib\i386\free\vxdwraps.clb" -OUT:$(OUTDIR)\StUmsPdr.pdr
LINK32_OBJS= \
	"$(OUTDIR)\umssctl.obj" \
	"$(OUTDIR)\umssaer.obj" \
	"$(OUTDIR)\umssdbg.obj" \
	"$(OUTDIR)\umssio.obj" \
	"$(OUTDIR)\umsslist.obj" \
	"$(OUTDIR)\umsspdr.obj"

"$(OUTDIR)\StUmsPdr.pdr" : $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

"$(BINDIR)\StUmsPdr.pdr" : "$(BINDIR)" "$(OUTDIR)\StUmsPdr.pdr"
	copy "$(OUTDIR)\StUmsPdr.pdr" "$(BINDIR)"

"$(OUTDIR)\StUmsPdr.cat" :
	"$(MSVC70)\common7\tools\bin\makecat" -v <<StUmsPdr.cdf
[CatalogHeader]
Name=StUmsPdr.cat
ResultDir=$(OUTDIR)
PublicVersion=5.131.3668.0

[CatalogFiles]
StUmsPdr.pdr=$(OUTDIR)\StUmsPdr.pdr
<<

"$(BINDIR)\StUmsPdr.cat" : "$(BINDIR)"   "$(OUTDIR)\StUmsPdr.cat"
	copy "$(OUTDIR)\StUmsPdr.cat" "$(BINDIR)"


!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
BINDIR=.\..\..\..\Release\Drivers\Debug

ALL : "$(BINDIR)\StUmsPdr.pdr" "$(BINDIR)\StUmsPdr.nms" "$(BINDIR)\StUmsPdr.cat"

CLEAN :
	-@erase /q "$(OUTDIR)\*.*"
	-rmdir   "$(OUTDIR)"
	-@erase  "$(BINDIR)\StUmsPdr.pdr"
	-@erase  "$(BINDIR)\StUmsPdr.cat"
	-@erase  "$(BINDIR)\StUmsPdr.bsc"
	-@erase  "$(BINDIR)\StUmsPdr.nms"


"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(BINDIR)" :
    if not exist "$(BINDIR)/$(NULL)" mkdir "$(BINDIR)"

CPP_PROJ=/nologo -Zp -Gs -c -DIS_32 -Zl -Zi -Od -DDEBLEVEL=1 -DDEBUG -Fd"$(OUTDIR)\\" -Fo"$(OUTDIR)\\" /D "IS_32" /I "$(W98DDK)\inc\win98" /I "$(W98DDK)\src\block\inc"
ASM_PROJ=-Zi /D "IS_32" /W2 -Zd /c /Cx /D "MASM6" /D "DEBLEVEL=1" /I "$(W98DDK)\inc\win98" /Fo "$(OUTDIR)\\"

BSC32="$(MSVC60)\VC98\Bin\bscmake.exe"
BSC32_FLAGS=/nologo /o "$(OUTDIR)\StUmsPdr.bsc" 
BSC32_SBRS= \
	"$(OUTDIR)\umssaer.sbr" \
	"$(OUTDIR)\umssdbg.sbr" \
	"$(OUTDIR)\umssio.sbr" \
	"$(OUTDIR)\umsslist.sbr" \
	"$(OUTDIR)\umsspdr.sbr"

"$(OUTDIR)\StUmsPdr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<
	
"$(BINDIR)\StUmsPdr.bsc" : "$(BINDIR)" "$(OUTDIR)\StUmsPdr.bsc"
	copy "$(OUTDIR)\StUmsPdr.bsc" "$(BINDIR)"

LINK32_FLAGS=/debug -DEBUGTYPE:CV /machine:I386 /DEF:umsspdr.def /LIBPATH:"$(W98DDK)\lib\win98" -ignore:4078 -ignore:4039 -ignore:4070 -VXD "$(W98DDK)\lib\i386\free\vxdwraps.clb" -OUT:$(OUTDIR)\StUmsPdr.pdr	-MAP:$(OUTDIR)\StUmsPdr.map /pdb:"$(OUTDIR)\StUmsPdr.pdb"
LINK32_OBJS= \
	"$(OUTDIR)\umssctl.obj" \
	"$(OUTDIR)\umssaer.obj" \
	"$(OUTDIR)\umssdbg.obj" \
	"$(OUTDIR)\umssio.obj" \
	"$(OUTDIR)\umsslist.obj" \
	"$(OUTDIR)\umsspdr.obj"

"$(OUTDIR)\StUmsPdr.pdr" : $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

"$(BINDIR)\StUmsPdr.pdr" : "$(BINDIR)" "$(OUTDIR)\StUmsPdr.pdr"
	copy "$(OUTDIR)\StUmsPdr.pdr" "$(BINDIR)"

"$(OUTDIR)\StUmsPdr.nms" : "$(OUTDIR)\StUmsPdr.pdr"
	$(SOFTICE)\nmsym /trans:source;package;always $(OUTDIR)\StUmsPdr.pdr 
	
"$(BINDIR)\StUmsPdr.nms" : "$(BINDIR)" "$(OUTDIR)\StUmsPdr.nms"
	copy "$(OUTDIR)\StUmsPdr.nms" "$(BINDIR)"

"$(OUTDIR)\StUmsPdr.cat" :
	"$(MSVC70)\common7\tools\bin\makecat" -v <<StUmsPdr.cdf
[CatalogHeader]
Name=StUmsPdr.cat
ResultDir=$(OUTDIR)
PublicVersion=5.131.3668.0

[CatalogFiles]
StUmsPdr.pdr=$(OUTDIR)\StUmsPdr.pdr
<<

"$(BINDIR)\StUmsPdr.cat" : "$(BINDIR)"   "$(OUTDIR)\StUmsPdr.cat"
	copy "$(OUTDIR)\StUmsPdr.cat" "$(BINDIR)"

!ENDIF 

!IF "$(CFG)" == "Stums_pdr - Win32 Release" || "$(CFG)" == "Stums_pdr - Win32 Debug"

SOURCE=.\umssctl.asm
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umssctl.obj" : $(SOURCE) "$(OUTDIR)"
	$(ASM) $(ASM_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umssctl.obj"	"$(OUTDIR)\umssctl.sbr" : $(SOURCE) "$(OUTDIR)"
	$(ASM) $(ASM_PROJ) $(SOURCE)

!ENDIF 

SOURCE=.\umssaer.c
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umssaer.obj" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umssaer.obj"	"$(OUTDIR)\umssaer.sbr" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

SOURCE=.\umssdbg.c
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umssdbg.obj" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umssdbg.obj"	"$(OUTDIR)\umssdbg.sbr" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

SOURCE=.\umssio.c
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umssio.obj" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umssio.obj"	"$(OUTDIR)\umssio.sbr" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

SOURCE=.\umsslist.c
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umsslist.obj" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umsslist.obj"	"$(OUTDIR)\umsslist.sbr" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

SOURCE=.\umsspdr.c
!IF  "$(CFG)" == "Stums_pdr - Win32 Release"

"$(OUTDIR)\umsspdr.obj" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "Stums_pdr - Win32 Debug"

"$(OUTDIR)\umsspdr.obj"	"$(OUTDIR)\umsspdr.sbr" : $(SOURCE) "$(OUTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 


!ENDIF 

