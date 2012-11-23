/*
 * q_stat - q value statistics tool
 *
 * Author: Ramax Lo <ramax.lo@vivotek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include "list_head.h"

int total_frame = 0;
int total_i_frame = 0;
int total_b_frame = 0;
int total_p_frame = 0;
int total_unknown = 0;
LIST_INIT(cache);

struct qentry
{
	struct TListHead entry;
	double qvalue;
	int count;
};

void help()
{
	printf("Usage: q_stat <Video file>\n");
}

double calc_qvalue(AVCodecContext *ctx, AVFrame *frame)
{
	int8_t *qtable = frame->qscale_table;
	int qstride = frame->qstride;
	int width = (ctx->width + 15) >> 4;
	int height = (ctx->height + 15) >> 4;
	int sum = 0;
	int x, y;
	int idx;
	int qbytes = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			idx = (y * qstride + x) * qbytes;
			sum += qtable[idx];
		}
	}

	return (double)sum / (width * height);
}

void update_qvalue_cache(double qvalue)
{
	struct TListHead *e;
	struct qentry *p, *p1;
	int found = 0;

	foreach (e, &cache)
	{
		p = container_of(e, struct qentry, entry);
		if (p->qvalue == qvalue)
		{
			p->count++;
			found = 1;
			break;
		}
	}

	if (found == 0)
	{
		p = malloc(sizeof(struct qentry));
		assert(p);

		p->qvalue = qvalue;
		p->count = 1;
		InitList(&p->entry);

		if (ListIsEmpty(&cache))
			ListAddTail(&p->entry, &cache);
		else
		{
			foreach (e, &cache)
			{
				p1 = container_of(e, struct qentry, entry);
				if (p1->qvalue > p->qvalue)
				{
					ListAdd(&p->entry, ListPrev(e));
					break;
				}
			}
		}
	}
}

void destroy_qvalue_cache()
{
	struct TListHead *e, *tmp;
	struct qentry *p;

	foreach_safe (e, tmp, &cache)
	{
		p = container_of(e, struct qentry, entry);
		free(p);
	}
}

void update_stat(AVCodecContext *ctx, AVFrame *frame)
{
	double qvalue;

	total_frame++;
	switch (frame->pict_type)
	{
		case FF_B_TYPE:
			total_b_frame++;
			break;
		case FF_P_TYPE:
			total_p_frame++;
			break;
		case FF_I_TYPE:
			total_i_frame++;
			break;
		default:
			total_unknown++;
			break;
	}

	qvalue = calc_qvalue(ctx, frame);
	update_qvalue_cache(qvalue);
}

void dump_qvalue()
{
	struct TListHead *e;
	struct qentry *p;
	double sum = 0;

	foreach (e, &cache)
	{
		p = container_of(e, struct qentry, entry);
		printf("%.04f: %d (%.2f)\n", p->qvalue, p->count, (double)p->count / total_frame * 100);

		sum += p->qvalue * p->count;
	}

	printf("Avg: %.04f\n", sum / total_frame);
}

void dump_stat()
{
	printf("\n");
	printf("Total:   %d\n", total_frame);
	printf("I:       %d (%.02f)\n", total_i_frame, (double)total_i_frame / total_frame * 100);
	printf("B:       %d (%.02f)\n", total_b_frame, (double)total_b_frame / total_frame * 100);
	printf("P:       %d (%.02f)\n", total_p_frame, (double)total_p_frame / total_frame * 100);
	printf("UNKNOWN: %d (%.02f)\n", total_unknown, (double)total_unknown / total_frame * 100);

	printf("==== qvalue statistics ====\n");
	dump_qvalue();
}

void print_err(int error)
{
	char buf[128];

	av_strerror(error, buf, 128);
	printf("%s\n", buf);
}

int main(int argc, char *argv[])
{
	AVFormatContext *fmtctx = NULL;
	AVCodecContext *codecctx = NULL;
	AVCodec *codec = NULL;
	AVFrame *frame = NULL;
	AVPacket packet;
	AVDictionary *options = NULL;
	int frame_finish;
	char *input_file;
	int rc;
	int video_stream = -1;
	int i;

	if (argc < 2)
	{
		help();
		return 1;
	}

	input_file = argv[1];

	av_register_all();

	rc = avformat_open_input(&fmtctx, input_file, NULL, NULL);
	if (rc)
	{
		printf("Can't open file: %s: ", input_file);
		print_err(rc);
		return 1;
	}

	rc = avformat_find_stream_info(fmtctx, NULL);
	if (rc < 0)
	{
		printf("Can't find stream info: ");
		print_err(rc);
		goto err_find_str_info;
	}

//	av_dump_format(fmtctx, 0, input_file, 0);

	for (i = 0; i < fmtctx->nb_streams; i++)
	{
		if (fmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream = i;
			break;
		}
	}
	if (video_stream == -1)
	{
		printf("No video stream found\n");
		rc = 1;
		goto err_find_str_info;
	}

	codecctx = fmtctx->streams[video_stream]->codec;
	codec = avcodec_find_decoder(codecctx->codec_id);
	if (codec == NULL)
	{
		printf("No suitable codec found: %d\n", codecctx->codec_id);
		rc = 1;
		goto err_no_codec;
	}

	rc = avcodec_open2(codecctx, codec, &options);
	if (rc < 0)
	{
		printf("Can't open codec: ");
		print_err(rc);
		goto err_no_codec;
	}

	frame = avcodec_alloc_frame();
	if (frame == NULL)
	{
		printf("Can't alloc frame\n");
		rc = 1;
		goto err_no_codec;
	}

	while (av_read_frame(fmtctx, &packet) >= 0)
	{
		if (packet.stream_index == video_stream)
		{
			avcodec_decode_video2(codecctx, frame, &frame_finish, &packet);

			if (frame_finish)
				update_stat(codecctx, frame);
		}

		av_free_packet(&packet);
	}

	dump_stat();

	rc = 0;

	destroy_qvalue_cache();
	av_free(frame);
err_no_codec:
	avcodec_close(codecctx);
err_find_str_info:
	avformat_close_input(&fmtctx);

	return rc;
}
