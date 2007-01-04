/***************************************************************************
                          mpnlbase.h  -  description
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
#ifndef MPNLBaseH
#define MPNLBaseH
/**
  *@author Mickael Faivre-Macon
  */

#include <exception>
#include <string>
#include <list>
#include <queue>
#include <sstream>
#include "MUtils.h"
#include "MThread.h"

#ifdef WIN32
   #include<winsock2.h>

   #define MYSOCKET SOCKET
   #define MYINVALID_SOCKET INVALID_SOCKET
   #define MYSOCKET_ERROR SOCKET_ERROR
   #define MYCLOSE(s) closesocket(s)
   #define SLEEP(x) Sleep(x)
#else
   #include <netinet/in.h>
   #include <arpa/inet.h> // inet_ntoa
   #include <sys/time.h>
   #include <netdb.h>

   #define MYSOCKET int
   #define MYINVALID_SOCKET -1
   #define MYSOCKET_ERROR -1
   #define MYCLOSE(s) close(s)
   #define SLEEP(x) usleep((x)*1000)
#endif

namespace MPNL {

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Global Fonctions

std::string GetLocalIP();
std::string GetHostName(int socket);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Types
enum MMode {MODE_LENPREFIXED, MODE_LINE, MODE_PACKET};
//class error : public std::exception {};
class MPNLBase;
class MSocket;
struct MClient {
   MClient () : Socket(NULL),  ToBeDeleted(false) {}
   MSocket*       Socket;
   bool ToBeDeleted;
   unsigned long ID;
   };
typedef std::list<MClient> MClientList;

class MTransferManager;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/** MSocket is our main threaded class that read and write to a socket
  *@author Mickael Faivre-Macon
  *
  */
class MSocket : public MThread {

public:
   void*          Data;
   MMode          Mode;
   int            PrefixLen;
   struct sockaddr_in ServAddr; ///< for UDP Send and SendTo
   std::string    PeerIP, LocalIP; ///< local IP (testing)
   int            PeerPort;
   std::iostream*  Stream;

   MSocket(int type, MPNLBase* owner);
   virtual ~MSocket();

   void           operator()(); // thread function
   bool           IsInvalid();
   bool           Read(std::string& msg);
   bool           Read(std::string& ip, std::string& msg); // for UDP
   virtual void   Stop();
   int            Send(const std::string& msg);
   unsigned long  SendStream();
   void           SetStream(std::iostream* s) {Stream = s;}
   int            SendTo(const std::string& ip, int port, const std::string& msg);
   MYSOCKET       GetS() const {return S;}
   void           SetS(MYSOCKET s) {S=s;}
   bool           Listen(int port);
   //bool           IsActive();
   bool           HasMsg();
   std::string    GetLocalIP();

protected:
   typedef std::queue<std::string> MMsgQueue;

   MYSOCKET       S;
   int            Type;
   MPNLBase*      Owner;
   MMsgQueue      Messages;
   MMsgQueue      IP; // for UDP
   TIME           LastMsgTime;
   boost::mutex   MsgMutex;

   void PushMsg(const std::string& msg);
   void PushMsg(const std::string& ip, const std::string& msg); // for UDP
   int Send(const char* str, int len, int flags);

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MPNLBase {

public:
   int         Port;

   MPNLBase();
   virtual ~MPNLBase();

   virtual void SetMode(MMode m)=0;
   virtual void SetPrefixLen(int len)=0;
   virtual void SetStream(std::iostream* s)=0;
   virtual bool OnConnection(MSocket* s)=0; ///< return false to reject the connection
   virtual void OnDisconnection(MSocket* s)=0;
   virtual void PushMsg(MSocket*) {};   // public because MSocket call this. Maybe change this.
   void SetLastError(const std::string& a) {
      boost::mutex::scoped_lock lock(Mutex);
      LastError = a;
      }
   std::string GetLastError() {
      boost::mutex::scoped_lock lock(Mutex);
      return LastError;
      }

protected:
   typedef std::deque<MSocket*> MMsgQueue;
   MMsgQueue      Messages;
   MMode          Mode;
   int            PrefixLen;

private:
   std::string    LastError;
   boost::mutex   Mutex;

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MTCPServer : public MPNLBase, public MThread /*accepting connections*/ {

public:
   struct MStats {
      MStats() {Total = CurCon = MaxCon = 0;}
      unsigned long Total, CurCon, MaxCon;
      };

   MTCPServer();
   virtual ~MTCPServer();

   virtual void operator()(); // thread function accepting connections
   bool     Listen();
   bool     Read(MSocket*& s);
   void     Disconnect(MSocket* s);
   virtual bool OnConnection(MSocket* s); // return false to reject the connection
   virtual void OnDisconnection(MSocket* s);
   virtual void SetMode(MMode m);
   virtual void SetPrefixLen(int len);
   virtual void SetStream(std::iostream* s);
   virtual void Stop();
   void     GetStats(MStats& s);
   void     BroadcastMsg(MSocket* s, const std::string& msg);
   int      Send(const std::string& ip, const std::string& msg);
   bool     IsConnected(const std::string& ip);
   int      NbClients() { // or use getstats
      boost::mutex::scoped_lock lock(ClientMutex);
      return Clients.size();
      }
   MClientList* LockClients() {
      while(ClientLock->locked()) {SLEEP(1);} // because it would throw a exception otherwise
      ClientLock->lock();
      return &Clients;
      }
   void UnlockClients() {
      ClientLock->unlock();
      }

protected:
   boost::mutex         ClientMutex;
   MStats               Stats;
   MClientList          Clients;
   boost::mutex::scoped_lock* ClientLock;
   boost::mutex         MsgMutex;
   MSocket*             Socket;
   std::iostream*       Stream;
   virtual void         PushMsg(MSocket* s);
   bool                 SocketHasMsg(MSocket* s);
private:

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MTCPClient : public MPNLBase {

public:
   std::string Host;

   MTCPClient();
   virtual ~MTCPClient();

   virtual bool   Connect();
   virtual void   Disconnect();


   /** Send a msg, see SetMode
     * return system's 'send' return value
     */
   virtual int    Send(const std::string& msg);
   virtual unsigned long  SendStream();

   virtual bool   Read(std::string& msg);
   virtual bool   OnConnection(MSocket* s);
   virtual void   OnDisconnection(MSocket* s);
   virtual bool   IsConnected() {return Socket->IsActive();}
   virtual void   SetMode(MMode m) {Mode = m;Socket->Mode = m;}
   virtual void   SetPrefixLen(int len);
   virtual void   SetStream(std::iostream* s);
   std::string GetLocalIP() {return Socket->GetLocalIP();}

private:
   //boost::mutex         Mutex;
   MSocket*             Socket;
   //boost::thread*       Thread; // reading socket thread
   struct sockaddr_in LocalAddr;
   struct sockaddr_in ServAddr;


   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class MUDPServer : public MPNLBase {

public:
   MUDPServer();
   virtual ~MUDPServer();

   virtual bool   Listen(int port);
   virtual bool   Read(MSocket*& s);
   virtual bool   OnConnection(MSocket* s); // return false to reject the connection
   virtual void   OnDisconnection(MSocket* s);
   virtual void   SetMode(MMode m);
   void SetPrefixLen(int len);
   void SetStream(std::iostream* s);

private:
   //boost::mutex         Mutex;
   boost::mutex         MsgMutex;
   MSocket*             Socket;
   bool                 Active;
   void                 PushMsg(MSocket* s);
   bool                 SocketHasMsg();

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MUDPClient : public MPNLBase {

public:
   std::string Host;

   MUDPClient();
   virtual ~MUDPClient();

   /** Use PrepareToSend once, if you send only message to a specified server on a determined port
     * \verbatim
     udpclient->Port = 6001;
     udpclient->Host = "192.168.8.3";
     udpclient->PrepareToSend() \endverbatim
     * or you can use SendTo(host,port,msg) to send to several servers or port
     */
   virtual bool   OnConnection(MSocket* s); // return false to reject the connection
   virtual void   OnDisconnection(MSocket* s);
   void SetPrefixLen(int len);
   void SetStream(std::iostream* s);

   bool PrepareToSend();
   
   /** Send a msg, see SetMode
     * return system's 'send' return value
     */
   virtual int    Send(const std::string& msg);
   virtual void   SetMode(MMode m) {Mode = m;Socket->Mode = m;}

private:
   MSocket*             Socket;

   };


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
enum MTransferDirection {tdDOWNLOAD=0, tdUPLOAD=1};
enum MTransferMode {tmCLIENT, tmSERVER, tmBOTH};
class MTransfer;
typedef std::list<MTransfer*> MTransferList;

//---------------------------------------------------------------------------
class MTransferTCPServer : public MTCPServer {

public:
   MTransferDirection Direction;
   MTransfer* Transfer;

   MTransferTCPServer(MTransfer* t) : MTCPServer() {Transfer=t;}
   ~MTransferTCPServer() {}


   bool OnConnection(MSocket* s) {
      if(Direction==tdDOWNLOAD) return true;
      boost::mutex::scoped_lock lock(ClientMutex);
      Stats.Total++; // we depend on it
      lock.unlock();
      s->SendStream();  // TODO 3: should test the return of send stream with the len of the file
      SetActive(false);
      return false;
      }
   void OnDisconnection(MSocket* s);
   };

//---------------------------------------------------------------------------
class MTransferTCPClient : public MTCPClient {

public:


   MTransfer* Transfer;

   MTransferTCPClient(MTransfer* t) : MTCPClient() {Transfer=t;}
   ~MTransferTCPClient() {}

   void OnDisconnection(MSocket* s);

   };

//---------------------------------------------------------------------------
/** \brief MTransfer is a one way stream transfer but using both a server and a client
to iniatiate the transfer. Once one of them has etabished a connection,
the other just sit.\n
Todo:
- Progress monitoring
- Bandwidth control
- Resume up/downloads

*/
class MTransfer : public MThread {
   friend class MTransferManager;
public:
   class MUpdate {
      public:
         MUpdate() : Stamp(0) {}
         MUpdate(MTransfer* t) : PeerIP(t->PeerIP), ID(t->ID), Stamp(0) {}
/*
         MUpdate(const MUpdate& u) {
            boost::mutex::scoped_lock lock(u.Mutex);
            Type  = u.Type;
            Msg   = u.Msg;
            Stamp = u.Stamp;
            }
         MUpdate& operator=(const MUpdate& u) {
            if (this == &u) return *this;
            boost::mutex::scoped_lock lock1(&Mutex < &u.Mutex ? Mutex : u.Mutex);
            boost::mutex::scoped_lock lock2(&Mutex > &u.Mutex ? Mutex : u.Mutex);
            Type  = u.Type;
            Msg   = u.Msg;
            Stamp = u.Stamp;
            // Progress
            return *this;
            }
*/
         enum MType {utADD, utPROGRESS, utMSG, utREMOVE} Type;
         enum MResult {urOK, urFAILED} Result;
         std::string Msg, PeerIP;
         int ID;
         MTransferDirection Direction;
         unsigned char GetStamp() {
            boost::mutex::scoped_lock lock(Mutex);
            return Stamp;
            }
         void SetStamp(unsigned char s) {
            boost::mutex::scoped_lock lock(Mutex);
            Stamp |= s;
            }
         //unsigned long Progress;

      private:
         /*mutable*/ boost::mutex   Mutex;
         unsigned char Stamp; // when == Manager::ClearStamp, it is erased by the manager
      };
   typedef std::deque<MUpdate*> MUpdateList;

   MTransferMode  Mode;
   std::string    PeerIP;
   std::string    LocalPath;
   std::string    MD5;
   std::string    URL;
   int            Port;
   // TODO 3: ID is for identifying a client.
   // Server will refuse any clients with not the same ID
   int            ID;
   bool           Dead;

   MTransfer();
   ~MTransfer();

   void Stop(void);  ///< Stop (Cancel) the thread. Disk file is kept.
   MTransferDirection GetDirection() {return Direction;}
   void SetDirection(MTransferDirection dir) {
      Direction = dir;
      Server->Direction = dir;
      }
   //bool IsActive() {
   //   boost::mutex::scoped_lock lock(Mutex);
   //   return Active;
   //   }
   bool IsPending() {
      boost::mutex::scoped_lock lock(AccessMutex);
      return Pending;
      }
   void SetPending(bool b) {
      boost::mutex::scoped_lock lock(AccessMutex);
      Pending = b;
      }
   /*const std::string& GetError() {
      boost::mutex::scoped_lock lock(Mutex);
      return Error;
      }*/
   void AddUpdate(MUpdate* u) {
      boost::mutex::scoped_lock lock(UpdateMutex);
      Updates.push_back(u);
      }

   MUpdateList* LockUpdates() {
      UpdateLock->lock();
      return &Updates;
      }
   void UnlockUpdates() {
      UpdateLock->unlock();
      }
      
   int NbUpdates() {
      boost::mutex::scoped_lock lock(UpdateMutex);
      return Updates.size();
      }

   std::string Mode2String() {
      switch(Mode) {
         case tmCLIENT: return "Client";
         case tmSERVER: return "Server";
         case tmBOTH: return "Both";
         default: return "Unknown";
         }
      }
   std::string Dir2String() {
      switch(Direction) {
         case tdUPLOAD: return "Upload";
         case tdDOWNLOAD: return "Download";
         default: return "Unknown";
         }
      }
   void Disconnected() {SetActive(false);}
   void EraseUpdate(MUpdateList::iterator& ite);

   void operator()(); ///< Thread function. Do not call, use Start()

private:
   //bool                 Active;
   bool                 Pending; ///< if true not started but want to be
   MTransferTCPServer*  Server;
   MTransferTCPClient*  Client;
   //boost::thread*       Thread;
   MTransferDirection   Direction;
   //std::string          Error;
   MUpdateList          Updates;
   boost::mutex::scoped_lock* UpdateLock;
   boost::mutex         UpdateMutex;
   boost::mutex         AccessMutex;

   //void Start(void); ///< Start negociations (thread). Moved to private recently.

   //void SetActive(bool b) {
   //   boost::mutex::scoped_lock lock(Mutex);
   //   Active = b;
   //   }
   /*void SetError(const std::string& s) {
      boost::mutex::scoped_lock lock(Mutex);
      Error = s;
      }*/

   };

inline  void MTransferTCPServer::OnDisconnection(MSocket* /*s*/) {
   Transfer->Disconnected();
   }
inline  void MTransferTCPClient::OnDisconnection(MSocket* /*s*/) {
   Transfer->Disconnected();
   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/** MTransferManager manage file transfers.\n
Features:
- LowId clients can upload to non LowId clients
Todo:
- Progress monitoring
- Bandwidth control
- Numbers of client control
Dream:
- Pass through several HighId clients to tranfer a file between 2 LowId clients
*/
class MTransferManager : public MThread {

public:

   struct MStats {
      MStats() : Total(0),Cur(0),Max(0) {}
      unsigned int Total, Cur, Max, DL, UP;
      } Stats;

   unsigned char ClearStamp;

   MTransferManager();
   ~MTransferManager();

   /** Ask to start this transfer
     * \return 1 if started, 0 if pending, -1 if cancelled
     */
   int StartTransfer(MTransfer* t);

   void Cancel(int id, MTransfer::MUpdate& u);

   bool SameTransfer(MTransfer* t1, MTransfer* t2);

   MTransferList* LockTransfers() {
      TransferLock->lock();
      return &Transfers;
      }
   void UnlockTransfers() {
      TransferLock->unlock();
      }

   void operator()();

private:
   MTransferList  Transfers;
   boost::mutex::scoped_lock* TransferLock;
   boost::mutex         TransferMutex;
   boost::mutex         StatMutex;


/*
   void           SetActive(bool a) {
      boost::mutex::scoped_lock lock(Mutex);
      Active = a;
      }
*/

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // namespace

#endif

