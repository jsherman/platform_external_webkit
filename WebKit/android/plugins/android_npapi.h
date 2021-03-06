/*
 * Copyright 2008, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*  Defines the android-specific types and functions as part of npapi
 
    In particular, defines the window and event types that are passed to
    NPN_GetValue, NPP_SetWindow and NPP_HandleEvent.
 
    To minimize what native libraries the plugin links against, some
    functionality is provided via function-ptrs (e.g. time, sound)
 */

#ifndef android_npapi_H
#define android_npapi_H

#include <stdint.h>

#include "npapi.h"

///////////////////////////////////////////////////////////////////////////////
// General types

enum ANPBitmapFormats {
    kUnknown_ANPBitmapFormat    = 0,
    kRGBA_8888_ANPBitmapFormat  = 1,
    kRGB_565_ANPBitmapFormat    = 2
};
typedef int32_t ANPBitmapFormat;

struct ANPBitmap {
    void*           baseAddr;
    ANPBitmapFormat format;
    int32_t         width;
    int32_t         height;
    int32_t         rowBytes;
};

struct ANPRectF {
    float   left;
    float   top;
    float   right;
    float   bottom;
};

struct ANPRectI {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

struct ANPCanvas;
struct ANPPaint;
struct ANPPath;
struct ANPRegion;
struct ANPTypeface;

///////////////////////////////////////////////////////////////////////////////
// NPN_GetValue

/*  queries for a specific ANPInterface.
 
    Maybe called with NULL for the NPP instance
 
    NPN_GetValue(inst, interface_enum, ANPInterface*)
 */
#define kLogInterfaceV0_ANPGetValue         ((NPNVariable)1000)
#define kAudioTrackInterfaceV0_ANPGetValue  ((NPNVariable)1001)
#define kCanvasInterfaceV0_ANPGetValue      ((NPNVariable)1002)
#define kPaintInterfaceV0_ANPGetValue       ((NPNVariable)1003)
#define kTypefaceInterfaceV0_ANPGetValue    ((NPNVariable)1004)
#define kWindowInterfaceV0_ANPGetValue      ((NPNVariable)1005)

/*  queries for which drawing model is desired (for the draw event)
 
    Should be called inside NPP_New(...)
 
    NPN_GetValue(inst, ANPSupportedDrawingModel_EnumValue, uint32_t* bits)
 */
#define kSupportedDrawingModel_ANPGetValue  ((NPNVariable)2000)

///////////////////////////////////////////////////////////////////////////////
// NPN_GetValue

/** Reqeust to set the drawing model.
 
    NPN_SetValue(inst, ANPRequestDrawingModel_EnumValue, (void*)foo_DrawingModel)
 */
#define kRequestDrawingModel_ANPSetValue    ((NPPVariable)1000)

/*  These are used as bitfields in ANPSupportedDrawingModels_EnumValue,
    and as-is in ANPRequestDrawingModel_EnumValue. The drawing model determines
    how to interpret the ANPDrawingContext provided in the Draw event and how
    to interpret the NPWindow->window field.
 */
enum ANPDrawingModels {
    /** Draw into a bitmap from the browser thread in response to a Draw event.
        NPWindow->window is reserved (ignore)
     */
    kBitmap_ANPDrawingModel = 0,
};
typedef int32_t ANPDrawingModel;

/*  Interfaces provide additional functionality to the plugin via function ptrs.
    Once an interface is retrived, it is valid for the lifetime of the plugin
    (just like browserfuncs).
 
    All ANPInterfaces begin with an inSize field, which must be set by the
    caller (plugin) with the number of bytes allocated for the interface.
    e.g. SomeInterface si; si.inSize = sizeof(si); browser->getvalue(..., &si);
 */
struct ANPInterface {
    uint32_t    inSize;     // size (in bytes) of this struct
};

enum ANPLogTypes {
    kError_ANPLogType   = 0,    // error
    kWarning_ANPLogType = 1,    // warning
    kDebug_ANPLogType   = 2     // debug only (informational)
};
typedef int32_t ANPLogType;

struct ANPLogInterfaceV0 : ANPInterface {
    // dumps printf messages to the log file
    // e.g. interface->log(instance, kWarning_ANPLogType, "value is %d", value);
    void (*log)(NPP instance, ANPLogType, const char format[], ...);
};

typedef uint32_t ANPColor;
#define ANP_MAKE_COLOR(a, r, g, b)  \
                                (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

enum ANPPaintFlag {
    kAntiAlias_ANPPaintFlag         = 1 << 0,
    kFilterBitmap_ANPPaintFlag      = 1 << 1,
    kDither_ANPPaintFlag            = 1 << 2,
    kUnderlineText_ANPPaintFlag     = 1 << 3,
    kStrikeThruText_ANPPaintFlag    = 1 << 4,
    kFakeBoldText_ANPPaintFlag      = 1 << 5,
};
typedef uint32_t ANPPaintFlags;

enum ANPPaintStyles {
    kFill_ANPPaintStyle             = 0,
    kStroke_ANPPaintStyle           = 1,
    kFillAndStroke_ANPPaintStyle    = 2
};
typedef int32_t ANPPaintStyle;

enum ANPPaintCaps {
    kButt_ANPPaintCap   = 0,
    kRound_ANPPaintCap  = 1,
    kSquare_ANPPaintCap = 2
};
typedef int32_t ANPPaintCap;

enum ANPPaintJoins {
    kMiter_ANPPaintJoin = 0,
    kRound_ANPPaintJoin = 1,
    kBevel_ANPPaintJoin = 2
};
typedef int32_t ANPPaintJoin;

enum ANPPaintAligns {
    kLeft_ANPPaintAlign     = 0,
    kCenter_ANPPaintAlign   = 1,
    kRight_ANPPaintAlign    = 2
};
typedef int32_t ANPPaintAlign;

enum ANPTextEncodings {
    kUTF8_ANPTextEncoding   = 0,
    kUTF16_ANPTextEncoding  = 1,
};
typedef int32_t ANPTextEncoding;

enum ANPTypefaceStyles {
    kBold_ANPTypefaceStyle      = 1 << 0,
    kItalic_ANPTypefaceStyle    = 1 << 1
};
typedef uint32_t ANPTypefaceStyle;

struct ANPTypefaceInterfaceV0 : ANPInterface {
    /** Return a new reference to the typeface that most closely matches the
        requested name and style. Pass null as the name to return
        the default font for the requested style. Will never return null

        @param name     May be NULL. The name of the font family.
        @param style    The style (normal, bold, italic) of the typeface.
        @return reference to the closest-matching typeface. Caller must call
                unref() when they are done with the typeface.
     */
    ANPTypeface* (*createFromName)(const char name[], ANPTypefaceStyle);

    /** Return a new reference to the typeface that most closely matches the
        requested typeface and specified Style. Use this call if you want to
        pick a new style from the same family of the existing typeface.
        If family is NULL, this selects from the default font's family.

        @param family  May be NULL. The name of the existing type face.
        @param s       The style (normal, bold, italic) of the type face.
        @return reference to the closest-matching typeface. Call must call
                unref() when they are done.
     */
    ANPTypeface* (*createFromTypeface)(const ANPTypeface* family,
                                       ANPTypefaceStyle);
    
    /** Return the owner count of the typeface. A newly created typeface has an
        owner count of 1. When the owner count is reaches 0, the typeface is
        deleted.
     */
    int32_t (*getRefCount)(const ANPTypeface*);

    /** Increment the owner count on the typeface
     */
    void (*ref)(ANPTypeface*);

    /** Decrement the owner count on the typeface. When the count goes to 0,
        the typeface is deleted.
     */
    void (*unref)(ANPTypeface*);
    
    /** Return the style bits for the specified typeface
     */
    ANPTypefaceStyle (*getStyle)(const ANPTypeface*);
};

struct ANPPaintInterfaceV0 : ANPInterface {
    /*  Return a new paint object, which holds all of the color and style
        attributes that affect how things (geometry, text, bitmaps) are drawn
        in a ANPCanvas.
    
        The paint that is returned is not tied to any particular plugin
        instance, but it must only be accessed from one thread at a time.
     */
    ANPPaint*   (*newPaint)();
    void        (*deletePaint)(ANPPaint*);
    
    ANPPaintFlags (*getFlags)(const ANPPaint*);
    void        (*setFlags)(ANPPaint*, ANPPaintFlags);
    
    ANPColor    (*getColor)(const ANPPaint*);
    void        (*setColor)(ANPPaint*, ANPColor);
    
    ANPPaintStyle (*getStyle)(const ANPPaint*);
    void        (*setStyle)(ANPPaint*, ANPPaintStyle);

    float       (*getStrokeWidth)(const ANPPaint*);
    float       (*getStrokeMiter)(const ANPPaint*);
    ANPPaintCap (*getStrokeCap)(const ANPPaint*);
    ANPPaintJoin (*getStrokeJoin)(const ANPPaint*);
    void        (*setStrokeWidth)(ANPPaint*, float);
    void        (*setStrokeMiter)(ANPPaint*, float);
    void        (*setStrokeCap)(ANPPaint*, ANPPaintCap);
    void        (*setStrokeJoin)(ANPPaint*, ANPPaintJoin);
    
    ANPTextEncoding (*getTextEncoding)(const ANPPaint*);
    ANPPaintAlign (*getTextAlign)(const ANPPaint*);
    float       (*getTextSize)(const ANPPaint*);
    float       (*getTextScaleX)(const ANPPaint*);
    float       (*getTextSkewX)(const ANPPaint*);
    void        (*setTextEncoding)(ANPPaint*, ANPTextEncoding);
    void        (*setTextAlign)(ANPPaint*, ANPPaintAlign);
    void        (*setTextSize)(ANPPaint*, float);
    void        (*setTextScaleX)(ANPPaint*, float);
    void        (*setTextSkewX)(ANPPaint*, float);

    /** Return the typeface ine paint, or null if there is none. This does not
        modify the owner count of the returned typeface.
     */
    ANPTypeface* (*getTypeface)(const ANPPaint*);

    /** Set the paint's typeface. If the paint already had a non-null typeface,
        its owner count is decremented. If the new typeface is non-null, its
        owner count is incremented.
     */
    void (*setTypeface)(ANPPaint*, ANPTypeface*);

    /** Return the width of the text. If bounds is not null, return the bounds
        of the text in that rectangle.
     */
    float (*measureText)(ANPPaint*, const void* text, uint32_t byteLength,
                         ANPRectF* bounds);
    
    /** Return the number of unichars specifed by the text.
        If widths is not null, returns the array of advance widths for each
            unichar.
        If bounds is not null, returns the array of bounds for each unichar.
     */
    int (*getTextWidths)(ANPPaint*, const void* text, uint32_t byteLength,
                         float widths[], ANPRectF bounds[]);
};

struct ANPCanvasInterfaceV0 : ANPInterface {
    /*  Return a canvas that will draw into the specified bitmap. Note: the
        canvas copies the fields of the bitmap, so it need not persist after
        this call, but the canvas DOES point to the same pixel memory that the
        bitmap did, so the canvas should not be used after that pixel memory
        goes out of scope. In the case of creating a canvas to draw into the
        pixels provided by kDraw_ANPEventType, those pixels are only while
        handling that event.
     
        The canvas that is returned is not tied to any particular plugin
        instance, but it must only be accessed from one thread at a time.
     */
    ANPCanvas*  (*newCanvas)(const ANPBitmap*);
    void        (*deleteCanvas)(ANPCanvas*);

    void        (*save)(ANPCanvas*);
    void        (*restore)(ANPCanvas*);
    void        (*translate)(ANPCanvas*, float tx, float ty);
    void        (*scale)(ANPCanvas*, float sx, float sy);
    void        (*rotate)(ANPCanvas*, float degrees);
    void        (*skew)(ANPCanvas*, float kx, float ky);
    void        (*clipRect)(ANPCanvas*, const ANPRectF*);
    void        (*clipPath)(ANPCanvas*, const ANPPath*);
    
    void        (*drawColor)(ANPCanvas*, ANPColor);
    void        (*drawPaint)(ANPCanvas*, const ANPPaint*);
    void        (*drawRect)(ANPCanvas*, const ANPRectF*, const ANPPaint*);
    void        (*drawOval)(ANPCanvas*, const ANPRectF*, const ANPPaint*);
    void        (*drawPath)(ANPCanvas*, const ANPPath*, const ANPPaint*);
    void        (*drawText)(ANPCanvas*, const void* text, uint32_t byteLength,
                            float x, float y, const ANPPaint*);
    void       (*drawPosText)(ANPCanvas*, const void* text, uint32_t byteLength,
                               const float xy[], const ANPPaint*);
    void        (*drawBitmap)(ANPCanvas*, const ANPBitmap*, float x, float y,
                              const ANPPaint*);
    void        (*drawBitmapRect)(ANPCanvas*, const ANPBitmap*,
                                  const ANPRectI* src, const ANPRectF* dst,
                                  const ANPPaint*);
};

struct ANPWindowInterfaceV0 : ANPInterface {
    /** Given the window field from the NPWindow struct, and an optional rect
        describing the subset of the window that will be drawn to (may be null)
        return true if the bitmap for that window can be accessed, and if so,
        fill out the specified ANPBitmap to point to the window's pixels.
     
        When drawing is complete, call unlock(window)
     */
    bool    (*lockRect)(void* window, const ANPRectI* inval, ANPBitmap*);
    /** The same as lockRect, but takes a region instead of a rect to specify
        the area that will be changed/drawn.
     */
    bool    (*lockRegion)(void* window, const ANPRegion* inval, ANPBitmap*);
    /** Given a successful call to lock(window, inval, &bitmap), call unlock
        to release access to the pixels, and allow the browser to display the
        results. If lock returned false, unlock should not be called.
     */
    void    (*unlock)(void* window);
};

///////////////////////////////////////////////////////////////////////////////

enum ANPSampleFormats {
    kUnknown_ANPSamleFormat     = 0,
    kPCM16Bit_ANPSampleFormat   = 1,
    kPCM8Bit_ANPSampleFormat    = 2
};
typedef int32_t ANPSampleFormat;

/** The audio buffer is passed to the callback proc to request more samples.
    It is owned by the system, and the callback may read it, but should not
    maintain a pointer to it outside of the scope of the callback proc.
 */
struct ANPAudioBuffer {
    // RO - repeat what was specified in newTrack()
    int32_t     channelCount;
    // RO - repeat what was specified in newTrack()
    ANPSampleFormat  format;
    /** This buffer is owned by the caller. Inside the callback proc, up to
        "size" bytes of sample data should be written into this buffer. The
        address is only valid for the scope of a single invocation of the
        callback proc.
     */
    void*       bufferData;
    /** On input, specifies the maximum number of bytes that can be written
        to "bufferData". On output, specifies the actual number of bytes that
        the callback proc wrote into "bufferData".
     */
    uint32_t    size;
};

enum ANPAudioEvents {
    /** This event is passed to the callback proc when the audio-track needs
        more sample data written to the provided buffer parameter.
     */
    kMoreData_ANPAudioEvent = 0,
    /** This event is passed to the callback proc if the audio system runs out
        of sample data. In this event, no buffer parameter will be specified
        (i.e. NULL will be passed to the 3rd parameter).
     */
    kUnderRun_ANPAudioEvent = 1
};
typedef int32_t ANPAudioEvent;

/** Called to feed sample data to the track. This will be called in a separate
    thread. However, you may call trackStop() from the callback (but you
    cannot delete the track).
 
    For example, when you have written the last chunk of sample data, you can
    immediately call trackStop(). This will take effect after the current
    buffer has been played.
 
    The "user" parameter is the same value that was passed to newTrack()
 */
typedef void (*ANPAudioCallbackProc)(ANPAudioEvent event, void* user,
                                     ANPAudioBuffer* buffer);

struct ANPAudioTrack;   // abstract type for audio tracks

struct ANPAudioTrackInterfaceV0 : ANPInterface {
    /*  Create a new audio track, or NULL on failure.
     */
    ANPAudioTrack*  (*newTrack)(uint32_t sampleRate,    // sampling rate in Hz
                                ANPSampleFormat,
                                int channelCount,       // MONO=1, STEREO=2
                                ANPAudioCallbackProc,
                                void* user);
    void (*deleteTrack)(ANPAudioTrack*);

    void (*start)(ANPAudioTrack*);
    void (*pause)(ANPAudioTrack*);
    void (*stop)(ANPAudioTrack*);
    /** Returns true if the track is not playing (e.g. pause or stop was called,
        or start was never called.
     */
    bool (*isStopped)(ANPAudioTrack*);
};

///////////////////////////////////////////////////////////////////////////////
// HandleEvent

enum ANPEventTypes {
    kNull_ANPEventType  = 0,
    kKey_ANPEventType   = 1,
    kTouch_ANPEventType = 2,
    kDraw_ANPEventType  = 3,
};
typedef int32_t ANPEventType;

enum ANPKeyActions {
    kDown_ANPKeyAction  = 0,
    kUp_ANPKeyAction    = 1,
};
typedef int32_t ANPKeyAction;

#include "ANPKeyCodes.h"
typedef int32_t ANPKeyCode;

enum ANPKeyModifiers {
    kAlt_ANPKeyModifier     = 1 << 0,
    kShift_ANPKeyModifier   = 1 << 1,
};
// bit-field containing some number of ANPKeyModifier bits
typedef uint32_t ANPKeyModifier;

enum ANPTouchActions {
    kDown_ANPTouchAction  = 0,
    kUp_ANPTouchAction    = 1,
};
typedef int32_t ANPTouchAction;

struct ANPDrawContext {
    ANPDrawingModel model;
    // relative to (0,0) in top-left of your plugin
    ANPRectI        clip;
    // use based on the value in model
    union {
        ANPBitmap   bitmap;
    } data;
};

/* This is what is passed to NPP_HandleEvent() */
struct ANPEvent {
    uint32_t        inSize;  // size of this struct in bytes
    ANPEventType    eventType;
    // use based on the value in eventType
    union {
        struct {
            ANPKeyAction    action;
            ANPKeyCode      nativeCode;
            int32_t         virtualCode;    // windows virtual key code
            ANPKeyModifier  modifiers;
            int32_t         repeatCount;    // 0 for initial down (or up)
            int32_t         unichar;        // 0 if there is no value
        } key;
        struct {
            ANPTouchAction  action;
            ANPKeyModifier  modifiers;
            int32_t         x;  // relative to your "window" (0...width)
            int32_t         y;  // relative to your "window" (0...height)
        } touch;
        ANPDrawContext  drawContext;
        int32_t         other[8];
    } data;
};

#endif

