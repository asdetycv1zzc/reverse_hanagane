#pragma once
#define TJS_BS_SEEK_SET 0
#define TJS_BS_SEEK_CUR 1
#define TJS_BS_SEEK_END 2
//---------------------------------------------------------------------------
#define TJS_BS_READ 0
#define TJS_BS_WRITE 1
#define TJS_BS_APPEND 2
#define TJS_BS_UPDATE 3

#define TJS_BS_ACCESS_MASK 0x0f

#define TJS_BS_SEEK_SET 0
#define TJS_BS_SEEK_CUR 1
#define TJS_BS_SEEK_END 2
//---------------------------------------------------------------------------
#define TJS_HS_DEFAULT_HASH_SIZE 64
#define TJS_HS_HASH_USING	0x1
#define TJS_HS_HASH_LV1	0x2
//---------------------------------------------------------------------------
// tTVPXP3Archive  : XP3 ( TVP's native archive format ) Implmentation
//---------------------------------------------------------------------------
#define TVP_XP3_INDEX_ENCODE_METHOD_MASK 0x07
#define TVP_XP3_INDEX_ENCODE_RAW      0
#define TVP_XP3_INDEX_ENCODE_ZLIB     1

#define TVP_XP3_INDEX_CONTINUE   0x80

#define TVP_XP3_FILE_PROTECTED (1<<31)

#define TVP_XP3_SEGM_ENCODE_METHOD_MASK  0x07
#define TVP_XP3_SEGM_ENCODE_RAW       0
#define TVP_XP3_SEGM_ENCODE_ZLIB      1

#define TVP_ATEXIT_PRI_PREPARE    10
#define TVP_ATEXIT_PRI_SHUTDOWN   100
#define TVP_ATEXIT_PRI_RELEASE    1000
#define TVP_ATEXIT_PRI_CLEANUP    10000

/*[*/
#define TJS_strcmp			wcscmp
#define TJS_strncmp			wcsncmp
#define TJS_strncpy			wcsncpy
#define TJS_strcat			wcscat
#define TJS_strstr			wcsstr
#define TJS_strchr			wcschr
#define TJS_malloc			malloc
#define TJS_free			free
#define TJS_realloc			realloc
#define TJS_nsprintf		sprintf
#define TJS_nstrcpy			strcpy
#define TJS_nstrcat			strcat
#define TJS_nstrlen			strlen
#define TJS_nstrstr			strstr
#define TJS_strftime		wcsftime
#define TJS_vfprintf		vfwprintf
#define TJS_octetcpy		memcpy
#define TJS_octetcmp		memcmp
#define TJS_strtod			wcstod
/*]*/

#include<iostream>
#include<string>
#include<fstream>
#include<algorithm>
#include<vector>
#include"C:\\Users\\John\\source\\repos\\krkrz\\cxdec_hanagane\\tjsTypes.h"
#include<Windows.h>
#include "G:\\ChromeDownload\\gcc-10.2.0\\zlib\\zlib.h"

typedef std::wstring ttstr;

tjs_char  TVPArchiveDelimiter = '>';
inline static bool TVPGetXP3ArchiveOffset(std::wfstream* st, const ttstr name, tjs_uint64& offset, bool raise)
{
	st->seekp(0);
	tjs_uint8 mark[11 + 1];
	static tjs_uint8 XP3Mark1[] =
	{ 0x58/*'X'*/, 0x50/*'P'*/, 0x33/*'3'*/, 0x0d/*'\r'*/,
		0x0a/*'\n'*/, 0x20/*' '*/, 0x0a/*'\n'*/, 0x1a/*EOF*/,
		0xff /* sentinel */ };
	static tjs_uint8 XP3Mark2[] =
	{ 0x8b, 0x67, 0x01, 0xff/* sentinel */ };

	// XP3 header mark contains:
	// 1. line feed and carriage return to detect corruption by unnecessary
	//    line-feeds convertion
	// 2. 1A EOF mark which indicates file's text readable header ending.
	// 3. 8B67 KANJI-CODE to detect curruption by unnecessary code convertion
	// 4. 01 file structure version and character coding
	//    higher 4 bits are file structure version, currently 0.
	//    lower 4 bits are character coding, currently 1, is BMP 16bit Unicode.

	static tjs_uint8 XP3Mark[11 + 1];
	// +1: I was warned by CodeGuard that the code will do
	// access overrun... because a number of 11 is not aligned by DWORD, 
	// and the processor may read the value of DWORD at last of this array
	// from offset 8. Then the last 1 byte would cause a fail.
	static bool DoInit = true;
	if (DoInit)
	{
		// the XP3 header above is splitted into two part; to avoid
		// mis-finding of the header in the program's initialized data area.
		DoInit = false;
		memcpy(XP3Mark, XP3Mark1, 8);
		memcpy(XP3Mark + 8, XP3Mark2, 3);
		// here joins it.
	}

	mark[0] = 0; // sentinel
	st->read((wchar_t*)mark, 12);
	//st->ReadBuffer(mark, 11);
	if (mark[0] == 0x4d/*'M'*/ && mark[1] == 0x5a/*'Z'*/)
	{
		// "MZ" is a mark of Win32/DOS executables,
		// TVP searches the first mark of XP3 archive
		// in the executeble file.
		bool found = false;

		offset = 16;
		st->seekp(16);

		// XP3 mark must be aligned by a paragraph ( 16 bytes )
		const tjs_uint one_read_size = 256 * 1024;
		tjs_uint read;
		tjs_uint8 buffer[one_read_size]; // read 256kbytes at once

		while (!st->eof())
		{
			st->read((wchar_t*)buffer, one_read_size);
			tjs_uint p = 0;
			while (p < read)
			{
				if (!memcmp(XP3Mark, buffer + p, 11))
				{
					// found the mark
					offset += p;
					found = true;
					break;
				}
				p += 16;
			}
			if (found) break;
			offset += one_read_size;
		}

		if (!found)
		{
			if (raise)
				//TVPThrowExceptionMessage(TVPCannotUnbindXP3EXE, name);
				;
			else
				return false;
		}
	}
	else if (!memcmp(XP3Mark, mark, 11))
	{
		// XP3 mark found
		offset = 0;
	}
	else
	{
		if (raise)
			//TVPThrowExceptionMessage(TVPCannotFindXP3Mark, name);
			return false;
	}

	return true;
}
class tvpstr
{
private:
	std::wstring _base;
public:
	bool IsEmpty();
	tvpstr Independ();
	tjs_char operator[](tjs_uint a);
};
struct tTVPXP3ArchiveSegment
{
	tjs_uint64 Start;  // start position in archive storage
	tjs_uint64 Offset; // offset in in-archive storage (in uncompressed offset)
	tjs_uint64 OrgSize; // original segment (uncompressed) size
	tjs_uint64 ArcSize; // in-archive segment (compressed) size
	bool IsCompressed; // is compressed ?
};
class tTJSBinaryStream
{
private:
	HANDLE Handle;
public:
	//-- must implement
	virtual tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence) = 0;
	/* if error, position is not changed */

//-- optionally to implement
	virtual tjs_uint TJS_INTF_METHOD Read(void* buffer, tjs_uint read_size) = 0;
	/* returns actually read size */

	virtual tjs_uint TJS_INTF_METHOD Write(const void* buffer, tjs_uint write_size) = 0;
	/* returns actually written size */

	virtual void TJS_INTF_METHOD SetEndOfStorage();
	// the default behavior is raising a exception
	/* if error, raises exception */

//-- should re-implement for higher performance
	virtual tjs_uint64 TJS_INTF_METHOD GetSize() = 0;

	virtual ~tTJSBinaryStream() { ; }

	tjs_uint64 GetPosition();

	void SetPosition(tjs_uint64 pos);

	void ReadBuffer(void* buffer, tjs_uint read_size);
	void WriteBuffer(const void* buffer, tjs_uint write_size);

	tjs_uint64 ReadI64LE(); // reads little-endian integers
	tjs_uint32 ReadI32LE();
	tjs_uint16 ReadI16LE();
};
class ReadXP3
{
private:
	ttstr Name;
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

	tjs_int Count;

	std::vector<tArchiveItem> ItemVector;

	//tTJSBinaryStream* CreateStreamByIndex(tjs_uint idx);
	static bool FindChunk(const tjs_uint8* data, const tjs_uint8* name,
		tjs_uint& start, tjs_uint& size);
	static tjs_int16 ReadI16FromMem(const tjs_uint8* mem);
	static tjs_int32 ReadI32FromMem(const tjs_uint8* mem);
	static tjs_int64 ReadI64FromMem(const tjs_uint8* mem);
public:
	ReadXP3(const ttstr& name);
	~ReadXP3();
	//static tTJSBinaryStream* TVPCreateStream(const ttstr& name);
	tjs_uint GetCount() { return Count; }
	const ttstr& GetName(tjs_uint idx) const { return ItemVector[idx].Name; }
	tjs_uint32 GetFileHash(tjs_uint idx) const { return ItemVector[idx].FileHash; }
	ttstr GetName(tjs_uint idx) { return ItemVector[idx].Name; }

	const ttstr& GetName() const { return Name; }

	//tTJSBinaryStream* CreateStreamByIndex(tjs_uint idx);

};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void TVPAddAtExitHandler(tjs_int pri, void (*handler)());

struct tTVPArchiveHandleCacheItem
{
	void* Pointer;
	tTJSBinaryStream* Stream;
	tjs_uint Age;
};
struct tTVPAtExitInfo
{
	tTVPAtExitInfo(tjs_int pri, void(*handler)())
	{
		Priority = pri, Handler = handler;
	}

	tjs_int Priority;
	void (*Handler)();

	bool operator <(const tTVPAtExitInfo& r) const
	{
		return this->Priority < r.Priority;
	}
	bool operator >(const tTVPAtExitInfo& r) const
	{
		return this->Priority > r.Priority;
	}
	bool operator ==(const tTVPAtExitInfo& r) const
	{
		return this->Priority == r.Priority;
	}

};
struct tTVPAtExit
{
	tTVPAtExit(tjs_int pri, void (*handler)())
	{
		TVPAddAtExitHandler(pri, handler);
	}
};
class tTVPSegmentData
{
	tjs_int RefCount;
	tjs_uint Size;
	tjs_uint8* Data;

public:
	tTVPSegmentData() { RefCount = 1; Size = 0; Data = NULL; }
	~tTVPSegmentData() { if (Data) delete[] Data; }

	void SetData(unsigned long outsize, tTJSBinaryStream* instream,
		unsigned long insize)
	{
		// uncompress data
		tjs_uint8* indata = new tjs_uint8[insize];
		try
		{
			instream->Read(indata, insize);

			Data = new tjs_uint8[outsize];
			unsigned long destlen = outsize;
			int result = uncompress((unsigned char*)Data, &outsize,
				(unsigned char*)indata, insize);
			if (result != Z_OK || destlen != outsize)
				;//TVPThrowExceptionMessage(TVPUncompressionFailed);
			Size = outsize;
		}
		catch (...)
		{
			delete[] indata;
			throw;
		}
		delete[] indata;
	}

	const tjs_uint8* GetData() const { return Data; }
	tjs_uint GetSize() const { return Size; }

	void AddRef() { RefCount++; }
	void Release()
	{
		if (RefCount == 1)
		{
			delete this;
		}
		else
		{
			RefCount--;
		}
	}
};
class tTVPXP3ArchiveStream : public tTJSBinaryStream
{
	ReadXP3* Owner;

	tjs_int StorageIndex; // index in archive

	std::vector<tTVPXP3ArchiveSegment>* Segments;
	tTJSBinaryStream* Stream;
	tjs_uint64 OrgSize; // original storage size

	tjs_int CurSegmentNum;
	tTVPXP3ArchiveSegment* CurSegment;
	// currently opened segment ( NULL for not opened )

	tjs_int LastOpenedSegmentNum;

	tjs_uint64 CurPos; // current position in absolute file position

	tjs_uint64 SegmentRemain; // remain bytes in current segment
	tjs_uint64 SegmentPos; // offset from current segment's start

	tTVPSegmentData* SegmentData; // uncompressed segment data

	bool SegmentOpened;

public:
	tTVPXP3ArchiveStream(ReadXP3* owner, tjs_int storageindex,
		std::vector<tTVPXP3ArchiveSegment>* segments, tTJSBinaryStream* stream,
		tjs_uint64 orgsize);
	~tTVPXP3ArchiveStream();

private:
	void EnsureSegment(); // ensure accessing to current segment
	void SeekToPosition(tjs_uint64 pos); // open segment at 'pos' and seek
	bool OpenNextSegment();


public:
	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);
	tjs_uint TJS_INTF_METHOD Read(void* buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void* buffer, tjs_uint write_size);
	tjs_uint64 TJS_INTF_METHOD GetSize();

};
