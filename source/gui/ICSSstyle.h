#ifndef _ICSSstyle_H_
#define _ICSSstyle_H_

#include "guimain.h"

namespace gui
{
	namespace dom
	{
		class IDOMdocument;
	};

	namespace css
	{
		class CCSSstringManager
		{
		public:
			static int getStringID(const WCHAR * szValue);
			static const WCHAR * getString(int id);

		protected:
			static Array<const WCHAR *> m_aStrings;
		};

		class ICSSproperty
		{
		public:
			enum TYPE
			{
				TYPE_UNDEFINED = 0,
				TYPE_VAR_INT,
				TYPE_VAR_FLOAT,
				TYPE_VAR_STRING,
				TYPE_VAR_MATRIX,

				TYPE_DISPLAY,
				TYPE_VISIBILITY,
				TYPE_FLOAT,
				TYPE_POSITION,
				TYPE_BACKGROUND_ATTACHMENT,
				TYPE_BACKGROUND_REPEAT,
				TYPE_BORDER_STYLE,
				TYPE_CURSOR,
				TYPE_DIRECTION,
				TYPE_FONT_STYLE,
				TYPE_FONT_WEIGHT,
				TYPE_TEXT_ALIGN,
				TYPE_TEXT_DECORATION_LINE,
				TYPE_OVERFLOW,
				TYPE_BORDER_METHOD,
				TYPE__GUI_TEXT_APPEARANCE,
				TYPE__GUI_TEXT_CURSOR,
				TYPE__GUI_TEXT_FORMAT
			};

			enum DIM
			{
				DIM_NONE,
				DIM_PX,
				DIM_EM,
				DIM_MM,
				DIM_PC,
				DIM_PT,
				DIM_AUTO
			};

			enum TYPE_DISPLAY_e
			{
				DISPLAY_NONE,
				DISPLAY_BLOCK,
				DISPLAY_INLINE,
				DISPLAY_INLINE_BLOCK,
				DISPLAY_LIST_ITEM,
				DISPLAY_TABLE,
				DISPLAY_TABLE_CAPTION,
				DISPLAY_TABLE_COLUMN,
				DISPLAY_TABLE_ROW,
				DISPLAY_TABLE_CELL,
				DISPLAY_INHERIT
			};

			enum TYPE_VISIBILITY_e
			{
				VISIBILITY_VISIBLE,
				VISIBILITY_HIDDEN
			};

			enum TYPE_POSITION_e
			{
				POSITION_STATIC,
				POSITION_ABSOLUTE,
				POSITION_RELATIVE,
				POSITION_FIXED
			};

			enum TYPE_FLOAT_e
			{
				FLOAT_NONE,
				FLOAT_LEFT,
				FLOAT_RIGHT
			};

			enum TYPE_BACKGROUND_ATTACHMENT_e
			{
				BACKGROUND_ATTACHMENT_SCROLL,
				BACKGROUND_ATTACHMENT_LOCAL,
				BACKGROUND_ATTACHMENT_FIXED
			};

			enum TYPE_BACKGROUND_REPEAT_e
			{
				BACKGROUND_REPEAT_NO_REPEAT = 0,
				BACKGROUND_REPEAT_REPEAT_X = 1,
				BACKGROUND_REPEAT_REPEAT_Y = 2,
				BACKGROUND_REPEAT_REPEAT = 3 // == BACKGROUND_REPEAT_REPEAT_X | BACKGROUND_REPEAT_REPEAT_Y
			};

			enum TYPE_BORDER_STYLE_e
			{
				BORDER_STYLE_NONE,
				BORDER_STYLE_INSET,
				BORDER_STYLE_OUTSET,
				BORDER_STYLE_DASHED,
				BORDER_STYLE_DOTTED,
				BORDER_STYLE_DOUBLE,
				BORDER_STYLE_GROOVE,
				BORDER_STYLE_RIDGE,
				BORDER_STYLE_SOLID
			};

			enum TYPE_BORDER_METHOD_e
			{
				BORDER_METHOD_RADIUS,
				BORDER_METHOD_CUT
			};

			enum TYPE_CURSOR_e
			{
				CURSOR_AUTO,
				CURSOR_CELL,
				CURSOR_COL_RESIZE,
				CURSOR_COPY,
				CURSOR_CROSSHAIR,
				CURSOR_DEFAULT,
				CURSOR_E_RESIZE,
				CURSOR_EW_RESIZE,
				CURSOR_HELP,
				CURSOR_MOVE,
				CURSOR_N_RESIZE,
				CURSOR_NE_RESIZE,
				CURSOR_NESW_RESIZE,
				CURSOR_NO_DROP,
				CURSOR_NONE,
				CURSOR_NOT_ALLOWED,
				CURSOR_NS_RESIZE,
				CURSOR_NW_RESIZE,
				CURSOR_NWSE_RESIZE,
				CURSOR_POINTER,
				CURSOR_PROGRESS,
				CURSOR_ROW_RESIZE,
				CURSOR_S_RESIZE,
				CURSOR_SE_RESIZE,
				CURSOR_SW_RESIZE,
				CURSOR_TEXT,
				CURSOR_VERTICAL_TEXT,
				CURSOR_W_RESIZE,
				CURSOR_WAIT,
				CURSOR_INHERIT,
				CURSOR_GRAB,
				CURSOR_GRABBING,
				CURSOR_ZOOM_IN,
				CURSOR_ZOOM_OUT,
				CURSOR_ALIAS,
				CURSOR_ALL_SCROLL
			};

			enum TYPE_DIRECTION_e
			{
				DIRECTION_INHERIT,
				DIRECTION_LTR,
				DIRECTION_RTL
			};

			enum TYPE_FONT_STYLE_e
			{
				FONT_STYLE_NORMAL,
				FONT_STYLE_ITALIC,
				FONT_STYLE_INHERIT
			};

			enum TYPE_FONT_WEIGHT_e
			{
				FONT_WEIGHT_NORMAL,
				FONT_WEIGHT_BOLD,
				FONT_WEIGHT_INHERIT
			};

			enum TYPE_TEXT_ALIGN_e
			{
				TEXT_ALIGN_INHERIT,
				TEXT_ALIGN_LEFT,
				TEXT_ALIGN_RIGHT,
				TEXT_ALIGN_CENTER
			};

			enum TYPE_TEXT_DECORATION_LINE_e
			{
				TEXT_DECORATION_LINE_NONE,
				TEXT_DECORATION_LINE_UNDERLINE,
				TEXT_DECORATION_LINE_LINE_THROUGH,
				TEXT_DECORATION_LINE_OVERLINE
			};

			enum TYPE_OVERFLOW_e
			{
				OVERFLOW_AUTO,
				OVERFLOW_HIDDEN,
				OVERFLOW_SCROLL,
				OVERFLOW_VISIBLE
			};

			enum TYPE__GUI_TEXT_APPEARANCE_e
			{
				_GUI_TEXT_APPEARANCE_MULTI_LINE,
				_GUI_TEXT_APPEARANCE_ONE_LINE
			};

			enum TYPE__GUI_TEXT_CURSOR_e
			{
				_GUI_TEXT_CURSOR_HIDE,
				_GUI_TEXT_CURSOR_SHOW
			};

			enum TYPE__GUI_TEXT_FORMAT_e
			{
				_GUI_TEXT_FORMAT_DEFAULT,
				_GUI_TEXT_FORMAT_PREFORMATTED
			};

			enum TIMING_FUNCTION_e
			{
				TIMING_FUNCTION_EASE = 0,
				TIMING_FUNCTION_EASE_IN,
				TIMING_FUNCTION_EASE_OUT,
				TIMING_FUNCTION_EASE_IN_OUT,
				TIMING_FUNCTION_LINEAR,
				TIMING_FUNCTION_STEP_START,
				TIMING_FUNCTION_STEP_END
			};

			enum FLAGS
			{
				FLAG_UPDATE_BORDERVIEW = 0x0001, //
				FLAG_UPDATE_BORDERLAYOUT = 0x0002, //
				FLAG_UPDATE_LAYOUT = 0x0004, //
				FLAG_UPDATE_VIEW = 0x0008, //
				FLAG_UPDATE_BACKGROUNDLAYOUT = 0x0010, //
				FLAG_UPDATE_STRUCTURE = 0x0020,
				FLAG_UPDATE_CURSOR = 0x0040,
				FLAG_UPDATE_BACKGROUNDVIEW = 0x0080, //
				FLAG_UPDATE_MASK = 0x00FF,
				FLAG_ALLOW_TRANSITION = 0x0100, //
				FLAG_DO_TRANSITION = 0x0200, //
				FLAG_PROP_COLOR = 0x0400 //
			};

			virtual ~ICSSproperty()
			{
			}

			virtual TYPE getType() const = 0;
			virtual void set(int val) = 0;
			virtual void set(float val) = 0;
			virtual void set(const StringW & val) = 0;
			virtual void set(const WCHAR * val) = 0;
			virtual void set(const SMMATRIX & val) = 0;

			virtual void setExt(int val) = 0;

			virtual int getInt() const = 0;
			virtual float getFloat() const = 0;
			virtual const WCHAR * getString() const = 0;
			virtual SMMATRIX getMatrix() const = 0;

			virtual void setDim(DIM d) = 0;

			virtual DIM getDim() const = 0;


			virtual float4_t getColor() const = 0;

			virtual bool isSet() const = 0;

			virtual bool operator==(const ICSSproperty &b) = 0;

			//virtual ICSSproperty & operator=(const ICSSproperty & p) = 0;

			virtual void unset() = 0;

			virtual UINT getFlags() = 0;

			virtual void setFlags(UINT f) = 0;

			virtual int getPX(int base) = 0;

			virtual void setupTransition(const ICSSproperty * pOldProperty, dom::IDOMdocument *pDoc) = 0;

			virtual void allowTransition(float fSpeed, float fDelay, TIMING_FUNCTION_e timingFunction) = 0;
			virtual bool updateTransition(float fTimeDelta) = 0;
			virtual void cancelTransitions(dom::IDOMdocument *pDoc) = 0;


		};

		class ICSSstyle
		{
		public:
			virtual ~ICSSstyle() = 0;

			ICSSproperty * display;

			ICSSproperty * color;

			ICSSproperty * cursor;

			ICSSproperty * direction;

			ICSSproperty * opacity;

			ICSSproperty * z_index;

			ICSSproperty * overflow_x;
			ICSSproperty * overflow_y;

			ICSSproperty * margin_left;
			ICSSproperty * margin_right;
			ICSSproperty * margin_top;
			ICSSproperty * margin_bottom;

			ICSSproperty * padding_left;
			ICSSproperty * padding_right;
			ICSSproperty * padding_top;
			ICSSproperty * padding_bottom;

			ICSSproperty * top;
			ICSSproperty * left;
			ICSSproperty * right;
			ICSSproperty * bottom;

			ICSSproperty * width;
			ICSSproperty * height;

			ICSSproperty * max_width;
			ICSSproperty * max_height;

			ICSSproperty * min_width;
			ICSSproperty * min_height;

			ICSSproperty * position;

			ICSSproperty * visibility;

			ICSSproperty * _float;

			ICSSproperty * background_image;
			ICSSproperty * background_color;
			ICSSproperty * background_position_x;
			ICSSproperty * background_position_y;
			ICSSproperty * background_attachment; //
			ICSSproperty * background_repeat; //
			ICSSproperty * background_size_x;
			ICSSproperty * background_size_y;


			ICSSproperty * border_top_style; //
			ICSSproperty * border_top_width;
			ICSSproperty * border_top_color;
			ICSSproperty * border_top_left_radius;
			ICSSproperty * border_top_right_radius;
			ICSSproperty * border_top_left_method;
			ICSSproperty * border_top_right_method;

			ICSSproperty * border_right_style; //
			ICSSproperty * border_right_width;
			ICSSproperty * border_right_color;

			ICSSproperty * border_bottom_style; //
			ICSSproperty * border_bottom_width;
			ICSSproperty * border_bottom_color;
			ICSSproperty * border_bottom_left_radius;
			ICSSproperty * border_bottom_right_radius;
			ICSSproperty * border_bottom_left_method;
			ICSSproperty * border_bottom_right_method;

			ICSSproperty * border_left_style; //
			ICSSproperty * border_left_width;
			ICSSproperty * border_left_color;


			ICSSproperty * font_name;
			ICSSproperty * font_size;
			ICSSproperty * font_style;
			ICSSproperty * font_weight;

			ICSSproperty * text_align;
			ICSSproperty * text_decoration_line;

			ICSSproperty * text_shadow_offset_x;
			ICSSproperty * text_shadow_offset_y;
			ICSSproperty * text_shadow_blur_radius;
			ICSSproperty * text_shadow_color;

			ICSSproperty * _gui_text_appearance;
			ICSSproperty * _gui_text_cursor;
			ICSSproperty * _gui_text_format;
			ICSSproperty * _gui_z;
			ICSSproperty * _gui_spin_percent;
			ICSSproperty * transform;
		};

		inline ICSSstyle::~ICSSstyle()
		{
		}
	};
};

#endif
