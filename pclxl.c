/* Parser for PCL XL, (c) 2012 Urs-Jakob Ruetschi */

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_OF_MEMORY 0
#define INVALID_ARGUMENT 0

#define PCLXL_STACK_SIZE 10
#define PCLXL_INPUT pclxl_input // return next byte or EOF on end-of-file
#define MAX(x,y) ((x)>(y)?(x):(y))

#include "pclxl.h"
#include "joblex.h"

enum pclxl_endian {
   PCLXL_ENDIAN_UNKNOWN = 0,
   PCLXL_ENDIAN_LITTLE = 0xCAFE, // low byte first
   PCLXL_ENDIAN_BIG = 0xFECA     // high byte first
};

union pclxl_number {
   unsigned char ubyte;
   unsigned short uint16;
   signed short sint16;
   unsigned long uint32;
   signed long sint32;
   float real32;
   unsigned char raw[4];
};

struct pclxl_pair {
   union pclxl_number x;
   union pclxl_number y;
};

union pclxl_value {
   union pclxl_number number;
   struct pclxl_pair pair;
   // if needed: add pairs and boxes
};

struct pclxl_attr {
   int type;  // Appendix D: UINT16_XY, SINT16_BOX, UINT32_ARRAY, etc.
   int attrid;  // see Appendix E
   union pclxl_value value;
   // if needed: struct pclxl_array [*]array;
   // structure: { int size, pclxl_number *values }
};

struct pclxl_stack {
   struct pclxl_attr *slots;
   int size; // number of allocated slots
   int top; // -1 <= top < size
};

/// Stack functions:

void
pclxl_stack_init(struct pclxl_stack *sp, int size)
{
   assert(size > 0);
   sp->slots = calloc(size, sizeof(struct pclxl_attr));
   assert(sp->slots != OUT_OF_MEMORY);
   sp->top = -1; // empty
   sp->size = size;
}

void
pclxl_stack_free(struct pclxl_stack *sp)
{
   free(sp->slots);
   sp->slots = 0;
   sp->size = 0;
   sp->top = -1;
}

struct pclxl_attr *
pclxl_stack_top(struct pclxl_stack *sp)
{
   return sp->top >= 0 ? &(sp->slots[sp->top]) : 0;
}

struct pclxl_attr * // null if not found
pclxl_stack_find(struct pclxl_stack *sp, int attrid)
{
   int i;
   for (i = 0; i <= sp->top; i++)
   {
      if (sp->slots[i].attrid == attrid)
      {
         return &sp->slots[i];
      }
   }
   return 0; // not found
}

void
pclxl_stack_push(
   struct pclxl_stack *sp,
   int type, union pclxl_value value, int attrid)
{
   assert(sp->top < sp->size - 1);

   struct pclxl_attr *slot;
   slot = &(sp->slots[++sp->top]);

   slot->type = type;
   slot->value = value;
   slot->attrid = attrid;
}

void
pclxl_stack_pop(struct pclxl_stack *sp)
{
   assert(sp->top >= 0);

   struct pclxl_attr *slot;
   slot = &(sp->slots[sp->top--]);

   // Paranoia: override popped slot with zeros
   memset(slot, 0, sizeof(struct pclxl_attr));
}

void
pclxl_stack_clear(struct pclxl_stack *sp)
{
   while (sp->top >= 0)
   {
      pclxl_stack_pop(sp);
   }
   assert(sp->top == -1);
}

/// Log dump functions:

void
pclxl_write_attr(FILE *fp, struct pclxl_attr *attr)
{
   const char *s;
   // [ubyte] eInch Measure
   // [uint16_xy] 600 600 UnitsPerMeasure

   union pclxl_value value = attr->value;

   //fprintf(fp, " %s", pclxl_tag_name(attr->type), fp);

   switch (attr->type) {

   case PCLXL_TAG_UBYTE:
      if (s = pclxl_enum_name(attr->attrid, value.number.ubyte))
         fprintf(fp, " %s", s);
      else
         fprintf(fp, " %d", value.number.ubyte);
      break;
   case PCLXL_TAG_UINT16:
      fprintf(fp, " %d", value.number.uint16);
      break;
   case PCLXL_TAG_SINT16:
      fprintf(fp, " %d", value.number.sint16);
      break;
   case PCLXL_TAG_UINT32:
      fprintf(fp, " %ld", value.number.uint32);
      break;
   case PCLXL_TAG_SINT32:
      fprintf(fp, " %ld", value.number.sint32);
      break;
   case PCLXL_TAG_REAL32:
      fprintf(fp, " %.4f", value.number.real32);
      break;

   case PCLXL_TAG_UBYTE_XY:
      fprintf(fp, " %d %d", value.pair.x.ubyte, value.pair.y.ubyte);
      break;
   case PCLXL_TAG_UINT16_XY:
      fprintf(fp, " %d %d", value.pair.x.uint16, value.pair.y.uint16);
      break;
   case PCLXL_TAG_SINT16_XY:
      fprintf(fp, " %d %d", value.pair.x.sint16, value.pair.y.sint16);
      break;
   case PCLXL_TAG_UINT32_XY:
      fprintf(fp, " %ld %ld", value.pair.x.uint32, value.pair.y.uint32);
      break;
   case PCLXL_TAG_SINT32_XY:
      fprintf(fp, " %ld %ld", value.pair.x.sint32, value.pair.y.sint32);
      break;
   case PCLXL_TAG_REAL32_XY:
      fprintf(fp, " %.4f %.4f", value.pair.x.real32, value.pair.y.real32);
      break;

   // TODO Etc...?

   default:
      fprintf(fp, " #");
   }

   fprintf(fp, " %s\n", pclxl_attr_name(attr->attrid));
}

void
pclxl_logop(FILE *fp, int op, struct pclxl_stack *sp)
{
   if (op == PCLXL_TAG_DATA_LENGTH || op == PCLXL_TAG_DATA_LENGTH_BYTE)
   {
      fprintf(fp, "[data]\n");
   }
   else
   {
      int i;

      for (i = 0; i <= sp->top; i++)
      {
         struct pclxl_attr *attr = &sp->slots[i];
         pclxl_write_attr(fp, attr);
      }
 
      fprintf(fp, "%s\n", pclxl_tag_name(op));
   }
}

const char *
pclxl_tag_name(int tag)
{
   switch (tag) {

   // White Space:

   case PCLXL_TAG_NUL:          return "NUL";
   case PCLXL_TAG_HT:           return "HT";
   case PCLXL_TAG_LF:           return "LF";
   case PCLXL_TAG_VT:           return "VT";
   case PCLXL_TAG_FF:           return "FF";
   case PCLXL_TAG_CR:           return "CR";
   case PCLXL_TAG_SP:           return "SP";

   // Stream Headers:

   case PCLXL_TAG_BIND_ASCII:   return "Binding: ASCII";
   case PCLXL_TAG_BIND_BIN_BE:  return "Binding: binary, high byte first";
   case PCLXL_TAG_BIND_BIN_LE:  return "Binding: binary, low byte first";

   // Operators:

   case PCLXL_TAG_BEGIN_SESSION:        return "BeginSession";
   case PCLXL_TAG_END_SESSION:          return "EndSession";

   case PCLXL_TAG_BEGIN_PAGE:           return "BeginPage";
   case PCLXL_TAG_END_PAGE:             return "EndPage";

   case PCLXL_TAG_COMMENT:              return "Comment";

   case PCLXL_TAG_OPEN_DATA_SOURCE:     return "OpenDataSource";
   case PCLXL_TAG_CLOSE_DATA_SOURCE:    return "CloseDataSource";

   case PCLXL_TAG_BEGIN_FONT_HEADER:    return "BeginFondHeader";
   case PCLXL_TAG_READ_FONT_HEADER:     return "ReadFontHeader";
   case PCLXL_TAG_END_FONT_HEADER:      return "EndFontHeader";

   case PCLXL_TAG_BEGIN_CHAR:           return "BeginChar";
   case PCLXL_TAG_READ_CHAR:            return "ReadChar";
   case PCLXL_TAG_END_CHAR:             return "EndChar";

   case PCLXL_TAG_REMOVE_FONT:          return "RemoveFont";

   case PCLXL_TAG_SET_CHAR_ATTRIBUTES:  return "SetCharAttributes";

   case PCLXL_TAG_BEGIN_STREAM:         return "BeginStream";
   case PCLXL_TAG_READ_STREAM:          return "ReadStream";
   case PCLXL_TAG_END_STREAM:           return "EndStream";
   case PCLXL_TAG_EXEC_STREAM:          return "ExecStream";
   case PCLXL_TAG_REMOVE_STREAM:        return "RemoveStream";

   case PCLXL_TAG_POP_GS:               return "PopGS";
   case PCLXL_TAG_PUSH_GS:              return "PushGS";

   case PCLXL_TAG_SET_CLIP_REPLACE:     return "SetClipReplace";
   case PCLXL_TAG_SET_BRUSH_SOURCE:     return "SetBrushSource";
   case PCLXL_TAG_SET_CHAR_ANGLE:       return "SetCharAngle";
   case PCLXL_TAG_SET_CHAR_SCALE:       return "SetCharScale";
   case PCLXL_TAG_SET_CHAR_SHEAR:       return "SetCharShear";
   case PCLXL_TAG_SET_CLIP_INTERSECT:   return "SetClipIntersect";
   case PCLXL_TAG_SET_CLIP_RECTANGLE:   return "SetClipRectangle";
   case PCLXL_TAG_SET_CLIP_TO_PAGE:     return "SetClipToPage";
   case PCLXL_TAG_SET_COLOR_SPACE:      return "SetColorSpace";
   case PCLXL_TAG_SET_CURSOR:           return "SetCursor";
   case PCLXL_TAG_SET_CURSOR_REL:       return "SetCursorRel";
   case PCLXL_TAG_SET_HALFTONE_METHOD:  return "SetHalftoneMethod";
   case PCLXL_TAG_SET_FILL_MODE:        return "SetFillMode";
   case PCLXL_TAG_SET_FONT:             return "SetFont";
   case PCLXL_TAG_SET_LINE_DASH:        return "SetLineDash";
   case PCLXL_TAG_SET_LINE_CAP:         return "SetLineCap";
   case PCLXL_TAG_SET_LINE_JOIN:        return "SetLineJoin";
   case PCLXL_TAG_SET_MITER_LIMIT:      return "SetMiterLimit";
   case PCLXL_TAG_SET_PAGE_DEFAULT_CTM: return "SetPageDefaultCTM";
   case PCLXL_TAG_SET_PAGE_ORIGIN:      return "SetPageOrigin";
   case PCLXL_TAG_SET_PAGE_ROTATION:    return "SetPageRotation";
   case PCLXL_TAG_SET_PAGE_SCALE:       return "SetPageScale";
   case PCLXL_TAG_SET_PATTERN_TX_MODE:  return "SetPatternTxMode";
   case PCLXL_TAG_SET_PEN_SOURCE:       return "SetPenSource";
   case PCLXL_TAG_SET_PEN_WIDTH:        return "SetPenWidth";
   case PCLXL_TAG_SET_ROP:              return "SetROP";
   case PCLXL_TAG_SET_SOURCE_TX_MODE:   return "SetSourceTxMode";
   case PCLXL_TAG_SET_CHAR_BOLD_VALUE:  return "SetCharBoldValue";
   case PCLXL_TAG_SET_CLIP_MODE:        return "SetClipMode";
   case PCLXL_TAG_SET_PATH_TO_CLIP:     return "SetPathToClip";
   case PCLXL_TAG_SET_CHAR_SUB_MODE:    return "SetCharSubMode";

   case PCLXL_TAG_CLOSE_SUB_PATH:       return "CloseSubPath";
   case PCLXL_TAG_NEW_PATH:             return "NewPath";
   case PCLXL_TAG_PAINT_PATH:           return "PaintPath";

   case PCLXL_TAG_ARC_PATH:             return "ArcPath";
   case PCLXL_TAG_BEZIER_PATH:          return "BezierPath";
   case PCLXL_TAG_BEZIER_REL_PATH:      return "BezierRelPath";
   case PCLXL_TAG_CHORD:                return "Chord";
   case PCLXL_TAG_CHORD_PATH:           return "ChordPath";
   case PCLXL_TAG_ELLIPSE:              return "Ellipse";
   case PCLXL_TAG_ELLIPSE_PATH:         return "EllipsePath";
   case PCLXL_TAG_LINE_PATH:            return "LinePath";
   case PCLXL_TAG_LINE_REL_PATH:        return "LineRelPath";
   case PCLXL_TAG_PIE:                  return "Pie";
   case PCLXL_TAG_PIE_PATH:             return "PiePath";
   case PCLXL_TAG_RECTANGLE:            return "Rectangle";
   case PCLXL_TAG_RECTANGLE_PATH:       return "RectanglePath";
   case PCLXL_TAG_ROUND_RECTANGLE:      return "RoundRectangle";
   case PCLXL_TAG_ROUND_RECTANGLE_PATH: return "RoundRectanglePath";

   case PCLXL_TAG_TEXT:                 return "Text";
   case PCLXL_TAG_TEXT_PATH:            return "TextPath";

   case PCLXL_TAG_BEGIN_IMAGE:          return "BeginImage";
   case PCLXL_TAG_READ_IMAGE:           return "ReadImage";
   case PCLXL_TAG_END_IMAGE:            return "EndImage";

   case PCLXL_TAG_BEGIN_RAST_PATTERN:   return "BeginRastPattern";
   case PCLXL_TAG_READ_RAST_PATTERN:    return "ReadRastPattern";
   case PCLXL_TAG_END_RAST_PATTERN:     return "EndRastPattern";

   case PCLXL_TAG_BEGIN_SCAN:           return "BeginScan";
   case PCLXL_TAG_END_SCAN:             return "EndScan";
   case PCLXL_TAG_SCAN_LINE_REL:        return "ScanLineRel";

   // Data Types:

   case PCLXL_TAG_UBYTE:                return "ubyte";
   case PCLXL_TAG_UINT16:               return "uint16";
   case PCLXL_TAG_UINT32:               return "uint32";
   case PCLXL_TAG_SINT16:               return "sint16";
   case PCLXL_TAG_SINT32:               return "sint32";
   case PCLXL_TAG_REAL32:               return "real32";

   case PCLXL_TAG_UBYTE_ARRAY:          return "ubyte_array";
   case PCLXL_TAG_UINT16_ARRAY:         return "uint16_array";
   case PCLXL_TAG_UINT32_ARRAY:         return "uint32_array";
   case PCLXL_TAG_SINT16_ARRAY:         return "sint16_array";
   case PCLXL_TAG_SINT32_ARRAY:         return "sint32_array";
   case PCLXL_TAG_REAL32_ARRAY:         return "real32_array";

   case PCLXL_TAG_UBYTE_XY:             return "ubyte_xy";
   case PCLXL_TAG_UINT16_XY:            return "uint16_xy";
   case PCLXL_TAG_UINT32_XY:            return "uint32";
   case PCLXL_TAG_SINT16_XY:            return "sint16_xy";
   case PCLXL_TAG_SINT32_XY:            return "sint32_xy";
   case PCLXL_TAG_REAL32_XY:            return "real32_xy";

   case PCLXL_TAG_UBYTE_BOX:            return "ubyte_box";
   case PCLXL_TAG_UINT16_BOX:           return "uint16_box";
   case PCLXL_TAG_UINT32_BOX:           return "uint32_box";
   case PCLXL_TAG_SINT16_BOX:           return "sint16_box";
   case PCLXL_TAG_SINT32_BOX:           return "sint32_box";
   case PCLXL_TAG_REAL32_BOX:           return "real32_box";

   // Attributes:

   case PCLXL_TAG_ATTR_UBYTE:           return "attr_ubyte";
   case PCLXL_TAG_ATTR_UINT16:          return "attr_uint16";

   // Embedded Data:

   case PCLXL_TAG_DATA_LENGTH:          return "dataLength";
   case PCLXL_TAG_DATA_LENGTH_BYTE:     return "dataLengthByte";

   // Not PCLXL:
   case EOF: return "EOF";
   }

   return "?tag?";
}

const char *
pclxl_attr_name(int attrid)
{
   switch (attrid) {
   case PCLXL_ATTR_PALETTE_DEPTH:           return "PaletteDepth";
   case PCLXL_ATTR_COLOR_SPACE:             return "ColorSpace";
   case PCLXL_ATTR_NULL_BRUSH:              return "NullBrush";
   case PCLXL_ATTR_NULL_PEN:                return "NullPen";
   case PCLXL_ATTR_PALETTE_DATA:            return "PaletteData";
   case PCLXL_ATTR_PATTERN_SELECT_ID:       return "PatternSelectID";
   case PCLXL_ATTR_GRAY_LEVEL:              return "GrayLevel";
   case PCLXL_ATTR_RGB_COLOR:               return "RGBColor";
   case PCLXL_ATTR_PATTERN_ORIGIN:          return "PatternOrigin";
   case PCLXL_ATTR_NEW_DESTINATION_SIZE:    return "NewDestinationSize";
   case PCLXL_ATTR_PRIMARY_ARRAY:           return "PrimaryArray";
   case PCLXL_ATTR_PRIMARY_DEPTH:           return "PrimaryDepth";
   case PCLXL_ATTR_DEVICE_MATRIX:           return "DeviceMatrix";
   case PCLXL_ATTR_DITHER_MATRIX_DATA_TYPE: return "DitherMatrixDataType";
   case PCLXL_ATTR_DITHER_ORIGIN:           return "DitherOrigin";
   case PCLXL_ATTR_MEDIA_SIZE:              return "MediaSize";
   case PCLXL_ATTR_MEDIA_SOURCE:            return "MediaSource";
   case PCLXL_ATTR_MEDIA_TYPE:              return "MediaType";
   case PCLXL_ATTR_ORIENTATION:             return "Orientation";
   case PCLXL_ATTR_PAGE_ANGLE:              return "PageAngle";
   case PCLXL_ATTR_PAGE_ORIGIN:             return "PageOrigin";
   case PCLXL_ATTR_PAGE_SCALE:              return "PageScale";
   case PCLXL_ATTR_ROP3:                    return "ROP3";
   case PCLXL_ATTR_TX_MODE:                 return "TxMode";
   case PCLXL_ATTR_CUSTOM_MEDIA_SIZE:       return "CustomMediaSize";
   case PCLXL_ATTR_CUSTOM_MEDIA_SIZE_UNITS: return "CustomMediaSizeUnits";
   case PCLXL_ATTR_PAGE_COPIES:             return "PageCopies";
   case PCLXL_ATTR_DITHER_MATRIX_SIZE:      return "DitherMatrixSize";
   case PCLXL_ATTR_DITHER_MATRIX_DEPTH:     return "DitherMatrixDepth";
   case PCLXL_ATTR_SIMPLEX_PAGE_MODE:       return "SimplexPageMode";
   case PCLXL_ATTR_DUPLEX_PAGE_MODE:        return "DuplexPageMode";
   case PCLXL_ATTR_DUPLEX_PAGE_SIDE:        return "DuplexPageSide";
   case PCLXL_ATTR_ARC_DIRECTION:           return "ArcDirection";
   case PCLXL_ATTR_BOUNDING_BOX:            return "BoundingBox";
   case PCLXL_ATTR_DASH_OFFSET:             return "DashOffset";
   case PCLXL_ATTR_ELLIPSE_DIMENSION:       return "EllipseDimension";
   case PCLXL_ATTR_END_POINT:               return "EndPoint";
   case PCLXL_ATTR_FILL_MODE:               return "FillMode";
   case PCLXL_ATTR_LINE_CAP_STYLE:          return "LineCapStyle";
   case PCLXL_ATTR_LINE_JOIN_STYLE:         return "LineJoinStyle";
   case PCLXL_ATTR_MITER_LENGTH:            return "MiterLength";
   case PCLXL_ATTR_LINE_DASH_STYLE:         return "LineDashStyle";
   case PCLXL_ATTR_PEN_WIDTH:               return "PenWidth";
   case PCLXL_ATTR_POINT:                   return "Point";
   case PCLXL_ATTR_NUMBER_OF_POINTS:        return "NumberOfPoints";
   case PCLXL_ATTR_SOLID_LINE:              return "SolidLine";
   case PCLXL_ATTR_START_POINT:             return "StartPoint";
   case PCLXL_ATTR_POINT_TYPE:              return "PointType";
   case PCLXL_ATTR_CONTROL_POINT_1:         return "ControlPoint1";
   case PCLXL_ATTR_CONTROL_POINT_2:         return "ControlPoint2";
   case PCLXL_ATTR_CLIP_REGION:             return "ClipRegion";
   case PCLXL_ATTR_CLIP_MODE:               return "ClipMode";
   case PCLXL_ATTR_COLOR_DEPTH:             return "ColorDepth";
   case PCLXL_ATTR_BLOCK_HEIGHT:            return "BlockHeight";
   case PCLXL_ATTR_COLOR_MAPPING:           return "ColorMapping";
   case PCLXL_ATTR_COMPRESS_MODE:           return "CompressMode";
   case PCLXL_ATTR_DESTINATION_BOX:         return "DestinationBox";
   case PCLXL_ATTR_DESTINATION_SIZE:        return "DestinationSize";
   case PCLXL_ATTR_PATTERN_PERSISTENCE:     return "PatternPersistence";
   case PCLXL_ATTR_PATTERN_DEFINE_ID:       return "PatternDefineID";
   case PCLXL_ATTR_SOURCE_HEIGHT:           return "SourceHeight";
   case PCLXL_ATTR_SOURCE_WIDTH:            return "SourceWidth";
   case PCLXL_ATTR_START_LINE:              return "StartLine";
   case PCLXL_ATTR_PAD_BYTES_MULTIPLE:      return "PadBytesMultiple";
   case PCLXL_ATTR_BLOCK_BYTE_LENGTH:       return "BlockByteLength";
   case PCLXL_ATTR_NUMBER_OF_SCAN_LINES:    return "NumberOfScanLines";
   case PCLXL_ATTR_COMMENT_DATA:            return "CommentData";
   case PCLXL_ATTR_DATA_ORG:                return "DataOrg";
   case PCLXL_ATTR_MEASURE:                 return "Measure";
   case PCLXL_ATTR_SOURCE_TYPE:             return "SourceType";
   case PCLXL_ATTR_UNITS_PER_MEASURE:       return "UnitsPerMeasure";
   case PCLXL_ATTR_STREAM_NAME:             return "StreamName";
   case PCLXL_ATTR_STREAM_DATA_LENGTH:      return "StreamDataLength";
   case PCLXL_ATTR_ERROR_REPORT:            return "ErrorReport";
   case PCLXL_ATTR_CHAR_ANGLE:              return "CharAngle";
   case PCLXL_ATTR_CHAR_CODE:               return "CharCode";
   case PCLXL_ATTR_CHAR_DATA_SIZE:          return "CharDataSize";
   case PCLXL_ATTR_CHAR_SCALE:              return "CharScale";
   case PCLXL_ATTR_CHAR_SHEAR:              return "CharShear";
   case PCLXL_ATTR_CHAR_SIZE:               return "CharSize";
   case PCLXL_ATTR_FONT_HEADER_LENGTH:      return "FontHeaderLength";
   case PCLXL_ATTR_FONT_NAME:               return "FontName";
   case PCLXL_ATTR_FONT_FORMAT:             return "FontFormat";
   case PCLXL_ATTR_SYMBOL_SET:              return "SymbolSet";
   case PCLXL_ATTR_TEXT_DATA:               return "TextData";
   case PCLXL_ATTR_CHAR_SUB_MODE_ARRAY:     return "CharSubModeArray";
   case PCLXL_ATTR_X_SPACING_DATA:          return "XSpacingData";
   case PCLXL_ATTR_Y_SPACING_DATA:          return "YSpacingData";
   case PCLXL_ATTR_CHAR_BOLD_VALUE:         return "CharBoldValue";
   }

   return "?attr?";
}

const char *
pclxl_enum_name(int attrid, int value)
{
   switch (attrid) {
   case PCLXL_ATTR_ARC_DIRECTION:
      switch (value) {
      case 0: return "eClockWise"; // 1.1
      case 1: return "eCounterClockWise"; // 1.1
      }
      break;
   case PCLXL_ATTR_CHAR_SUB_MODE_ARRAY:
      switch (value) {
      case 0: return "eNoSubstitution"; // 1.1
      case 1: return "eVerticalSubstitution"; // 1.1
      }
      break;
   case PCLXL_ATTR_CLIP_MODE:
   case PCLXL_ATTR_FILL_MODE:
      switch (value) {
      case 0: return "eNonZeroWinding"; // 1.1
      case 1: return "eEvenOdd"; // 1.1
      }
      break;
   case PCLXL_ATTR_CLIP_REGION:
      switch (value) {
      case 0: return "eInterior"; // 1.1
      case 1: return "eExterior"; // 1.1
      }
      break;
   case PCLXL_ATTR_COLOR_DEPTH:
      switch (value) {
      case 0: return "e1Bit"; // 1.1
      case 1: return "e4Bit"; // 1.1
      case 2: return "e8Bit"; // 1.1
      }
      break;
   case PCLXL_ATTR_COLOR_MAPPING:
      switch (value) {
      case 0: return "eDirectPixel"; // 1.1
      case 1: return "eIndexedPixel"; // 1.1
      }
      break;
   case PCLXL_ATTR_COLOR_SPACE:
      switch (value) {
      case 1: return "eGray"; // 1.1
      case 2: return "eRGB"; // 1.1
      }
      break;
   case PCLXL_ATTR_COMPRESS_MODE:
      switch (value) {
      case 0: return "eNoCompression"; // 1.1
      case 1: return "eRLECompression"; // 1.1
      case 2: return "eJPEGCompression"; // 2.0
      }
      break;
   case PCLXL_ATTR_DATA_ORG:
      switch (value) {
      case 0: return "eBinaryHighByteFirst"; // 1.1
      case 1: return "eBinaryLowByteFirst"; // 1.1
      }
      break;
   case PCLXL_ATTR_SOURCE_TYPE:
      switch (value) {
      case 0: return "eDefault"; // 1.1
      }
      break;
   case PCLXL_ATTR_DITHER_MATRIX_DATA_TYPE:
      switch (value) {
      case 0: return "eUByte"; // 1.1 always eUByte?
      case 1: return "eSByte"; // 1.1 unsure
      case 2: return "eUint16"; // 1.1 unsure
      case 3: return "eSint16"; // 1.1 unsure
      }
      break;
   case PCLXL_ATTR_DEVICE_MATRIX:
      switch (value) {
      case 0: return "eDeviceBest"; // 1.1
      }
      break;
   case PCLXL_ATTR_DUPLEX_PAGE_MODE:
      switch (value) {
      case 0: return "eDuplexHorizontalBinding"; // 1.1
      case 1: return "eDuplexVerticalBinding"; // 1.1
      }
      break;
   case PCLXL_ATTR_DUPLEX_PAGE_SIDE:
      switch (value) { 
      case 0: return "eFrontMediaSide"; // 1.1
      case 1: return "eBackMediaSide"; // 1.1
      }
      break;
   case PCLXL_ATTR_ERROR_REPORT:
      switch (value) {
      case 0: return "eNoReporting"; // 1.1
      case 1: return "eBackChannel"; // 1.1
      case 2: return "eErrorPage"; // 1.1
      case 3: return "eBakcChAndErrPage"; // 1.1
      case 4: return "eNWBackChannel"; // 2.0
      case 5: return "eNWErrorPage"; // 2.0
      case 6: return "eNWBackChAndErrPage"; // 2.0
      }
      break;
   // FILL_MODE: see CLIP_MODE
   case PCLXL_ATTR_LINE_CAP_STYLE:
      switch (value) {
      case 0: return "eButtCap"; // 1.1
      case 1: return "eRoundCap"; // 1.1
      case 2: return "eSquareCap"; // 1.1
      case 3: return "eTriangleCap"; // 1.1
      }
      break;
   case PCLXL_ATTR_LINE_JOIN_STYLE:
      switch (value) {
      case 0: return "eMiterJoin"; // 1.1
      case 1: return "eRoundJoin"; // 1.1
      case 2: return "eBevelJoin"; // 1.1
      case 3: return "eNoJoin"; // 1.1
      }
      break;
   case PCLXL_ATTR_MEASURE:
      switch (value) {
      case 0: return "eInch"; // 1.1
      case 1: return "eMillimeter"; // 1.1
      case 2: return "eTenthsOfAMillimeter"; // 1.1
      }
      break;
   case PCLXL_ATTR_MEDIA_SIZE:
      switch (value) {
      case 0: return "eLetterPaper";
      case 1: return "eLegalPaper";
      case 2: return "eA4Paper";
      case 3: return "eExecPaper";
      case 4: return "eLedgerPaper";
      case 5: return "eA3Paper";
      case 6: return "eCOM10Paper"; // 1.1
      case 7: return "eMonarchEnvelope"; // 1.1
      case 8: return "eC5Envelope"; // 1.1
      case 9: return "eDLEnvelope"; // 1.1
      case 10: return "eJB4Paper"; // 1.1
      case 11: return "eJB5Paper"; // 1.1
      case 12: return "eB5Envelope"; // 1.1
      // No 13 in Appendix G of PCL XL Feature Reference rev 2.2
      case 14: return "eJPostcard"; // 1.1
      case 15: return "eJDoublePostcard"; // 1.1
      case 16: return "eA5Paper"; // 1.1
      case 17: return "eA6Paper"; // 2.0
      case 18: return "eJB6Paper"; // 2.0
      }
      break;
   case PCLXL_ATTR_MEDIA_SOURCE:
      switch (value) {
      case 0: return "eDefaultSource"; // 1.1
      case 1: return "eAutoSelect"; // 1.1
      case 2: return "eManualFeed"; // 1.1
      case 3: return "eMultiPurposeTray"; // 1.1
      case 4: return "eUpperCassette"; // 1.1
      case 5: return "eLowerCassette"; // 1.1
      case 6: return "eEnvelopeTray"; // 1.1
      case 7: return "eThirdCassette"; // 2.0
      default: return "(ExternalTray)"; // 2.0
      }
      break;
//   case PCLXL_ATTR_MEDIA_DESTINATION:
//      switch (value) {
//      case 0: return "eDefaultDestination"; // 2.0
//      case 1: return "eFaceDownBin"; // 2.0
//      case 2: return "eFaceUpBin"; // 2.0
//      case 3: return "eJobOffsetBin"; // 2.0
//      default: return "(ExternalBin)"; // 2.0
//      }
//      break;
   case PCLXL_ATTR_ORIENTATION:
      switch (value) {
      case 0: return "ePortraitOrientation"; // 1.1
      case 1: return "eLandscapeOrientation"; // 1.1
      case 2: return "eReversePortrait"; // 1.1
      case 3: return "eReverseLandscape"; // 1.1
      }
      break;
   case PCLXL_ATTR_PATTERN_PERSISTENCE:
      switch (value) {
      case 0: return "eTempPattern"; // 1.1
      case 1: return "ePagePattern"; // 1.1
      case 2: return "eSessionPettern"; // 1.1
      }
      break;
   case PCLXL_ATTR_SYMBOL_SET:
      // See Appendix O. For example:
      // 14 is ISO-8859-1 aka Latin1
      // 327 is PC-851 aka Western
      // 18540 is Windings
      break;
   case PCLXL_ATTR_SIMPLEX_PAGE_MODE:
      switch (value) {
      case 0: return "eSimplexFrontSide"; // 1.1
      }
      break;
   case PCLXL_ATTR_TX_MODE:
      switch (value) {
      case 0: return "eOpaque"; // 1.1
      case 1: return "eTransparent"; // 1.1
      }
      break;
//   case PCLXL_ATTR_WRITING_MODE:
//      switch (value) {
//      case 0: return "eHorizontal"; // 2.0
//      case 1: return "eVertical"; // 2.0
//      }
//      break;
   }
   return 0;
}

/// Input functions:

union pclxl_value
pclxl_read_number(int type, enum pclxl_endian endian)
{
   int i, nbytes, value;
   union pclxl_value result;

   memset(&result, 0, sizeof(union pclxl_value));

   switch (type) {
   case PCLXL_TAG_UBYTE:
   case PCLXL_TAG_UBYTE_XY:
   case PCLXL_TAG_UBYTE_BOX:
   case PCLXL_TAG_UBYTE_ARRAY:
      nbytes = 1;
      break;
   case PCLXL_TAG_UINT16:
   case PCLXL_TAG_UINT16_XY:
   case PCLXL_TAG_UINT16_BOX:
   case PCLXL_TAG_UINT16_ARRAY:
   case PCLXL_TAG_SINT16:
   case PCLXL_TAG_SINT16_XY:
   case PCLXL_TAG_SINT16_BOX:
   case PCLXL_TAG_SINT16_ARRAY:
      nbytes = 2;
      break;
   case PCLXL_TAG_UINT32:
   case PCLXL_TAG_UINT32_XY:
   case PCLXL_TAG_UINT32_BOX:
   case PCLXL_TAG_UINT32_ARRAY:
   case PCLXL_TAG_SINT32:
   case PCLXL_TAG_SINT32_XY:
   case PCLXL_TAG_SINT32_BOX:
   case PCLXL_TAG_SINT32_ARRAY:
   case PCLXL_TAG_REAL32:
   case PCLXL_TAG_REAL32_XY:
   case PCLXL_TAG_REAL32_BOX:
   case PCLXL_TAG_REAL32_ARRAY:
      nbytes = 4;
      break;
   default:
      assert(INVALID_ARGUMENT);
   }

   switch (endian) {
   case PCLXL_ENDIAN_LITTLE:
      for (i = 0; i < nbytes; i++)
      {
         value = PCLXL_INPUT();
         if (value == EOF)
            fatal("Unexpected end-of-file while reading a number");
         result.number.raw[i] = (unsigned char) value;
      }
      break;
   case PCLXL_ENDIAN_BIG:
      for (i = nbytes - 1; i >= 0; i--)
      {
         value = PCLXL_INPUT();
         if (value == EOF)
            fatal("Unexpected end-of-file while reading a number");
         result.number.raw[i] = (unsigned char) value;
      }
      break;
   default:
      assert(INVALID_ARGUMENT);
   }

   return result;
}

union pclxl_value
pclxl_read_pair(int type, enum pclxl_endian endian)
{
   union pclxl_value result;

   result.pair.x = pclxl_read_number(type, endian).number;
   result.pair.y = pclxl_read_number(type, endian).number;

   return result;
}

void
pclxl_skip_bbox(int type, enum pclxl_endian endian)
{
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
   (void) pclxl_read_number(type, endian);
}

void
pclxl_skip_array(int type, enum pclxl_endian endian)
{
   // TODO check EOF
   union pclxl_value value;
   unsigned long size;
   int i, sizetype = PCLXL_INPUT();

   switch (sizetype) {
   case PCLXL_TAG_UBYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      size = value.number.ubyte;
      break;
   case PCLXL_TAG_UINT16:
      value = pclxl_read_number(PCLXL_TAG_UINT16, endian);
      size = value.number.uint16;
      break;
   case PCLXL_TAG_UINT32:
      value = pclxl_read_number(PCLXL_TAG_UINT32, endian);
      size = value.number.uint32;
      break;
   default:
      fatal("Invalid data type for array size: %d", sizetype);
   }

   for (i = 0; i < size; i++)
   {
      (void) pclxl_read_number(type, endian);
   }
}

void
pclxl_skip_data(int type, enum pclxl_endian endian)
{
   unsigned long i, nbytes;
   union pclxl_value value;

   switch (type) {
   case PCLXL_TAG_DATA_LENGTH_BYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      nbytes = value.number.ubyte;
      break;
   case PCLXL_TAG_DATA_LENGTH:
      value = pclxl_read_number(PCLXL_TAG_UINT32, endian);
      nbytes = value.number.uint32;
      break;
   default:
      fatal("Invalid data length tag: %d", type);
   }

   for (i = 0; i < nbytes; i++)
   {
      int value = PCLXL_INPUT();
      if (value == EOF)
         fatal("Unexpected end-of-file while reading embedded data");
   }
}

int
pclxl_read_attrid(enum pclxl_endian endian)
{
   union pclxl_value value;
   int tag = PCLXL_INPUT();

   switch (tag) {
   case PCLXL_TAG_ATTR_UBYTE:
      value = pclxl_read_number(PCLXL_TAG_UBYTE, endian);
      return value.number.ubyte;
   case PCLXL_TAG_ATTR_UINT16:
      value = pclxl_read_number(PCLXL_TAG_UINT16, endian);
      return value.number.uint16;
   }

   return -1; // invalid PCL XL format
}

void
pclxl_skip_until(char stop)
{
   int c;
   do { c = PCLXL_INPUT(); }
   while (c != stop && c != EOF);
}

static union pclxl_value novalue; // initialized by pclxl_parse()

int
pclxl_get_op(struct pclxl_stack *sp, enum pclxl_endian endian)
{
   int tag, attrid;
   union pclxl_value value;

   while (1)
   {
      tag = PCLXL_INPUT();

      switch (tag) {

      // Binding stuff:

      case PCLXL_TAG_BIND_ASCII:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_ASCII;

      case PCLXL_TAG_BIND_BIN_BE:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_BIN_BE;

      case PCLXL_TAG_BIND_BIN_LE:
         PCLXL_INPUT(); // skip the blank
         pclxl_skip_until('\n');
         return PCLXL_TAG_BIND_BIN_LE;

      // Data Types:

      case PCLXL_TAG_UBYTE:
      case PCLXL_TAG_UINT16:
      case PCLXL_TAG_SINT16:
      case PCLXL_TAG_UINT32:
      case PCLXL_TAG_SINT32:
      case PCLXL_TAG_REAL32:
         value = pclxl_read_number(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, value, attrid);
         break;

      case PCLXL_TAG_UBYTE_XY:
      case PCLXL_TAG_UINT16_XY:
      case PCLXL_TAG_SINT16_XY:
      case PCLXL_TAG_UINT32_XY:
      case PCLXL_TAG_SINT32_XY:
      case PCLXL_TAG_REAL32_XY:
         value = pclxl_read_pair(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, value, attrid);
         break;

      case PCLXL_TAG_UBYTE_BOX:
      case PCLXL_TAG_UINT16_BOX:
      case PCLXL_TAG_SINT16_BOX:
      case PCLXL_TAG_UINT32_BOX:
      case PCLXL_TAG_SINT32_BOX:
      case PCLXL_TAG_REAL32_BOX:
         pclxl_skip_bbox(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, novalue, attrid);
         break;

      case PCLXL_TAG_UBYTE_ARRAY:
      case PCLXL_TAG_UINT16_ARRAY:
      case PCLXL_TAG_SINT16_ARRAY:
      case PCLXL_TAG_UINT32_ARRAY:
      case PCLXL_TAG_SINT32_ARRAY:
      case PCLXL_TAG_REAL32_ARRAY:
         pclxl_skip_array(tag, endian);
         attrid = pclxl_read_attrid(endian);
         pclxl_stack_push(sp, tag, novalue, attrid);
         break;

      // All other tags (ie, Operators, dataLength, dataLengthByte,
      // white space and reserved) and the EOF marker: return as-is.

      default:
         return tag;
      }
   }
}

int
pclxl_get_page_duplex(struct pclxl_stack *sp)
{
   if (pclxl_stack_find(sp, PCLXL_ATTR_DUPLEX_PAGE_MODE))
      return 1;

   if (pclxl_stack_find(sp, PCLXL_ATTR_SIMPLEX_PAGE_MODE))
      return 0;

   return -1; // not specified; use PJL setting
}

float
pclxl_get_page_size(struct pclxl_stack *sp)
{
   struct pclxl_attr *attr = pclxl_stack_find(sp, PCLXL_ATTR_MEDIA_SIZE);
   if (attr != NULL)
   {
      switch (attr->type) {
      case PCLXL_TAG_UBYTE: return attr->value.number.ubyte;
      // TODO Support for "named" formats (UBTYE_ARRAY)
      }
   }

   // TODO Support for CUSTOM_MEDIA_SIZE with CUSTOM_MEDIA_SIZE_UNITS

   return -1; // not specified; use PJL setting or previous MEDIA_SIZE??
}

int
pclxl_get_page_copies(struct pclxl_stack *sp)
{
   struct pclxl_attr *attr = pclxl_stack_find(sp, PCLXL_ATTR_PAGE_COPIES);
   if (attr != NULL)
   {
      switch (attr->type) {
      case PCLXL_TAG_UBYTE: return attr->value.number.ubyte;
      case PCLXL_TAG_UINT16: return attr->value.number.uint16;
      case PCLXL_TAG_UINT32: return attr->value.number.uint32;
      }
   }

   return -1; // not specified; use PJL setting
}

int
pclxl_parse(struct printer *prt, FILE *logfp, int verbose)
{
   // logfp: where to log (job dump) info
   // verbose: 0=silent, 1=pages, 2=all ops

   struct pclxl_stack stack;
   enum pclxl_endian endian;
   int op, quit = 0, copies, duplex;
   float pagesize;

   endian = PCLXL_ENDIAN_UNKNOWN;
   memset(&novalue, 0, sizeof(novalue));
   pclxl_stack_init(&stack, PCLXL_STACK_SIZE);

   // TODO Don't modify prt directly, use joblex_printer_foo() instead

   while (!quit)
   {
      pclxl_stack_clear(&stack);

      op = pclxl_get_op(&stack, endian);

      if (verbose > 1)
         pclxl_logop(logfp, op, &stack);

      switch (op) {

      case PCLXL_TAG_BIND_ASCII:
         quit = -1; // TODO Don't know how to read this
         break;
      case PCLXL_TAG_BIND_BIN_LE:
         endian = PCLXL_ENDIAN_LITTLE;
         break;
      case PCLXL_TAG_BIND_BIN_BE:
         endian = PCLXL_ENDIAN_BIG;
         break;

      case PCLXL_TAG_DATA_LENGTH:
      case PCLXL_TAG_DATA_LENGTH_BYTE:
         // ignore data (it belongs to the preceding command)
         pclxl_skip_data(op, endian);
         break;

      case PCLXL_TAG_BEGIN_PAGE:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         duplex = pclxl_get_page_duplex(&stack);
         pagesize = pclxl_get_page_size(&stack);
         // TODO Special duplex/eject logic if pagesize != last_pagesize ??
         break;
      case PCLXL_TAG_END_PAGE:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         copies = MAX(0, pclxl_get_page_copies(&stack));
         // With PCL XL, copies may be 0, meaning to not print this page.
         // BeginPage's duplex and EndPage's copies override the
         // PJL DUPLEX and COPIES variables (PCL XL ref, Appendix I).
         joblex_printer_set_pagesize(prt, pagesize);
         joblex_printer_endpage(prt, duplex, copies);
         break;

      case PCLXL_TAG_BEGIN_SESSION:
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         break;

      case PCLXL_TAG_END_SESSION:
         // What if there's another session? Appendix H of the PCL XL
         // Feature Reference says that all PCL XL sessions MUST be
         // invoked through a PJL ENTER LANGUAGE command and that
         // EndSession in the stream SHOULD be followed by a UEL.
         if (verbose == 1) pclxl_logop(logfp, op, &stack);
         quit = 1;
         break;

      case EOF:
         if (verbose > 0) pclxl_logop(logfp, op, &stack);
         quit = 1;
         break;
      }
   }

   pclxl_stack_free(&stack);

   return quit < 0 ? quit : 0;
}
