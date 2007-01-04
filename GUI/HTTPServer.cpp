#ifdef WIN32
#pragma hdrstop
#pragma package(smart_init)
#endif

#include "HTTPServer.h"

using namespace std;

extern void WriteToLog(int, const string&);

//---------------------------------------------------------------------------
void MHTTPServer::operator()() {
// main server thread (accepting connections and launching new client threads)

   MYSOCKET socket;
   sockaddr_in sa;
#ifdef WIN32
   int salen;
#else
   socklen_t salen;
#endif
   struct timeval tv;
   tv.tv_sec = 1;
   tv.tv_usec = 0;

   fd_set readfds;
   int rv;
   WriteToLog(2,string("HTTP Server running on port ")+MUtils::toStr(Port));
   while(IsActive()) {

      SLEEP(100);
      
      // Accept new connections
      FD_ZERO(&readfds);
      FD_SET(Socket->GetS(), &readfds);
      rv = select(Socket->GetS()+1, &readfds, NULL, NULL, &tv);
      if (!FD_ISSET(Socket->GetS(), &readfds)) continue; // read timeout, do nothing
      if(rv < 0) {
         WriteToLog(1,string("Server::Select error: "));//+strerror(errno));
         break;
         }
      salen = sizeof(sa);
      if((socket = accept(Socket->GetS(),reinterpret_cast<sockaddr*>(&sa), &salen)) != MYINVALID_SOCKET) {
         // New connection, send infos
         WriteToLog(2,string("HTTP connection ")+inet_ntoa(sa.sin_addr));
         string str;
#ifdef WIN32 // stuck under linux... arrrg
         GetInfo(str);
#endif
         ostringstream s;
         s << "HTTP/1.1 200 OK\r\n";
         s << "Server: Centrale HTTP Server\r\n";
         s << "Date: " << MUtils::MyNow(1) << "\r\n";
         s << "Content-Type: text/html\r\n";
         s << "Content-Length: " << str.size() << "\r\n\r\n";
         s << str;
         send(socket,s.str().c_str(),s.str().size(),0);
         SLEEP(1000); // because without the browser says the connection was reset.
         // and 1000 because I don't know if we just need 100 to pass it to the
         // TCP stack or 1000 to pass it to the client
         MYCLOSE(socket);
         }
      else {
         WriteToLog(1,"HTTP Server Accept error, exiting");
         break;
         }
      } // while Active
   WriteToLog(2,"HTTP Server shutting down");

   }
