#ifndef CentraleControllerH
#define CentraleControllerH

#include <string>
#include <vector>

//---------------------------------------------------------------------------
class MHTTPServer;

//---------------------------------------------------------------------------
struct MItem {
   int tick;
   int power;
   };

typedef std::vector<MItem> MItemList;
//---------------------------------------------------------------------------
class MStatList {
public:
   void Add(const MItem& i);
   const MItemList* GetItems() {return &Items;} // return them but can not be modified

private:
   MItemList Items;
   };

//---------------------------------------------------------------------------
class CController {
public:
   unsigned long Power;
   unsigned long PositiveActiveEnergy;
   MHTTPServer* HTTP;
   MStatList Stats;

   CController();
   ~CController();

   void HTTPInfo(std::string& str);
   void Read();
   int GetPort();
private:

};

#endif

