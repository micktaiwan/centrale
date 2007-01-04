//---------------------------------------------------------------------------
#ifndef MUtilsH
#define MUtilsH

#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

#ifdef WIN32
   typedef unsigned long DWORD;
   #define TIME DWORD
   #define GETTIME(a)  a = GetTickCount()
   #define RESETTIME(a)  a = 0
   #define ISTIMEZERO(a) a == 0
#else
   #include <sys/time.h>
   #define TIME struct timeval
   #define GETTIME(a)  gettimeofday(&(a),NULL)
   #define RESETTIME(a)  a.tv_sec = a.tv_usec = 0
   #define ISTIMEZERO(a) (a.tv_sec == 0 &&  a.tv_usec == 0)
#endif

//---------------------------------------------------------------------------
class BadConversion : public std::runtime_error {
public:
   BadConversion(const std::string& s) : std::runtime_error(s) {}
   };

//---------------------------------------------------------------------------
namespace MUtils {

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MFilePtr {

   FILE* p;

public:

   MFilePtr(const char* n, const char *a) { p = fopen(n,a); }
   //MFilePtr(const std::string& n, const char *a) { MFilePtr(n.c_str(),a); }
   MFilePtr(FILE* pp) { p = pp; }
   ~MFilePtr(){if(p) fclose(p);}

   operator FILE*() {return p;}

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MCharPtr {

   char* p;

public:

   MCharPtr(char* n) { p = n; }
   ~MCharPtr(){if(p) delete [] p;}

   operator char*() {return p;}

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
class          MDate {
   public:
      class baddate : public std::exception {
         public:
            baddate(const std::string& m) {Msg=m;}
            ~baddate() throw() {}
            const char* what() const throw() {return Msg.c_str();}
         private:
            std::string Msg;
         };

      MDate() {}
      ~MDate() {}

      bool MDate::empty() const {return Date.empty();}

      void Set(const std::string& d) {
         //if(d.size() != 17) throw baddate("invalid date, size() must be 15");
         Date = d;
         }
      MDate& operator =(const MDate& d) {Set((std::string)d);return *this;}
      operator std::string() const {return Date;}
      const char* c_str() const {return Date.c_str();}
      std::string Format(int f=0) const;
      void Decode(int& y,int& m,int& d,int& h,int& mn,int& s,int& ms) const;
      //unsigned long AsMinutes();
   private:
      std::string Date;
   };
//---------------------------------------------------------------------------
bool operator <(const MDate& a, const MDate& b);
bool operator ==(const MDate& a, const MDate& b);
bool operator !=(const MDate& a, const MDate& b);
std::ostream& operator <<(std::ostream& o, const MDate& m);
MDate operator -(const MDate& a, const MDate& b);
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MProfiler {

public:
   MProfiler(const std::string& name, int loglevel=3, int average=1);
   ~MProfiler();

private:
   std::string Name;
   int LogLevel;
   int Average;
   TIME A;
   //std::vector<int> Data;

   };

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class MTree;
//class MTreeNode;
//typedef std::vector<MTreeNode*> MTreeNodeList;

//---------------------------------------------------------------------------
template <class T>
class MTreeNode {
public:
   T*          Data;
   MTreeNode<T>*     Parent;
   MTree*         Tree;
   int            Index;

   MTreeNode(MTree* t, void* d) {Tree = t; Data = d;}
   ~MTreeNode();

   void AddChild(MTreeNode<T>* c);
   MTreeNode<T>* GetNextSibling();
   MTreeNode<T>* GetChild(int index) {
      if(index<0 || index > Children.size()-1) return NULL;
      return *(Children.begin()+index);
      }
   MTreeNode<T>* GetFirstChild() {
      return GetChild(0);
      }

private:
   //MTreeNodeList  Children;
   std::vector<MTreeNode<T>*> Children;
   };

//---------------------------------------------------------------------------
/*
class MTree {
public:
   MTree() {};
   ~MTree();

   template <class T>
   MTreeNode<T>* AddChild(MTreeNode<T>* n, void* t);
   template <class T>
   MTreeNode<T>* GetFirstRoot() {
      if(Roots.size()) return *Roots.begin();
      return NULL;
      }
   template <class T>
   MTreeNode<T>* GetChild(int index) {
      if(index<0 || index > Roots.size()-1) return NULL;
      return *(Roots.begin()+index);
      }

private:
   template <class T>
   std::vector<MTreeNode<T>*> Children;

   };
*/

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Functions
ptime          Now();
void           Replace(std::string& str, const std::string& from, const std::string& to);
std::string    ToSQL(const std::string& token);
template<class OutIt>
/// Does not keep the empty strings !!!
void           Split(const std::string& s, const std::string& sep, OutIt dest);
std::string    Tolower(std::string str);
std::string    char2string(char i);
std::string    int2string(int i);
std::string    long2string(unsigned long i);
int            string2int(const std::string& s);
std::string    int2byte(int i);
//---------------------------------------------------------------------------
/// a is older, return in msecs
unsigned long  TimeInterval(const TIME& a, const TIME& b);
void GetDuration(const TIME& from, unsigned int& d, unsigned int& h, unsigned int& m);

//---------------------------------------------------------------------------
/** \brief return a date string
t can be:
 0:Date
 1:Date + Time
 2:Time w/ secs
 3:Time wo/ seconds
*/
std::string    MyNow(int t);
template <class T> std::string toStr(T i);
std::string    toStr(int i);
std::string    toStr(unsigned int i);
std::string    GetAppFileName();
void           Back2Slash(std::string& str);
void           Slash2Back(std::string& str);
std::string    ExtractFilePath(const std::string& str);
std::string    ExtractFileName(const std::string& str);
std::string    ExtractFileNameWithoutExt(const std::string& str);
std::string    ExtractFileExt(const std::string& str);
bool           FileExists(const std::string& str);
#ifdef WIN32
unsigned __int64 FileSize(const std::string& fn);
#else
unsigned long  FileSize(const std::string& fn);
#endif
ptime          FileDate(const std::string& str);
bool           RenameFile(const std::string& from,const std::string& to);
bool           DeleteFile(const std::string& str);
bool           IsPrefix(const std::string& prefix, const std::string& str);
bool           CheckAnotherInstance(const char* name);
std::string    FixedDigit(int i, int nb=2);
} // namespace
#endif
