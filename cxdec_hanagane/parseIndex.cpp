#include<iostream>
#include<Windows.h>
#include"tjsTypes.h"
#include"parseIndex.h"
#include <algorithm>
using namespace std;

tjs_uint64 tTVPXP3Index::ReadI64LE(HANDLE handle)
{
	tjs_uint64 temp;
	ReadBuffer(handle,&temp, 8);
	return temp;
}
void tTVPXP3Index::ReadBuffer(HANDLE handle,void* _buffer, tjs_uint _size)
{
	ReadFile(handle, _buffer, _size, NULL, NULL);
}

tjs_int64 tTVPXP3Index::ReadI64FromMem(const tjs_uint8* mem)
{
	tjs_uint64 ret = (tjs_uint64)mem[0] | ((tjs_uint64)mem[1] << 8) |
		((tjs_uint64)mem[2] << 16) | ((tjs_uint64)mem[3] << 24) |
		((tjs_uint64)mem[4] << 32) | ((tjs_uint64)mem[5] << 40) |
		((tjs_uint64)mem[6] << 48) | ((tjs_uint64)mem[7] << 56);
	return (tjs_int64)ret;
}
tjs_int32 tTVPXP3Index::ReadI32FromMem(const tjs_uint8* mem)
{
	tjs_uint32 ret = (tjs_uint32)mem[0] | ((tjs_uint32)mem[1] << 8) |
		((tjs_uint32)mem[2] << 16) | ((tjs_uint32)mem[3] << 24);
	return (tjs_int32)ret;
}
tjs_int16 tTVPXP3Index::ReadI16FromMem(const tjs_uint8* mem)
{
	tjs_uint16 ret = (tjs_uint16)mem[0] | ((tjs_uint16)mem[1] << 8);
	return (tjs_int16)ret;
}

bool tTVPXP3Index::FindChunk(const tjs_uint8* data, const tjs_uint8* name, tjs_uint& start, tjs_uint& size)
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
			return false;
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

ttstr tTVPXP3Index::TVPStringFromBMPUnicode(const tjs_uint16* src, tjs_int maxlen)
{
	// convert to ttstr from BMP unicode
	if (sizeof(tjs_char) == 2)
	{
		// sizeof(tjs_char) is 2 (windows native)
		if (maxlen == -1)
			return ttstr((const tjs_char*)src);
		else
			return ttstr((const tjs_char*)src, maxlen);
	}
	else if (sizeof(tjs_char) == 4)
	{
		// sizeof(tjs_char) is 4 (UCS32)
		// FIXME: NOT TESTED CODE
		tjs_int len = 0;
		const tjs_uint16* p = src;
		while (*p) len++, p++;
		if (maxlen != -1 && len > maxlen) len = maxlen;
		ttstr ret((tTJSStringBufferLength)(len));
		tjs_char* dest = ret.Independ();
		p = src;
		while (len && *p)
		{
			*dest = *p;
			dest++;
			p++;
			len--;
		}
		*dest = 0;
		ret.FixLen();
		return ret;
	}
	return (const tjs_char*)"1232";
}
void tTVPXP3Index::NormalizeInArchiveStorageName(ttstr& name)
{
	// normalization of in-archive storage name does :
	if (name.IsEmpty()) return;

	// make all characters small
	// change '\\' to '/'
	tjs_char* ptr = name.Independ();
	while (*ptr)
	{
		if (*ptr >= TJS_W('A') && *ptr <= TJS_W('Z'))
			*ptr += TJS_W('a') - TJS_W('A');
		else if (*ptr == TJS_W('\\'))
			*ptr = TJS_W('/');
		ptr++;
	}

	// eliminate duplicated slashes
	ptr = name.Independ();
	tjs_char* org_ptr = ptr;
	tjs_char* dest = ptr;
	while (*ptr)
	{
		if (*ptr != TJS_W('/'))
		{
			*dest = *ptr;
			ptr++;
			dest++;
		}
		else
		{
			if (ptr != org_ptr)
			{
				*dest = *ptr;
				ptr++;
				dest++;
			}
			while (*ptr == TJS_W('/')) ptr++;
		}
	}
	*dest = 0;

	name.FixLen();
}
tjs_uint64 tTVPXP3Index::FindIndex()
{
	HANDLE handle = CreateFileW(_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (GetLastError() != ERROR_SUCCESS) return GetLastError();
	DWORD* _buffer = new DWORD[0xB];
	DWORD _read_size;

	SetFilePointer(handle, 0, 0, FILE_BEGIN);
	ReadFile(handle, _buffer, 0xB, &_read_size, NULL); //XP3 Magic Header
	//SetFilePointer(handle, 0xB, 0, FILE_BEGIN);
	ReadFile(handle, _buffer, 0x8, &_read_size, NULL); //Unknown
	//SetFilePointer(handle, 0x17, 0, FILE_BEGIN);
	ReadFile(handle, _buffer, 0x1, &_read_size, NULL); //FLAG
	ReadFile(handle, _buffer, 0x8, &_read_size, NULL);
	delete[] _buffer;
	_buffer = new DWORD[0x8];
	ReadFile(handle, _buffer, 0x8, &_read_size, NULL); //Index Offset
	tjs_uint64 _result = (*(_buffer + 1));
	delete[] _buffer;
	_index_offset = _result;
	return _result;
}
std::vector<tArchiveItem>* tTVPXP3Index::GetIndex()
{
	HANDLE handle = CreateFileW(_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (GetLastError() != ERROR_SUCCESS) return nullptr;
	DWORD* _buffer = new DWORD[0xB];
	DWORD _read_size;

	SetFilePointer(handle, 32, 0, FILE_BEGIN);
	tjs_uint8* indexdata = NULL;

	while (true)
	{
		if (indexdata) delete[] indexdata;

		tjs_uint64 index_ofs = this->ReadI64LE(handle); //¶Á³ö8byte indexËùÔÚoffset
		SetFilePointer(handle, index_ofs, 0, FILE_BEGIN);

		tjs_uint8 index_flag;
		this->ReadBuffer(handle,&index_flag, 1);
		tjs_uint index_size;

		if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) ==
			TVP_XP3_INDEX_ENCODE_ZLIB)
		{
			// compressed index
			tjs_uint64 compressed_size = this->ReadI64LE(handle);
			tjs_uint64 r_index_size = this->ReadI64LE(handle);

			if ((tjs_uint)compressed_size != compressed_size ||
				(tjs_uint)r_index_size != r_index_size)
				break;
			// too large to handle, or corrupted
			index_size = (tjs_int)r_index_size;
			indexdata = new tjs_uint8[index_size];
			tjs_uint8* compressed = new tjs_uint8[(tjs_uint)compressed_size];
			try
			{
				this->ReadBuffer(handle, compressed, (tjs_uint)compressed_size);

				unsigned long destlen = (unsigned long)index_size;

				int result = uncompress(  /* uncompress from zlib */
					(unsigned char*)indexdata,
					&destlen, (unsigned char*)compressed,
					(unsigned long)compressed_size);
				if (result != Z_OK ||
					destlen != (unsigned long)index_size)
					return 0;
			}
			catch (...)
			{
				//delete[] _buffer;
				delete[] compressed;
				throw;
			}
			delete[] compressed;
			//delete[] _buffer;
		}
		else if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) ==
			TVP_XP3_INDEX_ENCODE_RAW)
		{
			// uncompressed index
			tjs_uint64 r_index_size = this->ReadI64LE(handle);
			if ((tjs_uint)r_index_size != r_index_size)
				break;
			// too large to handle or corrupted
			index_size = (tjs_uint)r_index_size;
			indexdata = new tjs_uint8[index_size];
			this->ReadBuffer(handle, indexdata, index_size);
		}
		else
		{
			// unknown encode method
			break;
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
				break;

			// read info sub-chunk
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
				break;

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
				else
					break;
				LARGE_INTEGER offset;
				offset.QuadPart = 0;
				SetFilePointerEx(handle, offset, &offset, FILE_CURRENT);
				seg.Start = ReadI64FromMem(indexdata + pos_base + 4) + offset.QuadPart;
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
				break;

			// read 'aldr' sub-chunk
			item.FileHash = ReadI32FromMem(indexdata + ch_adlr_start);

			// push information
			ItemVector.push_back(item);

			// to next file
			ch_file_start += ch_file_size;
			ch_file_size = index_size - ch_file_start;
			Count++;
		}
	}
	std::stable_sort(ItemVector.begin(), ItemVector.end());
	return &ItemVector;

}
tTVPXP3Index::tTVPXP3Index()
{
	//handle = NULL;
	_filename = nullptr;
}
tTVPXP3Index::tTVPXP3Index(const LPCWSTR& _filename)
{
	this->_filename = _filename;
	//HANDLE handle = CreateFileW(_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
}
tTVPXP3Index::~tTVPXP3Index()
{
	//CloseHandle(this->handle);
	_filename = nullptr;
	ItemVector.clear();
}

