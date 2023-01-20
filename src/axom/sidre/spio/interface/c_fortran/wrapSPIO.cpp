// wrapSPIO.cpp
// This file is generated by Shroud 0.12.2. Do not edit.
//
// Copyright (c) 2017-2023, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
#include <cstdlib>
#include "typesSPIO.h"

#include "axom/sidre/spio/IOManager.hpp"

// splicer begin CXX_definitions
// splicer end CXX_definitions

extern "C" {

// splicer begin C_definitions
// splicer end C_definitions

// Release library allocated memory.
void SPIO_SHROUD_memory_destructor(SPIO_SHROUD_capsule_data* cap)
{
  void* ptr = cap->addr;
  switch(cap->idtor)
  {
  case 0:  // --none--
  {
    // Nothing to delete
    break;
  }
  case 1:  // axom::sidre::IOManager
  {
    axom::sidre::IOManager* cxx_ptr =
      reinterpret_cast<axom::sidre::IOManager*>(ptr);
    delete cxx_ptr;
    break;
  }
  default:
  {
    // Unexpected case in destructor
    break;
  }
  }
  cap->addr = nullptr;
  cap->idtor = 0;  // avoid deleting again
}

}  // extern "C"
