/* /////////////////////////////////////////////////////////
 * includes
 */
#include "libflv.h"

/* /////////////////////////////////////////////////////////
 * types
 */

typedef struct __tb_flv_info_t
{
	tb_size_t audio_dts_last;
	tb_size_t video_dts_last;

}tb_flv_info_t;

/* /////////////////////////////////////////////////////////
 * callback
 */
static tb_void_t tb_flv_sdata_cb_func(tb_char_t const* spath, tb_flv_sdata_value_t const* value, tb_pointer_t cb_data)
{
	switch (value->type)
	{
	case TB_FLV_SDATA_TYPE_NUMBER:
		tb_trace_i("[demo]: %s = %lf", spath, value->u.number);
		break;
	case TB_FLV_SDATA_TYPE_BOOLEAN:
		tb_trace_i("[demo]: %s = %s", spath, value->u.boolean? "true" : "false");
		break;
	case TB_FLV_SDATA_TYPE_STRING:
	case TB_FLV_SDATA_TYPE_LONGSTRING:
		tb_trace_i("[demo]: %s = %s", spath, value->u.string.data);
		break;
	case TB_FLV_SDATA_TYPE_MOVIECLIP:
		tb_trace_i("[demo]: %s = movieclip", spath);
		break;
	case TB_FLV_SDATA_TYPE_NONE:
		tb_trace_i("[demo]: %s = null", spath);
		break;
	case TB_FLV_SDATA_TYPE_UNDEFINED:
		tb_trace_i("[demo]: %s = undefined", spath);
		break;
	case TB_FLV_SDATA_TYPE_REFERENCE:
		tb_trace_i("[demo]: %s = reference", spath);
		break;
	case TB_FLV_SDATA_TYPE_DATE:	
		tb_trace_i("[demo]: %s = date", spath);
		break;
	case TB_FLV_SDATA_TYPE_OBJECT:
		tb_trace_i("[demo]: %s = object", spath);
		break;
	case TB_FLV_SDATA_TYPE_ECMAARRAY:
		tb_trace_i("[demo]: %s = ecmaarray", spath);
		break;
	case TB_FLV_SDATA_TYPE_STRICTARRAY:
		tb_trace_i("[demo]: %s = strictarray", spath);
		break;
	default:
		break;
	}
}

static tb_void_t tb_flv_sdata_data_cb_func(tb_byte_t const* head_data, tb_size_t head_size, tb_byte_t const* body_data, tb_size_t body_size, tb_pointer_t cb_data)
{
	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: sdata_data: %d %d", head_size, body_size);
}
static tb_void_t tb_flv_hdata_cb_func(tb_byte_t const* data, tb_size_t size, tb_pointer_t cb_data)
{
	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: head_size: %d", size);
}
static tb_void_t tb_flv_audio_config_cb_func(tb_byte_t const* head_data, tb_size_t head_size, tb_byte_t const* body_data, tb_size_t body_size, tb_pointer_t cb_data)
{
	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: audio_config_size: %d %d", head_size, body_size);

	tb_uint8_t flags = head_data[11];
	static tb_uint32_t samplerate_table[] =
	{
		96000, 88200, 64000, 48000, 44100, 32000
	,	24000, 22050, 16000, 12000, 11025, 8000, 7350
	};

	static tb_uint8_t channels_table[8] = 
	{
		0, 1, 2, 3, 4, 5, 6, 8
	};

	// attach data
	tb_bstream_t bst;
	tb_bstream_init(&bst, body_data, body_size);
	
	// get object type
	tb_byte_t object_type = tb_bstream_get_ubits32(&bst, 5);
	if (object_type == 31) object_type = 32 + tb_bstream_get_ubits32(&bst, 6);

	// get samplerate 
	tb_size_t samplerate = 0;
	tb_byte_t rate_index = tb_bstream_get_ubits32(&bst, 4);
	if (rate_index == 0xf) samplerate = tb_bstream_get_ubits32(&bst, 24);
	else if (rate_index < 13) samplerate = samplerate_table[rate_index];
	else samplerate = (44100 << ((flags & TB_FLV_AUDIO_SAMPLERATE_MASK) >> TB_FLV_AUDIO_SAMPLERATE_OFFSET) >> 3);

	// get channels
	tb_byte_t channels_idex = tb_bstream_get_ubits32(&bst, 4);
	tb_size_t channels = channels_idex < 8? channels_table[channels_idex] : 2;
	if (!channels) channels = (flags & TB_FLV_AUDIO_CHANNEL_MASK) == TB_FLV_AUDIO_CHANNEL_STEREO ? 2 : 1;
	tb_trace_i("[demo]: samplerate: %u Hz, channels: %u, object_type: %u", samplerate, channels, object_type);
}
static tb_void_t tb_flv_video_config_cb_func(tb_byte_t const* head_data, tb_size_t head_size, tb_byte_t const* body_data, tb_size_t body_size, tb_pointer_t cb_data)
{
	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: video_config_size: %d %d", head_size, body_size);
	tb_bstream_t 	bst;
	tb_bstream_init(&bst, body_data, body_size);

	tb_uint8_t configure_version 		= tb_bstream_get_u8(&bst);
	tb_uint8_t avc_profile_indication 	= tb_bstream_get_u8(&bst);
	tb_uint8_t profile_compatibility 	= tb_bstream_get_u8(&bst);
	tb_uint8_t avc_level_indication 	= tb_bstream_get_u8(&bst);
	tb_trace_i("[demo]: version: %x profile: %x compatibility: %x, level: %x", configure_version, avc_profile_indication, profile_compatibility, avc_level_indication);

	tb_uint8_t length_size_minusone 	= tb_bstream_get_u8(&bst) & 0x03;
	tb_trace_i("[demo]: length_size_minusone: %u", length_size_minusone);

	tb_uint8_t sps_n = tb_bstream_get_u8(&bst) & 0x1f;
	tb_trace_i("[demo]: sps_n: %u", sps_n);

	tb_size_t i = 0;
	for (i = 0; i < sps_n; ++i)
	{
		tb_uint16_t size = tb_bstream_get_u16_be(&bst);
		tb_trace_i("[demo]: sps_size: %u", size);

		// analyze framerate
		if (size)
		{
			// get sps data
			tb_byte_t* data = tb_malloc0(size);
			tb_assert_return(data);
			tb_bstream_get_data(&bst, data, size);

			// remove emulation bytes
			size = tb_flv_video_h264_sps_analyze_remove_emulation(data, size);

			// analyze framerate
			tb_double_t framerate = tb_flv_video_h264_sps_analyze_framerate(data, size);
			tb_trace_i("[demo]: sps_framerate: %lf", framerate);

			// free data
			tb_free(data);
		}
	}

	tb_uint8_t pps_n = tb_bstream_get_u8(&bst) & 0x1f;
	tb_trace_i("[demo]: pps_n: %u", pps_n);

	for (i = 0; i < pps_n; ++i)
	{
		tb_uint16_t size = tb_bstream_get_u16_be(&bst);
		tb_trace_i("[demo]: pps_size: %u", size);
		tb_bstream_skip(&bst, size);
	}
}
static tb_bool_t tb_flv_audio_data_cb_func(tb_byte_t const* head_data, tb_size_t head_size, tb_byte_t const* body_data, tb_size_t body_size, tb_size_t dts, tb_pointer_t cb_data)
{	
	tb_flv_info_t* info = cb_data;
	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: audio_data_size: %u %u, dts: %u, dt: %u", head_size, body_size, dts, dts - info->audio_dts_last);
	info->audio_dts_last = dts;

	return tb_true;
}
static tb_bool_t tb_flv_video_data_cb_func(tb_byte_t const* head_data, tb_size_t head_size, tb_byte_t const* body_data, tb_size_t body_size, tb_size_t dts, tb_sint32_t cts, tb_pointer_t cb_data)
{
	tb_flv_info_t* info = cb_data;
	tb_uint8_t 	flags = head_data[11];
	tb_bool_t 	bkeyframe = (flags & TB_FLV_VIDEO_FRAMETYPE_MASK) == TB_FLV_FRAME_TYPE_KEY? tb_true : tb_false;

	tb_trace_i("[demo]: =================================================================================");
	tb_trace_i("[demo]: video_data_size: %u %u, dts: %u, cts: %d, pts: %u, dt: %u, %s", head_size, body_size, dts, cts, dts + (cts > 0? cts : 0), dts - info->video_dts_last, bkeyframe? "iframe" : "xframe");
	info->video_dts_last = dts;

	// set unit data
	tb_bstream_t bst;
	tb_bstream_init(&bst, body_data, body_size);
	tb_size_t unit_size = tb_bstream_get_u32_be(&bst);
	tb_size_t read_size = 4;
	while (read_size + unit_size <= body_size)
	{
		// read unit data
		if (unit_size)
		{
			// skip sei unit
			tb_byte_t unit_type = tb_bstream_get_u8(&bst) & 0x1f;
			tb_trace_i("[demo]: unit_type: %u, unit_size: %u", unit_type, unit_size);

			// skip unit
			tb_bstream_skip(&bst, unit_size - 1);

			// update read size
			read_size += unit_size;
		}

		// is end?
		if (read_size + 4 > body_size) break;

		// read unit_size 
		unit_size = tb_bstream_get_u32_be(&bst);
		read_size += 4;
	}


	return tb_true;
}


/* /////////////////////////////////////////////////////////
 * main
 */
tb_int_t tb_demo_flv_main(tb_int_t argc, tb_char_t** argv)
{
	// init info
	tb_flv_info_t info = {0};

	// create stream
	tb_gstream_t* gst = tb_gstream_init_from_url(argv[1]);
	tb_assert_and_check_goto(gst, end);
	
	// open stream
	if (!tb_gstream_open(gst)) goto end;
	
	// init decoder
	tb_handle_t hflv = tb_flv_init(gst);
	tb_assert_and_check_goto(hflv, end);

	// ioctl
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_SDATA, 			tb_flv_sdata_cb_func, 			&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_HDATA, 			tb_flv_hdata_cb_func, 			&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_SDATA_DATA, 		tb_flv_sdata_data_cb_func, 		&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_AUDIO_CONFIG, 	tb_flv_audio_config_cb_func, 	&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_VIDEO_CONFIG, 	tb_flv_video_config_cb_func, 	&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_AUDIO_DATA, 		tb_flv_audio_data_cb_func, 		&info);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_CB_VIDEO_DATA, 		tb_flv_video_data_cb_func, 		&info);

	//tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_SPANK_TYPE, TB_FLV_SPANK_TYPE_SDATA);
	tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_SPANK_TYPE, TB_FLV_SPANK_TYPE_HDATA | TB_FLV_SPANK_TYPE_SDATA | TB_FLV_SPANK_TYPE_VIDEO_CONFIG | TB_FLV_SPANK_TYPE_VIDEO_DATA);
	//tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_SPANK_TYPE, TB_FLV_SPANK_TYPE_HDATA | TB_FLV_SPANK_TYPE_SDATA | TB_FLV_SPANK_TYPE_AUDIO_CONFIG | TB_FLV_SPANK_TYPE_AUDIO_DATA);
	//tb_flv_ioctl(hflv, TB_FLV_IOCTL_CMD_SPANK_TYPE, TB_FLV_SPANK_TYPE_HDATA | TB_FLV_SPANK_TYPE_SDATA | TB_FLV_SPANK_TYPE_VIDEO_CONFIG | TB_FLV_SPANK_TYPE_AUDIO_CONFIG | TB_FLV_SPANK_TYPE_VIDEO_DATA | TB_FLV_SPANK_TYPE_AUDIO_DATA);

	// spank
	while (tb_flv_spank(hflv)) ;

	// free decoder
	tb_flv_exit(hflv);

end:
	// free gstream
	if (gst) tb_gstream_exit(gst);
	return 0;
}
