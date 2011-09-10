#include "MPI Client.h"

void InitPacketList( HWND hwndDlg, int nIDDlgItem ) {
  HWND         hwndList = GetDlgItem( hwndDlg, nIDDlgItem );
  LVCOLUMN     lvc      = {0};
  unsigned int iCol;

  lvc.cx   = 20;
  lvc.fmt  = LVCFMT_LEFT;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  for( iCol = 0; iCol < 4; iCol++ ) {
    lvc.iSubItem = iCol;

    switch( iCol ) {
      case 0:
        lvc.pszText = _T("");
        break;
      case 1:
        lvc.cx      = 50;
        lvc.pszText = _T("ID");
        break;
      case 2:
        lvc.pszText = _T("Size");
        break;
      case 3:
        lvc.pszText = _T("Data");
        break;
    }

    ListView_InsertColumn( hwndList, iCol, &lvc );
  }

  ListView_SetColumnWidth( hwndList, 3, LVSCW_AUTOSIZE_USEHEADER );
}