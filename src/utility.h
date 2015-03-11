#ifndef UTILITY_H_
#define UTILITY_H_

#define COMPILER_MSC  1
#define COMPILER_GNUC 2

#if defined(_MSC_VER)
#define BCLIMTOOL_COMPILER COMPILER_MSC
#else
#define BCLIMTOOL_COMPILER COMPILER_GNUC
#endif

#if BCLIMTOOL_COMPILER == COMPILER_MSC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

typedef int8_t n8;
typedef int16_t n16;
typedef int32_t n32;
typedef int64_t n64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if BCLIMTOOL_COMPILER == COMPILER_MSC
#define FFopen FFopenA
#define FFseek _fseeki64
#define FFtell _ftelli64
#define MSC_PUSH_PACKED <pshpack1.h>
#define MSC_POP_PACKED <poppack.h>
#define GNUC_PACKED
#else
#define FFopen FFopenA
#define FFseek fseeko
#define FFtell ftello
#define MSC_PUSH_PACKED <stdlib.h>
#define MSC_POP_PACKED <stdlib.h>
#define GNUC_PACKED __attribute__((packed))
#endif

#define CONVERT_ENDIAN(n) (((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | (((n) & 0xFF00) << 8) | (((n) & 0xFF) << 24))

void FSetLocale();

template<typename T>
vector<T> FSSplit(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	for (typename T::size_type nOffset = 0; nOffset < a_sString.size(); nOffset += a_sSeparator.size())
	{
		typename T::size_type nPos = a_sString.find(a_sSeparator, nOffset);
		if (nPos != T::npos)
		{
			vString.push_back(a_sString.substr(nOffset, nPos - nOffset));
			nOffset = nPos;
		}
		else
		{
			vString.push_back(a_sString.substr(nOffset));
			break;
		}
	}
	return vString;
}

FILE* FFopenA(const char* a_pFileName, const char* a_pMode);

#endif	// UTILITY_H_
