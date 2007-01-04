#include "CommCtrl.h"
#include <iostream>
using namespace std;

static HANDLE  hCOM1= (HANDLE) -1;
static HANDLE  hCOM2= (HANDLE) -1;
static HANDLE  *hComDevice;

/* ****************************************************************
* Calculates a data buffer CRC WORD
* Input : ptbuf = pointer to the first byte of the buffer
* num = number of bytes
* Output : //
* Return : CRC 16
*****************************************************************/
unsigned int calc_crc (char *ptbuf, unsigned int num) {
    unsigned int crc16;
    unsigned int temp;
    unsigned char c, flag;
    crc16 = 0xffff; /* init the CRC WORD */
    for (num; num>0; num--) {
        temp = (unsigned int) *ptbuf; /* temp has the first byte */
        temp &= 0x00ff; /* mask the MSB */
        crc16 = crc16 ^ temp; /* crc16 XOR with temp */
        for (c=0; c<8; c++) {
            flag = crc16 & 0x01; /* LSBit di crc16 is mantained */
            crc16 = crc16 >> 1; /* LSBit di crc16 is lost */
            if (flag != 0)
            crc16 = crc16 ^ 0x0a001; /* crc16 XOR with 0x0a001 */
            }
        ptbuf++; /* pointer to the next byte */
        }
    crc16 = (crc16 >> 8) | (crc16 << 8); /* LSB is exchanged with MSB */
    return (crc16);
    } /* calc_crc */

//---------------------------------------------------------------------------
void mycalc_crc(char* str, int num, unsigned char &g, unsigned char &h) {

    unsigned int crc = calc_crc(str,num);
    g = ((crc & 0xff00) >> 8);
    h = (crc & 0x00ff);
    //cout << "CRC: " <<  (int)g << " " << (int)h << endl;

    }

//---------------------------------------------------------------------------
void SendMsg(unsigned char d1, unsigned char d2, unsigned char nb_words) {

   //cout << "Polling..." << endl;
   short instr = 1;
   short func  = 0x03; // reading words
   char str[100];
   unsigned char a = 0x01; // instr
   unsigned char b = 0x03; // read words
   unsigned char c = d1;   // data address
   unsigned char d = d2;   // data address
   unsigned char e = 0x00; // nb of words
   unsigned char f = nb_words;   // nb of words
   unsigned char g;
   unsigned char h;
   sprintf(str,"%c%c%c%c%c%c",a,b,c,d,e,f);
   mycalc_crc(str,6,g,h);
   sprintf(str,"%c%c%c%c%c%c%c%c",a,b,c,d,e,f,g,h);
   int rv = SetOutput(str, 8, COM1);
   if(!rv) {
      cout << "Error setting output" << endl;
      exit(1);
      }
   }

//---------------------------------------------------------------------------
unsigned long get_value(char* buf, int nb) {
   unsigned long value = 0;
   unsigned long tmp;
   for(int i=0;i<nb;++i) {
      tmp = buf[3+i];
      tmp = tmp << (8*(nb-(i+1)));
      value += tmp;
      }
   return value;
   }

//---------------------------------------------------------------------------
unsigned long ReadPower() {
   SendMsg(0x03,0x19,0x02);
   char buf[50];
   ReadRS(buf, COM1);
   return (unsigned long)(get_value(buf,4)/100000.+0.5);
   }


unsigned long ReadFreq() {
   SendMsg(0x10,0x26,0x01);
   char buf[50];
   ReadRS(buf, COM1);
   cout << get_value(buf,2) << " Hz ";
   }

unsigned long ReadAmp() {
   SendMsg(0x03,0x0d,0x02);
   char buf[50];
   ReadRS(buf, COM1);
   cout << get_value(buf,4)/1000 << " A ";
   }

unsigned long ReadActivePower() {
   SendMsg(0x10,0x14,0x02);
   char buf[50];
   ReadRS(buf, COM1);
   cout << get_value(buf,4) << " ??? ";
   }

unsigned long ReadReactivePower() {
   SendMsg(0x03,0x1d,0x02);
   char buf[50];
   ReadRS(buf, COM1);
   cout << get_value(buf,4)/100000 << " KVAr ";
   }

unsigned long ReadVoltage() {
   SendMsg(0x03,0x29,0x02);
   char buf[50];
   ReadRS(buf, COM1);
   cout << get_value(buf,4)/1000 << " V ";
   }

//---------------------------------------------------------------------------
void loop () {
    char buf[50];
    for(int i=1; i <=53; i += 4) {
        SendMsg(0x03,i,0x02);
        ReadRS(buf, COM1);
        cout << i << ": " << get_value(buf,4) << " / ";
        }
    }

//---------------------------------------------------------------------------
bool SetPortOpen(int portx, DWORD dwBaudrate) {
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

   dcb.ByteSize = 8; 									// set 8 Bit
   dcb.Parity   = 0; 									// no parity
   dcb.fParity  = 0; 									// no parity check enable
   dcb.StopBits = ONESTOPBIT;							// 1 Stop Bit
   dcb.fBinary  = TRUE;									// binary

   bool rv = SetCommState(*hComDevice, &dcb );
   if (!rv) return false;

   return true;// set COM with dcb
   } // end of SetPortOpen()


//---------------------------------------------------------------------------
bool SetOutput(char* strTx , int size, int portx) {
   DWORD       dwBytesWritten ;
   if(portx == 0) hComDevice = &hCOM1; else  hComDevice = &hCOM2;
   if( *hComDevice == (HANDLE) -1) return(-1);
   return WriteFile(*hComDevice, strTx, size,&dwBytesWritten, NULL) ;
   }


//---------------------------------------------------------------------------
bool ReadRS(char* response, int portx) {
   //memset(response,'\0',nb_bytes);
   DWORD       dwBytesRead = 0;
   if(portx == 0) hComDevice = &hCOM1; else  hComDevice = &hCOM2;
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

//---------------------------------------------------------------------------
void CloseH() {
    CloseHandle(*hComDevice);
    }

