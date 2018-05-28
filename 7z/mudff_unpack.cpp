#include "mudff_unpack.h"

void* Alloc(void *p, size_t size) { return malloc(size); }
void Free(void *p, void *address) { if (address) free(address); }
ISzAlloc alloc = { Alloc, Free };


muff_unpack::muff_unpack()
{
	memset(arc_file,0,MAX_PATH);
}
muff_unpack::~muff_unpack()
{
	memset(arc_file,0,MAX_PATH);
}

BYTE * muff_unpack::decomp_entry(char * file, int *size)
{
	for (int j=0;j<head.fileznum;j++)
	{ 
		filez &entry = entries[j];
		if(strcmp(file,(const char*)entry.fname) ==0)
		{
			unsigned char * data = (unsigned char*)malloc(entry.compressed_len);
			memset(data,0,entry.compressed_len);
			unsigned char * out_mem = (unsigned char*)malloc(entry.uncompressed_len);
			memset(out_mem,0,entry.uncompressed_len);
			archive_file.seekg(0,ios::beg);
			archive_file.seekg (entry.off, ios::beg);
			archive_file.read ((char*)data,entry.compressed_len);
			unsigned int slen = entry.compressed_len;
			unsigned int destLen = entry.uncompressed_len;
			ELzmaStatus status;
			slen -= LZMA_PROPS_SIZE;
			SRes err = LzmaDecode((Byte*)out_mem, (SizeT*)&destLen, (const Byte*)data + LZMA_PROPS_SIZE, 
				(SizeT*)&slen, (const Byte*)data, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &alloc);
			if ( err != SZ_OK && destLen != entry.uncompressed_len )
			{
				return NULL;
			}
			*size = entry.uncompressed_len;
			delete [] data;
			return((BYTE*)out_mem);
		}
		
	}
	return NULL;
}

int muff_unpack::init(char* archive)
	{
	int num_files;
	archive_file.open (archive, ios::in|ios::binary);
	if (!archive_file.is_open()) return 0;
	strcpy(arc_file,archive);
	archive_file.seekg (0, ios::beg);
	archive_file.read ((char*)&head, sizeof(head));
	if(head.signature != 0x12111988) {
		return 0;
	}
	num_files = head.fileznum;
	archive_file.seekg (sizeof(head), ios::beg);
	for (int i = 0; i < head.fileznum; i++)
	{
		filez entry;
		archive_file.read ((char*)&entry, sizeof(filez));
		entries.push_back(entry);
	}
	return 1;
}

void muff_unpack::close()
{
	memset(arc_file,0,MAX_PATH);
	archive_file.close();
}




