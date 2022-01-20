#pragma once
#ifndef __IK_LOGGER_H__
#	define __IK_LOGGER_H__
#	include "hIK.h"

// #define SMOOTH_LOGGING
// #define PROFILE_IK

#	ifdef __cplusplus
extern "C" {
#	endif

const char *file_short(const char *file_f);

HIK_API void LogInfo(const char *file, unsigned int line, const char *info);
HIK_API void LogInfoErr(const char* file, unsigned int line, const char *info);

HIK_API void LogInfoWCharPtr(const char *file, unsigned int line, const char *token, const wchar_t* v);
HIK_API void LogInfoCharPtr(const char *file, unsigned int line, const char *token, const char* v);
HIK_API void LogInfoPtr(const char *file, unsigned int line, const char *token, const void* v);
HIK_API void LogInfoInt(const char *file, unsigned int line, const char *token, int v);
HIK_API void LogInfoUint(const char *file, unsigned int line, const char *token, unsigned int v);
HIK_API void LogInfoBool(const char *file, unsigned int line, const char *token, bool v);
HIK_API void LogInfoFloat(const char *file, unsigned int line, const char *token, float v);
HIK_API void LogInfoFloat3x3_m(const char *file, unsigned int line, const char *token, const float m[3][3]);
HIK_API void LogInfoDouble3x3(const char *file, unsigned int line, const char *token, const double *m);
HIK_API void LogInfoDouble1x3(const char *file, unsigned int line, const char *token, const double *v);
HIK_API void LogInfoTM(const char *file, unsigned int line, const char *token, const FTransform *v);

void AssertionFail(const char *file, unsigned int line);

#  define DECLARE_ENUMLOG(LogInfoEnum_x) \
	void LogInfoEnum_x(const char *file, unsigned int line, const char *token, short type);

#  define DECLARE_FLAGLOG(LogInfoFlag_x) \
	void LogInfoFlag_x(const char *file, unsigned int line, const char *token, short flag);

#  if defined _DEBUG

#	define LOGIKVar(func, var) func(__FILE__, __LINE__, #    var, var);
#		define LOGIKVarRel(msg)
#		define LOGIKVarErr(func, var)\
			func(__FILE__, __LINE__, "ERROR: "#var, var);\
			LOGIKFlush();
#		define LOGIKVarWarning(func, var)\
			func(__FILE__, __LINE__, "WARNING: "#var, var);\
			LOGIKFlush();
#	define LOGIK(msg) LogInfo(__FILE__, __LINE__, msg);
#		define LOGIKErr(msg) LogInfoErr(__FILE__, __LINE__, msg);
#	if defined HARDASSERTION
#		define IKAssert check
#	else
#		define IKAssert(v)\
			if(!(v))\
				AssertionFail(__FILE__, __LINE__);
#	endif
#  else

#	if !defined(NDEBUG)
#		error Delcare for no-debugging purpose
#	endif

#	define LOGIKVar(func, var)
#		define LOGIKVarRel(func, var) func(__FILE__, __LINE__, #    var, var);
#		define LOGIKVarErr(func, var)\
			func(__FILE__, __LINE__, "ERROR: "#var, var);\
			LOGIKFlush();
#		define LOGIKVarWarning(func, var)\
			func(__FILE__, __LINE__, "WARNING: "#var, var);\
			LOGIKFlush();
#	define LOGIK(msg)
#		define LOGIKErr(msg) LogInfoErr(__FILE__, __LINE__, msg);
#		define IKAssert(v)\
				if(!(v))\
					AssertionFail(__FILE__, __LINE__);

#  endif

#ifdef PROFILE_IK
#	define START_PROFILER(frame_id, token, rounds) \
	ULONGLONG ___tick_start = GetTickCount64(); \
	unsigned int ___line_start = __LINE__; \
	const char *___token = token; \
	unsigned int ___rounds = rounds; \
	unsigned int ___frame_id = frame_id; \
	for (int i = 0; i < rounds; i++) {

#	define STOP_PROFILER \
	} \
	ULONGLONG ___tick = GetTickCount64() - ___tick_start; \
	unsigned int ___line_end = __LINE__; \
	double ___millisec = (double)___tick / (double)___rounds; \
	LoggerFast_OutFmt("%s, %d:%d, frame_id=, %d, token=, %s, %f\n", \
				file_short(__FILE__), \
				___line_start, \
				___line_end, \
				___frame_id, \
				___token, \
				___millisec);
#else
#	define START_PROFILER(frame_id, token, rounds)
#	define STOP_PROFILER
#endif



int __cdecl LoggerFast_OutFmt(const char *fmt, ...);

DECLARE_FLAGLOG(LogInfoFlag_con)
DECLARE_ENUMLOG(LogInfoEnum_contype)
DECLARE_FLAGLOG(LogInfoFlag_bone)

#  ifdef __cplusplus
}
#  endif

#endif
