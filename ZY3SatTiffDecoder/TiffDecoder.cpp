/********************************************************************************
*   File:  TiffDecoder.cpp
*
*   CREAT _by_  :   fengyhack (Ind.)
*   Last modified:  2015-08-13 Thur ( EST: 2015-01-15 Thur)
*
*   Email:          fengyhack@{163,gmail}.com
*   Website:      http://fenyh.cn/
*
*   This is the <IMPLEMENTATION> of TiffDecoder class.
*
*   Supported compression type: UNCOMPRESSED, PACKBITS
*
*    -------------------------  How-to-use ----------------------------
*
*     // CHAR fileName[]="F:\\1.tiff";
*
*     TiffDecoder  td;
*     td.Open(fileName);
*     td.Prepare(); // Prepare before Load !!!
*
*     int width = td.GetWidth();
*     int height = td.GetHeight();
*     int imgSize = width*height;
*
*     // -- As for ZY3 TIFF image, we know it is 16bit ('depth') --
*
*     u16* img_src = new u16[imgSize];
*     td.LoadData((byte*)img_src);
*
*     //-- Or if we do NOT known the 'depth' filed:
*     // int nbytes=td.GetDataByteCount();
*     // byte* img_src=new byte[nbytes];
*     // td.LoadData(img_src);
*     // --
*
*     // td.Release(); // This will be called automatically (in td.dtor())
*     // Processing....
*
*      delete[] img_src;
*      // return...
*
*    --------------------------------------------------------------------
*
********************************************************************************/

#include "TiffDecoder.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

TiffDecoder::TiffDecoder()
{
	m_fileHandle = NULL;
	m_filePosition = 0L;
	m_byteOrder = ByteOrder::BO_DEFAULT_VAL;

	m_ifdCount = 0;
	_InitIfdNodeH();

	m_imageWidth = 0;
	m_imageHeight = 0;
	m_numComponents = 0;
	m_bitsPerComponent = 0;
	m_photoType = PhotoType::PT_DEFAULT_VAL;
	m_compressionType = CompressionType::CT_DEFAULT_VAL;
	m_isValidTiff = FALSE;

	m_stripCountStored = FALSE;	
	m_stripCount = 0;
	m_strip_offsets_array_offset = 0;
	m_strip_bycounts_array_offset = 0;
	m_rowsPerStrip = 0;

	m_vtStripOffsetsArray = ValueType::VT_DEFAULT_VAL;
	m_vtStripByCountsArray = ValueType::VT_DEFAULT_VAL;
	m_stripOffsetsArray = NULL;
	m_stripByteCountsArray = NULL;
	
	m_isPrepared = FALSE;
	m_supported = FALSE;
	m_dataByteCount = 0;

	m_isReleased = FALSE;
}

TiffDecoder::~TiffDecoder()
{
	Release();
}

BOOL TiffDecoder::Open(const CHAR* fileName)
{
	m_fileHandle = fopen(fileName, "rb");
	if (m_fileHandle == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL TiffDecoder::Prepare()
{
	if (m_fileHandle == NULL)
	{
		return FALSE;
	}

	_ReadTiffHeader();
	if (!m_isValidTiff)
	{
		return FALSE;
	}

	m_filePosition = ftell(m_fileHandle);	// RECORD the position after the HEADER '42'

	DWORD dword;
	fread(&dword, CI_DWORD_SZ, 1, m_fileHandle);
	if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
	{
		_ChangeByteOrderDWORD(&dword);
	}

	while (dword > 0)
	{
		dword = _ReadIfdNode(&m_pIfdNodeH, (LONG)dword);
	}

	// fseek(m_fileHandle, m_filePosition, SEEK_SET);

	_SupportCheck();

	if (!m_supported)
	{
		return FALSE;
	}

	m_dataByteCount = (UINT32)(1.0*m_imageWidth*m_imageHeight*m_numComponents*m_bitsPerComponent / 8.0);

	_InitStripArray();

	if (m_compressionType != CompressionType::CT_UNCOMPRESSED &&
		m_compressionType != CompressionType::CT_PACK_BITS)
	{
		return FALSE;
	}

	m_isPrepared = TRUE;

	return TRUE;
}

BOOL TiffDecoder::LoadData(BYTE* upkDataPtr)
{
	if (!m_isPrepared)
	{
		printf("NOT prepared / Failed to load image data !\n");
		return FALSE;
	}

	if (upkDataPtr == NULL)
	{
		return FALSE;
	}

	if (m_compressionType == CompressionType::CT_UNCOMPRESSED)
	{
		_DecodeBytes_UNCOPMRESSED(upkDataPtr);
	}
	else if (m_compressionType == CompressionType::CT_PACK_BITS)
	{
		_DecodeBytes_PACKBITS(upkDataPtr);
	}
	else
	{
		return FALSE;
	}

	_FlipDataOrder(upkDataPtr);

	return TRUE;
}

VOID TiffDecoder::Release()
{
	if (m_isReleased)
	{
		return;
	}

	if (m_stripCount > 0)
	{
		free(m_stripOffsetsArray);
		free(m_stripByteCountsArray);
		m_stripOffsetsArray = NULL;
		m_stripByteCountsArray = NULL;
		m_stripCount = 0;
	}

	if (m_ifdCount > 0)
	{
		// Since we set count=0 if pIfd=NULL
		UINT32 i;
		IfdNode* pNode = m_pIfdNodeH->PtrNext;
		for (i = 0; i < m_ifdCount; ++i)
		{
			free(pNode->A2_4_pDirEntries);
			pNode = pNode->PtrNext;
		}
		free(m_pIfdNodeH);
		m_pIfdNodeH = NULL;
		m_ifdCount = 0;
	}

	if (m_fileHandle != NULL)
	{
		fclose(m_fileHandle);
		m_fileHandle = NULL;
	}

	m_isReleased = TRUE;
}

VOID TiffDecoder::_ReadTiffHeader()
{
	WORD word;
	fread(&word, CI_WORD_SZ, 1, m_fileHandle);
	if (word == CW_II_LE)
	{
		m_isValidTiff = TRUE;
		m_byteOrder = ByteOrder::BO_LITTLE_ENDIAN;
	}
	else if (word == CW_MM_BE)
	{
		m_isValidTiff = TRUE;
		m_byteOrder = ByteOrder::BO_BIG_ENDIAN;
	}
	else
	{
		m_isValidTiff = FALSE;
		m_byteOrder = ByteOrder::BO_DEFAULT_VAL;
	}

	fread(&word, CI_WORD_SZ, 1, m_fileHandle);
	if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
	{
		_ChangeByteOrderWORD(&word);
	}

	if (word != CW_GN_42)
	{
		m_isValidTiff = FALSE;
	}
}

VOID TiffDecoder::_InitIfdNodeH()
{
	m_pIfdNodeH = (IfdNode*)malloc(CI_IFD_NODE_SZ);
	m_pIfdNodeH->A1_2_count = 0;
	m_pIfdNodeH->A2_4_pDirEntries = NULL;
	m_pIfdNodeH->A3_4_next = 0;
	m_pIfdNodeH->PtrNext = NULL;
}

DWORD  TiffDecoder::_ReadIfdNode(IfdNode** ppIfdNode, LONG offset)
{
	++m_ifdCount;
	IfdNode* pIfdNodeNew = (IfdNode*)malloc(CI_IFD_NODE_SZ);

	fseek(m_fileHandle, offset, SEEK_SET); // Find the (next) IFD

	WORD word;
	fread(&word, CI_WORD_SZ, 1, m_fileHandle);
	if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
	{
		_ChangeByteOrderWORD(&word);
	}

	pIfdNodeNew->A1_2_count = word;

	DirectoryEntry* pDirEntries = (DirectoryEntry*)malloc(word*CI_DIRENTRY_SZ);
	INT32 i;
	for (i = 0; i < word; ++i)
	{
		_ReadDirectoryEntry(&(pDirEntries[i]));
	}
	pIfdNodeNew->A2_4_pDirEntries = pDirEntries;

	DWORD dword;
	fread(&dword, CI_DWORD_SZ, 1, m_fileHandle);
	if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
	{
		_ChangeByteOrderDWORD(&dword);
	}
	pIfdNodeNew->A3_4_next = dword;

	pIfdNodeNew->PtrNext = NULL;

	(*ppIfdNode)->PtrNext = pIfdNodeNew;

	if ( !m_stripCountStored )
	{
		m_stripCount = (m_imageHeight + m_rowsPerStrip - 1) / m_rowsPerStrip;
		m_stripCountStored = TRUE;
	}

	return dword;
}

VOID TiffDecoder::_ReadDirectoryEntry(DirectoryEntry* p_de)
{
	fread(p_de, CI_DIRENTRY_SZ, 1, m_fileHandle);
	m_filePosition = ftell(m_fileHandle);

	if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
	{
		_ChangeByteOrderWORD(&(p_de->A1_2_tid));
		_ChangeByteOrderWORD(&(p_de->A2_2_type));
		_ChangeByteOrderDWORD(&(p_de->A3_4_count));
		_ChangeByteOrderDWORD(&(p_de->A4_4_val_offset));
	}
	WORD tagID = p_de->A1_2_tid;
	WORD type = p_de->A2_2_type;
	DWORD count = p_de->A3_4_count;
	DWORD value = p_de->A4_4_val_offset;

	switch (tagID)
	{
	case TagProperty::TP_IMAGE_WIDTH:
		m_imageWidth = _GetValueDW(type, value);
		break;
	case TagProperty::TP_IMAGE_HEIGHT:
		m_imageHeight = _GetValueDW(type, value);
		break;
	case TagProperty::TP_NUM_COMPONENTS:
		if (m_numComponents == 0)
		{
			m_numComponents = _GetValueDW(ValueType::VT_SHORT, value);
		}
		break;
	case TagProperty::TP_BITS_PER_COMPONENT:
		_ParseBitsPerComponentDE(count, value);
		break;
	case TagProperty::TP_PHOTO_INTERP:
		_ParsePhotoTypeDE(value);
		break;
	case TagProperty::TP_PLANAR_CFG:
		m_planarConfig = _GetValueDW(ValueType::VT_SHORT, value);
		break;
	case TagProperty::TP_ROWS_PERSTRIP:
		m_rowsPerStrip = _GetValueDW(type, value);
		break;
	case TagProperty::TP_STRIP_BYTE_CNT:
		if (!m_stripCountStored)
		{
			m_stripCount = count;
			m_stripCountStored = TRUE;
		}
		m_vtStripByCountsArray = (ValueType)type;
		m_strip_bycounts_array_offset = _GetValueDW(type, value);
		break;
	case TagProperty::TP_STRIP_OFFSETS:
		if (!m_stripCountStored)
		{
			m_stripCount = count;
			m_stripCountStored = TRUE;
		}
		m_vtStripOffsetsArray = (ValueType)type;
		m_strip_offsets_array_offset = _GetValueDW(type, value);
		break;
	case TagProperty::TP_COMPRESSION:
		_ParseCompressionTypeDE(value);
		break;
	default:
		break;
	}
}

VOID TiffDecoder::_ParseBitsPerComponentDE(DWORD count, DWORD value)
{
	if (m_numComponents == 0)
	{
		m_numComponents = count;
	}
	if (count == 1)
	{
		m_bitsPerComponent = _GetValueDW(ValueType::VT_SHORT, value);
	}
	else
	{
		fseek(m_fileHandle, value, SEEK_SET);
		USHORT ushv;
		fread(&ushv, sizeof(USHORT), 1, m_fileHandle);
		fseek(m_fileHandle, m_filePosition, SEEK_SET);  // !!!!!!!!!! 
		m_bitsPerComponent = ushv;
	}
}

VOID TiffDecoder::_ParsePhotoTypeDE(DWORD value)
{
	m_photoType = (PhotoType)(_GetValueDW(ValueType::VT_SHORT, value));
}

VOID TiffDecoder::_ParseCompressionTypeDE(DWORD value)
{
	m_compressionType = (CompressionType)(_GetValueDW(ValueType::VT_SHORT, value));
}

VOID TiffDecoder::_ChangeByteOrderWORD(WORD* p_word)
{
	BYTE* b2 = (BYTE*)p_word;
	*p_word = MAKEWORD(b2[1], b2[0]);
}

VOID TiffDecoder::_ChangeByteOrderDWORD(DWORD* p_dword)
{
	BYTE* b4 = (BYTE*)p_dword;
	*p_dword = MAKEDWORD(b4[3], b4[2], b4[1], b4[0]);
}

UINT32 TiffDecoder::_UnPackBits(BYTE* dst, BYTE* src, UINT32 count)
{
	UINT32 upkCount = 0; // Count of bytes unpacked 
	UINT32 position = 0; // Position of input buffer
	SBYTE n; // Flag
	BYTE value;

	while (position<count)
	{
		n = src[position]; // Read the flag
		++position; // From the next position

		if ( (n >= 0) && (n<=127) )
		{
			_BufferCopy(dst + upkCount, src + position, n + 1);
			upkCount += n + 1;
			position += n + 1;
		}
		else if ((-127 <= n) && (n <= -1) )
		{
			value = src[position];
			_BufferSetVal(dst + upkCount, value, 1 - n);
			++position;
			upkCount += 1 - n;
		}
		else
		{
			continue;
		}
	}

	return upkCount;
}

VOID TiffDecoder::_BufferSetVal(BYTE* buffer, BYTE value, UINT32 count)
{
	UINT32 i;
	for (i = 0; i < count; ++i)
	{
		buffer[i] = value;
	}
}

VOID TiffDecoder::_BufferCopy(BYTE* dstBuffer, BYTE* srcBuffer, UINT32 count)
{
	UINT32 i;
	for (i = 0; i < count; ++i)
	{
		dstBuffer[i] = srcBuffer[i];
	}
}

DWORD TiffDecoder::_GetValueDW(WORD type,DWORD value)
{
	if (type==ValueType::VT_SHORT)
	{
		return ( HIWORD(value) > 0 ? HIWORD(value) : LOWORD(value) );
	}
	else
	{
		return value;
	}
}

VOID TiffDecoder::_SupportCheck()
{
	UINT32 uns = 0;

	// 1 
	if (m_isValidTiff)
	{
		++uns;
	}

	// 2 
	switch (m_photoType)
	{
	case PT_WHITE_ZERO:
	case PT_BLACK_ZERO:
	case PT_RGB:
		++uns;
		break;
	default:
		break;
	}

	// 3 
	switch (m_compressionType)
	{
	case CT_UNCOMPRESSED:
	case CT_PACK_BITS:
	case CT_LZW:
		++uns;
		break;
	default:
		break;
	}

	// 4 
	if (m_planarConfig == 1)
	{
		++uns;
	}
	
	// 5 
	if (m_imageWidth > 0)
	{
		++uns;
	}

	// 6 
	if (m_imageHeight > 0)
	{
		++uns;
	}

	// 7
	if (m_numComponents == 1 || m_numComponents == 3||m_numComponents==4)
	{
		++uns;
	}

	// 8 
	if (m_bitsPerComponent % 8 == 0)
	{
		++uns;
	}

	// 9 
	if (m_stripCount > 0)
	{
		++uns;
	}

	m_supported = (uns == 9);
}

VOID TiffDecoder::_InitStripArray()
{
	WORD* tempArray = NULL;

	fseek(m_fileHandle, m_strip_offsets_array_offset, SEEK_SET);
	m_stripOffsetsArray = (DWORD*)malloc(m_stripCount*CI_DWORD_SZ);
	if (m_vtStripOffsetsArray == ValueType::VT_LONG)
	{
		fread(m_stripOffsetsArray, CI_DWORD_SZ, m_stripCount, m_fileHandle);
	}
	else
	{
		tempArray = (WORD*)malloc(m_stripCount*CI_WORD_SZ);
		fread(tempArray, CI_WORD_SZ, m_stripCount, m_fileHandle);
		_BufferCopyW2DW(m_stripOffsetsArray, tempArray, m_stripCount);
		free(tempArray);
		tempArray = NULL;
	}

	fseek(m_fileHandle, m_strip_bycounts_array_offset, SEEK_SET);
	m_stripByteCountsArray = (DWORD*)malloc(m_stripCount*CI_DWORD_SZ);
	if (m_vtStripByCountsArray == ValueType::VT_LONG)
	{
		fread(m_stripByteCountsArray, CI_DWORD_SZ, m_stripCount, m_fileHandle);
	}
	else
	{
		tempArray = (WORD*)malloc(m_stripCount*CI_WORD_SZ);
		fread(tempArray, CI_WORD_SZ, m_stripCount, m_fileHandle);
		_BufferCopyW2DW(m_stripByteCountsArray, tempArray, m_stripCount);
		free(tempArray);
		tempArray = NULL;
	}
}

VOID TiffDecoder::_BufferCopyW2DW(DWORD* dst, WORD* src, UINT32 count)
{
	UINT32 i;
	for (i = 0; i < count; ++i)
	{
		dst[i] = src[i];
	}
}

VOID TiffDecoder::_DecodeBytes_UNCOPMRESSED(BYTE* upkDataPtr)
{
	UINT32 i;
	DWORD count,offset;
	BYTE* ptr = upkDataPtr;
	int nc = m_bitsPerComponent / m_numComponents;
	for (i = 0; i < m_stripCount; ++i)
	{
		count = m_stripByteCountsArray[i];
		if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
		{
			_ChangeByteOrderDWORD(&count);
		}
		offset = m_stripOffsetsArray[i];
		if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
		{
			_ChangeByteOrderDWORD(&offset);
		}
		fseek(m_fileHandle, offset, SEEK_SET);
		fread(ptr, nc, count, m_fileHandle);
		ptr += count;
	}
}

VOID TiffDecoder::_DecodeBytes_PACKBITS(BYTE* upkDataPtr)
{
	UINT32 i;
	DWORD count, offset, upkCount;
	BYTE* src = NULL;
	BYTE* dst = upkDataPtr;
	for (i = 0; i < m_stripCount; ++i)
	{
		count = m_stripByteCountsArray[i];
		if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
		{
			_ChangeByteOrderDWORD(&count);
		}
		src = (BYTE*)malloc(count);
		if (src == NULL)
		{
			break;
		}
		offset = m_stripOffsetsArray[i];
		if (m_byteOrder == ByteOrder::BO_BIG_ENDIAN)
		{
			_ChangeByteOrderDWORD(&offset);
		}
		fseek(m_fileHandle, offset, SEEK_SET);
		fread(src, 1, count, m_fileHandle);
		upkCount = _UnPackBits(dst, src, count);
		free(src);
		dst += upkCount;
	}
}

VOID TiffDecoder::_FlipDataOrder(BYTE* imageData)
{
	UINT32 i;
	BYTE r, g, b, a;

	if (m_numComponents == 3)
	{
		for (i = 0; i < m_dataByteCount; i += 3)
		{
			r = imageData[i];
			b= imageData[i + 2];
			imageData[i] = b;
			imageData[i + 2] = r;
		}
	}
	else if (m_numComponents==4)
	{
		for (i = 0; i < m_dataByteCount; i += 4)
		{
			a = imageData[i];
			r = imageData[i + 1];
			g = imageData[i + 2];
			b = imageData[i + 3];
			imageData[i] = b;
			imageData[i + 1] = g;
			imageData[i + 2] = r;
			imageData[i + 3] = a;
		}
	}
	else
	{
		return;
	}
}

// End of File 