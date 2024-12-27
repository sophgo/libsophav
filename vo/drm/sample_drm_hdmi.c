#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include "buffers.h"

uint32_t disp_formats[] = {
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
};

struct bo *bo_obj;

void print_usage(char *arg)
{
	printf("usage:%s <file_name> <crtc_id> <connector_id> <mode_id> <fmt_id>\n", arg);
	printf("fmt_id:0-NV12, 1-NV21, 2-YUYV, 3-YVYU, 4-UYVY, 5-VYUY, 6-YUV420\n");
	printf("       7-YUV422, 8-RGB888, 9-BGR888 (RGB need bmp fmt)\n");
}

int main(int argc, char *argv[])
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	char filename[32];
	uint32_t crtc_id;
	uint32_t conn_id;
	uint32_t mode_id;
	uint32_t fmt_id;
	if (argc < 3) {
		print_usage(argv[0]);
		return 0;
	} else {
		sscanf(argv[1], "%s", filename);
		sscanf(argv[2], "%u", &crtc_id);
		sscanf(argv[3], "%u", &conn_id);
		sscanf(argv[4], "%u", &mode_id);
		sscanf(argv[5], "%u", &fmt_id);
	}

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	res = drmModeGetResources(fd);

	for (int i = 0; i < res->count_crtcs; i++) {
		if(crtc_id == res->crtcs[i])
			break;
		if(i == res->count_crtcs -1) {
			printf("not find crtc-%u\n", crtc_id);
			close(fd);
			return 0;
		}
	}

	for (int i = 0; i < res->count_connectors; i++) {
		if(conn_id == res->connectors[i])
			break;
		if(i == res->count_connectors -1) {
			printf("not find connector-%u\n", conn_id);
			close(fd);
			return 0;
		}
	}

	conn = drmModeGetConnector(fd, conn_id);

		bo_obj = bo_create(fd, disp_formats[fmt_id],
			conn->modes[mode_id].hdisplay, conn->modes[mode_id].vdisplay, filename);

		drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);

		printf("drmModeSetCrtc\n");
		getchar();

		bo_destroy(bo_obj);

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}
