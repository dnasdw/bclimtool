#ifndef BCLIMTOOL_H_
#define BCLIMTOOL_H_

#include <sdw.h>
#include "bclim.h"

class CBclimTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionDecode,
		kActionEncode,
		kActionCreate,
		kActionHelp
	};
	struct SOption
	{
		const UChar* Name;
		int Key;
		const UChar* Doc;
	};
	CBclimTool();
	~CBclimTool();
	int ParseOptions(int a_nArgc, UChar* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const UChar* a_pName, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, UChar* a_pArgv[]);
	bool decodeFile();
	bool encodeFile();
	bool createFile();
	EAction m_eAction;
	UString m_sFileName;
	UString m_sPngName;
	CBclim::ETextureFormat m_eTextureFormat;
	bool m_bVerbose;
	UString m_sMessage;
};

#endif	// BCLIMTOOL_H_
