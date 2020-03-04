/*
 * Copyright 2017 Rockchip Electronics Co., Ltd
 *     Author: Randy Li <randy.li@rock-chips.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef  __GST_MPP_H264_ENC_H__
#define  __GST_MPP_H264_ENC_H__

#include "gstmppvideoenc.h"

/* Begin Declaration */
G_BEGIN_DECLS
#define GST_TYPE_MPP_H264_ENC	(gst_mpp_h264_enc_get_type())
#define GST_MPP_H264_ENC(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MPP_H264_ENC, GstMppH264Enc))
#define GST_MPP_H264_ENC_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MPP_H264_ENC, GstMppH264EncClass))
#define GST_IS_MPP_H264_ENC(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MPP_H264_ENC))
#define GST_IS_MPP_H264_ENC_CLASS(obj) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MPP_H264_ENC))

typedef struct _GstMppH264Enc GstMppH264Enc;
typedef struct _GstMppH264EncClass GstMppH264EncClass;

struct _GstMppH264Enc
{
  GstMppVideoEnc parent;

  gint mode;
  gint quality;
	gint profile;

  gulong bitrate;

  guint qp_min;
  guint qp_max;
  guint qp_step;
  guint intra;
};

struct _GstMppH264EncClass
{
  GstMppVideoEncClass parent_class;
};

GType gst_mpp_h264_enc_get_type (void);

enum {
  ARG_0,
  ARG_MODE,
  ARG_BITRATE,
  ARG_QUALITY,
	ARG_PROFILE,
  ARG_QP_MIN,
  ARG_QP_MAX,
  ARG_QP_STEP,
  ARG_INTRA,
};
#define PROFILE_HIGH 100
#define PROFILE_MAIN 77
#define PROFILE_BASELINE 66

#define SZ_1K (1024)
#define SZ_1M (SZ_1K * SZ_1K)

#define MIN_BITRATE (SZ_1K + 1)
#define MAX_BITRATE (100 * SZ_1M - 1)

#define ARG_MODE_DEFAULT MPP_ENC_RC_MODE_CBR
#define ARG_BITRATE_DEFAULT 3000000

#define ARG_QUALITY_DEFAULT MPP_ENC_RC_QUALITY_MEDIUM
#define ARG_PROFILE_DEFAULT PROFILE_HIGH

#define ARG_QP_MIN_DEFAULT 10
#define ARG_QP_MAX_DEFAULT 51
#define ARG_QP_STEP_DEFAULT 4
#define ARG_INTRA_DEFAULT 1

G_END_DECLS
#endif /* __GST_MPP_H264_ENC_H__ */
