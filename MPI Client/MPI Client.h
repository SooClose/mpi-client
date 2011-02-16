#ifndef _MPI_H
#define _MPI_H

#include <Windows.h>
#include <tchar.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")
#include <Uxtheme.h>
#pragma comment(lib, "UxTheme.lib")
#include <TlHelp32.h>
#include "resource.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef  UNICODE
#define LOAD_LIB_NAME "LoadLibraryW"
#else
#define LOAD_LIB_NAME "LoadLibraryA"
#endif

#define REGPATH_SUBKEY  _T("Software\\MPI")
#define REGVAL_LOCATION _T("Payload Location")

INT_PTR CALLBACK InjectorProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK MPIProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK PlainDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

#endif