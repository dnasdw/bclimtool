#ifndef BCLIM_H_
#define BCLIM_H_

#include <sdw.h>

namespace pvrtexture
{
	class CPVRTexture;
}

#include SDW_MSC_PUSH_PACKED
struct SBclimHeader
{
	u32 Signature;
	u16 ByteOrder;
	u16 HeaderSize;
	u32 Version;
	u32 FileSize;
	u16 DataBlocks;
	u16 Reserved;
} SDW_GNUC_PACKED;

struct SImageBlock
{
	u32 Signature;
	u32 HeaderSize;
	u16 Width;
	u16 Height;
	u32 Format;
	u32 ImageSize;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

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
	void SetFileName(const UString& a_sFileName);
	void SetPngName(const UString& a_sPngName);
	void SetTextureFormat(ETextureFormat a_eTextureFormat);
	void SetVerbose(bool a_bVerbose);
	bool DecodeFile();
	bool EncodeFile();
	bool CreateFile();
	static bool IsBclimFile(const UString& a_sFileName);
	static const u32 s_uSignatureBclim;
	static const u32 s_uSignatureImage;
	static const int s_nBPP[];
	static const int s_nDecodeTransByte[64];
private:
	static n32 getBoundSize(n32 a_nSize);
	static int decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture);
	static void encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer);
	UString m_sFileName;
	UString m_sPngName;
	ETextureFormat m_eTextureFormat;
	bool m_bVerbose;
};

#endif	// BCLIM_H_
