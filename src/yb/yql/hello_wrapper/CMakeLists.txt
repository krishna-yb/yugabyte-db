# Copyright (c) YugabyteDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Hello Wrapper library
set(HELLO_WRAPPER_SRCS
  hello_wrapper.cc
)

# Create the library
ADD_YB_LIBRARY(yb_hello_wrapper
  SRCS ${HELLO_WRAPPER_SRCS}
  DEPS yb_process_wrapper yb_util
)

# Copy Python script to build directory
add_custom_target(copy_hello_python_script ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${YB_BUILD_ROOT}/share/hello_python
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/share/hello_python/hello_service.py
            ${YB_BUILD_ROOT}/share/hello_python/hello_service.py
    COMMENT "Copying hello_service.py to build directory"
    COMMAND_ERROR_IS_FATAL ANY
)

