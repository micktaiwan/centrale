#ifdef WIN32
#pragma package(smart_init)
#endif

#include <algorithm>
#include <sstream>
#include <fstream>
#include <ctime>
#include <cctype>
//#include "dirent.h"
#include "MUtils.h"

#ifdef WIN32
   #include <windows.h>
   #include <SysUtils.hpp> // VCL FileExists
                           // TODO 3: change this
   #include <dir.h>
#else

   #include <unistd.h>
   #include <sys/stat.h>
#endif

#undef DeleteFile

using namespace std;

extern void WriteToLog(int t,const string& msg);

//---------------------------------------------------------------------------
// split
template<class OutIt>
void MUtils::Split(const string& s, const string& sep, OutIt dest) {
// Does not keep the empty strings !!!

   string::size_type left = s.find_first_not_of( sep );
   string::size_type right = s.find_first_of( sep, left );
   while( left < right ) {
      *dest = s.substr( left, right-left );
      ++dest;
      left = s.find_first_not_of( sep, right );
      right = s.find_first_of( sep, left );
      }

   }
   
namespace MUtils {

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
string MDate::Format(int f) const {

   if(Date.size() < 17) return Date;

   if(f==0) // standard
      return Date.substr(0,4) + "/" + Date.substr(4,2) + "/" + Date.substr(6,2)
         + " " + Date.substr(8,2) + ":" + Date.substr(10,2) + ":" + Date.substr(12,2);
   if(f==1) {// relative
      MDate d = Now()-(*this);
      s = d.substr(0,4);
      if(s!="0000") return
       + " " + Date.substr(4,2) + " " + Date.substr(6,2)
         + " " + Date.substr(6,2) + ":" + Date.substr(8,2) + ":" + Date.substr(10,2);
      }

   return "invalid format";

   }

//---------------------------------------------------------------------------
void MDate::Decode(int& y,int& m,int& d,int& h,int& mn,int& s,int& ms) const {

   if(Date.size() < 17) {
      WriteToLog(1,"MDate::Decode invalid date");
      return;
      }

   y  = atoi(Date.substr(0,4).c_str());
   m  = atoi(Date.substr(4,2).c_str());
   d  = atoi(Date.substr(6,2).c_str());
   h  = atoi(Date.substr(8,2).c_str());
   mn = atoi(Date.substr(10,2).c_str());
   s  = atoi(Date.substr(12,2).c_str());
   ms = atoi(Date.substr(14,3).c_str());

   }

//---------------------------------------------------------------------------
unsigned long MDate::AsMinutes() {

   int y1,m1,d1,h1,mn1,s1,ms1;
   Decode(y1,m1,d1,h1,mn1,s1,ms1);
   return y1*

   }
*/
//---------------------------------------------------------------------------
/*
bool operator <(const MDate& a, const MDate& b) {

   return (string)a < (string)b;

   }

//---------------------------------------------------------------------------
bool operator ==(const MDate& a, const MDate& b) {

   return (string)a == (string)b;

   }

//---------------------------------------------------------------------------
bool operator !=(const MDate& a, const MDate& b) {

   return (string)a != (string)b;

   }

//---------------------------------------------------------------------------
MDate operator -(const MDate& a, const MDate& b) {

   int y1,m1,d1,h1,mn1,s1,ms1;
   int y2,m2,d2,h2,mn2,s2,ms2;
   if(a < b) {
      b.Decode(y1,m1,d1,h1,mn1,s1,ms1);
      a.Decode(y2,m2,d2,h2,mn2,s2,ms2);
      }
   else {
      a.Decode(y1,m1,d1,h1,mn1,s1,ms1);
      b.Decode(y2,m2,d2,h2,mn2,s2,ms2);
      }

   int y  =  y1  - y2;
   int m  =  m1  - m2;
   int d  =  d1  - d2;
   int h  =  h1  - h2;
   int mn =  mn1 - mn2;
   int s  =  s1  - s2;
   int ms =  ms1 - ms2;
   if(ms<0) {ms = 1000+ms;--s;}
   if(s<0) {s = 60+s;--mn;}
   if(mn<0) {mn = 60+mn;--h;}
   if(h<0) {h = 24+h;--d;}
   if(d<0) {d = 31+d;--m;}
   if(m<0) {m = 12+m;--y;}
   MDate date;
   if(y<0) {
      WriteToLog(1,"operator - : invalid date");
      return date;
      }

   // TODO: not finished
   ostringstream o;
   o << FixedDigit(y,4) << FixedDigit(m) << FixedDigit(d) << FixedDigit(h) << FixedDigit(mn) << FixedDigit(s) << FixedDigit(ms,3);
   date.Set(o.str());
   return date;

   }

//---------------------------------------------------------------------------

std::ostream& operator <<(std::ostream& o, const MDate& m) {

   o << (string)m;

   return o;

   }
*/
   
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MProfiler::MProfiler(const std::string& name, int loglevel, int average)
   : Name(name), LogLevel(loglevel), Average(average) {

   GETTIME(A);

   }

//---------------------------------------------------------------------------

MProfiler::~MProfiler() {

   TIME b;
   GETTIME(b);
   int d = TimeInterval(A,b);
   if(d==0) return; // tmp
   WriteToLog(LogLevel, Name+": "+int2string(d));

   }

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
MTreeNode::~MTreeNode() {
   delete Data;
   MTreeNodeList::iterator ite = Children.begin();

   while(ite!=Children.end()) {
      delete (*ite);
      ++ite;
      }
   }

//---------------------------------------------------------------------------
void MTreeNode::AddChild(MTreeNode* c) {

   c->Parent = this;
   c->Index = Children.size();
   Children.push_back(c);

   }

//---------------------------------------------------------------------------
MTreeNode* MTreeNode::GetNextSibling() {

   if(Parent) return Parent->GetChild(Index+1);
   return Tree->GetChild(Index+1);


   }



//---------------------------------------------------------------------------
MTree::~MTree() {
   MTreeNodeList::iterator ite = Roots.begin();
   while(ite!=Roots.end()) {
      delete (*ite);
      ++ite;
      }
   }

//---------------------------------------------------------------------------
MTreeNode* MTree::AddChild(MTreeNode* n, void* d) {

   MTreeNode* c = new MTreeNode(this, d);
   if(n) n->AddChild(c);
   else {
      c->Parent = NULL;
      c->Index = Roots.size();
      Roots.push_back(c);
      }
   return c;

   }
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
string FixedDigit(int i, int nb) {

   string i_str = int2string(i);
   int len = i_str.size();
   if(len >= nb) return i_str;

   if(i >= 0)  return string("0",nb-len) + i_str;
   else return string("-") + string("0",nb-len) + int2string(-i);

   }

//---------------------------------------------------------------------------
// Now
ptime Now() {

/*
   MDate n;
#ifdef WIN32
   SYSTEMTIME t;
   GetLocalTime(&t);
   ostringstream o;
   o << t.wYear << FixedDigit(t.wMonth) << FixedDigit(t.wDay) << FixedDigit(t.wHour) << FixedDigit(t.wMinute) << FixedDigit(t.wSecond) << FixedDigit(t.wMilliseconds,3);
   n.Set(o.str());
#else
   #error todo
#endif
   return n;
*/
   return (second_clock::local_time());

   }
//---------------------------------------------------------------------------
// Replace
void Replace(string& str, const string& from, const string& to) {

   unsigned int pos;
   /*while((*/
   pos = str.find(from);
   //)!= string::npos) {
   if(pos != string::npos)
      str.replace(pos, from.size(), to);
      //}

   }

//---------------------------------------------------------------------------
// ToSQL
string ToSQL(const string& token) {

   string rv = token;


   Replace(rv,"'","''");
   return rv;

   }

//---------------------------------------------------------------------------
// tolower
string Tolower(string str) {

   // gnu need this
   int (*pf)(int) = std::tolower;
   transform(str.begin(), str.end(), str.begin(), pf);
   return str;

   }

//---------------------------------------------------------------------------
// char2string
string char2string(char i) {
   ostringstream o;
   o << i;
   return o.str();
   }

//---------------------------------------------------------------------------
// int2string
string int2string(int i) {
   ostringstream o;
   o << i;
   return o.str();
   }

//---------------------------------------------------------------------------
// long2string
string long2string(unsigned long i) {
   ostringstream o;
   o << i;
   return o.str();
   }

//---------------------------------------------------------------------------
// string2int
int string2int(const string& s) {
   istringstream buf(s);
   int a=0;
   if(!(buf >> a))
      throw BadConversion(string("convertToInt(\"") + s + "\")");
   return a;
   }

//---------------------------------------------------------------------------
// int2byte
string int2byte(int i) {

   string s = "x";
   s[0] = i%256;
   return s;
   
   }

//---------------------------------------------------------------------------
// MyNow
string MyNow(int t) {

   const time_t ti = time(NULL);
   ostringstream o;
   struct tm ts;

#ifdef WIN32
   ts = *localtime(&ti);
#else
   localtime_r(&ti, &ts);
#endif

   if(t==0 || t==1)
      o << ts.tm_year+1900 << "/" << ts.tm_mon+1 << "/" << ts.tm_mday;
   if(t==1) o << " ";

   if(t>=1) {
      o  << ts.tm_hour << ":";
      if(ts.tm_min < 10) o << "0";
      o << ts.tm_min;
      }
   if(t==1 || t==2)
      o << ":" << ts.tm_sec;

   return o.str();

   }

//---------------------------------------------------------------------------
// TimeInterval
unsigned long TimeInterval(const TIME& a, const TIME& b) {
#ifdef WIN32
    return b-a;
#else
    return ((b.tv_sec - a.tv_sec)*1000000 + (b.tv_usec - a.tv_usec))/1000;
#endif
    }


//---------------------------------------------------------------------------
// GetAppFileName
string GetAppFileName() {

   char fn[256];
#ifdef WIN32
   GetModuleFileName(GetModuleHandle(NULL), fn, 256);
#else
   int c = readlink("/proc/self/exe",fn,256);
   if(c >= 0) fn[c] = 0;
   else fn[0] = 0;
#endif
   return string(fn);

   }

//---------------------------------------------------------------------------
// toStr
template <class T>
string toStr(T i) { // why it does not work ?
   ostringstream o;
   o << i;
   return o.str();
   }
string toStr(int i) {
   ostringstream o;
   o << i;
   return o.str();
   }
string toStr(unsigned int i) {
   ostringstream o;
   o << i;
   return o.str();
   }

//---------------------------------------------------------------------------
static char S2B(char c) {
   if(c=='/') return '\\';
   return c;
   }
static char B2S(char c) {
   if(c=='\\') return '/';
   return c;
   }
void Back2Slash(string& str) {
   transform(str.begin(), str.end(), str.begin(), B2S);
   }
void Slash2Back(string& str) {
   transform(str.begin(), str.end(), str.begin(), S2B);
   }

//---------------------------------------------------------------------------
string ExtractFilePath(const string& str) {

   string tmp = str;
   Back2Slash(tmp);
   unsigned int i = tmp.rfind('/');
   if(i==string::npos) return tmp;

   return tmp.substr(0,i+1);

   }

//---------------------------------------------------------------------------
string ExtractFileName(const string& str) {

   string tmp = str;
   Back2Slash(tmp);
   unsigned int i = str.rfind('/');
   if(i==string::npos) return "";
   return tmp.substr(i+1,tmp.length()-(i+1));

   }

//---------------------------------------------------------------------------
string ExtractFileNameWithoutExt(const string& str) {

   string tmp = ExtractFileName(str);
   unsigned int i = tmp.rfind('.');
   if(i==string::npos) return tmp;
   return tmp.substr(0,i);

   }

//---------------------------------------------------------------------------
string ExtractFileExt(const string& str) {

   unsigned int i = str.rfind('.');
   if(i==string::npos) return "";
   return str.substr(i,str.length()-i);

   }


//---------------------------------------------------------------------------


// FileExists
bool FileExists(const std::string& str) {

   ifstream f(str.c_str());
   if(f) return true;
   return false;

   }

//---------------------------------------------------------------------------
// FileSize

#ifdef WIN32
unsigned __int64 FileSize(const string& fn) {

   unsigned __int64 rv = 0;
   HANDLE hfile;
   unsigned long loworder, highorder;
   //const wchar_t* path
   hfile = CreateFile(fn.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, 0);
   if(hfile != INVALID_HANDLE_VALUE) {
      highorder = 0;

      loworder = GetFileSize(hfile, &highorder);
      if(highorder == 0) rv = loworder;
      else rv = (((unsigned __int64)highorder)<<32) + loworder;
      CloseHandle(hfile);
      }
   return rv;
#else
unsigned long FileSize(const string& fn) {

   struct stat s;
   stat(fn.c_str(),&s);
   return s.st_size;

#endif

   }
   
//---------------------------------------------------------------------------
// FileDate

ptime FileDate(const string& fn) {

   // TODO 3: does not work for directories
#ifdef WIN32
   struct ffblk s;
   ostringstream o;
   int i;
   string m, d, h, mn, se, ms = "000";
   if(findfirst(fn.c_str(),&s,FA_HIDDEN)==-1) // TODO 3: add diretories here ?
      return ptime(boost::date_time::not_a_date_time);//throw std::runtime_error(string("File not found: ")+fn);
   i = (s.ff_fdate & 0x01E0)>>5;
   m = MUtils::int2string(i);
   if(i < 10) m = string("0") + m;
   i = (s.ff_fdate & 0x001F);
   d = MUtils::int2string(i);
   if(i < 10) d = string("0") + d;

   i = (s.ff_ftime & 0xF800)>>11;
   h = MUtils::int2string(i);
   if(i < 10) h = string("0") + h;
   i = (s.ff_ftime & 0x07E0)>>6;
   mn = MUtils::int2string(i);
   if(i < 10) mn = string("0") + mn;
   i = (s.ff_ftime & 0x001F)*2;
   se = MUtils::int2string(i);
   if(i < 10) se = string("0") + se;
   o << (1980+((s.ff_fdate & 0xFE00)>>9)) << "/" << m << "/" << d << " " << h << ":" << mn << ":"  << se << ":"  << ms;
   return ptime ( time_from_string(o.str()));
#else
   //struct stat s;
   //stat(fn.c_str(),&s);
   //s.st_mtime;
   // TODO
   return ptime(boost::date_time::not_a_date_time);//throw std::runtime_error("todo");
#endif

   }

//---------------------------------------------------------------------------
bool RenameFile(const std::string& from,const std::string& to) {

#ifdef WIN32
   return ::RenameFile(from.c_str(),to.c_str()); // win32
#else
   return !rename(from.c_str(),to.c_str());
#endif


   }

//---------------------------------------------------------------------------
bool DeleteFile(const std::string& str) {

#ifdef WIN32
   return ::DeleteFileA(str.c_str()); // win32
#else
   return !remove(str.c_str());
#endif

   }

//---------------------------------------------------------------------------
bool IsPrefix(const string& prefix, const string& str) {

   //MUtils::MProfiler p("1");
   if(prefix.size() > str.size()) return false;
   const int len = prefix.size();
   for(int i=0; i < len; i++) {
      if(prefix[i] != str[i]) return false;
      }
   return true;

   }
   
//---------------------------------------------------------------------------
// Check Another Instance
bool CheckAnotherInstance(const char* name) {

#ifdef WIN32
   bool found = false;
   HANDLE m = CreateMutex( NULL, true, name);
   if(GetLastError() == ERROR_ALREADY_EXISTS) found = true;
   if(m) ReleaseMutex(m);
   return found;
#else
   return false;
#endif

   }

//---------------------------------------------------------------------------
// return days, hours, minutes
void GetDuration(const TIME& from, unsigned int& d, unsigned int& h, unsigned int& m) {

   TIME now;
   GETTIME(now);
   unsigned long i = MUtils::TimeInterval(from,now);
   d = i / (1000*60*60*24);
   h = (i / (1000*60*60))-d*24;
   m = (i / (1000*60))-(h*60 + d*24*60);

   }

} // namespace
