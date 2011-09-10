#include "MPI Client.h"

// Display packets as hex
BOOL DisplayPacket( PACKET_INFO* lpPI, PVOID lpData, HWND hwndDlg, int nIDDlgItem ) {
  HWND   hwndList  = GetDlgItem( hwndDlg, nIDDlgItem );
  TCHAR  szBuf[16] = {0};
  LVITEM lvi       = {0};
  DWORD  nSize     = lpPI -> cbData;
  TCHAR* szBuffer;

  lvi.mask   = LVIF_IMAGE;
  lvi.iItem  = ListView_GetItemCount( hwndList );
  lvi.iImage = lpPI -> dwData;
  ListView_InsertItem( hwndList, &lvi );

  lvi.mask = LVIF_TEXT;
  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), lvi.iItem + 1 );
  ListView_SetItemText( hwndList, lvi.iItem, 1, szBuf );
  _stprintf_s( szBuf, _countof( szBuf ), _T("%d"), nSize );
  ListView_SetItemText( hwndList, lvi.iItem, 2, szBuf );

  szBuffer = malloc( ( 3 * nSize + 1 ) * sizeof( TCHAR ) );

  if( szBuffer != NULL ) {
    unsigned int i;
    RtlZeroMemory( szBuffer, ( 3 * nSize + 1 ) * sizeof( TCHAR ) );

    for( i = 0; i < nSize; i++ ) {
      _stprintf_s( &szBuffer[i*3], 3, _T("%02X"), ( ( const byte* )lpData )[i] );
      szBuffer[ ( i + 1 )*3 - 1 ] = *_T(" ");
    }

    ListView_SetItemText( hwndList, lvi.iItem, 3, szBuffer );
    free( szBuffer );
    return TRUE;
  } else {
    return FALSE;
  }
}

// DialogProc for plain tab
INT_PTR CALLBACK PlainDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  static HWND hwndParent;

  switch( uMsg ) {
    case WM_INITDIALOG: {
      DWORD      dwStyle;
      int        nIconWidth, nIconHeight;
      HINSTANCE  hInstance;
      HIMAGELIST hIML;
      HBITMAP    hbmArrows;

      hwndParent = (HWND)lParam;

      EnableThemeDialogTexture( hwndDlg, ETDT_ENABLETAB );
      dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );
      InitPacketList( hwndDlg, IDC_PLAINLIST );

      nIconWidth  = GetSystemMetrics( SM_CXSMICON );
      nIconHeight = GetSystemMetrics( SM_CYSMICON );

      // Set ImageList
      hInstance = GetModuleHandle( NULL );
      hIML      = ImageList_Create( nIconWidth, nIconHeight, ILC_COLOR, 2, 0 );
      hbmArrows = ( HBITMAP )LoadImage( hInstance, MAKEINTRESOURCE( IDB_ARROWS ), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT );
      
      ImageList_Add( hIML, hbmArrows, NULL );
      DeleteObject( hbmArrows );

      SendMessage( hwndParent, WM_IMAGELISTREADY, ( WPARAM )hIML, 0 );
      break;
    }
    case WM_NEWPACKET: {
      if( !DisplayPacket( ( PACKET_INFO* )wParam, ( PVOID )lParam, hwndDlg, IDC_PLAINLIST ) ) {
        MessageBox( hwndParent, _T("Failed to display plain packet"),
            _T("Exiting MPI"), MB_OK | MB_ICONEXCLAMATION );
        SendMessage( hwndParent, WM_CLOSE, 0, 0);
      }
      break;
    }
    case WM_IMAGELISTREADY: {
      ListView_SetImageList( GetDlgItem( hwndDlg, IDC_PLAINLIST ), ( HIMAGELIST )wParam, LVSIL_SMALL );
      break;
    }
    case WM_CLOSE: {
      // Cleanup resources
      ImageList_Destroy( ListView_GetImageList( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVSIL_SMALL ) );
      break;
    }
    default:
      return FALSE;
  }

  return TRUE;
}