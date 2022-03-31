/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include <gtest/gtest.h>

#include "test_config.h"

using namespace ignition;

/////////////////////////////////////////////////
TEST(URI, TrivialError)
{
  std::string logFilename = "uri.log";
  std::string logDir = common::joinPaths(PROJECT_BINARY_PATH, "test", "uri");
  std::string logFile = common::joinPaths(logDir, logFilename);

  common::Console::SetVerbosity(4);
  common::URI uri;

  // Missing host will trigger:
  //   `ignerr << "A host is manadatory when using a scheme other than file\n";`
  // We are not logging to file yet, so ouput is not expected.
  EXPECT_FALSE(uri.Valid("https:///"));

  // Initialize the log file.
  ignLogInit(logDir, logFilename);

  // Run the same check, which should output the message to the log file.
  EXPECT_FALSE(uri.Valid("https:///"));
  std::ifstream t(logFile);
  std::string buffer((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
  EXPECT_TRUE(buffer.find("A host is mandatory when") != std::string::npos)
    << "Log file content[" << buffer << "]\n";
}
