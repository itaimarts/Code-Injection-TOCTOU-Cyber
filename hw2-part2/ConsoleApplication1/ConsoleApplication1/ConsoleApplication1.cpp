#include "stdafx.h"
#include <stdio.h>  
#include <windows.h>
#include <Winternl.h>
#include <string.h>
#include <tchar.h>

#include <strsafe.h>
#pragma comment(lib, "User32.lib")



typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList; // 8 byte
	LIST_ENTRY InMemoryOrderModuleList; // 8 byte
	LIST_ENTRY InInitializationOrderModuleList; // 8 byte
	PVOID BaseAddress; // 4 byte - offset 24 byte = 0x18
	PVOID EntryPoint; // 4 byte
	ULONG SizeOfImage; // 4 byte
	UNICODE_STRING FullDllName; // full path - 2 byte length, 2 byte maximum length + 4 byte pointer to unicode string => 8 byte total
	UNICODE_STRING BaseDllName; // name  - pointer to unicode string after 4 bytee - OFFSET = 48 byte = 0x30
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _EXPORT_DIRECTORY_TABLE {
	DWORD ExportFlags; // 4 byte
	DWORD TimeStamp; // 4 byte
	WORD MajorVersion; // 2 byte
	WORD MinorVersion; // 2 byte
	DWORD NameRVA; // 4 byte
	DWORD OrdinalBase; // 4 byte
	DWORD ExportAddressTableSize; // 4 byte 
	DWORD NamePointerTableSize; // 4 byte
	DWORD ExportAddressTableRVA; // 4 byte =>  total = 28 byte = 0x1C
	DWORD NamePointerTableRVA; // 4 byte => total = 32 byte = 0x20
	DWORD OrdinalTableRVA; // 4 byte
} EXPORT_DIRECTORY_TABLE, *PEXPORT_DIRECTORY_TABLE;



void displayError(LPTSTR lpszFunction) {
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	lpDisplayBuf =
		(LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf)
			+ lstrlen((LPCTSTR)lpszFunction)
			+ 40) // account for format string
			* sizeof(TCHAR));

	if (FAILED(StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error code %d as follows:\n%s"),
		lpszFunction,
		dw,
		lpMsgBuf))){
		printf("FATAL ERROR: Unable to output error code.\n");
	}

	_tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

//#define IMAGE_DIRECTORY_ENTRY_EXPORT 0

void main() {
	

	_PEB_LDR_DATA * ldr; 
	_LDR_MODULE * ldrModule;
	_IMAGE_DOS_HEADER * dosHeader;

	PIMAGE_NT_HEADERS32 ntHeader;
	PEXPORT_DIRECTORY_TABLE EdtPtr;

	char * funcName;

	int ids = IMAGE_DOS_SIGNATURE;
	int ints = IMAGE_NT_SIGNATURE;

	char command[] = "cmd.exe /C dir > b.txt";


	__asm
	{
		/* find kernel32.dll base address */
		XOR EBX, EBX
		MOV EBX, FS:[0x18]			// Get address of TIB
		MOV EBX, FS : [0x30]			//Get PEB address
		MOV EBX, [EBX + 0x0C]        //Get pointer to LDR
		MOV ldr, EBX
		MOV EBX, [EBX + 0x0C]        // EBX = address of InLoadOrder list- LDR_MODULE
		MOV ldrModule, EBX

		PUSH 0x006C0065				// Push "kernel" in Unicode onto the stack - 6 byte chars + 6 byte space
		PUSH 0x006E0072				// little endian
		PUSH 0x0065006B
		CLD							// Clear Direction Flag -  DF = 0
		XOR ECX, ECX


		find_kernel32_dll :
			MOV EBX, [EBX]				// Go to the next module (InLoadOrder - forward link)
			MOV ldrModule, EBX
			MOV ESI, ESP				// ESI = "kernel" (Unicode)
			MOV EDI, [EBX + 0x30]		// EDI = Module's name (Unicode)
			MOV CL, 12					// CL - 8 bit - 3 is for 3 DWORD - DWORD = 4 byte so 12 BYTE in total to compare with KERNEL (6 byte character + 6 byte spaces)
			REPE CMPSB					//  compare DWORD (= 4 byte) 
			JNZ SHORT find_kernel32_dll
		
		/* Find kernel32.dll's exported function CreateProcessW  */
		MOV EBX, [EBX + 0x18]			// EBX = kernel32.dll's base address = DOS_HEADER
		MOV dosHeader, EBX				// DEBUGGING DOS header

		MOV EDX, [EBX + 0x3C]			// PE header (RVL) - in IMAGE_DOS_HEADER -> offset to extended header
		ADD EDX, EBX					// Add base address offset to edx - to get actual VA - edx is relative address
		MOV ntHeader, EDX				// DEBUGGING PE header

		MOV EDX, [EDX + 0x78]		    //EDX = kernel32.dll's export directory (RVA) - in IMAGE_NT_HEADERS (0x18)-> IMAGE_OPTIONAL_HEADERS (0x60)-> IMAGE_DATA_DIRECTORY[0]->virtual address - offset 0x0
		ADD EDX, EBX					//Add base address offset to edx - to get actual VA
		MOV EdtPtr, EDX					//DEBUGGING PEXPORT_DIRECTORY_TABLE


		MOV EAX, [EDX + 0x1C]			//Address of exported funcs (EAX = EAT (RVA))
		MOV EDX, [EDX + 0x20]			//EDX = Address of function names (RVA)
		ADD EDX, EBX					//EDX = Address of function names (VA)
		ADD EAX, EBX					//EAX = EAT (VA)

		XOR ECX, ECX

		// push WinExec to stack - 8 bytes
		PUSH 0x00636578			// cex
		PUSH 0x456E6957			// EniW
		get_exports :
			MOV ESI, ESP					// ESI = 'CreateProcessW'
			MOV EDI, [EDX]					// EDI = Exported function name - (RVA)
			ADD EDI, EBX					// EDI = Exported function name - (VA)
			MOV funcName, EDI				// DEBUGGING - function name


			MOV CL, 0x07					// length of 'WinExec' - 7 bytes
			REPE CMPSB						
			JE SHORT found_set_context		//Jump if this is our function
			ADD EDX, 4						//Next function name
			ADD EAX, 4						//Next function address
			JMP SHORT get_exports

		found_set_context :
			
			MOV EDX, [EAX]					// EDX = address of CreateProcessW - (RVA)
			ADD EDX, EBX					// EDX = address of CreateProcessW - (VA)
			
			
			// building stack for WinExec
			push 0x00000000
			push 0x7478742E
			push 0x615C7377
			push 0x6F646E69
			push 0x575C3A43
			push 0x203E2032
			push 0x206F6863
			push 0x6520432F
			push 0x20657865
			push 0x2E646D63


			MOV ESI, ESP   // ESI = 'cmd.exe /C dir > a.txt\0'
			XOR ECX, ECX
			PUSH ECX	  // second argument for WinExec
			PUSH esi	 // first argumnt of WinExec - 'cmd.exe'
			
			call edx;

			add esp, 60
			
	}


	return;


}