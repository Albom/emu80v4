﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <string.h>

#include "EmuWindow.h"
#include "Emulation.h"
#include "EmuConfig.h"
#include "Platform.h"

using namespace std;


EmuWindow::EmuWindow()
{
    m_windowType = EWT_EMULATION;

    m_curWindowWidth = m_defWindowWidth;
    m_curWindowHeight = m_defWindowHeight;

    m_params.style = PWS_FIXED;
    m_params.visible = false;
    m_params.width = m_defWindowWidth;
    m_params.height = m_defWindowHeight;
    m_params.title = "";
    m_params.vsync = g_emulation->getVsync();
    m_params.antialiasing = false;

    applyParams();
}

EmuWindow::~EmuWindow()
{
    if (m_interlacedImage)
        delete m_interlacedImage;
}


void EmuWindow::init()
{
    initPalWindow();
}


string EmuWindow::getPlatformObjectName()
{
    if (m_platform)
        return m_platform->getName();
    else
        return "";
}


void EmuWindow::setDefaultWindowSize(int width, int height)
{
    m_defWindowWidth = width;
    m_defWindowHeight = height;
    if (m_frameScale == FS_BEST_FIT || m_frameScale == FS_FIT || m_frameScale == FS_FIT_KEEP_AR) {
        m_params.width = m_defWindowWidth;
        m_params.height = m_defWindowHeight;
        applyParams();

        m_curWindowWidth = m_defWindowWidth;
        m_curWindowHeight = m_defWindowHeight;
    }
}


void EmuWindow::setFrameScale(FrameScale fs)
{
    m_frameScale = fs;
}



void EmuWindow::setCaption(string caption)
{
    m_caption = caption;
    string ver = VERSION;
    m_params.title = "Emu80 " + ver + ": " + caption;
    applyParams();
}

string EmuWindow::getCaption()
{
    return m_caption;
}


void EmuWindow::setWindowStyle(WindowStyle ws)
{
    m_isFullscreenMode = false;
    if (ws == WS_FIXED || ws == WS_AUTOSIZE)
        m_params.style = PWS_FIXED;
    else // if (ws == WS_SIZABLE)
        m_params.style = PWS_SIZABLE;

    if (ws == WS_FIXED) {
        m_params.width = m_defWindowWidth;
        m_params.height = m_defWindowHeight;
    }

    applyParams();

    m_windowStyle = ws;
}


void EmuWindow::setFullScreen(bool fullscreen)
{
    if (fullscreen) {
        m_params.style = PWS_FULLSCREEN;
    } else
        setWindowStyle(m_windowStyle);
    applyParams();

    m_isFullscreenMode = fullscreen;
}


// Переписать - запоминать предыдущий режим!
void EmuWindow::toggleFullScreen()
{
    setFullScreen(!m_isFullscreenMode);
}


void EmuWindow::setFieldsMixing(FieldsMixing fm)
{
    m_fieldsMixing = fm;
}


void EmuWindow::setAntialiasing(bool aal)
{
    m_isAntialiased = aal;
    m_params.antialiasing = aal;
    applyParams();
}


void EmuWindow::setAspectCorrection(bool aspectCorrection)
{
    m_aspectCorrection = aspectCorrection;
}


void EmuWindow::setWideScreen(bool wideScreen)
{
    m_wideScreen = wideScreen;
}


void EmuWindow::show()
{
    m_params.visible = true;
    applyParams();
}


void EmuWindow::hide()
{
    m_params.visible = false;
    applyParams();
}


void EmuWindow::calcDstRect(EmuPixelData frame)
{
    m_curImgWidth = frame.width;
    m_curImgHeight = frame.height;

    double aspectRatio = 1.0;
    if (m_aspectCorrection)
        aspectRatio = m_wideScreen ? frame.aspectRatio * 4.0 / 3.0 : frame.aspectRatio;

    FrameScale tempFs = m_frameScale;

    if (m_isFullscreenMode)
        tempFs = m_isAntialiased ? FS_FIT_KEEP_AR : FS_BEST_FIT;

    getSize(m_curWindowWidth, m_curWindowHeight);

    switch(tempFs) {
        case FS_1X:
            m_dstWidth = frame.width * aspectRatio + .5;
            m_dstHeight = frame.height;
            m_dstX = (m_curWindowWidth - m_dstWidth) / 2;
            m_dstY = (m_curWindowHeight - m_dstHeight) /2;
            break;
        case FS_2X:
            m_dstWidth = frame.width * 2 * aspectRatio + .5;
            m_dstHeight = frame.height * 2;
            m_dstX = (m_curWindowWidth - m_dstWidth) / 2;
            m_dstY = (m_curWindowHeight - m_dstHeight) /2;
            break;
        case FS_3X:
            m_dstWidth = frame.width * 3 * aspectRatio + .5;
            m_dstHeight = frame.height * 3;
            m_dstX = (m_curWindowWidth - m_dstWidth) / 2;
            m_dstY = (m_curWindowHeight - m_dstHeight) /2;
            break;
        case FS_BEST_FIT: {
            int timesX = m_curWindowWidth / (frame.width * aspectRatio + .5);
            int timesY = m_curWindowHeight / frame.height;
            int times = timesX < timesY ? timesX : timesY;

            if (times == 0) {
                m_dstWidth = m_curWindowWidth;
                m_dstHeight = m_curWindowHeight;
                m_dstX = 0;
                m_dstY = 0;
            } else {
                int dx = int((m_curWindowWidth - frame.width * times * aspectRatio + .5)) / 2;
                int dy = (m_curWindowHeight - frame.height * times) / 2;
                m_dstX = dx;
                m_dstY = dy;
                m_dstWidth = frame.width * times * aspectRatio + .5;
                m_dstHeight = frame.height * times;
            }
            break;
        }
        case FS_FIT:
            m_dstWidth = m_curWindowWidth;
            m_dstHeight = m_curWindowHeight;
            m_dstX = 0;
            m_dstY = 0;
            break;
        case FS_FIT_KEEP_AR:
            int newW = m_curWindowHeight * frame.width * aspectRatio / frame.height + .5;
            int newH = m_curWindowWidth * frame.height / aspectRatio / frame.width + .5;
            if (newW <= m_curWindowWidth) {
                m_dstWidth = newW;
                m_dstHeight = m_curWindowHeight;
                m_dstX = (m_curWindowWidth - newW) / 2;
                m_dstY = 0;
            } else {
                m_dstWidth = m_curWindowWidth;
                m_dstHeight = newH;
                m_dstX = 0;
                m_dstY = (m_curWindowHeight - newH) / 2;
            }
            break;
    }

}


bool EmuWindow::translateCoords(int& x, int& y)
{
    if ((x < m_dstX) || (x >= m_dstX + m_dstWidth) || (y < m_dstY) || (y >= m_dstY + m_dstHeight))
        return false;

    x = (x - m_dstX) * m_curImgWidth / m_dstWidth;
    y = (y - m_dstY) * m_curImgHeight / m_dstHeight;

    return true;
}


void EmuWindow::interlaceFields(EmuPixelData frame)
{
    int requiredSize = frame.height * 2 * frame.width;
    if (requiredSize > m_interlacedImageSize) {
        if (m_interlacedImage)
            delete m_interlacedImage;
        m_interlacedImage = new uint32_t[requiredSize];
        m_interlacedImageSize = requiredSize;
    }

    for (int i = 0; i < frame.height; i++) {
        memcpy(m_interlacedImage + frame.width * i * 2, frame.pixelData + i * frame.width, frame.width * 4);
        memcpy(m_interlacedImage + frame.width * (i * 2 + 1), frame.prevPixelData + i * frame.width, frame.width * 4);
    }
}


void EmuWindow::prepareScanline(EmuPixelData frame)
{
    int requiredSize = frame.height * 2 * frame.width;
    if (requiredSize > m_interlacedImageSize) {
        if (m_interlacedImage)
            delete m_interlacedImage;
        m_interlacedImage = new uint32_t[requiredSize];
        m_interlacedImageSize = requiredSize;
    }

    for (int i = 0; i < frame.height; i++) {
        memcpy(m_interlacedImage + frame.width * i * 2, frame.pixelData + i * frame.width, frame.width * 4);
        memcpy(m_interlacedImage + frame.width * (i * 2 + 1), frame.pixelData + i * frame.width, frame.width * 4);
        uint8_t* secondLine = (uint8_t*)(m_interlacedImage + frame.width * (i * 2 + 1));
        for (int x = 0; x < frame.width * 4; x += 4) {
            int r = secondLine[x]; secondLine[x] = r >> 2;
            int g = secondLine[x + 1]; secondLine[x + 1] = g >> 2;
            int b = secondLine[x + 2]; secondLine[x + 2] = b >> 2;
        }
    }
}


void EmuWindow::drawFrame(EmuPixelData frame)
{
    if (frame.width == 0 || frame.height == 0 || !frame.pixelData) {
        drawFill(0x303050);
        return;
    }

    calcDstRect(frame);

    if ((m_windowStyle == WS_AUTOSIZE) && !m_isFullscreenMode && (m_frameScale == FS_1X || m_frameScale == FS_2X || m_frameScale == FS_3X)
          && (m_curWindowWidth != m_dstWidth || m_curWindowHeight != m_dstHeight)) {
        m_curWindowHeight = m_dstHeight;
        m_curWindowWidth = m_dstWidth;

        m_params.width = m_dstWidth;
        m_params.height = m_dstHeight;
        applyParams();
    }

    drawFill(0x282828);

    if (m_fieldsMixing != FM_INTERLACE && m_fieldsMixing != FM_SCANLINE) {
        drawImage((uint32_t*)frame.pixelData, frame.width, frame.height, m_dstX, m_dstY, m_dstWidth, m_dstHeight, false, false);
        if (m_fieldsMixing == FM_MIX && frame.prevPixelData && frame.prevHeight == frame.height && frame.prevWidth == frame.width) {
            drawImage((uint32_t*)frame.prevPixelData, frame.prevWidth, frame.prevHeight, m_dstX, m_dstY, m_dstWidth, m_dstHeight, true, false);
        }
    } else if (m_fieldsMixing == FM_INTERLACE && frame.prevPixelData && frame.prevHeight == frame.height && frame.prevWidth == frame.width) { // FM_INTERLACE
        if (frame.frameNo & 1)
            interlaceFields(frame);
        if (m_interlacedImage)
            drawImage(m_interlacedImage, frame.width, frame.height * 2, m_dstX, m_dstY, m_dstWidth, m_dstHeight, false, false);
    } else { // if (m_fieldsMixing == FM_SCANLINE)
        prepareScanline(frame);
        drawImage(m_interlacedImage, frame.width, frame.height * 2, m_dstX, m_dstY, m_dstWidth, m_dstHeight, false, false);
    }
}


void EmuWindow::drawOverlay(EmuPixelData frame)
{
    if (frame.width == 0 || frame.height == 0 || !frame.pixelData)
        return;

    drawImage((uint32_t*)frame.pixelData, frame.width, frame.height, m_dstX, m_dstY, m_dstWidth, m_dstHeight, true, true);

/*    if (m_fieldsMixing == FM_MIX && frame.prevPixelData) {
        drawImage((uint32_t*)frame.prevPixelData, frame.prevWidth, frame.prevHeight, m_dstX, m_dstY, m_dstWidth, m_dstHeight, true, true);
    }*/
}


void EmuWindow::endDraw()
{
    drawEnd();
}


void EmuWindow::sysReq(SysReq sr)
{
    switch (sr) {
        case SR_FULLSCREEN:
            toggleFullScreen();
            break;
        case SR_1X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_1X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_2X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_2X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_3X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_3X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_FIT:
            if (!m_isFullscreenMode)
                setWindowStyle(WS_SIZABLE);
            setFrameScale(FS_FIT_KEEP_AR);
            setAntialiasing(true);
            m_aspectCorrection = true;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_MAXIMIZE:
            setWindowStyle(WS_SIZABLE);
            setFrameScale(FS_FIT_KEEP_AR);
            setAntialiasing(true);
            maximize();
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_ASPECTCORRECTION:
            m_aspectCorrection = !m_aspectCorrection;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_WIDESCREEN:
            setWideScreen(!m_wideScreen);
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_ANTIALIASING:
            setAntialiasing(!m_isAntialiased);
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_SCREENSHOT:
            screenshotRequest(palOpenFileDialog("Save screenshot", "BMP files (*.bmp)|*.bmp", true, this));
            break;
        default:
            break;
    }
}


bool EmuWindow::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "caption") {
        setCaption(values[0].asString());
        return true;
    } else if (propertyName == "windowStyle") {
        if (values[0].asString() == "autosize") {
            setWindowStyle(WS_AUTOSIZE);
            return true;
        } else if (values[0].asString() == "fixed") {
            setWindowStyle(WS_FIXED);
            return true;
        } else if (values[0].asString() == "sizable") {
            setWindowStyle(WS_SIZABLE);
            return true;
        } else
            return false;
    } else if (propertyName == "frameScale") {
        if (values[0].asString() == "bestFit") {
            setFrameScale(FS_BEST_FIT);
            return true;
        } else if (values[0].asString() == "1x") {
            setFrameScale(FS_1X);
            return true;
        } else if (values[0].asString() == "2x") {
            setFrameScale(FS_2X);
            return true;
        } else if (values[0].asString() == "3x") {
            setFrameScale(FS_3X);
            return true;
        } else if (values[0].asString() == "fit") {
            setFrameScale(FS_FIT);
            return true;
        } else if (values[0].asString() == "fitKeepAR") {
            setFrameScale(FS_FIT_KEEP_AR);
            return true;
        }
    } else if (propertyName == "fieldsMixing") {
        if (values[0].asString() == "none") {
            setFieldsMixing(FM_NONE);
            return true;
        } else if (values[0].asString() == "mix") {
            setFieldsMixing(FM_MIX);
            return true;
        } else if (values[0].asString() == "interlace") {
            setFieldsMixing(FM_INTERLACE);
            return true;
        } else if (values[0].asString() == "scanline") {
            setFieldsMixing(FM_SCANLINE);
            return true;
        }
    } else if (propertyName == "defaultWindowSize") {
        if (values[0].isInt() && values[1].isInt()) {
            setDefaultWindowSize(values[0].asInt(), values[1].asInt());
            return true;
        }
    } else if (propertyName == "defaultWindowWidth") { // !!!
        if (values[0].isInt()) {
            setDefaultWindowSize(values[0].asInt(), m_defWindowHeight);
            return true;
        }
    } else if (propertyName == "defaultWindowHeight") { // !!!
        if (values[0].isInt()) {
            setDefaultWindowSize(m_defWindowWidth, values[0].asInt());
            return true;
        }
    } else if (propertyName == "antialiasing") {
        if (values[0].asString() == "yes") {
            setAntialiasing(true);
            return true;
        } else if (values[0].asString() == "no") {
            setAntialiasing(false);
            return true;
        }
    } else if (propertyName == "fullscreen") {
        if (values[0].asString() == "yes") {
            setFullScreen(true);
            return true;
        } else if (values[0].asString() == "no") {
            setFullScreen(false);
            return true;
        }
    } else if (propertyName == "aspectCorrection") {
        if (values[0].asString() == "no") {
            setAspectCorrection(false);
            return true;
        } else if (values[0].asString() == "yes") {
            setAspectCorrection(true);
            return true;
        }
    } else if (propertyName == "wideScreen") {
        if (values[0].asString() == "no") {
            setWideScreen(false);
            return true;
        } else if (values[0].asString() == "yes") {
            setWideScreen(true);
            return true;
        }
    }

    return false;
}


string EmuWindow::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "caption")
        return m_caption;
    else if (propertyName == "windowStyle") {
        switch (m_windowStyle) {
            case WS_AUTOSIZE:
                return "autosize";
            case WS_FIXED:
                return "fixed";
            case WS_SIZABLE:
                return "sizable";
        }
    } else if (propertyName == "frameScale") {
        switch (m_frameScale) {
            case FS_BEST_FIT:
                return "bestFit";
            case FS_1X:
                return "1x";
            case FS_2X:
                return "2x";
            case FS_3X:
                return "3x";
            case FS_FIT:
                return "fit";
            case FS_FIT_KEEP_AR:
                return "fitKeepAR";
        }

    } else if (propertyName == "fieldsMixing") {
        switch (m_fieldsMixing) {
            case FM_NONE:
                return "none";
            case FM_MIX:
                return "mix";
            case FM_INTERLACE:
                return "interlace";
            case FM_SCANLINE:
                return "scanline";
        }
    } else if (propertyName == "antialiasing") {
        return m_isAntialiased ? "yes" : "no";
    } else if (propertyName == "aspectCorrection") {
        return m_aspectCorrection ? "yes" : "no";
    } else if (propertyName == "wideScreen") {
        return m_wideScreen ? "yes" : "no";
    } else if (propertyName == "defaultWindowWidth") {
        string res;
        stringstream stringStream;
        stringStream << m_defWindowWidth;
        stringStream >> res;
        return res;
    } else if (propertyName == "defaultWindowHeight") {
        string res;
        stringstream stringStream;
        stringStream << m_defWindowHeight;
        stringStream >> res;
        return res;
    }

    return "";
}
