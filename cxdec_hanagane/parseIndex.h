#pragma once
//---------------------------------------------------------------------------
#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1

#define TVP_XP3_INDEX_CONTINUE   0x80

#define TVP_XP3_FILE_PROTECTED (1<<31)

#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1


#include<iostream>
#include<Windows.h>
#include<string>
#include "G:\\ChromeDownload\\gcc-10.2.0\\zlib\\zlib.h"
#include"tjsTypes.h"
#include <vector>
#include"tjsString.hpp"

struct tTVPXP3ArchiveSegment
{
	tjs_uint64 Start;  // start position in archive storage
	tjs_uint64 Offset; // offset in in-archive storage (in uncompressed offset)
	tjs_uint64 OrgSize; // original segment (uncompressed) size
	tjs_uint64 ArcSize; // in-archive segment (compressed) size
	bool IsCompressed; // is compressed ?
};
struct tArchiveItem
{
	ttstr Name;
	tjs_uint32 FileHash;
	tjs_uint64 OrgSize; // original ( uncompressed ) size
	tjs_uint64 ArcSize; // in-archive size
	std::vector<tTVPXP3ArchiveSegment> Segments;
	bool operator < (const tArchiveItem& rhs) const
	{
		return this->Name < rhs.Name;
	}
};


class tTVPXP3Index
{
private:
	//HANDLE handle;
	LPCWSTR _filename;
	tjs_uint64 _index_offset = 0;
	tjs_uint64 _offset = 0;
	int segmentcount = 0;
	std::vector<tArchiveItem> ItemVector;
	tjs_int Count;

	tjs_uint8 cn_File[4] =
	{ 0x46/*'F'*/, 0x69/*'i'*/, 0x6c/*'l'*/, 0x65/*'e'*/ };
	tjs_uint8 cn_info[4] =
	{ 0x69/*'i'*/, 0x6e/*'n'*/, 0x66/*'f'*/, 0x6f/*'o'*/ };
	tjs_uint8 cn_segm[4] =
	{ 0x73/*'s'*/, 0x65/*'e'*/, 0x67/*'g'*/, 0x6d/*'m'*/ };
	tjs_uint8 cn_adlr[4] =
	{ 0x61/*'a'*/, 0x64/*'d'*/, 0x6c/*'l'*/, 0x72/*'r'*/ };

	tjs_uint64 ReadI64LE(HANDLE handle);
	void ReadBuffer(HANDLE handle, void* _buffer, tjs_uint _size);

	tjs_int64 ReadI64FromMem(const tjs_uint8* mem);
	tjs_int32 ReadI32FromMem(const tjs_uint8* mem);
	tjs_int16 ReadI16FromMem(const tjs_uint8* mem);

	bool FindChunk(const tjs_uint8* data, const tjs_uint8* name, tjs_uint& start, tjs_uint& size);

	ttstr TVPStringFromBMPUnicode(const tjs_uint16* src, tjs_int maxlen);
	void NormalizeInArchiveStorageName(ttstr& name);
public:
	tjs_uint64 FindIndex();
	std::vector<tArchiveItem>* GetIndex();
	tTVPXP3Index();
	tTVPXP3Index(const LPCWSTR& _filename);
	~tTVPXP3Index();
};