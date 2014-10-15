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

//JNI
#include <DynamicAudioNormalizer_JDynamicAudioNormalizer.h>
#include <DynamicAudioNormalizer_JDynamicAudioNormalizer_Error.h>
#include <DynamicAudioNormalizer_JDynamicAudioNormalizer_NativeAPI.h>

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

static jobject g_javaLoggingHandler = NULL;
static jmethodID g_javaLoggingMethod = NULL;
static JavaVM *g_javaLoggingJVM = NULL;

static void javaLogMessageHelper(JNIEnv *env, const int &level, const char *const message)
{
	MY_CRITSEC_ENTER(g_javaLock);
	if(g_javaLoggingHandler && g_javaLoggingMethod)
	{
		jstring text = env->NewStringUTF(message);
		if(text)
		{
			env->CallVoidMethod(g_javaLoggingHandler, g_javaLoggingMethod, level, text);
			env->DeleteLocalRef(text);
		}
	}
	MY_CRITSEC_LEAVE(g_javaLock);
}

static void javaLogMessage(const int logLevel, const char *const message)
{
	JNIEnv *env = NULL;
	if(g_javaLoggingJVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK)
	{
		javaLogMessageHelper(env, logLevel, message);
		env->ExceptionClear();
	}
}

static void javaSetLoggingHandler(JNIEnv *env, jobject loggerGlobalReference, jmethodID loggerMethod)
{
	MY_CRITSEC_ENTER(g_javaLock);
	if(g_javaLoggingHandler)
	{
		env->DeleteGlobalRef(g_javaLoggingHandler);
		g_javaLoggingHandler = NULL;
	}
	if(env->GetJavaVM(&g_javaLoggingJVM) == 0)
	{
		g_javaLoggingHandler = loggerGlobalReference;
		g_javaLoggingMethod = loggerMethod;
		MDynamicAudioNormalizer::setLogFunction(javaLogMessage);
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

static jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getVersionInfo_Impl(JNIEnv *env, jclass, jintArray versionInfo)
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

static jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getBuildInfo_Impl(JNIEnv *env, jclass, jobject buildInfo)
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

static jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_setLoggingHandler_Impl(JNIEnv *env, jclass, jobject loggerObject)
{
	if(loggerObject == NULL)
	{
		javaSetLoggingHandler(env, NULL, NULL);
		return JNI_TRUE;
	}

	jclass loggerClass = NULL;
	jmethodID logMethod = NULL;

	JAVA_FIND_CLASS(loggerClass, "DynamicAudioNormalizer/JDynamicAudioNormalizer$Logger");
	JAVA_GET_METHOD(logMethod, loggerClass, "log", "(ILjava/lang/String;)V");

	if(!env->IsInstanceOf(loggerObject, loggerClass))
	{
		return JNI_FALSE;
	}

	jobject globalReference = env->NewGlobalRef(loggerObject);
	if(globalReference)
	{
		javaSetLoggingHandler(env, globalReference, logMethod);
		return JNI_TRUE;
	}

	return JNI_FALSE;
}

static jint JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_createInstance_Impl(JNIEnv *env, jclass, jint channels, jint sampleRate, jint frameLenMsec, jint filterSize, jdouble peakValue, jdouble maxAmplification, jdouble targetRms, jdouble compressFactor, jboolean channelsCoupled, jboolean enableDCCorrection, jboolean altBoundaryMode)
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

static jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_destroyInstance_Impl(JNIEnv *env, jclass, jint handle)
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
	JNIEXPORT jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getVersionInfo(JNIEnv *env, jclass clazz, jintArray versionInfo)
	{
		try
		{
			return Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getVersionInfo_Impl(env, clazz, versionInfo);
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

	JNIEXPORT jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getBuildInfo(JNIEnv *env, jclass clazz, jobject buildInfo)
	{
		try
		{
			return Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_getBuildInfo_Impl(env, clazz, buildInfo);
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

	JNIEXPORT jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_setLoggingHandler(JNIEnv *env, jclass clazz, jobject loggerObject)
	{
		try
		{
			return Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_setLoggingHandler_Impl(env, clazz, loggerObject);
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

	JNIEXPORT jint JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_createInstance(JNIEnv *env, jclass clazz, jint channels, jint sampleRate, jint frameLenMsec, jint filterSize, jdouble peakValue, jdouble maxAmplification, jdouble targetRms, jdouble compressFactor, jboolean channelsCoupled, jboolean enableDCCorrection, jboolean altBoundaryMode)
	{
		try
		{
			return Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_createInstance_Impl(env, clazz, channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
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

	JNIEXPORT jboolean JNICALL Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_destroyInstance(JNIEnv *env, jclass clazz, jint instance)
	{
		try
		{
			return Java_DynamicAudioNormalizer_JDynamicAudioNormalizer_00024NativeAPI_destroyInstance_Impl(env, clazz, instance);
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
