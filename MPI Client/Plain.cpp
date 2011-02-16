#include "MPI Client.h"

void InitPacketList( HWND hwndDlg, int nIDDlgItem ) {
  HWND     hwndList = GetDlgItem( hwndDlg, nIDDlgItem );
  LVCOLUMN lvc      = {0};

  lvc.cx   = 40;
  lvc.fmt  = LVCFMT_LEFT;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  for( int iCol = 0; iCol < 3; iCol++ ) {
    lvc.iSubItem = iCol;

    switch( iCol ) {
      case 0:
        lvc.pszText = _T("ID");
        break;
      case 1:
        lvc.pszText = _T("Size");
        break;
      case 2:
        lvc.pszText = _T("Data");
        break;
    }

    ListView_InsertColumn( hwndList, iCol, &lvc );
  }

  ListView_SetColumnWidth( hwndList, 2, LVSCW_AUTOSIZE_USEHEADER );
}

void DisplayPacket( DWORD nSize, PVOID lpData, HWND hwndDlg, int nIDDlgItem ) {
  HWND   hwndList  = GetDlgItem( hwndDlg, IDC_PLAINLIST );
  TCHAR  szBuf[16] = {0};
  LVITEM lvi       = {0};

  lvi.mask     = LVIF_TEXT;
  lvi.iSubItem = 0;
  lvi.pszText  = szBuf;
  lvi.iItem    = ListView_GetItemCount( hwndList );

  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), lvi.iItem + 1 );
      
  ListView_InsertItem( hwndList, &lvi );
  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), nSize );
  ListView_SetItemText( hwndList, lvi.iItem, 1, szBuf );

  TCHAR* szBuffer = ( TCHAR* )malloc( ( 3 * nSize + 1 ) * sizeof( TCHAR ) );
  RtlZeroMemory( szBuffer, ( 3 * nSize + 1 ) * sizeof( TCHAR ) );

  for( unsigned int i = 0; i < nSize; i++ ) {
    _stprintf_s( &szBuffer[i*3], 3, _T("%02X"), ( ( const byte* )lpData )[i] );
    szBuffer[ ( i + 1 )*3 - 1 ] = *_T(" ");
  }

  ListView_SetItemText( hwndList, lvi.iItem, 2, szBuffer );
  free( szBuffer );
}

INT_PTR CALLBACK PlainDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );

      EnableThemeDialogTexture( hwndDlg, ETDT_USETABTEXTURE );

      InitPacketList( hwndDlg, IDC_PLAINLIST );
      break;
    }
    case WM_COPYDATA: {
      DWORD nSize  = ( ( PCOPYDATASTRUCT )lParam ) -> cbData;
      PVOID lpData = ( ( PCOPYDATASTRUCT )lParam ) -> lpData;

      DisplayPacket( nSize, lpData, hwndDlg, IDC_PLAINLIST );
    }
    default:
      return false;
  }

  return true;
}