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

//Version check
#if PY_MAJOR_VERSION != 3
#error Python 3 is reuired for this file to be compiled!
#endif

//CRT
#include <unordered_set>

///////////////////////////////////////////////////////////////////////////////
// Instance Management
///////////////////////////////////////////////////////////////////////////////

static std::unordered_set<MDynamicAudioNormalizer*> m_instances;

static MDynamicAudioNormalizer *instance_add(MDynamicAudioNormalizer *const instance)
{
	m_instances.insert(instance);
	return instance;
}

static bool instance_check(MDynamicAudioNormalizer *const instance)
{
	return (m_instances.find(instance) != m_instances.end());
}

static MDynamicAudioNormalizer *instance_remove(MDynamicAudioNormalizer *const instance)
{
	m_instances.erase(instance);
	return instance;
}

///////////////////////////////////////////////////////////////////////////////
// Helper Macros
///////////////////////////////////////////////////////////////////////////////

#define PY_BOOLIFY(X) ((X) ? true : false)

#define PY_INIT_LNG(X,Y) (((X) && PyLong_Check((X)))  ? ((uint32_t)PyLong_AsUnsignedLong((X)))  : (Y))
#define PY_INIT_FLT(X,Y) (((X) && PyFloat_Check((X))) ? PyFloat_AsDouble((X))                   : (Y))
#define PY_INIT_BLN(X,Y) (((X) && PyBool_Check((X)))  ? PY_BOOLIFY(PyObject_IsTrue((X)))        : (Y))

#define PY_INSTANCE(X) reinterpret_cast<MDynamicAudioNormalizer*>(PyLong_AsVoidPtr((X)))

///////////////////////////////////////////////////////////////////////////////
// Method Implementation
///////////////////////////////////////////////////////////////////////////////

static PyObject *PyMDynamicAudioNormalizer_Create(PyObject *const self, PyObject *const args)
{
	PyObject *pyChannels = NULL, *pySampleRate = NULL, *pyFrameLenMsec = NULL, *pyFilterSize = NULL;
	PyObject *pyPeakValue = NULL, *pyMaxAmplification = NULL, *pyTargetRms = NULL, *pyCompressFactor = NULL;
	PyObject *pyChannelsCoupled = NULL, *pyEnableDCCorrection = NULL, *pyAltBoundaryMode = NULL;
	
	if (!PyArg_UnpackTuple(args, "create", 2, 11,
			&pyChannels, &pySampleRate, &pyFrameLenMsec, &pyFilterSize,
			&pyPeakValue, &pyMaxAmplification, &pyTargetRms, &pyCompressFactor,
			&pyChannelsCoupled, &pyEnableDCCorrection, &pyAltBoundaryMode))
	{
		Py_RETURN_NONE;
	}

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
	
	if ((channels > 0) && (sampleRate > 0))
	{
		MDynamicAudioNormalizer *const instance = new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
		if (instance->initialize())
		{
			return PyLong_FromVoidPtr(instance_add(instance));
		}
		delete instance;
	}

	Py_RETURN_NONE;
}

static PyObject *PyMDynamicAudioNormalizer_Destroy(PyObject *const self, PyObject *const args)
{
	if (PyLong_Check(args))
	{
		if (MDynamicAudioNormalizer *const instance = PY_INSTANCE(args))
		{
			if (instance_check(instance))
			{
				delete instance_remove(instance);
				Py_RETURN_TRUE;
			}
		}
	}

	Py_RETURN_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Module Definition
///////////////////////////////////////////////////////////////////////////////

static PyMethodDef PyMDynamicAudioNormalizer_Methods[] =
{
	{ "create",  PyMDynamicAudioNormalizer_Create,  METH_VARARGS,  "Create a new MDynamicAudioNormalizer instance and initialize it." },
	{ "destroy", PyMDynamicAudioNormalizer_Destroy, METH_O,        "Destroy an existing MDynamicAudioNormalizer instance."            },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef PyDynamicAudioNormalizer_ModuleDef =
{
	PyModuleDef_HEAD_INIT, "MDynamicAudioNormalizerAPI", "", -1, PyMDynamicAudioNormalizer_Methods
};

PyMODINIT_FUNC
PyInit_MDynamicAudioNormalizerAPI(void)
{
	return PyModule_Create(&PyDynamicAudioNormalizer_ModuleDef);
}

