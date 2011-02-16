#include "MPI Client.h"

HMODULE hInjected = NULL;

// returns open process handle
HANDLE InjectDLL( DWORD dwPID, LPTSTR szDLLPath ) {
  HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | 
                 PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                 PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwPID );

  if( hProcess == NULL ) {
    return NULL;
  }

  int cszDLL = ( _tcslen( szDLLPath ) + 1 ) * sizeof TCHAR;
  
  LPVOID lpAddress = VirtualAllocEx( hProcess, NULL, cszDLL, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
  WriteProcessMemory( hProcess, lpAddress, szDLLPath, cszDLL, NULL );
  HANDLE hThread   = CreateRemoteThread( hProcess, NULL, NULL,
                         (LPTHREAD_START_ROUTINE)( GetProcAddress( GetModuleHandle( _T("kernel32.dll") ),
                         LOAD_LIB_NAME ) ), lpAddress, NULL, NULL );

  if( hThread != 0 ) {
    WaitForSingleObject( hThread, INFINITE );
    GetExitCodeThread( hThread, ( LPDWORD )&hInjected );
    VirtualFreeEx( hProcess, lpAddress, NULL, MEM_RELEASE );
    CloseHandle( hThread );
  }

  return hThread != 0 ? hProcess : NULL;
}

INT_PTR CALLBACK MPIProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      TCHAR lpPath[MAX_PATH] = {0};
      DWORD cbData           = sizeof( lpPath );

      if( RegGetValue( HKEY_CURRENT_USER, REGPATH_SUBKEY, REGVAL_LOCATION,
          RRF_RT_REG_SZ, NULL, lpPath, &cbData ) == ERROR_SUCCESS ) {
        HANDLE  hProcess = NULL;
        HMODULE hLoaded  = NULL;
        FARPROC lpFunc   = NULL;
        DWORD   dwOffset = 0;

        if( ( hProcess = InjectDLL( lParam, lpPath ) ) == NULL ) {
          MessageBox( hwndDlg, _T("Problem injecting payload. Exiting.."),
              NULL, MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        }

        hLoaded = LoadLibrary( lpPath );
        lpFunc  = GetProcAddress( hLoaded, "Init" );

        dwOffset = ( DWORD )lpFunc - ( DWORD )hLoaded;
        FreeLibrary( hLoaded );

        HANDLE hThread = CreateRemoteThread( hProcess, NULL, NULL,
            ( LPTHREAD_START_ROUTINE )( dwOffset + ( DWORD )hInjected ),
            hwndDlg, NULL, NULL );

        CloseHandle( hProcess );

        if( hThread == NULL ) {
          MessageBox( hwndDlg, _T("Problem with initial communications. Exiting.."),
              NULL, MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        } else {
          CloseHandle( hThread );
        }
      } else {
        MessageBox( hwndDlg, _T("Problem reading payload location from registry. Exiting.."),
            NULL, MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
      }

      break;
    }
    default:
      return false;
  }

  return true;
}