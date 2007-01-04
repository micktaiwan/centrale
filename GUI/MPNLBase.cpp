/***************************************************************************
                          mpnlbase.cpp  -  description
                             -------------------
    begin                : Sat Feb 7 2004
    copyright            : (C) 2004 by Mickael Faivre-Macon
    email                : mickael@easyplay.com.tw
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef WIN32
#pragma hdrstop
#endif

#include "MPNLBase.h"

#ifdef WIN32
   #ifdef __BORLANDC__
      #include <SysUtils.hpp> // for catching WIN32 exceptions (SEH)
   #endif
#else
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <signal.h>
#endif

#include <iostream>
#include <fstream>

#undef DeleteFile

using namespace MPNL;
using namespace std;

//boost::mutex debugmutex;

#define DEBUG(x)
extern void WriteToLog(int, const string&);
/*
#define DEBUG(x) {\
   WriteToLog(103,x); \
   }
*/
/*
#define DEBUG(x) {\
   boost::mutex::scoped_lock lock(debugmutex); \
   cout << x << endl; \
   }
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static const int HOSTNAMELENGTH = 40;

//---------------------------------------------------------------------------
string MPNL::GetHostName(int socket) {

   struct sockaddr_in sin;
   struct hostent *host;
#ifdef WIN32
   int len;
#else
   socklen_t len;
#endif
   len = sizeof(sin);
   if (getpeername(socket, (struct sockaddr *) &sin, &len) < 0) return "";
   if ((host = gethostbyaddr((char *) &sin.sin_addr, sizeof sin.sin_addr, AF_INET)) == NULL) return "";
   return host->h_name;

   }

//---------------------------------------------------------------------------
string MPNL::GetLocalIP() {

#ifdef WIN32
   WORD v;
   WSADATA wsaData;
   v = MAKEWORD(2,0);
   int err = WSAStartup( v, &wsaData );
   if (err != 0) return "";
   if (LOBYTE( wsaData.wVersion ) != 2 || HIBYTE(wsaData.wVersion) != 0 ) {
      WSACleanup();
      return "";
      }
#endif
   char name[HOSTNAMELENGTH];
   gethostname(name,HOSTNAMELENGTH);
   hostent* h = gethostbyname(name);
   if(h==NULL) {
#ifdef WIN32
      WSACleanup();
#endif
      return "";
      }
   in_addr ia;
   memcpy(&ia, h->h_addr_list[0], 4);
   string rv = inet_ntoa(ia);
#ifdef WIN32
   WSACleanup();
#endif
   return rv;

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MSocket::MSocket(int type, MPNLBase* owner)
   :  MThread(), Data(NULL), Mode(MODE_LENPREFIXED), PrefixLen(4), PeerPort(0),
      Stream(NULL), S(MYINVALID_SOCKET), /*Active(true),*/
      Type(type), Owner(owner) {

   GETTIME(LastMsgTime);

   // SIGACTION
/*
   struct sigaction sa;
   sa.sa_handler = SIG_IGN;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   if(sigaction(SIGPIPE, &sa, NULL) == -1) {
      LastError = "sigaction failed";
      throw error();
      }
*/

   // create it
   S = socket(AF_INET, type, 0/*PF_INET*/);
/*
#define	PF_UNSPEC	0		// unspecified
#define	PF_UNIX		1		// UNIX internal protocol
#define	PF_INET		2		// internetwork: UDP, TCP, etc.
#define	PF_IMPLINK	3		// imp link protocols
#define	PF_PUP		4		// pup protocols: e.g. BSP
#define	PF_CHAOS	5		// mit CHAOS protocols
#define	PF_OISCP	6		// ois communication protocols
#define	PF_NBS		7		// nbs protocols
#define	PF_ECMA		8		// european computer manufacturers
#define	PF_DATAKIT	9		// datakit protocols
#define	PF_CCITT	10		// CCITT protocols, X.25 etc
*/

   }

//---------------------------------------------------------------------------
MSocket::~MSocket(){

   Stop();
   SetActive(false);
   MYCLOSE(S);
   
   }

//---------------------------------------------------------------------------
bool MSocket::IsInvalid() {

   //boost::mutex::scoped_lock lock(Mutex);
   return (S == MYINVALID_SOCKET);

   }

//---------------------------------------------------------------------------
bool MSocket::Listen(int port) {

   DEBUG("MSocket::Listen");
   struct sockaddr_in serv_addr;
   serv_addr.sin_family       = AF_INET;
   serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
   serv_addr.sin_port         = htons(port);

   if(bind(S,reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
      DEBUG(string("Bind error"));
      Owner->SetLastError("Bind error");
      return false;
      }

   if(Type == SOCK_STREAM) {
      if(listen(S, 5)) {
         DEBUG(string("Listen error"));
         Owner->SetLastError("ListenError");
         return false;
         }
      }
   SetActive(true); // must set it before we return
   //Start();


   return true;

   }

//---------------------------------------------------------------------------
void MSocket::Stop() {

   DEBUG("MSocket::Stop Enter");
   MThread::Stop();
   //MYCLOSE(S);
   DEBUG("MSocket::Stop Exit");

   }

//---------------------------------------------------------------------------
void MSocket::operator()() {
// read msgs

   DEBUG("MSocket::()");
   static const int MAXLEN = 8196;
   static char buf[MAXLEN];
   memset(buf,0,MAXLEN);

   int rv;
   unsigned long reallen,msglen=0;
   string msg;
   struct timeval tv;
   tv.tv_sec = 1;
   tv.tv_usec = 0;
   fd_set readfds;

   struct sockaddr_in from;
#ifdef WIN32   
   int fromlen = sizeof(from);
#else
   socklen_t fromlen = sizeof(from);
#endif   

   // For stream mode :
   bool first_msg = true;
   unsigned long total_stream_len=0, total_stream_len_received=0;

   while(IsActive()) {
      //DEBUG("MSocket::() read loop");
      FD_ZERO(&readfds);
      FD_SET(S, &readfds);
      rv = select(S+1, &readfds, NULL, NULL, &tv);

      SLEEP(10);

      if (!FD_ISSET(S, &readfds)) continue; // read timeout, do nothing
      if(rv <= 0) {
         DEBUG("select error");
         break;
         }

      if(Type==SOCK_STREAM)
         rv = recv(S, buf, MAXLEN, 0); // could use MSG_NOSIGNAL
      else
         rv = recvfrom(S, buf, MAXLEN, 0, reinterpret_cast<sockaddr*>(&from), &fromlen);
      if(rv == 0) {
         DEBUG("gracefully disconnected");
         break;
         }
      if(rv == MYSOCKET_ERROR) {
#ifdef WIN32
         int err = WSAGetLastError();
         DEBUG(string("recv error: ")+MUtils::int2string(err));
#else
         DEBUG(string("recv error: ")+MUtils::int2string(errno));
#endif
         break;
         }
      //DEBUG(string("MPNL::MSocket:Read something"));
      GETTIME(LastMsgTime);
      if(!Stream) { // if it is saved to a stream, we don't need to waste memory
         msg += string(buf,rv);
         msglen += rv;
         }
      else msglen = rv; // to enter the loop
      if(Mode==MODE_LENPREFIXED && msglen < (unsigned)PrefixLen) {
         DEBUG("Read : msglen==0 && rv < PrefixLen");
         goto checkdiscon;
         }
      if(msglen > 100*1024)
         DEBUG(string("msglen=")+MUtils::int2string(msglen));
      // we need another loop to take care of extra messages in the buffer
      while(msglen) {
         if(Mode==MODE_LENPREFIXED) {
            reallen = 0;
            if(!Stream) {
               for(int i=0; i < PrefixLen; i++) {
                  reallen += ((unsigned char)msg[i]) << (8*((PrefixLen-1)-i));
                  }
               }
            else if(first_msg) {
               for(int i=0; i < PrefixLen; i++) {
                  reallen += ((unsigned char)buf[i]) << (8*((PrefixLen-1)-i));
                  }
               }
            if(Stream) {
               if(first_msg) {
                  // First msg, we should have the len prefixed
                  total_stream_len = reallen;
                  for(int i=0; i < PrefixLen; i++) {
                     DEBUG(MUtils::int2string((unsigned char)buf[i]));
                     }
                  total_stream_len_received = rv-PrefixLen;
                  if(total_stream_len_received >= total_stream_len) {
                     Stream->write(buf+PrefixLen,total_stream_len);
                     // we have received 2 streams
                     DEBUG("2 streams received on first msg");
                     goto exit;
                     }
                  Stream->write(buf+PrefixLen,total_stream_len_received);
                  first_msg = false;
                  }
               else {

                  // not the first msg, we do not have the len prefixed
                  total_stream_len_received += rv;
                  if(total_stream_len_received >= total_stream_len) {
                     Stream->write(buf,rv-(total_stream_len_received - total_stream_len));
                     if(total_stream_len_received > total_stream_len) {
                        // we have received 2 streams
                        DEBUG("2 streams received");
                        }
                     goto exit;
                     }
                  else Stream->write(buf,rv);
                  }
               break;
               }
            else if(reallen > (msglen-PrefixLen)) break;
            if(reallen!=0) { // we have a msg and no stream
               string tmp = string(msg.c_str()+PrefixLen,reallen);
               //DEBUG(string("MSocket::() pushing msg ")+tmp);
               PushMsg(tmp);
               }
            //else WriteToLog(100,string("reallen: 0, msglen: ")+MUtils::int2string(msglen));
            msg = string(msg.c_str()+reallen+PrefixLen,msg.length()-(reallen+PrefixLen));
            msglen -= (reallen+PrefixLen);
            }
         else if(Mode==MODE_LINE) {
            int p = msg.find("\r\n");
            if(p==-1) goto checkdiscon;
            string tmp = msg.substr(0,p);
            //DEBUG(string("MSocket::() pushing msg ")+tmp);
            PushMsg(tmp);
            msg = string(msg.c_str()+p+2,msg.length()-(p+2));
            msglen -= (p+2);
            }
         else if(Mode==MODE_PACKET) {
            //DEBUG(string("MSocket::() pushing msg ")+msg);
            PushMsg(string(inet_ntoa(from.sin_addr)),msg);
            msg = "";
            goto checkdiscon;
            }
         else {
            }
         }
checkdiscon:
      ;
      // Check if not disconnected
      // never goes here if we do not pass the select, so useless
/*
      if(Type!=SOCK_DGRAM) {
         TIME b;
         GETTIME(b);
         if(MUtils::TimeInterval(LastMsgTime,b) >= 1000)
            if(Send("") < 0) break;
         }
*/
      } // while Active
exit:
   SetActive(false);
   Owner->OnDisconnection(this);
   DEBUG("MSocket::() exit");

   }

//---------------------------------------------------------------------------
void MSocket::PushMsg(const string& msg) {

   boost::mutex::scoped_lock lock(MsgMutex);
   Messages.push(msg);
   lock.unlock();

   Owner->PushMsg(this);

   }

//---------------------------------------------------------------------------
void MSocket::PushMsg(const string& ip, const string& msg) {

   boost::mutex::scoped_lock lock(MsgMutex);
   Messages.push(msg);
   IP.push(ip);
   lock.unlock();

   Owner->PushMsg(this);

   }
//---------------------------------------------------------------------------
bool MSocket::Read(string& msg) {

   boost::mutex::scoped_lock lock(MsgMutex);
   if(!Messages.size()) return false;
   msg = Messages.front();
   Messages.pop();
   return true;

   }

//---------------------------------------------------------------------------
bool MSocket::Read(string& ip, string& msg) {

   boost::mutex::scoped_lock lock(MsgMutex);
   if(!Messages.size()) return false;
   msg = Messages.front();
   Messages.pop();
   ip = IP.front();
   IP.pop();
   return true;

   }

//---------------------------------------------------------------------------
bool MSocket::HasMsg() {

   boost::mutex::scoped_lock lock(MsgMutex);
   return Messages.size();

   }

//---------------------------------------------------------------------------
//bool MSocket::IsActive() {

//   boost::mutex::scoped_lock lock(Mutex);
//   return Active;

//   }

//---------------------------------------------------------------------------
int MSocket::Send(const char* str, int len, int flags) {

   if(Type==SOCK_STREAM)
      return send(S,str,len,flags);
   else
      return sendto(S,str,len,flags,reinterpret_cast <sockaddr*>(&ServAddr),sizeof(ServAddr));

   }

//---------------------------------------------------------------------------
int MSocket::SendTo(const string& ip, int port, const string& msg) {

   ServAddr.sin_family = AF_INET;
   ServAddr.sin_addr.s_addr = inet_addr(ip.c_str());
   ServAddr.sin_port = htons(port);

   return sendto(S,msg.c_str(),msg.length(),0,reinterpret_cast <sockaddr*>(&ServAddr),sizeof(ServAddr));

   }

//---------------------------------------------------------------------------
int MSocket::Send(const string& msg) {

   if(!msg.empty()) DEBUG(string("sending (first 50 chars): ")+msg.substr(0,50));
   int rv;
   if(Mode == MODE_LENPREFIXED) {
      unsigned long len = msg.size();
      if(PrefixLen<4 && len > (unsigned long)(1 << (PrefixLen*8))-1) {
         DEBUG("msg len too big for PrefixLen");
         return -1;
         }

      string l(PrefixLen,'x');
/*
      l[0] = (len & 0xFF000000) >> 24;
      l[1] = (len & 0x00FF0000) >> 16;
      l[2] = (len & 0x0000FF00) >> 8;
      l[3] =  len & 0x000000FF;
*/
      unsigned long mask = 0x000000FF;
      for(int i=0; i < PrefixLen; i++) {
         l[i] = (len >> (8*((PrefixLen-1)-i))) & mask;
         }
      rv = Send(l.c_str(), PrefixLen, 0);
      if(rv<=0) return rv;
      rv = rv+Send(msg.c_str(), len, 0);
      }
   else if (Mode==MODE_LINE) {
      string tmp = msg+"\n";
      rv = Send(tmp.c_str(), tmp.size(), 0);
      }
   else {
      rv = Send(msg.c_str(), msg.size(), 0);
      }
   if(rv <= 0) {
      if(!msg.empty()) DEBUG("send failed");
      //SetActive(false);
      }
   else GETTIME(LastMsgTime);
   return rv;

   }

//---------------------------------------------------------------------------
unsigned long MSocket::SendStream() {

   if(!Stream) return 0;

   unsigned long total = 0, len;
   try {
      DEBUG(string("sending stream"));
      int rv;
      if(Mode == MODE_LENPREFIXED) {
         // get length of file:
         Stream->seekg (0, ios::end);
         len = Stream->tellg();
         Stream->seekg (0, ios::beg);
         DEBUG(string("total len: ")+MUtils::long2string(len));
         if(PrefixLen<4 && len > (unsigned long)(1 << (PrefixLen*8))-1) {
            DEBUG("msg len too big for PrefixLen");
            return 0;
            }

         char l[4];
   /*
         l[0] = (len & 0xFF000000) >> 24;
         l[1] = (len & 0x00FF0000) >> 16;
         l[2] = (len & 0x0000FF00) >> 8;
         l[3] =  len & 0x000000FF;
   */
         unsigned long mask = 0x000000FF;
         for(int i=0; i < PrefixLen; i++) {
            l[i] = (unsigned char)((len >> (8*((PrefixLen-1)-i))) & mask);
            DEBUG(MUtils::int2string((unsigned char)l[i]));
            }
         rv = Send(l, PrefixLen, 0);
         if(rv<=0) return 0;
         }
      char buf[8196];
      int r;
      while(Stream->good()) {
         r = Stream->readsome(buf,8196);
         //DEBUG(string("readsome: ")+MUtils::int2string(r));
         if(!r) break;
         int t = 0;
         while(t < r) {
            //DEBUG(string("remain: ")+MUtils::int2string(r-t));
            if((rv = Send(buf+t, r-t, 0)) < 0) {
               DEBUG("send stream failed");
               goto exit;
               }
            if(rv == 0) {
               DEBUG("Client disconnection");
               goto exit;
               }
            t += rv;
            //DEBUG(string("we sent: ")+MUtils::int2string(rv));
            }
         total += t;
         }
      }
   catch(exception& e) {
      DEBUG(string("MSocket::SendStream() : ")+e.what());
      }
#ifdef __BORLANDC__
   catch (Exception& e) {
      WriteToLog(1,string("MSocket::SendStream() : ")+e.Message.c_str());
      }
#endif
   catch(...) {
      DEBUG(string("MSocket::SendStream() : exception catched"));
      }
exit:
   return total;

   }

//---------------------------------------------------------------------------
string MSocket::GetLocalIP() {

   struct sockaddr_in name;
#ifdef WIN32
   int s;
#else
   socklen_t s;
#endif
   s = sizeof(name);
   getsockname(S,(sockaddr*)&name, &s);
   return inet_ntoa(name.sin_addr);

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MPNLBase::MPNLBase() : Port(0), Mode(MODE_LENPREFIXED), PrefixLen(4) {

#ifdef WIN32
   WSAData WSAData;
   if (WSAStartup(MAKEWORD(1,1), &WSAData) != 0) return;
#endif

   }

//---------------------------------------------------------------------------
MPNLBase::~MPNLBase() {

#ifdef WIN32
   WSACleanup();
#endif

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MTCPServer::MTCPServer() : MPNLBase(), /*Thread(NULL),*/ Stream(NULL) {

   Socket = new MSocket(SOCK_STREAM, this);
   ClientLock = new boost::mutex::scoped_lock(ClientMutex,false);

   }

//---------------------------------------------------------------------------
MTCPServer::~MTCPServer() {

   // The user must call Stop()
   // TCPServer->Stop(); // problem if not called by user: Ondisconnection is not virtualized
   // delete TCPServer;
   delete ClientLock;
   Stop();
   delete Socket;

   }

//---------------------------------------------------------------------------
void MTCPServer::SetMode(MMode m) {

   Mode = m;
   Socket->Mode = m;

   }

//---------------------------------------------------------------------------
void MTCPServer::SetPrefixLen(int len) {

   if(len>4) len =4;
   PrefixLen = len;
   Socket->PrefixLen = len;

   }

//---------------------------------------------------------------------------
void MTCPServer::SetStream(iostream* s) {

   Stream = s;

   }

//---------------------------------------------------------------------------
bool MTCPServer::Listen() {

   Stop();
   
   if(!Socket->Listen(Port)) return false;

   Start();
   DEBUG("MTCPServer::Listen (new thread)");

   return true;

   }
//---------------------------------------------------------------------------
void MTCPServer::Stop() {

   DEBUG("MTCPServer::Stop enter");

   Socket->Stop();
   MThread::Stop(); // stop listening

   // Normally the listening socket is deleting client on disconnection.
   // But here the listening socket is stopped before the clients so no new
   // clients are accepted while deleting clients,
   // so we must delete them now

   // stop all clients
   MClientList::iterator ite;
   for(ite=Clients.begin(); ite!=Clients.end();ite++) {
      ite->Socket->Stop(); // do the join
      delete ite->Socket;
      }
   //ClientThreads.join_all();
   // delete all clients
   //for(ite=Clients.begin(); ite!=Clients.end();ite++) {
      //delete ite->Socket;
      //ClientThreads.remove_thread(ite->Thread);
      //}
   Clients.clear();
   Messages.clear();

   DEBUG("MTCPServer::Stop exit");

   }

//---------------------------------------------------------------------------
bool MTCPServer::SocketHasMsg(MSocket* s) {

   boost::mutex::scoped_lock lock(MsgMutex);
   MMsgQueue::iterator ite;
   ite = Messages.begin();
   while(ite!=Messages.end()) {
      if((*ite)==s) return true;
      ++ite;
      }

   return s->HasMsg(); // double check

   }

//---------------------------------------------------------------------------
void MTCPServer::operator()() {
// main server thread (accepting connections and launching new client threads)

   DEBUG("MTCPServer::()");
   MYSOCKET socket;
   sockaddr_in sa;
#ifdef WIN32
   int salen;
#else
   socklen_t salen;
#endif

   struct timeval tv;
   fd_set readfds;
   tv.tv_sec = 1;
   tv.tv_usec = 0;

   int rv;

   while(IsActive()) {

      // Verify if disconnected
      MClientList::iterator ite, next;
      ite = Clients.begin();
      while(ite!=Clients.end()) {
         next = ite;
         ++next;
         if(!ite->Socket->IsActive()) {
            bool ready = false;
            if(ite->ToBeDeleted) {
               if(!SocketHasMsg(ite->Socket)) ready = true;
               else DEBUG(string("Server::Socket ")+MUtils::long2string(ite->ID)+" not yet ready to be deleted");
               }
            else {
               if(SocketHasMsg(ite->Socket)) {// we can not delete the socket now or we lose the messages in the queue
                  ite->ToBeDeleted = true;
                  DEBUG("Server::Socket has been queued to be deleted");
                  }
               else ready = true;
               }
            if(ready) {
               DEBUG("Server::Deleting socket");
               ite->Socket->Stop();
               //ite->Thread->join();
               //ClientThreads.remove_thread(ite->Thread);
               boost::mutex::scoped_lock lock(ClientMutex);

               Stats.CurCon--;
               Clients.erase(ite);
               delete ite->Socket;
               lock.unlock();
               DEBUG("Server::Socket deleted");
               }
            } // if Socket !Active
         ite = next;
         } // while
         
      SLEEP(10);

      // Accept new connections
      FD_ZERO(&readfds);
      FD_SET(Socket->GetS(), &readfds);
      rv = select(Socket->GetS()+1, &readfds, NULL, NULL, &tv);
      if (!FD_ISSET(Socket->GetS(), &readfds)) continue; // read timeout, do nothing
      if(rv < 0) {
         DEBUG(string("Server::Select error: ")+strerror(errno));
         break;
         }
      salen = sizeof(sa);
      if((socket = accept(Socket->GetS(),reinterpret_cast<sockaddr*>(&sa), &salen)) != MYINVALID_SOCKET) {
         DEBUG("MPNL::MTCPServer : New connection");
         MSocket* s = new MSocket(SOCK_STREAM, this);
         s->Mode = Mode;
         s->PrefixLen = PrefixLen;
         s->SetStream(Stream);
         s->SetS(socket);
         s->PeerIP = inet_ntoa(sa.sin_addr);
         s->PeerPort = sa.sin_port;
         if(!OnConnection(s)) {
            DEBUG("MPNL::MTCPServer : The connection is rejected");
            MYCLOSE(socket);
            delete s;
            }
         else {
            s->Start();
            boost::mutex::scoped_lock lock(ClientMutex);
            MClient c;
            c.Socket = s;
            c.ID = Stats.Total++; // shall we increase Total even if the connection was refused ?
            Clients.push_back(c);
            ++Stats.CurCon;
            if(Stats.MaxCon < Stats.CurCon) Stats.MaxCon = Stats.CurCon;
            }
         }
      else {
         DEBUG("Server::Accept error, exiting");
         break;
         }
      } // while Active

   }
//---------------------------------------------------------------------------
bool MTCPServer::OnConnection(MSocket*) {

   DEBUG("MPNL::MTCPServer::OnConnection");
   return true;

   }

//---------------------------------------------------------------------------
void MTCPServer::OnDisconnection(MSocket*) {

   DEBUG("MPNL::MTCPServer::OnDisconnection");
   
   }

//---------------------------------------------------------------------------
void MTCPServer::GetStats(MStats& s) {

   boost::mutex::scoped_lock lock(ClientMutex);
   s = Stats;

   }

//---------------------------------------------------------------------------
bool MTCPServer::Read(MSocket*& s) {

   boost::mutex::scoped_lock lock(MsgMutex);
   if(!Messages.size()) return false;
   s = Messages.front();
   Messages.pop_front();
   return true;

   }

//---------------------------------------------------------------------------
void MTCPServer::Disconnect(MSocket* s) {

   DEBUG("TCPServer::Disconnecting client");
   s->Stop();  // automatically deleted by the listening thread
/*
   MClientList::iterator ite;
   for(ite=Clients.begin(); ite!=Clients.end();ite++) {
      if(ite->Socket==s) {
         ite->Thread->join();
         // delete ite->Socket;
         //ClientThreads.remove_thread(ite->Thread);
         //Clients.erase(ite);
         DEBUG("ok");
         break;
         }
      }
*/

   }

//---------------------------------------------------------------------------
void MTCPServer::PushMsg(MSocket* s) {

   boost::mutex::scoped_lock lock(MsgMutex);
   Messages.push_back(s);

   }

//---------------------------------------------------------------------------
void MTCPServer::BroadcastMsg(MSocket* s, const string& msg) {

   MClientList::iterator ite = Clients.begin();
   while(ite != Clients.end()) {
      if(ite->Socket != s) ite->Socket->Send(msg);
      ++ite;
      }

   }

//---------------------------------------------------------------------------
int MTCPServer::Send(const string& ip, const string& msg) {

   MClientList::iterator ite = Clients.begin();
   while(ite != Clients.end()) {
      if(ite->Socket->PeerIP == ip)
         return ite->Socket->Send(msg);
      ++ite;

      }

   return 0;

   }

//---------------------------------------------------------------------------
bool MTCPServer::IsConnected(const string& ip) {

   for(MClientList::iterator ite=Clients.begin();ite!=Clients.end();ite++)
      if(ite->Socket->PeerIP == ip)
         return ite->Socket->IsActive();

   return false;

   }
   
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MTCPClient::MTCPClient() : MPNLBase() {

   //Thread = NULL;
   Socket = new MSocket(SOCK_STREAM, this);

   }

//---------------------------------------------------------------------------
MTCPClient::~MTCPClient() {

   Disconnect();
   delete Socket;

   }

//---------------------------------------------------------------------------
void MTCPClient::SetPrefixLen(int len) {

   if(len>4) len =4;
   Socket->PrefixLen = len;

   }

//---------------------------------------------------------------------------
void MTCPClient::SetStream(iostream* s) {

   Socket->Stream = s;

   }

//---------------------------------------------------------------------------
bool MTCPClient::Connect() {

   //Disconnect();
   // client decide whether or not disconnect
   // nov 2nd 2006
   // this was commented out
   // I don't remember why
   // I need to be able to connect again
   
   //static bool init = false;
   int rc;

   //if(!init) {
   /*
      struct   hostent*    H;
      H = gethostbyname(Host.c_str()); //IPv6: getaddrinfo();
      if(H==NULL) {

         DEBUG(string("Client::Connect: gethostbyname failed"));
         return false;
         }
   */

      // bind any local port number
/*
      LocalAddr.sin_family = AF_INET;
      LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      LocalAddr.sin_port = htons(0);

      rc = bind(Socket->GetS(), reinterpret_cast<struct sockaddr*>(&LocalAddr), sizeof(LocalAddr));
      if(rc<0) {
         int err = 0;
   #ifdef WIN32
         err = WSAGetLastError();
   #endif
         DEBUG(string("Client::Connect: bind failed: ")+MUtils::int2string(err));
         return false;
         }
*/

      // connect to server
      ServAddr.sin_family = AF_INET;
      ServAddr.sin_addr.s_addr = inet_addr(Host.c_str());
      ServAddr.sin_port = htons(Port);
      //init = true;
      //}

   rc = connect(Socket->GetS(), reinterpret_cast<struct sockaddr*>(&ServAddr), sizeof(ServAddr));
   if(rc<0) {
#ifdef WIN32
      int err = WSAGetLastError();
      DEBUG(string("connection failed: ")+MUtils::int2string(err));
#else
      DEBUG(string("connection failed: ")+MUtils::int2string(errno));
#endif
      return false;
      }

   Socket->PeerIP    = inet_ntoa(ServAddr.sin_addr);
   Socket->PeerPort  = ServAddr.sin_port;

   OnConnection(Socket);
   //Thread = new boost::thread(boost::ref(*Socket));
   Socket->Start();
   return true;

   }

//---------------------------------------------------------------------------
int MTCPClient::Send(const string& msg) {

   return Socket->Send(msg);

   }

//---------------------------------------------------------------------------
unsigned long  MTCPClient::SendStream() {

   return Socket->SendStream();

   }

//---------------------------------------------------------------------------
bool MTCPClient::Read(string& msg) {

   return Socket->Read(msg);

   }

//---------------------------------------------------------------------------
void MTCPClient::Disconnect() {

   Socket->Stop();
   //boost::mutex::scoped_lock lock(Mutex);
/*
   if(Thread) {
      Socket->Stop();
      Thread->join();
      delete Thread;
      Thread = NULL;
      }
*/

   }


//---------------------------------------------------------------------------
bool MTCPClient::OnConnection(MSocket*) {

   return true;

   }

//---------------------------------------------------------------------------
void MTCPClient::OnDisconnection(MSocket*) {
// inside socket thread, can not call join, or delete Thread

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MUDPServer::MUDPServer() : MPNLBase() {

   Socket = new MSocket(SOCK_DGRAM, this);
   SetMode(MODE_PACKET);

   }

//---------------------------------------------------------------------------
MUDPServer::~MUDPServer() {

   if(Socket) delete Socket;

   }

//---------------------------------------------------------------------------
void MUDPServer::SetMode(MMode m) {

   Mode = m;
   Socket->Mode = m;

   }

//---------------------------------------------------------------------------
void MUDPServer::SetPrefixLen(int len) {

   PrefixLen = len;

   }

//---------------------------------------------------------------------------
void MUDPServer::SetStream(iostream*) {


   }
   
//---------------------------------------------------------------------------
bool MUDPServer::OnConnection(MSocket*) {

   return true;

   }

//---------------------------------------------------------------------------
void MUDPServer::OnDisconnection(MSocket*) {

   }
   
//---------------------------------------------------------------------------
bool MUDPServer::Listen(int port) {

   if(!Socket->Listen(port)) return false;
   Socket->Start();
   DEBUG("MUDPServer::Listen (new thread)");
   return true;

   }
//---------------------------------------------------------------------------
bool MUDPServer::SocketHasMsg() {

   return Socket->HasMsg();

   }

//---------------------------------------------------------------------------
void MUDPServer::PushMsg(MSocket* s) {

   if(s!=Socket) {
      DEBUG("PushMsg: s != Socket");
      return;
      }
   //boost::mutex::scoped_lock lock(MsgMutex);
   //Messages.push_back(s);

   }
//---------------------------------------------------------------------------
bool MUDPServer::Read(MSocket*& s) {

   if(!Socket->HasMsg()) return false;
   s = Socket;
   return true;

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MUDPClient::MUDPClient() : MPNLBase() {

   //Thread = NULL;
   Socket = new MSocket(SOCK_DGRAM, this);
   SetMode(MODE_PACKET);
   //Thread = new boost::thread(boost::ref(*Socket));

   }


//---------------------------------------------------------------------------
MUDPClient::~MUDPClient() {

   //Stop();
   delete Socket;

   }

//---------------------------------------------------------------------------
void MUDPClient::SetPrefixLen(int len) {

   PrefixLen = len;

   }

//---------------------------------------------------------------------------
void MUDPClient::SetStream(iostream*) {

   

   }

//---------------------------------------------------------------------------
bool MUDPClient::OnConnection(MSocket*) {

   return true;

   }

//---------------------------------------------------------------------------
void MUDPClient::OnDisconnection(MSocket*) {

   }

//----------------------   -----------------------------------------------------
bool MUDPClient::PrepareToSend() {

   //struct   sockaddr_in LocalAddr;
   struct   hostent*    H;

   H = gethostbyname(Host.c_str()); /*IPv6: getaddrinfo();*/
   if(H==NULL) {
      DEBUG(string("UDPClient::PrepareToSend: gethostbyname failed"));
      return false;
      }

   // bind any local port number
/*
   LocalAddr.sin_family = AF_INET;
   LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   LocalAddr.sin_port = htons(0);

   int rc;
   rc = bind(Socket->GetS(), reinterpret_cast<struct sockaddr *>(&LocalAddr), sizeof(LocalAddr));
   if(rc<0) {
      int err = 0;
#ifdef WIN32
      err = WSAGetLastError();
#endif
      DEBUG(string("UDPClient::PrepareToSend: bind failed: ")+MUtils::int2string(err));
      return false;
      }
*/

   Socket->ServAddr.sin_family = H->h_addrtype;
   memcpy((char *) &Socket->ServAddr.sin_addr.s_addr, H->h_addr_list[0], H->h_length);
   Socket->ServAddr.sin_port = htons(Port);

   return true;

   }

//---------------------------------------------------------------------------
int MUDPClient::Send(const string& msg) {

   return Socket->Send(msg);

   }

//---------------------------------------------------------------------------
/*
bool MUDPClient::Read(string& msg) {

   return Socket->Read(msg);

   }
*/

//---------------------------------------------------------------------------
/*
void MUDPClient::Stop() {

   boost::mutex::scoped_lock lock(Mutex);
   if(Thread) {
      Socket->Stop();
      Thread->join();
      delete Thread;
      Thread = NULL;
      }

   }
*/

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
======================
First problem:
A downloading client connecting to a downloading server

Can be solved by attributing a different port to download and upload:
downloading client connects to port 6000
uploading server listen on the same port : 6000
uploading client connects to port 6001
downloading server listen on port 6001

======================
Second problem:
When using both client and server at the same time
there is a race condition: both clients can connect and stop their server, closing all the connections

Solved by solution to the 1st problem

======================
Second problem (bis):
When using a sender and a receiver at the same time
and using our own IP the second problem is not solved

Not solved

*/


//---------------------------------------------------------------------------
MTransfer::MTransfer() : MThread(), ID(0), Dead(false), Pending(false)  {

   Client = new MTransferTCPClient(this);
   Server = new MTransferTCPServer(this);
   UpdateLock = new boost::mutex::scoped_lock(UpdateMutex,false);

   }

//---------------------------------------------------------------------------
MTransfer::~MTransfer() {

   delete UpdateLock;

   Stop();

   delete Client;
   delete Server;

   }


//---------------------------------------------------------------------------
void MTransfer::operator()() {

   MUpdate* u;
   TIME a, b;
   MTCPServer::MStats stats;

   WriteToLog(3,string("Mode=")+Mode2String()+", Dir="+Dir2String());
   // open the file

   fstream file;
   string realpath = LocalPath;
   if(Direction == tdUPLOAD)

      file.open(LocalPath.c_str(), ios::in | ios::binary);
   else {
      string path, filename, ext, extra;
      int i = 0;
      while(MUtils::FileExists(realpath)) {
         ++i;
         path = MUtils::ExtractFilePath(LocalPath);
         filename = MUtils::ExtractFileNameWithoutExt(LocalPath);
         ext = MUtils::ExtractFileExt(LocalPath);
         extra = string("_")+MUtils::int2string(i);
         realpath = path+filename+extra+ext;
         }
      file.open(realpath.c_str(), ios::out | ios::binary);
      }

   if(!file) {
      u = new MUpdate(this);
      u->Type = MUpdate::utREMOVE;
      u->Result = MUpdate::urFAILED;
      u->Msg = string("Can not open file ")+LocalPath;
      AddUpdate(u);
      goto exit;
      }

   if(Direction == tdDOWNLOAD && LocalPath != realpath) {
      u = new MUpdate(this);
      u->Type = MUpdate::utMSG;
      u->Msg = string("Renaming to ")+MUtils::ExtractFileName(realpath);
      AddUpdate(u);
      }

   // settings
   Client->Host = PeerIP;
   // Ports
   if(Direction==tdDOWNLOAD) {
      Client->Port = Port;
      Server->Port = Port+1;
      }
   else {
      Client->Port = Port+1;
      Server->Port = Port;
      }
   Server->SetMode(MODE_LENPREFIXED);
   Server->SetPrefixLen(4);
   Client->SetMode(MODE_LENPREFIXED);
   Client->SetPrefixLen(4);
   Client->SetStream(&file);
   Server->SetStream(&file);

   // we listen and try to connect
   if((Mode==tmSERVER || Mode==tmBOTH) && !Server->Listen()) {
      u = new MUpdate(this);
      u->Type = MUpdate::utREMOVE;
      u->Result = MUpdate::urFAILED;
      u->Msg = Server->GetLastError();
      AddUpdate(u);
      goto exit;
      }
   GETTIME(a);
   while(IsActive() && (Mode==tmCLIENT || (Server->IsActive() && !stats.Total))) {
      if((Mode==tmCLIENT || Mode==tmBOTH) && Client->Connect()) {
         if(Mode==tmBOTH) Server->Stop(); // Since we are connected, we do not need the server anymore
         break;
         }
      else SLEEP(300);
      GETTIME(b);
      if(MUtils::TimeInterval(a,b) > 30000) {
         if((Mode==tmSERVER || Mode==tmBOTH)) Server->Stop();
         u = new MUpdate(this);
         u->Type = MUpdate::utREMOVE;
         u->Result = MUpdate::urFAILED;
         u->Msg = string("Timeout (")+PeerIP+" "+Mode2String()+" "+Dir2String()+")";
         AddUpdate(u);
         file.close();

         if(Direction==tdDOWNLOAD) {
            MUtils::DeleteFile(realpath.c_str());
            }
         goto exit;
         }

      Server->GetStats(stats);
      }
   if(!IsActive()) goto exit;

/*
   if(!Client->IsConnected() && !Server->IsActive()) {
      u.Type = MUpdate::utMSG;
      u.Msg = string("We have connected to ourselves?");
      AddUpdate(u);
      goto exit;
      }
*/

   // we are connected (server or client)
   u = new MUpdate(this);
   u->Type = MUpdate::utADD;
   AddUpdate(u);

   u = new MUpdate(this);
   u->Type = MUpdate::utMSG;
   u->Msg = string("Connected to ")+PeerIP;
   AddUpdate(u);

   if(IsActive() && (Mode==tmCLIENT || Mode==tmBOTH) && Client->IsConnected()) {
      // Client connected
      if(Direction == tdUPLOAD) {
         // we send
         Client->SendStream(); // TODO 3: should test the return of send stream with the len of the file
         Client->Disconnect();
         }
      else while(IsActive() && Client->IsConnected()) {
         // we should be receiving
         SLEEP(500);
         }
      }
   else if((Mode==tmSERVER || Mode==tmBOTH)) {
      // Server connected
      while(IsActive() && Server->NbClients() > 0) {
         // we should be sending or receiving
         SLEEP(500);
         }
      Server->Stop();
      }

   u = new MUpdate(this);
   u->Type = MUpdate::utREMOVE;
   u->Result = MUpdate::urOK;
   u->Msg = "OK";
   AddUpdate(u);
exit:
   SetActive(false);

   }


//---------------------------------------------------------------------------
void MTransfer::Stop() {

   MThread::Stop();

   MUpdateList::iterator ite;
   for(ite=Updates.begin();ite!=Updates.end();ite++)
      delete (*ite);
   Updates.clear();

   }

//---------------------------------------------------------------------------
void MTransfer::EraseUpdate(MUpdateList::iterator& ite) {

   delete (*ite);
   ite = Updates.erase(ite);
/*
   if(ite==Updates.end())
      WriteToLog(2,"inside erase: yes");
   else
      WriteToLog(2,"inside erase: no");
*/

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MTransferManager::MTransferManager() : MThread(), ClearStamp(1) {

   TransferLock = new boost::mutex::scoped_lock(TransferMutex,false);

   }

//---------------------------------------------------------------------------
MTransferManager::~MTransferManager() {

   delete TransferLock;
   Stop();
   boost::mutex::scoped_lock lock(TransferMutex);
   MTransferList::iterator ite;
   for(ite=Transfers.begin();ite!=Transfers.end();ite++) {
      if(*ite) delete (*ite);
      }
   Transfers.clear();

   }

//---------------------------------------------------------------------------
bool MTransferManager::SameTransfer(MTransfer* t1, MTransfer* t2) {

   if(t1->LocalPath      != t2->LocalPath)      return false;
   //if(t1->MD5            != t2->MD5)      return false;
   if(t1->GetDirection() != t2->GetDirection()) return false;
   //if(t1->PeerIP         != t2->PeerIP)       return false;
   // TODO 3: should be based on MD5 also, but we do not set the MD5 for a transfer... why ?
   WriteToLog(3,string("same transfer: ") + t1->LocalPath + " " +MUtils::int2string(t1->GetDirection()));
   return true;

   }

//---------------------------------------------------------------------------
int MTransferManager::StartTransfer(MTransfer* t) {

   boost::mutex::scoped_lock lock(TransferMutex);
   int uploads = 0;
   MTransferList::iterator ite;
   // Verification
   for(ite=Transfers.begin();ite!=Transfers.end();ite++) {
      if((*ite)->IsPending() || !(*ite)->IsActive()) continue;
      if((*ite)->GetDirection() == tdUPLOAD) ++uploads;
      // DONE 3: no pendings for now, too much complicated when restarting it
      // We cancel it and wait for the client to query again
      if(uploads >= 3) {
         //t->SetPending(true); // delay the start
         WriteToLog(3,string("too much uploads. transfer cancelled: ") + t->LocalPath + " " +MUtils::int2string(t->GetDirection()));
         //return 0;
         return -1;
         }
      if((*ite) != t && SameTransfer(*ite, t)) {
         WriteToLog(2,string("cancelled same transfer: ") + t->LocalPath + " " +MUtils::int2string(t->GetDirection()));
         return -1;
         }
      }

   if(t->GetDirection() == tdUPLOAD) {
      t->ID   = Stats.Total;
      t->Port = 6000 + Stats.Total;//*2;
      // Remember: We choose the local port but the peer's same port could be busy 
      }
   t->SetPending(false);
   t->Start();

   Transfers.push_back(t);
   ++Stats.Total;
   ++Stats.Cur;
   if(Stats.Max < Stats.Cur) Stats.Max = Stats.Cur;
   lock.unlock();

   Start();
   WriteToLog(2,t->Dir2String() + " started: " + t->LocalPath);
   return 1;

   }

//---------------------------------------------------------------------------
void MTransferManager::Cancel(int id, MTransfer::MUpdate& u) {

   boost::mutex::scoped_lock lock(TransferMutex);
   MTransferList::iterator ite;
   for(ite=Transfers.begin();ite!=Transfers.end();ite++) {
      if((*ite)->ID != id) continue;
      // update
      u.Type   = MTransfer::MUpdate::utREMOVE;
      u.Result = MTransfer::MUpdate::urOK;
      u.PeerIP = (*ite)->PeerIP;
      u.ID     = (*ite)->ID;
      u.Result  = MTransfer::MUpdate::urOK;
      u.Direction = (*ite)->GetDirection();
      u.Msg    = "Cancelled";
      // delete
      (*ite)->Stop();
      delete (*ite);
      (*ite) = NULL;
      Transfers.erase(ite);
      boost::mutex::scoped_lock lock(StatMutex);
      --Stats.Cur;
      break;
      }

   }

//---------------------------------------------------------------------------
void MTransferManager::operator()() {

   SetActive(true);
   MTransferList::iterator ite;
   boost::mutex::scoped_lock lock(TransferMutex,false);
   MTransfer::MUpdateList* u;
   while(IsActive()) {
      lock.lock();
      for(ite=Transfers.begin();ite!=Transfers.end();ite++) {
         if((*ite)->IsPending())
            StartTransfer(*ite);

         else {
            // Read Updates
            u = (*ite)->LockUpdates();
            MTransfer::MUpdateList::iterator uite = u->begin();
            while(uite!=u->end()) {
               if( ! ((*uite)->GetStamp() & 1)) {
                  (*uite)->SetStamp(1);
                  }
               if((*uite)->GetStamp()==ClearStamp) {
                  if((*uite)->Type == MTransfer::MUpdate::utREMOVE)
                     (*ite)->Dead = true;
                  (*ite)->EraseUpdate(uite);
                  }
               else ++uite;
               }
            (*ite)->UnlockUpdates();
            }


         // clean stopped transfers
/*
         1) at first is the transfer active ?
            Yes, it is Start() before beeing added to the Transfers list
            But beware of pendings if we use them
         2) Why the GUI is not updated when a transfer is finished ?
            Is !(*ite)->NbUpdates() working well ?
*/
         if(!(*ite)->IsActive() && (*ite)->Dead) {
            (*ite)->Stop(); // this also delete all the updates for this transfer
            delete (*ite);
            (*ite) = NULL;
            ite = Transfers.erase(ite);
            boost::mutex::scoped_lock lock(StatMutex);
            --Stats.Cur;
            }
         } // for transfers
      lock.unlock();
      SLEEP(1000);
      } // while active
   SetActive(false);

   }
