#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include "buffers.h"

struct bo *bo_obj;
struct bo *bo_obj_osd[8];

void print_usage(char *arg)
{
	printf("usage:%s <crtc_id> <connector_id> <mode_id> <osd_width> <osd_height> <file_name(primary_plane)>\n", arg);
}

static uint32_t get_property_id(int fd, drmModeObjectProperties *props,
				const char *name)
{
	drmModePropertyPtr property;
	uint32_t i, id = 0;

	for (i = 0; i < props->count_props; i++) {
		property = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(property->name, name))
			id = property->prop_id;
		drmModeFreeProperty(property);

		if (id)
			break;
	}

	return id;
}

int main(int argc, char *argv[])
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	drmModePlaneRes *plane_res;
	drmModeObjectProperties *props;
	drmModeAtomicReq *req;
	uint32_t crtc_id;
	uint32_t conn_id;
	uint32_t mode_id;
	uint32_t osd_width;
	uint32_t osd_height;
	uint32_t plane_id;
	uint32_t blob_id;
	uint32_t property_crtc_id;
	uint32_t property_mode_id;
	uint32_t property_active;
	uint32_t property_fb_id;

	uint32_t property_ow_fb_id;
	uint32_t property_osd_en;
	uint32_t property_osd_colorkey_en;
	uint32_t property_osd_colorkey;

	uint32_t disp_id = 0;
	int plane_num_each_crtc = 0;
	int i = 0;
	char filename[30];
	char osd_file_name[30];

	if (argc < 7) {
		print_usage(argv[0]);
		return 0;
	} else {
		sscanf(argv[1], "%u", &crtc_id);
		sscanf(argv[2], "%u", &conn_id);
		sscanf(argv[3], "%u", &mode_id);
		sscanf(argv[4], "%u", &osd_width);
		sscanf(argv[5], "%u", &osd_height);
		sscanf(argv[6], "%s", filename);
	}

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	res = drmModeGetResources(fd);

	for (i = 0; i < res->count_crtcs; i++) {
		if(crtc_id == res->crtcs[i]) {
			disp_id = i;
			break;
		}
		if(i == res->count_crtcs -1) {
			printf("not find crtc-%u\n", crtc_id);
			close(fd);
			return 0;
		}
	}

	for (i = 0; i < res->count_connectors; i++) {
		if(conn_id == res->connectors[i])
			break;
		if(i == res->count_connectors -1) {
			printf("not find connector-%u\n", conn_id);
			close(fd);
			return 0;
		}
	}

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	plane_res = drmModeGetPlaneResources(fd);
	//disp0/1 has same num planes
	plane_num_each_crtc = plane_res->count_planes / res->count_crtcs;
	plane_id = plane_res->planes[disp_id * plane_num_each_crtc];

	conn = drmModeGetConnector(fd, conn_id);

	bo_obj = bo_create(fd, DRM_FORMAT_RGB888,
		conn->modes[mode_id].hdisplay, conn->modes[mode_id].vdisplay, filename);

	for (i = 0; i < 5; i++){
		sprintf(osd_file_name, "OSD_%d.rgb", i);
		bo_obj_osd[i] = bo_create(fd, DRM_FORMAT_ARGB8888, osd_width,
									osd_height, osd_file_name);
	}

	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);

	props = drmModeObjectGetProperties(fd, conn_id,	DRM_MODE_OBJECT_CONNECTOR);
	property_crtc_id = get_property_id(fd, props, "CRTC_ID");
	drmModeFreeObjectProperties(props);

	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	property_active = get_property_id(fd, props, "ACTIVE");
	property_mode_id = get_property_id(fd, props, "MODE_ID");
	drmModeFreeObjectProperties(props);

	drmModeCreatePropertyBlob(fd, &conn->modes[mode_id],
				sizeof(conn->modes[mode_id]), &blob_id);

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, crtc_id, property_active, 1);
	drmModeAtomicAddProperty(req, crtc_id, property_mode_id, blob_id);
	drmModeAtomicAddProperty(req, conn_id, property_crtc_id, crtc_id);
	drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	drmModeAtomicFree(req);

	printf("drmModeAtomicCommit SetCrtc\n");
	getchar();

	props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
	property_crtc_id = get_property_id(fd, props, "CRTC_ID");
	property_fb_id = get_property_id(fd, props, "FB_ID");
	drmModeFreeObjectProperties(props);

	props = drmModeObjectGetProperties(fd, plane_id + 1, DRM_MODE_OBJECT_PLANE);
	property_ow_fb_id = get_property_id(fd, props, "OW_FB_ID");
	property_osd_en = get_property_id(fd, props, "OSD_EN");
	property_osd_colorkey_en = get_property_id(fd, props, "OSD_COLORKEY_EN");
	property_osd_colorkey = get_property_id(fd, props, "OSD_COLORKEY");
	drmModeFreeObjectProperties(props);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);

	printf("============== Set Osd0\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_ow_fb_id, 0);
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_en, 0x1);
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_colorkey_en, 0);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);
	drmModeSetPlane(fd, plane_id + 1, crtc_id, bo_obj_osd[0]->fb_id, 0,
				0, 0, osd_width, osd_height,
				0 << 16, 0 << 16, osd_width << 16, osd_height << 16);

	printf("============== Set Osd1\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_en, 0x3);
	drmModeAtomicAddProperty(req, plane_id + 1, property_ow_fb_id, 1);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);
	drmModeSetPlane(fd, plane_id + 1, crtc_id, bo_obj_osd[1]->fb_id, 0,
				40 + osd_width, 40 + osd_height, osd_width, osd_height,
				0 << 16, 0 << 16, osd_width << 16, osd_height << 16);

	printf("============== Set Osd2\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_en, 0x7);
	drmModeAtomicAddProperty(req, plane_id + 1, property_ow_fb_id, 2);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);
	drmModeSetPlane(fd, plane_id + 1, crtc_id, bo_obj_osd[2]->fb_id, 0,
				80 + osd_width * 2, 80 + osd_height * 2, osd_width, osd_height,
				0 << 16, 0 << 16, osd_width << 16, osd_height << 16);
	printf("============== Set Osd3\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_en, 0xf);
	drmModeAtomicAddProperty(req, plane_id + 1, property_ow_fb_id, 3);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);
	drmModeSetPlane(fd, plane_id + 1, crtc_id, bo_obj_osd[3]->fb_id, 0,
				120 + osd_width * 3, 120 + osd_height * 3, osd_width, osd_height,
				0 << 16, 0 << 16, osd_width << 16, osd_height << 16);

	printf("============== Set Osd4\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_en, 0x1f);
	drmModeAtomicAddProperty(req, plane_id + 1, property_ow_fb_id, 4);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);

	drmModeSetCrtc(fd, crtc_id, bo_obj->fb_id,
				0, 0, &conn_id, 1, &conn->modes[mode_id]);
	drmModeSetPlane(fd, plane_id + 1, crtc_id, bo_obj_osd[4]->fb_id, 0,
				160 + osd_width * 4, 160 + osd_height * 4, osd_width, osd_height,
				0 << 16, 0 << 16, osd_width << 16, osd_height << 16);

	printf("============== Colorkey Test\n");
	getchar();

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_colorkey_en, 0x1);
	drmModeAtomicAddProperty(req, plane_id + 1, property_osd_colorkey, 0x0);
	drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	drmModeAtomicFree(req);

	getchar();

	bo_destroy(bo_obj);
	 for (i = 0; i < 5; i++)
		bo_destroy(bo_obj_osd[i]);

	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}
