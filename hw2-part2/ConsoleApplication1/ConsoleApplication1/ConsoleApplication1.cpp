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
	
		XOR EBX, EBX
		MOV EBX, FS:[0x18]			
		MOV EBX, FS : [0x30]		
		MOV EBX, [EBX + 0x0C]       
		MOV EBX, [EBX + 0x0C]       

		PUSH 0x006C0065				
		PUSH 0x006E0072				
		PUSH 0x0065006B
		CLD							
		XOR ECX, ECX


		find_kernel32_dll :
			MOV EBX, [EBX]				
			MOV ESI, ESP			
			MOV EDI, [EBX + 0x30]	
			MOV CL, 12				
			REPE CMPSB			
			JNZ SHORT find_kernel32_dll
		
		
		MOV EBX, [EBX + 0x18]	

		MOV EDX, [EBX + 0x3C]	
		ADD EDX, EBX			

		MOV EDX, [EDX + 0x78]
		ADD EDX, EBX					
		
		MOV EAX, [EDX + 0x1C]			
		MOV EDX, [EDX + 0x20]			
		ADD EDX, EBX					
		ADD EAX, EBX					

		XOR ECX, ECX

		
		PUSH 0x00636578			
		PUSH 0x456E6957			
		get_exports :
			MOV ESI, ESP				
			MOV EDI, [EDX]				
			ADD EDI, EBX				

			MOV CL, 0x07				
			REPE CMPSB						
			JE SHORT found_set_context	
			ADD EDX, 4			
			ADD EAX, 4			
			JMP SHORT get_exports

		found_set_context :
			MOV EDX, [EAX]		
			ADD EDX, EBX		
			
			
			
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


			MOV ESI, ESP 
			XOR ECX, ECX
			PUSH ECX	
			PUSH ESI	
			
			call edx;
			add esp, 60


				MOV EDX, [EBX + 0x3C]	
				ADD EDX, EBX			

				MOV EDX, [EDX + 0x78]	
				ADD EDX, EBX			

				MOV EAX, [EDX + 0x1C]	
				MOV EDX, [EDX + 0x20]	
				ADD EDX, EBX			
				ADD EAX, EBX			

				XOR ECX, ECX

				PUSH 0x00737365 
				PUSH 0x636F7250 
				PUSH 0x74697845 

				get_exports_2 :
				MOV ESI, ESP		
				MOV EDI, [EDX]	
				ADD EDI, EBX	

				MOV CL, 11		
				REPE CMPSB
				JE SHORT found_set_context_2	
				ADD EDX, 4						
				ADD EAX, 4						
				JMP SHORT get_exports_2

				found_set_context_2 :
				MOV EDX, [EAX]					
				ADD EDX, EBX				

				XOR ECX, ECX
				PUSH ECX
				CALL EDX

				ADD ESP, 8
			
	}


	return;


}