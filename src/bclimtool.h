#ifndef BCLIMTOOL_H_
#define BCLIMTOOL_H_

#include "utility.h"
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
		const char* Name;
		int Key;
		const char* Doc;
	};
	CBclimTool();
	~CBclimTool();
	int ParseOptions(int a_nArgc, char* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, char* a_pArgv[]);
	bool decodeFile();
	bool encodeFile();
	bool createFile();
	EAction m_eAction;
	const char* m_pFileName;
	const char* m_pPngName;
	CBclim::ETextureFormat m_eTextureFormat;
	bool m_bVerbose;
	const char* m_pMessage;
};

#endif	// BCLIMTOOL_H_
