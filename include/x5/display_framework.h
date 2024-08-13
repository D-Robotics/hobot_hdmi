// Copyright (c) 2024，D-Robotics.
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

#include <stdio.h>
#include <getopt.h>

#include <iostream>
#include <memory>
#include <vector>

extern "C" {
  #include "common_utils.h"
  #include "display_utils.h"
}

class Frame {
 public:
  Frame(int height, int width, int plane_id, char *format)
    : height(height), width(width), plane_id(plane_id), format(format), dma_buf_fd(-1), mapped_memory(nullptr) {
  }

  int height;
  int width;
  int plane_id;                // 图层ID
  char *format;
  int dma_buf_fd = -1;         // dma buffer id
  void *mapped_memory = nullptr;
};

class DisplayFramework {
 public:
  DisplayFramework(std::string config_file);
  
  ~DisplayFramework();

  int FillBuffer(int plane_id, uint8_t *data);

  int Run();

 private:
  int Debug();

  int Init(std::string config_file);

  int InitFrameBuffer();

  int Release();
  
  x5_drm_context_t drm_ctx;

  std::vector<std::shared_ptr<Frame>> frames_;
};