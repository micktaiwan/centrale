#include <vcl.h>
#pragma hdrstop

#include <fstream>
#include "main.h"
#include "CentraleController.h"
//#include "MUtils.h"
using namespace std;

#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

string ProgVersion = "Centrale Controller V1.0";

//---------------------------------------------------------------------------
void WriteToLog(int, const string& str) {

   //Form1->Memo1->Lines->Add(str.c_str());
   ofstream file("./gui.log", ios_base::app);
   file << /*MUtils::MyNow(1) <<*/ ">" << str << endl;

   }

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
   : TForm(Owner) { }
   
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender) {

   Caption = ProgVersion.c_str();
   Cont = new CController();
   LabelPort->Caption = AnsiString("Port du web: ")+Cont->GetPort();

   }

//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender) {

   delete Cont;

   }

//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender) {

   // Read the value, fill the list
   Cont->Read();
   Label1->Caption = AnsiString(Cont->Power)+ " KW";

   // Graph
   typedef std::vector<MItem> MItemList;
   static const MItemList* si = Cont->Stats.GetItems();
   
   TCanvas* c = PB->Canvas;
   int w = PB->ClientWidth;
   int h = PB->ClientHeight;
   c->Brush->Color = clWhite;
   c->FillRect(Rect(0,0,w,h));
   c->Pen->Color = clBlack;
   const int size = si->size();
   const int max = 27;//Cont->Stats.Max();
   
   c->MoveTo(0,h);
   for(int i=0; i < w && i < size-1; ++i) {
      c->LineTo(i,h-((h*(*si)[i].power)/max));
      }

   }

//---------------------------------------------------------------------------

