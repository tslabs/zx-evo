/**
* This source code ("the Software")is provided by Bridgetek Pte Ltd
* ("Bridgetek")subject to the licence terms set out
*   http://brtchip.com/BRTSourceCodeLicenseAgreement/ ("the Licence Terms").
* You must read the Licence Terms before downloading or using the Software.
* By installing or using the Software you agree to the Licence Terms. If you
* do not agree to the Licence Terms then do not download or use the Software.
*
* Without prejudice to the Licence Terms, here is a summary of some of the key
* terms of the Licence Terms (and in the event of any conflict between this
* summary and the Licence Terms then the text of the Licence Terms will
* prevail).
*
* The Software is provided "as is".
* There are no warranties (or similar)in relation to the quality of the
* Software. You use it at your own risk.
* The Software should not be used in, or for, any medical device, system or
* appliance. There are exclusions of Bridgetek liability for certain types of loss
* such as: special loss or damage; incidental loss or damage; indirect or
* consequential loss or damage; loss of income; loss of business; loss of
* profits; loss of revenue; loss of contracts; business interruption; loss of
* the use of money or anticipated savings; loss of information; loss of
* opportunity; loss of goodwill or reputation; and/or loss of, damage to or
* corruption of data.
* There is a monetary cap on Bridgetek's liability.
* The Software may have subsequently been amended by another user and then
* distributed by that other user ("Adapted Software").  If so that user may
* have additional licence terms that apply to those amendments. However, Bridgetek
* has no liability in relation to those amendments.
*/

#ifndef EVE_CO_DL__H
#define EVE_CO_DL__H

#include "EVE_CoCmd.h"

/* 

The purpose of this header is to provide a simplified interface to display list instructions.
The functions do not match 1:1 with the display list instructions. Some instructions are combined 
to simplify compatibility between platforms. (For example, BITMAP_SIZE and BITMAP_SIZE_H.)
All functions write to the display list through EVE_CoCmd_dl.

If EVE_DL_OPTIMIZE is set to 1 in EVE_Config, these functions will ignore duplicate calls.
To bypass optmization, call `EVE_CoCmd_dl` directly (within a subroutine or saved context.)

If ESD_DL_END_PRIMITIVE is set to 0 in EVE_Config, the END() instruction will be avoided.

Compatibility:
- EVE_CoDl_vertexFormat and EVE_CoDl_vertex2f implement fallback functionality for
  VERTEX_FORMAT, VERTEX_TRANSLATE_X, and VERTEX_TRANSLATE_Y on FT80X series.
- EVE_CoDl_bitmapSize calls BITMAP_SIZE_H on FT81X series and higher.
- EVE_CoDl_bitmapLayout calls BITMAP_LAYOUT_H on FT81X series and higher.

*/

#define EVE_VERTEX2F_MIN -16384L
#define EVE_VERTEX2F_MAX 16383L
#define EVE_VERTEX2II_MIN 0UL
#define EVE_VERTEX2II_MAX 511UL
EVE_HAL_EXPORT void EVE_CoDlImpl_resetDlState(EVE_HalContext *phost);
EVE_HAL_EXPORT void EVE_CoDlImpl_resetCoState(EVE_HalContext *phost);

static inline void EVE_CoDl_display(EVE_HalContext *phost)
{
	EVE_CoCmd_dl(phost, DISPLAY());
}

/* Fixed point vertex with subprecision depending on current vertex format */
ESD_FUNCTION(EVE_CoDl_vertex2f, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(x, Type = int16_t)
ESD_PARAMETER(y, Type = int16_t)
static inline void EVE_CoDl_vertex2f(EVE_HalContext *phost, int16_t x, int16_t y)
{
#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID < EVE_FT810)
	{
		/* Compatibility */
		x <<= EVE_DL_STATE.VertexFormat; /* 4 - frac */
		y <<= EVE_DL_STATE.VertexFormat; /* 4 - frac */
		x += EVE_DL_STATE.VertexTranslateX;
		y += EVE_DL_STATE.VertexTranslateY;
	}
#endif
	EVE_CoCmd_dl(phost, VERTEX2F(x, y));
}

#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
/* Compatibility for FT80X series */
EVE_HAL_EXPORT void EVE_CoDlImpl_vertex2ii_translate(EVE_HalContext *phost, uint16_t x, uint16_t y, uint8_t handle, uint8_t cell);
#endif

ESD_FUNCTION(EVE_CoDl_vertex2ii, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(x, Type = uint16_t)
ESD_PARAMETER(y, Type = uint16_t)
ESD_PARAMETER(handle, Type = uint8_t)
ESD_PARAMETER(cell, Type = uint8_t)
inline static void EVE_CoDl_vertex2ii(EVE_HalContext *phost, uint16_t x, uint16_t y, uint8_t handle, uint8_t cell)
{
#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID < EVE_FT810 && (EVE_DL_STATE.VertexTranslateX || EVE_DL_STATE.VertexTranslateY))
	{
		/* Compatibility for FT80X series */
		EVE_CoDlImpl_vertex2ii_translate(phost, x, y, handle, cell);
	}
	else
#endif
	{
		EVE_CoCmd_dl(phost, VERTEX2II(x, y, handle, cell));
	}
}

inline static void EVE_CoDl_bitmapSource(EVE_HalContext *phost, uint32_t addr)
{
	EVE_CoCmd_dl(phost, BITMAP_SOURCE(addr));
}

inline static void EVE_CoDl_bitmapSource_ex(EVE_HalContext *phost, uint32_t addr, bool flash)
{
	EVE_CoCmd_dl(phost, BITMAP_SOURCE2(flash, addr));
}

/* Specify clear color RGB */
ESD_FUNCTION(EVE_CoDl_clearColorRgb_ex, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(c, Type = rgb32_t, DisplayName = "Color")
inline static void EVE_CoDl_clearColorRgb_ex(EVE_HalContext *phost, uint32_t c)
{
	EVE_CoCmd_dl(phost, CLEAR_COLOR_RGB(0, 0, 0) | (c & 0xFFFFFF));
}

inline static void EVE_CoDl_clearColorRgb(EVE_HalContext *phost, uint8_t r, uint8_t g, uint8_t b)
{
	EVE_CoCmd_dl(phost, CLEAR_COLOR_RGB(r, g, b));
}

/* Specify clear alpha channel */
ESD_FUNCTION(EVE_CoDl_clearColorA, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(alpha, Type = uint8_t, DisplayName = "Alpha", Default = 255, Min = 0, Max = 255)
inline static void EVE_CoDl_clearColorA(EVE_HalContext *phost, uint8_t alpha)
{
	EVE_CoCmd_dl(phost, CLEAR_COLOR_A(alpha));
}

/* Specify clear color: Alpha (bits 31:24) + RGB (bits 23:0) */
ESD_FUNCTION(EVE_CoDl_clearColorArgb_ex, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(c, Type = argb32_t, DisplayName = "Color")
inline static void EVE_CoDl_clearColorArgb_ex(EVE_HalContext *phost, uint32_t c)
{
	EVE_CoDl_clearColorRgb_ex(phost, c);
	EVE_CoDl_clearColorA(phost, c >> 24);
}

/* Set current tag. Must be returned to 255 after usage, to ensure next widgets don't draw with invalid tag */
ESD_FUNCTION(EVE_CoDl_tag, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(s, Type = uint8_t, DisplayName = "Tag", Default = 255, Min = 0, Max = 255)
inline static void EVE_CoDl_tag(EVE_HalContext *phost, uint8_t s)
{
	EVE_CoCmd_dl(phost, TAG(s));
}

/* Specify color RGB */
ESD_FUNCTION(EVE_CoDl_colorRgb_ex, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(c, Type = rgb32_t, DisplayName = "Color")
inline static void EVE_CoDl_colorRgb_ex(EVE_HalContext *phost, uint32_t c)
{
	uint32_t rgb = c & 0xFFFFFF;
#if EVE_DL_OPTIMIZE
	if (rgb != EVE_DL_STATE.ColorRGB)
	{
#endif
		EVE_CoCmd_dl(phost, COLOR_RGB(0, 0, 0) | (c & 0xFFFFFF));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.ColorRGB = rgb;
	}
#endif
}

inline static void EVE_CoDl_colorRgb(EVE_HalContext *phost, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
	EVE_CoDl_colorRgb_ex(phost, rgb);
}

/* Specify alpha channel */
ESD_FUNCTION(EVE_CoDl_colorA, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(alpha, Type = uint8_t, DisplayName = "Alpha", Default = 255, Min = 0, Max = 255)
inline static void EVE_CoDl_colorA(EVE_HalContext *phost, uint8_t alpha)
{
#if EVE_DL_OPTIMIZE
	if (alpha != EVE_DL_STATE.ColorA)
	{
#endif
		EVE_CoCmd_dl(phost, COLOR_A(alpha));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.ColorA = alpha;
	}
#endif
}

/* Specify color: Alpha (bits 31:24) + RGB (bits 23:0) */
ESD_FUNCTION(EVE_CoDl_colorArgb_ex, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(c, Type = argb32_t, DisplayName = "Color")
inline static void EVE_CoDl_colorArgb_ex(EVE_HalContext *phost, uint32_t c)
{
	EVE_CoDl_colorRgb_ex(phost, c);
	EVE_CoDl_colorA(phost, c >> 24);
}

/* Specify bitmap handle, see BITMAP_HANDLE */
ESD_FUNCTION(EVE_CoDl_bitmapHandle, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(handle, Type = uint8_t, Min = 0, Max = 31)
inline static void EVE_CoDl_bitmapHandle(EVE_HalContext *phost, uint8_t handle)
{
#if EVE_DL_OPTIMIZE
	if (handle != EVE_DL_STATE.Handle)
	{
#endif
		EVE_CoCmd_dl(phost, BITMAP_HANDLE(handle));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.Handle = handle;
	}
#endif
}

/* Specify cell number for bitmap, see CELL */
ESD_FUNCTION(EVE_CoDl_cell, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(cell, Type = uint8_t, Min = 0, Max = 255)
inline static void EVE_CoDl_cell(EVE_HalContext *phost, uint8_t cell)
{
#if EVE_DL_OPTIMIZE
	if (cell != EVE_DL_STATE.Cell)
	{
#endif
		EVE_CoCmd_dl(phost, CELL(cell));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.Cell = cell;
	}
#endif
}

static inline void EVE_CoDl_bitmapLayout(EVE_HalContext *phost, uint8_t format, uint16_t linestride, uint16_t height)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
	if (EVE_CHIPID >= EVE_FT810)
	{
		EVE_CoCmd_dl(phost, BITMAP_LAYOUT_H(linestride >> 10, height >> 9));
	}
#endif
	EVE_CoCmd_dl(phost, BITMAP_LAYOUT(format, linestride, height));
}

static inline void EVE_CoDl_bitmapSize(EVE_HalContext *phost, uint8_t filter, uint8_t wrapx, uint8_t wrapy, uint16_t width, uint16_t height)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
	if (EVE_CHIPID >= EVE_FT810)
	{
		EVE_CoCmd_dl(phost, BITMAP_SIZE_H(width >> 9, height >> 9));
	}
#endif
	EVE_CoCmd_dl(phost, BITMAP_SIZE(filter, wrapx, wrapy, width, height));
}

static inline void EVE_CoDl_alphaFunc(EVE_HalContext *phost, uint8_t func, uint8_t ref)
{
	EVE_CoCmd_dl(phost, ALPHA_FUNC(func, ref));
}

static inline void EVE_CoDl_stencilFunc(EVE_HalContext *phost, uint8_t func, uint8_t ref, uint8_t mask)
{
	EVE_CoCmd_dl(phost, STENCIL_FUNC(func, ref, mask));
}

static inline void EVE_CoDl_blendFunc(EVE_HalContext *phost, uint8_t src, uint8_t dst)
{
	EVE_CoCmd_dl(phost, BLEND_FUNC(src, dst));
}

static inline void EVE_CoDl_blendFunc_default(EVE_HalContext *phost)
{
	EVE_CoDl_blendFunc(phost, SRC_ALPHA, ONE_MINUS_SRC_ALPHA); // TODO: Add default calls for all display list state instructions
}

static inline void EVE_CoDl_stencilOp(EVE_HalContext *phost, uint8_t sfail, uint8_t spass)
{
	EVE_CoCmd_dl(phost, STENCIL_OP(sfail, spass));
}

ESD_FUNCTION(EVE_CoDl_pointSize, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(size, Type = esd_int16_f4_t)
inline static void EVE_CoDl_pointSize(EVE_HalContext *phost, int16_t size)
{
#if EVE_DL_OPTIMIZE
	if (size != EVE_DL_STATE.PointSize)
	{
#endif
		EVE_CoCmd_dl(phost, POINT_SIZE(size));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.PointSize = size;
	}
#endif
}

ESD_FUNCTION(EVE_CoDl_lineWidth, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(width, Type = esd_int16_f4_t)
inline static void EVE_CoDl_lineWidth(EVE_HalContext *phost, int16_t width)
{
#if EVE_DL_OPTIMIZE
	if (width != EVE_DL_STATE.LineWidth)
	{
#endif
		EVE_CoCmd_dl(phost, LINE_WIDTH(width));
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.LineWidth = width;
	}
#endif
}

static inline void EVE_CoDl_clearStencil(EVE_HalContext *phost, uint8_t s)
{
	EVE_CoCmd_dl(phost, CLEAR_STENCIL(s));
}

static inline void EVE_CoDl_clearTag(EVE_HalContext *phost, uint8_t s)
{
	EVE_CoCmd_dl(phost, CLEAR_TAG(s));
}

static inline void EVE_CoDl_stencilMask(EVE_HalContext *phost, uint8_t mask)
{
	EVE_CoCmd_dl(phost, STENCIL_MASK(mask));
}

static inline void EVE_CoDl_tagMask(EVE_HalContext *phost, bool mask)
{
	EVE_CoCmd_dl(phost, TAG_MASK(mask));
}

static inline void EVE_CoDl_bitmapTransformA(EVE_HalContext *phost, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_A(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformA_ex(EVE_HalContext *phost, bool p, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_A_EXT(p, v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformB(EVE_HalContext *phost, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_B(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformB_ex(EVE_HalContext *phost, bool p, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_B_EXT(p, v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformC(EVE_HalContext *phost, uint32_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_C(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformD(EVE_HalContext *phost, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_D(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformD_ex(EVE_HalContext *phost, bool p, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_D_EXT(p, v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformE(EVE_HalContext *phost, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_E(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformE_ex(EVE_HalContext *phost, bool p, uint16_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_E_EXT(p, v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransformF(EVE_HalContext *phost, uint32_t v)
{
	EVE_CoCmd_dl(phost, BITMAP_TRANSFORM_F(v));
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransform_ex(EVE_HalContext *phost, bool p, uint16_t a, uint16_t b, uint32_t c, uint16_t d, uint16_t e, uint32_t f)
{
	EVE_CoDl_bitmapTransformA_ex(phost, p, a);
	EVE_CoDl_bitmapTransformB_ex(phost, p, b);
	EVE_CoDl_bitmapTransformC(phost, c);
	EVE_CoDl_bitmapTransformD_ex(phost, p, d);
	EVE_CoDl_bitmapTransformE_ex(phost, p, e);
	EVE_CoDl_bitmapTransformF(phost, f);
#if EVE_DL_OPTIMIZE
	EVE_DL_STATE.BitmapTransform = true;
#endif
}

static inline void EVE_CoDl_bitmapTransform_identity(EVE_HalContext *phost)
{
#if EVE_DL_OPTIMIZE
	if (EVE_DL_STATE.BitmapTransform)
	{
		/* Setting matrix can be skipped if already identity, since it's a no-op */
#endif
		EVE_CoDl_bitmapTransformA_ex(phost, 0, 256);
		EVE_CoDl_bitmapTransformB_ex(phost, 0, 0);
		EVE_CoDl_bitmapTransformC(phost, 0);
		EVE_CoDl_bitmapTransformD_ex(phost, 0, 0);
		EVE_CoDl_bitmapTransformE_ex(phost, 0, 256);
		EVE_CoDl_bitmapTransformF(phost, 0);
#if EVE_DL_OPTIMIZE
		EVE_DL_STATE.BitmapTransform = false;
	}
#endif
}

static inline void EVE_CoDl_scissorXY(EVE_HalContext *phost, uint16_t x, uint16_t y)
{
#if EVE_DL_OPTIMIZE && EVE_DL_CACHE_SCISSOR
	if (EVE_DL_STATE.ScissorX != x || EVE_DL_STATE.ScissorY != y)
#endif
	{
		EVE_CoCmd_dl(phost, SCISSOR_XY(x, y));
#if EVE_DL_CACHE_SCISSOR
		EVE_DL_STATE.ScissorX = x;
		EVE_DL_STATE.ScissorY = y;
#endif
	}
}

static inline void EVE_CoDl_scissorSize(EVE_HalContext *phost, uint16_t width, uint16_t height)
{
#if EVE_DL_OPTIMIZE && EVE_DL_CACHE_SCISSOR
	if (EVE_DL_STATE.ScissorWidth != width || EVE_DL_STATE.ScissorHeight != height)
#endif
	{
		EVE_CoCmd_dl(phost, SCISSOR_SIZE(width, height));
#if EVE_DL_CACHE_SCISSOR
		EVE_DL_STATE.ScissorWidth = width;
		EVE_DL_STATE.ScissorHeight = height;
#endif
	}
}

static inline void EVE_CoDl_call(EVE_HalContext *phost, uint16_t dest)
{
	EVE_CoCmd_dl(phost, CALL(dest));
}

static inline void EVE_CoDl_jump(EVE_HalContext *phost, uint16_t dest)
{
	EVE_CoCmd_dl(phost, JUMP(dest));
}

ESD_FUNCTION(EVE_CoDl_begin, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(prim, Type = uint8_t)
static inline void EVE_CoDl_begin(EVE_HalContext *phost, uint8_t prim)
{
#if EVE_DL_OPTIMIZE
	/* For continuous primitives, reset the active primitive. */
	uint8_t oldPrim = phost->DlPrimitive;
	switch (oldPrim)
	{
	case LINE_STRIP:
	case EDGE_STRIP_R:
	case EDGE_STRIP_L:
	case EDGE_STRIP_A:
	case EDGE_STRIP_B:
		oldPrim = 0;
		break;
	default:
		break;
	}
	if (prim != oldPrim)
	{
#endif
		EVE_CoCmd_dl(phost, BEGIN(prim));
#if EVE_DL_OPTIMIZE
		phost->DlPrimitive = prim;
	}
#endif
}

static inline void EVE_CoDl_colorMask(EVE_HalContext *phost, bool r, bool g, bool b, bool a)
{
	EVE_CoCmd_dl(phost, COLOR_MASK(r, g, b, a));
}

ESD_FUNCTION(EVE_CoDl_end, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
inline static void EVE_CoDl_end(EVE_HalContext *phost)
{
#if EVE_DL_END_PRIMITIVE || !EVE_DL_OPTIMIZE
#if EVE_DL_OPTIMIZE
	if (phost->DlPrimitive != 0)
	{
#endif
		EVE_CoCmd_dl(phost, END());
#if EVE_DL_OPTIMIZE
		phost->DlPrimitive = 0;
	}
#endif
#endif
}

/* Save EVE context, see SAVE_CONTEXT */
ESD_FUNCTION(EVE_CoDl_saveContext, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
inline static void EVE_CoDl_saveContext(EVE_HalContext *phost)
{
#if (EVE_DL_OPTIMIZE) || (EVE_DL_CACHE_SCISSOR) || (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
	uint8_t nextState;
#endif
	EVE_CoCmd_dl(phost, SAVE_CONTEXT());
#if (EVE_DL_OPTIMIZE) || (EVE_DL_CACHE_SCISSOR) || (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
	nextState = (phost->DlStateIndex + 1) & EVE_DL_STATE_STACK_MASK;
	phost->DlState[nextState] = phost->DlState[phost->DlStateIndex];
	phost->DlStateIndex = nextState;
#endif
}

/* Restore EVE context, see RESTORE_CONTEXT */
ESD_FUNCTION(EVE_CoDl_restoreContext, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
inline static void EVE_CoDl_restoreContext(EVE_HalContext *phost)
{
	EVE_CoCmd_dl(phost, RESTORE_CONTEXT());
#if (EVE_DL_OPTIMIZE) || (EVE_DL_CACHE_SCISSOR) || (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
	phost->DlStateIndex = (phost->DlStateIndex - 1) & EVE_DL_STATE_STACK_MASK;
#endif
}

static inline void EVE_CoDl_return(EVE_HalContext *phost)
{
	EVE_CoCmd_dl(phost, RETURN());
}

static inline void EVE_CoDl_macro(EVE_HalContext *phost, uint16_t m)
{
	EVE_CoCmd_dl(phost, MACRO(m));
}

ESD_FUNCTION(EVE_CoDl_clear, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(c, Type = bool, DisplayName = "Clear Color")
ESD_PARAMETER(s, Type = bool, DisplayName = "Clear Stencil")
ESD_PARAMETER(t, Type = bool, DisplayName = "Clear Tag")
static inline void EVE_CoDl_clear(EVE_HalContext *phost, bool c, bool s, bool t)
{
	EVE_CoCmd_dl(phost, CLEAR(c, s, t));
}

static inline void EVE_CoDl_vertexFormat(EVE_HalContext *phost, uint8_t frac)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_FT810)
	{
#if EVE_DL_OPTIMIZE
		if (frac != EVE_DL_STATE.VertexFormat)
		{
#endif
			EVE_CoCmd_dl(phost, VERTEX_FORMAT(frac));
#if EVE_DL_OPTIMIZE
			EVE_DL_STATE.VertexFormat = frac;
		}
#endif
	}
	else
#endif
	{
#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
		/* Compatibility for FT80X series */
		EVE_DL_STATE.VertexFormat = (4 - frac);
#endif
	}
}

/* Set palette source, see PALETTE_SOURCE command */
ESD_FUNCTION(EVE_CoDl_paletteSource, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(addr, Type = uint32_t, Min = 0)
inline static void EVE_CoDl_paletteSource(EVE_HalContext *phost, uint32_t addr)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810)
	if (EVE_CHIPID >= EVE_FT810)
	{
#if EVE_DL_OPTIMIZE
		if (addr != EVE_DL_STATE.PaletteSource)
		{
#endif
			EVE_CoCmd_dl(phost, PALETTE_SOURCE(addr));
#if EVE_DL_OPTIMIZE
			EVE_DL_STATE.PaletteSource = addr;
		}
#endif
	}
#endif
}

static inline void EVE_CoDl_vertexTranslateX(EVE_HalContext *phost, int16_t x)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_FT810)
	{
		EVE_CoCmd_dl(phost, VERTEX_TRANSLATE_X(x));
	}
	else
#endif
	{
#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
		/* Compatibility for FT80X series */
		EVE_DL_STATE.VertexTranslateX = x;
#endif
	}
}

static inline void EVE_CoDl_vertexTranslateY(EVE_HalContext *phost, int16_t y)
{
#if (EVE_SUPPORT_CHIPID >= EVE_FT810) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_FT810)
	{
		EVE_CoCmd_dl(phost, VERTEX_TRANSLATE_Y(y));
	}
	else
#endif
	{
#if (EVE_SUPPORT_CHIPID < EVE_FT810) || defined(EVE_MULTI_TARGET)
		/* Compatibility for FT80X series */
		EVE_DL_STATE.VertexTranslateY = y;
#endif
	}
}

static inline void EVE_CoDl_nop(EVE_HalContext *phost)
{
#if (EVE_SUPPORT_CHIPID >= EVE_BT815) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_BT815)
	{
		EVE_CoCmd_dl(phost, NOP());
	}
#endif
}

static inline void EVE_CoDl_bitmapExtFormat(EVE_HalContext *phost, uint16_t format)
{
#if (EVE_SUPPORT_CHIPID >= EVE_BT815) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_BT815)
	{
		EVE_CoCmd_dl(phost, BITMAP_EXT_FORMAT(format));
	}
#endif
}

static inline void EVE_CoDl_bitmapSwizzle(EVE_HalContext *phost, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
#if (EVE_SUPPORT_CHIPID >= EVE_BT815) || defined(EVE_MULTI_TARGET)
	if (EVE_CHIPID >= EVE_BT815)
	{
		EVE_CoCmd_dl(phost, BITMAP_SWIZZLE(r, g, b, a));
	}
#endif
}

/* Fixed point vertex using 4 bits subprecision */
ESD_FUNCTION(EVE_CoDl_vertex2f_4, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(x, Type = esd_int16_f4_t)
ESD_PARAMETER(y, Type = esd_int16_f4_t)
inline static void EVE_CoDl_vertex2f_4(EVE_HalContext *phost, int16_t x, int16_t y)
{
	if (EVE_CHIPID >= EVE_FT810 && (x > EVE_VERTEX2F_MAX || y > EVE_VERTEX2F_MAX)) /* Support display up to 2048 px */
	{
		EVE_CoDl_vertexFormat(phost, 3);
		EVE_CoDl_vertex2f(phost, x >> 1, y >> 1);
	}
	else
	{
		EVE_CoDl_vertexFormat(phost, 4);
		EVE_CoDl_vertex2f(phost, x, y);
	}
}

/* Fixed point vertex using 2 bits subprecision */
ESD_FUNCTION(EVE_CoDl_vertex2f_2, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(x, Type = esd_int16_f2_t)
ESD_PARAMETER(y, Type = esd_int16_f2_t)
inline static void EVE_CoDl_vertex2f_2(EVE_HalContext *phost, int16_t x, int16_t y)
{
	EVE_CoDl_vertexFormat(phost, 2);
	EVE_CoDl_vertex2f(phost, x, y);
}

/* Fixed point vertex using 0 bits subprecision, or integer point vertex */
ESD_FUNCTION(EVE_CoDl_vertex2f_0, Type = void, Category = EveRenderFunctions, Inline)
ESD_PARAMETER(phost, Type = EVE_HalContext *, Default = Esd_GetHost, Hidden, Internal, Static) // PHOST
ESD_PARAMETER(x, Type = int16_t)
ESD_PARAMETER(y, Type = int16_t)
inline static void EVE_CoDl_vertex2f_0(EVE_HalContext *phost, int16_t x, int16_t y)
{
	EVE_CoDl_vertexFormat(phost, 0);
	EVE_CoDl_vertex2f(phost, x, y);
}

#endif /* EVE_CO_DL__H */

/* end of file */
