#include "MPI Client.h"

// Display packets as hex
void DisplayPacket( PACKET_INFO* lpPI, PVOID lpData, HWND hwndDlg, int nIDDlgItem ) {
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

  TCHAR* szBuffer = ( TCHAR* )malloc( ( 3 * nSize + 1 ) * sizeof( TCHAR ) );
  RtlZeroMemory( szBuffer, ( 3 * nSize + 1 ) * sizeof( TCHAR ) );

  for( unsigned int i = 0; i < nSize; i++ ) {
    _stprintf_s( &szBuffer[i*3], 3, _T("%02X"), ( ( const byte* )lpData )[i] );
    szBuffer[ ( i + 1 )*3 - 1 ] = *_T(" ");
  }

  ListView_SetItemText( hwndList, lvi.iItem, 3, szBuffer );
  free( szBuffer );
}

// DialogProc for plain tab
INT_PTR CALLBACK PlainDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
  switch( uMsg ) {
    case WM_INITDIALOG: {
      EnableThemeDialogTexture( hwndDlg, ETDT_ENABLETAB );
      DWORD dwStyle = SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
      SendMessage( GetDlgItem( hwndDlg, IDC_PLAINLIST ), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle | LVS_EX_FULLROWSELECT );
      InitPacketList( hwndDlg, IDC_PLAINLIST );

      int nIconWidth  = GetSystemMetrics( SM_CXSMICON );
      int nIconHeight = GetSystemMetrics( SM_CYSMICON );

      // Set ImageList
      HINSTANCE  hInstance = GetModuleHandle( NULL );
      HIMAGELIST hIML      = ImageList_Create( nIconWidth, nIconHeight, ILC_COLOR, 2, 0 );
      HBITMAP    hbmArrows = ( HBITMAP )LoadImage( hInstance, MAKEINTRESOURCE( IDB_ARROWS ), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT );
      
      ImageList_Add( hIML, hbmArrows, NULL );
      DeleteObject( hbmArrows );

      SendMessage( ( HWND )lParam, WM_IMAGELISTREADY, ( WPARAM )hIML, NULL );
      break;
    }
    case WM_NEWPACKET: {
      DisplayPacket( ( PACKET_INFO* )wParam, ( PVOID )lParam, hwndDlg, IDC_PLAINLIST );
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
      return false;
  }

  return true;
}