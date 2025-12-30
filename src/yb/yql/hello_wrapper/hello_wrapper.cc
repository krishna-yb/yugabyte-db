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

#include "yb/yql/hello_wrapper/hello_wrapper.h"

#include <string>
#include <vector>

#include "yb/gutil/strings/join.h"
#include "yb/util/env.h"
#include "yb/util/env_util.h"
#include "yb/util/flags.h"
#include "yb/util/path_util.h"
#include "yb/util/result.h"
#include "yb/util/subprocess.h"
#include "yb/util/status_format.h"

// GFlags for configuration
DEFINE_NON_RUNTIME_bool(enable_hello_service, false,
    "Enable the Hello Python service. This is a demo service that prints "
    "messages periodically to demonstrate ProcessWrapper integration.");

DEFINE_NON_RUNTIME_string(hello_service_message, "Hello from YugabyteDB TServer!",
    "Message to print from the Hello service");

DEFINE_NON_RUNTIME_int32(hello_service_interval, 5,
    "Interval between messages in seconds");

namespace yb {
namespace hello_wrapper {


// ============================================================================
// HelloServiceConf Implementation
// ============================================================================

std::string HelloServiceConf::ToString() const {
  return Format(
      "HelloServiceConf { message: '$0', interval: $1s, script: $2 }",
      message, interval, script_path);
}

// ============================================================================
// HelloServiceWrapper Implementation
// ============================================================================

HelloServiceWrapper::HelloServiceWrapper(const HelloServiceConf& conf)
    : conf_(conf) {
  LOG(INFO) << "Created Hello service wrapper: " << conf_.ToString();
}

Status HelloServiceWrapper::PreflightCheck() {
  LOG(INFO) << "Running preflight checks for Hello service...";
  
  // Check 1: Venv wrapper script exists
  std::string yb_root = env_util::GetRootDir("bin");
  std::string venv_wrapper = JoinPathSegments(yb_root, "build-support", "run_in_build_python_venv.sh");
  
  if (!Env::Default()->FileExists(venv_wrapper)) {
    return STATUS_FORMAT(
        NotFound,
        "Venv wrapper script not found: $0. "
        "This is required to run Python with the correct environment.",
        venv_wrapper);
  }
  RETURN_NOT_OK(CheckExecutableValid(venv_wrapper));
  LOG(INFO) << "Venv wrapper validated: " << venv_wrapper;
  
  // Check 2: Python service script exists
  std::string script_path = GetScriptPath();
  if (!Env::Default()->FileExists(script_path)) {
    return STATUS_FORMAT(
        NotFound,
        "Hello service script not found: $0. "
        "Make sure hello_service.py exists in share/hello_python/",
        script_path);
  }
  LOG(INFO) << "Script found: " << script_path;
  
  LOG(INFO) << "All preflight checks passed";
  return Status::OK();
}

Status HelloServiceWrapper::Start() {
  std::string script_path = GetScriptPath();
  
  // Use the YugabyteDB venv activation script to run Python
  // Use "build-support" as search dir to find the SOURCE root, not the build root
  std::string venv_wrapper_source_root = env_util::GetRootDir("build-support");
  std::string venv_wrapper = JoinPathSegments(venv_wrapper_source_root, "build-support", "run_in_build_python_venv.sh");
  
  // Validate that the wrapper script exists and is executable
  RETURN_NOT_OK(CheckExecutableValid(venv_wrapper));
  
  LOG(INFO) << "========================================";
  LOG(INFO) << "Starting Hello Python Service...";
  LOG(INFO) << "  Venv Wrapper: " << venv_wrapper;
  LOG(INFO) << "  Script: " << script_path;
  LOG(INFO) << "  Message: " << conf_.message;
  LOG(INFO) << "  Interval: " << conf_.interval << "s";
  LOG(INFO) << "========================================";
  
  // Build command line arguments
  // Format: run_in_build_python_venv.sh python3 <script> <args>
  std::vector<std::string> argv{
    venv_wrapper,
    "python3",
    script_path,
    "--message", conf_.message,
    "--interval", std::to_string(conf_.interval)
  };
  
  // Log the full command
  std::string cmd = JoinStrings(argv, " ");
  LOG(INFO) << "Command: " << cmd;
  
  // Create subprocess using the venv wrapper script
  proc_.emplace(venv_wrapper, argv);
  
  // Set environment variables
  SetEnvironmentVariables(&proc_.value());
  
  // Start the process!
  RETURN_NOT_OK(proc_->Start());
  
  LOG(INFO) << "Hello service started successfully";
  LOG(INFO) << "  PID: " << proc_->pid();
  LOG(INFO) << "========================================";
  
  return Status::OK();
}

Status HelloServiceWrapper::ReloadConfig() {
  if (proc_) {
    LOG(INFO) << "Reloading Hello service configuration (SIGHUP)";
    LOG(INFO) << "  New message: " << conf_.message;
    Kill(SIGHUP);
    LOG(INFO) << "SIGHUP sent to PID: " << proc_->pid();
  }
  return Status::OK();
}

Status HelloServiceWrapper::UpdateAndReloadConfig() {
  // Update config from GFlags
  conf_.message = FLAGS_hello_service_message;
  conf_.interval = FLAGS_hello_service_interval;
  
  return ReloadConfig();
}

std::string HelloServiceWrapper::GetScriptPath() {
  if (!conf_.script_path.empty()) {
    return conf_.script_path;
  }
  
  // Default: relative to YugabyteDB installation
  // Path: <yb_home>/share/hello_python/hello_service.py
  std::string yb_home = env_util::GetRootDir("yb-tserver");
  return JoinPathSegments(yb_home, "share", "hello_python", "hello_service.py");
}

// Set environment variables for the Python service
void HelloServiceWrapper::SetEnvironmentVariables(Subprocess* proc) {
  // Set service configuration via environment variables
  proc->SetEnv("HELLO_MESSAGE", conf_.message);
  proc->SetEnv("HELLO_INTERVAL", std::to_string(conf_.interval));
  
  // Note: PYTHONPATH and venv activation are handled by run_in_build_python_venv.sh
  
  LOG(INFO) << "Environment variables set:";
  LOG(INFO) << "  HELLO_MESSAGE=" << conf_.message;
  LOG(INFO) << "  HELLO_INTERVAL=" << conf_.interval;
}

// ============================================================================
// HelloServiceSupervisor Implementation
// ============================================================================

HelloServiceSupervisor::HelloServiceSupervisor(const HelloServiceConf& conf)
    : conf_(conf) {
  LOG(INFO) << "Created Hello service supervisor";
}

HelloServiceSupervisor::~HelloServiceSupervisor() {
  Stop();
}

std::shared_ptr<ProcessWrapper> HelloServiceSupervisor::CreateProcessWrapper() {
  LOG(INFO) << "Creating Hello service wrapper instance";
  return std::make_shared<HelloServiceWrapper>(conf_);
}

Status HelloServiceSupervisor::PrepareForStart() {
  LOG(INFO) << "Preparing to start Hello service...";
  LOG(INFO) << "Configuration: " << conf_.ToString();
  return Status::OK();
}

}  // namespace hello_wrapper
}  // namespace yb

