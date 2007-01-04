#include <iostream>
#include <cstdio>
#include "RS232_01/CommCtrl.h"
//#include "server/server.h"
using namespace std;

//void LoadWSDLL() {    }

int main()
{
    //LoadWSDLL();
    bool rv = SetPortOpen(COM1,9600);
    if(!rv) {
        cout << "Error opening port" << endl;
        return 1;
        }
    //Server();
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
