/*
 * Copyright (C) 2017 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#ifndef IGNITION_COMMON_SUPPRESSWARNING_HH_
#define IGNITION_COMMON_SUPPRESSWARNING_HH_

#include "detail/SuppressWarning.hh"

// This header contains cross-platform macros for suppressing warnings. Please
// only use these macros responsibly when you are certain that the compiler is
// producing a warning that is not applicable to the specific instance. Do not
// use these macros to ignore legitimate warnings, even if you may find them
// irritating.

/*
 * Usage example:
 *
 * SomeClass* ptr = CreatePtr();
 * IGN_COMMON_WARN_IGNORE__DELETE_NON_VIRTUAL_DESTRUCTOR
 * delete ptr;
 * IGN_COMMON_WARN_RESTORE(DELETE_NON_VIRTUAL_DESTRUCTOR)
 *
 */


// ---- List of available suppressions ----

/// \brief Compilers might warn about deleting a pointer to a class that has
/// virtual functions without a virtual destructor or a `final` declaration,
/// because the pointer might secretly be pointing to a more derived class type.
/// We want to suppress this warning when we know for certain (via the design
/// of our implementation) that the pointer is definitely not pointing to a more
/// derived type.
#define IGN_COMMON_WARN_IGNORE__DELETE_NON_VIRTUAL_DESTRUCTOR \
  DETAIL_IGN_COMMON_WARN_IGNORE__DELETE_NON_VIRTUAL_DESTRUCTOR


/// \brief Compilers
#define IGN_COMMON_WARN_IGNORE__DLL_INTERFACE_MISSING \
  DETAIL_IGN_COMMON_WARN_IGNORE__DLL_INTERFACE_MISSING


// TODO: Add more warning tokens as they become relevant. Do not add tokens to
// suppress unless it is necessary.


/// \brief Use this macro to finish suppressing a specific type of warning
/// within a specific block of code. This macro must be preceded by a call to
/// IGN_COMMON_WARN_IGNORE__XXXXX (see options below for XXXXX).
#define IGN_COMMON_WARN_RESTORE(warning) \
  DETAIL_IGN_COMMON_WARN_RESTORE(warning)

#endif
