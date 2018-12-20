//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Microsoft.NET Wrapper
// Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

#include <DynamicAudioNormalizer.h>

#define CORE_VERSION_MAKE_STR1(X) #X
#define CORE_VERSION_MAKE_STR2(X) CORE_VERSION_MAKE_STR1(X)
#define CORE_VERSION CORE_VERSION_MAKE_STR2(MDYNAMICAUDIONORMALIZER_CORE)

//
// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
//
[assembly:AssemblyTitleAttribute(L"DynamicAudioNormalizerNET")];
[assembly:AssemblyDescriptionAttribute(L"DynamicAudioNormalizer .NET-Wrapper")];
[assembly:AssemblyConfigurationAttribute(L"")];
[assembly:AssemblyCompanyAttribute(L"Free Software Foundation")];
[assembly:AssemblyProductAttribute(L"DynamicAudioNormalizerNET")];
[assembly:AssemblyCopyrightAttribute(L"Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>")];
[assembly:AssemblyTrademarkAttribute(L"")];
[assembly:AssemblyCultureAttribute(L"")];

//
// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version
//      Build Number
//      Revision
//
// You can specify all the value or you can default the Revision and Build Numbers
// by using the '*' as shown below:
//
[assembly:AssemblyVersionAttribute(CORE_VERSION ".0.*")];

[assembly:ComVisible(false)];

[assembly:CLSCompliantAttribute(true)];