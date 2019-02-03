/*
    This file is part of trim2unmap.
    Copyright (C) 2019 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <scsi/sg.h>

#include <buse.h>

static const char AppName[] = "trim2unmap";

static int fd = -1;
static size_t block_size = 0;
static int64_t device_size = 0;


const static uint8_t scsi_cmd_buf[10] = {0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00};

static uint8_t scsi_xfer_buf[24] = {0x00, 0x16, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				    0x00, 0x00, 0x00, 0x00, // LBA
				    0x00, 0x00, 0x00, 0x00, // Length
				    0x00, 0x00, 0x00, 0x00};

static uint32_t *scsi_unmap_lba = (uint32_t *)(scsi_xfer_buf + 12);
static uint32_t *scsi_unmap_num = (uint32_t *)(scsi_xfer_buf + 12 + 4);

static sg_io_hdr_t scsi_hdr = {
	.interface_id = 'S',
	.dxfer_direction = SG_DXFER_TO_DEV,
	.cmd_len = 10,
	.cmdp = (u_char *)scsi_cmd_buf,
	.mx_sb_len = 64,
	.iovec_count = 0,
	.dxfer_len = 24,
	.timeout = 60000,
	.flags = 0,
	.dxferp = scsi_xfer_buf
};

static int loopback_read(void *buf, uint32_t len, uint64_t offset, void *userdata) {
	lseek64(fd, offset, SEEK_SET);

	while (len > 0) {
		ssize_t bytes_read = read(fd, buf, len);
		assert(bytes_read > 0);
		len -= bytes_read;
		buf = (uint8_t *) buf + bytes_read;
	}

	return 0;
}

static int loopback_write(const void *buf, uint32_t len, uint64_t offset, void *userdata) {
	lseek64(fd, offset, SEEK_SET);

	while (len > 0) {
		ssize_t bytes_written = write(fd, buf, len);
		assert(bytes_written > 0);
		len -= bytes_written;
		buf = (uint8_t *) buf + bytes_written;
	}

	return 0;
}

static int loopback_trim(uint64_t from, uint32_t len, void *userdata) {
	fprintf(stderr, "%s: Debug: Trim: start=%" PRIu64 ", len=%" PRIu32 "\n", AppName, from, len);

	if (from % block_size) {
		fprintf(stderr, "%s: Trim: Error: start position (%" PRIu64 ") is not multiple of block size (%zu) !!\n", AppName, from, block_size);
		abort();
	}

	if (len % block_size) {
		fprintf(stderr, "%s: Trim: Error: length (%" PRIu32 ") is not multiple of block size (%zu) !!\n", AppName, len, block_size);
		abort();
	}

	uint64_t lba = from / block_size;
	uint64_t num = len / block_size;

	if (lba > UINT32_MAX) {
		fprintf(stderr, "%s: Trim: Error: calculated LBA (%" PRIu64 ") exceeds UINT32_MAX !!\n", AppName, lba);
		abort();
	}

	if (num > UINT32_MAX) {
		fprintf(stderr, "%s: Trim: Error: calculated NUM (%" PRIu64 ") exceeds UINT32_MAX !!\n", AppName, num);
		abort();
	}

	*scsi_unmap_lba = htobe32((uint32_t)lba);
	*scsi_unmap_num = htobe32((uint32_t)num);

	if (ioctl(fd, SG_IO, &scsi_hdr) < 0) {
		fprintf(stderr, "%s: Trim: Error: SG_IO ioctl failed: %s\n", AppName, strerror(errno));
		abort();
	}

	return 0;
}

static int loopback_flush(void *userdata) {
	fprintf(stderr, "%s: Debug: Flush: Begin\n", AppName);

	if (ioctl(fd, BLKFLSBUF, 0) < 0) {
		fprintf(stderr, "%s: Flush: Error: BLKFLSBUF ioctl failed: %s\n", AppName, strerror(errno));
		abort();
	}

	fprintf(stderr, "%s: Debug: Flush: Done\n", AppName);

	return 0;
}

static struct buse_operations bop = {
	.read = loopback_read,
	.write = loopback_write,
	.trim = loopback_trim,
	.flush = loopback_flush
};

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <phyical device> <virtual device>\n", AppName);
		return -1;
	}

	fd = open(argv[1], O_RDWR|O_LARGEFILE);

	if (fd == -1) {
		fprintf(stderr, "%s: failed to open `%s': %s\n", AppName, argv[1], strerror(errno));
		abort();
	}



	if (ioctl(fd, BLKGETSIZE64, &device_size) == -1) {
		fprintf(stderr, "%s: failed to get size of `%s': %s\n", AppName, argv[1], strerror(errno));
		abort();
	}

	fprintf(stderr, "%s: Info: The size of this device is %ld bytes.\n", AppName, device_size);

	if (ioctl(fd, BLKSSZGET, &block_size) == -1) {
		fprintf(stderr, "trim2unmap: failed to get block size of `%s': %s\n", argv[1], strerror(errno));
		abort();
	}

	fprintf(stderr, "%s: Info: The block size of this device is %ld bytes.\n", AppName, block_size);


	bop.size = device_size;
	bop.blksize = block_size;

	buse_main(argv[2], &bop, NULL);

	return 0;
}
