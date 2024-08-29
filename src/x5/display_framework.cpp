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

#include <iostream>

#include "display_framework.h"

using namespace std;

void drm_load_config(const char *filename, x5_drm_context_t *drm_ctx);

DisplayFramework::DisplayFramework(std::string config_file) {
  Init(config_file);
  InitFrameBuffer();
}

DisplayFramework::~DisplayFramework() {
  Release();
}

int DisplayFramework::Init(std::string config_file) {
  int ret = 0;

  printf("Starting program...\n");

  drm_load_config(config_file.c_str(), &drm_ctx);

  printf("Opening DRM...\n");
  drm_ctx.drm_fd = drmOpen("vs-drm", NULL);
  if (drm_ctx.drm_fd < 0) {
      perror("drmOpen failed");
      return -1;
  }

  printf("Setting DRM client capabilities...\n");
  drm_set_client_capabilities(drm_ctx.drm_fd);

  printf("Setting up KMS...\n");
  ret = drm_setup_kms(&drm_ctx);
  if (ret < 0)
  {
      printf("drm_setup_kms failed\n");
      close(drm_ctx.drm_fd);
      return -1;
  }

  printf("Opening memory module...\n");
  hb_mem_module_open();

  return 0;
}

int DisplayFramework::InitFrameBuffer() {

  for (int i = 0; i < drm_ctx.plane_count; i++) {
    std::shared_ptr<Frame> frame = std::make_shared<Frame>(drm_ctx.planes[i].src_h, drm_ctx.planes[i].src_w, drm_ctx.planes[i].plane_id, drm_ctx.planes[i].format);
    frames_.push_back(frame);
    if(create_and_map_buffer(&drm_ctx, i, &frame->mapped_memory, &frame->dma_buf_fd) != 0) {
      printf("Create_and_map_buffer error\n");
      return -1;
    }
  }
  return 0;
}

int DisplayFramework::Release() {
  printf("Closing memory module...\n");
  hb_mem_module_close();

  printf("Cleaning up DRM...\n");
  drm_cleanup(&drm_ctx);

  printf("Program finished successfully.\n");
  return 0;
}

int DisplayFramework::FillBuffer(int plane_id, uint8_t *data) {

  std::shared_ptr<Frame> frame;
  for (int i = 0; i < drm_ctx.plane_count; i++) {
    if (drm_ctx.planes[i].plane_id == plane_id)
    {
      frame = frames_[i];
      break;
    }
  }

  if (strcmp(frame->format, "NV12") == 0) {
    const size_t y_plane_size = frame->width * frame->height;
    const size_t uv_plane_size = frame->width * frame->height / 2;
    const size_t nv12_size = y_plane_size + uv_plane_size;
    memcpy(frame->mapped_memory, data, nv12_size);
  } else if (strcmp(frame->format, "RG24") == 0 || 
    strcmp(frame->format, "BG24") == 0) {
    const size_t size = frame->width * frame->height * 3;
    memcpy(frame->mapped_memory, data, size);
  } else if (strcmp(frame->format, "AR24") == 0 ||
    strcmp(frame->format, "RA24") == 0 ||
    strcmp(frame->format, "BA24") == 0 ||
    strcmp(frame->format, "AB24") == 0) {
    const size_t size = frame->width * frame->height * 4;
    memcpy(frame->mapped_memory, data, size);
  } else if (strcmp(frame->format, "RG16") == 0 || 
    strcmp(frame->format, "BG16") == 0) {
    const size_t size = frame->width * frame->height * 2;
    memcpy(frame->mapped_memory, data, size);
  } else {
    return -1;
  }

  return 0;
}

int DisplayFramework::Run() {

  int dma_buf_fds[MAX_PLANES] = {-1, -1, -1};
  
  int num = frames_.size() < MAX_PLANES ? frames_.size() : MAX_PLANES;
  for (int i = 0; i < num; i++) {
    dma_buf_fds[i] = frames_[i]->dma_buf_fd;
  }

  if (drm_display_frame(&drm_ctx, dma_buf_fds) < 0)
  {
    return -1;
  }
  return 0;
}