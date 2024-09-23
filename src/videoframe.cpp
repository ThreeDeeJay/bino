/*
 * This file is part of Bino, a 3D video player.
 *
 * Copyright (C) 2022, 2023, 2024
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "videoframe.hpp"
#include "log.hpp"


VideoFrame::VideoFrame()
{
    update(Input_Unknown, Surround_Unknown, QVideoFrame(), false);
}

bool VideoFrame::isValid() const
{
    return (qframe.isValid() && qframe.pixelFormat() != QVideoFrameFormat::Format_Invalid);
}

void VideoFrame::update(InputMode im, SurroundMode sm, const QVideoFrame& frame, bool newSrc)
{
    if (qframe.isMapped())
        qframe.unmap();
    qframe = frame;

    if (isValid()) {
        subtitle = qframe.subtitleText();
        width = qframe.width();
        height = qframe.height();
        aspectRatio = float(width) / height;
        LOG_FIREHOSE("videoframe receives new %dx%d frame with pixel format %s", width, height,
                qPrintable(QVideoFrameFormat::pixelFormatToString(qframe.pixelFormat())));
        if (im == Input_Unknown) {
            if (aspectRatio >= 3.0f)
                im = Input_Left_Right;
            else if (aspectRatio < 1.0f)
                im = Input_Top_Bottom;
            else
                im = Input_Mono;
            LOG_FIREHOSE("videoframe guesses input mode from aspect ratio %g: %s", aspectRatio, inputModeToString(im));
        }
        inputMode = im;
        if (sm == Surround_Unknown) {
            if (width == height && (inputMode == Input_Top_Bottom || inputMode == Input_Bottom_Top))
                sm = Surround_360;
            else if (width == 2 * height && inputMode == Input_Mono)
                sm = Surround_360;
            else if (width == 2 * height && (inputMode == Input_Top_Bottom_Half || inputMode == Input_Bottom_Top_Half))
                sm = Surround_360;
            else if (width == 2 * height && (inputMode == Input_Left_Right_Half || inputMode == Input_Right_Left_Half))
                sm = Surround_360;
            else if (width == 4 * height && (inputMode == Input_Left_Right || inputMode == Input_Right_Left))
                sm = Surround_360;
            else if (2 * width == height && (inputMode == Input_Top_Bottom || inputMode == Input_Bottom_Top))
                sm = Surround_180;
            else if (width == height && inputMode == Input_Mono)
                sm = Surround_180;
            else if (width == height && (inputMode == Input_Top_Bottom_Half || inputMode == Input_Bottom_Top_Half))
                sm = Surround_180;
            else if (width == height && (inputMode == Input_Left_Right_Half || inputMode == Input_Right_Left_Half))
                sm = Surround_180;
            else if (width == 2 * height && (inputMode == Input_Left_Right || inputMode == Input_Right_Left))
                sm = Surround_180;
            else
                sm = Surround_Off;
            LOG_FIREHOSE("videoframe guesses surround mode %s from frame size", surroundModeToString(sm));
        }
        surroundMode = sm;
        bool fallbackToImage = true;
        if (       qframe.pixelFormat() == QVideoFrameFormat::Format_ARGB8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_ARGB8888_Premultiplied
                || qframe.pixelFormat() == QVideoFrameFormat::Format_XRGB8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_BGRA8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_BGRA8888_Premultiplied
                || qframe.pixelFormat() == QVideoFrameFormat::Format_BGRX8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_ABGR8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_XBGR8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_RGBA8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_RGBX8888
                || qframe.pixelFormat() == QVideoFrameFormat::Format_YUV420P
                || qframe.pixelFormat() == QVideoFrameFormat::Format_YUV422P
                || qframe.pixelFormat() == QVideoFrameFormat::Format_YV12
                || qframe.pixelFormat() == QVideoFrameFormat::Format_NV12
                || qframe.pixelFormat() == QVideoFrameFormat::Format_P010
                || qframe.pixelFormat() == QVideoFrameFormat::Format_P016
                || qframe.pixelFormat() == QVideoFrameFormat::Format_Y8
                || qframe.pixelFormat() == QVideoFrameFormat::Format_Y16) {
            fallbackToImage = false;
        }
        if (fallbackToImage) {
            if (newSrc) {
                LOG_WARNING("%s", qPrintable(tr("Pixel format %1 is not hardware accelerated!")
                            .arg(QVideoFrameFormat::pixelFormatToString(qframe.pixelFormat()))));
            }
            storage = Storage_Image;
            pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format_RGB32);
            yuvValueRangeSmall = false;
            yuvSpace = YUV_AdobeRgb;
            image = qframe.toImage();
            image.convertTo(QImage::Format_RGB32);
        } else {
            storage = Storage_Mapped;
            pixelFormat = qframe.pixelFormat();
            // TODO: update this for Qt6.4 (and require Qt6.4!) once
            // that version is in Debian unstable
            switch (qframe.surfaceFormat().yCbCrColorSpace()) {
            case QVideoFrameFormat::YCbCr_Undefined:
            case QVideoFrameFormat::YCbCr_BT601:
                yuvValueRangeSmall = true;
                yuvSpace = YUV_BT601;
                break;
            case QVideoFrameFormat::YCbCr_BT709:
                yuvValueRangeSmall = true;
                yuvSpace = YUV_BT709;
                break;
            case QVideoFrameFormat::YCbCr_xvYCC601:
                yuvValueRangeSmall = false;
                yuvSpace = YUV_BT601;
                break;
            case QVideoFrameFormat::YCbCr_xvYCC709:
                yuvValueRangeSmall = false;
                yuvSpace = YUV_BT709;
                break;
            case QVideoFrameFormat::YCbCr_JPEG:
                yuvValueRangeSmall = false;
                yuvSpace = YUV_AdobeRgb;
                break;
            case QVideoFrameFormat::YCbCr_BT2020:
                yuvValueRangeSmall = true;
                yuvSpace = YUV_AdobeRgb;
                break;
            }
            qframe.map(QVideoFrame::ReadOnly);
            planeCount = qframe.planeCount();
            for (int p = 0; p < planeCount; p++) {
                bytesPerLine[p] = qframe.bytesPerLine(p);
                bytesPerPlane[p] = qframe.mappedBytes(p);
                mappedBits[p] = qframe.bits(p);
            }
        }
        subtitle = qframe.subtitleText();
        subtitle.replace(QLatin1Char('\n'), QChar::LineSeparator); // qvideoframe.cpp does this
    } else {
        // Synthesize a black frame
        inputMode = Input_Mono;
        surroundMode = Surround_Off;
        width = 1;
        height = 1;
        storage = Storage_Image;
        image = QImage(width, height, QImage::Format_RGB32);
        image.fill(0);
        aspectRatio = 1.0f;
        subtitle = QString();
    }
}

void VideoFrame::reUpdate()
{
    update(inputMode, surroundMode, qframe, false);
}

void VideoFrame::invalidate()
{
    if (isValid())
        update(Input_Unknown, Surround_Unknown, QVideoFrame(), false);
}

QDataStream &operator<<(QDataStream& ds, const VideoFrame& f)
{
    ds << static_cast<int>(f.inputMode);
    ds << static_cast<int>(f.surroundMode);
    ds << f.subtitle;
    ds << f.width;
    ds << f.height;
    ds << f.aspectRatio;
    switch (f.storage) {
    case VideoFrame::Storage_Mapped:
    case VideoFrame::Storage_Copied:
        ds << static_cast<int>(VideoFrame::Storage_Copied);
        ds << static_cast<int>(f.pixelFormat);
        ds << f.yuvValueRangeSmall;
        ds << static_cast<int>(f.yuvSpace);
        ds << f.planeCount;
        for (int p = 0; p < f.planeCount; p++) {
            ds << f.bytesPerLine[p];
            ds << f.bytesPerPlane[p];
            if (f.storage == VideoFrame::Storage_Mapped)
                ds.writeRawData(reinterpret_cast<const char*>(f.mappedBits[p]), f.bytesPerPlane[p]);
            else
                ds.writeRawData(reinterpret_cast<const char*>(f.bits[p].data()), f.bytesPerPlane[p]);
        }
        break;
    case VideoFrame::Storage_Image:
        ds.writeRawData(reinterpret_cast<const char*>(f.image.bits()), f.image.sizeInBytes());
        break;
    }
    return ds;
}

QDataStream &operator>>(QDataStream& ds, VideoFrame& f)
{
    int tmp;

    ds >> tmp;
    f.inputMode = static_cast<InputMode>(tmp);
    ds >> tmp;
    f.surroundMode = static_cast<SurroundMode>(tmp);
    ds >> f.subtitle;
    ds >> f.width;
    ds >> f.height;
    ds >> f.aspectRatio;
    ds >> tmp;
    f.storage = static_cast<enum VideoFrame::Storage>(tmp);
    switch (f.storage) {
    case VideoFrame::Storage_Mapped: // cannot happen, see above
    case VideoFrame::Storage_Copied:
        f.image = QImage();
        ds >> tmp;
        f.pixelFormat = static_cast<QVideoFrameFormat::PixelFormat>(tmp);
        ds >> f.yuvValueRangeSmall;
        ds >> tmp;
        f.yuvSpace = static_cast<enum VideoFrame::YUVSpace>(tmp);
        ds >> f.planeCount;
        for (int p = 0; p < 3; p++) {
            if (p < f.planeCount) {
                ds >> f.bytesPerLine[p];
                ds >> f.bytesPerPlane[p];
                f.bits[p].resize(f.bytesPerPlane[p]);
                ds.readRawData(reinterpret_cast<char*>(f.bits[p].data()), f.bytesPerPlane[p]);
            } else {
                f.bytesPerLine[p] = 0;
                f.bytesPerPlane[p] = 0;
                f.bits[p].clear();
            }
            f.mappedBits[p] = nullptr;
        }
        break;
    case VideoFrame::Storage_Image:
        f.pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format_RGB32);
        f.yuvValueRangeSmall = false;
        f.yuvSpace = VideoFrame::YUV_AdobeRgb;
        f.planeCount = 0;
        for (int p = 0; p < 3; p++) {
            f.bytesPerLine[p] = 0;
            f.bytesPerPlane[p] = 0;
            f.mappedBits[p] = nullptr;
            f.bits[p].clear();
        }
        f.image = QImage(f.width, f.height, QImage::Format_RGB32);
        ds.readRawData(reinterpret_cast<char*>(f.image.bits()), f.image.sizeInBytes());
        break;
    }
    return ds;
}
