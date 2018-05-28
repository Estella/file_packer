#include <cstdlib>
#include <vector>

using namespace std;
#include <fstream>

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <malloc.h>

extern "C"
{
#include "LzmaDec.h"
}

class muff_unpack
{
	struct head {
		DWORD  signature; //file signature
		DWORD  fileznum;    //number of files
	} head;
	struct filez{
		unsigned char  fname[64]; //filename
		DWORD   compressed_len;       // filedata length (compressed)
		DWORD   uncompressed_len;       // filedata length (compressed)
		DWORD   off;       // file offset
	};
	vector<filez> entries;
	fstream  archive_file;
	char arc_file[MAX_PATH];
public:
	muff_unpack();
	~muff_unpack();
	BYTE * decomp_entry(char * file, int *size);
	int init(char* archive);
	void close();
};




