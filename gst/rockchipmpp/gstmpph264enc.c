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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "gstmpph264enc.h"

#define GST_CAT_DEFAULT mppvideoenc_debug

#define GST_MPP_H264_ENC_MODE_TYPE (gst_mpp_h264_enc_mode_get_type())
static GType
gst_mpp_h264_enc_mode_get_type (void)
{
  static GType type = 0;

  static const GEnumValue types[] = {
    {MPP_ENC_RC_MODE_VBR, "Variable bitrate", "vbr"},
    {MPP_ENC_RC_MODE_CBR, "Constant bitrate", "cbr"},
    {0, NULL, NULL}
  };

  if (!type) {
    type = g_enum_register_static ("GstMppH264EncMode", types);
  }

  return type;
}

#define GST_MPP_H264_ENC_QUALITY_TYPE (gst_mpp_h264_enc_quality_get_type())
static GType
gst_mpp_h264_enc_quality_get_type (void)
{
  static GType type = 0;

  static const GEnumValue types[] = {
    {MPP_ENC_RC_QUALITY_WORST, "Worst", "worst"},
    {MPP_ENC_RC_QUALITY_WORSE, "Worse", "worse"},
    {MPP_ENC_RC_QUALITY_MEDIUM, "Medium", "medium"},
    {MPP_ENC_RC_QUALITY_BETTER, "Better", "better"},
    {MPP_ENC_RC_QUALITY_BEST, "Best", "best"},
    {MPP_ENC_RC_QUALITY_CQP, "[vbr] Constant QP (uses qp-min)", "cqp"},
    {0, NULL, NULL}
  };

  if (!type) {
    type = g_enum_register_static ("GstMppH264EncQuality", types);
  }

  return type;
}

#define GST_MPP_H264_ENC_PROFILE_TYPE (gst_mpp_h264_enc_profile_get_type())
static GType
gst_mpp_h264_enc_profile_get_type (void)
{
  static GType type = 0;

  static const GEnumValue types[] = {
    {PROFILE_BASELINE, "Baseline", "baseline"},
    {PROFILE_MAIN, "Main", "main"},
    {PROFILE_HIGH, "High", "high"},
    {0, NULL, NULL}
  };

  if (!type) {
    type = g_enum_register_static ("GstMppH264EncProfile", types);
  }

  return type;
}

#define parent_class gst_mpp_h264_enc_parent_class
G_DEFINE_TYPE (GstMppH264Enc, gst_mpp_h264_enc, GST_TYPE_MPP_VIDEO_ENC);

static GstStaticPadTemplate gst_mpp_h264_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264, "
        "width  = (int) [ 32, 1920 ], "
        "height = (int) [ 32, 1088 ], "
        "framerate = (fraction) [0/1, 60/1], "
        "stream-format = (string) { byte-stream }, "
        "alignment = (string) { au }, " "profile = (string) { high }")
    );

static gboolean
gst_mpp_h264_enc_open (GstVideoEncoder * encoder)
{
  GstMppVideoEnc *self = GST_MPP_VIDEO_ENC (encoder);

  GST_DEBUG_OBJECT (self, "Opening");

  if (mpp_create (&self->mpp_ctx, &self->mpi))
    goto failure;
  if (mpp_init (self->mpp_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC))
    goto failure;

  return TRUE;

failure:
  return FALSE;
}

static gboolean
gst_mpp_h264_enc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstMppH264Enc *self = GST_MPP_H264_ENC (encoder);
  GstMppVideoEnc *mpp_video_enc = GST_MPP_VIDEO_ENC (encoder);
  MppEncCodecCfg codec_cfg;
  MppEncRcCfg rc_cfg;

  memset (&rc_cfg, 0, sizeof (rc_cfg));
  memset (&codec_cfg, 0, sizeof (codec_cfg));

  codec_cfg.coding = MPP_VIDEO_CodingAVC;

  codec_cfg.h264.profile = self->profile;
  if (codec_cfg.h264.profile == PROFILE_HIGH) {
    /* Can only be 1 in high profile mode */
    codec_cfg.h264.transform8x8_mode = 1;
  }

  codec_cfg.h264.level = 40;

  // 0 | 1
  codec_cfg.h264.entropy_coding_mode = 1;
  // When coding mode 1 => 0 - 2
  codec_cfg.h264.cabac_init_idc = 0;

  rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_ALL;
  rc_cfg.rc_mode = self->mode;

  /* Estimation of the target bitrate for a GOP */
  rc_cfg.bps_target = GST_VIDEO_INFO_WIDTH (&state->info)
      * GST_VIDEO_INFO_HEIGHT (&state->info)
      / 8 * GST_VIDEO_INFO_FPS_N (&state->info)
      / GST_VIDEO_INFO_FPS_D (&state->info);

  /* Use bitrate if provided by user */
  if (self->bitrate > 0) {
    rc_cfg.bps_target = self->bitrate;
  }

  rc_cfg.fps_in_flex = 0;
  rc_cfg.fps_in_num = GST_VIDEO_INFO_FPS_N (&state->info);
  rc_cfg.fps_in_denorm = GST_VIDEO_INFO_FPS_D (&state->info);
  rc_cfg.fps_out_flex = 0;
  rc_cfg.fps_out_num = GST_VIDEO_INFO_FPS_N (&state->info);
  rc_cfg.fps_out_denorm = GST_VIDEO_INFO_FPS_D (&state->info);
  rc_cfg.gop = GST_VIDEO_INFO_FPS_N (&state->info)
      / GST_VIDEO_INFO_FPS_D (&state->info) * self->intra;
  rc_cfg.skip_cnt = 0;

  if (rc_cfg.rc_mode == MPP_ENC_RC_MODE_CBR) {
    /* Constant Bitrate Mode
     *
     * Bitrate is defined by rc_cfg.bps_* settings ONLY
     * quality parameters and codec_cfg.h264.qp_* parameters are ignored.
     */

    codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
        MPP_ENC_H264_CFG_CHANGE_ENTROPY | MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;

    rc_cfg.bps_min = rc_cfg.bps_target * 15 / 16;
    rc_cfg.bps_max = rc_cfg.bps_target * 17 / 16;
  } else if (rc_cfg.rc_mode == MPP_ENC_RC_MODE_VBR) {
    /* Variable Bitrate Mode
     *
     * Bitrate is defined by quality and qp parameters
     * rc_cfg.bps_* are ignored
     */

    codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
        MPP_ENC_H264_CFG_CHANGE_ENTROPY | MPP_ENC_H264_CFG_CHANGE_QP_LIMIT |
        MPP_ENC_H264_CFG_CHANGE_ALL;

    rc_cfg.quality = self->quality;

    codec_cfg.h264.qp_init = self->qp_min;
    codec_cfg.h264.qp_min = self->qp_min;
    codec_cfg.h264.qp_max = self->qp_max;
    codec_cfg.h264.qp_max_step = self->qp_step;

    rc_cfg.bps_max = MAX_BITRATE;
    rc_cfg.bps_min = MIN_BITRATE;

    if (rc_cfg.quality == MPP_ENC_RC_QUALITY_CQP) {
      /* Constant QP mode */
      codec_cfg.h264.qp_init = self->qp_min;
      codec_cfg.h264.qp_max = self->qp_min;
      codec_cfg.h264.qp_min = self->qp_min;
      codec_cfg.h264.qp_max_step = 0;

      rc_cfg.bps_target = -1;
      rc_cfg.bps_max = -1;
      rc_cfg.bps_min = -1;
    }
  }

  if (mpp_video_enc->mpi->control (mpp_video_enc->mpp_ctx, MPP_ENC_SET_RC_CFG,
          &rc_cfg)) {
    GST_DEBUG_OBJECT (self, "Setting rate control for rockchip mpp failed");
    return FALSE;
  }

  if (mpp_video_enc->mpi->control (mpp_video_enc->mpp_ctx,
          MPP_ENC_SET_CODEC_CFG, &codec_cfg)) {
    GST_DEBUG_OBJECT (self, "Setting codec info for rockchip mpp failed");
    return FALSE;
  }

  return GST_MPP_VIDEO_ENC_CLASS (parent_class)->set_format (encoder, state);
}

static GstFlowReturn
gst_mpp_h264_enc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstCaps *outcaps;
  GstStructure *structure;
  GstFlowReturn ret;

  outcaps = gst_caps_new_empty_simple ("video/x-h264");
  structure = gst_caps_get_structure (outcaps, 0);
  gst_structure_set (structure, "stream-format",
      G_TYPE_STRING, "byte-stream", NULL);
  gst_structure_set (structure, "alignment", G_TYPE_STRING, "au", NULL);

  ret = GST_MPP_VIDEO_ENC_CLASS (parent_class)->handle_frame (encoder, frame,
      outcaps);
  gst_caps_unref (outcaps);
  return ret;
}

static void
gst_mpp_h264_enc_init (GstMppH264Enc * encoder)
{
  encoder->mode = ARG_MODE_DEFAULT;
  encoder->intra = ARG_INTRA_DEFAULT;

  encoder->bitrate = ARG_BITRATE_DEFAULT;
  encoder->quality = ARG_QUALITY_DEFAULT;
  encoder->profile = ARG_PROFILE_DEFAULT;

  encoder->qp_min = ARG_QP_MIN_DEFAULT;
  encoder->qp_max = ARG_QP_MAX_DEFAULT;
  encoder->qp_step = ARG_QP_STEP_DEFAULT;
}

static void
gst_mpp_h264_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMppH264Enc *encoder;
  encoder = GST_MPP_H264_ENC (object);

  GST_OBJECT_LOCK (encoder);
  switch (prop_id) {
    case ARG_MODE:
      encoder->mode = g_value_get_enum (value);
      break;
    case ARG_BITRATE:
      encoder->bitrate = g_value_get_ulong (value);
      break;
    case ARG_QUALITY:
      encoder->quality = g_value_get_enum (value);
      break;
    case ARG_PROFILE:
      encoder->profile = g_value_get_enum (value);
      break;
    case ARG_INTRA:
      encoder->intra = g_value_get_uint (value);
      break;
    case ARG_QP_MIN:
      encoder->qp_min = g_value_get_uint (value);
      break;
    case ARG_QP_MAX:
      encoder->qp_max = g_value_get_uint (value);
      break;
    case ARG_QP_STEP:
      encoder->qp_step = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (encoder);
  return;
}

static void
gst_mpp_h264_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMppH264Enc *encoder;

  encoder = GST_MPP_H264_ENC (object);

  GST_OBJECT_LOCK (encoder);
  switch (prop_id) {
    case ARG_MODE:
      g_value_set_enum (value, encoder->mode);
      break;
    case ARG_BITRATE:
      g_value_set_ulong (value, encoder->bitrate);
      break;
    case ARG_QUALITY:
      g_value_set_enum (value, encoder->quality);
      break;
    case ARG_PROFILE:
      g_value_set_enum (value, encoder->profile);
      break;
    case ARG_INTRA:
      g_value_set_uint (value, encoder->intra);
      break;
    case ARG_QP_MIN:
      g_value_set_uint (value, encoder->qp_min);
      break;
    case ARG_QP_MAX:
      g_value_set_uint (value, encoder->qp_max);
      break;
    case ARG_QP_STEP:
      g_value_set_uint (value, encoder->qp_step);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (encoder);
}

static void
gst_mpp_h264_enc_class_init (GstMppH264EncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoEncoderClass *video_encoder_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);

  gobject_class->get_property = gst_mpp_h264_enc_get_property;
  gobject_class->set_property = gst_mpp_h264_enc_set_property;

  gst_element_class_set_static_metadata (element_class,
      "Rockchip Mpp H264 Encoder",
      "Codec/Encoder/Video",
      "Encode video streams via Rockchip Mpp",
      "Randy Li <randy.li@rock-chips.com>");

  video_encoder_class->open = GST_DEBUG_FUNCPTR (gst_mpp_h264_enc_open);
  video_encoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_mpp_h264_enc_set_format);
  video_encoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_mpp_h264_enc_handle_frame);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_mpp_h264_enc_src_template));

  g_object_class_install_property (gobject_class, ARG_MODE,
      g_param_spec_enum ("mode", "Mode",
          "Variable bitrate or custom bitrate", GST_MPP_H264_ENC_MODE_TYPE,
          ARG_MODE_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_INTRA,
      g_param_spec_uint ("intra", "Intra frames",
          "Gap between intra frames in seconds (0: every frame is an intra frame)",
          0, 1024, ARG_INTRA_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_BITRATE,
      g_param_spec_ulong ("bitrate", "Bitrate",
          "Target Bitrate in bit/s - 0: automatically calculate",
          0, (100 * 1000 * 1024), ARG_BITRATE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_QUALITY,
      g_param_spec_enum ("quality", "Quality",
          "[vbr] Quality", GST_MPP_H264_ENC_QUALITY_TYPE,
          ARG_QUALITY_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_PROFILE,
      g_param_spec_enum ("profile", "Profile",
          "Profile", GST_MPP_H264_ENC_PROFILE_TYPE,
          ARG_PROFILE_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_QP_MIN,
      g_param_spec_uint ("qp-min", "Minimum Quantizer",
          "Minimum quantizer (lower value => higher bitrate)",
          0, 48, ARG_QP_MIN_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_QP_MAX,
      g_param_spec_uint ("qp-max", "Maximum Quantizer",
          "Maximum quantizer (lower value => higher bitrate)",
          8, 51, ARG_QP_MAX_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_QP_STEP,
      g_param_spec_uint ("qp-step", "Maximum Quantizer Difference",
          "Maximum quantizer difference between frames",
          0, 50, ARG_QP_STEP_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
