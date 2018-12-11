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

#ifndef KBDLAYOUT_H
#define KBDLAYOUT_H

#include "PalKeys.h"
#include "EmuObjects.h"

enum EmuKey
{
    EK_NONE,

    // Common keys
    EK_0,
    EK_1,
    EK_2,
    EK_3,
    EK_4,
    EK_5,
    EK_6,
    EK_7,
    EK_8,
    EK_9,

    EK_A,
    EK_B,
    EK_C,
    EK_D,
    EK_E,
    EK_F,
    EK_G,
    EK_H,
    EK_I,
    EK_J,
    EK_K,
    EK_L,
    EK_M,
    EK_N,
    EK_O,
    EK_P,
    EK_Q,
    EK_R,
    EK_S,
    EK_T,
    EK_U,
    EK_V,
    EK_W,
    EK_X,
    EK_Y,
    EK_Z,

    EK_LBRACKET,
    EK_RBRACKET,
    EK_BKSLASH,
    EK_CARET,
    EK_AT,

    EK_SEMICOLON,
    EK_MINUS,
    EK_COLON,
    EK_PERIOD,
    EK_COMMA,
    EK_SLASH,
    EK_SPACE,

    EK_ESC,
    EK_TAB,
    EK_CR,
    EK_BSP,

    EK_F1,
    EK_F2,
    EK_F3,
    EK_F4,
    EK_F5,
    EK_LEFT,
    EK_UP,
    EK_RIGHT,
    EK_DOWN,

    EK_SHIFT,
    EK_CTRL,
    EK_LANG,

    // Added in RK86
    EK_LF,
    EK_HOME,
    EK_CLEAR,

    // Added in Specialist
    EK_F6,
    EK_F7,
    EK_F8,
    EK_F9,
    EK_F10,
    EK_F11,
    EK_RPT
};



class KbdLayout : public EmuObject
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void setQwertyMode() {m_mode = KLM_QWERTY;}
        void setJcukenMode() {m_mode = KLM_JCUKEN;}
        void setSmartMode()  {m_mode = KLM_SMART;}
        void processKey(PalKeyCode keyCode, bool isPressed, unsigned unicodeKey = 0);
        void resetKeys();

    protected:
        enum KbdLayoutMode {
            KLM_QWERTY,
            KLM_JCUKEN,
            KLM_SMART
        };

        KbdLayoutMode m_mode = KLM_QWERTY;
        virtual EmuKey translateKey(PalKeyCode keyCode) {return translateCommonKeys(keyCode);}
        virtual EmuKey translateUnicodeKey(unsigned unicodeKey, bool& shift, bool& lang) {return translateCommonUnicodeKeys(unicodeKey, shift, lang);}

        EmuKey translateCommonKeys(PalKeyCode keyCode);
        EmuKey translateCommonUnicodeKeys(unsigned unicodeKey, bool& shift, bool& lang);

    private:
        bool m_shiftPressed = false;
        bool m_langPressed = false;
        EmuKey m_lastNonUnicodeKey = EK_NONE;

        EmuKey translateCommonKeysQwerty(PalKeyCode keyCode);
        EmuKey translateCommonKeysJcuken(PalKeyCode keyCode);
};



class RkKbdLayout : public KbdLayout
{
    protected:
        virtual EmuKey translateKey(PalKeyCode keyCode);
        virtual EmuKey translateUnicodeKey(unsigned unicodeKey, bool& shift, bool& lang);

    public:
        static EmuObject* create(const EmuValuesList&) {return new RkKbdLayout();}
};

#endif  // KBDLAYOUT_H
