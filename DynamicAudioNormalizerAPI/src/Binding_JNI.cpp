//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#include "DynamicAudioNormalizer.h"

//StdLib
#include <algorithm>
#include <stdexcept>
#include <map>
#include <queue>
#include <climits>

//Generate the JNI header file name
#define JAVA_HDRNAME_GLUE1(X,Y) <##X##_r##Y##.h>
#define JAVA_HDRNAME_GLUE2(X,Y) JAVA_HDRNAME_GLUE1(X,Y)
#define JAVA_HDRNAME(X) JAVA_HDRNAME_GLUE2(X, MDYNAMICAUDIONORMALIZER_CORE)

//Generate JNI export names and corresponding helper functions
#define JAVA_FUNCTION_GLUE1(X,Y,Z) X##Y##_##Z
#define JAVA_FUNCTION_GLUE2(X,Y,Z) JAVA_FUNCTION_GLUE1(X,Y,Z)
#define JAVA_FUNCTION(X) JAVA_FUNCTION_GLUE2(Java_com_muldersoft_dynaudnorm_JDynamicAudioNormalizer_00024NativeAPI_1r, MDYNAMICAUDIONORMALIZER_CORE, X)
#define JAVA_FUNCIMPL(X) JAVA_FUNCTION_GLUE2(Impl_com_muldersoft_dynaudnorm_JDynamicAudioNormalizer_00024NativeAPI_1r, MDYNAMICAUDIONORMALIZER_CORE, X)

//JNI Headers
#include JAVA_HDRNAME(com_muldersoft_dynaudnorm_JDynamicAudioNormalizer_NativeAPI)

//PThread
#if defined(_WIN32) && defined(_MT)
#define PTW32_STATIC_LIB 1
#endif
#include <pthread.h>

//Internal
#include <Common.h>

//Globals
static pthread_mutex_t g_javaLock = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////

#define JAVA_FIND_CLASS(VAR,NAME) do \
{ \
	(VAR) = env->FindClass((NAME)); \
	if((VAR) == NULL) return JNI_FALSE; \
} \
while(0)

#define JAVA_GET_METHOD(VAR,CLASS,NAME,ARGS) do \
{ \
	(VAR) = env->GetMethodID((CLASS), (NAME), (ARGS)); \
	if((VAR) == NULL) return JNI_FALSE; \
} \
while(0)

#define JAVA_MAP_PUT(MAP,KEY,VAL) do \
{ \
	jstring _key = env->NewStringUTF((KEY)); \
	jstring _val = env->NewStringUTF((VAL)); \
	\
	if(_key && _val) \
	{ \
		jobject _ret = env->CallObjectMethod((MAP), putMethod, _key, _val); \
		if(_ret) env->DeleteLocalRef(_ret); \
	} \
	\
	if(_key) env->DeleteLocalRef(_key); \
	if(_val) env->DeleteLocalRef(_val); \
} \
while(0)

#define JAVA_THROW_EXCEPTION(TEXT, RET) do \
{ \
	if(jclass runtimeError = env->FindClass("java/lang/Error")) \
	{ \
		env->ThrowNew(runtimeError, (TEXT)); \
	} \
	return (RET); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Logging  Function
///////////////////////////////////////////////////////////////////////////////

static JavaVM *g_javaLoggingJVM = NULL;
static jobject g_javaLoggingHandler = NULL;

static jboolean javaLogMessage_Helper(JNIEnv *env, const int &level, const char *const message)
{
	jclass loggerClass = NULL;
	jmethodID logMethod = NULL;

	JAVA_FIND_CLASS(loggerClass, "com/muldersoft/dynaudnorm/JDynamicAudioNormalizer$Logger");
	JAVA_GET_METHOD(logMethod, loggerClass, "log", "(ILjava/lang/String;)V");

	jstring text = env->NewStringUTF(message);
	if(text)
	{
		env->CallVoidMethod(g_javaLoggingHandler, logMethod, level, text);
		env->DeleteLocalRef(text);
	}

	env->DeleteLocalRef(loggerClass);
	return JNI_TRUE;
}

static void javaLogMessage(const int logLevel, const char *const message)
{
	MY_CRITSEC_ENTER(g_javaLock);
	if(g_javaLoggingHandler && g_javaLoggingJVM)
	{
		JNIEnv *env = NULL;
		if(g_javaLoggingJVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK)
		{
			javaLogMessage_Helper(env, logLevel, message);
		}
	}
	MY_CRITSEC_LEAVE(g_javaLock);
}

static void javaSetLoggingHandler(JNIEnv *env, jobject loggerGlobalReference)
{
	MY_CRITSEC_ENTER(g_javaLock);
	if(g_javaLoggingHandler)
	{
		env->DeleteGlobalRef(g_javaLoggingHandler);
		g_javaLoggingHandler = NULL;
	}
	if(loggerGlobalReference)
	{
		if(env->GetJavaVM(&g_javaLoggingJVM) == 0)
		{
			g_javaLoggingHandler = loggerGlobalReference;
			MDynamicAudioNormalizer::setLogFunction(javaLogMessage);
		}
	}
	MY_CRITSEC_LEAVE(g_javaLock);
}

///////////////////////////////////////////////////////////////////////////////
// Instance Handling
///////////////////////////////////////////////////////////////////////////////

static jint g_instanceNextHandleValue = 0;
static std::map<jint, MDynamicAudioNormalizer*> g_instances;
static std::queue<jint> g_pendingHandles;

static int javaCreateHandle(MDynamicAudioNormalizer *const instance)
{
	MY_CRITSEC_ENTER(g_javaLock);
	
	jint handleValue = -1;
	if(!g_pendingHandles.empty())
	{
		handleValue = g_pendingHandles.front();
		g_pendingHandles.pop();
	}
	else if(g_instanceNextHandleValue < SHRT_MAX)
	{
		handleValue = g_instanceNextHandleValue++;
	}

	if(handleValue >= 0)
	{
		g_instances.insert(std::pair<jint, MDynamicAudioNormalizer*>(handleValue, instance));
	}

	MY_CRITSEC_LEAVE(g_javaLock);
	return handleValue;
}

static void javaCloseHandle(const jint &handleValue)
{
	MY_CRITSEC_ENTER(g_javaLock);
	
	if(g_instances.find(handleValue) != g_instances.end())
	{
		g_instances.erase(handleValue);
		g_pendingHandles.push(handleValue);
	}

	MY_CRITSEC_LEAVE(g_javaLock);
}

static MDynamicAudioNormalizer *javaHandleToInstance(const jint &handleValue)
{
	MY_CRITSEC_ENTER(g_javaLock);
	
	MDynamicAudioNormalizer *instance = NULL;
	if(g_instances.find(handleValue) != g_instances.end())
	{
		instance = g_instances[handleValue];
	}

	MY_CRITSEC_LEAVE(g_javaLock);
	return instance;
}

///////////////////////////////////////////////////////////////////////////////
// JNI Functions
///////////////////////////////////////////////////////////////////////////////

static jboolean JAVA_FUNCIMPL(getVersionInfo)(JNIEnv *env, jintArray versionInfo)
{
	if(versionInfo == NULL)
	{
		return JNI_FALSE;
	}

	if(env->GetArrayLength(versionInfo) == 3)
	{
		if(jint *versionInfoElements = env->GetIntArrayElements(versionInfo, JNI_FALSE))
		{
			uint32_t major, minor, patch;
			MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
			
			versionInfoElements[0] = (int32_t) std::min(major, uint32_t(INT32_MAX));
			versionInfoElements[1] = (int32_t) std::min(minor, uint32_t(INT32_MAX));
			versionInfoElements[2] = (int32_t) std::min(patch, uint32_t(INT32_MAX));

			env->ReleaseIntArrayElements(versionInfo, versionInfoElements, 0);
			return JNI_TRUE;
		}
	}

	return JNI_FALSE;
}

static jboolean JAVA_FUNCIMPL(getBuildInfo)(JNIEnv *env, jobject buildInfo)
{
	if(buildInfo == NULL)
	{
		return JNI_FALSE;
	}

	jclass mapClass = NULL;
	jmethodID putMethod = NULL, clearMethod = NULL;

	JAVA_FIND_CLASS(mapClass, "java/util/Map");
	JAVA_GET_METHOD(putMethod, mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	JAVA_GET_METHOD(clearMethod, mapClass, "clear", "()V");

	if(!env->IsInstanceOf(buildInfo, mapClass))
	{
		return JNI_FALSE;
	}

	env->CallVoidMethod(buildInfo, clearMethod);

	const char *date, *time, *compiler, *arch;
	bool debug;
	MDynamicAudioNormalizer::getBuildInfo(&date, &time, &compiler, &arch, debug);

	JAVA_MAP_PUT(buildInfo, "BuildDate",    date);
	JAVA_MAP_PUT(buildInfo, "BuildTime",    time);
	JAVA_MAP_PUT(buildInfo, "Compiler",     compiler);
	JAVA_MAP_PUT(buildInfo, "Architecture", arch);
	JAVA_MAP_PUT(buildInfo, "DebugBuild",   debug ? "Yes" : "No");

	env->DeleteLocalRef(mapClass);
		
	return JNI_TRUE;
}

static jboolean JAVA_FUNCIMPL(setLoggingHandler)(JNIEnv *env, jobject loggerObject)
{
	if(loggerObject == NULL)
	{
		javaSetLoggingHandler(env, NULL);
		return JNI_TRUE;
	}

	jclass loggerClass = NULL;
	JAVA_FIND_CLASS(loggerClass, "com/muldersoft/dynaudnorm/JDynamicAudioNormalizer$Logger");

	if(!env->IsInstanceOf(loggerObject, loggerClass))
	{
		return JNI_FALSE;
	}

	jobject globalReference = env->NewGlobalRef(loggerObject);
	if(globalReference)
	{
		javaSetLoggingHandler(env, globalReference);
		return JNI_TRUE;
	}

	env->DeleteLocalRef(loggerClass);
	return JNI_FALSE;
}

static jint JAVA_FUNCIMPL(createInstance)(JNIEnv *env, jint channels, jint sampleRate, jint frameLenMsec, jint filterSize, jdouble peakValue, jdouble maxAmplification, jdouble targetRms, jdouble compressFactor, jboolean channelsCoupled, jboolean enableDCCorrection, jboolean altBoundaryMode)
{
	if((channels > 0) && (sampleRate > 0) && (frameLenMsec > 0) & (filterSize > 0))
	{
		MDynamicAudioNormalizer *instance = new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, (channelsCoupled != 0), (enableDCCorrection != 0), (altBoundaryMode != 0));
		if(!instance->initialize())
		{
			delete instance;
			return -1;
		}

		const jint handle = javaCreateHandle(instance);
		if(handle < 0)
		{
			delete instance;
			return -1;
		}

		return handle;
	}
	return -1;
}

static jboolean JAVA_FUNCIMPL(destroyInstance)(JNIEnv *env, jint handle)
{
	if(MDynamicAudioNormalizer *instance = javaHandleToInstance(handle))
	{
		javaCloseHandle(handle);
		delete instance;
		return JNI_TRUE;
	}

	return JNI_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// JNI Entry Points
///////////////////////////////////////////////////////////////////////////////

extern "C"
{
	JNIEXPORT jboolean JNICALL JAVA_FUNCTION(getVersionInfo)(JNIEnv *env, jobject, jintArray versionInfo)
	{
		try
		{
			return JAVA_FUNCIMPL(getVersionInfo)(env, versionInfo);
		}
		catch(std::exception e)
		{
			JAVA_THROW_EXCEPTION(e.what(), JNI_FALSE);
		}
		catch(...)
		{
			JAVA_THROW_EXCEPTION("An unknown C++ exception has occurred!", JNI_FALSE);
		}
	}

	JNIEXPORT jboolean JNICALL JAVA_FUNCTION(getBuildInfo)(JNIEnv *env, jobject, jobject buildInfo)
	{
		try
		{
			return JAVA_FUNCIMPL(getBuildInfo)(env, buildInfo);
		}
		catch(std::exception e)
		{
			JAVA_THROW_EXCEPTION(e.what(), JNI_FALSE);
		}
		catch(...)
		{
			JAVA_THROW_EXCEPTION("An unknown C++ exception has occurred!", JNI_FALSE);
		}
	}

	JNIEXPORT jboolean JNICALL JAVA_FUNCTION(setLoggingHandler)(JNIEnv *env, jobject, jobject loggerObject)
	{
		try
		{
			return JAVA_FUNCIMPL(setLoggingHandler)(env, loggerObject);
		}
		catch(std::exception e)
		{
			JAVA_THROW_EXCEPTION(e.what(), JNI_FALSE);
		}
		catch(...)
		{
			JAVA_THROW_EXCEPTION("An unknown C++ exception has occurred!", JNI_FALSE);
		}
	}

	JNIEXPORT jint JNICALL JAVA_FUNCTION(createInstance)(JNIEnv *env, jobject, jint channels, jint sampleRate, jint frameLenMsec, jint filterSize, jdouble peakValue, jdouble maxAmplification, jdouble targetRms, jdouble compressFactor, jboolean channelsCoupled, jboolean enableDCCorrection, jboolean altBoundaryMode)
	{
		try
		{
			return JAVA_FUNCIMPL(createInstance)(env, channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
		}
		catch(std::exception e)
		{
			JAVA_THROW_EXCEPTION(e.what(), -1);
		}
		catch(...)
		{
			JAVA_THROW_EXCEPTION("An unknown C++ exception has occurred!", -1);
		}
	}

	JNIEXPORT jboolean JNICALL JAVA_FUNCTION(destroyInstance)(JNIEnv *env, jobject, jint instance)
	{
		try
		{
			return JAVA_FUNCIMPL(destroyInstance)(env, instance);
		}
		catch(std::exception e)
		{
			JAVA_THROW_EXCEPTION(e.what(), JNI_FALSE);
		}
		catch(...)
		{
			JAVA_THROW_EXCEPTION("An unknown C++ exception has occurred!", JNI_FALSE);
		}
	}
}
