#pragma hdrstop

#include "CentraleController.h"
#include "HTTPServer.h"
#include "RS232_01\\CommCtrl.h"

#pragma package(smart_init)
using namespace std;

extern void WriteToLog(int, const string& str);
extern string ProgVersion;

//---------------------------------------------------------------------------
CController::CController() : Power(0) {

   HTTP = new MHTTPServer();
   HTTP->Port = 8080;
   HTTP->SetCallback(HTTPInfo);
   if(!HTTP->Listen())
      WriteToLog(1,string("Can't listen on port ")+MUtils::toStr(HTTP->Port));
   if(!SetPortOpen(COM1,9600)) {
      WriteToLog(1,"Error opening port");
      return;
      }
   }
   
//---------------------------------------------------------------------------
CController::~CController() {
   delete HTTP;
   CloseH();
   }
//---------------------------------------------------------------------------
void CController::HTTPInfo(string& str) {

   ostringstream s;
   s << "<b>" << ProgVersion << "</b><br/><br/>";
   s << "<b>Date:</b> " << MUtils::MyNow(1) << "<br/>";
   s << "<b>Puissance:</b> " << Power << " KW<br/>";
   s << "<b>Active Energy:</b> " << PositiveActiveEnergy << "<br/>";
   str = s.str();

   }
   
//---------------------------------------------------------------------------
void CController::Read() {

   //Power = 23 + rand()% 3;
   Power = ReadPower();
   PositiveActiveEnergy = ReadPositiveActiveEnergy();
   MItem i;
   i.tick = GetTickCount();
   i.power = Power;
   Stats.Add(i);

   }

//---------------------------------------------------------------------------
int CController::GetPort() {

   return HTTP->Port;

   }
   
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void MStatList::Add(const MItem& i) {

   Items.push_back(i);

   }

