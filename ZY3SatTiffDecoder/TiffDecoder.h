/********************************************************************************
 *   File:  TiffDecoder.h
 *
 *   CREAT _by_   :  fengyhack (Ind.)
 *   Last modified:  2015-08-13 Thur ( EST: 2015-01-15 Thur)
 *
 *   Email:          fengyhack@{163,gmail}.com
 *   Website:      http://fenyh.cn/
 *   
 *   This is the <DECLARATION> of TiffDecoder class, and some definitions.
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

#ifndef TIFF_DECODER_H
#define TIFF_DECODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

enum ByteOrder
{
	BO_DEFAULT_VAL=0,
	BO_LITTLE_ENDIAN=1,
	BO_BIG_ENDIAN=2
};

enum ValueType
{
	VT_DEFAULT_VAL = 0,
	VT_BYTE = 1,
	VT_ASCII = 2,
	VT_SHORT = 3,
	VT_LONG = 4,
	VT_RATIONAL = 5,
	VT_SBYTE = 6,
	VT_UNDEFINED=7,
	VT_SSHORT = 8,
	VT_SLONG = 9,
	VT_SRATIONAL = 10,
	VT_FLOAT = 11,
	VT_DOUBLE = 12
};

enum TagProperty
{
	TP_NEW_SUB_FILETYPE = 254,
	TP_SUB_FILETYPE = 255, // 0x0FF
	TP_IMAGE_WIDTH = 256, // 0x100
	TP_IMAGE_HEIGHT = 257, // 0x101
	TP_BITS_PER_COMPONENT = 258, // 0x102
	TP_COMPRESSION = 259, // 0x103 
	TP_PHOTO_INTERP = 262, // 0x106 
	TP_THRESHOLDING = 263,
	TP_CELL_WIDTH = 264,
	TP_CELL_HEIGHT = 265,
	TP_FILL_ORDER = 266,
	TP_DOCUMENT_NAME = 269,
	TP_IMAGE_DESCRIPTION = 270,
	TP_MAKE = 271,
	TP_MODEL = 272,
	TP_STRIP_OFFSETS = 273, // 0x111
	TP_ORIENTATION = 274,
	TP_NUM_COMPONENTS = 277, // 0x115
	TP_ROWS_PERSTRIP = 278, // 0x116 
	TP_STRIP_BYTE_CNT = 279, // 0x117
	TP_MIN_SAMPL_VAL = 280,
	TP_MAX_SAMPL_VAL = 281,
	TP_XRESOLUTION = 282,
	TP_YRESOLUTION = 283,
	TP_PLANAR_CFG = 284, // 0x11C
	TP_XPOSITION = 286,
	TP_YPOSITION = 287,
	TP_FREE_OFFSETS = 288,
	TP_FREE_BYTE_CNT = 289,
	TP_RESOLUTION_UNIT = 296,
	TP_SOFTWARE = 305,
	TP_DATE_TIME = 306,
	TP_COLOR_MAP = 320,
	TP_SAMPLE_FMT = 339 // 0x153 
};

enum PhotoType
{
	PT_DEFAULT_VAL = -1,
	PT_WHITE_ZERO = 0,
	PT_BLACK_ZERO = 1,
	PT_RGB = 2,
	PT_RGB_PALETTE = 3,
	PT_TRANS_MASK = 4,
	TP_CMYK = 5,
	TP_YCB = 6,
	TP_CIE = 8,
};

enum CompressionType
{
	CT_DEFAULT_VAL = 0,
	CT_UNCOMPRESSED = 1,
	CT_CCITT_1D = 2,
	CT_FAX3 = 3,
	CT_FAX4 = 4,
	CT_LZW = 5,
	CT_JPEG = 6,
	CT_PACK_BITS = 32773
};

#pragma pack(push,1)

struct DirectoryEntry
{
	WORD   A1_2_tid;
	WORD   A2_2_type;
	DWORD A3_4_count;
	DWORD A4_4_val_offset;
};

struct IfdNode
{
	WORD A1_2_count;
	DirectoryEntry* A2_4_pDirEntries;
	DWORD A3_4_next;
	IfdNode* PtrNext;
};

#pragma pack(pop)

// LO/HI: BYTE/WORD
#define LOBYTE(w)  ( (BYTE)(w) )
#define HIBYTE(w)  ( (BYTE)( (WORD)(w) >> 8 ) )
#define LOWORD(dw) ( (WORD)(dw) )
#define HIWORD(dw) ( (WORD)( (DWORD)(dw)>>16 ) )
// MAKE WORD/DWORD (Little Endian)
#define MAKEWORD(b1,b2)  ( (WORD)( (b1) | ( (WORD)(b2)<<8 ) ) )
#define MAKEDWORD(b1,b2,b3,b4)  ((DWORD)((b1)|((DWORD)(b2)<<8)|((DWORD)(b3)<<16)|((DWORD)(b4)<<24)))

// Size of Datatype
const INT32 CI_WORD_SZ = sizeof(WORD);
const INT32 CI_DWORD_SZ = sizeof(DWORD);

// II: Little Endian
const WORD CW_II_LE = MAKEWORD(0x49, 0x49);
// MM: Big Endian 
const WORD CW_MM_BE = MAKEWORD(0x4D, 0x4D);
// Good Number: 42 
const WORD CW_GN_42 = 42;
// Size of Structure
const INT32 CI_IFD_NODE_SZ = sizeof(IfdNode);
const INT32 CI_DIRENTRY_SZ = sizeof(DirectoryEntry);

// Status/ Return value 
#define NULL  0
#define TRUE  1
#define FALSE  0

class TiffDecoder
{
public:
	TiffDecoder();
	~TiffDecoder();
	BOOL  Open(const CHAR* fileName);
	BOOL  Prepare();
	BOOL  LoadData(BYTE* upkDataPtr);
	VOID  Release();

	LONG  GetWidth() { return m_imageWidth; }
	LONG  GetHeight() { return m_imageHeight; }
	LONG  GetChannel() { return m_numComponents;  }
	ULONG  GetDataByteCount() { return m_dataByteCount; }

private:
	VOID       _ReadTiffHeader();	
	VOID       _InitIfdNodeH();
	DWORD  _ReadIfdNode(IfdNode** ppIfdNode, LONG offset);
	VOID       _ReadDirectoryEntry(DirectoryEntry* p_de);
	VOID       _ParseBitsPerComponentDE(DWORD count, DWORD value);
	VOID       _ParsePhotoTypeDE(DWORD value);
	VOID       _ParseCompressionTypeDE(DWORD value);

	VOID       _ChangeByteOrderWORD(WORD* p_word);
	VOID       _ChangeByteOrderDWORD(DWORD* p_dword);
	DWORD  _GetValueDW(WORD type, DWORD value);

	UINT32    _UnPackBits(BYTE* dst, BYTE* src, UINT32 count);
	VOID       _BufferSetVal(BYTE* buffer, BYTE value, UINT32 count);
	VOID       _BufferCopy(BYTE* dstBuffer, BYTE* srcBuffer, UINT32 count);
	VOID       _BufferCopyW2DW(DWORD* dst, WORD* src, UINT32 count);

	VOID       _InitStripArray();
	VOID       _DecodeBytes_UNCOPMRESSED(BYTE* upkDataPtr);
	VOID       _DecodeBytes_PACKBITS(BYTE* upkDataPtr);
	
	VOID       _FlipDataOrder(BYTE* imageData);

	VOID       _SupportCheck();

private:
	FILE*  m_fileHandle;
	LONG  m_filePosition;
	ByteOrder  m_byteOrder;
	BOOL  m_isValidTiff;

	UINT32  m_ifdCount;
	IfdNode*  m_pIfdNodeH;

	LONG  m_imageWidth;
	LONG  m_imageHeight;
	UINT32  m_numComponents;
	UINT32  m_bitsPerComponent;
	UINT32  m_planarConfig;

	PhotoType  m_photoType;
	CompressionType  m_compressionType;

	BOOL  m_stripCountStored;
	ULONG  m_stripCount;	
	ULONG  m_strip_offsets_array_offset;
	ULONG  m_strip_bycounts_array_offset;
	ULONG  m_rowsPerStrip;

	ValueType  m_vtStripOffsetsArray;
	ValueType  m_vtStripByCountsArray;
	DWORD*  m_stripOffsetsArray;
	DWORD*  m_stripByteCountsArray;

	BOOL  m_isPrepared;
	BOOL  m_supported;

	UINT32  m_dataByteCount;

	BOOL  m_isReleased;
};

#endif // TIFF_DECODER_H

// End of File