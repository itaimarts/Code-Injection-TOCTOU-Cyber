#include "stdafx.h"

#pragma comment(lib, "advapi32.lib")
#include <aclapi.h>

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

#define TEMP_FILE_TEMPLATE "att"
#define EXSISTING_FILE_NAME "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe"

#define SVCNAME TEXT("vulnerableService")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;


void PrintError(LPCTSTR errDesc);
LPCTSTR ErrorMessage(DWORD error);
LPTSTR createTempFile();

//Housekeeping functions

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}
}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			4150,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}



bool startupService() {
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return false;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	return true;
}


//  ErrorMessage support function.
//  Retrieves the system error message for the GetLastError() code.
//  Note: caller must use LocalFree() on the returned LPCTSTR buffer.
LPCTSTR ErrorMessage(DWORD error) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
		| FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	return((LPCTSTR)lpMsgBuf);
}

//  PrintError support function.
//  Simple wrapper function for error output.
void PrintError(LPCTSTR errDesc) {
	LPCTSTR errMsg = ErrorMessage(GetLastError());
	_tprintf(TEXT("\n** ERROR ** %s: %s\n"), errDesc, errMsg);
	LocalFree((LPVOID)errMsg);
}

LPTSTR createTempFile() {
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hTempFile = INVALID_HANDLE_VALUE;

	BOOL fSuccess = FALSE;
	DWORD dwRetVal = 0;
	UINT uRetVal = 0;

	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;

	TCHAR szTempFileName[MAX_PATH];
	TCHAR lpTempPathBuffer[MAX_PATH];
	char  chBuffer[MAX_PATH];

	LPCTSTR errMsg;

	//  Gets the temp path env string (no guarantee it's a valid path).
	dwRetVal = GetTempPath(MAX_PATH,          // length of the buffer
		lpTempPathBuffer); // buffer for path 
	if (dwRetVal > MAX_PATH || (dwRetVal == 0)) {
		PrintError(TEXT("GetTempPath failed"));
		if (!CloseHandle(hFile)) {
			PrintError(TEXT("CloseHandle(hFile) failed"));
			return NULL;
		}
		return NULL;
	}

	//  Generates a temporary file name. 
	uRetVal = GetTempFileName(lpTempPathBuffer, 			 // directory for tmp files
		TEXT(TEMP_FILE_TEMPLATE),     // temp file name prefix 
		0,                		   // create unique name 
		szTempFileName);            // buffer for name 
	if (uRetVal == 0) {
		PrintError(TEXT("GetTempFileName failed"));
		if (!CloseHandle(hFile)) {
			PrintError(TEXT("CloseHandle(hFile) failed"));
			return NULL;
		}
		return NULL;
	}


	//  Creates the new file to write to for the upper-case version.
	hTempFile = CreateFile((LPTSTR)szTempFileName, // file name 
		GENERIC_WRITE,        // open for write 
		FILE_SHARE_WRITE,                    // do not share 
		NULL,                 // default security 
		CREATE_ALWAYS,        // overwrite existing
		FILE_ATTRIBUTE_NORMAL,// normal file 
		NULL);                // no template 
	if (hTempFile == INVALID_HANDLE_VALUE) {
		PrintError(TEXT("Second CreateFile failed"));
		if (!CloseHandle(hFile)) {
			PrintError(TEXT("CloseHandle(hFile) failed"));
			return NULL;
		}
		return NULL;
	}
	

	CloseHandle(hTempFile);
	bool success = CopyFile(TEXT(EXSISTING_FILE_NAME), szTempFileName, FALSE);
	if (!success) {
	PrintError(TEXT("CopyFile failed"));
	return NULL;
	}

	_tprintf(TEXT("%s"), szTempFileName);
	return (LPTSTR)szTempFileName;
}



int SvcInit() {

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	LPTSTR szTempFileName;

	szTempFileName = createTempFile();

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	Sleep(5000);

	//Now we'll execute the resource file
	boolean fSuccess = CreateProcess(NULL, szTempFileName,
		NULL, NULL, NULL,
		0, NULL, NULL, &si, &pi);
	if (!fSuccess) {
		PrintError(TEXT("CreateProcess failed"));
		Sleep(2000);
		return 5;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);
	PrintError(TEXT("Finished waiting"));

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DeleteFile(szTempFileName);
	return 0;
}

void main(void) {
	SvcInit();
	return;
}





