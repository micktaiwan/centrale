#if !defined(AFX_COMMCTRL_H__4222DFCD_FE9D_11D5_BA38_0050DAE90380__INCLUDED_)
#define AFX_COMMCTRL_H__4222DFCD_FE9D_11D5_BA38_0050DAE90380__INCLUDED_

#define USECOMM      // yes, we need the COMM API
#include <windows.h>
#include <commdlg.h>
#include <string>

#define COM1 0
#define COM2 1

bool SetPortOpen(int,DWORD);
BOOL SetOutput(char* strTx , int size ,int);
BOOL ReadRS(char*, int);
void CloseH();

#endif

