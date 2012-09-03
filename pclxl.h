#ifndef _PCLXL_H_
#define _PCLXL_H_

extern const char *pclxl_tag_name(int tag);
extern const char *pclxl_attr_name(int attrid);
extern const char *pclxl_enum_name(int attrid, int value);

// Appendix B.
// Binary Stream Tag Values

// Tag type: White Space

#define PCLXL_TAG_NUL   0   //0x00
#define PCLXL_TAG_HT    9   //0x09
#define PCLXL_TAG_LF   10   //0x0a
#define PCLXL_TAG_VT   11   //0x0b
#define PCLXL_TAG_FF   12   //0x0c
#define PCLXL_TAG_CR   13   //0x0d
#define PCLXL_TAG_SP   32   //0x20

// Tag type: Binding

#define PCLXL_TAG_BIND_ASCII    39  //0x27
#define PCLXL_TAG_BIND_BIN_BE   40  //0x28
#define PCLXL_TAG_BIND_BIN_LE   41  //0x29

// Tag type: Operator

#define PCLXL_TAG_BEGIN_SESSION          65  //0x41
#define PCLXL_TAG_END_SESSION            66  //0x42

#define PCLXL_TAG_BEGIN_PAGE             67  //0x43
#define PCLXL_TAG_END_PAGE               68  //0x44

#define PCLXL_TAG_COMMENT                71  //0x47

#define PCLXL_TAG_OPEN_DATA_SOURCE       72  //0x48
#define PCLXL_TAG_CLOSE_DATA_SOURCE      73  //0x49

#define PCLXL_TAG_BEGIN_FONT_HEADER      79  //0x4f
#define PCLXL_TAG_READ_FONT_HEADER       80  //0x50
#define PCLXL_TAG_END_FONT_HEADER        81  //0x51

#define PCLXL_TAG_BEGIN_CHAR             82  //0x52
#define PCLXL_TAG_READ_CHAR              83  //0x53
#define PCLXL_TAG_END_CHAR               84  //0x54

#define PCLXL_TAG_REMOVE_FONT            85  //0x55

#define PCLXL_TAG_SET_CHAR_ATTRIBUTES    86  //0x56

#define PCLXL_TAG_BEGIN_STREAM           91  //0x5b
#define PCLXL_TAG_READ_STREAM            92  //0x5c
#define PCLXL_TAG_END_STREAM             93  //0x5d
#define PCLXL_TAG_EXEC_STREAM            94  //0x5e
#define PCLXL_TAG_REMOVE_STREAM          95  //0x5f

#define PCLXL_TAG_POP_GS                 96  //0x60
#define PCLXL_TAG_PUSH_GS                97  //0x61

#define PCLXL_TAG_SET_CLIP_REPLACE       98  //0x62
#define PCLXL_TAG_SET_BRUSH_SOURCE       99  //0x63
#define PCLXL_TAG_SET_CHAR_ANGLE        100  //0x64
#define PCLXL_TAG_SET_CHAR_SCALE        101  //0x65
#define PCLXL_TAG_SET_CHAR_SHEAR        102  //0x66
#define PCLXL_TAG_SET_CLIP_INTERSECT    103  //0x67
#define PCLXL_TAG_SET_CLIP_RECTANGLE    104  //0x68
#define PCLXL_TAG_SET_CLIP_TO_PAGE      105  //0x69
#define PCLXL_TAG_SET_COLOR_SPACE       106  //0x6a
#define PCLXL_TAG_SET_CURSOR            107  //0x6b
#define PCLXL_TAG_SET_CURSOR_REL        108  //0x6c
#define PCLXL_TAG_SET_HALFTONE_METHOD   109  //0x6d
#define PCLXL_TAG_SET_FILL_MODE         110  //0x6e
#define PCLXL_TAG_SET_FONT              111  //0x6f
#define PCLXL_TAG_SET_LINE_DASH         112  //0x70
#define PCLXL_TAG_SET_LINE_CAP          113  //0x71
#define PCLXL_TAG_SET_LINE_JOIN         114  //0x72
#define PCLXL_TAG_SET_MITER_LIMIT       115  //0x73
#define PCLXL_TAG_SET_PAGE_DEFAULT_CTM  116  //0x74
#define PCLXL_TAG_SET_PAGE_ORIGIN       117  //0x75
#define PCLXL_TAG_SET_PAGE_ROTATION     118  //0x76
#define PCLXL_TAG_SET_PAGE_SCALE        119  //0x77
#define PCLXL_TAG_SET_PATTERN_TX_MODE   120  //0x78
#define PCLXL_TAG_SET_PEN_SOURCE        121  //0x79
#define PCLXL_TAG_SET_PEN_WIDTH         122  //0x7a
#define PCLXL_TAG_SET_ROP               123  //0x7b
#define PCLXL_TAG_SET_SOURCE_TX_MODE    124  //0x7c
#define PCLXL_TAG_SET_CHAR_BOLD_VALUE   125  //0x7d
#define PCLXL_TAG_SET_CLIP_MODE         126  //0x7f
#define PCLXL_TAG_SET_PATH_TO_CLIP      127  //0x80
#define PCLXL_TAG_SET_CHAR_SUB_MODE     128  //0x81

#define PCLXL_TAG_CLOSE_SUB_PATH        132  //0x84
#define PCLXL_TAG_NEW_PATH              133  //0x85
#define PCLXL_TAG_PAINT_PATH            134  //0x86

#define PCLXL_TAG_ARC_PATH              145  //0x91
#define PCLXL_TAG_BEZIER_PATH           147  //0x93
#define PCLXL_TAG_BEZIER_REL_PATH       149  //0x95
#define PCLXL_TAG_CHORD                 150  //0x96
#define PCLXL_TAG_CHORD_PATH            151  //0x97
#define PCLXL_TAG_ELLIPSE               152  //0x98
#define PCLXL_TAG_ELLIPSE_PATH          153  //0x99
#define PCLXL_TAG_LINE_PATH             155  //0x9b
#define PCLXL_TAG_LINE_REL_PATH         157  //0x9d
#define PCLXL_TAG_PIE                   158  //0x9e
#define PCLXL_TAG_PIE_PATH              159  //0x9f
#define PCLXL_TAG_RECTANGLE             160  //0xa0
#define PCLXL_TAG_RECTANGLE_PATH        161  //0xa1
#define PCLXL_TAG_ROUND_RECTANGLE       162  //0xa2
#define PCLXL_TAG_ROUND_RECTANGLE_PATH  163  //0xa3

#define PCLXL_TAG_TEXT                  168  //0xa8
#define PCLXL_TAG_TEXT_PATH             169  //0xa9

#define PCLXL_TAG_BEGIN_IMAGE           176  //0xb0
#define PCLXL_TAG_READ_IMAGE            177  //0xb1
#define PCLXL_TAG_END_IMAGE             178  //0xb2

#define PCLXL_TAG_BEGIN_RAST_PATTERN    179  //0xb3
#define PCLXL_TAG_READ_RAST_PATTERN     180  //0xb4
#define PCLXL_TAG_END_RAST_PATTERN      181  //0xb5

#define PCLXL_TAG_BEGIN_SCAN            182  //0xb6
#define PCLXL_TAG_END_SCAN              184  //0xb8
#define PCLXL_TAG_SCAN_LINE_REL         185  //0xb9

// Tag type: Data Type

#define PCLXL_TAG_UBYTE                 192  //0xc0
#define PCLXL_TAG_UINT16                193  //0xc1
#define PCLXL_TAG_UINT32                194  //0xc2
#define PCLXL_TAG_SINT16                195  //0xc3
#define PCLXL_TAG_SINT32                196  //0xc4
#define PCLXL_TAG_REAL32                197  //0xc5

#define PCLXL_TAG_UBYTE_ARRAY           200  //0xc8
#define PCLXL_TAG_UINT16_ARRAY          201  //0xc9
#define PCLXL_TAG_UINT32_ARRAY          202  //0xca
#define PCLXL_TAG_SINT16_ARRAY          203  //0xcb
#define PCLXL_TAG_SINT32_ARRAY          204  //0xcc
#define PCLXL_TAG_REAL32_ARRAY          205  //0xcd

#define PCLXL_TAG_UBYTE_XY              208  //0xd0
#define PCLXL_TAG_UINT16_XY             209  //0xd1
#define PCLXL_TAG_UINT32_XY             210  //0xd2
#define PCLXL_TAG_SINT16_XY             211  //0xd3
#define PCLXL_TAG_SINT32_XY             212  //0xd4
#define PCLXL_TAG_REAL32_XY             213  //0xd5

#define PCLXL_TAG_UBYTE_BOX             224  //0xe0
#define PCLXL_TAG_UINT16_BOX            225  //0xe1
#define PCLXL_TAG_UINT32_BOX            226  //0xe2
#define PCLXL_TAG_SINT16_BOX            227  //0xe3
#define PCLXL_TAG_SINT32_BOX            228  //0xe4
#define PCLXL_TAG_REAL32_BOX            229  //0xe5

// Tag type: Attribute

#define PCLXL_TAG_ATTR_UBYTE            248  //0xf8
#define PCLXL_TAG_ATTR_UINT16           249  //0xf9

// Tag type: Embed Data

#define PCLXL_TAG_DATA_LENGTH           250  //0xfa
#define PCLXL_TAG_DATA_LENGTH_BYTE      251  //0xfb

// Appendix E.
// Attribute IDs

#define PCLXL_ATTR_PALETTE_DEPTH            2
#define PCLXL_ATTR_COLOR_SPACE              3
#define PCLXL_ATTR_NULL_BRUSH               4
#define PCLXL_ATTR_NULL_PEN                 5
#define PCLXL_ATTR_PALETTE_DATA             6
#define PCLXL_ATTR_PATTERN_SELECT_ID        8
#define PCLXL_ATTR_GRAY_LEVEL               9
#define PCLXL_ATTR_RGB_COLOR               11
#define PCLXL_ATTR_PATTERN_ORIGIN          12
#define PCLXL_ATTR_NEW_DESTINATION_SIZE    13
#define PCLXL_ATTR_PRIMARY_ARRAY           14
#define PCLXL_ATTR_PRIMARY_DEPTH           15
#define PCLXL_ATTR_DEVICE_MATRIX           33
#define PCLXL_ATTR_DITHER_MATRIX_DATA_TYPE 34
#define PCLXL_ATTR_DITHER_ORIGIN           35
#define PCLXL_ATTR_MEDIA_SIZE              37
#define PCLXL_ATTR_MEDIA_SOURCE            38
#define PCLXL_ATTR_MEDIA_TYPE              39
#define PCLXL_ATTR_ORIENTATION             40
#define PCLXL_ATTR_PAGE_ANGLE              41
#define PCLXL_ATTR_PAGE_ORIGIN             42
#define PCLXL_ATTR_PAGE_SCALE              43
#define PCLXL_ATTR_ROP3                    44
#define PCLXL_ATTR_TX_MODE                 45
#define PCLXL_ATTR_CUSTOM_MEDIA_SIZE       47
#define PCLXL_ATTR_CUSTOM_MEDIA_SIZE_UNITS 48
#define PCLXL_ATTR_PAGE_COPIES             49
#define PCLXL_ATTR_DITHER_MATRIX_SIZE      50
#define PCLXL_ATTR_DITHER_MATRIX_DEPTH     51
#define PCLXL_ATTR_SIMPLEX_PAGE_MODE       52
#define PCLXL_ATTR_DUPLEX_PAGE_MODE        53
#define PCLXL_ATTR_DUPLEX_PAGE_SIDE        54
#define PCLXL_ATTR_ARC_DIRECTION           65
#define PCLXL_ATTR_BOUNDING_BOX            66
#define PCLXL_ATTR_DASH_OFFSET             67
#define PCLXL_ATTR_ELLIPSE_DIMENSION       68
#define PCLXL_ATTR_END_POINT               69
#define PCLXL_ATTR_FILL_MODE               70
#define PCLXL_ATTR_LINE_CAP_STYLE          71
#define PCLXL_ATTR_LINE_JOIN_STYLE         72
#define PCLXL_ATTR_MITER_LENGTH            73
#define PCLXL_ATTR_LINE_DASH_STYLE         74
#define PCLXL_ATTR_PEN_WIDTH               75
#define PCLXL_ATTR_POINT                   76
#define PCLXL_ATTR_NUMBER_OF_POINTS        77
#define PCLXL_ATTR_SOLID_LINE              78
#define PCLXL_ATTR_START_POINT             79
#define PCLXL_ATTR_POINT_TYPE              80
#define PCLXL_ATTR_CONTROL_POINT_1         81
#define PCLXL_ATTR_CONTROL_POINT_2         82
#define PCLXL_ATTR_CLIP_REGION             83
#define PCLXL_ATTR_CLIP_MODE               84
#define PCLXL_ATTR_COLOR_DEPTH             98
#define PCLXL_ATTR_BLOCK_HEIGHT            99
#define PCLXL_ATTR_COLOR_MAPPING          100
#define PCLXL_ATTR_COMPRESS_MODE          101
#define PCLXL_ATTR_DESTINATION_BOX        102
#define PCLXL_ATTR_DESTINATION_SIZE       103
#define PCLXL_ATTR_PATTERN_PERSISTENCE    104
#define PCLXL_ATTR_PATTERN_DEFINE_ID      105
#define PCLXL_ATTR_SOURCE_HEIGHT          107
#define PCLXL_ATTR_SOURCE_WIDTH           108
#define PCLXL_ATTR_START_LINE             109
#define PCLXL_ATTR_PAD_BYTES_MULTIPLE     110
#define PCLXL_ATTR_BLOCK_BYTE_LENGTH      111
#define PCLXL_ATTR_NUMBER_OF_SCAN_LINES   115
#define PCLXL_ATTR_COMMENT_DATA           129
#define PCLXL_ATTR_DATA_ORG               130
#define PCLXL_ATTR_MEASURE                134
#define PCLXL_ATTR_SOURCE_TYPE            136
#define PCLXL_ATTR_UNITS_PER_MEASURE      137
#define PCLXL_ATTR_STREAM_NAME            139
#define PCLXL_ATTR_STREAM_DATA_LENGTH     140
#define PCLXL_ATTR_ERROR_REPORT           143
#define PCLXL_ATTR_CHAR_ANGLE             161
#define PCLXL_ATTR_CHAR_CODE              162
#define PCLXL_ATTR_CHAR_DATA_SIZE         163
#define PCLXL_ATTR_CHAR_SCALE             164
#define PCLXL_ATTR_CHAR_SHEAR             165
#define PCLXL_ATTR_CHAR_SIZE              166
#define PCLXL_ATTR_FONT_HEADER_LENGTH     167
#define PCLXL_ATTR_FONT_NAME              168
#define PCLXL_ATTR_FONT_FORMAT            169
#define PCLXL_ATTR_SYMBOL_SET             170
#define PCLXL_ATTR_TEXT_DATA              171
#define PCLXL_ATTR_CHAR_SUB_MODE_ARRAY    172
#define PCLXL_ATTR_X_SPACING_DATA         175
#define PCLXL_ATTR_Y_SPACING_DATA         176
#define PCLXL_ATTR_CHAR_BOLD_VALUE        177
 
#endif // _PCLXL_H_
