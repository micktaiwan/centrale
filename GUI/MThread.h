//---------------------------------------------------------------------------
#ifndef MThreadH
#define MThreadH
#include <boost/thread/thread.hpp>

//---------------------------------------------------------------------------
class MThread {

public:

   MThread(bool a=false) : Active(a), Thread(NULL) {}
   virtual ~MThread() {}

   virtual void operator()()=0;
   virtual void Start();
   virtual void Stop();

   virtual void SetActive(bool a) {
      boost::mutex::scoped_lock lock(ActiveMutex);
      Active = a;
      }

   virtual bool IsActive() {
      boost::mutex::scoped_lock lock(ActiveMutex);
      return Active;
      }

private:
   boost::mutex ActiveMutex;
   bool Active;
   boost::thread* Thread;

   };

#endif

