#include "MPI Client.h"

// returns open process handle
HANDLE InjectDLL( DWORD dwPID, LPTSTR szDLLPath, HMODULE* lphInjected ) {
  HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | 
                 PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                 PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwPID );

  if( hProcess == NULL ) {
    return NULL;
  }

  int cszDLL = ( _tcslen( szDLLPath ) + 1 ) * sizeof TCHAR;
  
  // Injection
  LPVOID lpAddress = VirtualAllocEx( hProcess, NULL, cszDLL, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
  WriteProcessMemory( hProcess, lpAddress, szDLLPath, cszDLL, NULL );
  HANDLE hThread   = CreateRemoteThread( hProcess, NULL, NULL,
                         (LPTHREAD_START_ROUTINE)( GetProcAddress( GetModuleHandle( _T("kernel32.dll") ),
                         LOAD_LIB_NAME ) ), lpAddress, NULL, NULL );

  // Locate address our payload was loaded
  if( hThread != 0 ) {
    WaitForSingleObject( hThread, INFINITE );
    GetExitCodeThread( hThread, ( LPDWORD )lphInjected );
    VirtualFreeEx( hProcess, lpAddress, NULL, MEM_RELEASE );
    CloseHandle( hThread );
  }

  return hThread != 0 ? hProcess : NULL;
}

// DialogProc for main window
INT_PTR CALLBACK MPIProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  static HWND       hwndPlain      = NULL;
  static HWND       hwndFormatted  = NULL;
  static int        nLastActiveTab = 0;
  static HIMAGELIST hImageList = NULL;

  switch( uMsg ) {
    case WM_INITDIALOG: {
      TCHAR lpPath[MAX_PATH] = {0};
      DWORD cbData           = sizeof( lpPath );

      TC_ITEM tci     = {0};
      HWND    hwndTab = GetDlgItem( hwndDlg, IDC_TAB );

      // Add tabs
      tci.mask    = TCIF_TEXT;
      tci.pszText = _T("Plain Log");
      TabCtrl_InsertItem( hwndTab, 0, &tci );

      tci.pszText = _T("Formatted Log");
      TabCtrl_InsertItem( hwndTab, 1, &tci );

      // Add tab contents. Also pass in parent window ( this ) as lParam.
      // This is later used to share the image list.
      hwndPlain     = CreateDialogParam( GetModuleHandle( NULL ),
          MAKEINTRESOURCE( IDD_PLAIN ), hwndTab, PlainDialogProc, ( LPARAM )hwndDlg );
      hwndFormatted = CreateDialog( GetModuleHandle( NULL ),
          MAKEINTRESOURCE( IDD_FORMATTED ), hwndTab, FormattedDialogProc );

      // Check payload exists at alleged location
      if( RegGetValue( HKEY_CURRENT_USER, REGPATH_SUBKEY, REGVAL_LOCATION,
          RRF_RT_REG_SZ, NULL, lpPath, &cbData ) == ERROR_SUCCESS ) {
        if( !PathFileExists( lpPath ) ) {
          MessageBox( hwndDlg, _T("Payload does not exist at specified location. Exiting.."),
              NULL, MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        }

        HANDLE  hProcess  = NULL;
        HMODULE hLoaded   = NULL;
        HMODULE hInjected = NULL;
        FARPROC lpFunc    = NULL;
        DWORD   dwOffset  = 0;

        // Inject payload
        if( ( hProcess = InjectDLL( lParam, lpPath, &hInjected ) ) == NULL ) {
          MessageBox( hwndDlg, _T("Problem injecting payload. Exiting.."),
              NULL, MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        }

        // Load payload in our own virtual address space
        hLoaded = LoadLibrary( lpPath );
        lpFunc  = GetProcAddress( hLoaded, "Init" );

        // Calculate offset of exported function Init() from base
        dwOffset = ( DWORD )lpFunc - ( DWORD )hLoaded;
        FreeLibrary( hLoaded );

        // Use this offset to calculate VA for Init() in target and invoke it
        HANDLE hThread = CreateRemoteThread( hProcess, NULL, NULL,
            ( LPTHREAD_START_ROUTINE )( dwOffset + ( DWORD )hInjected ),
            hwndDlg, NULL, NULL );

        CloseHandle( hProcess );

        if( hThread == NULL ) {
          MessageBox( hwndDlg, _T("Problem with initial communications. Exiting.."),
              NULL, MB_OK | MB_ICONEXCLAMATION );
          SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
          break;
        } else {
          CloseHandle( hThread );
        }
      } else {
        MessageBox( hwndDlg, _T("Problem reading payload location from registry. Exiting.."),
            NULL, MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndDlg, WM_CLOSE, 0, 0 );
        break;
      }

      break;
    }
    // Packet received in its raw form
    case WM_COPYDATA: {
      // Allocate memory locally and copy and return
      // Before returning PostMessage WM_NEWPACKET
      COPYDATASTRUCT cds    = {0};
      DWORD          nSize  = ( ( PCOPYDATASTRUCT )lParam ) -> cbData;
      PVOID          lpData = malloc( nSize );
      PVOID          lpPI   = malloc( sizeof( PACKET_INFO ) );

      memcpy_s( lpData, nSize, ( ( PCOPYDATASTRUCT )lParam ) -> lpData, nSize );
      
      ( ( PACKET_INFO* )lpPI ) -> cbData = nSize;
      ( ( PACKET_INFO* )lpPI ) -> dwData = ( ( PCOPYDATASTRUCT )lParam ) -> dwData;
      PostMessage( hwndDlg, WM_NEWPACKET, ( WPARAM )lpPI, ( LPARAM )lpData );
      break;
    }
    // Send the received packet to the 2 different packet windows
    // Free allocated memory from above message
    case WM_NEWPACKET: {
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
          return false;
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
          return false;
      }

      break;
    }
    case WM_CLOSE: {
      // Give content tabs a chance to cleanup
      SendMessage( hwndPlain, WM_CLOSE, 0, 0 );
      SendMessage( hwndFormatted, WM_CLOSE, 0, 0 );
      EndDialog( hwndDlg, 0 );
      break;
    }
    default:
      return false;
  }

  return true;
}