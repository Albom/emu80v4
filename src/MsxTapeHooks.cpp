﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2021
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

#include "Emulation.h"
#include "WavReader.h"
#include "MsxTapeHooks.h"
#include "Cpu8080.h"
#include "TapeRedirector.h"

using namespace std;


static const uint8_t headerSeq[8] = {0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74};
static const uint8_t lvtHeaderSeq[9] = {0x4C, 0x56, 0x4F, 0x56, 0x2F, 0x32, 0x2E, 0x30, 0x2F}; // "LVOV/2.0/"


bool MsxTapeOutHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        return false;

    /*if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;*/

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);
    uint8_t outByte;
    if (m_regC)
        outByte = cpu->getBC() & 0xFF;
    else
        outByte = (cpu->getAF() & 0xFF00) >> 8;

    if (m_curPos != 0 && m_file->getPos() == 8) // LVT signature length
        m_curPos = 0;

    if (!m_file->isLvt() || m_curPos >= 9)
        m_file->writeByte(outByte);
    ++m_curPos;

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


bool MsxTapeOutHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CpuHook::setProperty(propertyName, values))
        return true;

    if (propertyName == "outReg") {
        if (values[0].asString() == "A" || values[0].asString() == "C") {
            m_regC = values[0].asString() == "C";
            return true;
        }
    }

    return false;
}


bool MsxTapeOutHeaderHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    if (!m_file->isLvt()) {
        // CAS
        unsigned padding = 8 - m_file->getPos() % 8;
        padding %= 8;

        for (unsigned i = 0; i < padding; i++)
            m_file->writeByte(0);

        for (int i = 0; i < 8; i++)
            m_file->writeByte(headerSeq[i]);
    } else {
        // LVT
        bool longHeader = (static_cast<Cpu8080Compatible*>(m_cpu)->getAF() & 0xFF00) != 0;

        // if (longHeader)
        // m_file->switchToNextFile()

        if (longHeader)
            for (int i = 0; i < 9; i++)
                m_file->writeByte(lvtHeaderSeq[i]);
    }

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


bool MsxTapeInHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    uint8_t inByte = 0;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);
    uint16_t af = cpu->getAF() & 0xfffe;

    if (!m_file->isOpen() || m_file->isEof())
        af |= 0x0001; // set C

    //int pos = m_file->getPos();
    if (m_apogeyFix && m_file && m_file->getPos() == 24)
        m_file->waitForSequence(headerSeq, 8); // читаем короткий заголовок после заголовка файла

    if (m_file) {
        if (!m_file->isLvt())
            // CAS
            inByte = m_ignoreHeaders ? m_file->readByteSkipSeq(headerSeq, 8) : m_file->readByte();
        else {
            // LVT
            int filePos = m_file->getPos();
            if (filePos == 9) {
                m_type = m_file->readByte();
                m_typeRptCount = 10;
                inByte = m_type;
            } else if (filePos == 10) {
                if (--m_typeRptCount)
                    inByte = m_type;
                else
                    inByte = m_file->readByte();
            } else
                inByte = m_file->readByte();
        }
    }

    if (m_file->isCancelled())
        return false;

    cpu->setAF((af & 0xFF) | (inByte << 8));

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


bool MsxTapeInHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CpuHook::setProperty(propertyName, values))
        return true;

    if (propertyName == "ignoreHeaders") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_ignoreHeaders = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "apogeyFix") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_apogeyFix = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


bool MsxTapeInHeaderHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);
    uint16_t af = cpu->getAF() & 0xfffe;

    if (!m_file->isOpen() || m_file->isEof())
        af |= 0x0001; // set C

    //int pos = m_file->getPos();

    if (m_file) {
        if (!m_file->isLvt())
            // CAS
            m_file->waitForSequence(headerSeq, 8);
        else {
            // LVT
            if (m_file->getPos() == 0)
                m_file->skipSeq(lvtHeaderSeq, 9);
        }
    }

    cpu->setAF((af));

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}
