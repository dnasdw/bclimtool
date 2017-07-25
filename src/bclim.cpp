#include "bclim.h"
#include <png.h>
#include <PVRTextureUtilities.h>

const u32 CBclim::s_uSignatureBclim = SDW_CONVERT_ENDIAN32('CLIM');
const u32 CBclim::s_uSignatureImage = SDW_CONVERT_ENDIAN32('imag');
const int CBclim::s_nBPP[] = { 8, 8, 8, 16, 16, 16, 24, 16, 16, 32, 4, 8, 4, 4 };
const int CBclim::s_nDecodeTransByte[64] =
{
	 0,  1,  4,  5, 16, 17, 20, 21,
	 2,  3,  6,  7, 18, 19, 22, 23,
	 8,  9, 12, 13, 24, 25, 28, 29,
	10, 11, 14, 15, 26, 27, 30, 31,
	32, 33, 36, 37, 48, 49, 52, 53,
	34, 35, 38, 39, 50, 51, 54, 55,
	40, 41, 44, 45, 56, 57, 60, 61,
	42, 43, 46, 47, 58, 59, 62, 63
};

CBclim::CBclim()
	: m_pFileName(nullptr)
	, m_pPngName(nullptr)
	, m_eTextureFormat(kTextureFormatRGBA8888)
	, m_bVerbose(false)
	, m_fpBclim(nullptr)
{
}

CBclim::~CBclim()
{
}

void CBclim::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CBclim::SetPngName(const char* a_pPngName)
{
	m_pPngName = a_pPngName;
}

void CBclim::SetTextureFormat(ETextureFormat a_eTextureFormat)
{
	m_eTextureFormat = a_eTextureFormat;
}

void CBclim::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CBclim::DecodeFile()
{
	bool bResult = true;
	m_fpBclim = Fopen(m_pFileName, "rb");
	if (m_fpBclim == nullptr)
	{
		return false;
	}
	Fseek(m_fpBclim, 0, SEEK_END);
	n64 nFileSize = Ftell(m_fpBclim);
	Fseek(m_fpBclim, 0, SEEK_SET);
	u8* pBin = new u8[static_cast<size_t>(nFileSize)];
	fread(pBin, 1, static_cast<size_t>(nFileSize), m_fpBclim);
	fclose(m_fpBclim);
	SBclimHeader* pBclimHeader = reinterpret_cast<SBclimHeader*>(pBin + nFileSize - (sizeof(SBclimHeader) + sizeof(SImageBlock)));
	SImageBlock* pImageBlock = reinterpret_cast<SImageBlock*>(pBin + nFileSize - sizeof(SImageBlock));
	do
	{
		if (pImageBlock->Format < kTextureFormatL8 || pImageBlock->Format > kTextureFormatA4)
		{
			printf("ERROR: unknown format %d\n\n", pImageBlock->Format);
			bResult = false;
			break;
		}
		n32 nWidth = getBoundSize(pImageBlock->Width);
		n32 nHeight = getBoundSize(pImageBlock->Height);
		n32 nCheckSize = nWidth * nHeight * s_nBPP[pImageBlock->Format] / 8;
		if (pImageBlock->ImageSize != nCheckSize && m_bVerbose)
		{
			printf("INFO: width: %X(%X), height: %X(%X), checksize: %X, size: %X, bpp: %d, format: %0X\n", nWidth, pImageBlock->Width, nHeight, pImageBlock->Height, nCheckSize, pImageBlock->ImageSize, pImageBlock->ImageSize * 8 / nWidth / nHeight, pImageBlock->Format);
		}
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		if (decode(pBin, nWidth, nHeight, pImageBlock->Format, &pPVRTexture) == 0)
		{
			FILE* fp = Fopen(m_pPngName, "wb");
			if (fp == nullptr)
			{
				delete pPVRTexture;
				bResult = false;
				break;
			}
			png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
			if (png_ptr == nullptr)
			{
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: png_create_write_struct error\n\n");
				bResult = false;
				break;
			}
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if (info_ptr == nullptr)
			{
				png_destroy_write_struct(&png_ptr, (png_infopp)nullptr);
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: png_create_info_struct error\n\n");
				bResult = false;
				break;
			}
			if (setjmp(png_jmpbuf(png_ptr)) != 0)
			{
				png_destroy_write_struct(&png_ptr, &info_ptr);
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: setjmp error\n\n");
				bResult = false;
				break;
			}
			png_init_io(png_ptr, fp);
			png_set_IHDR(png_ptr, info_ptr, pImageBlock->Width, pImageBlock->Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			u8* pData = static_cast<u8*>(pPVRTexture->getDataPtr());
			png_bytepp pRowPointers = new png_bytep[pImageBlock->Height];
			for (n32 i = 0; i < pImageBlock->Height; i++)
			{
				pRowPointers[i] = pData + i * nWidth * 4;
			}
			png_set_rows(png_ptr, info_ptr, pRowPointers);
			png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&png_ptr, &info_ptr);
			delete[] pRowPointers;
			fclose(fp);
			delete pPVRTexture;
		}
		else
		{
			printf("ERROR: decode error\n\n");
			bResult = false;
			break;
		}
	} while (false);
	delete[] pBin;
	return bResult;
}

bool CBclim::EncodeFile()
{
	bool bResult = true;
	m_fpBclim = Fopen(m_pFileName, "rb");
	if (m_fpBclim == nullptr)
	{
		return false;
	}
	Fseek(m_fpBclim, 0, SEEK_END);
	n64 nFileSize = Ftell(m_fpBclim);
	Fseek(m_fpBclim, 0, SEEK_SET);
	u8* pBin = new u8[static_cast<size_t>(nFileSize)];
	fread(pBin, 1, static_cast<size_t>(nFileSize), m_fpBclim);
	fclose(m_fpBclim);
	SBclimHeader* pBclimHeader = reinterpret_cast<SBclimHeader*>(pBin + nFileSize - (sizeof(SBclimHeader) + sizeof(SImageBlock)));
	SImageBlock* pImageBlock = reinterpret_cast<SImageBlock*>(pBin + nFileSize - sizeof(SImageBlock));
	do
	{
		if (pImageBlock->Format < kTextureFormatL8 || pImageBlock->Format > kTextureFormatA4)
		{
			printf("ERROR: unknown format %d\n\n", pImageBlock->Format);
			bResult = false;
			break;
		}
		n32 nWidth = getBoundSize(pImageBlock->Width);
		n32 nHeight = getBoundSize(pImageBlock->Height);
		n32 nCheckSize = nWidth * nHeight * s_nBPP[pImageBlock->Format] / 8;
		if (pImageBlock->ImageSize != nCheckSize && m_bVerbose)
		{
			printf("INFO: width: %X(%X), height: %X(%X), checksize: %X, size: %X, bpp: %d, format: %0X\n", nWidth, pImageBlock->Width, nHeight, pImageBlock->Height, nCheckSize, pImageBlock->ImageSize, pImageBlock->ImageSize * 8 / nWidth / nHeight, pImageBlock->Format);
		}
		FILE* fp = Fopen(m_pPngName, "rb");
		if (fp == nullptr)
		{
			bResult = false;
			break;
		}
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
		if (png_ptr == nullptr)
		{
			fclose(fp);
			printf("ERROR: png_create_read_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == nullptr)
		{
			png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop end_info = png_create_info_struct(png_ptr);
		if (end_info == nullptr)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		if (setjmp(png_jmpbuf(png_ptr)) != 0)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(fp);
			printf("ERROR: setjmp error\n\n");
			bResult = false;
			break;
		}
		png_init_io(png_ptr, fp);
		png_read_info(png_ptr, info_ptr);
		n32 nPngWidth = png_get_image_width(png_ptr, info_ptr);
		n32 nPngHeight = png_get_image_height(png_ptr, info_ptr);
		n32 nBitDepth = png_get_bit_depth(png_ptr, info_ptr);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nBitDepth != 8\n\n");
			bResult = false;
			break;
		}
		n32 nColorType = png_get_color_type(png_ptr, info_ptr);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nColorType != PNG_COLOR_TYPE_RGB_ALPHA\n\n");
			bResult = false;
			break;
		}
		nWidth = getBoundSize(nPngWidth);
		nHeight = getBoundSize(nPngHeight);
		u8* pData = new unsigned char[nWidth * nHeight * 4];
		png_bytepp pRowPointers = new png_bytep[nHeight];
		for (n32 i = 0; i < nHeight; i++)
		{
			pRowPointers[i] = pData + i * nWidth * 4;
		}
		png_read_image(png_ptr, pRowPointers);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		for (n32 i = 0; i < nPngHeight; i++)
		{
			for (n32 j = nPngWidth; j < nWidth; j++)
			{
				*reinterpret_cast<u32*>(pRowPointers[i] + j * 4) = *reinterpret_cast<u32*>(pRowPointers[i] + (nPngWidth - 1) * 4);
			}
		}
		for (n32 i = nPngHeight; i < nHeight; i++)
		{
			memcpy(pRowPointers[i], pRowPointers[nPngHeight - 1], nWidth * 4);
		}
		delete[] pRowPointers;
		fclose(fp);
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		bool bSame = false;
		if (pImageBlock->Width == nPngWidth && pImageBlock->Height == nPngHeight && decode(pBin, nWidth, nHeight, pImageBlock->Format, &pPVRTexture) == 0)
		{
			u8* pTextureData = static_cast<u8*>(pPVRTexture->getDataPtr());
			bSame = true;
			for (n32 i = 0; i < nPngHeight; i++)
			{
				if (memcmp(pTextureData + i * nWidth * 4, pData + i * nWidth * 4, nPngWidth * 4) != 0)
				{
					bSame = false;
					break;
				}
			}
		}
		delete pPVRTexture;
		if (!bSame)
		{
			m_fpBclim = Fopen(m_pFileName, "wb");
			if (m_fpBclim != nullptr)
			{
				u8* pBuffer = nullptr;
				encode(pData, nWidth, nHeight, pImageBlock->Format, 1, s_nBPP[pImageBlock->Format], &pBuffer);
				pImageBlock->Width = nPngWidth;
				pImageBlock->Height = nPngHeight;
				pImageBlock->ImageSize = nWidth * nHeight * s_nBPP[pImageBlock->Format] / 8;
				pBclimHeader->FileSize = pImageBlock->ImageSize + sizeof(*pBclimHeader) + sizeof(*pImageBlock);
				fwrite(pBuffer, 1, pImageBlock->ImageSize, m_fpBclim);
				fwrite(pBclimHeader, sizeof(*pBclimHeader), 1, fp);
				fwrite(pImageBlock, sizeof(*pImageBlock), 1, fp);
				delete[] pBuffer;
				fclose(m_fpBclim);
			}
			else
			{
				bResult = false;
			}
		}
		delete[] pData;
	} while (false);
	delete[] pBin;
	return bResult;
}

bool CBclim::CreateFile()
{
	bool bResult = true;
	do
	{
		FILE* fp = Fopen(m_pPngName, "rb");
		if (fp == nullptr)
		{
			bResult = false;
			break;
		}
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
		if (png_ptr == nullptr)
		{
			fclose(fp);
			printf("ERROR: png_create_read_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == nullptr)
		{
			png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop end_info = png_create_info_struct(png_ptr);
		if (end_info == nullptr)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		if (setjmp(png_jmpbuf(png_ptr)) != 0)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(fp);
			printf("ERROR: setjmp error\n\n");
			bResult = false;
			break;
		}
		png_init_io(png_ptr, fp);
		png_read_info(png_ptr, info_ptr);
		n32 nPngWidth = png_get_image_width(png_ptr, info_ptr);
		n32 nPngHeight = png_get_image_height(png_ptr, info_ptr);
		n32 nBitDepth = png_get_bit_depth(png_ptr, info_ptr);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nBitDepth != 8\n\n");
			bResult = false;
			break;
		}
		n32 nColorType = png_get_color_type(png_ptr, info_ptr);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nColorType != PNG_COLOR_TYPE_RGB_ALPHA\n\n");
			bResult = false;
			break;
		}
		n32 nWidth = getBoundSize(nPngWidth);
		n32 nHeight = getBoundSize(nPngHeight);
		u8* pData = new unsigned char[nWidth * nHeight * 4];
		png_bytepp pRowPointers = new png_bytep[nHeight];
		for (n32 i = 0; i < nHeight; i++)
		{
			pRowPointers[i] = pData + i * nWidth * 4;
		}
		png_read_image(png_ptr, pRowPointers);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		for (n32 i = 0; i < nPngHeight; i++)
		{
			for (n32 j = nPngWidth; j < nWidth; j++)
			{
				*reinterpret_cast<u32*>(pRowPointers[i] + j * 4) = *reinterpret_cast<u32*>(pRowPointers[i] + (nPngWidth - 1) * 4);
			}
		}
		for (n32 i = nPngHeight; i < nHeight; i++)
		{
			memcpy(pRowPointers[i], pRowPointers[nPngHeight - 1], nWidth * 4);
		}
		delete[] pRowPointers;
		fclose(fp);
		m_fpBclim = Fopen(m_pFileName, "wb");
		if (m_fpBclim != nullptr)
		{
			SImageBlock imageBlock;
			imageBlock.Signature = s_uSignatureImage;
			imageBlock.HeaderSize = 0x10;
			imageBlock.Width = nPngWidth;
			imageBlock.Height = nPngHeight;
			imageBlock.Format = m_eTextureFormat;
			imageBlock.ImageSize = nWidth * nHeight * s_nBPP[imageBlock.Format] / 8;
			SBclimHeader bclimHeader;
			bclimHeader.Signature = s_uSignatureBclim;
			bclimHeader.ByteOrder = 0xFEFF;
			bclimHeader.HeaderSize = sizeof(bclimHeader);
			bclimHeader.Version = 0x02020000;
			bclimHeader.FileSize = imageBlock.ImageSize + sizeof(bclimHeader) + sizeof(imageBlock);
			bclimHeader.DataBlocks = 1;
			bclimHeader.Reserved = 0;
			u8* pBuffer = nullptr;
			encode(pData, nWidth, nHeight, imageBlock.Format, 1, s_nBPP[imageBlock.Format], &pBuffer);
			fwrite(pBuffer, 1, imageBlock.ImageSize, m_fpBclim);
			fwrite(&bclimHeader, sizeof(bclimHeader), 1, fp);
			fwrite(&imageBlock, sizeof(imageBlock), 1, fp);
			delete[] pBuffer;
			fclose(m_fpBclim);
		}
		else
		{
			bResult = false;
		}
		delete[] pData;
	} while (false);
	return bResult;
}

bool CBclim::IsBclimFile(const char* a_pFileName)
{
	FILE* fp = Fopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
	if (nFileSize < sizeof(SBclimHeader) + sizeof(SImageBlock))
	{
		return false;
	}
	Fseek(fp, nFileSize - (sizeof(SBclimHeader) + sizeof(SImageBlock)), SEEK_SET);
	SBclimHeader bclimHeader;
	fread(&bclimHeader, sizeof(bclimHeader), 1, fp);
	fclose(fp);
	return bclimHeader.Signature == s_uSignatureBclim;
}

n32 CBclim::getBoundSize(n32 a_nSize)
{
	n32 nSize = 8;
	while (nSize < a_nSize)
	{
		nSize *= 2;
	}
	return nSize;
}

int CBclim::decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture)
{
	u8* pRGBA = nullptr;
	u8* pAlpha = nullptr;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pTemp[(i * 64 + j) * 4 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pRGBA[(i * a_nWidth + j) * 4 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGB888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pTemp[(i * 64 + j) * 3 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pRGBA[(i * a_nWidth + j) * 3 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGBA5551:
	case kTextureFormatRGB565:
	case kTextureFormatRGBA4444:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatLA88:
	case kTextureFormatHL8:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL8:
	case kTextureFormatA8:
	case kTextureFormatLA44:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					pTemp[i * 64 + j] = a_pBuffer[i * 64 + s_nDecodeTransByte[j]];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL4:
	case kTextureFormatA4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j += 2)
				{
					pTemp[i * 64 + j] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] & 0xF) * 0x11;
					pTemp[i * 64 + j + 1] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] >> 4 & 0xF) * 0x11;
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[i * 8 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1_A4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[8 + i * 16 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
			pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 16; i++)
			{
				for (n32 j = 0; j < 4; j++)
				{
					pTemp[i * 16 + j] = (a_pBuffer[i * 16 + j * 2] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 4] = (a_pBuffer[i * 16 + j * 2] >> 4 & 0x0F) * 0x11;
					pTemp[i * 16 + j + 8] = (a_pBuffer[i * 16 + j * 2 + 1] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 12] = (a_pBuffer[i * 16 + j * 2 + 1] >> 4 & 0x0F) * 0x11;
				}
			}
			pAlpha = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pAlpha[i * a_nWidth + j] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4];
				}
			}
			delete[] pTemp;
		}
		break;
	}
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	case kTextureFormatETC1_A4:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	}
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	*a_pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBA);
	delete[] pRGBA;
	pvrtexture::Transcode(**a_pPVRTexture, pvrtexture::PVRStandard8PixelType, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		u8* pData = static_cast<u8*>((*a_pPVRTexture)->getDataPtr());
		for (n32 i = 0; i < a_nHeight; i++)
		{
			for (n32 j = 0; j < a_nWidth; j++)
			{
				pData[(i * a_nWidth + j) * 4 + 3] = pAlpha[i * a_nWidth + j];
			}
		}
		delete[] pAlpha;
	}
	return 0;
}

void CBclim::encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer)
{
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PVRStandard8PixelType.PixelTypeID;
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	pvrtexture::CPVRTexture* pPVRTexture = nullptr;
	pvrtexture::CPVRTexture* pPVRTextureAlpha = nullptr;
	if (a_nFormat != kTextureFormatETC1_A4)
	{
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, a_pData);
	}
	else
	{
		u8* pRGBAData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pRGBAData, a_pData, a_nWidth * a_nHeight * 4);
		u8* pAlphaData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pAlphaData, a_pData, a_nWidth * a_nHeight * 4);
		for (n32 i = 0; i < a_nWidth * a_nHeight; i++)
		{
			pRGBAData[i * 4 + 3] = 0xFF;
			pAlphaData[i * 4] = 0;
			pAlphaData[i * 4 + 1] = 0;
			pAlphaData[i * 4 + 2] = 0;
		}
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBAData);
		pPVRTextureAlpha = new pvrtexture::CPVRTexture(pvrTextureHeader, pAlphaData);
		delete[] pRGBAData;
		delete[] pAlphaData;
	}
	if (a_nMipmapLevel != 1)
	{
		pvrtexture::GenerateMIPMaps(*pPVRTexture, pvrtexture::eResizeNearest, a_nMipmapLevel);
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pvrtexture::GenerateMIPMaps(*pPVRTextureAlpha, pvrtexture::eResizeNearest, a_nMipmapLevel);
		}
	}
	pvrtexture::uint64 uPixelFormat = 0;
	pvrtexture::ECompressorQuality eCompressorQuality = pvrtexture::ePVRTCBest;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	case kTextureFormatETC1_A4:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	}
	pvrtexture::Transcode(*pPVRTexture, uPixelFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eCompressorQuality);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		pvrtexture::Transcode(*pPVRTextureAlpha, pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, pvrtexture::ePVRTCBest);
	}
	n32 nTotalSize = 0;
	n32 nCurrentSize = 0;
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		nTotalSize += (a_nWidth >> l) * (a_nHeight >> l) * a_nBPP / 8;
	}
	*a_pBuffer = new u8[nTotalSize];
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		n32 nMipmapWidth = a_nWidth >> l;
		n32 nMipmapHeight = a_nHeight >> l;
		u8* pRGBA = static_cast<u8*>(pPVRTexture->getDataPtr(l));
		u8* pAlpha = nullptr;
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pAlpha = static_cast<u8*>(pPVRTextureAlpha->getDataPtr(l));
		}
		switch (a_nFormat)
		{
		case kTextureFormatRGBA8888:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 4];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k] = pRGBA[(i * nMipmapWidth + j) * 4 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k] = pTemp[(i * 64 + j) * 4 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGB888:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 3];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k] = pRGBA[(i * nMipmapWidth + j) * 3 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k] = pTemp[(i * 64 + j) * 3 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGBA5551:
		case kTextureFormatRGB565:
		case kTextureFormatRGBA4444:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatLA88:
		case kTextureFormatHL8:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight * 2];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * nMipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL8:
		case kTextureFormatA8:
		case kTextureFormatLA44:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						pMipmapBuffer[i * 64 + s_nDecodeTransByte[j]] = pTemp[i * 64 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL4:
		case kTextureFormatA4:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[((i / 8) * (nMipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * nMipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j += 2)
					{
						pMipmapBuffer[i * 32 + s_nDecodeTransByte[j] / 2] = ((pTemp[i * 64 + j] / 0x11) & 0x0F) | ((pTemp[i * 64 + j + 1] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
				for (n32 i = 0; i < nMipmapHeight; i += 4)
				{
					for (n32 j = 0; j < nMipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[i * 8 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1_A4:
			{
				u8* pTemp = new u8[nMipmapWidth * nMipmapHeight / 2];
				for (n32 i = 0; i < nMipmapHeight; i += 4)
				{
					for (n32 j = 0; j < nMipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (nMipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[8 + i * 16 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
				pTemp = new u8[nMipmapWidth * nMipmapHeight];
				for (n32 i = 0; i < nMipmapHeight; i++)
				{
					for (n32 j = 0; j < nMipmapWidth; j++)
					{
						pTemp[(((i / 8) * (nMipmapWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4] = pAlpha[i * nMipmapWidth + j];
					}
				}
				for (n32 i = 0; i < nMipmapWidth * nMipmapHeight / 16; i++)
				{
					for (n32 j = 0; j < 4; j++)
					{
						pMipmapBuffer[i * 16 + j * 2] = ((pTemp[i * 16 + j] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 4] / 0x11) << 4 & 0xF0);
						pMipmapBuffer[i * 16 + j * 2 + 1] = ((pTemp[i * 16 + j + 8] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 12] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		}
		nCurrentSize += nMipmapWidth * nMipmapHeight * a_nBPP / 8;
	}
	delete pPVRTexture;
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		delete pPVRTextureAlpha;
	}
}
