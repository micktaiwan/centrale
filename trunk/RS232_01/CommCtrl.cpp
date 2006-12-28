
#include "CommCtrl.h"
#include <iostream>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
HANDLE  hCOM1= (HANDLE) -1;
HANDLE  hCOM2= (HANDLE) -1;
HANDLE  *hComDevice;

bool SetPortOpen(int portx, DWORD dwBaudrate)
{
   char       szPort[ 15 ] ;
   DCB        dcb ;

    if(portx == 0) hComDevice = &hCOM1; else  hComDevice = &hCOM2;
	if( *hComDevice != (HANDLE) -1) return true;			// port is still open

    wsprintf( szPort, "%s%d", "COM", portx+1 ) ;

    if ((*hComDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,	// open COMM device
                  0,														// exclusive access
                  NULL,														// no security attrs
                  OPEN_EXISTING,
                  0,
                  NULL                                                      // no overlapped I/O
                   )) == (HANDLE) -1 )  return ( FALSE ) ;

   SetupComm(*hComDevice , 4096, 4096 ) ;						// setup device buffers
   PurgeComm(*hComDevice, PURGE_TXABORT | PURGE_RXABORT |PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
																// purge any information in the buffer
   dcb.DCBlength = sizeof( DCB );
   GetCommState(*hComDevice, &dcb ) ;					// read com properties to dcb

   dcb.BaudRate = dwBaudrate;							// set baudrate

   dcb.ByteSize = 8;									// set 8 Bit
   dcb.Parity   = 0;									// no parity
   dcb.fParity  = 0;									// no parity check enable
   dcb.StopBits = ONESTOPBIT;							// 1 Stop Bit
   dcb.fBinary  = TRUE;									// binary

   bool rv = SetCommState(*hComDevice, &dcb );
   if (!rv) return false;

   return true;			// set COM with dcb
   } // end of SetPortOpen()


BOOL SetOutput(char* strTx , int size, int portx) {
	BOOL        fWriteStat ;
	DWORD       dwBytesWritten ;
	if(portx == 0) hComDevice = &hCOM1; else  hComDevice = &hCOM2;
	if( *hComDevice == (HANDLE) -1) return(-1);
	fWriteStat = WriteFile(*hComDevice, strTx, size,&dwBytesWritten, NULL) ;
	return(fWriteStat);
    }


BOOL ReadRS(char* response, int portx) {
	//memset(response,'\0',nb_bytes);
	DWORD       dwBytesRead = 0;
	if(portx == 0) hComDevice = &hCOM1;	else  hComDevice = &hCOM2;
	if( *hComDevice == (HANDLE) -1) return(-1);
	int i = 0;
	ReadFile(*hComDevice, response+(i++), 1, &dwBytesRead, NULL); // instr
	ReadFile(*hComDevice, response+(i++), 1, &dwBytesRead, NULL); // func
	ReadFile(*hComDevice, response+(i++), 1, &dwBytesRead, NULL); // nb bytes data
	int nb = response[2]+2;
	//cout << nb-2 << endl;
	while(nb--)
        ReadFile(*hComDevice, response+(i++), 1, &dwBytesRead, NULL);
    return true;
    }

void CloseH() {

    CloseHandle(*hComDevice);

    }
