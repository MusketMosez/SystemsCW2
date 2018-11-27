#include <ctype.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <queue>

#include <thread>
#include <mutex>

using namespace std;

struct workQ{
private:
  queue<string> workQ;
  mutex mutex;
public:
  void add_to_queue(char *name){
	  unique_lock<mutex> lock(mutex);
	  workQ.push_back(name);
  }  
	
  void remove_front(char *name){
	  unique_lock<mutex> lock(mutex);
	  workQ.pop_front(name);
  }
  list<string>::iterator begin()  { return workQ.begin(); }
  list<string>::iterator end()    { return workQ.end(); }
  
}

struct theTable{
private:
  unordered_map<string, list<string>> theTable;
  mutex mutex;
public:
  void add_to_map(char *obj, char *arg){
	  unique_lock<mutex> lock(mutex);
	  theTable.insert({obj, arg}):
  }
  char *find(char *key){
	  return theTable.find(key);
  }      
  unordered_map<string, list<string>::iterator end() {return theTable.end(); }
}
      
  
vector<string> dirs;
//unordered_map<string, list<string>> theTable;
//list<string> workQ;


string dirName(const char * c_str) {
  string s = c_str; // s takes ownership of the string content by allocating memory for it
  if (s.back() != '/') { s += '/'; }
  return s;
}

pair<string, string> parseFile(const char* c_file) {
  string file = c_file;
  string::size_type pos = file.rfind('.');
  if (pos == string::npos) {
    return {file, ""};
  } else {
    return {file.substr(0, pos), file.substr(pos + 1)};
  }
}

// open file using the directory search path constructed in main()
static FILE *openFile(const char *file) {
  FILE *fd;
  for (unsigned int i = 0; i < dirs.size(); i++) {
    string path = dirs[i] + file;
    fd = fopen(path.c_str(), "r");
    if (fd != NULL)
      return fd; // return the first file that successfully opens
  }
  return NULL;
}

// process file, looking for #include "foo.h" lines
static void process(const char *file, list<string> *ll) {
  char buf[4096], name[4096];
  // 1. open the file
  FILE *fd = openFile(file);
  if (fd == NULL) {
    fprintf(stderr, "Error opening %s\n", file);
    exit(-1);
  }
  while (fgets(buf, sizeof(buf), fd) != NULL) {
    char *p = buf;
    // 2a. skip leading whitespace
    while (isspace((int)*p)) { p++; }
    // 2b. if match #include 
    if (strncmp(p, "#include", 8) != 0) { continue; }
    p += 8; // point to first character past #include
    // 2bi. skip leading whitespace
    while (isspace((int)*p)) { p++; }
    if (*p != '"') { continue; }
    // 2bii. next character is a "
    p++; // skip "
    // 2bii. collect remaining characters of file name
    char *q = name;
    while (*p != '\0') {
      if (*p == '"') { break; }
      *q++ = *p++;
    }
    *q = '\0';
    // 2bii. append file name to dependency list
    ll->push_back( {name} );
    // 2bii. if file name not already in table ...
    if (theTable.find(name) != theTable.end()) { continue; }
    // ... insert mapping from file name to empty list in table ...
    theTable.add_to_map( { name, {} } );
    // ... append file name to workQ
    workQ.add_to_queue( name );
  }
  // 3. close file
  fclose(fd);
}
void processQ(){
	 // 4. for each file on the workQ
  while ( workQ.size() > 0 ) {
    string filename = workQ.front();
    workQ.remover_front();

    if (theTable.find(filename) == theTable.end()) {
      fprintf(stderr, "Mismatch between table and workQ\n");
      return -1;
    }

    // 4a&b. lookup dependencies and invoke 'process'
    process(filename.c_str(), &theTable[filename]);
  }
}	

// iteratively print dependencies
static void printDependencies(unordered_set<string> *printed,
                              list<string> *toProcess,
                              FILE *fd) {
  if (!printed || !toProcess || !fd) return;

  // 1. while there is still a file in the toProcess list
  while ( toProcess->size() > 0 ) {
    // 2. fetch next file to process
    string name = toProcess->front();
    toProcess->pop_front();
    // 3. lookup file in the table, yielding list of dependencies
    list<string> *ll = &theTable[name];
    // 4. iterate over dependencies
    for (auto iter = ll->begin(); iter != ll->end(); iter++) {
      // 4a. if filename is already in the printed table, continue
      if (printed->find(*iter) != printed->end()) { continue; }
      // 4b. print filename
      fprintf(fd, " %s", iter->c_str());
      // 4c. insert into printed
      printed->insert( *iter );
      // 4d. append to toProcess
      toProcess->push_back( *iter );
    }
  }
}

int main(int argc, char *argv[]) {
  // 1. look up CPATH in environment
  char *cpath = getenv("CPATH");
  int nthread;

  // determine the number of -Idir arguments
  int i;
  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-I", 2) != 0)
      break;
  }
  int start = i;
  char *crawler = getenev("CRAWLER_THREADS");
  if (!crawler) nthread = 2;
  else sscanf(crawler, "%d%, &nthread);
  // 2. start assembling dirs vector
  dirs.push_back( dirName("./") ); // always search current directory first
  for (i = 1; i < start; i++) {
    dirs.push_back( dirName(argv[i] + 2 /* skip -I */) );
  }
  if (cpath != NULL) {
    string str( cpath );
    string::size_type last = 0;
    string::size_type next = 0;
    while((next = str.find(":", last)) != string::npos) {
      dirs.push_back( str.substr(last, next-last) );
      last = next + 1;
    }
    dirs.push_back( str.substr(last) );
  }
  // 2. finished assembling dirs vector

  // 3. for each file argument ...
  for (i = start; i < argc; i++) {
    pair<string, string> pair = parseFile(argv[i]);
    if (pair.second != "c" && pair.second != "y" && pair.second != "l") {
      fprintf(stderr, "Illegal extension: %s - must be .c, .y or .l\n",
              pair.second.c_str());
      return -1;
    }

    string obj = pair.first + ".o";

    // 3a. insert mapping from file.o to file.ext
    theTable.add_to_map( { obj, { argv[i] } } );
    
    // 3b. insert mapping from file.ext to empty list
    theTable.add_to_map( { argv[i], { } } );
    
    // 3c. append file.ext on workQ
    workQ.add_to_queue( argv[i] );
  }
  thread threads[nthread];
  for(i = 0, i < nthreads, i++)
	  thread[i](processQ);
  
  for (i = 0; i < nthread; i++) threads[i].join();
   
  // 5. for each file argument
  for (i = start; i < argc; i++) {
    // 5a. create hash table in which to track file names already printed
    unordered_set<string> printed;
    // 5b. create list to track dependencies yet to print
    list<string> toProcess;

    pair<string, string> pair = parseFile(argv[i]);

    string obj = pair.first + ".o";
    // 5c. print "foo.o:" ...
    printf("%s:", obj.c_str());
    // 5c. ... insert "foo.o" into hash table and append to list
    printed.insert( obj );
    toProcess.push_back( obj );
    // 5d. invoke
    printDependencies(&printed, &toProcess, stdout);

    printf("\n");
  }

  return 0;
}
