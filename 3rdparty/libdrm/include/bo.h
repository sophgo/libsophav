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

struct bo
{
	int fd;
	void *ptr;
	size_t size;
	size_t offset;
	size_t pitch;
	unsigned handle;
	uint32_t fb_id;
    int buff_fd;

    void *planes[3];
    unsigned int plane_num;
    unsigned int length[3];
};

struct bo *bo_create_dumb_fd(int fd, unsigned int width, unsigned int height, unsigned int format);
void bo_destroy_dumb(struct bo *bo);
void bo_destroy(struct bo *bo);
