#include<iostream>
#include<vector>
#include<string>
#include<fstream>
#include"G:/ChromeDownload/gcc-10.2.0/zlib/zlib.h"
#include"readXP3.h"
#include <algorithm>
#include<windows.h>
using namespace std;

void NormalizeInArchiveStorageName(ttstr& name)
{

}
//---------------------------------------------------------------------------
bool ReadXP3::FindChunk(const tjs_uint8* data, const tjs_uint8* name,
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
tjs_int16 ReadXP3::ReadI16FromMem(const tjs_uint8* mem)
{
	tjs_uint16 ret = (tjs_uint16)mem[0] | ((tjs_uint16)mem[1] << 8);
	return (tjs_int16)ret;
}
//---------------------------------------------------------------------------
tjs_int32 ReadXP3::ReadI32FromMem(const tjs_uint8* mem)
{
	tjs_uint32 ret = (tjs_uint32)mem[0] | ((tjs_uint32)mem[1] << 8) |
		((tjs_uint32)mem[2] << 16) | ((tjs_uint32)mem[3] << 24);
	return (tjs_int32)ret;
}
//---------------------------------------------------------------------------
tjs_int64 ReadXP3::ReadI64FromMem(const tjs_uint8* mem)
{
	tjs_uint64 ret = (tjs_uint64)mem[0] | ((tjs_uint64)mem[1] << 8) |
		((tjs_uint64)mem[2] << 16) | ((tjs_uint64)mem[3] << 24) |
		((tjs_uint64)mem[4] << 32) | ((tjs_uint64)mem[5] << 40) |
		((tjs_uint64)mem[6] << 48) | ((tjs_uint64)mem[7] << 56);
	return (tjs_int64)ret;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// tTJSBinaryStream
//---------------------------------------------------------------------------

void TJS_INTF_METHOD tTJSBinaryStream::SetEndOfStorage()
{
	//TJS_eTJSError(TJSWriteError);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTJSBinaryStream::GetSize()
{
	tjs_uint64 orgpos = GetPosition();
	tjs_uint64 size = Seek(0, TJS_BS_SEEK_END);
	Seek(orgpos, SEEK_SET);
	return size;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream::GetPosition()
{
	return Seek(0, SEEK_CUR);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::SetPosition(tjs_uint64 pos)
{
	if (pos != Seek(pos, TJS_BS_SEEK_SET));
		//TJS_eTJSError(TJSSeekError);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::ReadBuffer(void* buffer, tjs_uint read_size)
{
	if (Read(buffer, read_size) != read_size);
		//TJS_eTJSError(TJSReadError);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::WriteBuffer(const void* buffer, tjs_uint write_size)
{
	if (Write(buffer, write_size) != write_size);
		//TJS_eTJSError(TJSWriteError);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTJSBinaryStream::Read(void* buffer, tjs_uint read_size)
{
	DWORD ret = 0;
	ReadFile(Handle, buffer, read_size, &ret, NULL);
	return ret;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTJSBinaryStream::Write(const void* buffer, tjs_uint write_size)
{
	DWORD ret = 0;
	WriteFile(Handle, buffer, write_size, &ret, NULL);
	return ret;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream::ReadI64LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[8];
	ReadBuffer(buffer, 8);
	tjs_uint64 ret = 0;
	for (tjs_int i = 0; i < 8; i++)
		ret += (tjs_uint64)buffer[i] << (i * 8);
	return ret;
#else
	tjs_uint64 temp;
	ReadBuffer(&temp, 8);
	return temp;
#endif
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSBinaryStream::ReadI32LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[4];
	ReadBuffer(buffer, 4);
	tjs_uint32 ret = 0;
	for (tjs_int i = 0; i < 4; i++)
		ret += (tjs_uint32)buffer[i] << (i * 8);
	return ret;
#else
	tjs_uint32 temp;
	ReadBuffer(&temp, 4);
	return temp;
#endif
}
//---------------------------------------------------------------------------
tjs_uint16 tTJSBinaryStream::ReadI16LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[2];
	ReadBuffer(buffer, 2);
	tjs_uint16 ret = 0;
	for (tjs_int i = 0; i < 2; i++)
		ret += (tjs_uint16)buffer[i] << (i * 8);
	return ret;
#else
	tjs_uint16 temp;
	ReadBuffer(&temp, 2);
	return temp;
#endif
}
//---------------------------------------------------------------------------
#define TVP_MAX_ARCHIVE_HANDLE_CACHE 8
static tTVPArchiveHandleCacheItem* TVPArchiveHandleCachePool = NULL;
static bool TVPArchiveHandleCacheInit = false;
static bool TVPArchiveHandleCacheShutdown = false;
//static tTJSCriticalSection TVPArchiveHandleCacheCS;
//---------------------------------------------------------------------------
// TVPCreateStream
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// TVPPreNormalizeStorageName
//---------------------------------------------------------------------------
void TVPPreNormalizeStorageName(ttstr& name)
{
	// if the name is an OS's native expression, change it according with the
	// TVP storage system naming rule.
	tjs_int namelen = name.size();
	if (namelen == 0) return;

	if (namelen >= 2)
	{
		if ((name[0] >= TJS_W('a') && name[0] <= TJS_W('z') ||
			name[0] >= TJS_W('A') && name[0] <= TJS_W('Z')) &&
			name[1] == TJS_W(':'))
		{
			// Windows drive:path expression
			ttstr newname(TJS_W("file://./"));
			newname += name[0];
			newname += (name.c_str() + 2);
			name = newname;
			return;
		}
	}

	if (namelen >= 3)
	{
		if (name[0] == TJS_W('\\') && name[1] == TJS_W('\\') ||
			name[0] == TJS_W('/') && name[1] == TJS_W('/'))
		{
			// unc expression
			name = ttstr(TJS_W("file:")) + name;
			return;
		}
	}
}
//---------------------------------------------------------------------------
/*
static tTJSBinaryStream* TVPGetCachedArchiveHandle(void* pointer, const ttstr& name)
{
	// get cached archive file handle from the pool
	if (TVPArchiveHandleCacheShutdown)
	{
		// the pool has shutdown
		return ReadXP3::TVPCreateStream(name);
	}

	//tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

	if (!TVPArchiveHandleCacheInit)
	{
		// initialize the pool
		TVPArchiveHandleCachePool =
			new tTVPArchiveHandleCacheItem[TVP_MAX_ARCHIVE_HANDLE_CACHE];
		for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
		{
			TVPArchiveHandleCachePool[i].Pointer = NULL;
			TVPArchiveHandleCachePool[i].Stream = NULL;
			TVPArchiveHandleCachePool[i].Age = 0;
		}
		TVPArchiveHandleCacheInit = true;
	}

	// linear search wiil be enough here because the 
	// TVP_MAX_ARCHIVE_HANDLE_CACHE is relatively small
	for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
	{
		tTVPArchiveHandleCacheItem* item =
			TVPArchiveHandleCachePool + i;
		if (item->Stream && item->Pointer == pointer)
		{
			// found in the pool
			tTJSBinaryStream* stream = item->Stream;
			item->Stream = NULL;
			return stream;
		}
	}

	// not found in the pool
	// simply create a stream and return it
	return ReadXP3::TVPCreateStream(name);
}
*/
//---------------------------------------------------------------------------
static tjs_uint TVPArchiveHandleCacheAge = 0;
static void TVPReleaseCachedArchiveHandle(void* pointer,
	tTJSBinaryStream* stream)
{
	// release archive file handle
	if (TVPArchiveHandleCacheShutdown) return;
	if (!TVPArchiveHandleCacheInit) return;

	//tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

	// search empty cell in the pool
	tjs_uint oldest_age = 0;
	tjs_int oldest = 0;
	for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
	{
		tTVPArchiveHandleCacheItem* item =
			TVPArchiveHandleCachePool + i;
		if (item->Stream == NULL)
		{
			// found the empty cell; fill it
			item->Pointer = pointer;
			item->Stream = stream;
			item->Age = ++TVPArchiveHandleCacheAge;
			// counter overflow in TVPArchiveHandleCacheAge
			// is not so a big problem.
			// counter overflow can worsen the cache performance,
			// but it occurs only when the counter is overflowed
			// (it's too far less than usual)
			return;
		}

		if (i == 0 || oldest_age > item->Age)
		{
			oldest_age = item->Age;
			oldest = i;
		}
	}

	// empty cell not found
	// free oldest cell and fill it
	tTVPArchiveHandleCacheItem* oldest_item =
		TVPArchiveHandleCachePool + oldest;
	delete oldest_item->Stream, oldest_item->Stream = NULL;
	oldest_item->Pointer = pointer;
	oldest_item->Stream = stream;
	oldest_item->Age = ++TVPArchiveHandleCacheAge;
}
//---------------------------------------------------------------------------
static void TVPFreeArchiveHandlePoolByPointer(void* pointer)
{
	// free all streams which have specified pointer
	if (TVPArchiveHandleCacheShutdown) return;
	if (!TVPArchiveHandleCacheInit) return;

	//tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

	for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
	{
		tTVPArchiveHandleCacheItem* item =
			TVPArchiveHandleCachePool + i;
		if (item->Stream && item->Pointer == pointer)
		{
			delete item->Stream, item->Stream = NULL;
			item->Pointer = NULL;
			item->Age = 0;
		}
	}
}
//---------------------------------------------------------------------------
static void TVPFreeArchiveHandlePool()
{
	// free all streams
	if (TVPArchiveHandleCacheShutdown) return;
	if (!TVPArchiveHandleCacheInit) return;

	//tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

	for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
	{
		tTVPArchiveHandleCacheItem* item =
			TVPArchiveHandleCachePool + i;
		if (item->Stream)
		{
			delete item->Stream, item->Stream = NULL;
			item->Pointer = NULL;
			item->Age = 0;
		}
	}
}
//---------------------------------------------------------------------------
static void TVPShutdownArchiveHandleCache()
{
	// free all stream and shutdown the pool
	//tTJSCriticalSectionHolder cs_holder(TVPArchiveHandleCacheCS);

	TVPArchiveHandleCacheShutdown = true;
	if (!TVPArchiveHandleCacheInit) return;

	for (tjs_int i = 0; i < TVP_MAX_ARCHIVE_HANDLE_CACHE; i++)
	{
		if (TVPArchiveHandleCachePool[i].Stream)
			delete TVPArchiveHandleCachePool[i].Stream;
	}
	delete[] TVPArchiveHandleCachePool;
}
//---------------------------------------------------------------------------
static tTVPAtExit TVPShutdownArchiveCacheAtExit
(TVP_ATEXIT_PRI_CLEANUP, TVPShutdownArchiveHandleCache);

//---------------------------------------------------------------------------
/*
tTJSBinaryStream* ReadXP3::CreateStreamByIndex(tjs_uint idx)
{
	if (idx >= ItemVector.size()); //TVPThrowExceptionMessage(TVPReadError);

	tArchiveItem& item = ItemVector[idx];

	tTJSBinaryStream* stream = TVPGetCachedArchiveHandle(this, Name);

	tTJSBinaryStream* out;
	try
	{
		out = new tTVPXP3ArchiveStream(this, idx, &(item.Segments), stream,
			item.OrgSize);
	}
	catch (...)
	{
		TVPReleaseCachedArchiveHandle(this, stream);
		throw;
	}

	return out;
}
*/
static bool TVPAtExitShutdown = false;
static std::vector<tTVPAtExitInfo>* TVPAtExitInfos = NULL;
void TVPAddAtExitHandler(tjs_int pri, void (*handler)())
{
	if (TVPAtExitShutdown) return;

	if (!TVPAtExitInfos) TVPAtExitInfos = new std::vector<tTVPAtExitInfo>();
	TVPAtExitInfos->push_back(tTVPAtExitInfo(pri, handler));
}

ReadXP3::ReadXP3(const ttstr& name)
{
	
	Name = name;
	Count = 0;

	tjs_uint64 offset;

	//tTJSBinaryStream* st = TVPCreateStream(name);

	tjs_uint8* indexdata = NULL;

	//static tjs_uint8 cn_File[] =
	//{ 0x46/*'F'*/, 0x69/*'i'*/, 0x6c/*'l'*/, 0x65/*'e'*/ };
	//static tjs_uint8 cn_info[] =
	//{ 0x69/*'i'*/, 0x6e/*'n'*/, 0x66/*'f'*/, 0x6f/*'o'*/ };
	//static tjs_uint8 cn_segm[] =
	//{ 0x73/*'s'*/, 0x65/*'e'*/, 0x67/*'g'*/, 0x6d/*'m'*/ };
	//static tjs_uint8 cn_adlr[] =
	//{ 0x61/*'a'*/, 0x64/*'d'*/, 0x6c/*'l'*/, 0x72/*'r'*/ };

	//TVPAddLog(TVPFormatMessage(TVPInfoTryingToReadXp3VirtualFileSystemInformationFrom, name));
	/*
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

					int result = uncompress(  
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
				if (!FindChunk(indexdata, cn_info, ch_info_start, ch_info_size));
				//TVPThrowExceptionMessage(TVPReadError);

			// read info sub-chunk
				tArchiveItem item;
				tjs_uint32 flags = ReadI32FromMem(indexdata + ch_info_start + 0);
				//if (!TVPAllowExtractProtectedStorage && (flags & TVP_XP3_FILE_PROTECTED))
					//TVPThrowExceptionMessage(TVPSpecifiedStorageHadBeenProtected);
				item.OrgSize = ReadI64FromMem(indexdata + ch_info_start + 4);
				item.ArcSize = ReadI64FromMem(indexdata + ch_info_start + 12);

				tjs_int len = ReadI16FromMem(indexdata + ch_info_start + 20);
				//ttstr name = TVPStringFromBMPUnicode(
					//(const tjs_uint16*)(indexdata + ch_info_start + 22), len);
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
					;
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
	*/
	//TVPAddLog(TVPFormatMessage(TVPInfoDoneWithContains, ttstr(Count), ttstr(segmentcount)));
}