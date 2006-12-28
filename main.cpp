#include <iostream>
#include <cstdio>
#include "RS232_01/CommCtrl.h"

using namespace std;

unsigned int calc_crc (char *ptbuf, unsigned int num)
    /* ****************************************************************
    * Descrizione : calculates a data buffer CRC WORD
    * Input : ptbuf = pointer to the first byte of the buffer
    * num = number of bytes
    * Output : //
    * Return : CRC 16
    *****************************************************************/
    {
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

void mycalc_crc(char* str, int num, unsigned char &g, unsigned char &h) {

    unsigned int crc = calc_crc(str,num);
    g = ((crc & 0xff00) >> 8);
    h = (crc & 0x00ff);
    //cout << "CRC: " <<  (int)g << " " << (int)h << endl;

    }

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


unsigned long ReadPower() {

    SendMsg(0x03,0x19,0x02);
    char buf[50];
    ReadRS(buf, COM1);
	cout << (int)(get_value(buf,4)/100000.+0.5) << " KW ";
    /*
    unsigned char g;
    unsigned char h;
    mycalc_crc(buf,7,g,h);
    cout << "CRC: " <<  (int)g << " " << (int)h << endl;
    */

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



void loop () {

    char buf[50];
    for(int i=1; i <=53; i += 4) {
        SendMsg(0x03,i,0x02);
        ReadRS(buf, COM1);
        cout << i << ": " << get_value(buf,4) << " / ";
        }
    }

int main()
{
    bool rv = SetPortOpen(COM1,9600);
    if(!rv) {
        cout << "Error opening port" << endl;
        return 1;
        }
    while(1) {
        ReadPower();
        ReadVoltage();
        //ReadFreq();
        ReadAmp();
        //ReadActivePower();
        ReadReactivePower();
        //loop();
        cout << endl;
        Sleep(1000);
        }
    CloseH();
	cout << "Exiting" << endl;
	return 0;
}
