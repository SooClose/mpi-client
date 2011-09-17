#include "MPI Client.h"

// returns open process handle
HANDLE InjectDLL( DWORD dwPID, LPCWSTR szDLLPath, HMODULE* lphInjected ) {
  int     cszDLL;
  LPVOID  lpAddress;
  HMODULE hMod;
  HANDLE  hThread;
  HANDLE  hProcess = OpenProcess( PROCESS_CREATE_THREAD | 
      PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
      PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwPID );

  if( hProcess == NULL ) {
    return NULL;
  }

  cszDLL = ( wcslen( szDLLPath ) + 1 ) * sizeof( WCHAR );
  
  // Injection
  lpAddress = VirtualAllocEx( hProcess, NULL, cszDLL, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
  if( lpAddress == NULL ) {
    return NULL;
  }

  WriteProcessMemory( hProcess, lpAddress, szDLLPath, cszDLL, NULL );

  hMod = GetModuleHandle( L"kernel32.dll" );
  if( hMod == NULL ) {
    return NULL;
  }

  hThread = CreateRemoteThread( hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)( GetProcAddress( hMod,
        "LoadLibraryW" ) ), lpAddress, 0, NULL );

  // Locate address our payload was loaded
  if( hThread != 0 ) {
    WaitForSingleObject( hThread, INFINITE );
    GetExitCodeThread( hThread, ( LPDWORD )lphInjected );
    VirtualFreeEx( hProcess, lpAddress, 0, MEM_RELEASE );
    CloseHandle( hThread );
  }

  return hThread != 0 ? hProcess : NULL;
}

void* GetPayloadExportAddr( LPCWSTR lpPath, HMODULE hPayloadBase, LPCSTR lpFunctionName ) {
  // Load payload in our own virtual address space
  HMODULE hLoaded = LoadLibrary( lpPath );

  if( hLoaded == NULL ) {
    return NULL;
  } else {
    void* lpFunc   = GetProcAddress( hLoaded, lpFunctionName );
    DWORD dwOffset = (DWORD)lpFunc - (DWORD)hLoaded;
    
    FreeLibrary( hLoaded );
    return (DWORD)hPayloadBase + dwOffset;
  }
}

BOOL InitPayload( HANDLE hProcess, LPCWSTR lpPath, HMODULE hPayloadBase, HWND hwndDlg ) {
  void* lpInit = GetPayloadExportAddr( lpPath, hPayloadBase, "Init" );
  if( lpInit == NULL ) {
    return FALSE;
  } else {
    HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0,
        lpInit, hwndDlg, 0, NULL );

    if( hThread == NULL ) {
      return FALSE;
    } else {
      CloseHandle( hThread );
    }
  }

  return TRUE;
}

BOOL CleanupPayload( HANDLE hProcess, LPCWSTR lpPath, HMODULE hPayloadBase ) {
  void* lpCleanup = GetPayloadExportAddr( lpPath, hPayloadBase, "Cleanup" );
  if( lpCleanup == NULL ) {
    return FALSE;
  } else {
    HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0,
        lpCleanup, NULL, 0, NULL );

    if( hThread == NULL ) {
      return FALSE;
    } else {
      CloseHandle( hThread );
    }
  }

  return TRUE;
}

void CreateTabs( HWND hwndDlg, HWND* lpHwndPlain, HWND* lpHwndFormatted ) {
  TC_ITEM tci     = {0};
  HWND    hwndTab = GetDlgItem( hwndDlg, IDC_TAB );

  // Add tabs
  tci.mask    = TCIF_TEXT;
  tci.pszText = L"Plain Log";
  TabCtrl_InsertItem( hwndTab, 0, &tci );

  tci.pszText = L"Formatted Log";
  TabCtrl_InsertItem( hwndTab, 1, &tci );

  // Add tab contents. Also pass in parent window ( this ) as lParam.
  // This is later used to share the image list.
  *lpHwndPlain     = CreateDialogParam( GetModuleHandle( NULL ),
      MAKEINTRESOURCE( IDD_PLAIN ), hwndTab, PlainDialogProc, ( LPARAM )hwndDlg );
  *lpHwndFormatted = CreateDialogParam( GetModuleHandle( NULL ),
      MAKEINTRESOURCE( IDD_FORMATTED ), hwndTab, FormattedDialogProc, ( LPARAM )hwndDlg );
}

// DialogProc for main window
INT_PTR CALLBACK MPIProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  static HWND       hwndPlain      = NULL;
  static HWND       hwndFormatted  = NULL;
  static int        nLastActiveTab = 0;
  static HIMAGELIST hImageList     = NULL;

  static HANDLE     hProcess       = NULL;
  static HMODULE    hInjected      = NULL;

  switch( uMsg ) {
    case WM_INITDIALOG: {
      WCHAR lpPath[MAX_PATH] = {0};
      DWORD cbData           = sizeof( lpPath );

      // Check payload exists at alleged location
      if( RegGetValue( HKEY_CURRENT_USER, REGPATH_SUBKEY, REGVAL_LOCATION,
          RRF_RT_REG_SZ, NULL, lpPath, &cbData ) == ERROR_SUCCESS ) {
        if( !PathFileExists( lpPath ) ) {
          MessageBox( hwndDlg, L"Payload does not exist at specified location",
              L"Please restart MPI with valid settings", MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        }

        // Inject payload
        if( ( hProcess = InjectDLL( lParam, lpPath, &hInjected ) ) == NULL ) {
          MessageBox( hwndDlg, L"Payload injection failed",
              L"Exiting MPI :(", MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        }

        // Initialise payload
        if( !InitPayload( hProcess, lpPath, hInjected, hwndDlg ) ) {
          MessageBox( hwndDlg, L"Problem with initialising payload",
              L"Exiting MPI :(", MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        }
      } else {
        MessageBox( hwndDlg, L"Problem reading payload location from registry",
            L"Exiting MPI :(", MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        break;
      }

      CreateTabs( hwndDlg, &hwndPlain, &hwndFormatted );
      break;
    }
    // Packet received in its raw form
    case WM_COPYDATA: {
      // Allocate memory locally and copy and return
      // Before returning, PostMessage WM_NEWPACKET
      COPYDATASTRUCT cds    = {0};
      DWORD          nSize  = ( ( PCOPYDATASTRUCT )lParam ) -> cbData;
      PVOID          lpData = malloc( nSize );
      PVOID          lpPI   = malloc( sizeof( PACKET_INFO ) );

      if( lpData == NULL || lpPI == NULL ) {
        MessageBox( hwndDlg, L"Problem allocating memory for receiving packet",
            L"Exiting MPI :(", MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        break;
      }

      memcpy_s( lpData, nSize, ( ( PCOPYDATASTRUCT )lParam ) -> lpData, nSize );
      
      ( ( PACKET_INFO* )lpPI ) -> cbData = nSize;
      ( ( PACKET_INFO* )lpPI ) -> dwData = ( ( PCOPYDATASTRUCT )lParam ) -> dwData;
      PostMessage( hwndDlg, WM_NEWPACKET, ( WPARAM )lpPI, ( LPARAM )lpData );
      break;
    }
    // Send the received packet to the 2 different packet windows
    // Free allocated memory from above message
    case WM_NEWPACKET: {
      if( hwndPlain == NULL || hwndFormatted == NULL ) {
        MessageBox( hwndDlg, L"Received packet before views have initialised",
            L"Exiting MPI :(", MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        break;
      }

      SendMessage( hwndPlain, WM_NEWPACKET, wParam, lParam );
      SendMessage( hwndFormatted, WM_NEWPACKET, wParam, lParam );
      free( ( PVOID )lParam );
      free( ( PVOID )wParam );
      break;
    }
    case WM_TIMER: {
      switch( wParam ) {
        case IMAGELIST_TIMER:
          SendMessage( hwndDlg, WM_IMAGELISTREADY, ( WPARAM )hImageList, 0 );
          break;
        default:
          return FALSE;
      }
      break;
    }
    // If window handles are not ready yet then set timer to wait
    case WM_IMAGELISTREADY: {
      if( hwndPlain == NULL || hwndFormatted == NULL ) {
        hImageList = ( HIMAGELIST )wParam;
        SetTimer( hwndDlg, IMAGELIST_TIMER, 100, NULL );
      } else {
        SendMessage( hwndPlain, WM_IMAGELISTREADY, wParam, lParam );
        SendMessage( hwndFormatted, WM_IMAGELISTREADY, wParam, lParam );
        KillTimer( hwndDlg, IMAGELIST_TIMER );
      }

      break;
    }
    case WM_NOTIFY: {
      switch( ( ( NMHDR* )lParam ) -> code ) {
        // Switching tabs
        case TCN_SELCHANGE: {
          HWND hwndTab = GetDlgItem( hwndDlg, IDC_TAB );

          if( hwndPlain == NULL || hwndFormatted == NULL ) {
            MessageBox( hwndDlg,
                L"Interaction attempted with tabs before views have been initialised",
                L"Exiting MPI :(",
                MB_OK | MB_ICONEXCLAMATION );
            SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
            break;
          }

          switch( nLastActiveTab ) {
            case 0:
              ShowWindow( hwndPlain, SW_HIDE );
              break;
            case 1:
              ShowWindow( hwndFormatted, SW_HIDE );
              break;
          }

          nLastActiveTab = TabCtrl_GetCurFocus( hwndTab );

          switch( nLastActiveTab ) {
            case 0:
              ShowWindow( hwndPlain, SW_SHOW );
              break;
            case 1:
              ShowWindow( hwndFormatted, SW_SHOW );
              break;
          }

          break;
        }
        default:
          return FALSE;
      }

      break;
    }
    case WM_CLOSE: {
      WCHAR lpPath[MAX_PATH] = {0};
      DWORD cbData           = sizeof( lpPath );

      // At this point we should be able to assume the path is valid
      RegGetValue( HKEY_CURRENT_USER, REGPATH_SUBKEY, REGVAL_LOCATION,
          RRF_RT_REG_SZ, NULL, lpPath, &cbData );

      CleanupPayload( hProcess, lpPath, hInjected );

      // Give content tabs a chance to cleanup
      if( hwndPlain != NULL ) {
        SendMessage( hwndPlain, WM_CLOSE, 0, 0 );
      }

      if( hwndFormatted != NULL ) {
        SendMessage( hwndFormatted, WM_CLOSE, 0, 0 );
      }

      if( hProcess != NULL ) {
        CloseHandle( hProcess );
      }
      EndDialog( hwndDlg, 0 );
      break;
    }
    default:
      return FALSE;
  }

  return TRUE;
}