/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <II420ColorConverter.h>
#include <OMX_IVCommon.h>
#include <string.h>

/*
 * getDecoderOutputFormat
 * Returns the color format (OMX_COLOR_FORMATTYPE) of the decoder output.
 * If it is I420 (OMX_COLOR_FormatYUV420Planar), no conversion is needed,
 * and convertDecoderOutputToI420() can be a no-op.
 */
static int getDecoderOutputFormat() {
    return OMX_COLOR_FormatYUV420Planar;
}

/*
 * convertDecoderOutputToI420
 * @Desc     Converts from the decoder output format to I420 format.
 * @note     Caller (e.g. VideoEditor) owns the buffers
 * @param    decoderBits   (IN) Pointer to the buffer contains decoder output
 * @param    decoderWidth  (IN) Buffer width, as reported by the decoder
 *                              metadata (kKeyWidth)
 * @param    decoderHeight (IN) Buffer height, as reported by the decoder
 *                              metadata (kKeyHeight)
 * @param    decoderRect   (IN) The rectangle of the actual frame, as
 *                              reported by decoder metadata (kKeyCropRect)
 * @param    dstBits      (OUT) Pointer to the output I420 buffer
 * @return   -1 Any error
 * @return   0  No Error
 */
static int convertDecoderOutputToI420(
    void* srcBits, int srcWidth, int srcHeight, ARect srcRect, void* dstBits) {
    return 0;
}

/*
 * getEncoderIntputFormat
 * Returns the color format (OMX_COLOR_FORMATTYPE) of the encoder input.
 * If it is I420 (OMX_COLOR_FormatYUV420Planar), no conversion is needed,
 * and convertI420ToEncoderInput() and getEncoderInputBufferInfo() can
 * be no-ops.
 */
static int getEncoderInputFormat() {
    return OMX_COLOR_FormatYUV420Planar;
}

/* convertI420ToEncoderInput
 * @Desc     This function converts from I420 to the encoder input format
 * @note     Caller (e.g. VideoEditor) owns the buffers
 * @param    srcBits       (IN) Pointer to the input I420 buffer
 * @param    srcWidth      (IN) Width of the I420 frame
 * @param    srcHeight     (IN) Height of the I420 frame
 * @param    encoderWidth  (IN) Encoder buffer width, as calculated by
 *                              getEncoderBufferInfo()
 * @param    encoderHeight (IN) Encoder buffer height, as calculated by
 *                              getEncoderBufferInfo()
 * @param    encoderRect   (IN) Rect coordinates of the actual frame inside
 *                              the encoder buffer, as calculated by
 *                              getEncoderBufferInfo().
 * @param    encoderBits  (OUT) Pointer to the output buffer. The size of
 *                              this buffer is calculated by
 *                              getEncoderBufferInfo()
 * @return   -1 Any error
 * @return   0  No Error
 */
static int convertI420ToEncoderInput(
    void* srcBits, int srcWidth, int srcHeight,
    int dstWidth, int dstHeight, ARect dstRect,
    void* dstBits) {
    return 0;
}

/* getEncoderInputBufferInfo
 * @Desc     This function returns metadata for the encoder input buffer
 *           based on the actual I420 frame width and height.
 * @note     This API should be be used to obtain the necessary information
 *           before calling convertI420ToEncoderInput().
 *           VideoEditor knows only the width and height of the I420 buffer,
 *           but it also needs know the width, height, and size of the
 *           encoder input buffer. The encoder input buffer width and height
 *           are used to set the metadata for the encoder.
 * @param    srcWidth      (IN) Width of the I420 frame
 * @param    srcHeight     (IN) Height of the I420 frame
 * @param    encoderWidth  (OUT) Encoder buffer width needed
 * @param    encoderHeight (OUT) Encoder buffer height needed
 * @param    encoderRect   (OUT) Rect coordinates of the actual frame inside
 *                               the encoder buffer
 * @param    encoderBufferSize  (OUT) The size of the buffer that need to be
 *                              allocated by the caller before invoking
 *                              convertI420ToEncoderInput().
 * @return   -1 Any error
 * @return   0  No Error
 */
static int getEncoderInputBufferInfo(
    int actualWidth, int actualHeight,
    int* encoderWidth, int* encoderHeight,
    ARect* encoderRect, int* encoderBufferSize) {

    *encoderWidth = actualWidth;
    *encoderHeight = actualHeight;
    encoderRect->left = 0;
    encoderRect->top = 0;
    encoderRect->right = actualWidth - 1;
    encoderRect->bottom = actualHeight - 1;
    *encoderBufferSize = (actualWidth * actualHeight * 3 / 2);

    return 0;
}

extern "C" void getI420ColorConverter(II420ColorConverter *converter) {
    converter->getDecoderOutputFormat = getDecoderOutputFormat;
    converter->convertDecoderOutputToI420 = convertDecoderOutputToI420;
    converter->getEncoderInputFormat = getEncoderInputFormat;
    converter->convertI420ToEncoderInput = convertI420ToEncoderInput;
    converter->getEncoderInputBufferInfo = getEncoderInputBufferInfo;
}
