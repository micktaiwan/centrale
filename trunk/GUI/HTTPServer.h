#ifndef HTTPServerH
#define HTTPServerH

#include "MPNLBase.h"

//---------------------------------------------------------------------------
class MAntServer;

class MHTTPServer : public MPNL::MTCPServer {

   public:
#ifdef WIN32
      typedef void (__closure *MC)(std::string&);
#else
      
      typedef void (MAntServer::*MC)(std::string&);
#endif
      MHTTPServer() {}
      virtual ~MHTTPServer() {}

      // thread function accepting connections and sending infos
      virtual void operator()();
      void SetCallback(MC c) {GetInfo = c;}

   private:
      MC GetInfo;

   };

#endif
