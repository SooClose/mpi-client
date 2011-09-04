#include "MPI Client.h"

// Display packet as plaintext
bool DisplayFormattedPacket( PACKET_INFO* lpPI, LPBYTE lpData, HWND hwndDlg, int nIDDlgItem ) {
  HWND   hwndList  = GetDlgItem( hwndDlg, nIDDlgItem );
  TCHAR  szBuf[16] = {0};
  LVITEM lvi       = {0};
  DWORD  nSize     = lpPI -> cbData;

  lvi.mask   = LVIF_IMAGE;
  lvi.iItem  = ListView_GetItemCount( hwndList );
  lvi.iImage = lpPI -> dwData;
  ListView_InsertItem( hwndList, &lvi );

  lvi.mask = LVIF_TEXT;
  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), lvi.iItem + 1 );
  ListView_SetItemText( hwndList, lvi.iItem, 1, szBuf );
  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), nSize );
  ListView_SetItemText( hwndList, lvi.iItem, 2, szBuf );

  char* szNormalized = ( char* )malloc( ( nSize + 1 ) );

  if( szNormalized != NULL ) {
    RtlZeroMemory( szNormalized, ( nSize + 1 ) );
    memcpy_s( szNormalized, nSize + 1, lpData, nSize );

    for( unsigned int i = 0; i < nSize; i++ ) {
      if( !isprint( lpData [i] ) ) {
        szNormalized[i] = '.';
      }
    }

    // Nasty conversion needed..
    TCHAR* szBuffer = ( TCHAR* )malloc( ( nSize + 1 ) * sizeof( TCHAR ) );
    RtlZeroMemory( szBuffer, ( nSize + 1 ) * sizeof( TCHAR ) );

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szNormalized, -1, szBuffer, nSize + 1 );

    ListView_SetItemText( hwndList, lvi.iItem, 3, szBuffer );
    free( szBuffer );
    free( szNormalized );
    return true;
  } else {
    return false;
  }
}

// DialogProc for formatted tab
INT_PTR CALLBACK FormattedDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  static HWND hwndParent;

  switch( uMsg ) {
    case WM_INITDIALOG: {
      hwndParent = (HWND)lParam;

      EnableThemeDialogTexture( hwndDlg, ETDT_ENABLETAB );
      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_FORMATTEDLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_FORMATTEDLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );

      InitPacketList( hwndDlg, IDC_FORMATTEDLIST );
      break;
    }
    case WM_NEWPACKET: {
      if( !DisplayFormattedPacket( ( PACKET_INFO* )wParam, ( LPBYTE )lParam, hwndDlg, IDC_FORMATTEDLIST ) ) {
        MessageBox( hwndParent, _T("Failed to display formatted packet"),
            _T("Exiting MPI"), MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndParent, WM_CLOSE, NULL, NULL );
        break;
      }
      break;
    }
    case WM_IMAGELISTREADY: {
      ListView_SetImageList( GetDlgItem( hwndDlg, IDC_FORMATTEDLIST ), ( HIMAGELIST )wParam, LVSIL_SMALL );
      break;
    }
    case WM_CLOSE: {
      // Cleanup resources
      ImageList_Destroy( ListView_GetImageList( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVSIL_SMALL ) );
      break;
    }
    default:
      return false;
  }

  return true;
}