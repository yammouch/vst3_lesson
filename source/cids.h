//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace MyCompanyName {
//------------------------------------------------------------------------
static const Steinberg::FUID kvst002ProcessorUID (0xFD2128C6, 0x71F15C89, 0x8E1DD71B, 0xF4C2179D);
static const Steinberg::FUID kvst002ControllerUID (0x9C4F029E, 0x04B55919, 0xAD501CF1, 0x068B5FB2);

#define vst002VST3Category "Fx"

//------------------------------------------------------------------------
} // namespace MyCompanyName
