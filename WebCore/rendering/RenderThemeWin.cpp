/*
 * Copyright (C) 2006, 2007 Apple Inc.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderThemeWin.h"

#include "CSSValueKeywords.h"
#include "Document.h"
#include "GraphicsContext.h"
#include "HTMLElement.h"
#include "HTMLSelectElement.h"
#include "Icon.h"
#include "RenderSlider.h"
#include "SoftLinking.h"

#include <tchar.h>

/* 
 * The following constants are used to determine how a widget is drawn using
 * Windows' Theme API. For more information on theme parts and states see
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/commctls/userex/topics/partsandstates.asp
 */

// Generic state constants
#define TS_NORMAL    1
#define TS_HOVER     2
#define TS_ACTIVE    3
#define TS_DISABLED  4
#define TS_FOCUSED   5

// Button constants
#define BP_BUTTON    1
#define BP_RADIO     2
#define BP_CHECKBOX  3

// Textfield constants
#define TFP_TEXTFIELD 1
#define TFS_READONLY  6

// ComboBox constants (from tmschema.h)
#define CP_DROPDOWNBUTTON 1

// TrackBar (slider) parts
#define TKP_TRACK       1
#define TKP_TRACKVERT   2

// TrackBar (slider) thumb parts
#define TKP_THUMBBOTTOM 4
#define TKP_THUMBTOP    5
#define TKP_THUMBLEFT   7
#define TKP_THUMBRIGHT  8

// Trackbar (slider) thumb states
#define TUS_NORMAL      1
#define TUS_HOT         2
#define TUS_PRESSED     3
#define TUS_FOCUSED     4
#define TUS_DISABLED    5

// button states
#define PBS_NORMAL      1
#define PBS_HOT         2
#define PBS_PRESSED     3
#define PBS_DISABLED    4
#define PBS_DEFAULTED   5

// This is the fixed width IE and Firefox use for buttons on dropdown menus
static const int dropDownButtonWidth = 17;

static const int shell32MagnifierIconIndex = 22;

SOFT_LINK_LIBRARY(uxtheme)
SOFT_LINK(uxtheme, OpenThemeData, HANDLE, WINAPI, (HWND hwnd, LPCWSTR pszClassList), (hwnd, pszClassList))
SOFT_LINK(uxtheme, CloseThemeData, HRESULT, WINAPI, (HANDLE hTheme), (hTheme))
SOFT_LINK(uxtheme, DrawThemeBackground, HRESULT, WINAPI, (HANDLE hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect), (hTheme, hdc, iPartId, iStateId, pRect, pClipRect))
SOFT_LINK(uxtheme, IsThemeActive, BOOL, WINAPI, (), ())
SOFT_LINK(uxtheme, IsThemeBackgroundPartiallyTransparent, BOOL, WINAPI, (HANDLE hTheme, int iPartId, int iStateId), (hTheme, iPartId, iStateId))

static bool haveTheme;

namespace WebCore {

static bool gWebKitIsBeingUnloaded;

void RenderThemeWin::setWebKitIsBeingUnloaded()
{
    gWebKitIsBeingUnloaded = true;
}

#if !USE(SAFARI_THEME)
RenderTheme* theme()
{
    static RenderThemeWin winTheme;
    return &winTheme;
}
#endif

RenderThemeWin::RenderThemeWin()
    : m_buttonTheme(0)
    , m_textFieldTheme(0)
    , m_menuListTheme(0)
    , m_sliderTheme(0)
{
    haveTheme = uxthemeLibrary() && IsThemeActive();
}

RenderThemeWin::~RenderThemeWin()
{
    // If WebKit is being unloaded, then uxtheme.dll is no longer available.
    if (gWebKitIsBeingUnloaded || !uxthemeLibrary())
        return;
    close();
}

HANDLE RenderThemeWin::buttonTheme() const
{
    if (haveTheme && !m_buttonTheme)
        m_buttonTheme = OpenThemeData(0, L"Button");
    return m_buttonTheme;
}

HANDLE RenderThemeWin::textFieldTheme() const
{
    if (haveTheme && !m_textFieldTheme)
        m_textFieldTheme = OpenThemeData(0, L"Edit");
    return m_textFieldTheme;
}

HANDLE RenderThemeWin::menuListTheme() const
{
    if (haveTheme && !m_menuListTheme)
        m_menuListTheme = OpenThemeData(0, L"ComboBox");
    return m_menuListTheme;
}

HANDLE RenderThemeWin::sliderTheme() const
{
    if (haveTheme && !m_sliderTheme)
        m_sliderTheme = OpenThemeData(0, L"TrackBar");
    return m_sliderTheme;
}

void RenderThemeWin::close()
{
    // This method will need to be called when the OS theme changes to flush our cached themes.
    if (m_buttonTheme)
        CloseThemeData(m_buttonTheme);
    if (m_textFieldTheme)
        CloseThemeData(m_textFieldTheme);
    if (m_menuListTheme)
        CloseThemeData(m_menuListTheme);
    if (m_sliderTheme)
        CloseThemeData(m_sliderTheme);
    m_buttonTheme = m_textFieldTheme = m_menuListTheme = m_sliderTheme = 0;

    haveTheme = uxthemeLibrary() && IsThemeActive();
}

void RenderThemeWin::themeChanged()
{
    close();
}

bool RenderThemeWin::supportsHover(const RenderStyle*) const
{
    // The Classic/2k look has no hover effects.
    return haveTheme;
}

Color RenderThemeWin::platformActiveSelectionBackgroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWin::platformInactiveSelectionBackgroundColor() const
{
    COLORREF color = GetSysColor(COLOR_GRAYTEXT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWin::platformActiveSelectionForegroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHTTEXT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWin::platformInactiveSelectionForegroundColor() const
{
    return Color::white;
}

static void fillFontDescription(FontDescription& fontDescription, LOGFONT& logFont)
{    
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.firstFamily().setFamily(String(logFont.lfFaceName));
    fontDescription.setSpecifiedSize(abs(logFont.lfHeight));
    fontDescription.setWeight(logFont.lfWeight >= 700 ? FontWeightBold : FontWeightNormal); // FIXME: Use real weight.
    fontDescription.setItalic(logFont.lfItalic);
}

void RenderThemeWin::systemFont(int propId, FontDescription& fontDescription) const
{
    static FontDescription captionFont;
    static FontDescription smallCaptionFont;
    static FontDescription menuFont;
    static FontDescription iconFont;
    static FontDescription messageBoxFont;
    static FontDescription statusBarFont;
    static FontDescription systemFont;
    
    static bool initialized;
    static NONCLIENTMETRICS ncm;

    if (!initialized) {
        initialized = true;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    }
 
    switch (propId) {
        case CSSValueIcon: {
            if (!iconFont.isAbsoluteSize()) {
                LOGFONT logFont;
                ::SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(logFont), &logFont, 0);
                fillFontDescription(iconFont, logFont);
            }
            fontDescription = iconFont;
            break;
        }
        case CSSValueMenu:
            if (!menuFont.isAbsoluteSize())
                fillFontDescription(menuFont, ncm.lfMenuFont);
            fontDescription = menuFont;
            break;
        case CSSValueMessageBox:
            if (!messageBoxFont.isAbsoluteSize())
                fillFontDescription(messageBoxFont, ncm.lfMessageFont);
            fontDescription = messageBoxFont;
            break;
        case CSSValueStatusBar:
            if (!statusBarFont.isAbsoluteSize())
                fillFontDescription(statusBarFont, ncm.lfStatusFont);
            fontDescription = statusBarFont;
            break;
        case CSSValueCaption:
            if (!captionFont.isAbsoluteSize())
                fillFontDescription(captionFont, ncm.lfCaptionFont);
            fontDescription = captionFont;
            break;
        case CSSValueSmallCaption:
        case CSSValueWebkitSmallControl: // Equivalent to small-caption.
        case CSSValueWebkitMiniControl: // Just map to small.
        case CSSValueWebkitControl: // Just map to small.
            if (!smallCaptionFont.isAbsoluteSize())
                fillFontDescription(smallCaptionFont, ncm.lfSmCaptionFont);
            fontDescription = smallCaptionFont;
            break;
        default: { // Everything else uses the stock GUI font.
            if (!systemFont.isAbsoluteSize()) {
                HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
                if (hGDI) {
                    LOGFONT logFont;
                    if (::GetObject(hGDI, sizeof(logFont), &logFont) > 0)
                        fillFontDescription(systemFont, logFont);
                }
            }
            fontDescription = systemFont;
        }
    }
}

bool RenderThemeWin::supportsFocus(ControlPart appearance)
{
    switch (appearance) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
        case TextFieldPart:
        case TextAreaPart:
            return true;
        case MenulistPart:
            return false;
        default:
            return false;
    }
}

unsigned RenderThemeWin::determineClassicState(RenderObject* o)
{
    unsigned state = 0;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
            state = DFCS_BUTTONPUSH;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
            break;
        case RadioPart:
        case CheckboxPart:
            state = (o->style()->appearance() == RadioPart) ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
            if (isChecked(o))
                state |= DFCS_CHECKED;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
            break;
        case MenulistPart:
            state = DFCS_SCROLLCOMBOBOX;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
        default:
            break;
    }
    return state;
}

unsigned RenderThemeWin::determineState(RenderObject* o)
{
    unsigned result = TS_NORMAL;
    ControlPart appearance = o->style()->appearance();
    if (!isEnabled(o))
        result = TS_DISABLED;
    else if (isReadOnlyControl(o) && (TextFieldPart == appearance || TextAreaPart == appearance))
        result = TFS_READONLY; // Readonly is supported on textfields.
    else if (isPressed(o)) // Active overrides hover and focused.
        result = TS_ACTIVE;
    else if (supportsFocus(appearance) && isFocused(o))
        result = TS_FOCUSED;
    else if (isHovered(o))
        result = TS_HOVER;
    if (isChecked(o))
        result += 4; // 4 unchecked states, 4 checked states.
    return result;
}

unsigned RenderThemeWin::determineSliderThumbState(RenderObject* o)
{
    unsigned result = TUS_NORMAL;
    if (!isEnabled(o->parent()))
        result = TUS_DISABLED;
    else if (supportsFocus(o->style()->appearance()) && isFocused(o->parent()))
        result = TUS_FOCUSED;
    else if (static_cast<RenderSlider*>(o->parent())->inDragMode())
        result = TUS_PRESSED;
    else if (isHovered(o))
        result = TUS_HOT;
    return result;
}

unsigned RenderThemeWin::determineButtonState(RenderObject* o)
{
    unsigned result = PBS_NORMAL;
    if (!isEnabled(o))
        result = PBS_DISABLED;
    else if (isPressed(o))
        result = PBS_PRESSED;
    else if (supportsFocus(o->style()->appearance()) && isFocused(o))
        result = PBS_DEFAULTED;
    else if (isHovered(o))
        result = PBS_HOT;
    else if (isDefault(o))
        result = PBS_DEFAULTED;
    return result;
}

ThemeData RenderThemeWin::getClassicThemeData(RenderObject* o)
{
    ThemeData result;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
        case CheckboxPart:
        case RadioPart:
            result.m_part = DFC_BUTTON;
            result.m_state = determineClassicState(o);
            break;
        case MenulistPart:
            result.m_part = DFC_SCROLL;
            result.m_state = determineClassicState(o);
            break;
        case TextFieldPart:
        case TextAreaPart:
            result.m_part = TFP_TEXTFIELD;
            result.m_state = determineState(o);
            break;
        case SliderHorizontalPart:
            result.m_part = TKP_TRACK;
            result.m_state = TS_NORMAL;
            break;
        case SliderVerticalPart:
            result.m_part = TKP_TRACKVERT;
            result.m_state = TS_NORMAL;
            break;
        case SliderThumbHorizontalPart:
            result.m_part = TKP_THUMBBOTTOM;
            result.m_state = determineSliderThumbState(o);
            break;
        case SliderThumbVerticalPart:
            result.m_part = TKP_THUMBRIGHT;
            result.m_state = determineSliderThumbState(o);
            break;
        default:
            break;
    }
    return result;
}

ThemeData RenderThemeWin::getThemeData(RenderObject* o)
{
    if (!haveTheme)
        return getClassicThemeData(o);

    ThemeData result;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
            result.m_part = BP_BUTTON;
            result.m_state = determineButtonState(o);
            break;
        case CheckboxPart:
            result.m_part = BP_CHECKBOX;
            result.m_state = determineState(o);
            break;
        case MenulistPart:
        case MenulistButtonPart:
            result.m_part = CP_DROPDOWNBUTTON;
            result.m_state = determineState(o);
            break;
        case RadioPart:
            result.m_part = BP_RADIO;
            result.m_state = determineState(o);
            break;
        case TextFieldPart:
        case TextAreaPart:
            result.m_part = TFP_TEXTFIELD;
            result.m_state = determineState(o);
            break;
        case SliderHorizontalPart:
            result.m_part = TKP_TRACK;
            result.m_state = TS_NORMAL;
            break;
        case SliderVerticalPart:
            result.m_part = TKP_TRACKVERT;
            result.m_state = TS_NORMAL;
            break;
        case SliderThumbHorizontalPart:
            result.m_part = TKP_THUMBBOTTOM;
            result.m_state = determineSliderThumbState(o);
            break;
        case SliderThumbVerticalPart:
            result.m_part = TKP_THUMBRIGHT;
            result.m_state = determineSliderThumbState(o);
            break;
    }

    return result;
}

static void drawControl(GraphicsContext* context, RenderObject* o, HANDLE theme, const ThemeData& themeData, const IntRect& r)
{
    bool alphaBlend = false;
    if (theme)
        alphaBlend = IsThemeBackgroundPartiallyTransparent(theme, themeData.m_part, themeData.m_state);
    HDC hdc = context->getWindowsContext(r, alphaBlend);
    RECT widgetRect = r;
    if (theme)
        DrawThemeBackground(theme, hdc, themeData.m_part, themeData.m_state, &widgetRect, NULL);
    else {
        if (themeData.m_part == TFP_TEXTFIELD) {
            ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            if (themeData.m_state == TS_DISABLED || themeData.m_state ==  TFS_READONLY)
                ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_BTNFACE+1));
            else
                ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_WINDOW+1));
        } else if (themeData.m_part == TKP_TRACK || themeData.m_part == TKP_TRACKVERT) {
            ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            ::FillRect(hdc, &widgetRect, (HBRUSH)GetStockObject(GRAY_BRUSH));
        } else if ((o->style()->appearance() == SliderThumbHorizontalPart ||
                    o->style()->appearance() == SliderThumbVerticalPart) && 
                   (themeData.m_part == TKP_THUMBBOTTOM || themeData.m_part == TKP_THUMBTOP || 
                    themeData.m_part == TKP_THUMBLEFT || themeData.m_part == TKP_THUMBRIGHT)) {
            ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
            if (themeData.m_state == TUS_DISABLED) {
                static WORD patternBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
                HBITMAP patternBmp = ::CreateBitmap(8, 8, 1, 1, patternBits);
                if (patternBmp) {
                    HBRUSH brush = (HBRUSH) ::CreatePatternBrush(patternBmp);
                    COLORREF oldForeColor = ::SetTextColor(hdc, ::GetSysColor(COLOR_3DFACE));
                    COLORREF oldBackColor = ::SetBkColor(hdc, ::GetSysColor(COLOR_3DHILIGHT));
                    POINT p;
                    ::GetViewportOrgEx(hdc, &p);
                    ::SetBrushOrgEx(hdc, p.x + widgetRect.left, p.y + widgetRect.top, NULL);
                    HBRUSH oldBrush = (HBRUSH) ::SelectObject(hdc, brush);
                    ::FillRect(hdc, &widgetRect, brush);
                    ::SetTextColor(hdc, oldForeColor);
                    ::SetBkColor(hdc, oldBackColor);
                    ::SelectObject(hdc, oldBrush);
                    ::DeleteObject(brush); 
                } else
                    ::FillRect(hdc, &widgetRect, (HBRUSH)COLOR_3DHILIGHT);
                ::DeleteObject(patternBmp);
            }
        } else {
            // Push buttons, buttons, checkboxes and radios, and the dropdown arrow in menulists.
            if (o->style()->appearance() == DefaultButtonPart) {
                HBRUSH brush = ::GetSysColorBrush(COLOR_3DDKSHADOW);
                ::FrameRect(hdc, &widgetRect, brush);
                ::InflateRect(&widgetRect, -1, -1);
                ::DrawEdge(hdc, &widgetRect, BDR_RAISEDOUTER, BF_RECT | BF_MIDDLE);
            }
            ::DrawFrameControl(hdc, &widgetRect, themeData.m_part, themeData.m_state);
        }
    }
    context->releaseWindowsContext(hdc, r, alphaBlend);
}

bool RenderThemeWin::paintButton(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{  
    drawControl(i.context,  o, buttonTheme(), getThemeData(o), r);
    return false;
}

void RenderThemeWin::setCheckboxSize(RenderStyle* style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    // FIXME:  A hard-coded size of 13 is used.  This is wrong but necessary for now.  It matches Firefox.
    // At different DPI settings on Windows, querying the theme gives you a larger size that accounts for
    // the higher DPI.  Until our entire engine honors a DPI setting other than 96, we can't rely on the theme's
    // metrics.
    if (style->width().isIntrinsicOrAuto())
        style->setWidth(Length(13, Fixed));
    if (style->height().isAuto())
        style->setHeight(Length(13, Fixed));
}

bool RenderThemeWin::paintTextField(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{
    drawControl(i.context,  o, textFieldTheme(), getThemeData(o), r);
    return false;
}

bool RenderThemeWin::paintMenuList(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{
    // The outer box of a menu list is just a text field.  Paint it first.
    drawControl(i.context,  o, textFieldTheme(), ThemeData(TFP_TEXTFIELD, determineState(o)), r);
    
    return paintMenuListButton(o, i, r);
}

void RenderThemeWin::adjustMenuListStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    adjustMenuListButtonStyle(selector, style, e);
}

void RenderThemeWin::adjustMenuListButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    // These are the paddings needed to place the text correctly in the <select> box
    const int dropDownBoxPaddingTop    = 2;
    const int dropDownBoxPaddingRight  = style->direction() == LTR ? 4 + dropDownButtonWidth : 4;
    const int dropDownBoxPaddingBottom = 2;
    const int dropDownBoxPaddingLeft   = style->direction() == LTR ? 4 : 4 + dropDownButtonWidth;
    // The <select> box must be at least 12px high for the button to render nicely on Windows
    const int dropDownBoxMinHeight = 12;
    
    // Position the text correctly within the select box and make the box wide enough to fit the dropdown button
    style->setPaddingTop(Length(dropDownBoxPaddingTop, Fixed));
    style->setPaddingRight(Length(dropDownBoxPaddingRight, Fixed));
    style->setPaddingBottom(Length(dropDownBoxPaddingBottom, Fixed));
    style->setPaddingLeft(Length(dropDownBoxPaddingLeft, Fixed));

    // Height is locked to auto
    style->setHeight(Length(Auto));

    // Calculate our min-height
    int minHeight = style->font().height();
    minHeight = max(minHeight, dropDownBoxMinHeight);

    style->setMinHeight(Length(minHeight, Fixed));
    
    // White-space is locked to pre
    style->setWhiteSpace(PRE);
}

bool RenderThemeWin::paintMenuListButton(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{
    // FIXME: Don't make hardcoded assumptions about the thickness of the textfield border.
    int borderThickness = haveTheme ? 1 : 2;

    // Paint the dropdown button on the inner edge of the text field,
    // leaving space for the text field's 1px border
    IntRect buttonRect(r);
    buttonRect.inflate(-borderThickness);
    if (o->style()->direction() == LTR)
        buttonRect.setX(buttonRect.right() - dropDownButtonWidth);
    buttonRect.setWidth(dropDownButtonWidth);

    drawControl(i.context, o, menuListTheme(), getThemeData(o), buttonRect);

    return false;
}

const int trackWidth = 4;

bool RenderThemeWin::paintSliderTrack(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{
    IntRect bounds = r;
    
    if (o->style()->appearance() ==  SliderHorizontalPart) {
        bounds.setHeight(trackWidth);
        bounds.setY(r.y() + r.height() / 2 - trackWidth / 2);
    } else if (o->style()->appearance() == SliderVerticalPart) {
        bounds.setWidth(trackWidth);
        bounds.setX(r.x() + r.width() / 2 - trackWidth / 2);
    }
    
    drawControl(i.context,  o, sliderTheme(), getThemeData(o), bounds);
    return false;
}

bool RenderThemeWin::paintSliderThumb(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{   
    drawControl(i.context,  o, sliderTheme(), getThemeData(o), r);
    return false;
}

const int sliderThumbWidth = 7;
const int sliderThumbHeight = 15;

void RenderThemeWin::adjustSliderThumbSize(RenderObject* o) const
{
    if (o->style()->appearance() == SliderThumbVerticalPart) {
        o->style()->setWidth(Length(sliderThumbHeight, Fixed));
        o->style()->setHeight(Length(sliderThumbWidth, Fixed));
    } else if (o->style()->appearance() == SliderThumbHorizontalPart) {
        o->style()->setWidth(Length(sliderThumbWidth, Fixed));
        o->style()->setHeight(Length(sliderThumbHeight, Fixed));
    }
 }

bool RenderThemeWin::paintSearchField(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

void RenderThemeWin::adjustSearchFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{   
    // Override padding size to match AppKit text positioning.
    const int padding = 1;
    style->setPaddingLeft(Length(padding, Fixed));
    style->setPaddingRight(Length(padding, Fixed));
    style->setPaddingTop(Length(padding, Fixed));
    style->setPaddingBottom(Length(padding, Fixed));
}

bool RenderThemeWin::paintSearchFieldCancelButton(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    Color buttonColor = (o->element() && o->element()->active()) ? Color(138, 138, 138) : Color(186, 186, 186);

    IntSize cancelSize(10, 10);
    IntSize cancelRadius(cancelSize.width() / 2, cancelSize.height() / 2);
    int x = r.x() + (r.width() - cancelSize.width()) / 2;
    int y = r.y() + (r.height() - cancelSize.height()) / 2 + 1;
    IntRect cancelBounds(IntPoint(x, y), cancelSize);
    paintInfo.context->save();
    paintInfo.context->addRoundedRectClip(cancelBounds, cancelRadius, cancelRadius, cancelRadius, cancelRadius);
    paintInfo.context->fillRect(cancelBounds, buttonColor);

    // Draw the 'x'
    IntSize xSize(3, 3);
    IntRect xBounds(cancelBounds.location() + IntSize(3, 3), xSize);
    paintInfo.context->setStrokeColor(Color::white);
    paintInfo.context->drawLine(xBounds.location(),  xBounds.location() + xBounds.size());
    paintInfo.context->drawLine(IntPoint(xBounds.right(), xBounds.y()),  IntPoint(xBounds.x(), xBounds.bottom()));

    paintInfo.context->restore(); 
    return false;
}

void RenderThemeWin::adjustSearchFieldCancelButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    IntSize cancelSize(13, 11);
    style->setWidth(Length(cancelSize.width(), Fixed));
    style->setHeight(Length(cancelSize.height(), Fixed));
}

void RenderThemeWin::adjustSearchFieldDecorationStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    IntSize emptySize(1, 11);
    style->setWidth(Length(emptySize.width(), Fixed));
    style->setHeight(Length(emptySize.height(), Fixed));
}
   
void RenderThemeWin::adjustSearchFieldResultsDecorationStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    IntSize magnifierSize(15, 11);
    style->setWidth(Length(magnifierSize.width(), Fixed));
    style->setHeight(Length(magnifierSize.height(), Fixed));
}

bool RenderThemeWin::paintSearchFieldResultsDecoration(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    IntRect bounds = r;
    bounds.setWidth(bounds.height());

    TCHAR buffer[MAX_PATH];
    UINT length = ::GetSystemDirectory(buffer, ARRAYSIZE(buffer));
    if (!length)
        return 0;
    
    if (_tcscat_s(buffer, TEXT("\\shell32.dll")))
        return 0;

    HICON hIcon;
    if (!::ExtractIconEx(buffer, shell32MagnifierIconIndex, 0, &hIcon, 1))
        return 0;

    RefPtr<Icon> icon = Icon::create(hIcon);
    icon->paint(paintInfo.context, bounds);
    return false;
}

void RenderThemeWin::adjustSearchFieldResultsButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    IntSize magnifierSize(15, 11);
    style->setWidth(Length(magnifierSize.width(), Fixed));
    style->setHeight(Length(magnifierSize.height(), Fixed));
}

bool RenderThemeWin::paintSearchFieldResultsButton(RenderObject* o, const RenderObject::PaintInfo& paintInfo, const IntRect& r)
{
    paintSearchFieldResultsDecoration(o, paintInfo, r);
    return false;
}

// Map a CSSValue* system color to an index understood by GetSysColor
static int cssValueIdToSysColorIndex(int cssValueId)
{
    switch (cssValueId) {
        case CSSValueActiveborder: return COLOR_ACTIVEBORDER;
        case CSSValueActivecaption: return COLOR_ACTIVECAPTION;
        case CSSValueAppworkspace: return COLOR_APPWORKSPACE;
        case CSSValueBackground: return COLOR_BACKGROUND;
        case CSSValueButtonface: return COLOR_BTNFACE;
        case CSSValueButtonhighlight: return COLOR_BTNHIGHLIGHT;
        case CSSValueButtonshadow: return COLOR_BTNSHADOW;
        case CSSValueButtontext: return COLOR_BTNTEXT;
        case CSSValueCaptiontext: return COLOR_CAPTIONTEXT;
        case CSSValueGraytext: return COLOR_GRAYTEXT;
        case CSSValueHighlight: return COLOR_HIGHLIGHT;
        case CSSValueHighlighttext: return COLOR_HIGHLIGHTTEXT;
        case CSSValueInactiveborder: return COLOR_INACTIVEBORDER;
        case CSSValueInactivecaption: return COLOR_INACTIVECAPTION;
        case CSSValueInactivecaptiontext: return COLOR_INACTIVECAPTIONTEXT;
        case CSSValueInfobackground: return COLOR_INFOBK;
        case CSSValueInfotext: return COLOR_INFOTEXT;
        case CSSValueMenu: return COLOR_MENU;
        case CSSValueMenutext: return COLOR_MENUTEXT;
        case CSSValueScrollbar: return COLOR_SCROLLBAR;
        case CSSValueThreeddarkshadow: return COLOR_3DDKSHADOW;
        case CSSValueThreedface: return COLOR_3DFACE;
        case CSSValueThreedhighlight: return COLOR_3DHIGHLIGHT;
        case CSSValueThreedlightshadow: return COLOR_3DLIGHT;
        case CSSValueThreedshadow: return COLOR_3DSHADOW;
        case CSSValueWindow: return COLOR_WINDOW;
        case CSSValueWindowframe: return COLOR_WINDOWFRAME;
        case CSSValueWindowtext: return COLOR_WINDOWTEXT;
        default: return -1; // Unsupported CSSValue
    }
}

Color RenderThemeWin::systemColor(int cssValueId) const
{
    int sysColorIndex = cssValueIdToSysColorIndex(cssValueId);
    ASSERT(sysColorIndex != -1);
    COLORREF color = GetSysColor(sysColorIndex);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

}
