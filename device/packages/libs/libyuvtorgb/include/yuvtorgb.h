#ifndef YUVTORGB_H
#define YUVTORGB_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int yuvAddr;//phyaddr
	int outAddr;//phyaddr, here do not flush, do this yourself.
	int inwidth;//align 16
	int inheight;//align 16
	int inColor;
	int outColor;
	int outwidth;//align 8
	int outheight;//align 8
	int rotation;
}YUV2RGBParams;

#define PP_IN_YUV420sp 1 //the same in Postprocess.h
#define PP_IN_YUV420_P 2

#define PP_OUT_RGB565 0
#define PP_OUT_ARGB 1

#define PP_ROTATION_NONE                                0U
#define PP_ROTATION_RIGHT_90                            1U
#define PP_ROTATION_LEFT_90                             2U
#define PP_ROTATION_HOR_FLIP                            3U
#define PP_ROTATION_VER_FLIP                            4U
#define PP_ROTATION_180                                 5U


int doYuvToRgb(YUV2RGBParams *param);

#ifdef __cplusplus
}
#endif
#endif
