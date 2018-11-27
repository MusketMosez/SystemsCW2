#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <thread>
#include <mutex>

using namespace std;

struct tsll {
private:
    list<string> workQ;
    mutex m;
    int size;
public:

    tsll() {
		size = 0;
		workQ = list<string>();

	}

    void add_to_queue(string name){
	    unique_lock<mutex> lock(m);
	    workQ.push_back(name);
            size++;
    }

    string front() {
        unique_lock<mutex> lock(m);
        return workQ.front();
    }

    void remover_front(){
	    unique_lock<mutex> lock(m);
	    workQ.pop_front();
            size--;
    }

    int get_size() {
        unique_lock<mutex> lock(m);
        return workQ.size();
    }


    /*list<string>::iterator end() {
        unique_lock<mutex> lock(m);
        return workQ.end();
    }*/

};

struct tsht {

private:
    unordered_map<string, list<string>> theTable;
    mutex m;

public:
  unordered_map<string,list<string> >::iterator search(string item)
  {
    unique_lock<mutex> lock(m);
    return theTable.find(item);
  }

    void add_to_map(string obj, list<string> arg) {
	    unique_lock<mutex> lock(m);
	    theTable.insert({obj, arg});
    }

    list<string> *find(string key) {
        unique_lock<mutex> lock(m);
	      auto iter = theTable.find(key);
        return &(iter -> second);
    }

    unordered_map<string, list<string>>::iterator end() {
        unique_lock<mutex> lock(m);
        return theTable.end();
    }

};

vector<string> dirs;
tsht *theTable = new tsht();
tsll *workQ = new tsll();


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

// open file using the directory sprocessqearch path constructed in main()
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
    // 1. open the file, tsht *theTable , tsll *workQ
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
        p += 8; // point to first cha, tsht *theTable , tsll *workQracter past #include
        // 2bi. skip leading whitespace
        while (isspace((int)*p)) { p++; }
        if (*p != '"') { continue; }

        // 2bii. next character is a "
        p++; // skip "
        // 2bii. collect remaininprocessqg characters of file name)
        char *q = name;
        while (*p != '\0') {
            if (*p == '"') { break; }
            *q++ = *p++;
        }
        *q = '\0';
        // 2bii. append file name to dependency list
        ll->push_back( {name} );
        // 2bii. if file name not already in theTable ...
        if (theTable->search(name)!= theTable->end()) { continue; }
        // ... insert mapping froprocessqm file name to empty list in theTable ...
        theTable->add_to_map(name, {});
        // ... append file name to workQ
        workQ->add_to_queue( name );
    }
    // 3. close file
    fclose(fd);

}

void par() {//  threads[i] = thread(processQ);

  while ( workQ->get_size() > 0 ) {
      string filename = workQ->front();
      workQ->remover_front();

      if (!theTable->find(filename)) {
          fprintf(stderr, "Mismatch between theTable and workQ\n");
          return;
      }

      // 4a&b. lookup dependencies and invoke 'process'
      //process(filename.c_str(), (*thetheTable).find(filename));
      process(filename.c_str(), theTable->find(filename));
  }
}

void processQ(int nthread) {
	 // 4. for each file on the workQ

   vector<thread> threads;

  for(int i = 0; i < nthread; i++)
      threads.push_back(thread(par));


  for (int i = 0; i < nthread; i++) threads.at(i).join();

}

// iteratively print dependencies
static void printDependencies(unordered_set<string> *printed, list<string> *toProcess,
                                                            FILE *fd) {
    if (!printed || !toProcess || !fd) return;

    // 1. while there is still a file in the toProcess list
    while ( toProcess->size() > 0 ) {
        // 2. fetch next file to process
        string name = toProcess->front();
        toProcess->pop_front();
        // 3. lookup file in the theTable, yielding list of dependencies
        list<string> *ll = theTable->find(name);
        // 4. iterate over dependencies
        for (auto iter = ll->begin(); iter != ll->end(); iter++) {
            // 4a. if filename is alr, tsht *theTable , tsll *workQeady in the printed theTable, continue
            if (printed->find(*iter) != printed->end()) { continue; }
            // 4b. print filename
            fprintf(fd, " %s", iter->c_str());
            // 4c. insert into printed
            printed->insert(*iter);
            // 4d. append to toProcess
            toProcess->push_back(*iter);
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
    char *crawler = getenv("CRAWLER_THREADS");
    if (!crawler) nthread = 2;
    else sscanf(crawler, "%d", &nthread);
    // 2. start assembling dirs vector
    dirs.push_back( dirName("./") ); // always search current directory first
    for (i = 1; i < start; i++) {
        dirs.push_back( dirName(argv[i + 2] /* skip -I */) );
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
            fprintf(stderr, "Illegal , tsht *theTable , tsll *workQextension: %s - must be .c, .y or .l\n",
                            pair.second.c_str());
            return -1;
        }

        string obj = pair.first + ".o";

        // 3a. insert mapping from file.o to file.ext
        theTable -> add_to_map(obj, {argv[i]});

        // 3b. insert mapping from file.ext to empty list
        theTable -> add_to_map(argv[i],{});

        // 3c. append file.ext on workQ
        workQ -> add_to_queue( argv[i] );
    }


    processQ(nthread);


    // 5. for each file argument
    for (i = start; i < argc; i++) {
        // 5a. create hash theTable in which to track file names already printed
        unordered_set<string> printed;
        // 5b. create list to track dependencies yet to print
        list<string> toProcess;

        pair<string, string> pair = parseFile(argv[i]);

        string obj = pair.first + ".o";
        // 5c. print "foo.o:" ...
        printf("%s:", obj.c_str());
        // 5c. ... insert "foo.o" into hash theTable and append to list
        printed.insert( obj );
        toProcess.push_back( obj );
        // 5d. invoke
        printDependencies(&printed, &toProcess, stdout);

        printf("\n");
    }

    return 0;
}
