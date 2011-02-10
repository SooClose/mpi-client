#include "MPI Client.h"

int WINAPI _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, 
    LPTSTR lpCmdLine, int nShowCmd ) {
  INITCOMMONCONTROLSEX iccex = {0};
  iccex.dwSize               = sizeof iccex;
  iccex.dwICC                = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES |
                                   ICC_LISTVIEW_CLASSES;

  if( !InitCommonControlsEx( &iccex ) ) {
    MessageBox( NULL, _T("Problem registering classes from comctl32.dll"),
        NULL, MB_OK | MB_ICONEXCLAMATION );
  }

  return ERROR_SUCCESS;
}