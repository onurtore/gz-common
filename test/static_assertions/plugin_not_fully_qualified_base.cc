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

#include <gz/utils/SuppressWarning.hh>

GZ_UTILS_WARN_IGNORE__DEPRECATED_DECLARATION
#define SUPPRESS_IGNITION_HEADER_DEPRECATION

#include "ignition/common/PluginMacros.hh"

namespace ns
{

class Base
{
  int test3;
};

class A : public Base
{
  float test1;
  double test2;
};

}

using ns::Base;

IGN_COMMON_REGISTER_SINGLE_PLUGIN(ns::A, Base)

GZ_UTILS_WARN_RESUME__DEPRECATED_DECLARATION

#undef SUPPRESS_IGNITION_HEADER_DEPRECATION
