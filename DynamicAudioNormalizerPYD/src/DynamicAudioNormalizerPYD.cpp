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
static pthread_mutex_t m_initalized_mutex = PTHREAD_MUTEX_INITIALIZER;
static PyObject *g_arrayModule = NULL, *g_arrayClass = NULL;

static bool global_init_function(void)
{
	if (!(g_arrayModule = PyImport_ImportModule("array")))
	{
		return false;
	}
	if (!(g_arrayClass = PyObject_GetAttrString(g_arrayModule, "array")))
	{
		PY_FREE(&g_arrayModule);
		return false;
	}
	if (!PyType_Check(g_arrayClass))
	{
		PY_FREE(2, &g_arrayClass, &g_arrayModule);
		return false;
	}
	return true;
}

static bool global_exit_function(void)
{
	PY_FREE(2, &g_arrayClass, &g_arrayModule);
	return true;
}

static bool global_init(void)
{
	bool success = false;
	MY_CRITSEC_ENTER(m_initalized_mutex);
	if (success = (g_initialized ? true : global_init_function()))
	{
		g_initialized++;
	}
	MY_CRITSEC_LEAVE(m_initalized_mutex);
	return success;
}

static bool global_exit(void)
{
	bool success = false;
	MY_CRITSEC_ENTER(m_initalized_mutex);
	if (g_initialized)
	{
		success = (--g_initialized) ? true : global_exit_function();
	}
	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Instance Management
///////////////////////////////////////////////////////////////////////////////

static std::unordered_set<MDynamicAudioNormalizer*> g_instances;
static pthread_mutex_t g_instances_mutex = PTHREAD_MUTEX_INITIALIZER;

static MDynamicAudioNormalizer *instance_add(MDynamicAudioNormalizer *const instance)
{
	MY_CRITSEC_ENTER(g_instances_mutex);
	g_instances.insert(instance);
	MY_CRITSEC_LEAVE(g_instances_mutex);
	return instance;
}

static bool instance_check(MDynamicAudioNormalizer *const instance)
{
	bool result;
	MY_CRITSEC_ENTER(g_instances_mutex);
	result = (g_instances.find(instance) != g_instances.end());
	MY_CRITSEC_LEAVE(g_instances_mutex);
	return result;
}

static MDynamicAudioNormalizer *instance_remove(MDynamicAudioNormalizer *const instance)
{
	MY_CRITSEC_ENTER(g_instances_mutex);
	g_instances.erase(instance);
	MY_CRITSEC_LEAVE(g_instances_mutex);
	return instance;
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

///////////////////////////////////////////////////////////////////////////////
// Module Definition
///////////////////////////////////////////////////////////////////////////////

static PyMethodDef PyMDynamicAudioNormalizer_Methods[] =
{
	{ "create",    PY_METHOD_DECL(Create),    METH_VARARGS,  "Create a new MDynamicAudioNormalizer instance and initialize it."       },
	{ "destroy",   PY_METHOD_DECL(Destroy),   METH_O,        "Destroy an existing MDynamicAudioNormalizer instance."                  },
	{ "getConfig", PY_METHOD_DECL(GetConfig), METH_O,        "Get the configuration of an existing MDynamicAudioNormalizer instance." },
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

