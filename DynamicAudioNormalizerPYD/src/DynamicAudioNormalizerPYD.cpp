//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Python Wrapper
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// http://opensource.org/licenses/MIT
//////////////////////////////////////////////////////////////////////////////////

//Dynamic Audio Normalizer API
#include <DynamicAudioNormalizer.h>

//Python API
#include <Python.h>

//Commons
#include <Common.h>
#include <Threads.h>

//Version check
#if PY_MAJOR_VERSION != 3
#error Python 3 is reuired for this file to be compiled!
#endif

//CRT
#include <unordered_set>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Helper Macros
///////////////////////////////////////////////////////////////////////////////

#define PY_METHOD_DECL(X) PyMethodWrap_##X
#define PY_METHOD_IMPL(X) static PyObject *PyMethodImpl_##X(PyObject *const self, PyObject *const args)

#define PY_BOOLIFY(X) ((X) ? true : false)
#define PY_ULONG(X) static_cast<unsigned long>((X))

#define PY_INIT_LNG(X,Y) (((X) && PyLong_Check((X)))  ? ((uint32_t)PyLong_AsUnsignedLong((X)))  : (Y))
#define PY_INIT_FLT(X,Y) (((X) && PyFloat_Check((X))) ? PyFloat_AsDouble((X))                   : (Y))
#define PY_INIT_BLN(X,Y) (((X) && PyBool_Check((X)))  ? PY_BOOLIFY(PyObject_IsTrue((X)))        : (Y))

#define PY_EXCEPTION(X) do \
{ \
	if (!PyErr_Occurred()) \
	{ \
		PyErr_SetString(PyExc_RuntimeError, (X)); \
	} \
} \
while(0)

static void PY_FREE(PyObject **const obj)
{
	Py_XDECREF(*obj);
	*obj = NULL;
}

static void PY_FREE(const size_t n, ...)
{
	va_list arg;
	va_start(arg, n);
	for (size_t i = 0; i < n ; i++)
	{
		PyObject **const iter = va_arg(arg, PyObject**);
		PY_FREE(iter);
	}
	va_end(arg);
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

static uint64_t g_initialized = 0L;
static pthread_mutex_t m_initalization_mutex = PTHREAD_MUTEX_INITIALIZER;
static PyObject *g_arrayClass = NULL;

static bool global_init_function(void)
{
	bool success = false;
	if (PyObject *const arrayModule = PyImport_ImportModule("array"))
	{
		if (PyObject *arrayClass = PyObject_GetAttrString(arrayModule, "array"))
		{
			if (success = PyType_Check(arrayClass))
			{
				std::swap(g_arrayClass, arrayClass);
			}
			Py_XDECREF(arrayClass);
		}
		Py_XDECREF(arrayModule);
	}
	return success;
}

static bool global_exit_function(void)
{
	PY_FREE(&g_arrayClass);
	return true;
}

static bool global_init(void)
{
	bool success = false;
	MY_CRITSEC_ENTER(m_initalization_mutex);
	if (success = (g_initialized ? true : global_init_function()))
	{
		g_initialized++;
	}
	MY_CRITSEC_LEAVE(m_initalization_mutex);
	return success;
}

static bool global_exit(void)
{
	bool success = false;
	MY_CRITSEC_ENTER(m_initalization_mutex);
	if (g_initialized)
	{
		success = (--g_initialized) ? true : global_exit_function();
	}
	MY_CRITSEC_LEAVE(m_initalization_mutex);
	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Instance Management
///////////////////////////////////////////////////////////////////////////////

static std::unordered_set<MDynamicAudioNormalizer*> g_instances;
static pthread_mutex_t g_instance_mutex = PTHREAD_MUTEX_INITIALIZER;

static MDynamicAudioNormalizer *instance_add(MDynamicAudioNormalizer *const instance)
{
	MY_CRITSEC_ENTER(g_instance_mutex);
	g_instances.insert(instance);
	MY_CRITSEC_LEAVE(g_instance_mutex);
	return instance;
}

static MDynamicAudioNormalizer *instance_remove(MDynamicAudioNormalizer *const instance)
{
	MY_CRITSEC_ENTER(g_instance_mutex);
	const std::unordered_set<MDynamicAudioNormalizer*>::iterator iter = g_instances.find(instance);
	if (iter != g_instances.end())
	{
		g_instances.erase(iter);
	}
	MY_CRITSEC_LEAVE(g_instance_mutex);
	return instance;
}

static bool instance_check(MDynamicAudioNormalizer *const instance)
{
	bool result;
	MY_CRITSEC_ENTER(g_instance_mutex);
	result = (g_instances.find(instance) != g_instances.end());
	MY_CRITSEC_LEAVE(g_instance_mutex);
	return result;
}

static MDynamicAudioNormalizer *PY2INSTANCE(PyObject *const obj)
{
	if (obj && PyLong_Check(obj))
	{
		if (MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(PyLong_AsVoidPtr(obj)))
		{
			if (instance_check(instance))
			{
				return instance;
			}
		}
	}
	PY_EXCEPTION("Invalid MDynamicAudioNormalizer handle value!");
	return (NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Buffer Management
///////////////////////////////////////////////////////////////////////////////

static const uint32_t BUFFERS_MAX_CHANNEL = 8;

typedef struct
{
	uint32_t size;
	double *buffers[BUFFERS_MAX_CHANNEL];
	Py_buffer views[BUFFERS_MAX_CHANNEL];
}
buffers_t;

static bool unwrap_buffers(MDynamicAudioNormalizer *const instance, buffers_t &buffers, PyObject *const data, bool writable)
{
	memset(&buffers, 0, sizeof(buffers_t));

	uint32_t channels, sampleRate, frameLen, filterSize;
	if (!instance->getConfiguration(channels, sampleRate, frameLen, filterSize))
	{
		PY_EXCEPTION("Failed to get MDynamicAudioNormalizer configuration!");
		return false;
	}

	if (!(PyTuple_Check(data) && (channels <= BUFFERS_MAX_CHANNEL) && (static_cast<uint32_t>(PyTuple_Size(data)) < channels)))
	{
		PY_EXCEPTION("Input object does not appear to be a tuple or proper size!");
		return false;
	}

	PyObject *pyArrays[BUFFERS_MAX_CHANNEL];
	for (uint32_t c = 0; c < channels; ++c)
	{
		if (!(pyArrays[c] = PyTuple_GetItem(data, static_cast<Py_ssize_t>(c))))
		{
			PY_EXCEPTION("Failed to get array object from input tuple!");
			return false;
		}
		if (!PyObject_IsSubclass(pyArrays[c], g_arrayClass))
		{
			PY_EXCEPTION("Input object does not appear to be an array!");
			return false;
		}
	}

	for (uint32_t c = 0; c < channels; ++c)
	{
		if (!PyObject_GetBuffer(pyArrays[c], &buffers.views[c], (writable ? PyBUF_WRITABLE : 0)))
		{
			for (uint32_t k = 0; k < c; ++k)
			{
				PyBuffer_Release(&buffers.views[k]);
				memset(&buffers.views[k], 0, sizeof(Py_buffer));
			}
			PY_EXCEPTION("Failed to get buffer from array object!");
			return false;
		}
	}

	buffers.size = UINT32_MAX;
	for (uint32_t c = 0; c < channels; ++c)
	{
		buffers.buffers[c] = reinterpret_cast<double*>(buffers.views[c].buf);
		buffers.size = std::min(buffers.size, static_cast<uint32_t>(buffers.views[c].len / buffers.views[c].itemsize));
	}

	return true;
}

static void release_buffers(buffers_t &buffers)
{
	buffers.size = 0;
	memset(&buffers.buffers, 0, sizeof(double*) * BUFFERS_MAX_CHANNEL);
	for (uint32_t c = 0; c < BUFFERS_MAX_CHANNEL; ++c)
	{
		if ((buffers.views[c].buf) && (buffers.views[c].obj))
		{
			PyBuffer_Release(&buffers.views[c]);
		}
		memset(&buffers.views[c], 0, sizeof(Py_buffer));
	}
}

///////////////////////////////////////////////////////////////////////////////
// Method Implementation
///////////////////////////////////////////////////////////////////////////////

PY_METHOD_IMPL(Create)
{
	//Perform gloabl initialization first
	if (!global_init())
	{
		PY_EXCEPTION("Global library initialization has failed!");
		return (NULL);
	}

	//Unpack the function arguments (references are "borrowed"!)
	PyObject *pyChannels = NULL, *pySampleRate = NULL, *pyFrameLenMsec = NULL, *pyFilterSize = NULL, *pyPeakValue = NULL, *pyMaxAmplification = NULL, *pyTargetRms = NULL, *pyCompressFactor = NULL, *pyChannelsCoupled = NULL, *pyEnableDCCorrection = NULL, *pyAltBoundaryMode = NULL;
	if (!PyArg_UnpackTuple(args, "create", 2, 11, &pyChannels, &pySampleRate, &pyFrameLenMsec, &pyFilterSize, &pyPeakValue, &pyMaxAmplification, &pyTargetRms, &pyCompressFactor, &pyChannelsCoupled, &pyEnableDCCorrection, &pyAltBoundaryMode))
	{
		PY_EXCEPTION("Failed to unpack MDynamicAudioNormalizer arguments!");
		global_exit();
		return (NULL);
	}

	//Parse the input parameters
	const uint32_t channels           = PY_INIT_LNG(pyChannels,             0L  );
	const uint32_t sampleRate         = PY_INIT_LNG(pySampleRate,           0L  );
	const uint32_t frameLenMsec       = PY_INIT_LNG(pyFrameLenMsec,       500L  );
	const uint32_t filterSize         = PY_INIT_LNG(pyFilterSize,          31L  );
	const double   peakValue          = PY_INIT_FLT(pyPeakValue,            0.95);
	const double   maxAmplification   = PY_INIT_FLT(pyMaxAmplification,    10.  );
	const double   targetRms          = PY_INIT_FLT(pyTargetRms,            0.  );
	const double   compressFactor     = PY_INIT_FLT(pyCompressFactor,       0.  );
	const bool     channelsCoupled    = PY_INIT_BLN(pyChannelsCoupled ,     true);
	const bool     enableDCCorrection = PY_INIT_BLN(pyEnableDCCorrection,  false);
	const bool     altBoundaryMode    = PY_INIT_BLN(pyAltBoundaryMode,     false);

	//Validate parameters
	if (PyErr_Occurred() || (!channels) || (!sampleRate))
	{
		PY_EXCEPTION("Invalid or incomplete MDynamicAudioNormalizer arguments!");
		global_exit();
		return (NULL);
	}

	//Create and initialize the instance
	if (MDynamicAudioNormalizer *const instance = new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode))
	{
		if (instance->initialize())
		{
			return PyLong_FromVoidPtr(instance_add(instance));
		}
		PY_EXCEPTION("Failed to initialize MDynamicAudioNormalizer instance!");
		delete instance;
	}

	//Failure
	PY_EXCEPTION("Failed to create MDynamicAudioNormalizer instance!");
	global_exit();
	return (NULL);
}

PY_METHOD_IMPL(Destroy)
{
	if (MDynamicAudioNormalizer *const instance = PY2INSTANCE(args))
	{
		delete instance_remove(instance);
		global_exit();
		Py_RETURN_TRUE;
	}
	PY_EXCEPTION("Failed to destory MDynamicAudioNormalizer instance!");
	return (NULL);
}

PY_METHOD_IMPL(GetConfig)
{
	if (MDynamicAudioNormalizer *const instance = PY2INSTANCE(args))
	{
		uint32_t channels, sampleRate, frameLen, filterSize;
		if (instance->getConfiguration(channels, sampleRate, frameLen, filterSize))
		{
			if (PyObject *const result = Py_BuildValue("kkkk", PY_ULONG(channels), PY_ULONG(sampleRate), PY_ULONG(frameLen), PY_ULONG(filterSize)))
			{
				return result;
			}
		}
	}
	PY_EXCEPTION("Failed to get MDynamicAudioNormalizer configuration!");
	return (NULL);
}

PY_METHOD_IMPL(ProcessInplace)
{
	long long inputSize = NULL;
	PyObject *pyInstance = NULL, *pySamplesInOut = NULL;

	if (!PyArg_ParseTuple(args, "OOK", &pyInstance, &pySamplesInOut, &inputSize))
	{
		return NULL;
	}

	if (MDynamicAudioNormalizer *const instance = PY2INSTANCE(args))
	{
		buffers_t samplesInOut;
		if (unwrap_buffers(instance, samplesInOut, pySamplesInOut, true))
		{
			int64_t outputSize;
			const bool success = instance->processInplace(samplesInOut.buffers, static_cast<int64_t>(inputSize), outputSize);
			release_buffers(samplesInOut);
			if(success)
			{
				return PyLong_FromLongLong(static_cast<long long>(outputSize));
			}
		}
	}

	PY_EXCEPTION("Failed to process audio samples in-place!");
	return (NULL);
}

///////////////////////////////////////////////////////////////////////////////
// Method Wrappers
///////////////////////////////////////////////////////////////////////////////

#define PY_METHOD_WRAP(X) \
static PyObject *PyMethodWrap_##X(PyObject *const self, PyObject *const args) \
{ \
	try \
	{ \
		return PyMethodImpl_##X(self, args); \
	} \
	catch (...) \
	{ \
		PyErr_SetString(PyExc_SystemError, "Internal C++ exception error!"); \
		return NULL; \
	} \
}

PY_METHOD_WRAP(Create)
PY_METHOD_WRAP(Destroy)
PY_METHOD_WRAP(GetConfig)
PY_METHOD_WRAP(ProcessInplace)

///////////////////////////////////////////////////////////////////////////////
// Module Definition
///////////////////////////////////////////////////////////////////////////////

static PyMethodDef PyMDynamicAudioNormalizer_Methods[] =
{
	{ "create",         PY_METHOD_DECL(Create),         METH_VARARGS,  "Create a new MDynamicAudioNormalizer instance and initialize it."       },
	{ "destroy",        PY_METHOD_DECL(Destroy),        METH_O,        "Destroy an existing MDynamicAudioNormalizer instance."                  },
	{ "getConfig",      PY_METHOD_DECL(GetConfig),      METH_O,        "Get the configuration of an existing MDynamicAudioNormalizer instance." },
	{ "processInplace", PY_METHOD_DECL(ProcessInplace), METH_VARARGS,  "Process next chunk audio samples, in-place."                            },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef PyDynamicAudioNormalizer_ModuleDef =
{
	PyModuleDef_HEAD_INIT, "DynamicAudioNormalizerAPI", "", -1, PyMDynamicAudioNormalizer_Methods
};

PyMODINIT_FUNC
PyInit_DynamicAudioNormalizerAPI(void)
{
	return PyModule_Create(&PyDynamicAudioNormalizer_ModuleDef);
}

