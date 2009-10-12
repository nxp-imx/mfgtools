/*
 * W i n D r i v e r
 * =================
 *
 * Header file for Windows 95/98/ME/NT/NTE/Win2000/WinXP/WinCE/Linux/Solaris/VxWorks.
 * FOR DETAILS ON THE WinDriver FUNCTIONS, PLEASE SEE THE WinDriver MANUAL
 * OR INCLUDED HELP FILES.
 *
 * This file may not be distributed -- it may only be used for development
 * or evaluation purposes. (see \WinDriver\docs\license.txt for details).
 *
 * Web site: http://www.jungo.com
 * Email:    support@jungo.com
 *
 * (C) Jungo 1997 - 2003
 */
#ifndef _WINDRVR_H_
#define _WINDRVR_H_
#if defined(__cplusplus)
    extern "C" {
#endif

#include "wd_ver.h"

// these lines break standard VxWorks builds. remove the -asni flag from the
// compiler flags (from the makefile or the Tornado IDE) to compile WinDriver
// related source code.
#if defined(WIN32DLL)
    #define DLLCALLCONV _stdcall
#else
    #define DLLCALLCONV
#endif
        
#if defined(VXWORKS)
    #define WD_PROD_NAME "DriverBuilder"
#else
    #define WD_PROD_NAME "WinDriver"
#endif

#if defined(VXWORKS)
    #if defined(MBX860)
        #define WD_CPU_SPEC " PPC860"
    #elif defined(MCP750)
        #define WD_CPU_SPEC " PPC750"
    #elif defined(x86)
        #define WD_CPU_SPEC " X86"
    #else
        #define WD_CPU_SPEC ""
    #endif
#else
    #if defined(SPARC)
        #define WD_CPU_SPEC " Sparc"
    #elif defined(WINCE)
        #define WD_CPU_SPEC ""
    #else
        #define WD_CPU_SPEC " X86"
    #endif
#endif

#if defined(WINNT)
    #define WD_FILE_FORMAT " SYS"
#elif defined(WIN95)
    #define WD_FILE_FORMAT " VXD"
#else
    #define WD_FILE_FORMAT ""
#endif

#define WD_VER_STR  WD_PROD_NAME " v" WD_VERSION_STR " Jungo (c) 1997 - 2003 Build Date: " __DATE__ WD_CPU_SPEC WD_FILE_FORMAT

#if !defined(UNIX) && (defined(LINUX) || defined(SOLARIS) || defined(VXWORKS))
    #define UNIX
#endif

#if !defined(SPARC) && (defined(__sparc__) || defined (__sparc) || \
        defined(sparc))
    #define SPARC
#endif

#if !defined(WIN32) && (defined(WINCE) || defined(WIN95) || defined(WINNT))
    #define WIN32
#endif

#if defined(_WIN32_WCE) && !defined(WINCE)
    #define WINCE
#endif

#if !defined(x86) && ( defined(LINUX) || (defined(WIN32) && !defined(WINCE) && !defined(_ALPHA_) ))
    #define x86
#endif

#if defined(_KERNEL) && !defined(__KERNEL__)
    #define __KERNEL__
#endif

#if defined( __KERNEL__) && !defined(_KERNEL)
    #define _KERNEL
#endif

#if !defined(WIN32) && !defined(WINCE) && !defined(UNIX)
    #define WIN32
#endif

#if  !defined(POWERPC) && defined(VXWORKS) && !defined(x86)
    #define POWERPC
#endif
#if !defined(_BIGENDIAN) && (defined (SPARC) || defined(POWERPC))
    #define _BIGENDIAN
#endif

#if defined(UNIX)
    #if !defined(__P_TYPES__)
        #if !defined(VXWORKS)
            typedef void VOID;
            typedef unsigned char UCHAR;
            typedef unsigned short USHORT;
            typedef unsigned int UINT;
            typedef unsigned long ULONG;
            typedef ULONG BOOL;
        #endif
        typedef void *PVOID;
        typedef unsigned char *PBYTE;
        typedef char CHAR;
        typedef char *PCHAR;
        typedef unsigned short *PWORD;
        typedef unsigned long DWORD, *PDWORD;
        typedef PVOID HANDLE;
    #endif

    #if !defined(__KERNEL__)
        #include <string.h>
        #include <ctype.h>
        #include <stdlib.h>
    #endif
    #define TRUE 1
    #define FALSE 0
    #define __cdecl
    #define WINAPI

    #if defined(__KERNEL__)
        #if defined(LINUX)
            #include <linux/types.h>
            #include <linux/string.h>
        #endif
    #else
        #include <unistd.h>
        #if defined(LINUX)
                #include <sys/ioctl.h> /* for BSD ioctl() */
            #include <unistd.h>
        #else
            #include <unistd.h> /* for SVR4 ioctl()*/
        #endif
        #if defined(VXWORKS)
            #include <vxWorks.h>
            #undef SPARC /* defined in vxworks.h */
            #include <string.h>
            #include <memLib.h>
            #include <stdlib.h>
            #include <taskLib.h>
            #include <ioLib.h>
            #include <iosLib.h>
            #include <taskLib.h>
            #include <semLib.h>
            #include <timers.h>
        #endif
        #include <sys/types.h>
        #include <sys/stat.h>
        #include <fcntl.h>
    #endif
#elif defined(WINCE)
    #include <windows.h>
    #include <winioctl.h>
    typedef char CHAR;
#elif defined(WIN32)
    #if defined(__KERNEL__)
        int sprintf(char *buffer, const char *format, ...);
    #else
        #include <windows.h>
        #include <winioctl.h>
    #endif
    #if defined(__KERNEL__) && defined(WIN32)
        int sprintf(char *buffer, const char *format, ...);
    #endif
    #if defined(__KERNEL__) && defined(WIN95) && !defined(__KERPLUG__)
        #include "kdstdlib.h"
    #endif
#endif

#if !defined(_WINDEF_)
    typedef unsigned char BYTE;
    typedef unsigned short int WORD;
#endif
#define KPTR DWORD

typedef enum
{
    CMD_NONE = 0,       // No command
    CMD_END = 1,        // End command

    RP_BYTE = 10,       // Read port byte
    RP_WORD = 11,       // Read port word
    RP_DWORD = 12,      // Read port dword
    WP_BYTE = 13,       // Write port byte
    WP_WORD = 14,       // Write port word
    WP_DWORD = 15,      // Write port dword

    RP_SBYTE = 20,      // Read port string byte
    RP_SWORD = 21,      // Read port string word
    RP_SDWORD = 22,     // Read port string dword
    WP_SBYTE = 23,      // Write port string byte
    WP_SWORD = 24,      // Write port string word
    WP_SDWORD = 25,     // Write port string dword

    RM_BYTE = 30,       // Read memory byte
    RM_WORD = 31,       // Read memory word
    RM_DWORD = 32,      // Read memory dword
    WM_BYTE = 33,       // Write memory byte
    WM_WORD = 34,       // Write memory word
    WM_DWORD = 35,      // Write memory dword

    RM_SBYTE = 40,      // Read memory string byte
    RM_SWORD = 41,      // Read memory string word
    RM_SDWORD = 42,     // Read memory string dword
    WM_SBYTE = 43,      // Write memory string byte
    WM_SWORD = 44,      // Write memory string word
    WM_SDWORD = 45,     // Write memory string dword
    RM_SQWORD = 46,     // Read memory string quad word
    WM_SQWORD = 47,     // Write memory string quad word
} WD_TRANSFER_CMD;

enum { WD_DMA_PAGES = 256 };

enum {
    DMA_KERNEL_BUFFER_ALLOC = 1, // The system allocates a contiguous buffer.
                          // The user does not need to supply linear_address.
    DMA_KBUF_BELOW_16M = 2, // If DMA_KERNEL_BUFFER_ALLOC if used,
                          // this will make sure it is under 16M.
    DMA_LARGE_BUFFER = 4, // If DMA_LARGE_BUFFER is used,
                          // the maximum number of pages are dwPages, and not
                          // WD_DMA_PAGES. If you lock a user buffer (not a kernel
                          // allocated buffer) that is larger than 1MB, then use this
                          // option and allocate memory for pages.
    DMA_ALLOW_CACHE = 8,  // allow caching of the memory for contiguous memory 
                          // allocation on Windows NT/2k/XP (not recommended!)
};

typedef struct
{
    KPTR pPhysicalAddr;   // Physical address of page.
    DWORD dwBytes;          // Size of page.
} WD_DMA_PAGE, WD_DMA_PAGE_V30;

typedef struct
{
    DWORD hDma;             // Handle of dma buffer.
    PVOID pUserAddr;        // Beginning of buffer.
    KPTR  pKernelAddr;      // Kernel mapping of kernel allocated buffer
    DWORD dwBytes;          // Size of buffer.
    DWORD dwOptions;        // Allocation options:
                            // DMA_KERNEL_BUFFER_ALLOC, DMA_KBUF_BELOW_16M, DMA_LARGE_BUFFER.
    DWORD dwPages;          // Number of pages in buffer.
    DWORD dwPad1;           // Reserved for internal use
    WD_DMA_PAGE Page[WD_DMA_PAGES];
} WD_DMA, WD_DMA_V30;

typedef struct
{
    KPTR dwPort;       // IO port for transfer or kernel memory address.
    DWORD cmdTrans;    // Transfer command WD_TRANSFER_CMD.

    // Parameters used for string transfers:
    DWORD dwBytes;     // For string transfer.
    DWORD fAutoinc;    // Transfer from one port/address
                       // or use incremental range of addresses.
    DWORD dwOptions;   // Must be 0.
    union
    {
        BYTE Byte;     // Use for 8 bit transfer.
        WORD Word;     // Use for 16 bit transfer.
        DWORD Dword;   // Use for 32 bit transfer.
        BYTE Reserved[8];   //  Reserved do not use
        PVOID pBuffer; // Use for string transfer.
    } Data;
} WD_TRANSFER, WD_TRANSFER_V30;


enum { INTERRUPT_LEVEL_SENSITIVE = 1 };
enum { INTERRUPT_CMD_COPY = 2 };
enum { INTERRUPT_CE_INT_ID = 4 };

typedef struct
{
    DWORD hKernelPlugIn;
    DWORD dwMessage;
    PVOID pData;
    DWORD dwResult;
} WD_KERNEL_PLUGIN_CALL, WD_KERNEL_PLUGIN_CALL_V40;

enum
{ 
    INTERRUPT_STOPPED = 1,
    INTERRUPT_INTERRUPTED,
};

typedef struct
{
    DWORD hInterrupt;    // Handle of interrupt.
    DWORD dwOptions;     // Interrupt options: can be INTERRUPT_CMD_COPY

    WD_TRANSFER *Cmd;    // Commands to do on interrupt.
    DWORD dwCmds;        // Number of commands.

    // For WD_IntEnable():
    WD_KERNEL_PLUGIN_CALL kpCall; // Kernel PlugIn call.
    DWORD fEnableOk;     // TRUE if interrupt was enabled (WD_IntEnable() succeed).

    // For WD_IntWait() and WD_IntCount():
    DWORD dwCounter;     // Number of interrupts received.
    DWORD dwLost;        // Number of interrupts not yet dealt with.
    DWORD fStopped;      // Was interrupt disabled during wait.
} WD_INTERRUPT, WD_INTERRUPT_V40;

typedef struct
{
    DWORD dwVer;
    CHAR cVer[128];
} WD_VERSION, WD_VERSION_V30;

enum
{
    LICENSE_DEMO = 0x1,
    LICENSE_WD = 0x4,
    LICENSE_IO = 0x8,
    LICENSE_MEM = 0x10,
    LICENSE_INT = 0x20,
    LICENSE_PCI = 0x40,
    LICENSE_DMA = 0x80,
    LICENSE_NT = 0x100,
    LICENSE_95 = 0x200,
    LICENSE_ISAPNP = 0x400,
    LICENSE_PCMCIA = 0x800,
    LICENSE_PCI_DUMP = 0x1000,
    LICENSE_MSG_GEN = 0x2000,
    LICENSE_MSG_EDU = 0x4000,
    LICENSE_MSG_INT = 0x8000,
    LICENSE_KER_PLUG = 0x10000,
    LICENSE_LINUX = 0x20000,
    LICENSE_CE = 0x80000,
    LICENSE_VXWORKS = 0x10000000,
    LICENSE_THIS_PC = 0x100000,
    LICENSE_WIZARD = 0x200000,
    LICENSE_KD = 0x400000,
    LICENSE_SOLARIS = 0x800000,
    LICENSE_CPU0 = 0x40000,
    LICENSE_CPU1 = 0x1000000,
    LICENSE_CPU2 = 0x2000000,
    LICENSE_CPU3 = 0x4000000,
    LICENSE_USB = 0x8000000,
};

enum
{
    LICENSE2_EVENT = 0x8,
    LICENSE2_WDLIB = 0x10,
};

enum
{
    LICENSE_OS_WITH_WIZARD = LICENSE_95 | LICENSE_NT,
    LICENSE_OS_WITHOUT_WIZARD = LICENSE_LINUX | LICENSE_CE | LICENSE_VXWORKS | LICENSE_SOLARIS,
};

enum
{
    LICENSE_CPU_ALL = LICENSE_CPU3 | LICENSE_CPU2 | LICENSE_CPU1 |
        LICENSE_CPU0,
    LICENSE_X86 = LICENSE_CPU0,
    LICENSE_ALPHA = LICENSE_CPU1,
    LICENSE_SPARC = LICENSE_CPU1 | LICENSE_CPU0,
    LICENSE_PPC = LICENSE_CPU2,
};

#define WD_LICENSE_LENGTH 128
typedef struct
{
    CHAR cLicense[WD_LICENSE_LENGTH]; // Buffer with license string to put.
                      // If empty string then get current license setting
                      // into dwLicense.
    DWORD dwLicense;  // Returns license settings: LICENSE_DEMO, LICENSE_WD 
                      // etc..., or 0 for invalid license.
    DWORD dwLicense2; // Returns additional license settings, if dwLicense 
                      // could not hold all the information.
                      // Then dwLicense will return 0.
} WD_LICENSE, WD_LICENSE_V44;

typedef struct
{
    DWORD dwBusType;        // Bus Type: ISA, EISA, PCI, PCMCIA.
    DWORD dwBusNum;         // Bus number.
    DWORD dwSlotFunc;       // Slot number on Bus.
} WD_BUS, WD_BUS_V30;

enum
{
    WD_BUS_USB = 0xfffffffe,
    WD_BUS_ISA = 1,
    WD_BUS_EISA = 2,
    WD_BUS_PCI = 5,
    WD_BUS_PCMCIA = 8,
};

typedef enum
{
    ITEM_NONE=0,
    ITEM_INTERRUPT=1,
    ITEM_MEMORY=2,
    ITEM_IO=3,
    ITEM_BUS=5,
} ITEM_TYPE;

typedef struct
{
    DWORD item; // ITEM_TYPE
    DWORD fNotSharable;
    DWORD dwContext; // Reserved for internal use
    DWORD dwPad1;    // Reserved for internal use
    union
    {
        struct
        { // ITEM_MEMORY
            DWORD dwPhysicalAddr;     // Physical address on card.
            DWORD dwBytes;            // Address range.
            KPTR dwTransAddr;         // Returns the address to pass on to transfer commands.
            DWORD dwUserDirectAddr;   // Returns the address for direct user read/write.
            DWORD dwCpuPhysicalAddr;  // Returns the CPU physical address of card.
            DWORD dwBar;              // Base Address Register number of PCI card.
        } Mem;
        struct
        { // ITEM_IO
            KPTR dwAddr;          // Beginning of io address.
            DWORD dwBytes;        // IO range.
            DWORD dwBar;          // Base Address Register number of PCI card.
        } IO;
        struct
        { // ITEM_INTERRUPT
            DWORD dwInterrupt; // Number of interrupt to install.
            DWORD dwOptions;   // Interrupt options. For level sensitive
                               // interrupts - set to: INTERRUPT_LEVEL_SENSITIVE.
            DWORD hInterrupt;  // Returns the handle of the interrupt installed.
        } Int;
        WD_BUS Bus; // ITEM_BUS
        struct
        {
            DWORD dw1, dw2, dw3, dw4, dw5, dw6;
        } Val;
    } I;
} WD_ITEMS, WD_ITEMS_V30;

enum { WD_CARD_ITEMS = 20 };

typedef struct
{
    DWORD dwItems;
    WD_ITEMS Item[WD_CARD_ITEMS];
} WD_CARD, WD_CARD_V30;

enum { CARD_VX_NO_MMU_INIT = 0x4000000 };

typedef struct
{
    WD_CARD Card;           // Card to register.
    DWORD fCheckLockOnly;   // Only check if card is lockable, return hCard=1 if OK.
    DWORD hCard;            // Handle of card.
    DWORD dwOptions;        // Should be zero.
    CHAR cName[32];         // Name of card.
    CHAR cDescription[100]; // Description.
} WD_CARD_REGISTER, WD_CARD_REGISTER_V40;

enum { WD_PCI_CARDS = 100 };

typedef struct
{
    DWORD dwBus;
    DWORD dwSlot;
    DWORD dwFunction;
} WD_PCI_SLOT;

typedef struct
{
    DWORD dwVendorId;
    DWORD dwDeviceId;
} WD_PCI_ID;

typedef struct
{
    WD_PCI_ID searchId;     // If dwVendorId==0 - scan all vendor IDs.
                            // If dwDeviceId==0 - scan all device IDs.
    DWORD dwCards;          // Number of cards found.
    WD_PCI_ID cardId[WD_PCI_CARDS]; // VendorID & DeviceID of cards found.
    WD_PCI_SLOT cardSlot[WD_PCI_CARDS]; // Pci slot info of cards found.
} WD_PCI_SCAN_CARDS, WD_PCI_SCAN_CARDS_V30;

typedef struct
{
    WD_PCI_SLOT pciSlot;    // Pci slot.
    WD_CARD Card;           // Get card parameters for pci slot.
} WD_PCI_CARD_INFO, WD_PCI_CARD_INFO_V30;

typedef enum
{
    PCI_ACCESS_OK = 0,
    PCI_ACCESS_ERROR = 1,
    PCI_BAD_BUS = 2,
    PCI_BAD_SLOT = 3,
} PCI_ACCESS_RESULT;

typedef enum
{
    PCMCIA_ACCESS_OK = 0,
    PCMCIA_BAD_SOCKET = 1,
    PCMCIA_BAD_OFFSET = 2,
    PCMCIA_ACCESS_ERROR =3,

} PCMCIA_ACCESS_RESULT;

typedef struct
{
    WD_PCI_SLOT pciSlot;    // Pci bus, slot and function number.
    PVOID       pBuffer;    // Buffer for read/write.
    DWORD       dwOffset;   // Offset in pci configuration space to read/write from.
    DWORD       dwBytes;    // Bytes to read/write from/to buffer.
                            // Returns the number of bytes read/wrote.
    DWORD       fIsRead;    // If 1 then read pci config, 0 write pci config.
    DWORD       dwResult;   // PCI_ACCESS_RESULT
} WD_PCI_CONFIG_DUMP, WD_PCI_CONFIG_DUMP_V30;

enum { WD_ISAPNP_CARDS = 16 };
enum { WD_ISAPNP_COMPATIBLE_IDS = 10 };
enum { WD_ISAPNP_COMP_ID_LENGTH = 7 }; // ISA compressed ID is 7 chars long.
enum { WD_ISAPNP_ANSI_LENGTH = 32 }; // ISA ANSI ID is limited to 32 chars long.
typedef CHAR WD_ISAPNP_COMP_ID[WD_ISAPNP_COMP_ID_LENGTH+1];
typedef CHAR WD_ISAPNP_ANSI[WD_ISAPNP_ANSI_LENGTH+1+3]; // Add 3 bytes for DWORD alignment.
typedef struct
{
    WD_ISAPNP_COMP_ID cVendor; // Vendor ID.
    DWORD dwSerial; // Serial number of card.
} WD_ISAPNP_CARD_ID;

typedef struct
{
    WD_ISAPNP_CARD_ID cardId;  // VendorID & serial number of cards found.
    DWORD dwLogicalDevices;    // Logical devices on the card.
    BYTE bPnPVersionMajor;     // ISA PnP version Major.
    BYTE bPnPVersionMinor;     // ISA PnP version Minor.
    BYTE bVendorVersionMajor;  // Vendor version Major.
    BYTE bVendorVersionMinor;  // Vendor version Minor.
    WD_ISAPNP_ANSI cIdent;     // Device identifier.
} WD_ISAPNP_CARD, WD_ISAPNP_CARD_V40;

typedef struct
{
    WD_ISAPNP_CARD_ID searchId; // If searchId.cVendor[0]==0 - scan all vendor IDs.
                                // If searchId.dwSerial==0 - scan all serial numbers.
    DWORD dwCards;              // Number of cards found.
    WD_ISAPNP_CARD Card[WD_ISAPNP_CARDS]; // Cards found.
} WD_ISAPNP_SCAN_CARDS, WD_ISAPNP_SCAN_CARDS_V40;

typedef struct
{
    WD_ISAPNP_CARD_ID cardId;   // VendorID and serial number of card.
    DWORD dwLogicalDevice;      // Logical device in card.
    WD_ISAPNP_COMP_ID cLogicalDeviceId; // Logical device ID.
    DWORD dwCompatibleDevices;  // Number of compatible device IDs.
    WD_ISAPNP_COMP_ID CompatibleDevice[WD_ISAPNP_COMPATIBLE_IDS]; // Compatible device IDs.
    WD_ISAPNP_ANSI cIdent;      // Device identifier.
    WD_CARD Card;               // Get card parameters for the ISA PnP card.
} WD_ISAPNP_CARD_INFO, WD_ISAPNP_CARD_INFO_V40;

typedef enum
{
    ISAPNP_ACCESS_OK = 0,
    ISAPNP_ACCESS_ERROR = 1,
    ISAPNP_BAD_ID = 2,
} ISAPNP_ACCESS_RESULT;

typedef struct
{
    WD_ISAPNP_CARD_ID cardId; // VendorID and serial number of card.
    DWORD dwOffset;   // Offset in ISA PnP configuration space to read/write from.
    DWORD fIsRead;    // If 1 then read ISA PnP config, 0 write ISA PnP config.
    BYTE  bData;      // Result data of byte read/write.
    DWORD dwResult;   // ISAPNP_ACCESS_RESULT.
} WD_ISAPNP_CONFIG_DUMP, WD_ISAPNP_CONFIG_DUMP_V40;

// PCMCIA Card Services

// Extreme case - two PCMCIA slots and two multi-function (4 functions) cards
enum
{
    WD_PCMCIA_CARDS = 8,
    WD_PCMCIA_VERSION_LEN = 4,
    WD_PCMCIA_MANUFACTURER_LEN = 48,
    WD_PCMCIA_PRODUCTNAME_LEN = 48,
    WD_PCMCIA_MAX_SOCKET = 2,
    WD_PCMCIA_MAX_FUNCTION = 2,
};

typedef struct
{
    BYTE uSocket;      // Specifies the socket number (first socket is 0)
    BYTE uFunction;    // Specifies the function number (first function is 0)
    BYTE uPadding0;    // 2 bytes padding so structure will be 4 bytes aligned
    BYTE uPadding1;
} WD_PCMCIA_SLOT, WD_PCMCIA_SLOT_V41;

typedef struct
{
    DWORD dwManufacturerId; // card manufacturer
    DWORD dwCardId;         // card type and model
} WD_PCMCIA_ID;

typedef struct
{
    WD_PCMCIA_ID searchId;           // device ID to search for
    DWORD dwCards;                   // number of cards found
    WD_PCMCIA_ID cardId[WD_PCMCIA_CARDS]; // device IDs of cards found
    WD_PCMCIA_SLOT cardSlot[WD_PCMCIA_CARDS]; // pcmcia slot info of cards found
} WD_PCMCIA_SCAN_CARDS, WD_PCMCIA_SCAN_CARDS_V41;

typedef struct
{
    WD_PCMCIA_SLOT pcmciaSlot; // pcmcia slot
    WD_CARD Card;              // get card parameters for pcmcia slot
    CHAR cVersion[WD_PCMCIA_VERSION_LEN];
    CHAR cManufacturer[WD_PCMCIA_MANUFACTURER_LEN];
    CHAR cProductName[WD_PCMCIA_PRODUCTNAME_LEN];
    DWORD dwManufacturerId;    // card manufacturer
    DWORD dwCardId;            // card type and model
    DWORD dwFuncId;            // card function code
} WD_PCMCIA_CARD_INFO, WD_PCMCIA_CARD_INFO_V41;

typedef struct
{
    WD_PCMCIA_SLOT pcmciaSlot;
    PVOID pBuffer;    // buffer for read/write
    DWORD dwOffset;   // offset in pcmcia configuration space to
                      //    read/write from
    DWORD dwBytes;    // bytes to read/write from/to buffer
                      //    returns the number of bytes read/wrote
    DWORD fIsRead;    // if 1 then read pci config, 0 write pci config
    DWORD dwResult;   // PCMCIA_ACCESS_RESULT
} WD_PCMCIA_CONFIG_DUMP, WD_PCMCIA_CONFIG_DUMP_V41;

enum { SLEEP_NON_BUSY = 1 };
typedef struct
{
    DWORD dwMicroSeconds; // Sleep time in Micro Seconds (1/1,000,000 Second)
    DWORD dwOptions;      // can be: SLEEP_NON_BUSY (10000 uSec +)
} WD_SLEEP, WD_SLEEP_V40;

typedef enum
{
    D_OFF       = 0,
    D_ERROR     = 1,
    D_WARN      = 2,
    D_INFO      = 3,
    D_TRACE     = 4
} DEBUG_LEVEL;

typedef enum
{
    S_ALL       = 0xffffffff,
    S_IO        = 0x8,
    S_MEM       = 0x10,
    S_INT       = 0x20,
    S_PCI       = 0x40,
    S_DMA       = 0x80,
    S_MISC      = 0x100,
    S_LICENSE   = 0x200,
    S_ISAPNP    = 0x400,
    S_PCMCIA    = 0x800,
    S_KER_PLUG  = 0x10000,
    S_CARD_REG  = 0x2000,
    S_KER_DRV   = 0x4000,
    S_USB       = 0x8000,
    S_EVENT     = 0x20000,
} DEBUG_SECTION;

typedef enum
{
    DEBUG_STATUS = 1,
    DEBUG_SET_FILTER = 2,
    DEBUG_SET_BUFFER = 3,
    DEBUG_CLEAR_BUFFER = 4,
    DEBUG_DUMP_SEC_ON = 5,
    DEBUG_DUMP_SEC_OFF = 6,
    KERNEL_DEBUGGER_ON = 7,
    KERNEL_DEBUGGER_OFF = 8
} DEBUG_COMMAND;

typedef struct
{
    DWORD dwCmd;     // DEBUG_COMMAND: DEBUG_STATUS, DEBUG_SET_FILTER, DEBUG_SET_BUFFER, DEBUG_CLEAR_BUFFER
    // used for DEBUG_SET_FILTER
    DWORD dwLevel;   // DEBUG_LEVEL: D_ERROR, D_WARN..., or D_OFF to turn debugging off
    DWORD dwSection; // DEBUG_SECTION: for all sections in driver: S_ALL
                     // for partial sections: S_IO, S_MEM...
    DWORD dwLevelMessageBox; // DEBUG_LEVEL to print in a message box
    // used for DEBUG_SET_BUFFER
    DWORD dwBufferSize; // size of buffer in kernel
} WD_DEBUG, WD_DEBUG_V40;

typedef struct
{
    PCHAR pcBuffer;  // buffer to receive debug messages
    DWORD dwSize;    // size of buffer in bytes
} WD_DEBUG_DUMP, WD_DEBUG_DUMP_V40;

typedef struct
{
    CHAR pcBuffer[256];
    DWORD dwLevel;
    DWORD dwSection;
} WD_DEBUG_ADD, WD_DEBUG_ADD_V503;

typedef struct
{
    DWORD hKernelPlugIn;
    PCHAR pcDriverName;
    PCHAR pcDriverPath; // if NULL the driver will be searched in the windows system directory
    PVOID pOpenData;
} WD_KERNEL_PLUGIN, WD_KERNEL_PLUGIN_V40;

typedef enum
{
    EVENT_STATUS_OK = 0,
} EVENT_STATUS;

typedef enum {
    PIPE_TYPE_CONTROL     = 0,
    PIPE_TYPE_ISOCHRONOUS = 1,
    PIPE_TYPE_BULK        = 2,
    PIPE_TYPE_INTERRUPT   = 3,
} USB_PIPE_TYPE;

#define WD_USB_MAX_PIPE_NUMBER 32
#define WD_USB_MAX_ENDPOINTS WD_USB_MAX_PIPE_NUMBER
#define WD_USB_MAX_INTERFACES 30

#define WD_USB_MAX_DEVICE_NUMBER 30

typedef struct
{
    DWORD dwVendorId;
    DWORD dwProductId;
} WD_USB_ID;


#ifndef LINUX
typedef enum {
    USB_DIR_IN     = 1,
    USB_DIR_OUT    = 2,
    USB_DIR_IN_OUT = 3,
} USB_DIR;
#endif

typedef enum {
    WDU_DIR_IN     = 1,
    WDU_DIR_OUT    = 2,
    WDU_DIR_IN_OUT = 3,
} WDU_DIR;

typedef struct
{
    DWORD dwNumber;        // Pipe 0 is the default pipe
    DWORD dwMaximumPacketSize;
    DWORD type;            // USB_PIPE_TYPE
    DWORD direction;       // WDU_DIR
                           // Isochronous, Bulk, Interrupt are either USB_DIR_IN or USB_DIR_OUT
                           // Control are USB_DIR_IN_OUT
    DWORD dwInterval;      // interval in ms relevant to Interrupt pipes
} WD_USB_PIPE_INFO, WD_USB_PIPE_INFO_V43, WDU_PIPE_INFO;

typedef struct
{
    DWORD dwNumInterfaces;
    DWORD dwValue;
    DWORD dwAttributes;
    DWORD MaxPower;
} WD_USB_CONFIG_DESC;

typedef struct
{
    DWORD dwNumber;
    DWORD dwAlternateSetting;
    DWORD dwNumEndpoints;
    DWORD dwClass;
    DWORD dwSubClass;
    DWORD dwProtocol;
    DWORD dwIndex;
} WD_USB_INTERFACE_DESC, WD_USB_INTERFACE_DESC_V43;

typedef struct
{
    DWORD dwEndpointAddress;
    DWORD dwAttributes;
    DWORD dwMaxPacketSize;
    DWORD dwInterval;
} WD_USB_ENDPOINT_DESC, WD_USB_ENDPOINT_DESC_V43;

typedef struct
{
    WD_USB_INTERFACE_DESC Interface;
    WD_USB_ENDPOINT_DESC Endpoints[WD_USB_MAX_ENDPOINTS];
} WD_USB_INTERFACE;

typedef struct
{
    DWORD uniqueId;
    DWORD dwConfigurationIndex;
    WD_USB_CONFIG_DESC configuration;
    DWORD dwInterfaceAlternatives;
    WD_USB_INTERFACE Interface[WD_USB_MAX_INTERFACES];
    DWORD dwStatus;  // Configuration status code - see WD_ERROR_CODES enum definition.
                     // WD_USBD_STATUS_SUCCESS for a successful configuration.
} WD_USB_CONFIGURATION, WD_USB_CONFIGURATION_V52;

typedef struct
{
    DWORD   fBusPowered;
    DWORD   dwPorts;              // Number of ports on this hub.
    DWORD   dwCharacteristics;    // Hub Characteristics.
    DWORD   dwPowerOnToPowerGood; // Port power on till power good in 2ms.
    DWORD   dwHubControlCurrent;  // Max current in mA.
} WD_USB_HUB_GENERAL_INFO, WD_USB_HUB_GENERAL_INFO_V43;

#define WD_SINGLE_INTERFACE 0xFFFFFFFF

typedef struct
{
    WD_USB_ID deviceId;
    DWORD dwHubNum; // Unused
    DWORD dwPortNum; // Unused
    DWORD fHub; // Unused
    DWORD fFullSpeed; // Unused
    DWORD dwConfigurationsNum; 
    DWORD deviceAddress; // Unused
    WD_USB_HUB_GENERAL_INFO hubInfo; // Unused
    DWORD deviceClass;  
    DWORD deviceSubClass;      
    DWORD dwInterfaceNum; // For a single device WinDriver sets this
                          // value to WD_SINGLE_INTERFACE
} WD_USB_DEVICE_GENERAL_INFO;

typedef struct
{
    DWORD dwPipes;
    WD_USB_PIPE_INFO Pipe[WD_USB_MAX_PIPE_NUMBER];
} WD_USB_DEVICE_INFO, WD_USB_DEVICE_INFO_V43;

// IOCTL Structures
typedef struct
{
    WD_USB_ID searchId; // If dwVendorId==0 - scan all vendor IDs.
                        // If dwProductId==0 - scan all product IDs.
    DWORD dwDevices;
    DWORD uniqueId[WD_USB_MAX_DEVICE_NUMBER]; // a unique id to identify the device
    WD_USB_DEVICE_GENERAL_INFO deviceGeneralInfo[WD_USB_MAX_DEVICE_NUMBER];
    DWORD dwStatus;     // Scan status code - see WD_ERROR_CODES enum definition.
                        // WD_USBD_STATUS_SUCCESS for a successful scan.
} WD_USB_SCAN_DEVICES, WD_USB_SCAN_DEVICES_V52;

// USB TRANSFER options
enum {
    USB_ISOCH_FULL_PACKETS_ONLY = 0x20,
    
    // The following flags are no longer used beginning with v6.0:
    USB_TRANSFER_HALT = 0x1,
    USB_SHORT_TRANSFER = 0x2,
    USB_FULL_TRANSFER = 0x4,
    USB_ISOCH_ASAP = 0x8,
    USB_ISOCH_RESET = 0x10,
};

// new USB API definitions

typedef PVOID WDU_REGISTER_DEVICES_HANDLE;

#define WDU_ENDPOINT_TYPE_MASK                    0x03

#define WDU_ENDPOINT_DIRECTION_MASK               0x80
// test direction bit in the bEndpointAddress field of
// an endpoint descriptor.
#define WDU_ENDPOINT_DIRECTION_OUT(addr)          (!((addr) & WDU_ENDPOINT_DIRECTION_MASK))
#define WDU_ENDPOINT_DIRECTION_IN(addr)           ((addr) & WDU_ENDPOINT_DIRECTION_MASK)

typedef struct 
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bInterfaceNumber;
    UCHAR bAlternateSetting;
    UCHAR bNumEndpoints;
    UCHAR bInterfaceClass;
    UCHAR bInterfaceSubClass;
    UCHAR bInterfaceProtocol;
    UCHAR iInterface;
} WDU_INTERFACE_DESCRIPTOR;

typedef struct 
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bEndpointAddress;
    UCHAR bmAttributes;
    USHORT wMaxPacketSize;
    UCHAR bInterval;
} WDU_ENDPOINT_DESCRIPTOR;

typedef struct 
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT wTotalLength;
    UCHAR bNumInterfaces;
    UCHAR bConfigurationValue;
    UCHAR iConfiguration;
    UCHAR bmAttributes;
    UCHAR MaxPower;
} WDU_CONFIGURATION_DESCRIPTOR;

typedef struct 
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;

    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} WDU_DEVICE_DESCRIPTOR;

typedef struct
{
    WDU_INTERFACE_DESCRIPTOR Descriptor;
    WDU_ENDPOINT_DESCRIPTOR *pEndpointDescriptors;
    WDU_PIPE_INFO *pPipes;
} WDU_ALTERNATE_SETTING;

typedef struct
{
    WDU_ALTERNATE_SETTING *pAlternateSettings;
    DWORD dwNumAltSettings;
    WDU_ALTERNATE_SETTING *pActiveAltSetting;
} WDU_INTERFACE;

typedef struct
{
    WDU_CONFIGURATION_DESCRIPTOR Descriptor; 
    DWORD dwNumInterfaces; 
    WDU_INTERFACE *pInterfaces;
} WDU_CONFIGURATION;

typedef struct {
    WDU_DEVICE_DESCRIPTOR Descriptor;
    WDU_PIPE_INFO Pipe0;
    WDU_CONFIGURATION *pConfigs;
    WDU_CONFIGURATION *pActiveConfig;
    WDU_INTERFACE *pActiveInterface;
} WDU_DEVICE;

// note: any devices found matching this table will be controlled by WD
typedef struct
{
    WORD wVendorId;
    WORD wProductId;
    BYTE bDeviceClass;
    BYTE bDeviceSubClass;
    BYTE bInterfaceClass;
    BYTE bInterfaceSubClass;
    BYTE bInterfaceProtocol;
} WDU_MATCH_TABLE;

// IOCTL's parameters

typedef struct 
{
    DWORD dwUniqueID;
    PVOID pBuf;
    DWORD dwBytes;
    DWORD dwOptions;
} WDU_GET_DEVICE_DATA;

typedef struct 
{
    DWORD dwUniqueID;
    DWORD dwInterfaceNum;
    DWORD dwAlternateSetting;
    DWORD dwOptions;
} WDU_SET_INTERFACE;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum;
    DWORD dwOptions;
} WDU_RESET_PIPE;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum;
    DWORD dwOptions;
} WDU_HALT_TRANSFER;

typedef struct
{
    DWORD dwUniqueID;
    DWORD dwPipeNum;    // Pipe number on device.
    DWORD fRead;        // TRUE for read (IN) transfers; FALSE for write (OUT) transfers.
    DWORD dwOptions;    // USB_TRANSFER options:
                        //    USB_TRANSFER_HALT halts the pervious transfer.
                        //    USB_SHORT_TRANSFER - if set, WDU_Transfer() will return
                        //    once a data transfer occurs (within the dwTimeout period),
                        //    even if the device sent less data than requested in dwBytes.
    PVOID pBuffer;    // Pointer to buffer to read/write.
    DWORD dwBufferSize; // Amount of bytes to transfer.
    DWORD dwBytesTransferred; // Returns the number of bytes actually read/written
    BYTE SetupPacket[8];          // Setup packet for control pipe transfer.
    DWORD dwTimeout;    // Timeout for the transfer in milliseconds. Set to 0 for infinite wait.
} WDU_TRANSFER;

typedef struct
{
    DWORD dwUniqueID;
    UCHAR bType; 
    UCHAR bIndex;
    WORD wLength;
    PVOID pBuffer;
    WORD wLanguage;
} WDU_GET_DESCRIPTOR;

// dwStatus returned values:
typedef enum {
    WD_STATUS_SUCCESS = 0x0L,
    WD_STATUS_INVALID_WD_HANDLE = 0xffffffffL,
    
// The following statuses are returned by WinDriver:
    WD_WINDRIVER_STATUS_ERROR = 0x20000000L,

    WD_INVALID_HANDLE = 0x20000001L,
    WD_INVALID_PIPE_NUMBER = 0x20000002L,
    WD_READ_WRITE_CONFLICT = 0x20000003L,    // request to read from an OUT (write) pipe or
                                             // request to write to an IN (read) pipe
    WD_ZERO_PACKET_SIZE = 0x20000004L,       // maximum packet size is zero
    WD_INSUFFICIENT_RESOURCES = 0x20000005L,
    WD_UNKNOWN_PIPE_TYPE = 0x20000006L,
    WD_SYSTEM_INTERNAL_ERROR = 0x20000007L,
    WD_DATA_MISMATCH = 0x20000008L,
    WD_NO_LICENSE = 0x20000009L,
    WD_NOT_IMPLEMENTED = 0x2000000aL,
    WD_KERPLUG_FAILURE = 0x2000000bL,
    WD_FAILED_ENABLING_INTERRUPT = 0x2000000cL,
    WD_INTERRUPT_NOT_ENABLED = 0x2000000dL,
    WD_RESOURCE_OVERLAP = 0x2000000eL,
    WD_DEVICE_NOT_FOUND = 0x2000000fL,
    WD_WRONG_UNIQUE_ID = 0x20000010L,
    WD_OPERATION_ALREADY_DONE = 0x20000011L,
    WD_USB_DESCRIPTOR_ERROR = 0x20000012L,
    WD_SET_CONFIGURATION_FAILED = 0x20000013L,
    WD_CANT_OBTAIN_PDO = 0x20000014L,
    WD_TIME_OUT_EXPIRED = 0x20000015L,
    WD_IRP_CANCELED = 0x20000016L,
    WD_FAILED_USER_MAPPING = 0x20000017L,
    WD_FAILED_KERNEL_MAPPING = 0x20000018L,
    WD_NO_RESOURCES_ON_DEVICE = 0x20000019L,
    WD_NO_EVENTS = 0x2000001aL,
    WD_INVALID_PARAMETER = 0x2000001bL,
    WD_INCORRECT_VERSION = 0x2000001cL,
    WD_TRY_AGAIN = 0x2000001dL,
    WD_WINDRIVER_NOT_FOUND = 0x2000001eL,
// The following statuses are returned by USBD:
    // USBD status types:
    WD_USBD_STATUS_SUCCESS = 0x00000000L,
    WD_USBD_STATUS_PENDING = 0x40000000L,
    WD_USBD_STATUS_ERROR = 0x80000000L,
    WD_USBD_STATUS_HALTED = 0xC0000000L,

    // USBD status codes:
    // NOTE: The following status codes are comprised of one of the status types above and an
    // error code [i.e. 0xXYYYYYYYL - where: X = status type; YYYYYYY = error code].
    // The same error codes may also appear with one of the other status types as well.

    // HC (Host Controller) status codes.
    // [NOTE: These status codes use the WD_USBD_STATUS_HALTED status type]:
    WD_USBD_STATUS_CRC = 0xC0000001L,
    WD_USBD_STATUS_BTSTUFF = 0xC0000002L,
    WD_USBD_STATUS_DATA_TOGGLE_MISMATCH = 0xC0000003L,
    WD_USBD_STATUS_STALL_PID = 0xC0000004L,
    WD_USBD_STATUS_DEV_NOT_RESPONDING = 0xC0000005L,
    WD_USBD_STATUS_PID_CHECK_FAILURE = 0xC0000006L,
    WD_USBD_STATUS_UNEXPECTED_PID = 0xC0000007L,
    WD_USBD_STATUS_DATA_OVERRUN = 0xC0000008L,
    WD_USBD_STATUS_DATA_UNDERRUN = 0xC0000009L,
    WD_USBD_STATUS_RESERVED1 = 0xC000000AL,
    WD_USBD_STATUS_RESERVED2 = 0xC000000BL,
    WD_USBD_STATUS_BUFFER_OVERRUN = 0xC000000CL,
    WD_USBD_STATUS_BUFFER_UNDERRUN = 0xC000000DL,
#if defined(WINCE)
    WD_USBD_STATUS_NOT_ACCESSED_ALT = 0xC000000FL,  // HCD maps this to E when encountered 
    WD_USBD_STATUS_NOT_ACCESSED = 0xC000000EL,  
#else
    WD_USBD_STATUS_NOT_ACCESSED = 0xC000000FL,    
#endif
    WD_USBD_STATUS_FIFO = 0xC0000010L,

    WD_USBD_STATUS_ISOCH = 0xC0000100L,
    WD_USBD_STATUS_CANCELED = 0xC0000101L,
    WD_USBD_STATUS_NOT_COMPLETE = 0xC0000103L,
    WD_USBD_STATUS_CLIENT_BUFFER = 0xC0000104L,

    // Returned by HCD (Host Controller Driver) if a transfer is submitted to an endpoint that is
    // stalled:
    WD_USBD_STATUS_ENDPOINT_HALTED = 0xC0000030L,

    // Software status codes
    // [NOTE: The following status codes have only the error bit set]:
    WD_USBD_STATUS_NO_MEMORY = 0x80000100L,
    WD_USBD_STATUS_INVALID_URB_FUNCTION = 0x80000200L,
    WD_USBD_STATUS_INVALID_PARAMETER = 0x80000300L,

    // Returned if client driver attempts to close an endpoint/interface
    // or configuration with outstanding transfers:
    WD_USBD_STATUS_ERROR_BUSY = 0x80000400L,

    // Returned by USBD if it cannot complete a URB request. Typically this
    // will be returned in the URB status field when the Irp is completed
    // with a more specific NT error code. [The Irp statuses are indicated in
    // WinDriver's Monitor Debug Messages (wddebug_gui) tool]:
    WD_USBD_STATUS_REQUEST_FAILED = 0x80000500L,

    WD_USBD_STATUS_INVALID_PIPE_HANDLE = 0x80000600L,

    // Returned when there is not enough bandwidth available
    // to open a requested endpoint:
    WD_USBD_STATUS_NO_BANDWIDTH = 0x80000700L,

    // Generic HC (Host Controller) error:
    WD_USBD_STATUS_INTERNAL_HC_ERROR = 0x80000800L,

    // Returned when a short packet terminates the transfer
    // i.e. USBD_SHORT_TRANSFER_OK bit not set:
    WD_USBD_STATUS_ERROR_SHORT_TRANSFER = 0x80000900L,

    // Returned if the requested start frame is not within
    // USBD_ISO_START_FRAME_RANGE of the current USB frame,
    // NOTE: that the stall bit is set:
    WD_USBD_STATUS_BAD_START_FRAME = 0xC0000A00L,

    // Returned by HCD (Host Controller Driver) if all packets in an iso transfer complete with
    // an error:
    WD_USBD_STATUS_ISOCH_REQUEST_FAILED = 0xC0000B00L,

    // Returned by USBD if the frame length control for a given
    // HC (Host Controller) is already taken by another driver:
    WD_USBD_STATUS_FRAME_CONTROL_OWNED = 0xC0000C00L,

    // Returned by USBD if the caller does not own frame length control and
    // attempts to release or modify the HC frame length:
    WD_USBD_STATUS_FRAME_CONTROL_NOT_OWNED = 0xC0000D00L,
} WD_ERROR_CODES;

typedef struct
{
    DWORD hDevice;      // Handle of USB device to read from or write to.
    DWORD dwPipe;       // Pipe number on device.
    DWORD fRead;        // TRUE for read (IN) transfers; FALSE for write (OUT) transfers.
    DWORD dwOptions;    // USB_TRANSFER options:
                        //    USB_TRANSFER_HALT halts the pervious transfer.
                        //    USB_SHORT_TRANSFER - if set, WD_UsbTransfer() will return
                        //    once a data transfer occurs (within the dwTimeout period),
                        //    even if the device sent less data than requested in dwBytes.
    PVOID pBuffer;      // Pointer to buffer to read/write.
    DWORD dwBytes;      // Amount of bytes to transfer.
    DWORD dwTimeout;    // Timeout for the transfer in milliseconds. Set to 0 for infinite wait.
    DWORD dwBytesTransfered;    // Returns the number of bytes actually read/written
    BYTE  SetupPacket[8];       // Setup packet for control pipe transfer.
    DWORD fOK;          // TRUE if transfer succeeded.
    DWORD dwStatus;     // Transfer status code - see WD_ERROR_CODES enum definition.
                        // WD_USBD_STATUS_SUCCESS for a successful transfer.
} WD_USB_TRANSFER, WD_USB_TRANSFER_V52;

typedef struct {
    DWORD uniqueId;              // The device unique ID.
    DWORD dwConfigurationIndex;  // The index of the configuration to register.
    DWORD dwInterfaceNum;        // Interface to register.
    DWORD dwInterfaceAlternate;
    DWORD hDevice;               // Handle of device.
    WD_USB_DEVICE_INFO Device;   // Description of the device.
    DWORD dwOptions;             // Should be zero.
    CHAR  cName[32];             // Name of card.
    CHAR  cDescription[100];     // Description.
    DWORD dwStatus;              // Register status code - see WD_ERROR_CODES enum definition.
                                 // WD_USBD_STATUS_SUCCESS for a successful registration.
} WD_USB_DEVICE_REGISTER, WD_USB_DEVICE_REGISTER_V52;

typedef struct
{
    DWORD hDevice;
    DWORD dwPipe;
    DWORD dwStatus;   // Reset status code - see WD_ERROR_CODES enum definition.
                      // WD_USBD_STATUS_SUCCESS for a successful reset.
} WD_USB_RESET_PIPE, WD_USB_RESET_PIPE_V52;

typedef enum
{
    WD_INSERT                   = 0x1,
    WD_REMOVE                   = 0x2,
    WD_POWER_CHANGED_D0         = 0x10,  //power states for the power management.
    WD_POWER_CHANGED_D1         = 0x20,
    WD_POWER_CHANGED_D2         = 0x40,
    WD_POWER_CHANGED_D3         = 0x80,
    WD_POWER_SYSTEM_WORKING     = 0x100,
    WD_POWER_SYSTEM_SLEEPING1   = 0x200,
    WD_POWER_SYSTEM_SLEEPING2   = 0x400,
    WD_POWER_SYSTEM_SLEEPING3   = 0x800,
    WD_POWER_SYSTEM_HIBERNATE   = 0x1000,
    WD_POWER_SYSTEM_SHUTDOWN    = 0x2000,
} WD_EVENT_ACTION;

typedef enum
{
    WD_ACKNOWLEDGE              = 0x1,
    WD_ACCEPT_CONTROL           = 0x2,  // used in WD_EVENT_SEND (acknowledge)
} WD_EVENT_OPTION;

#define WD_ACTIONS_POWER (WD_POWER_CHANGED_D0 | WD_POWER_CHANGED_D1 | WD_POWER_CHANGED_D2 | \
    WD_POWER_CHANGED_D3 | WD_POWER_SYSTEM_WORKING | WD_POWER_SYSTEM_SLEEPING1 | \
    WD_POWER_SYSTEM_SLEEPING3 | WD_POWER_SYSTEM_HIBERNATE | WD_POWER_SYSTEM_SHUTDOWN)
#define WD_ACTIONS_ALL (WD_ACTIONS_POWER | WD_INSERT | WD_REMOVE)

typedef struct
{
    DWORD handle;
    DWORD dwAction; // WD_EVENT_ACTION
    DWORD dwStatus; // EVENT_STATUS
    DWORD dwEventId;
    DWORD dwCardType; //WD_BUS_PCI or WD_BUS_USB
    DWORD hKernelPlugIn;
    DWORD dwOptions; // WD_EVENT_OPTION
    union
    {
        struct
        {
            WD_PCI_ID cardId;
            WD_PCI_SLOT pciSlot;
        } Pci;
        struct
        {
            WD_USB_ID deviceId;
            DWORD dwUniqueID;
        } Usb;
    } u;
    DWORD dwEventVer;
    DWORD dwNumMatchTables;
    WDU_MATCH_TABLE matchTables[1];
} WD_EVENT, WD_EVENT_V60; 

typedef struct
{
    DWORD applications_num;
    DWORD devices_num;
} WD_USAGE;

enum { WD_USB_HARD_RESET = 1 };

typedef struct
{
    DWORD hDevice;
    DWORD dwOptions; // USB_RESET options:
                     //    WD_USB_HARD_RESET - will reset the device
                     //    even if it is not disabled.
                     //    After using this option it is advised to
                     //    un-register the device (WD_UsbDeviceUnregister())
                     //    and register it again - to make sure that the
                     //    device has all its resources.
    DWORD dwStatus;  // Reset status code - see WD_ERROR_CODES enum definition.
                     // WD_USBD_STATUS_SUCCESS for a successful reset.
} WD_USB_RESET_DEVICE, WD_USB_RESET_DEVICE_V52;

#define WD_KERNEL_DRIVER_PLUGIN_HANDLE 0xffff0000

#ifndef BZERO
    #define BZERO(buf) memset(&(buf), 0, sizeof(buf))
#endif

#ifndef INVALID_HANDLE_VALUE
    #define INVALID_HANDLE_VALUE ((HANDLE)(-1))
#endif

#ifndef CTL_CODE
    #define CTL_CODE(DeviceType, Function, Method, Access) ( \
        ((DeviceType)<<16) | ((Access)<<14) | ((Function)<<2) | (Method) \
    )

    #define METHOD_BUFFERED   0
    #define METHOD_IN_DIRECT  1
    #define METHOD_OUT_DIRECT 2
    #define METHOD_NEITHER    3
    #define FILE_ANY_ACCESS   0
    #define FILE_READ_ACCESS  1    // file & pipe
    #define FILE_WRITE_ACCESS 2    // file & pipe
#endif

#if defined(UNIX)
    #define WD_CTL_CODE(wFuncNum) wFuncNum
#else
    // Device type
    #define WD_TYPE 38200
    #define WD_CTL_CODE(wFuncNum) ((DWORD) CTL_CODE( WD_TYPE, wFuncNum, METHOD_NEITHER, FILE_ANY_ACCESS))
#endif
// WinDriver function IOCTL calls.  For details on the WinDriver functions,
// see the WinDriver manual or included help files.

#define IOCTL_WD_DMA_LOCK                 WD_CTL_CODE(0x901)
#define IOCTL_WD_DMA_UNLOCK               WD_CTL_CODE(0x902)
#define IOCTL_WD_TRANSFER                 WD_CTL_CODE(0x903)
#define IOCTL_WD_MULTI_TRANSFER           WD_CTL_CODE(0x904)
#define IOCTL_WD_PCI_SCAN_CARDS           WD_CTL_CODE(0x90e)
#define IOCTL_WD_PCI_GET_CARD_INFO        WD_CTL_CODE(0x90f)
#define IOCTL_WD_VERSION                  WD_CTL_CODE(0x910)
#define IOCTL_WD_PCI_CONFIG_DUMP          WD_CTL_CODE(0x91a)
#define IOCTL_WD_KERNEL_PLUGIN_OPEN       WD_CTL_CODE(0x91b)
#define IOCTL_WD_KERNEL_PLUGIN_CLOSE      WD_CTL_CODE(0x91c)
#define IOCTL_WD_KERNEL_PLUGIN_CALL       WD_CTL_CODE(0x91d)
#define IOCTL_WD_INT_ENABLE               WD_CTL_CODE(0x91e)
#define IOCTL_WD_INT_DISABLE              WD_CTL_CODE(0x91f)
#define IOCTL_WD_INT_COUNT                WD_CTL_CODE(0x920)
#define IOCTL_WD_ISAPNP_SCAN_CARDS        WD_CTL_CODE(0x924)
#define IOCTL_WD_ISAPNP_CONFIG_DUMP       WD_CTL_CODE(0x926)
#define IOCTL_WD_SLEEP                    WD_CTL_CODE(0x927)
#define IOCTL_WD_DEBUG                    WD_CTL_CODE(0x928)
#define IOCTL_WD_DEBUG_DUMP               WD_CTL_CODE(0x929)
#define IOCTL_WD_CARD_UNREGISTER          WD_CTL_CODE(0x92b)
#define IOCTL_WD_ISAPNP_GET_CARD_INFO     WD_CTL_CODE(0x92d)
#define IOCTL_WD_PCMCIA_SCAN_CARDS        WD_CTL_CODE(0x92f)
#define IOCTL_WD_PCMCIA_GET_CARD_INFO     WD_CTL_CODE(0x930)
#define IOCTL_WD_PCMCIA_CONFIG_DUMP       WD_CTL_CODE(0x931)
#define IOCTL_WD_CARD_REGISTER            WD_CTL_CODE(0x97d)
#define IOCTL_WD_INT_WAIT                 WD_CTL_CODE(0x94b)
#define IOCTL_WD_LICENSE                  WD_CTL_CODE(0x952)
#define IOCTL_WD_USB_RESET_PIPE           WD_CTL_CODE(0x971)
#define IOCTL_WD_USB_RESET_DEVICE         WD_CTL_CODE(0x93f)
#define IOCTL_WD_USB_SCAN_DEVICES         WD_CTL_CODE(0x969)
#define IOCTL_WD_USB_TRANSFER             WD_CTL_CODE(0x967)
#define IOCTL_WD_USB_DEVICE_REGISTER      WD_CTL_CODE(0x968)
#define IOCTL_WD_USB_DEVICE_UNREGISTER    WD_CTL_CODE(0x970)
#define IOCTL_WD_USB_GET_CONFIGURATION    WD_CTL_CODE(0x974)
#define IOCTL_WD_EVENT_REGISTER           WD_CTL_CODE(0x986)
#define IOCTL_WD_EVENT_UNREGISTER         WD_CTL_CODE(0x987)
#define IOCTL_WD_EVENT_PULL               WD_CTL_CODE(0x988)
#define IOCTL_WD_EVENT_SEND               WD_CTL_CODE(0x989)
#define IOCTL_WD_DEBUG_ADD                WD_CTL_CODE(0x964)
#define IOCTL_WD_USB_RESET_DEVICEEX       WD_CTL_CODE(0x973)
#define IOCTL_WD_USAGE                    WD_CTL_CODE(0x976)
#define IOCTL_WDU_GET_DEVICE_DATA         WD_CTL_CODE(0x980)
#define IOCTL_WDU_SET_INTERFACE           WD_CTL_CODE(0x981)
#define IOCTL_WDU_RESET_PIPE              WD_CTL_CODE(0x982)
#define IOCTL_WDU_TRANSFER                WD_CTL_CODE(0x983)
#define IOCTL_WDU_HALT_TRANSFER           WD_CTL_CODE(0x985)

#if defined(UNIX)
    typedef struct
    {
        DWORD dwHeader;
        PVOID pData;
        DWORD dwSize;
    } WD_IOCTL_HEADER;

    enum { WD_IOCTL_HEADER_CODE = 0xa410b413 };
#endif

#if defined(__KERNEL__)
    HANDLE __cdecl WD_Open();
    void __cdecl WD_Close(HANDLE hWD);
    DWORD __cdecl KP_DeviceIoControl(DWORD dwFuncNum, HANDLE h, PVOID pParam,
        DWORD dwSize);
    #define WD_FUNCTION(wFuncNum, h, pParam, dwSize, fWait) \
        KP_DeviceIoControl(\
            (DWORD) wFuncNum, h,\
            (PVOID) pParam, (DWORD) dwSize\
            )
#else
    #if defined(UNIX)
        static inline ULONG WD_FUNCTION_LOCAL( DWORD wFuncNum, HANDLE h,
                PVOID pParam, DWORD dwSize, BOOL fWait)
        {
            WD_IOCTL_HEADER ioctl_hdr;
            ioctl_hdr.dwHeader = WD_IOCTL_HEADER_CODE;
            ioctl_hdr.pData = pParam;
            ioctl_hdr.dwSize = dwSize;
            #if defined(VXWORKS)
                return (ULONG) ioctl((int)(h), wFuncNum, (int)&ioctl_hdr);
            #elif defined(LINUX) || defined(SOLARIS)
                return (ULONG) ioctl((int)(DWORD)(h), wFuncNum, &ioctl_hdr);
            #endif
        }

        #if defined(VXWORKS)
            #define WD_OpenLocal()\
                ((HANDLE)open("/windrvr6", O_RDWR, 0644))
        #else
            #define WD_OpenLocal()\
                ((HANDLE)(DWORD)open("/dev/windrvr6", O_RDWR))
        #endif

        #define WD_CloseLocal(h)\
            close((int)(DWORD)(h))

    #elif defined(WINCE) && defined(_WIN32_WCE_EMULATION)
        HANDLE WINAPI WCE_EMU_WD_Open();
        void WINAPI WCE_EMU_WD_Close(HANDLE hWD);
        BOOL WINAPI WCE_EMU_WD_FUNCTION(DWORD wFuncNum, HANDLE h, PVOID
            pParam, DWORD dwSize);
        #define WD_OpenLocal() WCE_EMU_WD_Open()
        #define WD_CloseLocal(h) WCE_EMU_WD_Close(h)
        #define WD_FUNCTION_LOCAL(wFuncNum, h, pParam, dwSize, fWait) \
            WCE_EMU_WD_FUNCTION(wFuncNum, h, pParam, dwSize)
    #elif defined(WIN32) || defined(WINCE)
        #define WD_CloseLocal(h)\
            CloseHandle(h)
        #if defined(WINCE)
            void WINAPI SetProcPermissions(DWORD dwPermissions);
            #define WD_OpenLocal()\
                (SetProcPermissions(0xFFFF), CreateFile(\
                    TEXT("WDR1:"),\
                    GENERIC_READ,\
                    FILE_SHARE_READ | FILE_SHARE_WRITE,\
                    NULL, OPEN_EXISTING, 0, NULL))
            static DWORD WD_FUNCTION_LOCAL(DWORD wFuncNum, HANDLE h,
                PVOID pParam, DWORD dwSize, BOOL fWait)
            {
                DWORD dwTmp;
                DWORD rc = (DWORD)WD_STATUS_INVALID_WD_HANDLE;
                ((DWORD) DeviceIoControl(h, wFuncNum, pParam, dwSize, &rc, sizeof(DWORD),
                    &dwTmp, NULL));
                return rc;
            }
        #elif defined(WIN32)
            #define WD_OpenLocal()\
                CreateFile(\
                    TEXT("\\\\.\\WINDRVR6"),\
                    GENERIC_READ,\
                    FILE_SHARE_READ | FILE_SHARE_WRITE,\
                    NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)
            static DWORD WD_FUNCTION_LOCAL(DWORD wFuncNum, HANDLE h,
                PVOID pParam, DWORD dwSize, BOOL fWait)
            {
                DWORD dwTmp;
                HANDLE hWD = fWait ? WD_OpenLocal() : h;
                DWORD rc = (DWORD)WD_STATUS_INVALID_WD_HANDLE;
                if (hWD==INVALID_HANDLE_VALUE)
                    return (DWORD) -1;
                DeviceIoControl(hWD, (DWORD) wFuncNum, (PVOID) pParam,
                    (DWORD) dwSize, &rc, sizeof(DWORD), &dwTmp, NULL);
                if (rc != WD_STATUS_SUCCESS)
                {
                    DWORD dwError = GetLastError();

                    dwError = dwError + 1;
                }
                if (fWait)
                    WD_CloseLocal(hWD);
                return rc;
            }
        #endif
    #endif
    #define WD_FUNCTION WD_FUNCTION_LOCAL
    #define WD_Close WD_CloseLocal
    #define WD_Open WD_OpenLocal
#endif

#define WD_Debug(h,pDebug)\
    WD_FUNCTION(IOCTL_WD_DEBUG, h, pDebug, sizeof (WD_DEBUG), FALSE)
#define WD_DebugDump(h,pDebugDump)\
    WD_FUNCTION(IOCTL_WD_DEBUG_DUMP, h, pDebugDump, sizeof (WD_DEBUG_DUMP), FALSE)
#define WD_DebugAdd(h, pDebugAdd)\
    WD_FUNCTION(IOCTL_WD_DEBUG_ADD, h, pDebugAdd, sizeof(WD_DEBUG_ADD), FALSE)
#define WD_Transfer(h,pTransfer)\
    WD_FUNCTION(IOCTL_WD_TRANSFER, h, pTransfer, sizeof (WD_TRANSFER), FALSE)
#define WD_MultiTransfer(h,pTransferArray,dwNumTransfers)\
    WD_FUNCTION(IOCTL_WD_MULTI_TRANSFER, h, pTransferArray, sizeof (WD_TRANSFER) * dwNumTransfers, FALSE)
#define WD_DMALock(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_LOCK, h, pDma, sizeof (WD_DMA), FALSE)
#define WD_DMAUnlock(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_UNLOCK, h, pDma, sizeof (WD_DMA), FALSE)
#define WD_CardRegister(h,pCard)\
    WD_FUNCTION(IOCTL_WD_CARD_REGISTER, h, pCard, sizeof (WD_CARD_REGISTER), FALSE)
#define WD_CardUnregister(h,pCard)\
    WD_FUNCTION(IOCTL_WD_CARD_UNREGISTER, h, pCard, sizeof (WD_CARD_REGISTER), FALSE)
#define WD_PciScanCards(h,pPciScan)\
    WD_FUNCTION(IOCTL_WD_PCI_SCAN_CARDS, h, pPciScan, sizeof (WD_PCI_SCAN_CARDS), FALSE)
#define WD_PciGetCardInfo(h,pPciCard)\
    WD_FUNCTION(IOCTL_WD_PCI_GET_CARD_INFO, h, pPciCard, sizeof (WD_PCI_CARD_INFO), FALSE)
#define WD_PciConfigDump(h,pPciConfigDump)\
    WD_FUNCTION(IOCTL_WD_PCI_CONFIG_DUMP, h, pPciConfigDump, sizeof (WD_PCI_CONFIG_DUMP), FALSE)
#define WD_Version(h,pVerInfo)\
    WD_FUNCTION(IOCTL_WD_VERSION, h, pVerInfo, sizeof (WD_VERSION), FALSE)
#define WD_License(h,pLicense)\
    WD_FUNCTION(IOCTL_WD_LICENSE, h, pLicense, sizeof (WD_LICENSE), FALSE)
#define WD_KernelPlugInOpen(h,pKernelPlugIn)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_OPEN, h, pKernelPlugIn, sizeof (WD_KERNEL_PLUGIN), FALSE)
#define WD_KernelPlugInClose(h,pKernelPlugIn)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_CLOSE, h, pKernelPlugIn, sizeof (WD_KERNEL_PLUGIN), FALSE)
#define WD_KernelPlugInCall(h,pKernelPlugInCall)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_CALL, h, pKernelPlugInCall, sizeof (WD_KERNEL_PLUGIN_CALL), FALSE)
#define WD_IntEnable(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_ENABLE, h, pInterrupt, sizeof (WD_INTERRUPT), FALSE)
#define WD_IntDisable(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_DISABLE, h, pInterrupt, sizeof (WD_INTERRUPT), FALSE)
#define WD_IntCount(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_COUNT, h, pInterrupt, sizeof (WD_INTERRUPT), FALSE)
#define WD_IntWait(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_WAIT, h, pInterrupt, sizeof (WD_INTERRUPT), TRUE)
#define WD_IsapnpScanCards(h,pIsapnpScan)\
    WD_FUNCTION(IOCTL_WD_ISAPNP_SCAN_CARDS, h, pIsapnpScan, sizeof (WD_ISAPNP_SCAN_CARDS), FALSE)
#define WD_IsapnpGetCardInfo(h,pIsapnpCard)\
    WD_FUNCTION(IOCTL_WD_ISAPNP_GET_CARD_INFO, h, pIsapnpCard, sizeof (WD_ISAPNP_CARD_INFO), FALSE)
#define WD_IsapnpConfigDump(h,pIsapnpConfigDump)\
    WD_FUNCTION(IOCTL_WD_ISAPNP_CONFIG_DUMP, h, pIsapnpConfigDump, sizeof (WD_ISAPNP_CONFIG_DUMP), FALSE)
#define WD_PcmciaScanCards(h,pPcmciaScan)\
    WD_FUNCTION(IOCTL_WD_PCMCIA_SCAN_CARDS, h, pPcmciaScan, sizeof (WD_PCMCIA_SCAN_CARDS), FALSE)
#define WD_PcmciaGetCardInfo(h,pPcmciaCard)\
    WD_FUNCTION(IOCTL_WD_PCMCIA_GET_CARD_INFO, h, pPcmciaCard, sizeof (WD_PCMCIA_CARD_INFO), FALSE)
#define WD_PcmciaConfigDump(h,pPcmciaConfigDump)\
    WD_FUNCTION(IOCTL_WD_PCMCIA_CONFIG_DUMP, h, pPcmciaConfigDump, sizeof (WD_PCMCIA_CONFIG_DUMP), FALSE)
#define WD_Sleep(h,pSleep)\
    WD_FUNCTION(IOCTL_WD_SLEEP, h, pSleep, sizeof (WD_SLEEP), FALSE)
#define WD_UsbScanDevice(h, pScan)\
    WD_FUNCTION(IOCTL_WD_USB_SCAN_DEVICES, h, pScan,sizeof(WD_USB_SCAN_DEVICES), FALSE)
#define WD_UsbGetConfiguration(h, pConfig) \
    WD_FUNCTION(IOCTL_WD_USB_GET_CONFIGURATION, h, pConfig, sizeof(WD_USB_CONFIGURATION), FALSE)
#define WD_UsbDeviceRegister(h, pRegister)\
    WD_FUNCTION(IOCTL_WD_USB_DEVICE_REGISTER, h, pRegister, sizeof(WD_USB_DEVICE_REGISTER), FALSE)
#define WD_UsbTransfer(h, pTrans)\
    WD_FUNCTION(IOCTL_WD_USB_TRANSFER, h, pTrans, sizeof(WD_USB_TRANSFER), TRUE)
#define WD_UsbDeviceUnregister(h, pRegister)\
    WD_FUNCTION(IOCTL_WD_USB_DEVICE_UNREGISTER, h, pRegister, sizeof(WD_USB_DEVICE_REGISTER), FALSE)
#define WD_UsbResetPipe(h, pResetPipe)\
    WD_FUNCTION(IOCTL_WD_USB_RESET_PIPE, h, pResetPipe, sizeof(WD_USB_RESET_PIPE), FALSE)
#define WD_UsbResetDevice(h, hDevice)\
    WD_FUNCTION(IOCTL_WD_USB_RESET_DEVICE, h, &hDevice, sizeof(DWORD), FALSE)
#define WD_EventRegister(h, pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_REGISTER, h, pEvent, sizeof(WD_EVENT), FALSE)
#define WD_EventUnregister(h, pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_UNREGISTER, h, pEvent, sizeof(WD_EVENT), FALSE)
#define WD_EventPull(h,pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_PULL, h, pEvent, sizeof(WD_EVENT), FALSE)
#define WD_EventSend(h,pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_SEND, h, pEvent, sizeof(WD_EVENT), FALSE)
#define WD_UsbResetDeviceEx(h, pReset)\
    WD_FUNCTION(IOCTL_WD_USB_RESET_DEVICEEX, h, pReset, sizeof(WD_USB_RESET_DEVICE), FALSE)
#define WD_Usage(h, pStop) \
    WD_FUNCTION(IOCTL_WD_USAGE, h, pStop, sizeof(WD_USAGE), FALSE)

#define WD_UGetDeviceData(h, pGetDevData) \
    WD_FUNCTION(IOCTL_WDU_GET_DEVICE_DATA, h, pGetDevData, sizeof(WDU_GET_DEVICE_DATA), FALSE);
#define WD_USetInterface(h, pSetIfc) \
    WD_FUNCTION(IOCTL_WDU_SET_INTERFACE, h, pSetIfc, sizeof(WDU_SET_INTERFACE), FALSE);
#define WD_UResetPipe(h, pResetPipe) \
    WD_FUNCTION(IOCTL_WDU_RESET_PIPE, h, pResetPipe, sizeof(WDU_RESET_PIPE), FALSE);
#define WD_UTransfer(h, pTrans) \
    WD_FUNCTION(IOCTL_WDU_TRANSFER, h, pTrans, sizeof(WDU_TRANSFER), TRUE);
#define WD_UHaltTransfer(h, pHaltTrans) \
    WD_FUNCTION(IOCTL_WDU_HALT_TRANSFER, h, pHaltTrans, sizeof(WDU_HALT_TRANSFER), FALSE);

#ifdef __cplusplus
}
#endif

#endif

