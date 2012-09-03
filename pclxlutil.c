#include <stdio.h> // for EOF

#include "pclxl.h"

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
