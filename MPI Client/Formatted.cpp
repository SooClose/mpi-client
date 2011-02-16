#include "MPI Client.h"

void DisplayFormattedPacket( DWORD nSize, PVOID lpData, HWND hwndDlg, int nIDDlgItem ) {
  HWND   hwndList  = GetDlgItem( hwndDlg, nIDDlgItem );
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

  char* szNormalized = ( char* )malloc( ( nSize + 1 ) );
  RtlZeroMemory( szNormalized, ( nSize + 1 ) );
  memcpy_s( szNormalized, nSize + 1, lpData, nSize );

  for( unsigned int i = 0; i < nSize; i++ ) {
    if( !isprint( ( ( ( const byte* )lpData )[i] ) ) ) {
      szNormalized[i] = '.';
    }
  }

  TCHAR* szBuffer = ( TCHAR* )malloc( ( nSize + 1 ) * sizeof( TCHAR ) );
  RtlZeroMemory( szBuffer, ( nSize + 1 ) * sizeof( TCHAR ) );

  MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szNormalized, -1, szBuffer, nSize + 1 );

  ListView_SetItemText( hwndList, lvi.iItem, 2, szBuffer );
  free( szBuffer );
  free( szNormalized );
}

INT_PTR CALLBACK FormattedDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_FORMATTEDLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_FORMATTEDLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );

      EnableThemeDialogTexture( hwndDlg, ETDT_USETABTEXTURE );
      InitPacketList( hwndDlg, IDC_FORMATTEDLIST );
      break;
    }
    case WM_NEWPACKET: {
      DisplayFormattedPacket( wParam, ( PVOID )lParam, hwndDlg, IDC_FORMATTEDLIST );
      break;
    }
    default:
      return false;
  }

  return true;
}