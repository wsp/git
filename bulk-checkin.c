/*
 * Copyright (c) 2011, Google Inc.
 */
#include "bulk-checkin.h"
#include "csum-file.h"
#include "pack.h"

static int pack_compression_level = Z_DEFAULT_COMPRESSION;

static struct bulk_checkin_state {
	unsigned plugged:1;

	char *pack_tmp_name;
	struct sha1file *f;
	off_t offset;
	struct pack_idx_option pack_idx_opts;

	struct pack_idx_entry **written;
	uint32_t alloc_written;
	uint32_t nr_written;
} state;

static void finish_bulk_checkin(struct bulk_checkin_state *state)
{
	unsigned char sha1[20];
	char packname[PATH_MAX];
	int i;

	if (!state->f)
		return;

	if (state->nr_written == 0) {
		close(state->f->fd);
		unlink(state->pack_tmp_name);
		goto clear_exit;
	} else if (state->nr_written == 1) {
		sha1close(state->f, sha1, CSUM_FSYNC);
	} else {
		int fd = sha1close(state->f, sha1, 0);
		fixup_pack_header_footer(fd, sha1, state->pack_tmp_name,
					 state->nr_written, sha1,
					 state->offset);
		close(fd);
	}

	sprintf(packname, "%s/pack/pack-", get_object_directory());
	finish_tmp_packfile(packname, state->pack_tmp_name,
			    state->written, state->nr_written,
			    &state->pack_idx_opts, sha1);
	for (i = 0; i < state->nr_written; i++)
		free(state->written[i]);

clear_exit:
	free(state->written);
	memset(state, 0, sizeof(*state));

	/* Make objects we just wrote available to ourselves */
	reprepare_packed_git();
}

static int already_written(struct bulk_checkin_state *state, unsigned char sha1[])
{
	int i;

	/* The object may already exist in the repository */
	if (has_sha1_file(sha1))
		return 1;

	/* Might want to keep the list sorted */
	for (i = 0; i < state->nr_written; i++)
		if (!hashcmp(state->written[i]->sha1, sha1))
			return 1;
	return 0;
}

static void deflate_to_pack(struct bulk_checkin_state *state,
			    unsigned char sha1[],
			    int fd, size_t size, enum object_type type,
			    const char *path, unsigned flags)
{
	unsigned char obuf[16384];
	unsigned hdrlen;
	git_zstream s;
	git_SHA_CTX ctx;
	int write_object = (flags & HASH_WRITE_OBJECT);
	int status = Z_OK;
	struct pack_idx_entry *idx = NULL;
	struct sha1file_checkpoint checkpoint;

	hdrlen = sprintf((char *)obuf, "%s %" PRIuMAX,
			 typename(type), (uintmax_t)size) + 1;
	git_SHA1_Init(&ctx);
	git_SHA1_Update(&ctx, obuf, hdrlen);

	if (write_object) {
		idx = xcalloc(1, sizeof(*idx));
		idx->offset = state->offset;
		sha1file_checkpoint(state->f, &checkpoint);
		crc32_begin(state->f);
	}
	memset(&s, 0, sizeof(s));
	git_deflate_init(&s, pack_compression_level);

	hdrlen = encode_in_pack_object_header(type, size, obuf);
	s.next_out = obuf + hdrlen;
	s.avail_out = sizeof(obuf) - hdrlen;

	while (status != Z_STREAM_END) {
		unsigned char ibuf[16384];

		if (size && !s.avail_in) {
			ssize_t rsize = size < sizeof(ibuf) ? size : sizeof(ibuf);
			if (xread(fd, ibuf, rsize) != rsize)
				die("failed to read %d bytes from '%s'",
				    (int)rsize, path);
			git_SHA1_Update(&ctx, ibuf, rsize);
			s.next_in = ibuf;
			s.avail_in = rsize;
			size -= rsize;
		}

		status = git_deflate(&s, size ? 0 : Z_FINISH);

		if (!s.avail_out || status == Z_STREAM_END) {
			size_t written = s.next_out - obuf;
			if (write_object) {
				sha1write(state->f, obuf, written);
				state->offset += written;
			}
			s.next_out = obuf;
			s.avail_out = sizeof(obuf);
		}

		switch (status) {
		case Z_OK:
		case Z_BUF_ERROR:
		case Z_STREAM_END:
			continue;
		default:
			die("unexpected deflate failure: %d", status);
		}
	}
	git_deflate_end(&s);
	git_SHA1_Final(sha1, &ctx);
	if (write_object) {
		idx->crc32 = crc32_end(state->f);

		if (already_written(state, sha1)) {
			sha1file_truncate(state->f, &checkpoint);
			state->offset = checkpoint.offset;
			free(idx);
		} else {
			hashcpy(idx->sha1, sha1);
			ALLOC_GROW(state->written,
				   state->nr_written + 1, state->alloc_written);
			state->written[state->nr_written++] = idx;
		}
	}
}

int index_bulk_checkin(unsigned char *sha1,
		       int fd, size_t size, enum object_type type,
		       const char *path, unsigned flags)
{
	if (!state.f && (flags & HASH_WRITE_OBJECT)) {
		state.f = create_tmp_packfile(&state.pack_tmp_name);
		reset_pack_idx_option(&state.pack_idx_opts);
		/* Pretend we are going to write only one object */
		state.offset = write_pack_header(state.f, 1);
		if (!state.offset)
			die_errno("unable to write pack header");
	}

	deflate_to_pack(&state, sha1, fd, size, type, path, flags);
	if (!state.plugged)
		finish_bulk_checkin(&state);
	return 0;
}

void plug_bulk_checkin(void)
{
	state.plugged = 1;
}

void unplug_bulk_checkin(void)
{
	state.plugged = 0;
	if (state.f)
		finish_bulk_checkin(&state);
}
