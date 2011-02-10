#include "MPI Client.h"

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
  TCHAR          str[16]       = {0};
  int            iIndex        = 0;
  HANDLE         hSnapshot     = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

  ListView_DeleteAllItems( hwndList );

  ProcessStruct.dwSize = sizeof ProcessStruct;
  Process32First( hSnapshot, &ProcessStruct );

  lvI.mask     = LVIF_TEXT | LVIF_PARAM;
  lvI.iSubItem = 0;
  lvI.pszText  = str;

  do {
    lvI.iItem = iIndex + 1;
    _ultot_s( ProcessStruct.th32ProcessID, str, _countof( str ), 10 );

    lvI.lParam = ProcessStruct.th32ProcessID;
    iIndex = ListView_InsertItem( hwndList, &lvI );
    ListView_SetItemText( hwndList, iIndex, 0, (LPTSTR)&ProcessStruct.szExeFile );
    ListView_SetItemText( hwndList, iIndex, 1, str );
  }
  while( Process32Next( hSnapshot, &ProcessStruct ) );

  CloseHandle( hSnapshot );
}

INT_PTR CALLBACK InjectorProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      HWND hwndList = GetDlgItem( hwndDlg, IDC_PROCESSLIST );
      InitProcessList( hwndList );
      FillProcessList( hwndList );

      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_PROCESSLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_PROCESSLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );
      break;
    }
    case WM_NOTIFY: {
      if( ( ( LPNMHDR )lParam ) -> hwndFrom == GetDlgItem( hwndDlg, IDC_PROCESSLIST ) &&
          ( ( ( LPNMHDR )lParam ) -> code == NM_DBLCLK ) ) {
        HWND hwndList = GetDlgItem( hwndDlg, IDC_PROCESSLIST );
        TCHAR  szProcess[64] = {0};
        int    nIndex        = ( ( LPNMITEMACTIVATE )lParam ) -> iItem;

        if( nIndex != -1 ) {
          ListView_GetItemText( hwndList, nIndex, 0, szProcess, _countof( szProcess ) );
          MessageBox( 0, szProcess, NULL, NULL );
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