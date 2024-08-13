#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <stdint.h>
#include <drm/drm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <cjson/cJSON.h>
#include <drm_fourcc.h>
#include "uthash.h"

#include <unistd.h>
#include <sys/mman.h>

#define MAX_PLANES 3
#define ION_MAX_BUFFERS 3

typedef struct
{
    int dma_buf_fd;
    uint32_t fb_id;
    UT_hash_handle hh; // uthash 处理器
} dma_buf_map_t;

typedef struct
{
    uint32_t plane_id;
    uint32_t src_w;
    uint32_t src_h;
    uint32_t crtc_x;
    uint32_t crtc_y;
    uint32_t crtc_w;
    uint32_t crtc_h;
    uint32_t z_pos;
    uint32_t alpha;
    char format[8];          // 图层格式
    uint32_t rotation;       // 旋转属性
    uint32_t color_encoding; // 颜色编码
    uint32_t color_range;    // 颜色范围
    uint32_t pixel_blend_mode; // alpha模式
} plane_config_t;

typedef struct
{
    int drm_fd;
    uint32_t crtc_id;
    uint32_t connector_id;
    plane_config_t planes[MAX_PLANES];
    uint32_t width;
    uint32_t height;
    int plane_count;
    dma_buf_map_t *buffer_map; // 使用哈希表
    int buffer_count;
    int max_buffers; // 动态调整 buffer_map 的大小
} x5_drm_context_t;

void drm_load_config(const char *filename, x5_drm_context_t *drm_ctx);
void drm_cleanup(x5_drm_context_t *ctx);
void drm_set_client_capabilities(int drm_fd);
int drm_setup_kms(x5_drm_context_t *ctx);
int drm_display_frame(x5_drm_context_t *ctx, int dma_buf_fds[MAX_PLANES]);
int drm_display_frame_non_zero_copy(x5_drm_context_t *ctx, uint8_t *src_data, int plane_index);
int get_nv12_frame(x5_drm_context_t *ctx, const uint8_t *nv12_data, const int width, const int height);

int create_and_map_nv12_buffer(x5_drm_context_t *ctx, int width, int height, void **mapped_memory, int *dma_buf_fd);
void update_nv12_buffer(void *mapped_memory, const uint8_t *nv12_data, const int width, const int height);
int create_and_map_buffer(x5_drm_context_t *ctx, int plane_index, void **mapped_memory, int *dma_buf_fd);

#endif // DISPLAY_UTILS_H