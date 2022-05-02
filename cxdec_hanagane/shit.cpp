#include<Windows.h>
#include<vector>
#include <tjsTypes.h>
#include "tvpMisc.h"
#include "G:\\ChromeDownload\\gcc-10.2.0\\zlib\\zlib.h"
#include <algorithm>

//---------------------------------------------------------------------------
// TVPLocalExtrectFilePath
//---------------------------------------------------------------------------
ttstr TVPLocalExtractFilePath(const ttstr& name)
{
	// this extracts given name's path under local filename rule
	const tjs_char* p = name.c_str();
	tjs_int i = name.size() - 1;
	for (; i >= 0; i--)
	{
		if (p[i] == TJS_W(':') || p[i] == TJS_W('/') ||
			p[i] == TJS_W('\\'))
			break;
	}
	return ttstr(p, i + 1);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPLocalFileStream
//---------------------------------------------------------------------------
tTVPLocalFileStream::tTVPLocalFileStream(const ttstr& origname,
	const ttstr& localname, tjs_uint32 flag)
{
	Handle = INVALID_HANDLE_VALUE;

	tjs_uint32 access = flag & TJS_BS_ACCESS_MASK;

	DWORD dwcd;
	DWORD rw;
	switch (access)
	{
	case TJS_BS_READ:
		rw = GENERIC_READ;					dwcd = OPEN_EXISTING;		break;
	case TJS_BS_WRITE:
		rw = GENERIC_WRITE;					dwcd = CREATE_ALWAYS;		break;
	case TJS_BS_APPEND:
		rw = GENERIC_WRITE;					dwcd = OPEN_ALWAYS;			break;
	case TJS_BS_UPDATE:
		rw = GENERIC_WRITE | GENERIC_READ;	dwcd = OPEN_EXISTING;		break;
	}

	tjs_int trycount = 0;

retry:
	Handle = CreateFile(
		localname.c_str(),
		rw,
		FILE_SHARE_READ, // read shared accesss is strongly needed
		NULL,
		dwcd,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (Handle == INVALID_HANDLE_VALUE)
	{
		if (trycount == 0 && access == TJS_BS_WRITE)
		{
			trycount++;

			// retry after creating the folder
			if (TVPCreateFolders(TVPLocalExtractFilePath(localname)))
				goto retry;
		}
		//TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
	}

	if (access == TJS_BS_APPEND) // move the file pointer to last
		SetFilePointer(Handle, 0, NULL, FILE_END);

	// push current tick as an environment noise
	DWORD tick = GetTickCount();
	//TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
tTVPLocalFileStream::~tTVPLocalFileStream()
{
	if (Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle);

	// push current tick as an environment noise
	// (timing information from file accesses may be good noises)
	DWORD tick = GetTickCount();
	//TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPLocalFileStream::Seek(tjs_int64 offset, tjs_int whence)
{
	DWORD dwmm;
	switch (whence)
	{
	case TJS_BS_SEEK_SET:	dwmm = FILE_BEGIN;		break;
	case TJS_BS_SEEK_CUR:	dwmm = FILE_CURRENT;	break;
	case TJS_BS_SEEK_END:	dwmm = FILE_END;		break;
	default:				dwmm = FILE_BEGIN;		break; // may be enough
	}

	DWORD low;

	low = SetFilePointer(Handle, (LONG)offset, ((LONG*)&offset) + 1, dwmm);

	if (low == 0xffffffff && GetLastError() != NO_ERROR)
	{
		return TJS_UI64_VAL(0xffffffffffffffff);
	}
	tjs_uint64 ret;
	*(DWORD*)&ret = low;
	*((DWORD*)&ret + 1) = *((DWORD*)&offset + 1);

	return ret;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPLocalFileStream::Read(void* buffer, tjs_uint read_size)
{
	DWORD ret = 0;
	ReadFile(Handle, buffer, read_size, &ret, NULL);
	return ret;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPLocalFileStream::Write(const void* buffer, tjs_uint write_size)
{
	DWORD ret = 0;
	WriteFile(Handle, buffer, write_size, &ret, NULL);
	return ret;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPLocalFileStream::SetEndOfStorage()
{
	if (!SetEndOfFile(Handle))
		//TVPThrowExceptionMessage(TVPWriteError);
		;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPLocalFileStream::GetSize()
{
	tjs_uint64 ret;
	*(DWORD*)&ret = GetFileSize(Handle, ((DWORD*)&ret + 1));
	return ret;
}
//---------------------------------------------------------------------------
// TVPCheckExistantLocalFolder
//---------------------------------------------------------------------------
bool TVPCheckExistentLocalFolder(const ttstr& name)
{
	DWORD attrib = GetFileAttributes(name.c_str());
	if (attrib != 0xffffffff && (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return true; // a folder
	else
		return false; // not a folder
}
//---------------------------------------------------------------------------
// TVPCreateFolders
//---------------------------------------------------------------------------
static bool _TVPCreateFolders(const ttstr& folder)
{
	// create directories along with "folder"
	if (folder.size() == 0 ) return true;

	if (TVPCheckExistentLocalFolder(folder))
		return true; // already created

	const tjs_char* p = folder.c_str();
	tjs_int i = folder.size() - 1;

	if (p[i] == TJS_W(':')) return true;

	while (i >= 0 && (p[i] == TJS_W('/') || p[i] == TJS_W('\\'))) i--;

	if (i >= 0 && p[i] == TJS_W(':')) return true;

	for (; i >= 0; i--)
	{
		if (p[i] == TJS_W(':') || p[i] == TJS_W('/') ||
			p[i] == TJS_W('\\'))
			break;
	}

	ttstr parent(p, i + 1);

	if (!_TVPCreateFolders(parent)) return false;

	BOOL res = ::CreateDirectory(folder.c_str(), NULL);
	return 0 != res;
}

bool TVPCreateFolders(const ttstr& folder)
{
	if (folder.size() == 0) return true;

	const tjs_char* p = folder.c_str();
	tjs_int i = folder.size() - 1;

	if (p[i] == TJS_W(':')) return true;

	if (p[i] == TJS_W('/') || p[i] == TJS_W('\\')) i--;

	return _TVPCreateFolders(ttstr(p, i + 1));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

tTVPXP3Archive::tTVPXP3Archive(const ttstr& name) : tTVPArchive(name)
{
	Name = name;
	Count = 0;

	tjs_uint64 offset;

	tTJSBinaryStream* st = TVPCreateStream(name);

	tjs_uint8* indexdata = NULL;

	static tjs_uint8 cn_File[] =
	{ 0x46/*'F'*/, 0x69/*'i'*/, 0x6c/*'l'*/, 0x65/*'e'*/ };
	static tjs_uint8 cn_info[] =
	{ 0x69/*'i'*/, 0x6e/*'n'*/, 0x66/*'f'*/, 0x6f/*'o'*/ };
	static tjs_uint8 cn_segm[] =
	{ 0x73/*'s'*/, 0x65/*'e'*/, 0x67/*'g'*/, 0x6d/*'m'*/ };
	static tjs_uint8 cn_adlr[] =
	{ 0x61/*'a'*/, 0x64/*'d'*/, 0x6c/*'l'*/, 0x72/*'r'*/ };

	//TVPAddLog(TVPFormatMessage(TVPInfoTryingToReadXp3VirtualFileSystemInformationFrom, name));

	int segmentcount = 0;
	try
	{
		// retrieve archive offset
		TVPGetXP3ArchiveOffset(st, name, offset, true);

		// read index position and seek
		st->SetPosition(11 + offset);

		// read all XP3 indices
		while (true)
		{
			if (indexdata) delete[] indexdata;

			tjs_uint64 index_ofs = st->ReadI64LE();
			st->SetPosition(index_ofs + offset);

			// read index to memory
			tjs_uint8 index_flag;
			st->ReadBuffer(&index_flag, 1);
			tjs_uint index_size;

			if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) ==
				TVP_XP3_INDEX_ENCODE_ZLIB)
			{
				// compressed index
				tjs_uint64 compressed_size = st->ReadI64LE();
				tjs_uint64 r_index_size = st->ReadI64LE();

				if ((tjs_uint)compressed_size != compressed_size ||
					(tjs_uint)r_index_size != r_index_size)
					;//TVPThrowExceptionMessage(TVPReadError);
				// too large to handle, or corrupted
				index_size = (tjs_int)r_index_size;
				indexdata = new tjs_uint8[index_size];
				tjs_uint8* compressed = new tjs_uint8[(tjs_uint)compressed_size];
				try
				{
					st->ReadBuffer(compressed, (tjs_uint)compressed_size);

					unsigned long destlen = (unsigned long)index_size;

					int result = uncompress(  /* uncompress from zlib */
						(unsigned char*)indexdata,
						&destlen, (unsigned char*)compressed,
						(unsigned long)compressed_size);
					if (result != Z_OK ||
						destlen != (unsigned long)index_size)
						;//TVPThrowExceptionMessage(TVPUncompressionFailed);
				}
				catch (...)
				{
					delete[] compressed;
					throw;
				}
				delete[] compressed;
			}
			else if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) ==
				TVP_XP3_INDEX_ENCODE_RAW)
			{
				// uncompressed index
				tjs_uint64 r_index_size = st->ReadI64LE();
				if ((tjs_uint)r_index_size != r_index_size)
					//TVPThrowExceptionMessage(TVPReadError);
				// too large to handle or corrupted
				index_size = (tjs_uint)r_index_size;
				indexdata = new tjs_uint8[index_size];
				st->ReadBuffer(indexdata, index_size);
			}
			else
			{
				// unknown encode method
				//TVPThrowExceptionMessage(TVPReadError);
			}


			// read index information from memory
			tjs_uint ch_file_start = 0;
			tjs_uint ch_file_size = index_size;
			//Count = 0;
			for (;;)
			{
				// find 'File' chunk
				if (!FindChunk(indexdata, cn_File, ch_file_start, ch_file_size))
					break; // not found

				// find 'info' sub-chunk
				tjs_uint ch_info_start = ch_file_start;
				tjs_uint ch_info_size = ch_file_size;
				if (!FindChunk(indexdata, cn_info, ch_info_start, ch_info_size))
					//TVPThrowExceptionMessage(TVPReadError);

				// read info sub-chunk
					;
				tArchiveItem item;
				tjs_uint32 flags = ReadI32FromMem(indexdata + ch_info_start + 0);
				item.OrgSize = ReadI64FromMem(indexdata + ch_info_start + 4);
				item.ArcSize = ReadI64FromMem(indexdata + ch_info_start + 12);

				tjs_int len = ReadI16FromMem(indexdata + ch_info_start + 20);
				ttstr name = TVPStringFromBMPUnicode(
					(const tjs_uint16*)(indexdata + ch_info_start + 22), len);
				item.Name = name;
				NormalizeInArchiveStorageName(item.Name);

				// find 'segm' sub-chunk
				// Each of in-archive storages can be splitted into some segments.
				// Each segment can be compressed or uncompressed independently.
				// segments can share partial area of archive storage. ( this is used for
				// OggVorbis' VQ code book sharing )
				tjs_uint ch_segm_start = ch_file_start;
				tjs_uint ch_segm_size = ch_file_size;
				if (!FindChunk(indexdata, cn_segm, ch_segm_start, ch_segm_size))
					//TVPThrowExceptionMessage(TVPReadError);

				// read segm sub-chunk
				tjs_int segment_count = ch_segm_size / 28;
				tjs_uint64 offset_in_archive = 0;
				for (tjs_int i = 0; i < segment_count; i++)
				{
					tjs_uint pos_base = i * 28 + ch_segm_start;
					tTVPXP3ArchiveSegment seg;
					tjs_uint32 flags = ReadI32FromMem(indexdata + pos_base);

					if ((flags & TVP_XP3_SEGM_ENCODE_METHOD_MASK) ==
						TVP_XP3_SEGM_ENCODE_RAW)
						seg.IsCompressed = false;
					else if ((flags & TVP_XP3_SEGM_ENCODE_METHOD_MASK) ==
						TVP_XP3_SEGM_ENCODE_ZLIB)
						seg.IsCompressed = true;
					else;
						//TVPThrowExceptionMessage(TVPReadError); // unknown encode method

					seg.Start = ReadI64FromMem(indexdata + pos_base + 4) + offset;
					// data offset in archive
					seg.Offset = offset_in_archive; // offset in in-archive storage
					seg.OrgSize = ReadI64FromMem(indexdata + pos_base + 12); // original size
					seg.ArcSize = ReadI64FromMem(indexdata + pos_base + 20); // archived size
					item.Segments.push_back(seg);
					offset_in_archive += seg.OrgSize;
					segmentcount++;
				}

				// find 'aldr' sub-chunk
				tjs_uint ch_adlr_start = ch_file_start;
				tjs_uint ch_adlr_size = ch_file_size;
				if (!FindChunk(indexdata, cn_adlr, ch_adlr_start, ch_adlr_size))
					;//TVPThrowExceptionMessage(TVPReadError);

				// read 'aldr' sub-chunk
				item.FileHash = ReadI32FromMem(indexdata + ch_adlr_start);

				// push information
				ItemVector.push_back(item);

				// to next file
				ch_file_start += ch_file_size;
				ch_file_size = index_size - ch_file_start;
				Count++;
			}

			if (!(index_flag & TVP_XP3_INDEX_CONTINUE))
				break; // continue reading index when the bit sets
		}

		// sort item vector by its name (required for tTVPArchive specification)
		std::stable_sort(ItemVector.begin(), ItemVector.end());
	}
	catch (...)
	{
		if (indexdata) delete[] indexdata;
		delete st;
		//TVPAddLog((const tjs_char*)TVPInfoFailed);
		throw;
	}
	if (indexdata) delete[] indexdata;
	delete st;

	//TVPAddLog(TVPFormatMessage(TVPInfoDoneWithContains, ttstr(Count), ttstr(segmentcount)));
}
//---------------------------------------------------------------------------
tTVPXP3Archive::~tTVPXP3Archive()
{
	TVPFreeArchiveHandlePoolByPointer(this);
}
//---------------------------------------------------------------------------

static bool TVPGetXP3ArchiveOffset(tTJSBinaryStream* st, const ttstr name,
	tjs_uint64& offset, bool raise)
{
	st->SetPosition(0);
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
	st->ReadBuffer(mark, 11);
	if (mark[0] == 0x4d/*'M'*/ && mark[1] == 0x5a/*'Z'*/)
	{
		// "MZ" is a mark of Win32/DOS executables,
		// TVP searches the first mark of XP3 archive
		// in the executeble file.
		bool found = false;

		offset = 16;
		st->SetPosition(16);

		// XP3 mark must be aligned by a paragraph ( 16 bytes )
		const tjs_uint one_read_size = 256 * 1024;
		tjs_uint read;
		tjs_uint8 buffer[one_read_size]; // read 256kbytes at once

		while (0 != (read = st->Read(buffer, one_read_size)))
		{
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
//---------------------------------------------------------------------------
bool tTVPXP3Archive::FindChunk(const tjs_uint8* data, const tjs_uint8* name,
	tjs_uint& start, tjs_uint& size)
{
	tjs_uint start_save = start;
	tjs_uint size_save = size;

	tjs_uint pos = 0;
	while (pos < size)
	{
		bool found = !memcmp(data + start, name, 4);
		start += 4;
		tjs_uint64 r_size = ReadI64FromMem(data + start);
		start += 8;
		tjs_uint size_chunk = (tjs_uint)r_size;
		if (size_chunk != r_size)
			//TVPThrowExceptionMessage(TVPReadError);
		if (found)
		{
			// found
			size = size_chunk;
			return true;
		}
		start += size_chunk;
		pos += size_chunk + 4 + 8;
	}

	start = start_save;
	size = size_save;
	return false;
}
//---------------------------------------------------------------------------
tjs_int16 tTVPXP3Archive::ReadI16FromMem(const tjs_uint8* mem)
{
	tjs_uint16 ret = (tjs_uint16)mem[0] | ((tjs_uint16)mem[1] << 8);
	return (tjs_int16)ret;
}
//---------------------------------------------------------------------------
tjs_int32 tTVPXP3Archive::ReadI32FromMem(const tjs_uint8* mem)
{
	tjs_uint32 ret = (tjs_uint32)mem[0] | ((tjs_uint32)mem[1] << 8) |
		((tjs_uint32)mem[2] << 16) | ((tjs_uint32)mem[3] << 24);
	return (tjs_int32)ret;
}
//---------------------------------------------------------------------------
tjs_int64 tTVPXP3Archive::ReadI64FromMem(const tjs_uint8* mem)
{
	tjs_uint64 ret = (tjs_uint64)mem[0] | ((tjs_uint64)mem[1] << 8) |
		((tjs_uint64)mem[2] << 16) | ((tjs_uint64)mem[3] << 24) |
		((tjs_uint64)mem[4] << 32) | ((tjs_uint64)mem[5] << 40) |
		((tjs_uint64)mem[6] << 48) | ((tjs_uint64)mem[7] << 56);
	return (tjs_int64)ret;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// tTVPXP3ArchiveHandleCache
//---------------------------------------------------------------------------
#define TVP_MAX_ARCHIVE_HANDLE_CACHE 8
static tjs_uint TVPArchiveHandleCacheAge = 0;
struct tTVPArchiveHandleCacheItem
{
	void* Pointer;
	tTJSBinaryStream* Stream;
	tjs_uint Age;
};
//---------------------------------------------------------------------------
static tTVPArchiveHandleCacheItem* TVPArchiveHandleCachePool = NULL;
static bool TVPArchiveHandleCacheInit = false;
static bool TVPArchiveHandleCacheShutdown = false;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


class ReadXP3
{
private:
	unsigned int key;
	std::vector<unsigned int> index;
	unsigned int pointer;
	static BOOL ReadMagicHead(LPCWSTR _address)
	{

	}
	static BOOL ReadTo(DWORD* _buffer,LPCWSTR _address)
	{

	}
	
public:

};