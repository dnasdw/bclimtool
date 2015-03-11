#ifndef BCLIM_H_
#define BCLIM_H_

#include "utility.h"

namespace pvrtexture
{
	class CPVRTexture;
}

#include MSC_PUSH_PACKED
struct SBclimHeader
{
	u32 Signature;
	u16 ByteOrder;
	u16 HeaderSize;
	u32 Version;
	u32 FileSize;
	u16 DataBlocks;
	u16 Reserved;
} GNUC_PACKED;

struct SImageBlock
{
	u32 Signature;
	u32 HeaderSize;
	u16 Width;
	u16 Height;
	u32 Format;
	u32 ImageSize;
} GNUC_PACKED;
#include MSC_POP_PACKED

class CBclim
{
public:
	enum ETextureFormat
	{
		kTextureFormatL8 = 0,
		kTextureFormatA8 = 1,
		kTextureFormatLA44 = 2,
		kTextureFormatLA88 = 3,
		kTextureFormatHL8 = 4,
		kTextureFormatRGB565 = 5,
		kTextureFormatRGB888 = 6,
		kTextureFormatRGBA5551 = 7,
		kTextureFormatRGBA4444 = 8,
		kTextureFormatRGBA8888 = 9,
		kTextureFormatETC1 = 10,
		kTextureFormatETC1_A4 = 11,
		kTextureFormatL4 = 12,
		kTextureFormatA4 = 13
	};
	CBclim();
	~CBclim();
	void SetFileName(const char* a_pFileName);
	void SetPngName(const char* a_pPngName);
	void SetTextureFormat(ETextureFormat a_eTextureFormat);
	void SetVerbose(bool a_bVerbose);
	bool DecodeFile();
	bool EncodeFile();
	bool CreateFile();
	static bool IsBclimFile(const char* a_pFileName);
	static const u32 s_uSignatureBclim;
	static const u32 s_uSignatureImage;
	static const int s_nBPP[];
	static const int s_nDecodeTransByte[64];
private:
	static n32 getBoundSize(n32 a_nSize);
	static int decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture);
	static void encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer);
	const char* m_pFileName;
	const char* m_pPngName;
	ETextureFormat m_eTextureFormat;
	bool m_bVerbose;
	FILE* m_fpBclim;
};

#endif	// BCLIM_H_
