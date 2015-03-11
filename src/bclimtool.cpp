#include "bclimtool.h"

CBclimTool::SOption CBclimTool::s_Option[] =
{
	{ "decode", 'd', "decode the bclim file" },
	{ "encode", 'e', "encode the bclim file" },
	{ "create", 'c', "create the bclim file" },
	{ "file", 'f', "the bclim file" },
	{ "png", 'p', "the png file for the bclim file" },
	{ "format", 't', "[L8|A8|LA44|LA88|HL8|RGB565|RGB888|RGBA5551|RGBA4444|RGBA8888|ETC1|ETC1_A4|L4|A4]\n\t\tthe format for the bclim file, only for the create action, default is RGBA8888" },
	{ "verbose", 'v', "show the info" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, nullptr }
};

CBclimTool::CBclimTool()
	: m_eAction(kActionNone)
	, m_pFileName(nullptr)
	, m_pPngName(nullptr)
	, m_eTextureFormat(CBclim::kTextureFormatRGBA8888)
	, m_bVerbose(false)
	, m_pMessage(nullptr)
{
}

CBclimTool::~CBclimTool()
{
}

int CBclimTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(strlen(a_pArgv[i]));
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			printf("ERROR: illegal option\n\n");
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnUnknownArgument:
				printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
				return 1;
			case kParseOptionReturnOptionConflict:
				printf("ERROR: option conflict\n\n");
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int CBclimTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_pFileName == nullptr)
		{
			printf("ERROR: no --file option\n\n");
			return 1;
		}
		if (m_pPngName == nullptr)
		{
			printf("ERROR: no --png option\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionDecode || m_eAction == kActionEncode)
	{
		if (!CBclim::IsBclimFile(m_pFileName))
		{
			printf("ERROR: %s is not a bclim file\n\n", m_pFileName);
			return 1;
		}
	}
	return 0;
}

int CBclimTool::Help()
{
	printf("bclimtool %s by dnasdw\n\n", BCLIMTOOL_VERSION);
	printf("usage: bclimtool [option...] [option]...\n");
	printf("sample:\n");
	printf("  bclimtool -dvfp title.bclim title.png\n");
	printf("  bclimtool -evfp title.bclim title_new.png\n");
	printf("  bclimtool -cvtfp ETC1_A4 title_new.bclim title_new.png\n");
	printf("\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			printf("  ");
			if (pOption->Key != 0)
			{
				printf("-%c,", pOption->Key);
			}
			else
			{
				printf("   ");
			}
			printf(" --%-8s", pOption->Name);
			if (strlen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				printf("\n%16s", "");
			}
		}
		if (pOption->Doc != nullptr)
		{
			printf("%s", pOption->Doc);
		}
		printf("\n");
		pOption++;
	}
	return 0;
}

int CBclimTool::Action()
{
	if (m_eAction == kActionDecode)
	{
		if (!decodeFile())
		{
			printf("ERROR: decode file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionEncode)
	{
		if (!encodeFile())
		{
			printf("ERROR: encode file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			printf("ERROR: create file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CBclimTool::EParseOptionReturn CBclimTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "decode") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionDecode;
		}
		else if (m_eAction != kActionDecode && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "encode") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionEncode;
		}
		else if (m_eAction != kActionEncode && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "create") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "png") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pPngName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "format") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		char* pType = a_pArgv[++a_nIndex];
		if (strcmp(pType, "L8") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatL8;
		}
		else if (strcmp(pType, "A8") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatA8;
		}
		else if (strcmp(pType, "LA44") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatLA44;
		}
		else if (strcmp(pType, "LA88") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatLA88;
		}
		else if (strcmp(pType, "HL8") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatHL8;
		}
		else if (strcmp(pType, "RGB565") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGB565;
		}
		else if (strcmp(pType, "RGB888") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGB888;
		}
		else if (strcmp(pType, "RGBA5551") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA5551;
		}
		else if (strcmp(pType, "RGBA4444") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA4444;
		}
		else if (strcmp(pType, "RGBA8888") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA8888;
		}
		else if (strcmp(pType, "ETC1") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatETC1;
		}
		else if (strcmp(pType, "ETC1_A4") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatETC1_A4;
		}
		else if (strcmp(pType, "L4") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatL4;
		}
		else if (strcmp(pType, "A4") == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatA4;
		}
		else
		{
			m_pMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CBclimTool::EParseOptionReturn CBclimTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool CBclimTool::decodeFile()
{
	CBclim bclim;
	bclim.SetFileName(m_pFileName);
	bclim.SetPngName(m_pPngName);
	bclim.SetVerbose(m_bVerbose);
	return bclim.DecodeFile();
}

bool CBclimTool::encodeFile()
{
	CBclim bclim;
	bclim.SetFileName(m_pFileName);
	bclim.SetPngName(m_pPngName);
	bclim.SetVerbose(m_bVerbose);
	return bclim.EncodeFile();
}

bool CBclimTool::createFile()
{
	CBclim bclim;
	bclim.SetFileName(m_pFileName);
	bclim.SetPngName(m_pPngName);
	bclim.SetTextureFormat(m_eTextureFormat);
	bclim.SetVerbose(m_bVerbose);
	return bclim.CreateFile();
}

int main(int argc, char* argv[])
{
	FSetLocale();
	CBclimTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
