#include "bclimtool.h"

CBclimTool::SOption CBclimTool::s_Option[] =
{
	{ USTR("decode"), USTR('d'), USTR("decode the target file") },
	{ USTR("encode"), USTR('e'), USTR("encode the target file") },
	{ USTR("create"), USTR('c'), USTR("create the target file") },
	{ USTR("file"), USTR('f'), USTR("the target file") },
	{ USTR("png"), USTR('p'), USTR("the png file for the target file") },
	{ USTR("format"), USTR('t'), USTR("[L8|A8|LA44|LA88|HL8|RGB565|RGB888|RGBA5551|RGBA4444|RGBA8888|ETC1|ETC1_A4|L4|A4]\n\t\tthe format for the target file, only for the create action, default is RGBA8888") },
	{ USTR("verbose"), USTR('v'), USTR("show the info") },
	{ USTR("help"), USTR('h'), USTR("show this help") },
	{ nullptr, 0, nullptr }
};

CBclimTool::CBclimTool()
	: m_eAction(kActionNone)
	, m_eTextureFormat(CBclim::kTextureFormatRGBA8888)
	, m_bVerbose(false)
{
}

CBclimTool::~CBclimTool()
{
}

int CBclimTool::ParseOptions(int a_nArgc, UChar* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(UCslen(a_pArgv[i]));
		if (nArgpc == 0)
		{
			continue;
		}
		int nIndex = i;
		if (a_pArgv[i][0] != USTR('-'))
		{
			UPrintf(USTR("ERROR: illegal option\n\n"));
			return 1;
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != USTR('-'))
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					UPrintf(USTR("ERROR: illegal option\n\n"));
					return 1;
				case kParseOptionReturnNoArgument:
					UPrintf(USTR("ERROR: no argument\n\n"));
					return 1;
				case kParseOptionReturnUnknownArgument:
					UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
					return 1;
				case kParseOptionReturnOptionConflict:
					UPrintf(USTR("ERROR: option conflict\n\n"));
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == USTR('-'))
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				UPrintf(USTR("ERROR: illegal option\n\n"));
				return 1;
			case kParseOptionReturnNoArgument:
				UPrintf(USTR("ERROR: no argument\n\n"));
				return 1;
			case kParseOptionReturnUnknownArgument:
				UPrintf(USTR("ERROR: unknown argument \"%") PRIUS USTR("\"\n\n"), m_sMessage.c_str());
				return 1;
			case kParseOptionReturnOptionConflict:
				UPrintf(USTR("ERROR: option conflict\n\n"));
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
		UPrintf(USTR("ERROR: nothing to do\n\n"));
		return 1;
	}
	if (m_eAction != kActionHelp)
	{
		if (m_sFileName.empty())
		{
			UPrintf(USTR("ERROR: no --file option\n\n"));
			return 1;
		}
		if (m_sPngName.empty())
		{
			UPrintf(USTR("ERROR: no --png option\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionDecode || m_eAction == kActionEncode)
	{
		if (!CBclim::IsBclimFile(m_sFileName))
		{
			UPrintf(USTR("ERROR: %") PRIUS USTR(" is not a bclim file\n\n"), m_sFileName.c_str());
			return 1;
		}
	}
	return 0;
}

int CBclimTool::Help()
{
	UPrintf(USTR("bclimtool %") PRIUS USTR(" by dnasdw\n\n"), AToU(BCLIMTOOL_VERSION).c_str());
	UPrintf(USTR("usage: bclimtool [option...] [option]...\n"));
	UPrintf(USTR("sample:\n"));
	UPrintf(USTR("  bclimtool -dvfp title.bclim title.png\n"));
	UPrintf(USTR("  bclimtool -evfp title.bclim title_new.png\n"));
	UPrintf(USTR("  bclimtool -cvtfp ETC1_A4 title_new.bclim title_new.png\n"));
	UPrintf(USTR("\n"));
	UPrintf(USTR("option:\n"));
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
		{
			UPrintf(USTR("  "));
			if (pOption->Key != 0)
			{
				UPrintf(USTR("-%c,"), pOption->Key);
			}
			else
			{
				UPrintf(USTR("   "));
			}
			UPrintf(USTR(" --%-8") PRIUS, pOption->Name);
			if (UCslen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				UPrintf(USTR("\n%16") PRIUS, USTR(""));
			}
		}
		if (pOption->Doc != nullptr)
		{
			UPrintf(USTR("%") PRIUS, pOption->Doc);
		}
		UPrintf(USTR("\n"));
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
			UPrintf(USTR("ERROR: decode file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionEncode)
	{
		if (!encodeFile())
		{
			UPrintf(USTR("ERROR: encode file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			UPrintf(USTR("ERROR: create file failed\n\n"));
			return 1;
		}
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

CBclimTool::EParseOptionReturn CBclimTool::parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[])
{
	if (UCscmp(a_pName, USTR("decode")) == 0)
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
	else if (UCscmp(a_pName, USTR("encode")) == 0)
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
	else if (UCscmp(a_pName, USTR("create")) == 0)
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
	else if (UCscmp(a_pName, USTR("file")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sFileName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("png")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_sPngName = a_pArgv[++a_nIndex];
	}
	else if (UCscmp(a_pName, USTR("format")) == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		UChar* pType = a_pArgv[++a_nIndex];
		if (UCscmp(pType, USTR("L8")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatL8;
		}
		else if (UCscmp(pType, USTR("A8")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatA8;
		}
		else if (UCscmp(pType, USTR("LA44")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatLA44;
		}
		else if (UCscmp(pType, USTR("LA88")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatLA88;
		}
		else if (UCscmp(pType, USTR("HL8")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatHL8;
		}
		else if (UCscmp(pType, USTR("RGB565")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGB565;
		}
		else if (UCscmp(pType, USTR("RGB888")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGB888;
		}
		else if (UCscmp(pType, USTR("RGBA5551")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA5551;
		}
		else if (UCscmp(pType, USTR("RGBA4444")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA4444;
		}
		else if (UCscmp(pType, USTR("RGBA8888")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatRGBA8888;
		}
		else if (UCscmp(pType, USTR("ETC1")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatETC1;
		}
		else if (UCscmp(pType, USTR("ETC1_A4")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatETC1_A4;
		}
		else if (UCscmp(pType, USTR("L4")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatL4;
		}
		else if (UCscmp(pType, USTR("A4")) == 0)
		{
			m_eTextureFormat = CBclim::kTextureFormatA4;
		}
		else
		{
			m_sMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (UCscmp(a_pName, USTR("verbose")) == 0)
	{
		m_bVerbose = true;
	}
	else if (UCscmp(a_pName, USTR("help")) == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

CBclimTool::EParseOptionReturn CBclimTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, UChar* a_pArgv[])
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
	bclim.SetFileName(m_sFileName);
	bclim.SetPngName(m_sPngName);
	bclim.SetVerbose(m_bVerbose);
	return bclim.DecodeFile();
}

bool CBclimTool::encodeFile()
{
	CBclim bclim;
	bclim.SetFileName(m_sFileName);
	bclim.SetPngName(m_sPngName);
	bclim.SetVerbose(m_bVerbose);
	return bclim.EncodeFile();
}

bool CBclimTool::createFile()
{
	CBclim bclim;
	bclim.SetFileName(m_sFileName);
	bclim.SetPngName(m_sPngName);
	bclim.SetTextureFormat(m_eTextureFormat);
	bclim.SetVerbose(m_bVerbose);
	return bclim.CreateFile();
}

int UMain(int argc, UChar* argv[])
{
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
