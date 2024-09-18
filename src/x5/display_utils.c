// #include <unistd.h>
// #include <sys/mman.h>

#include "display_utils.h"
void add_property(int drm_fd, drmModeAtomicReq *req, uint32_t obj_id, uint32_t obj_type, const char *name, uint64_t value);

static uint32_t get_bpp_from_format(uint32_t format)
{
    switch (format)
    {
    case DRM_FORMAT_ARGB4444:
    case DRM_FORMAT_XRGB4444:
    case DRM_FORMAT_ABGR4444:
    case DRM_FORMAT_XBGR4444:
        return 16; // 16 bits per pixel (4 bits per channel)
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        return 16; // 16 bits per pixel (5, 6, 5 bits per channel)
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_XBGR1555:
        return 16; // 16 bits per pixel (1 bit alpha, 5 bits per RGB)
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_XBGR8888:
        return 32; // 32 bits per pixel (8 bits per channel)
    case DRM_FORMAT_RGB888:
    case DRM_FORMAT_BGR888:
        return 24; // 24 bits per pixel (8 bits per channel, no alpha)
    case DRM_FORMAT_YUYV:
    case DRM_FORMAT_YVYU:
        return 16; // 16 bits per pixel for YUV 4:2:2
    case DRM_FORMAT_NV12:
    case DRM_FORMAT_NV21:
        return 12; // 12 bits per pixel for YUV 4:2:0
    default:
        return 0; // Unsupported format
    }
}
static uint32_t get_format_from_string(const char *format_str)
{
    if (strcmp(format_str, "AR12") == 0)
    {
        return DRM_FORMAT_ARGB4444;
    }
    else if (strcmp(format_str, "AR15") == 0)
    {
        return DRM_FORMAT_ARGB1555;
    }
    else if (strcmp(format_str, "RG16") == 0)
    {
        return DRM_FORMAT_RGB565;
    }
    else if (strcmp(format_str, "AR24") == 0)
    {
        return DRM_FORMAT_ARGB8888;
    }
    else if (strcmp(format_str, "RA12") == 0)
    {
        return DRM_FORMAT_RGBA4444;
    }
    else if (strcmp(format_str, "RA15") == 0)
    {
        return DRM_FORMAT_RGBA5551;
    }
    else if (strcmp(format_str, "RA24") == 0)
    {
        return DRM_FORMAT_RGBA8888;
    } 
    else if (strcmp(format_str, "RG24") == 0)
    {
        return DRM_FORMAT_RGB888;
    }
    else if (strcmp(format_str, "AB12") == 0)
    {
        return DRM_FORMAT_ABGR4444;
    }
    else if (strcmp(format_str, "AB15") == 0)
    {
        return DRM_FORMAT_ABGR1555;
    }
    else if (strcmp(format_str, "BG16") == 0)
    {
        return DRM_FORMAT_BGR565;
    }
    else if (strcmp(format_str, "BG24") == 0)
    {
        return DRM_FORMAT_BGR888;
    }
    else if (strcmp(format_str, "AB24") == 0)
    {
        return DRM_FORMAT_ABGR8888;
    }
    else if (strcmp(format_str, "BA12") == 0)
    {
        return DRM_FORMAT_BGRA4444;
    }
    else if (strcmp(format_str, "BA15") == 0)
    {
        return DRM_FORMAT_BGRA5551;
    }
    else if (strcmp(format_str, "BA24") == 0)
    {
        return DRM_FORMAT_BGRA8888;
    }
    else if (strcmp(format_str, "YUYV") == 0)
    {
        return DRM_FORMAT_YUYV;
    }
    else if (strcmp(format_str, "YVYU") == 0)
    {
        return DRM_FORMAT_YVYU;
    }
    else if (strcmp(format_str, "NV12") == 0)
    {
        return DRM_FORMAT_NV12;
    }
    else if (strcmp(format_str, "NV21") == 0)
    {
        return DRM_FORMAT_NV21;
    }
    else
    {
        return 0; // Unsupported format
    }
}

void drm_load_config(const char *filename, x5_drm_context_t *drm_ctx)
{
    printf("Loading config file: %s\n", filename);

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("fopen");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data)
    {
        perror("malloc");
        fclose(file);
        return;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(data);
    if (!json)
    {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(data);
        return;
    }

    printf("Parsing JSON...\n");

    drm_ctx->crtc_id = cJSON_GetObjectItem(json, "crtc_id")->valueint;
    drm_ctx->connector_id = cJSON_GetObjectItem(json, "connector_id")->valueint;
    drm_ctx->width = cJSON_GetObjectItem(json, "width")->valueint;
    drm_ctx->height = cJSON_GetObjectItem(json, "height")->valueint;

    cJSON *planes = cJSON_GetObjectItem(json, "planes");
    drm_ctx->plane_count = cJSON_GetArraySize(planes);

    if (drm_ctx->plane_count > MAX_PLANES)
    {
        fprintf(stderr, "Plane count exceeds MAX_PLANES\n");
        drm_ctx->plane_count = MAX_PLANES;
    }

    for (int i = 0; i < drm_ctx->plane_count; i++)
    {
        cJSON *plane = cJSON_GetArrayItem(planes, i);

        drm_ctx->planes[i].plane_id = cJSON_GetObjectItem(plane, "plane_id")->valueint;
        drm_ctx->planes[i].src_w = cJSON_GetObjectItem(plane, "src_w")->valueint;
        drm_ctx->planes[i].src_h = cJSON_GetObjectItem(plane, "src_h")->valueint;
        drm_ctx->planes[i].crtc_x = cJSON_GetObjectItem(plane, "crtc_x")->valueint;
        drm_ctx->planes[i].crtc_y = cJSON_GetObjectItem(plane, "crtc_y")->valueint;
        drm_ctx->planes[i].crtc_w = cJSON_GetObjectItem(plane, "crtc_w")->valueint;
        drm_ctx->planes[i].crtc_h = cJSON_GetObjectItem(plane, "crtc_h")->valueint;
        strcpy(drm_ctx->planes[i].format, cJSON_GetObjectItem(plane, "format")->valuestring);

        cJSON *item = cJSON_GetObjectItem(plane, "z_pos");
        drm_ctx->planes[i].z_pos = item ? item->valueint : -1;

        item = cJSON_GetObjectItem(plane, "alpha");
        drm_ctx->planes[i].alpha = item ? item->valueint : -1;

        item = cJSON_GetObjectItem(plane, "pixel_blend_mode");
        drm_ctx->planes[i].pixel_blend_mode = item ? item->valueint : -1;

        item = cJSON_GetObjectItem(plane, "rotation");
        drm_ctx->planes[i].rotation = item ? item->valueint : -1;

        item = cJSON_GetObjectItem(plane, "color_encoding");
        drm_ctx->planes[i].color_encoding = item ? item->valueint : -1;

        item = cJSON_GetObjectItem(plane, "color_range");
        drm_ctx->planes[i].color_range = item ? item->valueint : -1;

        printf("------------------------------------------------------\n");
        printf("Plane %d:\n", i);
        printf("  Plane ID: %d\n", drm_ctx->planes[i].plane_id);
        printf("  Src W: %d\n", drm_ctx->planes[i].src_w);
        printf("  Src H: %d\n", drm_ctx->planes[i].src_h);
        printf("  CRTC X: %d\n", drm_ctx->planes[i].crtc_x);
        printf("  CRTC Y: %d\n", drm_ctx->planes[i].crtc_y);
        printf("  CRTC W: %d\n", drm_ctx->planes[i].crtc_w);
        printf("  CRTC H: %d\n", drm_ctx->planes[i].crtc_h);
        printf("  Format: %s\n", drm_ctx->planes[i].format);
        printf("  Z Pos: %d\n", drm_ctx->planes[i].z_pos);
        printf("  Alpha: %d\n", drm_ctx->planes[i].alpha);
        printf("  Pixel Blend Mode: %d\n", drm_ctx->planes[i].pixel_blend_mode);
        printf("  Rotation: %d\n", drm_ctx->planes[i].rotation);
        printf("  Color Encoding: %d\n", drm_ctx->planes[i].color_encoding);
        printf("  Color Range: %d\n", drm_ctx->planes[i].color_range);
        printf("------------------------------------------------------\n");
    }

    drm_ctx->max_buffers = drm_ctx->plane_count * ION_MAX_BUFFERS;
    drm_ctx->buffer_map = NULL; // 初始化哈希表指针
    drm_ctx->buffer_count = 0;

    cJSON_Delete(json);
    free(data);
}

void drm_cleanup(x5_drm_context_t *ctx)
{
    dma_buf_map_t *current, *tmp;

    // 释放 buffer_map 中的所有条目
    HASH_ITER(hh, ctx->buffer_map, current, tmp)
    {
        if (current->fb_id)
        {
            if (drmModeRmFB(ctx->drm_fd, current->fb_id) < 0)
            {
                perror("drmModeRmFB");
            }
        }
        HASH_DEL(ctx->buffer_map, current);
        free(current);
    }

    drmModeSetCrtc(ctx->drm_fd, ctx->crtc_id, 0, 0, 0, NULL, 0, NULL);

    drmModeRes *resources = drmModeGetResources(ctx->drm_fd);
    if (!resources)
    {
        perror("drmModeGetResources");
        return;
    }

    for (int i = 0; i < resources->count_crtcs; i++)
    {
        drmModeFreeCrtc(drmModeGetCrtc(ctx->drm_fd, resources->crtcs[i]));
    }

    for (int i = 0; i < resources->count_connectors; i++)
    {
        drmModeFreeConnector(drmModeGetConnector(ctx->drm_fd, resources->connectors[i]));
    }

    for (int i = 0; i < resources->count_encoders; i++)
    {
        drmModeFreeEncoder(drmModeGetEncoder(ctx->drm_fd, resources->encoders[i]));
    }

    drmModePlaneRes *plane_resources = drmModeGetPlaneResources(ctx->drm_fd);
    if (plane_resources)
    {
        for (uint32_t i = 0; i < plane_resources->count_planes; i++)
        {
            drmModeFreePlane(drmModeGetPlane(ctx->drm_fd, plane_resources->planes[i]));
        }
        drmModeFreePlaneResources(plane_resources);
    }

    drmModeFreeResources(resources);

    if (ctx->drm_fd >= 0)
    {
        close(ctx->drm_fd);
        ctx->drm_fd = -1;
    }

    printf("\r\nDRM resources cleaned up.\n");
}

void drm_set_client_capabilities(int drm_fd)
{
    if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1) < 0)
    {
        perror("drmSetClientCap DRM_CLIENT_CAP_ATOMIC");
    }

    if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0)
    {
        perror("drmSetClientCap DRM_CLIENT_CAP_UNIVERSAL_PLANES");
    }
}

int drm_setup_kms(x5_drm_context_t *ctx)
{
    drmModeRes *resources = drmModeGetResources(ctx->drm_fd);
    if (!resources)
    {
        perror("drmModeGetResources");
        return -1;
    }

    drmModeConnector *connector = drmModeGetConnector(ctx->drm_fd, ctx->connector_id);
    if (!connector)
    {
        perror("drmModeGetConnector");
        drmModeFreeResources(resources);
        return -1;
    }

    drmModeCrtc *crtc = drmModeGetCrtc(ctx->drm_fd, ctx->crtc_id);
    if (!crtc)
    {
        perror("drmModeGetCrtc");
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }

    drmModeModeInfo *mode = NULL;
    for (int i = 0; i < connector->count_modes; i++)
    {
        if (connector->modes[i].hdisplay == ctx->width && connector->modes[i].vdisplay == ctx->height)
        {
            mode = &connector->modes[i];
            break;
        }
    }

    if (!mode)
    {
        fprintf(stderr, "Mode not found\n");
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }

    uint32_t blob_id;
    if (drmModeCreatePropertyBlob(ctx->drm_fd, mode, sizeof(*mode), &blob_id) < 0)
    {
        perror("drmModeCreatePropertyBlob");
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }

    drmModeAtomicReq *req = drmModeAtomicAlloc();
    if (!req)
    {
        perror("drmModeAtomicAlloc");
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }

    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    add_property(ctx->drm_fd, req, ctx->crtc_id, DRM_MODE_OBJECT_CRTC, "ACTIVE", 1);
    add_property(ctx->drm_fd, req, ctx->crtc_id, DRM_MODE_OBJECT_CRTC, "MODE_ID", blob_id);
    add_property(ctx->drm_fd, req, ctx->connector_id, DRM_MODE_OBJECT_CONNECTOR, "CRTC_ID", ctx->crtc_id);

    for (int i = 0; i < ctx->plane_count; i++)
    {
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "SRC_X", 0);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "SRC_Y", 0);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "SRC_W", ctx->planes[i].src_w << 16);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "SRC_H", ctx->planes[i].src_h << 16);

        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_X", ctx->planes[i].crtc_x);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_Y", ctx->planes[i].crtc_y);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_W", ctx->planes[i].crtc_w);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_H", ctx->planes[i].crtc_h);

        if (ctx->planes[i].z_pos != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "zpos", ctx->planes[i].z_pos);
        }

        if (ctx->planes[i].alpha != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "alpha", ctx->planes[i].alpha);
        }

        if (ctx->planes[i].pixel_blend_mode != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "pixel blend mode", ctx->planes[i].pixel_blend_mode);
        }

        if (ctx->planes[i].rotation != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "rotation", ctx->planes[i].rotation);
        }

        if (ctx->planes[i].color_encoding != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "COLOR_ENCODING", ctx->planes[i].color_encoding);
        }

        if (ctx->planes[i].color_range != -1)
        {
            add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "COLOR_RANGE", ctx->planes[i].color_range);
        }
    }

    if (drmModeAtomicCommit(ctx->drm_fd, req, flags, NULL) < 0)
    {
        perror("drmModeAtomicCommit");
        drmModeAtomicFree(req);
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return -1;
    }

    drmModeAtomicFree(req);
    drmModeFreeCrtc(crtc);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);

    return 0;
}

void add_property(int drm_fd, drmModeAtomicReq *req, uint32_t obj_id, uint32_t obj_type, const char *name, uint64_t value)
{
    drmModeObjectProperties *props = drmModeObjectGetProperties(drm_fd, obj_id, obj_type);
    if (!props)
    {
        fprintf(stderr, "Failed to get properties for object %u\n", obj_id);
        return;
    }

    uint32_t prop_id = 0;
    for (uint32_t i = 0; i < props->count_props; i++)
    {
        drmModePropertyRes *prop = drmModeGetProperty(drm_fd, props->props[i]);
        if (!prop)
        {
            continue;
        }

        if (strcmp(prop->name, name) == 0)
        {
            prop_id = prop->prop_id;
            drmModeFreeProperty(prop);
            break;
        }

        drmModeFreeProperty(prop);
    }

    drmModeFreeObjectProperties(props);

    if (prop_id == 0)
    {
        fprintf(stderr, "Property '%s' not found on object %u\n", name, obj_id);
        return;
    }

    if (drmModeAtomicAddProperty(req, obj_id, prop_id, value) < 0)
    {
        fprintf(stderr, "Failed to add property '%s' on object %u: %s\n", name, obj_id, strerror(errno));
    }
}

uint32_t get_framebuffer(x5_drm_context_t *ctx, int dma_buf_fd, int plane_index)
{
    dma_buf_map_t *entry = NULL;
    HASH_FIND_INT(ctx->buffer_map, &dma_buf_fd, entry);
    if (entry)
    {
        return entry->fb_id;
    }

    if (ctx->buffer_count >= ctx->max_buffers)
    {
        printf("Buffer map is full, unable to add new framebuffer\n");
        return 0;
    }

    struct drm_prime_handle prime_handle = {
        .fd = dma_buf_fd,
        .flags = 0,
        .handle = 0,
    };

    if (drmIoctl(ctx->drm_fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_handle) < 0)
    {
        perror("DRM_IOCTL_PRIME_FD_TO_HANDLE");
        printf("Failed to map dma_buf_fd=%d to GEM handle\n", dma_buf_fd);
        return 0;
    }

    uint32_t handles[4] = {0};
    uint32_t strides[4] = {0};
    uint32_t offsets[4] = {0};
    if (strcmp(ctx->planes[plane_index].format, "NV12") == 0) {
        handles[0] = prime_handle.handle;
        strides[0] = ctx->planes[plane_index].src_w;
        offsets[0] = 0;
        handles[1] = prime_handle.handle;
        strides[1] = ctx->planes[plane_index].src_w;
        offsets[1] = ctx->planes[plane_index].src_w * ctx->planes[plane_index].src_h;
    } else if (strcmp(ctx->planes[plane_index].format, "RG24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "BG24") == 0){
        handles[0] = prime_handle.handle;
        strides[0] = ctx->planes[plane_index].src_w * 3;
        offsets[0] = 0;
    } else if (strcmp(ctx->planes[plane_index].format, "AR24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "RA24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "AG24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "GA24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "BA24") == 0 ||
                strcmp(ctx->planes[plane_index].format, "AB24") == 0) {
        handles[0] = prime_handle.handle;
        strides[0] = ctx->planes[plane_index].src_w * 4;
        offsets[0] = 0;       
    }

    uint32_t fb_id;
    uint32_t drm_format = get_format_from_string(ctx->planes[plane_index].format);
    if (drmModeAddFB2(ctx->drm_fd, ctx->planes[plane_index].src_w, ctx->planes[plane_index].src_h, drm_format, handles, strides, offsets, &fb_id, 0))
    {
        perror("drmModeAddFB2");
        return 0;
    }

    printf("Created new framebuffer: fb_id=%u for dma_buf_fd=%d\n", fb_id, dma_buf_fd);

    entry = (dma_buf_map_t *)malloc(sizeof(dma_buf_map_t));
    if (!entry)
    {
        perror("malloc");
        return 0;
    }

    entry->dma_buf_fd = dma_buf_fd;
    entry->fb_id = fb_id;
    HASH_ADD_INT(ctx->buffer_map, dma_buf_fd, entry);
    ctx->buffer_count++;

    return fb_id;
}

int drm_display_frame(x5_drm_context_t *ctx, int dma_buf_fds[MAX_PLANES])
{
    drmModeAtomicReq *req = drmModeAtomicAlloc();
    if (!req)
    {
        perror("drmModeAtomicAlloc");
        return -1;
    }

    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;

    for (int i = 0; i < ctx->plane_count; i++)
    {
        if (dma_buf_fds[i] == -1)
        {
            continue;
        }

        uint32_t fb_id = get_framebuffer(ctx, dma_buf_fds[i], i);
        if (fb_id == 0)
        {
            fprintf(stderr, "Failed to get framebuffer for plane %d\n", i);
            drmModeAtomicFree(req);
            return -1;
        }
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "CRTC_ID", ctx->crtc_id);
        add_property(ctx->drm_fd, req, ctx->planes[i].plane_id, DRM_MODE_OBJECT_PLANE, "FB_ID", fb_id);
    }

    int ret = drmModeAtomicCommit(ctx->drm_fd, req, flags, NULL);

    if (ret < 0)
    {
        perror("drmModeAtomicCommit");
        drmModeAtomicFree(req);
        return -1;
    }

    drmModeAtomicFree(req);

    return 0;
}

int create_and_map_buffer(x5_drm_context_t *ctx, int plane_index, void **mapped_memory, int *dma_buf_fd) {
    uint32_t format = get_format_from_string(ctx->planes[plane_index].format);
    uint32_t bpp = get_bpp_from_format(format);

    struct drm_prime_handle prime_handle;
    struct drm_mode_create_dumb create_dumb;
    struct drm_mode_map_dumb map_dumb;
    struct drm_mode_destroy_dumb destroy_dumb;

    // 分配一个 GEM 对象
    memset(&create_dumb, 0, sizeof(create_dumb));
    create_dumb.width = ctx->planes[plane_index].src_w;
    create_dumb.height = ctx->planes[plane_index].src_h;
    create_dumb.bpp = bpp;

    if (drmIoctl(ctx->drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb)) {
        perror("ioctl DRM_IOCTL_MODE_CREATE_DUMB failed");
        close(ctx->drm_fd);
        return -1;
    }

    // 准备导出 GEM 对象为 dma-buf 文件描述符
    memset(&prime_handle, 0, sizeof(prime_handle));
    prime_handle.handle = create_dumb.handle;
    prime_handle.flags = 0;

    if (drmIoctl(ctx->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_handle)) {
        perror("ioctl DRM_IOCTL_PRIME_HANDLE_TO_FD failed");
        // 清理创建的 dumb buffer
        memset(&destroy_dumb, 0, sizeof(destroy_dumb));
        destroy_dumb.handle = create_dumb.handle;
        drmIoctl(ctx->drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb);
        close(ctx->drm_fd);
        return -1;
    }

    *dma_buf_fd = prime_handle.fd;

    // 映射缓冲区
    memset(&map_dumb, 0, sizeof(map_dumb));
    map_dumb.handle = create_dumb.handle;
    if (drmIoctl(ctx->drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb)) {
        perror("ioctl DRM_IOCTL_MODE_MAP_DUMB failed");
        close(*dma_buf_fd);
        close(ctx->drm_fd);
        return -1;
    }

    // 映射内存
    *mapped_memory = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->drm_fd, map_dumb.offset);
    if (*mapped_memory == MAP_FAILED) {
        perror("mmap failed");
        close(*dma_buf_fd);
        close(ctx->drm_fd);
        return -1;
    }

    return 0;
}

