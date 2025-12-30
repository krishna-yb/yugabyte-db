// Copyright (c) YugabyteDB, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <string>
#include <memory>

#include "yb/yql/process_wrapper/process_wrapper.h"
#include "yb/util/status.h"
#include "yb/util/subprocess.h"

namespace yb {
namespace hello_wrapper {

// ============================================================================
// Configuration for Hello Python Service
// ============================================================================
struct HelloServiceConf {
  // Message to print
  std::string message = "Hello from TServer!";
  
  // Interval between messages (seconds)
  int interval = 5;
  
  // Python executable path
  std::string python_executable = "/usr/bin/python3";
  
  // Script path
  std::string script_path;
  
  std::string ToString() const;
};

// ============================================================================
// ProcessWrapper for Hello Python Service
// ============================================================================
class HelloServiceWrapper : public ProcessWrapper {
 public:
  explicit HelloServiceWrapper(const HelloServiceConf& conf);
  
  virtual ~HelloServiceWrapper() = default;
  
  // Required by ProcessWrapper
  Status PreflightCheck() override;
  Status Start() override;
  Status ReloadConfig() override;
  Status UpdateAndReloadConfig() override;
  
 private:
  std::string GetScriptPath();
  void SetEnvironmentVariables(Subprocess* proc);
  
  HelloServiceConf conf_;
};

// ============================================================================
// ProcessSupervisor for Hello Python Service
// ============================================================================
class HelloServiceSupervisor : public ProcessSupervisor {
 public:
  explicit HelloServiceSupervisor(const HelloServiceConf& conf);
  
  virtual ~HelloServiceSupervisor();
  
  std::shared_ptr<ProcessWrapper> CreateProcessWrapper() override;
  
 protected:
  std::string GetProcessName() override { return "Hello Python Service"; }
  
  Status PrepareForStart() override;
  
 private:
  HelloServiceConf conf_;
};

}  // namespace hello_wrapper
}  // namespace yb

