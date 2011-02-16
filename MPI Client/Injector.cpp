#include "MPI Client.h"

BOOL SetPath( HWND hwndDlg, LPTSTR lpPath, DWORD nMaxFile ) {
  OPENFILENAME ofn = {0};

  ofn.lStructSize = sizeof ofn;
  ofn.hwndOwner   = hwndDlg;
  ofn.lpstrFilter = _T("MPI Payload DLL\0*.dll\0\0");
  ofn.hInstance   = GetModuleHandle( NULL );
  ofn.lpstrFile   = lpPath;
  ofn.nMaxFile    = nMaxFile;
  ofn.lpstrTitle  = _T("Select MPI Payload");
  ofn.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_HIDEREADONLY;

  return GetOpenFileName( &ofn );
}

void InitProcessList( HWND hwndList ) {
  LVCOLUMN lvc = {0};
  lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  for( int iCol = 0; iCol < 2; iCol++ ) {
    lvc.iSubItem = iCol;
    lvc.pszText  = iCol ? _T("PID") : _T("Process Name");
    lvc.cx       = 150;
    lvc.fmt      = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, iCol, &lvc );
  }

  ListView_SetColumnWidth( hwndList, 1, LVSCW_AUTOSIZE_USEHEADER );
}

void FillProcessList( HWND hwndList ) {
  LVITEM         lvI           = {0};
  PROCESSENTRY32 ProcessStruct = {0};
  TCHAR          lpStr[16]     = {0};
  int            iIndex        = 0;
  HANDLE         hSnapshot     = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

  ListView_DeleteAllItems( hwndList );

  ProcessStruct.dwSize = sizeof ProcessStruct;
  Process32First( hSnapshot, &ProcessStruct );

  lvI.mask     = LVIF_TEXT | LVIF_PARAM;
  lvI.iSubItem = 0;
  lvI.pszText  = lpStr;

  do {
    lvI.iItem = iIndex + 1;
    _ultot_s( ProcessStruct.th32ProcessID, lpStr, _countof( lpStr ), 10 );

    lvI.lParam = ProcessStruct.th32ProcessID;
    iIndex = ListView_InsertItem( hwndList, &lvI );
    ListView_SetItemText( hwndList, iIndex, 0, (LPTSTR)&ProcessStruct.szExeFile );
    ListView_SetItemText( hwndList, iIndex, 1, lpStr );
  }
  while( Process32Next( hSnapshot, &ProcessStruct ) );

  CloseHandle( hSnapshot );
}

INT_PTR CALLBACK InjectorProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      TCHAR lpPath[MAX_PATH] = {0};
      DWORD cbData           = sizeof( lpPath );
      HWND  hwndList = GetDlgItem( hwndDlg, IDC_PROCESSLIST );
      InitProcessList( hwndList );
      FillProcessList( hwndList );

      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_PROCESSLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_PROCESSLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );

      if( RegGetValue( HKEY_CURRENT_USER, REGPATH_SUBKEY, REGVAL_LOCATION,
          RRF_RT_REG_SZ, NULL, lpPath, &cbData ) == ERROR_SUCCESS ) {
        SetDlgItemText( hwndDlg, IDC_DLLPATH, lpPath );
      }

      break;
    }
    case WM_COMMAND: {
      switch( LOWORD( wParam ) ) {
        case IDC_BROWSE: {
          TCHAR lpPath[MAX_PATH] = {0};
          HKEY  hkResult         = NULL;

          if( SetPath( hwndDlg, lpPath, _countof( lpPath ) ) != 0 ) {
            if( RegCreateKeyEx( HKEY_CURRENT_USER, REGPATH_SUBKEY, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkResult, NULL ) == ERROR_SUCCESS ) {
              if( RegSetValueEx( hkResult, REGVAL_LOCATION, 0, REG_SZ,
                  ( const BYTE* )lpPath, ( _tcsclen( lpPath ) + 1 ) * sizeof TCHAR ) != ERROR_SUCCESS ) {
                MessageBox( hwndDlg, _T("Problem setting registry entry for payload location"),
                    NULL, MB_OK | MB_ICONEXCLAMATION );
              } else {
                SetDlgItemText( hwndDlg, IDC_DLLPATH, lpPath );
              }

              if( RegCloseKey( hkResult ) != ERROR_SUCCESS ) {
                MessageBox( hwndDlg, _T("Problem closing registry handle \
                    during setting of payload location"), NULL, MB_OK | MB_ICONEXCLAMATION );
              }
            } else {
              MessageBox( hwndDlg, _T("Problem creating registry entry for payload location"),
                  NULL, MB_OK | MB_ICONEXCLAMATION );
            }
          }
          break;
        }
        default:
          return false;
      }

      break;
    }
    case WM_NOTIFY: {
      if( ( ( LPNMHDR )lParam ) -> hwndFrom == GetDlgItem( hwndDlg, IDC_PROCESSLIST ) &&
          ( ( ( LPNMHDR )lParam ) -> code == NM_DBLCLK ) ) {
        HWND hwndList = GetDlgItem( hwndDlg, IDC_PROCESSLIST );
        TCHAR  szPID[64] = {0};
        int    nIndex    = ( ( LPNMITEMACTIVATE )lParam ) -> iItem;

        if( nIndex != -1 ) {
          ListView_GetItemText( hwndList, nIndex, 1, szPID, _countof( szPID ) );
          EndDialog( hwndDlg, _tstol( szPID ) );
        }
      } else {
        return false;
      }

      return true;
    }
    case WM_CLOSE:
      EndDialog( hwndDlg, 0 );
      break;
    default:
      return false;
  }

  return true;
}