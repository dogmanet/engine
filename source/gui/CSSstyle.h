#ifndef _CCSSstyle_H_
#define _CCSSstyle_H_


#include "ICSS.h"

namespace gui
{
	namespace css
	{
		class CCSSproperty: public ICSSproperty
		{
		public:
			CCSSproperty():m_type(TYPE_UNDEFINED), m_isSet(false), m_isChanged(false)
			{
				m_value.i = 0;
			}

			CCSSproperty(TYPE type):m_type(type), m_isSet(false), m_isChanged(true)
			{
				m_value.i = 0;
			}
			CCSSproperty(int value):m_type(TYPE_VAR_INT), m_isSet(true), m_isChanged(true)
			{
				m_value.i = value;
			}
			CCSSproperty(float value):m_type(TYPE_VAR_FLOAT), m_isSet(true), m_isChanged(true)
			{
				m_value.f = value;
			}
			/*CCSSproperty(const StringW & value):m_type(TYPE_VAR_STRING), m_isSet(true), m_isChanged(true)
			{
				int len = value.length();
				m_value.s = new WCHAR[len + 1];
				memcpy(m_value.s, value.c_str(), (len + 1) * sizeof(WCHAR));
			}
			CCSSproperty(const WCHAR * value):m_type(TYPE_VAR_STRING), m_isSet(true), m_isChanged(true)
			{
				int len = lstrlenW(value);
				m_value.s = new WCHAR[len + 1];
				memcpy(m_value.s, value, (len + 1) * sizeof(WCHAR));
			}*/
			CCSSproperty(const SMMATRIX & val):m_type(TYPE_VAR_MATRIX), m_isSet(true), m_isChanged(true)
			{
				memcpy(m_value.m, val.m, sizeof(val.m));
			}

			~CCSSproperty()
			{
				if(m_type == TYPE_VAR_STRING)
				{
					//mem_delete_a(m_value.s);
				}
			}

			TYPE getType() const
			{
				return(m_type);
			}

			void set(int val)
			{
				if(m_type == TYPE_VAR_STRING)
				{
					//mem_delete_a(m_value.s);
				}
				m_isSet = true;
				if(m_type != TYPE_VAR_INT || m_value.i != val)
				{
					m_isChanged = true;
				}
				m_value.i = val;
				m_type = TYPE_VAR_INT;
			}
			void set(float val)
			{
				if(m_type == TYPE_VAR_STRING)
				{
					//mem_delete_a(m_value.s);
				}
				m_isSet = true;
				if(m_type != TYPE_VAR_FLOAT || m_value.f != val)
				{
					m_isChanged = true;
				}
				m_value.f = val;
				m_type = TYPE_VAR_FLOAT;
			}
			void set(const StringW & val)
			{
				set(val.c_str());
			}
			void set(const WCHAR * val)
			{
				int idx = CCSSstringManager::getStringID(val);
				if(m_type == TYPE_VAR_STRING)
				{
					if(m_value.i != idx)
					{
						m_isChanged = true;
					}
					//mem_delete_a(m_value.s);
				}
				else
				{
					m_isChanged = true;
				}
				m_isSet = true;
				m_value.i = idx;
				/*int len = lstrlenW(val);
				m_value.s = new WCHAR[len + 1];
				memcpy(m_value.s, val, (len + 1) * sizeof(WCHAR));*/
				m_type = TYPE_VAR_STRING;
			}
			void set(const SMMATRIX & val)
			{
				if(m_type == TYPE_VAR_STRING)
				{
					//mem_delete_a(m_value.s);
				}
				m_isSet = true;
				if(m_type != TYPE_VAR_MATRIX || memcmp(m_value.m, val.m, sizeof(val.m)) != 0)
				{
					m_isChanged = true;
				}
				memcpy(m_value.m, val.m, sizeof(val.m));
				m_type = TYPE_VAR_MATRIX;
			}

			void setExt(int val)
			{
				if(m_value.i != val)
				{
					m_isChanged = true;
				}
				m_value.i = val;
				m_isSet = true;
			}

			int getInt() const
			{
				if(m_type == gui::css::ICSSproperty::TYPE_VAR_FLOAT)
				{
					return((int)m_value.f);
				}
				return(m_value.i);
			}
			float getFloat() const
			{
				if(m_type == gui::css::ICSSproperty::TYPE_VAR_INT)
				{
					return((float)m_value.i);
				}
				return(m_value.f);
			}
			const WCHAR * getString() const
			{
				return(CCSSstringManager::getString(m_value.i));
			}
			SMMATRIX getMatrix() const
			{
				SMMATRIX m;
				memcpy(m.m, m_value.m, sizeof(m_value.m));
				return(m);
			}

			void setDim(DIM d)
			{
				m_dim = d;
			}

			DIM getDim() const
			{
				return(m_dim);
			}

			void setChanged(bool b)
			{
				m_isChanged = b;
			}

			void unChange()
			{
				m_isChanged = false;
			}

			float4_t getColor()const
			{
				return(float4_t(
					(float)(((UINT)m_value.i & 0xFF000000) >> 24) / 255.0f,
					(float)(((UINT)m_value.i & 0x00FF0000) >> 16) / 255.0f,
					(float)(((UINT)m_value.i & 0x0000FF00) >> 8) / 255.0f,
					(float)(((UINT)m_value.i & 0x000000FF)) / 255.0f
					));
			}

			bool isSet() const
			{
				return(m_isSet);
			}

			bool isChanged() const
			{
				return(m_isChanged);
			}

			bool operator==(const ICSSproperty &_b)
			{
				const CCSSproperty * b = (CCSSproperty*)&_b;
				bool bbb;
				/*if(m_type == TYPE_VAR_STRING)
				{
					bbb = lstrcmpW(m_value.s, b->m_value.s) == 0;
				}
				else */if(m_type == TYPE_VAR_MATRIX)
				{
					bbb = memcmp(m_value.m, b->m_value.m, sizeof(m_value.m)) == 0;
				}
				else
				{
					bbb = m_value.i == b->m_value.i;
				}
				return(m_type == b->m_type && m_dim == b->m_dim && bbb);
			}

			static StringW getDimName(DIM dim)
			{
				switch(dim)
				{
				case DIM_EM:
					return(L"em");
				case DIM_MM:
					return(L"mm");
				case DIM_PC:
					return(L"%");
				case DIM_PT:
					return(L"pt");
				case DIM_PX:
					return(L"px");
				}
				return(L"");
			}

			CCSSproperty & operator=(const CCSSproperty & p)
			{
				if(!(*this == p))
				{
					if(m_type == TYPE_VAR_STRING)
					{
						//mem_delete_a(m_value.s);
					}
					/*if(p.m_type == TYPE_VAR_STRING)
					{
						m_value.s = new WCHAR[wcslen(p.m_value.s) + 1];
						memcpy(m_value.s, p.m_value.s, (wcslen(p.m_value.s) + 1) * sizeof(WCHAR));
						m_type = TYPE_VAR_STRING;
					}
					else */if(p.m_type == TYPE_VAR_MATRIX)
					{
						memcpy(m_value.m, p.m_value.m, sizeof(p.m_value.m));
					}
					else
					{
						m_value.i = p.m_value.i;
					}
				}
				m_isSet = p.m_isSet;
				m_type = p.m_type;
				m_dim = p.m_dim;
				m_isChanged = p.m_isChanged;
				return(*this);
			}

			void unset()
			{
				if(m_type == TYPE_VAR_MATRIX)
				{
					set(SMMatrixIdentity());
					m_isSet = false;
					return;
				}
				TYPE t = m_type;
				set(0);
				m_isSet = false;
				if(m_type != TYPE_VAR_STRING &&
					m_type != TYPE_VAR_INT &&
					m_type != TYPE_VAR_FLOAT)
				{
					m_type = t;
				}
				else
				{
					m_type = TYPE_UNDEFINED;
				}

				if(m_flags & FLAG_DO_TRANSITION)
				{
					m_flags ^= FLAG_DO_TRANSITION;
				}
			}

			UINT getFlags()
			{
				return(m_flags);
			}

			void setFlags(UINT f)
			{
				m_flags = f;
			}

			int getPX(int base)
			{
				if(m_type == css::ICSSproperty::TYPE_UNDEFINED)
				{
					return(0);
				}
				float falue = getFloat();
				switch(getDim())
				{
				case css::ICSSproperty::DIM_PC:
					return((int)_round((float)base * (falue * 0.01f)));
				case css::ICSSproperty::DIM_PX:
				case css::ICSSproperty::DIM_NONE:
					return((int)_round(falue));
				case css::ICSSproperty::DIM_PT:
					return((int)_round((float)falue * 4.0f / 3.0f));
				case css::ICSSproperty::DIM_MM:
					return((int)_round((float)falue * 3.7795f));
					break;
				default:
					return(base);
				}
			}

			void setupTransition(const ICSSproperty * pOldProperty, dom::IDOMdocument *pDoc);

			void allowTransition(float fDuration, float fDelay, TIMING_FUNCTION_e timingFunction)
			{
				if(m_flags & ICSSproperty::FLAG_ALLOW_TRANSITION)
				{
					m_flags |= ICSSproperty::FLAG_DO_TRANSITION;
					m_fTransitionSpeed = 1.0f / fDuration;
					m_fTransitionDelay = fDelay;
					m_transitionTimingFunction = timingFunction;
				}
			}

			bool updateTransition(float fTimeDelta)
			{
				if(m_flags & ICSSproperty::FLAG_DO_TRANSITION)
				{
					m_fTransitionFrac += fTimeDelta * m_fTransitionSpeed;
					if(m_fTransitionFrac > 1.0f)
					{
						m_fTransitionFrac = 1.0f;
					}

					//P = (1−t)^3P1 + 3(1−t)^2tP2 + 3(1−t)t^2P3 + t^3P4
					float2 p1(0.0f, 0.0f);
					float2 p2(0.0f, 0.0f);
					float2 p3(1.0f, 1.0f);
					float2 p4(1.0f, 1.0f);

					float t = m_fTransitionFrac;
					switch(m_transitionTimingFunction)
					{
					case TIMING_FUNCTION_EASE:
						p2 = float2(0.25f, 0.1f);
						p3 = float2(0.25f, 1.0f);
						break;
					case TIMING_FUNCTION_EASE_IN:
						p2 = float2(0.42f, 0.0f);
						p3 = float2(1.0f, 1.0f);
						break;
					case TIMING_FUNCTION_EASE_IN_OUT:
						p2 = float2(0.42f, 0.0f);
						p3 = float2(0.58f, 1.0f);
						break;
					case TIMING_FUNCTION_EASE_OUT:
						p2 = float2(0.0f, 0.0f);
						p3 = float2(0.58f, 1.0f);
						break;
					case TIMING_FUNCTION_LINEAR:
						// empty
						break;
					case TIMING_FUNCTION_STEP_START:
						t = 1.0f;
						break;
					case TIMING_FUNCTION_STEP_END:
						if(t < 1.0f)
						{
							t = 0.0f;
						}
						break;
					}

					float2 p = powf(1.0f - t, 3.0f) * p1 + 3.0f * powf(1.0f - t, 2.0f) * t * p2 + 3.0f * (1.0f - t) * powf(t, 2.0f) * p3 + powf(t, 3.0f) * p4;
					t = p.y;

					switch(m_type)
					{
					case TYPE_VAR_FLOAT:
						m_value.f = lerpf(m_valueOld.f, m_valueTo.f, t);
						break;
					case TYPE_VAR_INT:
						if(m_flags & FLAG_PROP_COLOR)
						{
							UINT clr = *((UINT*)(&(m_valueOld.i)));

							UINT r1 = ((clr & 0xFF000000) >> 24);
							UINT g1 = ((clr & 0x00FF0000) >> 16);
							UINT b1 = ((clr & 0x0000FF00) >> 8);
							UINT a1 = (clr & 0x000000FF);

							clr = *((UINT*)(&(m_valueTo.i)));

							UINT r2 = ((clr & 0xFF000000) >> 24);
							UINT g2 = ((clr & 0x00FF0000) >> 16);
							UINT b2 = ((clr & 0x0000FF00) >> 8);
							UINT a2 = (clr & 0x000000FF);

							r1 = (int)roundf(lerpf(r1, r2, t));
							g1 = (int)roundf(lerpf(g1, g2, t));
							b1 = (int)roundf(lerpf(b1, b2, t));
							a1 = (int)roundf(lerpf(a1, a2, t));

							*((UINT*)(&(m_value.i))) = (r1 << 24) | (g1 << 16) | (b1 << 8) | a1;
						}
						else
						{
							m_value.i = (int)roundf(lerpf(m_valueOld.i, m_valueTo.i, t));
						}
						break;
					case TYPE_VAR_MATRIX:
						for(int i = 0; i < 4; ++i)
						{
							for(int j = 0; j < 4; ++j)
							{
								m_value.m[i][j] = lerpf(m_valueOld.m[i][j], m_valueTo.m[i][j], t);
							}
						}
					}
					setChanged(true);
					return(m_fTransitionFrac == 1.0f);
				}
				return(true);
			}

			void cancelTransitions(dom::IDOMdocument *pDoc);

		protected:
			TYPE m_type;
			DIM m_dim;
			bool m_isSet;
			bool m_isChanged;
			UINT m_flags;
			union
			{
				int i;
				float f;
				WCHAR * s;
				float m[4][4];
			} 
				m_value, //!< Действующее значение
				m_valueOld, //!< Начальное значение, для transiton
				m_valueTo; //!< Конечное значение, для transition

			float m_fTransitionFrac;
			float m_fTransitionDelay;
			float m_fTransitionSpeed;
			TIMING_FUNCTION_e m_transitionTimingFunction;
		};

		class CCSSstyle: public ICSSstyle
		{
		public:
			CCSSstyle();
			~CCSSstyle();

			void loadStyleFromString(const WCHAR * str, UINT * pNumSyms);
			void parsePair(const StringW & key, const StringW & val);
			void parseColor(ICSSproperty * clr, const StringW & val);
			void parseValue(ICSSproperty * clr, const StringW & val);
			void parseTransform(ICSSproperty * clr, const StringW & val);
			bool parseBeginArgs(const wchar_t ** pszStr);
			bool parseNextArgs(const wchar_t ** pszStr);
			bool parseEndArgs(const wchar_t ** pszStr);
			bool parseAngleArgs(const wchar_t ** pszStr, float * pfOut);
			bool parseTimeArgs(const wchar_t ** pszStr, float * pfOut);
			bool parseNumberArgs(const wchar_t ** pszStr, float * pfOut);
			//const String & operator[](const String & key);
			//void Set(const String & key, const String & val);

			UINT getChangesFlags();

			void resetChanges();

			void addRule(const ICSSrule & rule)
			{
				m_pRules.push_back(rule);
			};

			void reset();

#ifdef _DEBUG
			void debugDumpStyles();
#endif

			void captureChanges(const ICSSstyle * pOldStyles, dom::CDOMdocument *pDoc);

#define TMP_COPY_RULE(r) r = p.r
			CCSSstyle & operator=(const CCSSstyle & p)
			{
				m_pRules = p.m_pRules;

				
				TMP_COPY_RULE(p_display);

				TMP_COPY_RULE(p_color);

				TMP_COPY_RULE(p_cursor);

				TMP_COPY_RULE(p_direction);

				TMP_COPY_RULE(p_opacity);

				TMP_COPY_RULE(p_z_index);

				TMP_COPY_RULE(p_overflow_x);
				TMP_COPY_RULE(p_overflow_y);

				TMP_COPY_RULE(p_margin_left);
				TMP_COPY_RULE(p_margin_right);
				TMP_COPY_RULE(p_margin_top);
				TMP_COPY_RULE(p_margin_bottom);

				TMP_COPY_RULE(p_padding_left);
				TMP_COPY_RULE(p_padding_right);
				TMP_COPY_RULE(p_padding_top);
				TMP_COPY_RULE(p_padding_bottom);

				TMP_COPY_RULE(p_top);
				TMP_COPY_RULE(p_left);
				TMP_COPY_RULE(p_right);
				TMP_COPY_RULE(p_bottom);

				TMP_COPY_RULE(p_width);
				TMP_COPY_RULE(p_height);

				TMP_COPY_RULE(p_max_width);
				TMP_COPY_RULE(p_max_height);

				TMP_COPY_RULE(p_min_width);
				TMP_COPY_RULE(p_min_height);

				TMP_COPY_RULE(p_position);

				TMP_COPY_RULE(p_visibility);

				TMP_COPY_RULE(p_float);

				TMP_COPY_RULE(p_background_image);
				TMP_COPY_RULE(p_background_color);
				TMP_COPY_RULE(p_background_position_x);
				TMP_COPY_RULE(p_background_position_y);
				TMP_COPY_RULE(p_background_attachment); //
				TMP_COPY_RULE(p_background_repeat); //
				TMP_COPY_RULE(p_background_size_x);
				TMP_COPY_RULE(p_background_size_y);


				TMP_COPY_RULE(p_border_top_style); //
				TMP_COPY_RULE(p_border_top_width);
				TMP_COPY_RULE(p_border_top_color);
				TMP_COPY_RULE(p_border_top_left_radius);
				TMP_COPY_RULE(p_border_top_right_radius);
				TMP_COPY_RULE(p_border_top_left_method);
				TMP_COPY_RULE(p_border_top_right_method);

				TMP_COPY_RULE(p_border_right_style); //
				TMP_COPY_RULE(p_border_right_width);
				TMP_COPY_RULE(p_border_right_color);

				TMP_COPY_RULE(p_border_bottom_style); //
				TMP_COPY_RULE(p_border_bottom_width);
				TMP_COPY_RULE(p_border_bottom_color);
				TMP_COPY_RULE(p_border_bottom_left_radius);
				TMP_COPY_RULE(p_border_bottom_right_radius);
				TMP_COPY_RULE(p_border_bottom_left_method);
				TMP_COPY_RULE(p_border_bottom_right_method);

				TMP_COPY_RULE(p_border_left_style); //
				TMP_COPY_RULE(p_border_left_width);
				TMP_COPY_RULE(p_border_left_color);


				TMP_COPY_RULE(p_font_name);
				TMP_COPY_RULE(p_font_size);
				TMP_COPY_RULE(p_font_style);
				TMP_COPY_RULE(p_font_weight);

				TMP_COPY_RULE(p_text_align);
				TMP_COPY_RULE(p_text_decoration_line);

				TMP_COPY_RULE(p_text_shadow_offset_x);
				TMP_COPY_RULE(p_text_shadow_offset_y);
				TMP_COPY_RULE(p_text_shadow_blur_radius);
				TMP_COPY_RULE(p_text_shadow_color);

				TMP_COPY_RULE(p__gui_text_appearance);
				TMP_COPY_RULE(p__gui_text_cursor);
				TMP_COPY_RULE(p__gui_text_format);
				TMP_COPY_RULE(p__gui_z);
				TMP_COPY_RULE(p__gui_spin_percent);
				TMP_COPY_RULE(p_transform);



				display = &p_display;
				color = &p_color;
				cursor = &p_cursor;
				direction = &p_direction;
				opacity = &p_opacity;
				z_index = &p_z_index;
				overflow_x = &p_overflow_x;
				overflow_y = &p_overflow_y;
				margin_left = &p_margin_left;
				margin_right = &p_margin_right;
				margin_top = &p_margin_top;
				margin_bottom = &p_margin_bottom;
				padding_left = &p_padding_left;
				padding_right = &p_padding_right;
				padding_top = &p_padding_top;
				padding_bottom = &p_padding_bottom;
				top = &p_top;
				left = &p_left;
				right = &p_right;
				bottom = &p_bottom;
				width = &p_width;
				height = &p_height;
				max_width = &p_max_width;
				max_height = &p_max_height;
				min_width = &p_min_width;
				min_height = &p_min_height;
				position = &p_position;
				visibility = &p_visibility;
				_float = &p_float;
				background_image = &p_background_image;
				background_color = &p_background_color;
				background_position_x = &p_background_position_x;
				background_position_y = &p_background_position_y;
				background_attachment = &p_background_attachment;
				background_repeat = &p_background_repeat;
				background_size_x = &p_background_size_x;
				background_size_y = &p_background_size_y;
				border_top_style = &p_border_top_style;
				border_top_width = &p_border_top_width;
				border_top_color = &p_border_top_color;
				border_top_left_radius = &p_border_top_left_radius;
				border_top_right_radius = &p_border_top_right_radius;
				border_top_left_method = &p_border_top_left_method;
				border_top_right_method = &p_border_top_right_method;
				border_right_style = &p_border_right_style;
				border_right_width = &p_border_right_width;
				border_right_color = &p_border_right_color;
				border_bottom_style = &p_border_bottom_style;
				border_bottom_width = &p_border_bottom_width;
				border_bottom_color = &p_border_bottom_color;
				border_bottom_left_radius = &p_border_bottom_left_radius;
				border_bottom_right_radius = &p_border_bottom_right_radius;
				border_bottom_left_method = &p_border_bottom_left_method;
				border_bottom_right_method = &p_border_bottom_right_method;
				border_left_style = &p_border_left_style;
				border_left_width = &p_border_left_width;
				border_left_color = &p_border_left_color;
				font_name = &p_font_name;
				font_size = &p_font_size;
				font_style = &p_font_style;
				font_weight = &p_font_weight;
				text_align = &p_text_align;
				text_decoration_line = &p_text_decoration_line;
				text_shadow_offset_x = &p_text_shadow_offset_x;
				text_shadow_offset_y = &p_text_shadow_offset_y;
				text_shadow_blur_radius = &p_text_shadow_blur_radius;
				text_shadow_color = &p_text_shadow_color;
				_gui_text_appearance = &p__gui_text_appearance;
				_gui_text_cursor = &p__gui_text_cursor;
				_gui_text_format = &p__gui_text_format;
				_gui_z = &p__gui_z;
				_gui_spin_percent = &p__gui_spin_percent;
				transform = &p_transform;


				m_aTransitions = p.m_aTransitions;

				return(*this);
			}
#undef TMP_COPY_RULE
			//protected:
			ICSSRuleSet m_pRules;

			CCSSproperty p_display;

			CCSSproperty p_color;

			CCSSproperty p_cursor;

			CCSSproperty p_direction;

			CCSSproperty p_opacity;

			CCSSproperty p_z_index;

			CCSSproperty p_overflow_x;
			CCSSproperty p_overflow_y;

			CCSSproperty p_margin_left;
			CCSSproperty p_margin_right;
			CCSSproperty p_margin_top;
			CCSSproperty p_margin_bottom;

			CCSSproperty p_padding_left;
			CCSSproperty p_padding_right;
			CCSSproperty p_padding_top;
			CCSSproperty p_padding_bottom;

			CCSSproperty p_top;
			CCSSproperty p_left;
			CCSSproperty p_right;
			CCSSproperty p_bottom;

			CCSSproperty p_width;
			CCSSproperty p_height;

			CCSSproperty p_max_width;
			CCSSproperty p_max_height;

			CCSSproperty p_min_width;
			CCSSproperty p_min_height;

			CCSSproperty p_position;

			CCSSproperty p_visibility;

			CCSSproperty p_float;

			CCSSproperty p_background_image;
			CCSSproperty p_background_color;
			CCSSproperty p_background_position_x;
			CCSSproperty p_background_position_y;
			CCSSproperty p_background_attachment; //
			CCSSproperty p_background_repeat; //
			CCSSproperty p_background_size_x;
			CCSSproperty p_background_size_y;


			CCSSproperty p_border_top_style; //
			CCSSproperty p_border_top_width;
			CCSSproperty p_border_top_color;
			CCSSproperty p_border_top_left_radius;
			CCSSproperty p_border_top_right_radius;
			CCSSproperty p_border_top_left_method;
			CCSSproperty p_border_top_right_method;

			CCSSproperty p_border_right_style; //
			CCSSproperty p_border_right_width;
			CCSSproperty p_border_right_color;

			CCSSproperty p_border_bottom_style; //
			CCSSproperty p_border_bottom_width;
			CCSSproperty p_border_bottom_color;
			CCSSproperty p_border_bottom_left_radius;
			CCSSproperty p_border_bottom_right_radius;
			CCSSproperty p_border_bottom_left_method;
			CCSSproperty p_border_bottom_right_method;

			CCSSproperty p_border_left_style; //
			CCSSproperty p_border_left_width;
			CCSSproperty p_border_left_color;


			CCSSproperty p_font_name;
			CCSSproperty p_font_size;
			CCSSproperty p_font_style;
			CCSSproperty p_font_weight;

			CCSSproperty p_text_align;
			CCSSproperty p_text_decoration_line;

			CCSSproperty p_text_shadow_offset_x;
			CCSSproperty p_text_shadow_offset_y;
			CCSSproperty p_text_shadow_blur_radius;
			CCSSproperty p_text_shadow_color;

			CCSSproperty p__gui_text_appearance;
			CCSSproperty p__gui_text_cursor;
			CCSSproperty p__gui_text_format;
			CCSSproperty p__gui_z;
			CCSSproperty p__gui_spin_percent;

			CCSSproperty p_transform;


			void setupTransitions(const CCSSstyle *pStyle);
			void cancelTransitions();

			void inheritTransitions(const CCSSstyle *pOther);

		protected:

			struct CPropPtr
			{
				typedef CCSSproperty CCSSstyle::*PropPtr;

				template<typename... Args>
				CPropPtr(Args&&... args)
				{
					int expander[] = {0,
						(void(ptrs.push_back(args)), 0)...
					};
				}
				Array<PropPtr> ptrs;
			};
			typedef AssotiativeArray<StringW, CPropPtr> PropsMap;
			static PropsMap m_mPropPtrs;

			static PropsMap *getPropsMap();

			struct CTransitionItem
			{
				CTransitionItem():
					m_fDuration(0.0f),
					m_fDelay(0.0f),
					m_timingFunction(ICSSproperty::TIMING_FUNCTION_EASE)
				{
				}
				StringW m_szProperty;
				float m_fDuration;
				float m_fDelay;
				ICSSproperty::TIMING_FUNCTION_e m_timingFunction;
			};

			Array<CTransitionItem> m_aTransitions;

			void fixupTransitions();

			dom::CDOMdocument *m_pDoc;

		};
	};
};

#endif
