#include <cstdlib>
#include <vector>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

extern "C"
{
#include "7z/LzmaEnc.h"
#include "7z/LzmaDec.h"
}


#ifdef WIN32
    #include <direct.h>
    #include <malloc.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#define VER         "0.3"

int decomp(char * file, char* directory);
int comp(char * file, char* directory);
void std_err(void);
void io_err(void);

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

int main(int argc, char *argv[]) {
  
    setbuf(stdout, NULL);
    fputs("\n"
    "MUFFDIVER "VER":\n"
	"a simple LZMA based file archiver\n"
    "by mudlord\n"
    "e-mail: pf4spthc@gmail.com\n"
    "web:    mudlord.emuxhaven.net\n"
    "\n", stdout);

	if(argc < 3) {
		printf("\nUsage: %s -c <file.muff> <source_directory>\n"
			   "Otherwise it unpacks.\n",
		argv[0]);
		exit(1);
	}

	if (strcmp("-c", argv[1]) == 0)
	{
		comp(argv[2],argv[3]);
	}
	else
	{
		decomp(argv[1], argv[2]);
	}
		
	return(0);
   
}

BYTE *Load_Input_File(char *FileName, DWORD *Size)
{
	BYTE *Memory;
	FILE *Input = fopen(FileName, "rb");
	if(!Input) return(NULL);
	// Get the filesize
	fseek(Input, 0, SEEK_END);
	*Size = ftell(Input);
	fseek(Input, 0, SEEK_SET);
	Memory = (BYTE *) malloc(*Size);
	if(!Memory) return(NULL);
	if(fread(Memory, 1, *Size, Input) != (size_t) *Size) return(NULL);
	if(Input) fclose(Input);
	Input = NULL;
	return(Memory);
}

void* Alloc(void *p, size_t size) { return malloc(size); }
void Free(void *p, void *address) { if (address) free(address); }
ISzAlloc alloc = { Alloc, Free };

int decomp (char * file, char * directory)
{
	std::vector<filez> entries;
	int             err, num_files;
	FILE *fd = fopen(file, "rb");
	if(!fd) std_err();

	err = fread(&head, sizeof(head), 1, fd);
	if(err != 1) io_err();
	if(head.signature != 0x12111988) {
		fputs("\nError: the archive is not a valid file\n", stdout);
		exit(1);
	}
	printf("\n"
		"Total files:     %u\n"
		"Signature:       0x%08x\n",
		head.fileznum,
		head.signature);
	num_files = head.fileznum;

	fseek(fd, sizeof(head), SEEK_SET);

	for (int i = 0; i < head.fileznum; i++)
	{
		filez entry;
		fread(&entry, sizeof(filez), 1, fd);
		entries.push_back(entry);
	}

	err = chdir( directory);
	if(err < 0) std_err();

	for (int j=0;j<num_files;j++)
	{ 
		filez &entry = entries[j];
		unsigned char * data = (unsigned char*)malloc(entry.compressed_len);
		memset(data,0,entry.compressed_len);
		unsigned char * out_mem = (unsigned char*)malloc(entry.uncompressed_len);
		memset(out_mem,0,entry.uncompressed_len);
		rewind(fd);
		fseek(fd, entry.off, SEEK_SET);
		fread(data,entry.compressed_len,1,fd);

		unsigned int slen = entry.compressed_len;
		unsigned int destLen = entry.uncompressed_len;
		ELzmaStatus status;
		slen -= LZMA_PROPS_SIZE;
		SRes err = LzmaDecode((Byte*)out_mem, (SizeT*)&destLen, (const Byte*)data + LZMA_PROPS_SIZE, 
		(SizeT*)&slen, (const Byte*)data, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &alloc);
		if ( err != SZ_OK && destLen != entry.uncompressed_len )
		{
			printf("File unpacking failed!\n");
			return 0;
		}

		printf("Unpacked File: %s (%d bytes at %08X)...\n", entry.fname, entry.uncompressed_len,  entry.off);
		FILE *fdout = fopen((const char*)entry.fname, "wb");
		fwrite(out_mem,entry.uncompressed_len,1,fdout);
		free(out_mem);
		free(data);
		fclose(fdout);
	}
	printf("Files unpacked successfully!\n");
	
	fclose(fd);
}


int comp(char * file, char* directory)
{
	FILE *fd,*fdout;
	int  err;
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	static int      winnt = -1;
	OSVERSIONINFO   osver;
	DWORD dwError=0;
	int num_files = 0;
	std::vector<filez> entries;
	filez file_struct;

	strcpy(szDir, directory);
	strcat(szDir,"\\*.*");

	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		printf("Can't find files in directory!\n");
		return dwError;
	} 

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		}
		else
		{
			filez entry;
			memset(entry.fname,0,64);
			entry.uncompressed_len = 0xB00BFACE;
			entry.compressed_len = 0xB00BFACE;
			entry.off = 0xFEEDFACE;
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			memcpy(entry.fname,ffd.cFileName,64);
			entries.push_back(entry);
			num_files++;
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	//write archive header
	fd = fopen(file, "wb");
	if(!fd) std_err();
	head.fileznum = num_files;
	head.signature = 0x12111988;
	err = fwrite(&head, sizeof(head), 1, fd);
	if(err != 1) io_err();

	//prepping file headers
	for (int i = 0; i< num_files; i++)
	{
		memset(file_struct.fname,0,64);
		file_struct.compressed_len = 0xB00BFACE;
		file_struct.uncompressed_len = 0xB00BFACE;
		file_struct.off = 0xFEEDFACE;
		err = fwrite(&file_struct,sizeof(filez), 1, fd);
	}

	err = chdir(directory);
	if(err < 0) std_err();
	for (int j=0;j<num_files;j++)
	{
		filez &entry = entries[j];
		BYTE *Input_Mem;
		DWORD Input_Size;
		unsigned int Packed_Size;
		DWORD cur_pos, file_pos;
		cur_pos = ftell(fd);
		Input_Mem = Load_Input_File((char*)entry.fname, &Input_Size);
		BYTE *Packed_Mem = (BYTE *) malloc(Input_Size * 2);
		memset(Packed_Mem, 0, Input_Size * 2);
		Packed_Size = Input_Size * 2;

		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		props.level = 6;
		props.fb = 128;
		props.algo = 1;
		props.numThreads = 4;
		SizeT s = LZMA_PROPS_SIZE;
		SRes err = LzmaEncode((Byte*)Packed_Mem + LZMA_PROPS_SIZE, (SizeT*)&Packed_Size, 
		(Byte*)Input_Mem, Input_Size, &props, (Byte*)Packed_Mem, &s, 1, NULL, &alloc, &alloc);
		Packed_Size += LZMA_PROPS_SIZE;
		if ( err != SZ_OK )
		{
			free(Input_Mem);
			free(Packed_Mem);
			fclose(fd);
			printf("file packing failed!\n");
			return 1;
		}

		printf("Packed %s (%d of %d bytes at %08X)...\n", entry.fname, Packed_Size,Input_Size, cur_pos);
		fwrite(Packed_Mem,Packed_Size,1,fd);
		file_pos = ftell(fd);
		free(Input_Mem);
		free(Packed_Mem);
		//fix the file headers
		rewind(fd);
		fseek(fd, sizeof(head) + j * sizeof(filez), SEEK_SET);
		memset(file_struct.fname,0,64);
		memcpy(file_struct.fname,entry.fname,strlen((const char*)entry.fname));
		file_struct.uncompressed_len = Input_Size;
		file_struct.compressed_len = Packed_Size;
		file_struct.off = cur_pos;
		fwrite(&file_struct,sizeof(filez),1,fd);
		fseek(fd,file_pos,SEEK_SET);
	}

	printf("Files packed successfully!\n");
	fclose(fd);
	return 0;
}


void std_err(void) {
    perror("\nError");
    exit(1);
}

void io_err(void) {
    fputs("\nError: I/O error, the file is incomplete or the disk space is finished\n", stdout);
    exit(1);
}