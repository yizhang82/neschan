// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

#include <vector>
#include <memory>
#include <fstream>
#include <string>
#include <iostream>

//
// NESchan headers
//
#include "nes_cycle.h"
#include "nes_component.h"
#include "nes_system.h"
#include "nes_memory.h"
#include "nes_mapper.h"
#include "nes_ppu.h"
#include "nes_cpu.h"