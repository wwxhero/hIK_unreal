#include "ik_logger_unreal.h"
#include <assert.h>


#ifndef _WIN32
#define _WIN32
#endif

const char *file_short(const char *file_f)
{
#ifdef _WIN32
	#define DELIMITER '\\'
#else
	#define DELIMITER '/'
#endif
	const char* p_delim = NULL;
	for (const char* p = file_f
		; *p != '\0'
		; p ++)
	{
		if (*p == DELIMITER)
			p_delim = p;
	}
	assert(NULL != p_delim);
	return ++ p_delim;
}

void AssertionFail(const char *file, unsigned int line)
{
	FString strFile(file_short(file));
	UE_LOG(LogHIK, Error
		, TEXT("[%s:%d] ASSERTION FAILED")
		, *strFile
		, line);
}

void LogInfo(const char* file, unsigned int line, const char *info)
{
	FString strFile(file_short(file));
	FString strInfo(info);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s")
						, *strFile
						, line
						, *strInfo);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoWCharPtr(const char *file, unsigned int line, const char *token, const wchar_t* v)
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString strV(v);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %s")
									, *strFile
									, line
									, *strToken
									, *strV);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoCharPtr(const char *file, unsigned int line, const char *token, const char* v)
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString strV(v);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %s")
									, *strFile
									, line
									, *strToken
									, *strV);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoPtr(const char* file, unsigned int line, const char* token, const void* v)
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %p")
									, *strFile
									, line
									, *strToken
									, v);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoInt(const char* file, unsigned int line, const char* token, int v)
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %d")
									, *strFile
									, line
									, *strToken
									, v);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoBool(const char* file, unsigned int line, const char* token, bool v)
{
	const TCHAR* repre_b[] = {TEXT("false"), TEXT("true")};
	int repre_i = (v ? 1 : 0);
	FString strFile(file_short(file));
	FString strToken(token);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %s")
		, *strFile
		, line
		, *strToken
		, repre_b[repre_i]);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoFloat(const char* file, unsigned int line, const char* token, float v)
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = %.4f\n")
						, *strFile
						, line
						, *strToken
						, v);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

template<typename TValue>
void LogInfo3x3_m(const char* file, unsigned int line, const char* token, const TValue m[3][3])
{
	FString strFile(file_short(file));
	FString strToken(token);
	FString logItem = FString::Printf(TEXT("[%s:%d] %s = [%.8f\t%.8f\t%.8f;\t%.8f\t%.8f\t%.8f;\t%.8f\t%.8f\t%.8f]\n")
						, *strFile
						, line
						, *strToken
						, m[0][0], m[0][1], m[0][2]
						, m[1][0], m[1][1], m[1][2]
						, m[2][0], m[2][1], m[2][2]);
	UE_LOG(LogHIK, Display, TEXT("%s"), *logItem);
}

void LogInfoFloat3x3_m(const char* file, unsigned int line, const char* token, const float m[3][3])
{
	LogInfo3x3_m<float>(file, line, token, m);
}
void LogInfoDouble3x3(const char* file, unsigned int line, const char* token, const double* m)
{


}

void LogInfoDouble1x3(const char* file, unsigned int line, const char* token, const double* v)
{

}

typedef struct
{
	short value;
	const char* text;
} FlagText;

typedef FlagText EnumText;

void LogInfoFlag(short flag, FlagText* dfns, unsigned short n_dfn, const char* file, unsigned int line, const char* token)
{

}

void LogInfoEnum(short flag, EnumText* dfns, unsigned short n_dfn, const char* file, unsigned int line, const char* token)
{

}

int __cdecl LoggerFast_OutFmt(const char* _Format, ...)
{
	return -1;
}