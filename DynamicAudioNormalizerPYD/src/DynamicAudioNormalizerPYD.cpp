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
#include <stdexcept>

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

class PyBufferWrapper
{
public:
	PyBufferWrapper(PyObject *const data, const uint32_t count, const bool writable)
	:
		m_count(count),
		m_length(UINT32_MAX)
	{
		memset(m_buffers, 0, sizeof(double*) * MAX_BUFFERS);
		memset(m_views, 0, sizeof(Py_buffer) * MAX_BUFFERS);

		if (!((data) && PyTuple_Check(data)))
		{
			PY_EXCEPTION("Input Python object does not appear to be a tuple!");
			throw std::invalid_argument("Invalid buffer object!");
		}
		if (!((count > 0) && (count <= MAX_BUFFERS) && (static_cast<uint32_t>(PyTuple_Size(data)) >= count)))
		{
			PY_EXCEPTION("Input tuple does not have the required minimum size!");
			throw std::invalid_argument("Invalid buffer count!");
		}

		PyObject *pyArrays[MAX_BUFFERS];
		for (uint32_t c = 0; c < count; ++c)
		{
			if (!(pyArrays[c] = PyTuple_GetItem(data, static_cast<Py_ssize_t>(c))))
			{
				PY_EXCEPTION("Failed to obtain the n-th element of the given input tuple!");
				throw std::invalid_argument("PyTuple_GetItem() error!");
			}
			if (PyObject_IsInstance(pyArrays[c], g_arrayClass) <= 0)
			{
				PY_EXCEPTION("The n-th element of the given tuple is not an array object!");
				throw std::invalid_argument("PyObject_IsInstance() error!");
			}
		}

		for (uint32_t c = 0; c < count; ++c)
		{
			if (!PyObject_CheckBuffer(pyArrays[c]))
			{
				PY_EXCEPTION("The provided array object does not support the 'buffer' protocol!");
				releaseBuffers();
				throw std::invalid_argument("PyObject_CheckBuffer() error!");
			}
			if (PyObject_GetBuffer(pyArrays[c], &m_views[c], (writable ? PyBUF_WRITABLE : 0)) < 0)
			{
				PY_EXCEPTION("Failed to get the internal buffer from array object!");
				releaseBuffers();
				throw std::invalid_argument("PyObject_GetBuffer() error!");
			}
			if (m_views[c].itemsize != sizeof(double))
			{
				PY_EXCEPTION("The provided array has an invalid item size! Not 'double' array?");
				releaseBuffers();
				throw std::invalid_argument("Py_buffer.itemsize is invalid!");
			}
			if ((m_length = std::min(m_length, static_cast<uint32_t>(m_views[c].len / m_views[c].itemsize))) < 1)
			{
				PY_EXCEPTION("The size of the provided buffers is way too small!");
				releaseBuffers();
				throw std::invalid_argument("Py_buffer.len is invalid!");
			}
			if (!(m_buffers[c] = reinterpret_cast<double*>(m_views[c].buf)))
			{
				PY_EXCEPTION("Pointer to the arrays's buffer appears to be NULL!");
				releaseBuffers();
				throw std::invalid_argument("Py_buffer.buf is invalid!");
			}
		}
	}

	~PyBufferWrapper(void)
	{
		releaseBuffers();
	}

	uint32_t getLength(void) const
	{
		return m_length;
	}

	double *const *getBuffer(void) const
	{
		return m_buffers;
	}

private:
	static const uint32_t MAX_BUFFERS = 8;
	const uint32_t m_count;
	uint32_t m_length;
	double *m_buffers[MAX_BUFFERS];
	Py_buffer m_views[MAX_BUFFERS];

	void releaseBuffers(void)
	{
		for (uint32_t k = 0; k < m_count; ++k)
		{
			if ((m_views[k].buf) && (m_views[k].obj))
			{
				PyBuffer_Release(&m_views[k]);
			}
		}

		memset(m_buffers, 0, sizeof(double*) * MAX_BUFFERS);
		memset(m_views, 0, sizeof(Py_buffer) * MAX_BUFFERS);
	}

	PyBufferWrapper& operator=(const PyBufferWrapper&) { throw 666; }
	PyBufferWrapper(const PyBufferWrapper&):m_count(0) { throw 666; }
};

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

	if (MDynamicAudioNormalizer *const instance = PY2INSTANCE(pyInstance))
	{
		uint32_t channels, ignored[3];
		if (!instance->getConfiguration(channels, ignored[0], ignored[1], ignored[2]))
		{
			PY_EXCEPTION("Failed to get number of channels from instance!");
			return (NULL);
		}
		try
		{
			PyBufferWrapper samplesInOut(pySamplesInOut, channels, true);
			if (static_cast<uint64_t>(inputSize) > static_cast<uint64_t>(samplesInOut.getLength()))
			{
				PY_EXCEPTION("Specified input size exceeds the size of the given array!");
				return (NULL);
			}
			int64_t outputSize;
			if(instance->processInplace(samplesInOut.getBuffer(), static_cast<uint64_t>(inputSize), outputSize))
			{
				return PyLong_FromLongLong(static_cast<long long>(outputSize));
			}
		}
		catch (const std::invalid_argument&)
		{
			PY_EXCEPTION("Failed to get buffers from inpout arrays!");
			return (NULL);
		}
	}

	PY_EXCEPTION("Failed to process audio samples in-place!");
	return (NULL);
}

PY_METHOD_IMPL(FlushBuffer)
{
	PyObject *pyInstance = NULL, *pySamplesOut = NULL;

	if (!PyArg_ParseTuple(args, "OO", &pyInstance, &pySamplesOut))
	{
		return NULL;
	}

	if (MDynamicAudioNormalizer *const instance = PY2INSTANCE(pyInstance))
	{
		uint32_t channels, ignored[3];
		if (!instance->getConfiguration(channels, ignored[0], ignored[1], ignored[2]))
		{
			PY_EXCEPTION("Failed to get number of channels from instance!");
			return (NULL);
		}
		try
		{
			PyBufferWrapper samplesOut(pySamplesOut, channels, true);
			int64_t outputSize;
			if (instance->flushBuffer(samplesOut.getBuffer(), samplesOut.getLength(), outputSize))
			{
				return PyLong_FromLongLong(static_cast<long long>(outputSize));
			}
		}
		catch (const std::invalid_argument&)
		{
			PY_EXCEPTION("Failed to get buffers from inpout arrays!");
			return (NULL);
		}
	}

	PY_EXCEPTION("Failed to process audio samples in-place!");
	return (NULL);
}

PY_METHOD_IMPL(GetVersionInfo)
{
	const char *date, *time, *compiler, *arch;
	uint32_t versionMajor, versionMinor, versionPatch;
	bool debug;

	MDynamicAudioNormalizer::getVersionInfo(versionMajor, versionMinor, versionPatch);
	MDynamicAudioNormalizer::getBuildInfo(&date, &time, &compiler, &arch, debug);

	PyObject *const pyVersionInfo = Py_BuildValue("III", static_cast<unsigned int>(versionMajor), static_cast<unsigned int>(versionMinor), static_cast<unsigned int>(versionPatch));
	PyObject *const pyBuildInfo   = Py_BuildValue("sssss", date, time, compiler, arch, debug ? "DEBUG" : "RELEASE");

	PyObject *result = NULL;
	if ((pyVersionInfo) && (pyBuildInfo))
	{
		result = PyTuple_Pack(2, pyVersionInfo, pyBuildInfo);
	}

	Py_XDECREF(pyVersionInfo);
	Py_XDECREF(pyBuildInfo);

	return result;
}

PY_METHOD_IMPL(GetCoreVersion)
{
	return PyLong_FromUnsignedLong(static_cast<unsigned long>(MDYNAMICAUDIONORMALIZER_CORE));
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
PY_METHOD_WRAP(FlushBuffer)
PY_METHOD_WRAP(GetVersionInfo)
PY_METHOD_WRAP(GetCoreVersion)

///////////////////////////////////////////////////////////////////////////////
// Module Definition
///////////////////////////////////////////////////////////////////////////////

static PyMethodDef PyMDynamicAudioNormalizer_Methods[] =
{
	{ "create",         PY_METHOD_DECL(Create),         METH_VARARGS,  "Create a new MDynamicAudioNormalizer instance and initialize it."       },
	{ "destroy",        PY_METHOD_DECL(Destroy),        METH_O,        "Destroy an existing MDynamicAudioNormalizer instance."                  },
	{ "getConfig",      PY_METHOD_DECL(GetConfig),      METH_O,        "Get the configuration of an existing MDynamicAudioNormalizer instance." },
	{ "processInplace", PY_METHOD_DECL(ProcessInplace), METH_VARARGS,  "Process next chunk audio samples, in-place."                            },
	{ "flushBuffer",    PY_METHOD_DECL(FlushBuffer),    METH_VARARGS,  "Flushes pending samples out of the MDynamicAudioNormalizer instance."   },
	{ "getVersionInfo", PY_METHOD_DECL(GetVersionInfo), METH_NOARGS,   "Returns version and build info. This is a \"static\" method."           },
	{ "getCoreVersion", PY_METHOD_DECL(GetCoreVersion), METH_NOARGS,   "Returns the API version. This is a \"static\" method."                  },
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

