#include "GUIbase.h"

#include "ICSS.h"
#include "CSSstyle.h"
#include "DOMdocument.h"


namespace gui
{
	namespace css
	{

		const UINT ICSSrule::PSEUDOCLASS_COUNT = 20;

#ifdef _DEBUG
		void ICSSrule::debugDumpStyles()
		{
			UINT len = m_selectors.size();
			for(UINT i = 0; i < len; i++)
			{
				ICSSselectorItem * sel = &m_selectors[i];
				if(sel->s_tag.length())
				{
					wprintf(L"%s", sel->s_tag.c_str());
				}
				if(sel->s_id.length())
				{
					wprintf(L"#%s", sel->s_id.c_str());
				}
				for(UINT c = 0; c < sel->s_class.size(); c++)
				{
					wprintf(L".%s", sel->s_class[c].c_str());
				}

				if(sel->pseudoclass & PSEUDOCLASS_ACTIVE)
				{
					wprintf(L":active");
				}
				if(sel->pseudoclass & PSEUDOCLASS_CHECKED)
				{
					wprintf(L":checked");
				}
				if(sel->pseudoclass & PSEUDOCLASS_DISABLED)
				{
					wprintf(L":disabled");
				}
				if(sel->pseudoclass & PSEUDOCLASS_EMPTY)
				{
					wprintf(L":empty");
				}
				if(sel->pseudoclass & PSEUDOCLASS_ENABLED)
				{
					wprintf(L":enabled");
				}
				if(sel->pseudoclass & PSEUDOCLASS_FIRST_CHILD)
				{
					wprintf(L":first-child");
				}
				if(sel->pseudoclass & PSEUDOCLASS_FIRST_OF_TYPE)
				{
					wprintf(L":first-of-type");
				}
				if(sel->pseudoclass & PSEUDOCLASS_FOCUS)
				{
					wprintf(L":focus");
				}
				if(sel->pseudoclass & PSEUDOCLASS_HOVER)
				{
					wprintf(L":hover");
				}
				if(sel->pseudoclass & PSEUDOCLASS_LAST_CHILD)
				{
					wprintf(L":last-child");
				}
				if(sel->pseudoclass & PSEUDOCLASS_LAST_OF_TYPE)
				{
					wprintf(L":last-of-type");
				}
				if(sel->pseudoclass & PSEUDOCLASS_ONLY_CHILD)
				{
					wprintf(L":only-child");
				}
				if(sel->pseudoclass & PSEUDOCLASS_ONLY_OF_TYPE)
				{
					wprintf(L":only-of-type");
				}
				if(sel->pseudoclass & PSEUDOCLASS_READ_ONLY)
				{
					wprintf(L":read-only");
				}
				if(sel->pseudoclass & PSEUDOCLASS_READ_WRITE)
				{
					wprintf(L":read-write");
				}
				if(sel->pseudoclass & PSEUDOCLASS_NTH_CHILD)
				{
					wprintf(L":nth-child");
				}
				if(sel->pseudoclass & PSEUDOCLASS_NTH_LAST_CHILD)
				{
					wprintf(L":nth-last-child");
				}
				if(sel->pseudoclass & PSEUDOCLASS_NTH_LAST_OF_TYPE)
				{
					wprintf(L":nth-last-of-type");
				}
				if(sel->pseudoclass & PSEUDOCLASS_NTH_OF_TYPE)
				{
					wprintf(L":nth-of-type");
				}
				if(sel->pseudoclass & (PSEUDOCLASS_NTH_CHILD | PSEUDOCLASS_NTH_LAST_CHILD | PSEUDOCLASS_NTH_LAST_OF_TYPE | PSEUDOCLASS_NTH_OF_TYPE))
				{
					wprintf(L"(%u", sel->nth_num);
					if(sel->pseudoclass & PSEUDOCLASS_NTH_REPEAT)
					{
						wprintf(L"n");
						if(sel->nth_offset)
						{
							wprintf(L"-%u", sel->nth_offset);
						}
					}
					wprintf(L")");
				}

				switch(sel->pseudoelement)
				{
				case PSEUDOELEMENT_FIRST_LETTER:
					wprintf(L"::first-letter");
					break;
				case PSEUDOELEMENT_FIRST_LINE:
					wprintf(L"::first-line");
					break;
				case PSEUDOELEMENT_SELECTION:
					wprintf(L"::selection");
					break;
				}
				wprintf(L" ");
				switch(sel->connector)
				{
				case CONNECTOR_AFTER:
					wprintf(L"~ ");
					break;
				case CONNECTOR_CHILD:
					wprintf(L"> ");
					break;
				case CONNECTOR_NEXT:
					wprintf(L"+ ");
					break;
				}
			}
		}
#endif

//##########################################################################

		void ICSSrule::ICSSselectorItem::setPseudoClass(const StringW & str)
		{
			if(str == L"active")
			{
				pseudoclass |= PSEUDOCLASS_ACTIVE;
			}
			else if(str == L"checked")
			{
				pseudoclass |= PSEUDOCLASS_CHECKED;
			}
			else if(str == L"disabled")
			{
				pseudoclass |= PSEUDOCLASS_DISABLED;
			}
			else if(str == L"focus")
			{
				pseudoclass |= PSEUDOCLASS_FOCUS;
			}
			else if(str == L"empty")
			{
				pseudoclass |= PSEUDOCLASS_EMPTY;
			}
			else if(str == L"enabled")
			{
				pseudoclass |= PSEUDOCLASS_ENABLED;
			}
			else if(str == L"first-child")
			{
				pseudoclass |= PSEUDOCLASS_FIRST_CHILD;
			}
			else if(str == L"first-of-type")
			{
				pseudoclass |= PSEUDOCLASS_FIRST_OF_TYPE;
			}
			else if(str == L"hover")
			{
				pseudoclass |= PSEUDOCLASS_HOVER;
			}
			else if(str == L"last-child")
			{
				pseudoclass |= PSEUDOCLASS_LAST_CHILD;
			}
			else if(str == L"last-of-type")
			{
				pseudoclass |= PSEUDOCLASS_LAST_OF_TYPE;
			}
			else if(str == L"only-of-type")
			{
				pseudoclass |= PSEUDOCLASS_ONLY_OF_TYPE;
			}
			else if(str == L"only-child")
			{
				pseudoclass |= PSEUDOCLASS_ONLY_CHILD;
			}
			else if(str == L"read-only")
			{
				pseudoclass |= PSEUDOCLASS_READ_ONLY;
			}
			else if(str == L"read-write")
			{
				pseudoclass |= PSEUDOCLASS_READ_WRITE;
			}
			else
			{
				if(str.substr(0, 9) == L"nth-child")
				{
					pseudoclass |= PSEUDOCLASS_NTH_CHILD;
				}
				else if(str.substr(0, 14) == L"nth-last-child")
				{
					pseudoclass |= PSEUDOCLASS_NTH_LAST_CHILD;
				}
				else if(str.substr(0, 11) == L"nth-of-type")
				{
					pseudoclass |= PSEUDOCLASS_NTH_OF_TYPE;
				}
				else if(str.substr(0, 16) == L"nth-last-of-type")
				{
					pseudoclass |= PSEUDOCLASS_NTH_LAST_OF_TYPE;
				}
				if(pseudoclass & (PSEUDOCLASS_NTH_CHILD | PSEUDOCLASS_NTH_LAST_CHILD | PSEUDOCLASS_NTH_OF_TYPE | PSEUDOCLASS_NTH_LAST_OF_TYPE))
				{
					UINT pos = str.find(L"(");
					if(pos != (UINT)(-1))
					{
						UINT d = 0, o = 0;
						WCHAR cn;
						StringW prm = str.substr(pos + 1);
						UINT c = swscanf_s(prm.c_str(), L"%d%c-%d", &d, &cn, 1, &o);
						if(c >= 1)
						{
							nth_num = d;
							if(c >= 2)
							{
								if(cn == L'n')
								{
									pseudoclass |= PSEUDOCLASS_NTH_REPEAT;
								}
								if(c >= 3)
								{
									nth_offset = o;
								}
							}
						}
					}
				}
			}
		}

		void ICSSrule::ICSSselectorItem::setPseudoElement(const StringW & str)
		{
			if(str == L"first-letter")
			{
				pseudoelement = PSEUDOELEMENT_FIRST_LETTER;
			}
			else if(str == L"first-line")
			{
				pseudoelement = PSEUDOELEMENT_FIRST_LINE;
			}
			else if(str == L"selection")
			{
				pseudoelement = PSEUDOELEMENT_SELECTION;
			}
		}

		void ICSSrule::ICSSselectorItem::buildIndex(ICSS * css)
		{
			dom::CDOMdocument * doc = css->getDoc();
			if(s_tag.length())
			{
				dom_tag = doc->getIndexForTagString(s_tag);
			}
			if(s_id.length())
			{
				dom_id = doc->getIndexForIdString(s_id);
			}
			for(UINT i = 0; i < s_class.size(); i++)
			{
				dom_class.push_back(doc->getIndexForClassString(s_class[i]));
			}
		}

//##########################################################################

#ifdef _DEBUG
		void ICSS::debugDumpStyles()
		{
			for(AssotiativeArray<StringW, ICSSstyleSet>::Iterator i = m_styleSets.begin(); i; ++i)
			{
				wprintf(L"########################################\n");
				wprintf(L"%s\n", i.first->c_str());
				wprintf(L"########################################\n");
				i.second->debugDumpStyles();
			}
		}
#endif
	
		void ICSS::addFile(const WCHAR * szFile, int iMaxWidth)
		{
			StringW file(StringW(m_pDesktopStack->getResourceDir()) + L"/css/" + szFile);
			FILE * pF;
			_wfopen_s(&pF, file.c_str(), L"rt, ccs=UTF-8");
			if(!pF)
			{
				printf("[Error]: Unable to open \"%s\" file for reading\n", String(file).c_str());
				return;
			}

			fseek(pF, 0, SEEK_END);
			UINT fs = ftell(pF);
			fseek(pF, 0, SEEK_SET);
			wchar_t * str = new wchar_t[fs + 1];
			if(!str)
			{
				printf("[Error]: Unable to allocate memory\n");
				return;
			}
			str[fread(str, sizeof(wchar_t), fs, pF)] = 0;
			fclose(pF);
			ICSSstyleSet css(this);
			css.setMaxWidth(iMaxWidth);
			css.loadFromString(str);
			m_styleSets[szFile] = css;
			m_styleOrder.push_back(&m_styleSets[szFile]);
			mem_delete_a(str);
			
		}

		void ICSS::dropStyles()
		{
			m_styleSets.clear();
			addFile(L"system.css");
		}

//##########################################################################

#ifdef _DEBUG
		void ICSSstyleSet::debugDumpStyles()
		{
			for(UINT i = 0; i < m_pRules.size(); i++)
			{
				((CCSSstyle*)&m_pRules[i])->debugDumpStyles();
			}
		}
#endif

		void ICSSstyleSet::loadFromString(const WCHAR * wstr)
		{
			UINT len = (UINT)wcslen(wstr);

			bool is_wait_begin = true;
			bool is_read_pseudoclass = false;
			bool is_read_pseudoelement = false;
			bool is_read_tag = false;
			bool is_read_class = false;
			bool is_read_id = false;
			bool is_read_connector = false;
			bool is_connector_read = false;

			Array<ICSSrule> rules;

			ICSSrule::ICSSselectorItem curSelector;
			ICSSrule curRule;

			StringW str;

			WCHAR c;
			for(UINT i = 0; i < len; i++)
			{
				c = wstr[i];
				// tag#id.class1.class2 > tag2#id2 + tag3#id3 ~ tags
				if(iswspace(c))
				{
					if(is_read_pseudoclass)
					{
						curSelector.setPseudoClass(str);
						str = L"";
						is_read_pseudoclass = false;
						is_read_connector = true;
					}
					if(is_read_pseudoelement)
					{
						curSelector.setPseudoElement(str);
						str = L"";
						is_read_pseudoelement = false;
						is_read_connector = true;
					}
					if(is_read_tag)
					{
						curSelector.s_tag = str;
						str = L"";
						is_read_tag = false;
						is_read_connector = true;
					}
					if(is_read_class)
					{
						curSelector.addClass(str);
						str = L"";
						is_read_class = false;
						is_read_connector = true;
					}
					if(is_read_id)
					{
						curSelector.s_id = str;
						str = L"";
						is_read_id = false;
						is_read_connector = true;
					}
				}
				else
				{
					if(c == L'>' || c == L'+' || c == L'~' || c == ',')
					{
						if(is_read_pseudoclass)
						{
							curSelector.setPseudoClass(str);
							str = L"";
							is_read_pseudoclass = false;
							is_read_connector = true;
						}
						if(is_read_pseudoelement)
						{
							curSelector.setPseudoElement(str);
							str = L"";
							is_read_pseudoelement = false;
							is_read_connector = true;
						}
						if(is_read_tag)
						{
							curSelector.s_tag = str;
							str = L"";
							is_read_tag = false;
							is_read_connector = true;
						}
						if(is_read_class)
						{
							curSelector.addClass(str);
							str = L"";
							is_read_class = false;
							is_read_connector = true;
						}
						if(is_read_id)
						{
							curSelector.s_id = str;
							str = L"";
							is_read_id = false;
							is_read_connector = true;
						}
						if(c == L',')
						{
							curSelector.buildIndex(m_pCSS);
							curRule.addSelector(curSelector);
							curSelector = ICSSrule::ICSSselectorItem();
							curSelector.dom_class.clear();
							curSelector.s_class.clear();
							rules.push_back(curRule);
							curRule.release();
							is_read_connector = false;
						}
						else
						{
							if(is_read_connector)
							{
								switch(c)
								{
								case L'>':
									curSelector.connector = ICSSrule::CONNECTOR_CHILD;
									break;
								case L'+':
									curSelector.connector = ICSSrule::CONNECTOR_NEXT;
									break;
								case L'~':
									curSelector.connector = ICSSrule::CONNECTOR_AFTER;
									break;
								};
								is_read_connector = false;
								is_connector_read = true;
								curSelector.buildIndex(m_pCSS);
								curRule.addSelector(curSelector);
								curSelector = ICSSrule::ICSSselectorItem();
								curSelector.dom_class.clear();
								curSelector.s_class.clear();
							}
						}
					}
					else
					{
						if(is_read_connector)
						{
							if(c != '{')
							{
								curSelector.connector = ICSSrule::CONNECTOR_SIMPLE;
							}
							is_read_connector = false;
							is_connector_read = true;
							curSelector.buildIndex(m_pCSS);
							curRule.addSelector(curSelector);
							curSelector = ICSSrule::ICSSselectorItem();
							curSelector.dom_class.clear();
							curSelector.s_class.clear();
						}
						if(c == L'.' || c == L'#' || c == L':' || c == '{')
						{
							bool r = false;
							if(is_read_pseudoclass)
							{
								curSelector.setPseudoClass(str);
								str = L"";
								is_read_pseudoclass = false;
								r = true;
							}
							if(is_read_pseudoelement)
							{
								curSelector.setPseudoElement(str);
								str = L"";
								is_read_pseudoelement = false;
								r = true;
							}
							if(is_read_tag)
							{
								curSelector.s_tag = str;
								str = L"";
								is_read_tag = false;
								r = true;
							}
							if(is_read_class)
							{
								curSelector.addClass(str);
								str = L"";
								is_read_class = false;
								r = true;
							}
							if(is_read_id)
							{
								curSelector.s_id = str;
								str = L"";
								is_read_id = false;
								r = true;
							}
							switch(c)
							{
							case L'.':
								is_read_class = true;
								break;

							case L'#':
								is_read_id = true;
								break;
							case L':':
								if(wstr[i + 1] == L':')
								{
									i++;
									is_read_pseudoelement = true;
								}
								else
								{
									is_read_pseudoclass = true;
								}
								break;
							}
							//finish read
							if(c == L'{')
							{
								if(r)
								{
									curSelector.buildIndex(m_pCSS);
									curRule.addSelector(curSelector);
									curSelector = ICSSrule::ICSSselectorItem();
									curSelector.dom_class.clear();
									curSelector.s_class.clear();
								}
								rules.push_back(curRule);
								//curRule = ICSSrule();
								curRule.release();
								CCSSstyle css;
								UINT u;
								css.loadStyleFromString(wstr + i + 1, &u);
								for(UINT r = 0; r < rules.size(); r++)
								{
									css.addRule(rules[r]);
								}
								m_pRules.push_back(css);
								i += u;
								is_wait_begin = true;
								is_read_pseudoclass = false;
								is_read_pseudoelement = false;
								is_read_tag = false;
								is_read_class = false;
								is_read_id = false;
								is_read_connector = false;
								is_connector_read = false;
								rules.clear();
							}
						}
						else
						{
							if(is_read_class || is_read_id || is_read_pseudoclass || is_read_pseudoelement || is_read_tag)
							{
								str += c;
							}
							else
							{
								is_read_tag = true;
								str += c;
							}
						}
					}
				}
			}
		}

//##########################################################################
		
		CCSSstyle::CCSSstyle():
			p_display(ICSSproperty::TYPE_DISPLAY),
			p_visibility(ICSSproperty::TYPE_VISIBILITY),
			p_float(ICSSproperty::TYPE_FLOAT),
			p_position(ICSSproperty::TYPE_POSITION),
			p_background_attachment(ICSSproperty::TYPE_BACKGROUND_ATTACHMENT),
			p_background_repeat(ICSSproperty::TYPE_BACKGROUND_REPEAT),
			p_border_top_style(ICSSproperty::TYPE_BORDER_STYLE),
			p_border_right_style(ICSSproperty::TYPE_BORDER_STYLE),
			p_border_bottom_style(ICSSproperty::TYPE_BORDER_STYLE),
			p_border_left_style(ICSSproperty::TYPE_BORDER_STYLE),
			p_cursor(ICSSproperty::TYPE_CURSOR),
			p_direction(ICSSproperty::TYPE_DIRECTION),
			p_font_style(ICSSproperty::TYPE_FONT_STYLE),
			p_font_weight(ICSSproperty::TYPE_FONT_WEIGHT),
			p_text_align(ICSSproperty::TYPE_TEXT_ALIGN),
			p_text_decoration_line(ICSSproperty::TYPE_TEXT_DECORATION_LINE),
			p_overflow_x(ICSSproperty::TYPE_OVERFLOW),
			p_overflow_y(ICSSproperty::TYPE_OVERFLOW),
			p_border_top_left_method(ICSSproperty::TYPE_BORDER_METHOD),
			p_border_top_right_method(ICSSproperty::TYPE_BORDER_METHOD),
			p_border_bottom_left_method(ICSSproperty::TYPE_BORDER_METHOD),
			p_border_bottom_right_method(ICSSproperty::TYPE_BORDER_METHOD),
			p__gui_text_appearance(ICSSproperty::TYPE__GUI_TEXT_APPEARANCE),
			p__gui_text_cursor(ICSSproperty::TYPE__GUI_TEXT_CURSOR),
			p__gui_text_format(ICSSproperty::TYPE__GUI_TEXT_FORMAT),

			m_pDoc(NULL)
		{
			p_display.setFlags(ICSSproperty::FLAG_UPDATE_STRUCTURE);

			p_color.setFlags(ICSSproperty::FLAG_UPDATE_VIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);

			p_cursor.setFlags(ICSSproperty::FLAG_UPDATE_CURSOR);

			p_direction.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);

			p_opacity.setFlags(ICSSproperty::FLAG_UPDATE_VIEW | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_z_index.setFlags(0);

			p_overflow_x.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_overflow_y.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);

			p_margin_left.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_margin_right.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_margin_top.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_margin_bottom.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_padding_left.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_padding_right.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_padding_top.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_padding_bottom.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_top.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_left.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_right.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_bottom.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_width.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_height.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_max_width.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_max_height.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_min_width.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_min_height.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);

			p_position.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_UPDATE_STRUCTURE);

			p_visibility.setFlags(ICSSproperty::FLAG_UPDATE_VIEW);

			p_float.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);

			p_background_image.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT);
			p_background_color.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);
			p_background_position_x.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_background_position_y.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_background_attachment.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT); //
			p_background_repeat.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDVIEW); //
			p_background_size_x.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_background_size_y.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);


			p_border_top_style.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT); //
			p_border_top_width.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_top_color.setFlags(ICSSproperty::FLAG_UPDATE_BORDERVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);
			p_border_top_left_radius.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_top_right_radius.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_top_left_method.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT);
			p_border_top_right_method.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT);

			p_border_right_style.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT); //
			p_border_right_width.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_right_color.setFlags(ICSSproperty::FLAG_UPDATE_BORDERVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);

			p_border_bottom_style.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT); //
			p_border_bottom_width.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_bottom_color.setFlags(ICSSproperty::FLAG_UPDATE_BORDERVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);
			p_border_bottom_left_radius.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_bottom_right_radius.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_bottom_left_method.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT);
			p_border_bottom_right_method.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT);

			p_border_left_style.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT); //
			p_border_left_width.setFlags(ICSSproperty::FLAG_UPDATE_BORDERLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_border_left_color.setFlags(ICSSproperty::FLAG_UPDATE_BORDERVIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);


			p_font_name.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_font_size.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_font_style.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_font_weight.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);

			p_text_align.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_text_decoration_line.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);

			p_text_shadow_offset_x.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_text_shadow_offset_y.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_text_shadow_blur_radius.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p_text_shadow_color.setFlags(ICSSproperty::FLAG_UPDATE_VIEW | ICSSproperty::FLAG_ALLOW_TRANSITION | ICSSproperty::FLAG_PROP_COLOR);

			p__gui_text_appearance.setFlags(ICSSproperty::FLAG_UPDATE_LAYOUT);
			p__gui_text_cursor.setFlags(0);
			p__gui_text_format.setFlags(0);
			p__gui_z.setFlags(ICSSproperty::FLAG_UPDATE_VIEW | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p__gui_spin_percent.setFlags(ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT | ICSSproperty::FLAG_ALLOW_TRANSITION);
			p_transform.setFlags(ICSSproperty::FLAG_UPDATE_VIEW | ICSSproperty::FLAG_ALLOW_TRANSITION);




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

		};


		CCSSstyle::~CCSSstyle()
		{
			cancelTransitions();
		}

#ifdef _DEBUG
		void CCSSstyle::debugDumpStyles()
		{
			UINT len = m_pRules.size();
			for(UINT i = 0; i < len; i++)
			{
				m_pRules[i].debugDumpStyles();
				if(i + 1 < len)
				{
					wprintf(L",\n");
				}
			}
			wprintf(L"\n{\n");
			if(p_display.isSet())
			{
				wprintf(L"\tdisplay: ");
				switch(p_display.getInt())
				{
				case ICSSproperty::DISPLAY_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::DISPLAY_BLOCK:
					wprintf(L"block;\n");
					break;
				case ICSSproperty::DISPLAY_INLINE:
					wprintf(L"inline;\n");
					break;
				case ICSSproperty::DISPLAY_INLINE_BLOCK:
					wprintf(L"inline-block;\n");
					break;
				case ICSSproperty::DISPLAY_LIST_ITEM:
					wprintf(L"list-item;\n");
					break;
				case ICSSproperty::DISPLAY_TABLE:
					wprintf(L"table;\n");
					break;
				case ICSSproperty::DISPLAY_TABLE_CAPTION:
					wprintf(L"table-caption;\n");
					break;
				case ICSSproperty::DISPLAY_TABLE_COLUMN:
					wprintf(L"table-column;\n");
					break;
				case ICSSproperty::DISPLAY_TABLE_ROW:
					wprintf(L"table-row;\n");
					break;
				case ICSSproperty::DISPLAY_TABLE_CELL:
					wprintf(L"table-cell;\n");
					break;
				case ICSSproperty::DISPLAY_INHERIT:
					wprintf(L"inherit;\n");
					break;
				}
			}
			if(p_border_top_left_method.isSet())
			{
				wprintf(L"\tborder-top-left-method: ");
				switch(p_border_top_left_method.getInt())
				{
				case ICSSproperty::BORDER_METHOD_RADIUS:
					wprintf(L"radius;\n");
					break;
				case ICSSproperty::BORDER_METHOD_CUT:
					wprintf(L"cut;\n");
					break;
				}
			}
			if(p_border_top_right_method.isSet())
			{
				wprintf(L"\tborder-top-right-method: ");
				switch(p_border_top_right_method.getInt())
				{
				case ICSSproperty::BORDER_METHOD_RADIUS:
					wprintf(L"radius;\n");
					break;
				case ICSSproperty::BORDER_METHOD_CUT:
					wprintf(L"cut;\n");
					break;
				}
			}
			if(p_border_bottom_left_method.isSet())
			{
				wprintf(L"\tborder-bottom-left-method: ");
				switch(p_border_bottom_left_method.getInt())
				{
				case ICSSproperty::BORDER_METHOD_RADIUS:
					wprintf(L"radius;\n");
					break;
				case ICSSproperty::BORDER_METHOD_CUT:
					wprintf(L"cut;\n");
					break;
				}
			}
			if(p_border_bottom_right_method.isSet())
			{
				wprintf(L"\tborder-bottom-right-method: ");
				switch(p_border_bottom_right_method.getInt())
				{
				case ICSSproperty::BORDER_METHOD_RADIUS:
					wprintf(L"radius;\n");
					break;
				case ICSSproperty::BORDER_METHOD_CUT:
					wprintf(L"cut;\n");
					break;
				}
			}
			if(p_color.isSet())
			{
				wprintf(L"\tcolor: #%08X;\n", p_color.getInt());
			}
			if(p_opacity.isSet())
			{
				wprintf(L"\topacity: %f;\n", p_opacity.getFloat());
			}
			if(p_z_index.isSet())
			{
				wprintf(L"\tz-index: %d;\n", p_z_index.getInt());
			}
			if(p_overflow_x.isSet())
			{
				if(p_overflow_x == p_overflow_y)
				{
					wprintf(L"\toverflow: ");
					switch(p_overflow_x.getInt())
					{
					case ICSSproperty::OVERFLOW_AUTO:
						wprintf(L"auto;\n");
						break;
					case ICSSproperty::OVERFLOW_HIDDEN:
						wprintf(L"hidden;\n");
						break;
					case ICSSproperty::OVERFLOW_SCROLL:
						wprintf(L"scroll;\n");
						break;
					case ICSSproperty::OVERFLOW_VISIBLE:
						wprintf(L"visible;\n");
						break;
					}
				}
				else
				{
					wprintf(L"\toverflow-x: ");
					switch(p_overflow_x.getInt())
					{
					case ICSSproperty::OVERFLOW_AUTO:
						wprintf(L"auto;\n");
						break;
					case ICSSproperty::OVERFLOW_HIDDEN:
						wprintf(L"hidden;\n");
						break;
					case ICSSproperty::OVERFLOW_SCROLL:
						wprintf(L"scroll;\n");
						break;
					case ICSSproperty::OVERFLOW_VISIBLE:
						wprintf(L"visible;\n");
						break;
					}
				}
			}
			if(p_overflow_y.isSet())
			{
				if(!(p_overflow_x == p_overflow_y))
				{
					wprintf(L"\toverflow-y: ");
					switch(p_overflow_x.getInt())
					{
					case ICSSproperty::OVERFLOW_AUTO:
						wprintf(L"auto;\n");
						break;
					case ICSSproperty::OVERFLOW_HIDDEN:
						wprintf(L"hidden;\n");
						break;
					case ICSSproperty::OVERFLOW_SCROLL:
						wprintf(L"scroll;\n");
						break;
					case ICSSproperty::OVERFLOW_VISIBLE:
						wprintf(L"visible;\n");
						break;
					}
				}
			}

			if(p_margin_left.isSet())
			{
				wprintf(L"\tmargin-left: %f%s;\n", p_margin_left.getFloat(), CCSSproperty::getDimName(p_margin_left.getDim()).c_str());
			}
			if(p_margin_right.isSet())
			{
				wprintf(L"\tmargin-right: %f%s;\n", p_margin_right.getFloat(), CCSSproperty::getDimName(p_margin_right.getDim()).c_str());
			}
			if(p_margin_top.isSet())
			{
				wprintf(L"\tmargin-top: %f%s;\n", p_margin_top.getFloat(), CCSSproperty::getDimName(p_margin_top.getDim()).c_str());
			}
			if(p_margin_bottom.isSet())
			{
				wprintf(L"\tmargin-bottom: %f%s;\n", p_margin_bottom.getFloat(), CCSSproperty::getDimName(p_margin_bottom.getDim()).c_str());
			}

			if(p_padding_left.isSet())
			{
				wprintf(L"\tpadding-left: %f%s;\n", p_padding_left.getFloat(), CCSSproperty::getDimName(p_padding_left.getDim()).c_str());
			}
			if(p_padding_right.isSet())
			{
				wprintf(L"\tpadding-right: %f%s;\n", p_padding_right.getFloat(), CCSSproperty::getDimName(p_padding_right.getDim()).c_str());
			}
			if(p_padding_top.isSet())
			{
				wprintf(L"\tpadding-top: %f%s;\n", p_padding_top.getFloat(), CCSSproperty::getDimName(p_padding_top.getDim()).c_str());
			}
			if(p_padding_bottom.isSet())
			{
				wprintf(L"\tpadding-bottom: %f%s;\n", p_padding_bottom.getFloat(), CCSSproperty::getDimName(p_padding_bottom.getDim()).c_str());
			}

			if(p_left.isSet())
			{
				wprintf(L"\tleft: %f%s;\n", p_left.getFloat(), CCSSproperty::getDimName(p_left.getDim()).c_str());
			}
			if(p_right.isSet())
			{
				wprintf(L"\tright: %f%s;\n", p_right.getFloat(), CCSSproperty::getDimName(p_right.getDim()).c_str());
			}
			if(p_top.isSet())
			{
				wprintf(L"\ttop: %f%s;\n", p_top.getFloat(), CCSSproperty::getDimName(p_top.getDim()).c_str());
			}
			if(p_bottom.isSet())
			{
				wprintf(L"\tbottom: %f%s;\n", p_bottom.getFloat(), CCSSproperty::getDimName(p_bottom.getDim()).c_str());
			}

			if(p_width.isSet())
			{
				wprintf(L"\twidth: %f%s;\n", p_width.getFloat(), CCSSproperty::getDimName(p_width.getDim()).c_str());
			}
			if(p_height.isSet())
			{
				wprintf(L"\theight: %f%s;\n", p_height.getFloat(), CCSSproperty::getDimName(p_height.getDim()).c_str());
			}

			if(p_max_width.isSet())
			{
				wprintf(L"\tmax-width: %f%s;\n", p_max_width.getFloat(), CCSSproperty::getDimName(p_max_width.getDim()).c_str());
			}
			if(p_max_height.isSet())
			{
				wprintf(L"\tmax-height: %f%s;\n", p_max_height.getFloat(), CCSSproperty::getDimName(p_max_height.getDim()).c_str());
			}

			if(p_min_width.isSet())
			{
				wprintf(L"\tmin-width: %f%s;\n", p_min_width.getFloat(), CCSSproperty::getDimName(p_min_width.getDim()).c_str());
			}
			if(p_min_height.isSet())
			{
				wprintf(L"\tmin-height: %f%s;\n", p_min_height.getFloat(), CCSSproperty::getDimName(p_min_height.getDim()).c_str());
			}

			if(p_cursor.isSet())
			{
				wprintf(L"\tcursor: ");
				switch(p_cursor.getInt())
				{
				case ICSSproperty::CURSOR_AUTO:
					wprintf(L"auto;\n");
					break;
				case ICSSproperty::CURSOR_CELL:
					wprintf(L"cell;\n");
					break;
				case ICSSproperty::CURSOR_COL_RESIZE:
					wprintf(L"col-resize;\n");
					break;
				case ICSSproperty::CURSOR_COPY:
					wprintf(L"copy;\n");
					break;
				case ICSSproperty::CURSOR_CROSSHAIR:
					wprintf(L"crosshair;\n");
					break;
				case ICSSproperty::CURSOR_DEFAULT:
					wprintf(L"default;\n");
					break;
				case ICSSproperty::CURSOR_E_RESIZE:
					wprintf(L"e-resize;\n");
					break;
				case ICSSproperty::CURSOR_EW_RESIZE:
					wprintf(L"ew-resize;\n");
					break;
				case ICSSproperty::CURSOR_HELP:
					wprintf(L"help;\n");
					break;
				case ICSSproperty::CURSOR_MOVE:
					wprintf(L"move;\n");
					break;
				case ICSSproperty::CURSOR_N_RESIZE:
					wprintf(L"n-resize;\n");
					break;
				case ICSSproperty::CURSOR_NE_RESIZE:
					wprintf(L"ne-resize;\n");
					break;
				case ICSSproperty::CURSOR_NESW_RESIZE:
					wprintf(L"nesw-resize;\n");
					break;
				case ICSSproperty::CURSOR_NO_DROP:
					wprintf(L"no-drop;\n");
					break;
				case ICSSproperty::CURSOR_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::CURSOR_NOT_ALLOWED:
					wprintf(L"not-allowed;\n");
					break;
				case ICSSproperty::CURSOR_NS_RESIZE:
					wprintf(L"ns-resize;\n");
					break;
				case ICSSproperty::CURSOR_NW_RESIZE:
					wprintf(L"nw-resize;\n");
					break;
				case ICSSproperty::CURSOR_NWSE_RESIZE:
					wprintf(L"nwse-resize;\n");
					break;
				case ICSSproperty::CURSOR_POINTER:
					wprintf(L"pointer;\n");
					break;
				case ICSSproperty::CURSOR_PROGRESS:
					wprintf(L"progress;\n");
					break;
				case ICSSproperty::CURSOR_ROW_RESIZE:
					wprintf(L"row-resize;\n");
					break;
				case ICSSproperty::CURSOR_S_RESIZE:
					wprintf(L"s-resize;\n");
					break;
				case ICSSproperty::CURSOR_SE_RESIZE:
					wprintf(L"se-resize;\n");
					break;
				case ICSSproperty::CURSOR_SW_RESIZE:
					wprintf(L"sw-resize;\n");
					break;
				case ICSSproperty::CURSOR_TEXT:
					wprintf(L"text;\n");
					break;
				case ICSSproperty::CURSOR_VERTICAL_TEXT:
					wprintf(L"vertical-text;\n");
					break;
				case ICSSproperty::CURSOR_W_RESIZE:
					wprintf(L"w-resize;\n");
					break;
				case ICSSproperty::CURSOR_WAIT:
					wprintf(L"wait;\n");
					break;
				case ICSSproperty::CURSOR_INHERIT:
					wprintf(L"inherit;\n");
					break;
				case ICSSproperty::CURSOR_GRAB:
					wprintf(L"grab;\n");
					break;
				case ICSSproperty::CURSOR_GRABBING:
					wprintf(L"grabbing;\n");
					break;
				case ICSSproperty::CURSOR_ZOOM_IN:
					wprintf(L"zoom-in;\n");
					break;
				case ICSSproperty::CURSOR_ZOOM_OUT:
					wprintf(L"zoom-out;\n");
					break;
				case ICSSproperty::CURSOR_ALIAS:
					wprintf(L"alias;\n");
					break;
				case ICSSproperty::CURSOR_ALL_SCROLL:
					wprintf(L"all-scroll;\n");
					break;
				}
			}

			if(p_direction.isSet())
			{
				wprintf(L"\tdirection: ");
				switch(p_direction.getInt())
				{
				case ICSSproperty::DIRECTION_INHERIT:
					wprintf(L"inherit;\n");
					break;
				case ICSSproperty::DIRECTION_LTR:
					wprintf(L"ltr;\n");
					break;
				case ICSSproperty::DIRECTION_RTL:
					wprintf(L"rtl;\n");
					break;
				}
			}

			if(p_position.isSet())
			{
				wprintf(L"\tposition: ");
				switch(p_position.getInt())
				{
				case ICSSproperty::POSITION_STATIC:
					wprintf(L"static;\n");
					break;
				case ICSSproperty::POSITION_ABSOLUTE:
					wprintf(L"absolute;\n");
					break;
				case ICSSproperty::POSITION_RELATIVE:
					wprintf(L"relative;\n");
					break;
				case ICSSproperty::POSITION_FIXED:
					wprintf(L"fixed;\n");
					break;
				}
			}

			if(p_visibility.isSet())
			{
				wprintf(L"\tvisibility: ");
				switch(p_visibility.getInt())
				{
				case ICSSproperty::VISIBILITY_VISIBLE:
					wprintf(L"visible;\n");
					break;
				case ICSSproperty::VISIBILITY_HIDDEN:
					wprintf(L"hidden;\n");
					break;
				}
			}

			if(p_float.isSet())
			{
				wprintf(L"\tfloat: ");
				switch(p_float.getInt())
				{
				case ICSSproperty::FLOAT_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::FLOAT_LEFT:
					wprintf(L"left;\n");
					break;
				case ICSSproperty::FLOAT_RIGHT:
					wprintf(L"right;\n");
					break;
				}
			}

			if(p_float.isSet())
			{
				wprintf(L"\tfloat: ");
				switch(p_float.getInt())
				{
				case ICSSproperty::FLOAT_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::FLOAT_LEFT:
					wprintf(L"left;\n");
					break;
				case ICSSproperty::FLOAT_RIGHT:
					wprintf(L"right;\n");
					break;
				}
			}
			if(p_background_image.isSet())
			{
				wprintf(L"\tbackground-image: %s;\n", p_background_image.getString());
			}

			if(p_background_color.isSet())
			{
				wprintf(L"\tcolor: #%08X;\n", p_background_color.getInt());
			}

			if(p_background_position_x.isSet())
			{
				wprintf(L"\tbackground-position-x: %f%s;\n", p_background_position_x.getFloat(), CCSSproperty::getDimName(p_background_position_x.getDim()).c_str());
			}
			if(p_background_position_y.isSet())
			{
				wprintf(L"\tbackground-position-y: %f%s;\n", p_background_position_y.getFloat(), CCSSproperty::getDimName(p_background_position_y.getDim()).c_str());
			}

			if(p_background_size_x.isSet())
			{
				wprintf(L"\tbackground-size-x: %f%s;\n", p_background_size_x.getFloat(), CCSSproperty::getDimName(p_background_size_x.getDim()).c_str());
			}
			if(p_background_size_y.isSet())
			{
				wprintf(L"\tbackground-size-y: %f%s;\n", p_background_size_y.getFloat(), CCSSproperty::getDimName(p_background_size_y.getDim()).c_str());
			}

			if(p_background_attachment.isSet())
			{
				wprintf(L"\tbackground-attachment: ");
				switch(p_background_attachment.getInt())
				{
				case ICSSproperty::BACKGROUND_ATTACHMENT_FIXED:
					wprintf(L"fixed;\n");
					break;
				case ICSSproperty::BACKGROUND_ATTACHMENT_LOCAL:
					wprintf(L"local;\n");
					break;
				case ICSSproperty::BACKGROUND_ATTACHMENT_SCROLL:
					wprintf(L"scroll;\n");
					break;
				}
			}

			if(p_background_repeat.isSet())
			{
				wprintf(L"\tbackground-repeat: ");
				switch(p_background_repeat.getInt())
				{
				case ICSSproperty::BACKGROUND_REPEAT_NO_REPEAT:
					wprintf(L"no-repeat;\n");
					break;
				case ICSSproperty::BACKGROUND_REPEAT_REPEAT_X:
					wprintf(L"repeat-x;\n");
					break;
				case ICSSproperty::BACKGROUND_REPEAT_REPEAT_Y:
					wprintf(L"repeat-y;\n");
					break;
				case ICSSproperty::BACKGROUND_REPEAT_REPEAT:
					wprintf(L"repeat;\n");
					break;
				}
			}

			if(p_border_top_style.isSet())
			{
				wprintf(L"\tborder-top-style: ");
				switch(p_border_top_style.getInt())
				{
				case ICSSproperty::BORDER_STYLE_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::BORDER_STYLE_INSET:
					wprintf(L"inset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_OUTSET:
					wprintf(L"outset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DASHED:
					wprintf(L"dashed;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOTTED:
					wprintf(L"dotted;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOUBLE:
					wprintf(L"double;\n");
					break;
				case ICSSproperty::BORDER_STYLE_GROOVE:
					wprintf(L"groove;\n");
					break;
				case ICSSproperty::BORDER_STYLE_RIDGE:
					wprintf(L"ridge;\n");
					break;
				case ICSSproperty::BORDER_STYLE_SOLID:
					wprintf(L"solid;\n");
					break;
				}
			}
			if(p_border_left_style.isSet())
			{
				wprintf(L"\tborder-left-style: ");
				switch(p_border_left_style.getInt())
				{
				case ICSSproperty::BORDER_STYLE_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::BORDER_STYLE_INSET:
					wprintf(L"inset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_OUTSET:
					wprintf(L"outset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DASHED:
					wprintf(L"dashed;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOTTED:
					wprintf(L"dotted;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOUBLE:
					wprintf(L"double;\n");
					break;
				case ICSSproperty::BORDER_STYLE_GROOVE:
					wprintf(L"groove;\n");
					break;
				case ICSSproperty::BORDER_STYLE_RIDGE:
					wprintf(L"ridge;\n");
					break;
				case ICSSproperty::BORDER_STYLE_SOLID:
					wprintf(L"solid;\n");
					break;
				}
			}
			if(p_border_right_style.isSet())
			{
				wprintf(L"\tborder-right-style: ");
				switch(p_border_right_style.getInt())
				{
				case ICSSproperty::BORDER_STYLE_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::BORDER_STYLE_INSET:
					wprintf(L"inset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_OUTSET:
					wprintf(L"outset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DASHED:
					wprintf(L"dashed;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOTTED:
					wprintf(L"dotted;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOUBLE:
					wprintf(L"double;\n");
					break;
				case ICSSproperty::BORDER_STYLE_GROOVE:
					wprintf(L"groove;\n");
					break;
				case ICSSproperty::BORDER_STYLE_RIDGE:
					wprintf(L"ridge;\n");
					break;
				case ICSSproperty::BORDER_STYLE_SOLID:
					wprintf(L"solid;\n");
					break;
				}
			}
			if(p_border_bottom_style.isSet())
			{
				wprintf(L"\tborder-bottom-style: ");
				switch(p_border_bottom_style.getInt())
				{
				case ICSSproperty::BORDER_STYLE_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::BORDER_STYLE_INSET:
					wprintf(L"inset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_OUTSET:
					wprintf(L"outset;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DASHED:
					wprintf(L"dashed;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOTTED:
					wprintf(L"dotted;\n");
					break;
				case ICSSproperty::BORDER_STYLE_DOUBLE:
					wprintf(L"double;\n");
					break;
				case ICSSproperty::BORDER_STYLE_GROOVE:
					wprintf(L"groove;\n");
					break;
				case ICSSproperty::BORDER_STYLE_RIDGE:
					wprintf(L"ridge;\n");
					break;
				case ICSSproperty::BORDER_STYLE_SOLID:
					wprintf(L"solid;\n");
					break;
				}
			}

			if(p_border_top_width.isSet())
			{
				wprintf(L"\tborder-top-width: %f%s;\n", p_border_top_width.getFloat(), CCSSproperty::getDimName(p_border_top_width.getDim()).c_str());
			}
			if(p_border_right_width.isSet())
			{
				wprintf(L"\tborder-right-width: %f%s;\n", p_border_right_width.getFloat(), CCSSproperty::getDimName(p_border_right_width.getDim()).c_str());
			}
			if(p_border_bottom_width.isSet())
			{
				wprintf(L"\tborder-bottom-width: %f%s;\n", p_border_bottom_width.getFloat(), CCSSproperty::getDimName(p_border_bottom_width.getDim()).c_str());
			}
			if(p_border_left_width.isSet())
			{
				wprintf(L"\tborder-left-width: %f%s;\n", p_border_left_width.getFloat(), CCSSproperty::getDimName(p_border_left_width.getDim()).c_str());
			}

			if(p_border_top_color.isSet())
			{
				wprintf(L"\tborder_top_color: #%08X;\n", p_border_top_color.getInt());
			}
			if(p_border_right_color.isSet())
			{
				wprintf(L"\tborder_right_color: #%08X;\n", p_border_right_color.getInt());
			}
			if(p_border_bottom_color.isSet())
			{
				wprintf(L"\tborder_bottom_color: #%08X;\n", p_border_bottom_color.getInt());
			}
			if(p_border_left_color.isSet())
			{
				wprintf(L"\tborder_left_color: #%08X;\n", p_border_left_color.getInt());
			}

			if(p_border_top_left_radius.isSet())
			{
				wprintf(L"\tborder-top-left-radius: %f%s;\n", p_border_top_left_radius.getFloat(), CCSSproperty::getDimName(p_border_top_left_radius.getDim()).c_str());
			}
			if(p_border_top_right_radius.isSet())
			{
				wprintf(L"\tborder-top-right-radius: %f%s;\n", p_border_top_right_radius.getFloat(), CCSSproperty::getDimName(p_border_top_right_radius.getDim()).c_str());
			}

			if(p_border_bottom_left_radius.isSet())
			{
				wprintf(L"\tborder-bottom-left-radius: %f%s;\n", p_border_bottom_left_radius.getFloat(), CCSSproperty::getDimName(p_border_bottom_left_radius.getDim()).c_str());
			}
			if(p_border_bottom_right_radius.isSet())
			{
				wprintf(L"\tborder-bottom-right-radius: %f%s;\n", p_border_bottom_right_radius.getFloat(), CCSSproperty::getDimName(p_border_bottom_right_radius.getDim()).c_str());
			}

			if(p_font_name.isSet())
			{
				wprintf(L"\tfont-name: %s;\n", p_font_name.getString());
			}

			if(p_font_size.isSet())
			{
				wprintf(L"\tfont-size: %f%s;\n", p_font_size.getFloat(), CCSSproperty::getDimName(p_font_size.getDim()).c_str());
			}

			if(p_font_style.isSet())
			{
				wprintf(L"\tfont-style: ");
				switch(p_font_style.getInt())
				{
				case ICSSproperty::FONT_STYLE_NORMAL:
					wprintf(L"normal;\n");
					break;
				case ICSSproperty::FONT_STYLE_ITALIC:
					wprintf(L"italic;\n");
					break;
				case ICSSproperty::FONT_STYLE_INHERIT:
					wprintf(L"inherit;\n");
					break;
				}
			}

			if(p_font_weight.isSet())
			{
				wprintf(L"\tfont-weight: ");
				switch(p_font_weight.getInt())
				{
				case ICSSproperty::FONT_WEIGHT_NORMAL:
					wprintf(L"normal;\n");
					break;
				case ICSSproperty::FONT_WEIGHT_BOLD:
					wprintf(L"bold;\n");
					break;
				case ICSSproperty::FONT_WEIGHT_INHERIT:
					wprintf(L"inherit;\n");
					break;
				}
			}

			if(p_text_align.isSet())
			{
				wprintf(L"\ttext-align: ");
				switch(p_text_align.getInt())
				{
				case ICSSproperty::TEXT_ALIGN_LEFT:
					wprintf(L"left;\n");
					break;
				case ICSSproperty::TEXT_ALIGN_RIGHT:
					wprintf(L"right;\n");
					break;
				case ICSSproperty::TEXT_ALIGN_INHERIT:
					wprintf(L"inherit;\n");
					break;
				case ICSSproperty::TEXT_ALIGN_CENTER:
					wprintf(L"center;\n");
					break;
				}
			}

			if(p_text_decoration_line.isSet())
			{
				wprintf(L"\ttext-decoration-line: ");
				switch(p_text_decoration_line.getInt())
				{
				case ICSSproperty::TEXT_DECORATION_LINE_NONE:
					wprintf(L"none;\n");
					break;
				case ICSSproperty::TEXT_DECORATION_LINE_UNDERLINE:
					wprintf(L"underline;\n");
					break;
				case ICSSproperty::TEXT_DECORATION_LINE_LINE_THROUGH:
					wprintf(L"line-through;\n");
					break;
				case ICSSproperty::TEXT_DECORATION_LINE_OVERLINE:
					wprintf(L"overline;\n");
					break;
				}
			}

			wprintf(L"}\n\n");
		}
#endif

		void CCSSstyle::loadStyleFromString(const WCHAR * str, UINT * pNumSyms)
		{
			StringW key;
			StringW val;
			bool readKeyBgn = true;
			bool readKey = false;
			bool readValBgn = false;
			bool readVal = false;
			UINT len = (UINT)wcslen(str);
			WCHAR c;
			*pNumSyms = 0;
			for(UINT i = 0; i < len; i++)
			{
				(*pNumSyms)++;
				c = str[i];
				if(c == L'}')
				{
					break;
				}
				if(!iswspace(c))
				{
					if(c == L';') // end pair
					{
						if(!readVal)
						{
							printf("[Error]: Unexpected symbol ';' in css string\n");
						}
						else
						{
							readKey = false;
							readVal = false;
							readKeyBgn = true;
							readValBgn = false;
							//Process pair
							parsePair(key, val);
						}
					}
					else if(c == L':') // end key begin value
					{
						if(!readKey)
						{
							printf("[Error]: Unexpected symbol ':' in css string\n");
						}
						else
						{
							readKey = false;
							readValBgn = true;
						}
					}
					else
					{
						if(readKey)
						{
							key += c;
						}
						else if(readValBgn)
						{
							readValBgn = false;
							readVal = true;
							val = c;
						}
						else if(readKeyBgn)
						{
							readKeyBgn = false;
							readKey = true;
							key = c;
						}
						else if(readVal)
						{
							val += c;
						}
					}
				}
				else
				{
					if(readVal)
					{
						val += c;
					}
				}
			}
			if(readVal)
			{
				parsePair(key, val);
				//Process pair
			}


			fixupTransitions();
		}

		void CCSSstyle::parsePair(const StringW & key, const StringW & val)
		{
			if(key == L"display")
			{
				if(val == L"none")
				{
					p_display.setExt(ICSSproperty::DISPLAY_NONE);
				}
				else if(val == L"block")
				{
					p_display.setExt(ICSSproperty::DISPLAY_BLOCK);
				}
				else if(val == L"inline")
				{
					p_display.setExt(ICSSproperty::DISPLAY_INLINE);
				}
				else if(val == L"inline-block")
				{
					p_display.setExt(ICSSproperty::DISPLAY_INLINE_BLOCK);
				}
				else if(val == L"list-item")
				{
					p_display.setExt(ICSSproperty::DISPLAY_LIST_ITEM);
				}
				else if(val == L"table")
				{
					p_display.setExt(ICSSproperty::DISPLAY_TABLE);
				}
				else if(val == L"table-caption")
				{
					p_display.setExt(ICSSproperty::DISPLAY_TABLE_CAPTION);
				}
				else if(val == L"table-column")
				{
					p_display.setExt(ICSSproperty::DISPLAY_TABLE_COLUMN);
				}
				else if(val == L"table-row")
				{
					p_display.setExt(ICSSproperty::DISPLAY_TABLE_ROW);
				}
				else if(val == L"table-cell")
				{
					p_display.setExt(ICSSproperty::DISPLAY_TABLE_CELL);
				}
				else if(val == L"inherit")
				{
					p_display.setExt(ICSSproperty::DISPLAY_INHERIT);
				}
			}
			else if(key == L"color")
			{
				parseColor(&p_color, val);
			}
			else if(key == L"cursor")
			{
				if(val == L"auto")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_AUTO);
				}
				else if(val == L"cell")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_CELL);
				}
				else if(val == L"col-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_COL_RESIZE);
				}
				else if(val == L"copy")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_COPY);
				}
				else if(val == L"crosshair")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_CROSSHAIR);
				}
				else if(val == L"default")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_DEFAULT);
				}
				else if(val == L"e-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_E_RESIZE);
				}
				else if(val == L"ew-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_EW_RESIZE);
				}
				else if(val == L"help")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_HELP);
				}
				else if(val == L"move")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_MOVE);
				}
				else if(val == L"n-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_N_RESIZE);
				}
				else if(val == L"ne-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NE_RESIZE);
				}
				else if(val == L"nesw-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NESW_RESIZE);
				}
				else if(val == L"no-drop")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NO_DROP);
				}
				else if(val == L"none")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NONE);
				}
				else if(val == L"not-allowed")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NOT_ALLOWED);
				}
				else if(val == L"ns-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NS_RESIZE);
				}
				else if(val == L"nw-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NW_RESIZE);
				}
				else if(val == L"nwse-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_NWSE_RESIZE);
				}
				else if(val == L"pointer")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_POINTER);
				}
				else if(val == L"progress")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_PROGRESS);
				}
				else if(val == L"row-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_ROW_RESIZE);
				}
				else if(val == L"s-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_S_RESIZE);
				}
				else if(val == L"se-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_SE_RESIZE);
				}
				else if(val == L"sw-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_SW_RESIZE);
				}
				else if(val == L"text")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_TEXT);
				}
				else if(val == L"vertical-text")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_VERTICAL_TEXT);
				}
				else if(val == L"w-resize")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_W_RESIZE);
				}
				else if(val == L"wait")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_WAIT);
				}
				else if(val == L"inherit")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_INHERIT);
				}
				else if(val == L"grab")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_GRAB);
				}
				else if(val == L"grabbing")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_GRABBING);
				}
				else if(val == L"zoom-in")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_ZOOM_IN);
				}
				else if(val == L"zoom-out")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_ZOOM_OUT);
				}
				else if(val == L"alias")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_ALIAS);
				}
				else if(val == L"all-scroll")
				{
					p_cursor.setExt(ICSSproperty::CURSOR_ALL_SCROLL);
				}
				// FIXME: Add support for custom cursor
			}
			else if(key == L"direction")
			{
				if(val == L"inherit")
				{
					p_direction.setExt(ICSSproperty::DIRECTION_INHERIT);
				}
				else if(val == L"ltr")
				{
					p_direction.setExt(ICSSproperty::DIRECTION_LTR);
				}
				else if(val == L"rtl")
				{
					p_direction.setExt(ICSSproperty::DIRECTION_RTL);
				}
			}
			else if(key == L"opacity")
			{
				float p;
				swscanf_s(val.c_str(), L"%f", &p);
				p_opacity.set(p);
			}
			else if(key == L"z-index")
			{
				int p;
				swscanf_s(val.c_str(), L"%d", &p);
				p_z_index.set(p);
			}
			else if(key == L"overflow-x")
			{
				if(val == L"auto")
				{
					p_overflow_x.setExt(ICSSproperty::OVERFLOW_AUTO);
				}
				else if(val == L"hidden")
				{
					p_overflow_x.setExt(ICSSproperty::OVERFLOW_HIDDEN);
				}
				else if(val == L"scroll")
				{
					p_overflow_x.setExt(ICSSproperty::OVERFLOW_SCROLL);
				}
				else if(val == L"visible")
				{
					p_overflow_x.setExt(ICSSproperty::OVERFLOW_VISIBLE);
				}
			}
			else if(key == L"overflow-y")
			{
				if(val == L"auto")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_AUTO);
				}
				else if(val == L"hidden")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_HIDDEN);
				}
				else if(val == L"scroll")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_SCROLL);
				}
				else if(val == L"visible")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_VISIBLE);
				}
			}
			else if(key == L"overflow")
			{
				if(val == L"auto")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_AUTO);
				}
				else if(val == L"hidden")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_HIDDEN);
				}
				else if(val == L"scroll")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_SCROLL);
				}
				else if(val == L"visible")
				{
					p_overflow_y.setExt(ICSSproperty::OVERFLOW_VISIBLE);
				}
				p_overflow_x = p_overflow_y;
			}
			else if(key == L"margin-left")
			{
				parseValue(&p_margin_left, val);
			}
			else if(key == L"margin-right")
			{
				parseValue(&p_margin_right, val);
			}
			else if(key == L"margin-top")
			{
				parseValue(&p_margin_top, val);
			}
			else if(key == L"margin-bottom")
			{
				parseValue(&p_margin_bottom, val);
			}
			else if(key == L"margin")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_margin_bottom, val);
					p_margin_top = p_margin_left = p_margin_right = p_margin_bottom;
					break;
				case 2:
					parseValue(&p_margin_top, val);
					parseValue(&p_margin_right, StringW(&(val.c_str()[parts[1]])));
					p_margin_bottom = p_margin_top;
					p_margin_left = p_margin_right;
					break;
				case 3:
					parseValue(&p_margin_top, val);
					parseValue(&p_margin_right, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_margin_bottom, StringW(&(val.c_str()[parts[2]])));
					p_margin_left = p_margin_right;
					break;
				default: // 4 or more
					parseValue(&p_margin_top, val);
					parseValue(&p_margin_right, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_margin_bottom, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_margin_left, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"padding-left")
			{
				parseValue(&p_padding_left, val);
			}
			else if(key == L"padding-right")
			{
				parseValue(&p_padding_right, val);
			}
			else if(key == L"padding-top")
			{
				parseValue(&p_padding_top, val);
			}
			else if(key == L"padding-bottom")
			{
				parseValue(&p_padding_bottom, val);
			}
			else if(key == L"padding")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_padding_bottom, val);
					p_padding_top = p_padding_left = p_padding_right = p_padding_bottom;
					break;
				case 2:
					parseValue(&p_padding_top, val);
					parseValue(&p_padding_right, StringW(&(val.c_str()[parts[1]])));
					p_padding_bottom = p_padding_top;
					p_padding_left = p_padding_right;
					break;
				case 3:
					parseValue(&p_padding_top, val);
					parseValue(&p_padding_right, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_padding_bottom, StringW(&(val.c_str()[parts[2]])));
					p_padding_left = p_padding_right;
					break;
				default: // 4 or more
					parseValue(&p_padding_top, val);
					parseValue(&p_padding_right, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_padding_bottom, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_padding_left, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"top")
			{
				parseValue(&p_top, val);
			}
			else if(key == L"left")
			{
				parseValue(&p_left, val);
			}
			else if(key == L"right")
			{
				parseValue(&p_right, val);
			}
			else if(key == L"bottom")
			{
				parseValue(&p_bottom, val);
			}
			else if(key == L"width")
			{
				parseValue(&p_width, val);
			}
			else if(key == L"height")
			{
				parseValue(&p_height, val);
			}
			else if(key == L"min-width")
			{
				parseValue(&p_min_width, val);
			}
			else if(key == L"min-height")
			{
				parseValue(&p_min_height, val);
			}
			else if(key == L"max-width")
			{
				parseValue(&p_max_width, val);
			}
			else if(key == L"max-height")
			{
				parseValue(&p_max_height, val);
			}
			else if(key == L"position")
			{
				if(val == L"static")
				{
					p_position.setExt(ICSSproperty::POSITION_STATIC);
				}
				else if(val == L"absolute")
				{
					p_position.setExt(ICSSproperty::POSITION_ABSOLUTE);
				}
				else if(val == L"relative")
				{
					p_position.setExt(ICSSproperty::POSITION_RELATIVE);
				}
				else if(val == L"fixed")
				{
					p_position.setExt(ICSSproperty::POSITION_FIXED);
				}
			}
			else if(key == L"visibility")
			{
				if(val == L"visible")
				{
					p_visibility.setExt(ICSSproperty::VISIBILITY_VISIBLE);
				}
				else if(val == L"hidden")
				{
					p_visibility.setExt(ICSSproperty::VISIBILITY_HIDDEN);
				}
			}
			else if(key == L"float")
			{
				if(val == L"none")
				{
					p_float.setExt(ICSSproperty::FLOAT_NONE);
				}
				else if(val == L"left")
				{
					p_float.setExt(ICSSproperty::FLOAT_LEFT);
				}
				else if(val == L"right")
				{
					p_float.setExt(ICSSproperty::FLOAT_RIGHT);
				}
			}
			else if(key == L"background-image")
			{
				p_background_image.set(val);
			}
			else if(key == L"background-color")
			{
				parseColor(&p_background_color, val);
			}
			else if(key == L"background-position-x")
			{
				parseValue(&p_background_position_x, val);
			}
			else if(key == L"background-position-y")
			{
				parseValue(&p_background_position_y, val);
			}
			else if(key == L"background-attachment")
			{
				if(val == L"fixed")
				{
					p_background_attachment.setExt(ICSSproperty::BACKGROUND_ATTACHMENT_FIXED);
				}
				else if(val == L"local")
				{
					p_background_attachment.setExt(ICSSproperty::BACKGROUND_ATTACHMENT_LOCAL);
				}
				else if(val == L"scroll")
				{
					p_background_attachment.setExt(ICSSproperty::BACKGROUND_ATTACHMENT_SCROLL);
				}
			}
			else if(key == L"background-repeat")
			{
				if(val == L"no-repeat")
				{
					p_background_repeat.setExt(ICSSproperty::BACKGROUND_REPEAT_NO_REPEAT);
				}
				else if(val == L"repeat-x")
				{
					p_background_repeat.setExt(ICSSproperty::BACKGROUND_REPEAT_REPEAT_X);
				}
				else if(val == L"repeat-y")
				{
					p_background_repeat.setExt(ICSSproperty::BACKGROUND_REPEAT_REPEAT_Y);
				}
				else if(val == L"repeat")
				{
					p_background_repeat.setExt(ICSSproperty::BACKGROUND_REPEAT_REPEAT);
				}
			}
			else if(key == L"background-size-x")
			{
				parseValue(&p_background_size_x, val);
			}
			else if(key == L"background-size-y")
			{
				parseValue(&p_background_size_y, val);
			}
			else if(key == L"border-top-style")
			{
				if(val == L"none")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_NONE);
				}
				else if(val == L"inset")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_INSET);
				}
				else if(val == L"outset")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_OUTSET);
				}
				else if(val == L"dashed")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_DASHED);
				}
				else if(val == L"dotted")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_DOTTED);
				}
				else if(val == L"double")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_DOUBLE);
				}
				else if(val == L"groove")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_GROOVE);
				}
				else if(val == L"ridge")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_RIDGE);
				}
				else if(val == L"solid")
				{
					p_border_top_style.setExt(ICSSproperty::BORDER_STYLE_SOLID);
				}
			}
			else if(key == L"border-top-width")
			{
				parseValue(&p_border_top_width, val);
			}
			else if(key == L"border-top-left-radius")
			{
				parseValue(&p_border_top_left_radius, val);
			}
			else if(key == L"border-top-right-radius")
			{
				parseValue(&p_border_top_right_radius, val);
			}
			/*else if(key == L"border-radius")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_border_top_left_radius, val);
					p_border_bottom_right_radius = p_border_top_right_radius = p_border_bottom_left_radius = p_border_top_left_radius;
					break;
				case 2:
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					p_border_bottom_left_radius = p_border_top_left_radius;
					p_border_bottom_right_radius = p_border_top_right_radius;
					break;
				case 3:
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_radius, StringW(&(val.c_str()[parts[2]])));
					p_border_bottom_left_radius = p_border_bottom_right_radius;
					break;
				default: // 4 or more
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_radius, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_border_bottom_left_radius, StringW(&(val.c_str()[parts[3]])));
				}
			}*/
			/*else if(key == L"border-method")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					if(val == L"radius")
					{
						p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
					}
					else if(val == L"cut")
					{
						p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
					}
					p_border_bottom_right_method = p_border_top_right_method = p_border_bottom_left_method = p_border_top_left_method;
					break;
				case 2:
					if(val == L"radius")
					{
						p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
					}
					else if(val == L"cut")
					{
						p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
					}
					parseValue(&p_border_top_left_method, val);
					parseValue(&p_border_top_right_method, StringW(&(val.c_str()[parts[1]])));
					p_border_bottom_left_method = p_border_top_left_method;
					p_border_bottom_right_method = p_border_top_right_method;
					break;
				case 3:
					parseValue(&p_border_top_left_method, val);
					parseValue(&p_border_top_right_method, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_method, StringW(&(val.c_str()[parts[2]])));
					p_border_bottom_left_method = p_border_bottom_right_method;
					break;
				default: // 4 or more
					parseValue(&p_border_top_left_method, val);
					parseValue(&p_border_top_right_method, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_method, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_border_bottom_left_method, StringW(&(val.c_str()[parts[3]])));
				}
			}*/
			else if(key == L"border-top-right-method")
			{
				if(val == L"radius")
				{
					p_border_top_right_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
				}
				else if(val == L"cut")
				{
					p_border_top_right_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
				}
			}
			else if(key == L"border-top-left-method")
			{
				if(val == L"radius")
				{
					p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
				}
				else if(val == L"cut")
				{
					p_border_top_left_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
				}
			}
			else if(key == L"border-bottom-right-method")
			{
				if(val == L"radius")
				{
					p_border_bottom_right_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
				}
				else if(val == L"cut")
				{
					p_border_bottom_right_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
				}
			}
			else if(key == L"border-bottom-left-method")
			{
				if(val == L"radius")
				{
					p_border_bottom_left_method.setExt(ICSSproperty::BORDER_METHOD_RADIUS);
				}
				else if(val == L"cut")
				{
					p_border_bottom_left_method.setExt(ICSSproperty::BORDER_METHOD_CUT);
				}
			}
			else if(key == L"border-right-style")
			{
				if(val == L"none")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_NONE);
				}
				else if(val == L"inset")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_INSET);
				}
				else if(val == L"outset")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_OUTSET);
				}
				else if(val == L"dashed")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DASHED);
				}
				else if(val == L"dotted")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DOTTED);
				}
				else if(val == L"double")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DOUBLE);
				}
				else if(val == L"groove")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_GROOVE);
				}
				else if(val == L"ridge")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_RIDGE);
				}
				else if(val == L"solid")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_SOLID);
				}
			}
			else if(key == L"border-right-width")
			{
				parseValue(&p_border_right_width, val);
			}
			else if(key == L"border-left-style")
			{
				if(val == L"none")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_NONE);
				}
				else if(val == L"inset")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_INSET);
				}
				else if(val == L"outset")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_OUTSET);
				}
				else if(val == L"dashed")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_DASHED);
				}
				else if(val == L"dotted")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_DOTTED);
				}
				else if(val == L"double")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_DOUBLE);
				}
				else if(val == L"groove")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_GROOVE);
				}
				else if(val == L"ridge")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_RIDGE);
				}
				else if(val == L"solid")
				{
					p_border_left_style.setExt(ICSSproperty::BORDER_STYLE_SOLID);
				}
			}
			else if(key == L"border-style")
			{
				if(val == L"none")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_NONE);
				}
				else if(val == L"inset")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_INSET);
				}
				else if(val == L"outset")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_OUTSET);
				}
				else if(val == L"dashed")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DASHED);
				}
				else if(val == L"dotted")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DOTTED);
				}
				else if(val == L"double")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_DOUBLE);
				}
				else if(val == L"groove")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_GROOVE);
				}
				else if(val == L"ridge")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_RIDGE);
				}
				else if(val == L"solid")
				{
					p_border_right_style.setExt(ICSSproperty::BORDER_STYLE_SOLID);
				}
				p_border_left_style = p_border_top_style = p_border_bottom_style = p_border_right_style;
			}
			else if(key == L"border-left-width")
			{
				parseValue(&p_border_left_width, val);
			}
			else if(key == L"border-bottom-style")
			{
				if(val == L"none")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_NONE);
				}
				else if(val == L"inset")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_INSET);
				}
				else if(val == L"outset")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_OUTSET);
				}
				else if(val == L"dashed")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_DASHED);
				}
				else if(val == L"dotted")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_DOTTED);
				}
				else if(val == L"double")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_DOUBLE);
				}
				else if(val == L"groove")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_GROOVE);
				}
				else if(val == L"ridge")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_RIDGE);
				}
				else if(val == L"solid")
				{
					p_border_bottom_style.setExt(ICSSproperty::BORDER_STYLE_SOLID);
				}
			}
			else if(key == L"border-bottom-width")
			{
				parseValue(&p_border_bottom_width, val);
			}
			else if(key == L"border-width")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_border_top_width, val);
					p_border_left_width = p_border_right_width = p_border_bottom_width = p_border_top_width;
					break;
				case 2:
					parseValue(&p_border_top_width, val);
					parseValue(&p_border_right_width, StringW(&(val.c_str()[parts[1]])));
					p_border_bottom_width = p_border_top_width;
					p_border_left_width = p_border_right_width;
					break;
				case 3:
					parseValue(&p_border_top_width, val);
					parseValue(&p_border_right_width, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_width, StringW(&(val.c_str()[parts[2]])));
					p_border_left_width = p_border_right_width;
					break;
				default: // 4 or more
					parseValue(&p_border_top_width, val);
					parseValue(&p_border_right_width, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_width, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_border_left_width, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"border-radius")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_border_top_left_radius, val);
					p_border_top_right_radius = p_border_bottom_left_radius = p_border_bottom_right_radius = p_border_top_left_radius;
					break;
				case 2:
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					p_border_bottom_left_radius = p_border_top_left_radius;
					p_border_bottom_right_radius = p_border_top_right_radius;
					break;
				case 3:
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_radius, StringW(&(val.c_str()[parts[2]])));
					p_border_bottom_left_radius = p_border_top_left_radius;
					break;
				default: // 4 or more
					parseValue(&p_border_top_left_radius, val);
					parseValue(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_border_bottom_right_radius, StringW(&(val.c_str()[parts[2]])));
					parseValue(&p_border_bottom_left_radius, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"border-bottom-left-radius")
			{
				parseValue(&p_border_bottom_left_radius, val);
			}
			else if(key == L"border-bottom-right-radius")
			{
				parseValue(&p_border_bottom_right_radius, val);
			}
			else if(key == L"font-name")
			{
				p_font_name.set(val);
			}
			else if(key == L"font-size")
			{
				parseValue(&p_font_size, val);
			}
			else if(key == L"font-style")
			{
				if(val == L"inherit")
				{
					p_font_style.setExt(ICSSproperty::FONT_STYLE_INHERIT);
				}
				else if(val == L"italic")
				{
					p_font_style.setExt(ICSSproperty::FONT_STYLE_ITALIC);
				}
				else if(val == L"normal")
				{
					p_font_style.setExt(ICSSproperty::FONT_STYLE_NORMAL);
				}
			}
			else if(key == L"-gui-text-appearance")
			{
				if(val == L"multi-line")
				{
					p__gui_text_appearance.setExt(ICSSproperty::_GUI_TEXT_APPEARANCE_MULTI_LINE);
				}
				else if(val == L"one-line")
				{
					p__gui_text_appearance.setExt(ICSSproperty::_GUI_TEXT_APPEARANCE_ONE_LINE);
				}
			}
			else if(key == L"font-weight")
			{
				if(val == L"bold")
				{
					p_font_weight.setExt(ICSSproperty::FONT_WEIGHT_BOLD);
				}
				else if(val == L"inherit")
				{
					p_font_weight.setExt(ICSSproperty::FONT_WEIGHT_INHERIT);
				}
				else if(val == L"normal")
				{
					p_font_weight.setExt(ICSSproperty::FONT_WEIGHT_NORMAL);
				}
			}
			else if(key == L"text-align")
			{
				if(val == L"inherit")
				{
					p_text_align.setExt(ICSSproperty::TEXT_ALIGN_INHERIT);
				}
				else if(val == L"left")
				{
					p_text_align.setExt(ICSSproperty::TEXT_ALIGN_LEFT);
				}
				else if(val == L"right")
				{
					p_text_align.setExt(ICSSproperty::TEXT_ALIGN_RIGHT);
				}
				else if(val == L"center")
				{
					p_text_align.setExt(ICSSproperty::TEXT_ALIGN_CENTER);
				}
			}
			else if(key == L"text-decoration-line")
			{
				if(val == L"none")
				{
					p_text_decoration_line.setExt(ICSSproperty::TEXT_DECORATION_LINE_NONE);
				}
				else if(val == L"underline")
				{
					p_text_decoration_line.setExt(ICSSproperty::TEXT_DECORATION_LINE_UNDERLINE);
				}
				else if(val == L"line-through")
				{
					p_text_decoration_line.setExt(ICSSproperty::TEXT_DECORATION_LINE_LINE_THROUGH);
				}
				else if(val == L"overline")
				{
					p_text_decoration_line.setExt(ICSSproperty::TEXT_DECORATION_LINE_OVERLINE);
				}
			}
			else if(key == L"border-color")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px 9px
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseColor(&p_border_top_color, val);
					p_border_left_color = p_border_bottom_color = p_border_right_color = p_border_top_color;
					break;
				case 2:
					parseColor(&p_border_top_color, val);
					parseColor(&p_border_right_color, StringW(&(val.c_str()[parts[1]])));
					p_border_bottom_color = p_border_top_color;
					p_border_left_color = p_border_right_color;
					break;
				case 3:
					parseColor(&p_border_top_color, val);
					parseColor(&p_border_right_color, StringW(&(val.c_str()[parts[1]])));
					parseColor(&p_border_bottom_color, StringW(&(val.c_str()[parts[2]])));
					p_border_bottom_color = p_border_top_color;
					break;
				default: // 4 or more
					parseColor(&p_border_top_color, val);
					parseColor(&p_border_top_right_radius, StringW(&(val.c_str()[parts[1]])));
					parseColor(&p_border_bottom_color, StringW(&(val.c_str()[parts[2]])));
					parseColor(&p_border_left_color, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"border-top-color")
			{
				parseColor(&p_border_top_color, val);
			}
			else if(key == L"border-left-color")
			{
				parseColor(&p_border_left_color, val);
			}
			else if(key == L"border-bottom-color")
			{
				parseColor(&p_border_bottom_color, val);
			}
			else if(key == L"border-right-color")
			{
				parseColor(&p_border_right_color, val);
			}
			else if(key == L"-gui-text-cursor")
			{
				if(val == L"hide")
				{
					p__gui_text_cursor.setExt(ICSSproperty::_GUI_TEXT_CURSOR_HIDE);
				}
				else if(val == L"show")
				{
					p__gui_text_cursor.setExt(ICSSproperty::_GUI_TEXT_CURSOR_SHOW);
				}
			}
			else if(key == L"-gui-text-format")
			{
				if(val == L"default")
				{
					p__gui_text_format.setExt(ICSSproperty::_GUI_TEXT_FORMAT_DEFAULT);
				}
				else if(val == L"preformatted")
				{
					p__gui_text_format.setExt(ICSSproperty::_GUI_TEXT_FORMAT_PREFORMATTED);
				}
				
			}
			else if(key == L"-gui-z")
			{
				parseValue(&p__gui_z, val);
			}
			else if(key == L"transform")
			{
				parseTransform(&p_transform, val);
			}
			else if(key == L"text-shadow")
			{
				// 1px				partcount = 1
				// 1px 5px
				// 1px 5px 7px
				// 1px 5px 7px #ccc
				UINT l = val.length();
				UINT partcount = 1;
				bool issp = true;
				UINT parts[4] = {0};
				for(UINT i = 0; i < l; i++)
				{
					if(val[i] == L' ')
					{
						if(!issp)
						{
							issp = true;
							partcount++;
						}
					}
					else
					{
						if(issp)
						{
							issp = false;
							parts[partcount - 1] = i;
						}
					}
				}
				switch(partcount)
				{
				case 1:
					parseValue(&p_text_shadow_offset_x, val);
					break;
				case 2:
					parseValue(&p_text_shadow_offset_x, val);
					parseValue(&p_text_shadow_offset_y, StringW(&(val.c_str()[parts[1]])));
					break;
				case 3:
					parseValue(&p_text_shadow_offset_x, val);
					parseValue(&p_text_shadow_offset_y, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_text_shadow_blur_radius, StringW(&(val.c_str()[parts[2]])));
					break;
				default: // 4 or more
					parseValue(&p_text_shadow_offset_x, val);
					parseValue(&p_text_shadow_offset_y, StringW(&(val.c_str()[parts[1]])));
					parseValue(&p_text_shadow_blur_radius, StringW(&(val.c_str()[parts[2]])));
					parseColor(&p_text_shadow_color, StringW(&(val.c_str()[parts[3]])));
				}
			}
			else if(key == L"-gui-spin-percent")
			{
				parseValue(&p__gui_spin_percent, val);
			}
			else if(key == L"transition")
			{
				printf(COLOR_LRED "transition property is not supporter, use 'transition-duration', 'transition-property', 'transition-timing-function', 'transition-delay' instead\n" COLOR_RESET);
			}
			else if(key == L"transition-duration")
			{
				const wchar_t * szProp = val.c_str();
				float fTime;
				int i = 0;
				while(parseTimeArgs(&szProp, &fTime))
				{
					m_aTransitions[i++].m_fDuration = fTime;
					if(!parseNextArgs(&szProp))
					{
						break;
					}
				}
			}
			else if(key == L"transition-delay")
			{
				const wchar_t * szProp = val.c_str();
				float fTime;
				int i = 0;
				while(parseTimeArgs(&szProp, &fTime))
				{
					m_aTransitions[i++].m_fDelay = fTime;
					if(!parseNextArgs(&szProp))
					{
						break;
					}
				}
			}
			else if(key == L"transition-property")
			{
				const wchar_t * szProp = val.c_str();
				int i = 0;

				do
				{
					while(*szProp && iswspace(*szProp))
					{
						++szProp;
					}

					if(!*szProp)
					{
						break;
					}

					const wchar_t * szProp2 = szProp;
					while(*szProp2 && !iswspace(*szProp2) && *szProp2 != L',')
					{
						++szProp2;
					}

					wchar_t *prop = (wchar_t*)alloca(sizeof(wchar_t)* (szProp2 - szProp + 1));
					memcpy(prop, szProp, sizeof(wchar_t)* (szProp2 - szProp));
					prop[szProp2 - szProp] = 0;
					m_aTransitions[i++].m_szProperty = prop;
					szProp = szProp2;
				}
				while(parseNextArgs(&szProp));
			}
			else if(key == L"transition-timing-function")
			{
				const wchar_t * szProp = val.c_str();
				int i = 0;

				do
				{
					while(*szProp && iswspace(*szProp))
					{
						++szProp;
					}

					if(!*szProp)
					{
						break;
					}

					const wchar_t * szProp2 = szProp;
					while(*szProp2 && !iswspace(*szProp2) && *szProp2 != L',')
					{
						++szProp2;
					}

					wchar_t *prop = (wchar_t*)alloca(sizeof(wchar_t)* (szProp2 - szProp + 1));
					memcpy(prop, szProp, sizeof(wchar_t)* (szProp2 - szProp));
					prop[szProp2 - szProp] = 0;

					ICSSproperty::TIMING_FUNCTION_e tf = ICSSproperty::TIMING_FUNCTION_EASE;
					if(lstrcmpW(prop, L"ease") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_EASE;
					}
					else if(lstrcmpW(prop, L"ease-in") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_EASE_IN;
					}
					else if(lstrcmpW(prop, L"ease-out") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_EASE_OUT;
					}
					else if(lstrcmpW(prop, L"ease-in-out") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_EASE_IN_OUT;
					}
					else if(lstrcmpW(prop, L"linear") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_LINEAR;
					}
					else if(lstrcmpW(prop, L"step-start") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_STEP_START;
					}
					else if(lstrcmpW(prop, L"step-end") == 0)
					{
						tf = ICSSproperty::TIMING_FUNCTION_STEP_END;
					}

					m_aTransitions[i++].m_timingFunction = tf;
					szProp = szProp2;
				}
				while(parseNextArgs(&szProp));
			}
		}

		void CCSSstyle::parseTransform(ICSSproperty * prop, const StringW & val)
		{
			const wchar_t * szProp = val.c_str();
			SMMATRIX mRes;
			while(*szProp)
			{
				while(*szProp && iswspace(*szProp))
				{
					++szProp;
				}
#define TMP_IS_KW(kw) memcmp(szProp, kw, lstrlenW(kw) * sizeof(wchar_t)) == 0 && (szProp += lstrlenW(kw))
				if(TMP_IS_KW(L"rotateX"))
				{
					if(parseBeginArgs(&szProp))
					{
						float fAngle;
						if(parseAngleArgs(&szProp, &fAngle))
						{
							if(parseEndArgs(&szProp))
							{
								mRes *= SMMatrixRotationX(fAngle);
								continue;
							}
						}
					}
				}
				if(TMP_IS_KW(L"rotateY"))
				{
					if(parseBeginArgs(&szProp))
					{
						float fAngle;
						if(parseAngleArgs(&szProp, &fAngle))
						{
							if(parseEndArgs(&szProp))
							{
								mRes *= SMMatrixRotationY(fAngle);
								continue;
							}
						}
					}
				}
				else if(TMP_IS_KW(L"rotateZ"))
				{
					if(parseBeginArgs(&szProp))
					{
						float fAngle;
						if(parseAngleArgs(&szProp, &fAngle))
						{
							if(parseEndArgs(&szProp))
							{
								mRes *= SMMatrixRotationZ(fAngle);
								continue;
							}
						}
					}
				}
				else if(TMP_IS_KW(L"rotate3d"))
				{
					float3_t fDir;
					float fAngle;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &fDir.x)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &fDir.y)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &fDir.z)
						&& parseNextArgs(&szProp)
						&& parseAngleArgs(&szProp, &fAngle)
						&& parseEndArgs(&szProp)
					)
					{
						mRes *= SMQuaternion(fDir, fAngle).GetMatrix();
						continue;
					}
				}
				else if(TMP_IS_KW(L"scale3d"))
				{
					float3_t vScale;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &vScale.x)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &vScale.y)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &vScale.z)
						&& parseEndArgs(&szProp)
					)
					{
						mRes *= SMMatrixScaling(vScale);
						continue;
					}
				}
				else if(TMP_IS_KW(L"scaleX"))
				{
					float fScale;
					if(
						parseBeginArgs(&szProp)
						&& parseAngleArgs(&szProp, &fScale)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixScaling(fScale, 1.0f, 1.0f);
						continue;
					}
				}
				else if(TMP_IS_KW(L"scaleY"))
				{
					float fScale;
					if(
						parseBeginArgs(&szProp)
						&& parseAngleArgs(&szProp, &fScale)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixScaling(1.0f, fScale, 1.0f);
						continue;
					}
				}
				else if(TMP_IS_KW(L"scaleZ"))
				{
					float fScale;
					if(
						parseBeginArgs(&szProp)
						&& parseAngleArgs(&szProp, &fScale)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixScaling(1.0f, 1.0f, fScale);
						continue;
					}
				}
				else if(TMP_IS_KW(L"scale"))
				{
					float2_t vScale;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &vScale.x)
						)
					{
						if(
							parseNextArgs(&szProp)
							&& parseNumberArgs(&szProp, &vScale.y)
							)
						{
							mRes *= SMMatrixScaling(vScale.x, vScale.y, 1.0f);
						}
						else
						{
							mRes *= SMMatrixScaling(vScale.x, vScale.x, 1.0f);
						}
						if(parseEndArgs(&szProp))
						{
							continue;
						}
					}
				}
				else if(TMP_IS_KW(L"translateX"))
				{
					float fVal;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &fVal)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixTranslation(fVal, 0.0f, 0.0f);
						continue;
					}
				}
				else if(TMP_IS_KW(L"translateY"))
				{
					float fVal;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &fVal)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixTranslation(0.0f, fVal, 0.0f);
						continue;
					}
				}
				else if(TMP_IS_KW(L"translateZ"))
				{
					float fVal;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &fVal)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixTranslation(0.0f, 0.0f, fVal);
						continue;
					}
				}
				else if(TMP_IS_KW(L"translate3d"))
				{
					float3_t vVal;
					if(
						parseBeginArgs(&szProp)
						&& parseNumberArgs(&szProp, &vVal.x)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &vVal.y)
						&& parseNextArgs(&szProp)
						&& parseNumberArgs(&szProp, &vVal.z)
						&& parseEndArgs(&szProp)
						)
					{
						mRes *= SMMatrixTranslation(vVal);
						continue;
					}
				}
				printf(COLOR_YELLOW "Unable to parse css transform string: '%s'\n" COLOR_RESET, String(val).c_str());
				return;
#undef TMP_IS_KW
			}
			//prop->set(SMMatrixIdentity());
			//prop->set(SMMatrixRotationZ(SMToRadian(30.0f)));
			//prop->set(SMMatrixScaling(0.5f, 1.0f, 1.0f));
			prop->set(mRes);
		}

		bool CCSSstyle::parseAngleArgs(const wchar_t ** pszStr, float * pfOut)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			wchar_t szDim[5];
			memset(szDim, 0, sizeof(szDim));
			int n = swscanf_s(*pszStr, L"%f%[adegr]", pfOut, szDim, sizeof(szDim) / sizeof(szDim[0]));
			if(n < 2)
			{
				return(false);
			}
			if(wcscmp(szDim, L"deg") == 0)
			{
				*pfOut = SMToRadian(*pfOut);
			}
			else if(wcscmp(szDim, L"rad") == 0)
			{
				// do nothing
			}
			else if(wcscmp(szDim, L"grad") == 0)
			{
				*pfOut *= SM_2PI / 400.0f;
			}
			else if(wcscmp(szDim, L"turn") == 0)
			{
				*pfOut *= SM_2PI;
			}
			else
			{
				return(false);
			}
			while(**pszStr && (
				(**pszStr >= L'0' && **pszStr <= L'9')
				|| **pszStr == L'.' || **pszStr == L'-'
			))
			{
				++(*pszStr);
			}
			*pszStr += wcslen(szDim);
			return(true);
		}

		bool CCSSstyle::parseTimeArgs(const wchar_t ** pszStr, float * pfOut)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			wchar_t szDim[5];
			memset(szDim, 0, sizeof(szDim));
			int n = swscanf_s(*pszStr, L"%f%[ms]", pfOut, szDim, sizeof(szDim) / sizeof(szDim[0]));
			if(n < 2)
			{
				return(false);
			}
			if(wcscmp(szDim, L"ms") == 0)
			{
				*pfOut = *pfOut * 0.001f;
			}
			else if(wcscmp(szDim, L"s") == 0)
			{
				// do nothing
			}
			else
			{
				return(false);
			}
			while(**pszStr && (
				(**pszStr >= L'0' && **pszStr <= L'9')
				|| **pszStr == L'.' || **pszStr == L'-'
				))
			{
				++(*pszStr);
			}
			*pszStr += wcslen(szDim);
			return(true);
		}
		
		bool CCSSstyle::parseNumberArgs(const wchar_t ** pszStr, float * pfOut)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			//wchar_t szDim[5];
			//memset(szDim, 0, sizeof(szDim));
			int n = swscanf_s(*pszStr, L"%f", pfOut);
			if(n < 1)
			{
				return(false);
			}
			
			while(**pszStr && (
				(**pszStr >= L'0' && **pszStr <= L'9')
				|| **pszStr == L'.' || **pszStr == L'-'
				))
			{
				++(*pszStr);
			}
			return(true);
		}

		bool CCSSstyle::parseBeginArgs(const wchar_t ** pszStr)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			if(**pszStr == L'(')
			{
				++(*pszStr);
				return(**pszStr != 0);
			}
			return(false);
		}

		bool CCSSstyle::parseNextArgs(const wchar_t ** pszStr)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			if(**pszStr == L',')
			{
				++(*pszStr);
				return(**pszStr != 0);
			}
			return(false);
		}

		bool CCSSstyle::parseEndArgs(const wchar_t ** pszStr)
		{
			while(**pszStr && iswspace(**pszStr))
			{
				++(*pszStr);
			}
			if(**pszStr == L')')
			{
				++(*pszStr);
				return(true);
			}
			return(false);
		}
		
		void CCSSstyle::parseColor(ICSSproperty * clr, const StringW & val)
		{
			if(val.c_str()[0] == L'#') // hex color
			{
				UINT c;
				WCHAR str[9];
				str[8] = 0;
				if(val.length() == 4)
				{
					str[0] = str[1] = val[1];
					str[2] = str[3] = val[2];
					str[4] = str[5] = val[3];
					str[6] = str[7] = L'F';
				}
				else if(val.length() == 7)
				{
					memcpy(str, &(val.c_str()[1]), sizeof(WCHAR) * 6);
					str[6] = str[7] = L'F';
				}
				else if(val.length() == 9)
				{
					memcpy(str, &(val.c_str()[1]), sizeof(WCHAR) * 8);
				}
				else
				{
					str[0] = str[1] = str[2] = str[3] = str[4] = str[5] = L'0';
					str[6] = str[7] = L'F';
				}
				swscanf_s(str, L"%x", &c);
				clr->set((int)c);
			}
			else
			{
				printf("[Error]: Unsupported color type in css: '%s'\n", String(val).c_str());
			}
		}

		void CCSSstyle::parseValue(ICSSproperty * clr, const StringW & val)
		{
			float p;
			WCHAR a = 0, b;
			StringW _val = val;
			UINT l = val.length();
			UINT i;
			for(i = 0; i < l; i++)
			{
				if(!((val[i] >= L'0' && val[i] <= L'9') || val[i] == L'.'))
				{
					a = val[i];
					if(a == L'e')
					{
						_val[i] = L'!';
					}
					break;
				}
			}

			int n = swscanf_s(_val.c_str(), L"%f%c%c", &p, &a, 1, &b, 1);
			clr->set(p);
			if(n == 1)
			{
				clr->setDim(ICSSproperty::DIM_NONE);
			}
			else if(n == 2)
			{
				if(a == L'%')
				{
					clr->setDim(ICSSproperty::DIM_PC);
				}
				else
				{
					printf("[Error]: Unknown dimension '%c' in css string\n", (char)a);
				}
			}
			else if(n == 3)
			{
				if(a == L'!' && b == L'm')
				{
					clr->setDim(ICSSproperty::DIM_EM);
				}
				else if(a == L'm' && b == L'm')
				{
					clr->setDim(ICSSproperty::DIM_MM);
				}
				else if(a == L'p' && b == L't')
				{
					clr->setDim(ICSSproperty::DIM_PT);
				}
				else if(a == L'p' && b == L'x')
				{
					clr->setDim(ICSSproperty::DIM_PX);
				}
				else
				{
					printf("[Error]: Unknown dimension '%c%c' in css string\n", (char)a, (char)b);
				}
			}
		}

		UINT CCSSstyle::getChangesFlags()
		{
			return((p_display.isChanged() ? p_display.getFlags() : 0) |

				(p_color.isChanged() ? p_color.getFlags() : 0) |

				(p_cursor.isChanged() ? p_cursor.getFlags() : 0) |

				(p_direction.isChanged() ? p_direction.getFlags() : 0) |

				(p_opacity.isChanged() ? p_opacity.getFlags() : 0) |

				(p_z_index.isChanged() ? p_z_index.getFlags() : 0) |

				(p_overflow_x.isChanged() ? p_overflow_x.getFlags() : 0) |
				(p_overflow_y.isChanged() ? p_overflow_y.getFlags() : 0) |

				(p_margin_left.isChanged() ? p_margin_left.getFlags() : 0) |
				(p_margin_right.isChanged() ? p_margin_right.getFlags() : 0) |
				(p_margin_top.isChanged() ? p_margin_top.getFlags() : 0) |
				(p_margin_bottom.isChanged() ? p_margin_bottom.getFlags() : 0) |

				(p_padding_left.isChanged() ? p_padding_left.getFlags() : 0) |
				(p_padding_right.isChanged() ? p_padding_right.getFlags() : 0) |
				(p_padding_top.isChanged() ? p_padding_top.getFlags() : 0) |
				(p_padding_bottom.isChanged() ? p_padding_bottom.getFlags() : 0) |

				(p_top.isChanged() ? p_top.getFlags() : 0) |
				(p_left.isChanged() ? p_left.getFlags() : 0) |
				(p_right.isChanged() ? p_right.getFlags() : 0) |
				(p_bottom.isChanged() ? p_bottom.getFlags() : 0) |

				(p_width.isChanged() ? p_width.getFlags() : 0) |
				(p_height.isChanged() ? p_height.getFlags() : 0) |

				(p_max_width.isChanged() ? p_max_width.getFlags() : 0) |
				(p_max_height.isChanged() ? p_max_height.getFlags() : 0) |

				(p_min_width.isChanged() ? p_min_width.getFlags() : 0) |
				(p_min_height.isChanged() ? p_min_height.getFlags() : 0) |

				(p_position.isChanged() ? p_position.getFlags() : 0) |

				(p_visibility.isChanged() ? p_visibility.getFlags() : 0) |

				(p_float.isChanged() ? p_float.getFlags() : 0) |

				(p_background_image.isChanged() ? p_background_image.getFlags() : 0) |
				(p_background_color.isChanged() ? p_background_color.getFlags() : 0) |
				(p_background_position_x.isChanged() ? p_background_position_x.getFlags() : 0) |
				(p_background_position_y.isChanged() ? p_background_position_y.getFlags() : 0) |
				(p_background_attachment.isChanged() ? p_background_attachment.getFlags() : 0) |//
				(p_background_repeat.isChanged() ? p_background_repeat.getFlags() : 0) |//
				(p_background_size_x.isChanged() ? p_background_size_x.getFlags() : 0) |
				(p_background_size_y.isChanged() ? p_background_size_y.getFlags() : 0) |


				(p_border_top_style.isChanged() ? p_border_top_style.getFlags() : 0) |//
				(p_border_top_width.isChanged() ? p_border_top_width.getFlags() : 0) |
				(p_border_top_color.isChanged() ? p_border_top_color.getFlags() : 0) |
				(p_border_top_left_radius.isChanged() ? p_border_top_left_radius.getFlags() : 0) |
				(p_border_top_right_radius.isChanged() ? p_border_top_right_radius.getFlags() : 0) |
				(p_border_top_left_method.isChanged() ? p_border_top_left_method.getFlags() : 0) |
				(p_border_top_right_method.isChanged() ? p_border_top_right_method.getFlags() : 0) |

				(p_border_right_style.isChanged() ? p_border_right_style.getFlags() : 0) |//
				(p_border_right_width.isChanged() ? p_border_right_width.getFlags() : 0) |
				(p_border_right_color.isChanged() ? p_border_right_color.getFlags() : 0) |

				(p_border_bottom_style.isChanged() ? p_border_bottom_style.getFlags() : 0) |//
				(p_border_bottom_width.isChanged() ? p_border_bottom_width.getFlags() : 0) |
				(p_border_bottom_color.isChanged() ? p_border_bottom_color.getFlags() : 0) |
				(p_border_bottom_left_radius.isChanged() ? p_border_bottom_left_radius.getFlags() : 0) |
				(p_border_bottom_right_radius.isChanged() ? p_border_bottom_right_radius.getFlags() : 0) |
				(p_border_bottom_left_method.isChanged() ? p_border_bottom_left_method.getFlags() : 0) |
				(p_border_bottom_right_method.isChanged() ? p_border_bottom_right_method.getFlags() : 0) |

				(p_border_left_style.isChanged() ? p_border_left_style.getFlags() : 0) |//
				(p_border_left_width.isChanged() ? p_border_left_width.getFlags() : 0) |
				(p_border_left_color.isChanged() ? p_border_left_color.getFlags() : 0) |


				(p_font_name.isChanged() ? p_font_name.getFlags() : 0) |
				(p_font_size.isChanged() ? p_font_size.getFlags() : 0) |
				(p_font_style.isChanged() ? p_font_style.getFlags() : 0) |
				(p_font_weight.isChanged() ? p_font_weight.getFlags() : 0) |

				(p_text_align.isChanged() ? p_text_align.getFlags() : 0) |
				(p_text_decoration_line.isChanged() ? p_text_decoration_line.getFlags() : 0) |
				(p__gui_text_appearance.isChanged() ? p__gui_text_appearance.getFlags() : 0) |
				(p__gui_text_cursor.isChanged() ? p__gui_text_cursor.getFlags() : 0) |
				(p__gui_text_format.isChanged() ? p__gui_text_format.getFlags() : 0) |
				(p__gui_z.isChanged() ? p__gui_z.getFlags() : 0) |
				(p__gui_spin_percent.isChanged() ? p__gui_spin_percent.getFlags() : 0) |
				(p_transform.isChanged() ? p_transform.getFlags() : 0) |

				(p_text_shadow_offset_x.isChanged() ? p_text_shadow_offset_x.getFlags() : 0) |
				(p_text_shadow_offset_y.isChanged() ? p_text_shadow_offset_y.getFlags() : 0) |
				(p_text_shadow_blur_radius.isChanged() ? p_text_shadow_blur_radius.getFlags() : 0) |
				(p_text_shadow_color.isChanged() ? p_text_shadow_color.getFlags() : 0)
			);
		}

		void CCSSstyle::resetChanges()
		{
			p_display.unChange();

			p_color.unChange();

			p_cursor.unChange();

			p_direction.unChange();

			p_opacity.unChange();

			p_z_index.unChange();

			p_overflow_x.unChange();
			p_overflow_y.unChange();

			p_margin_left.unChange();
			p_margin_right.unChange();
			p_margin_top.unChange();
			p_margin_bottom.unChange();

			p_padding_left.unChange();
			p_padding_right.unChange();
			p_padding_top.unChange();
			p_padding_bottom.unChange();

			p_top.unChange();
			p_left.unChange();
			p_right.unChange();
			p_bottom.unChange();

			p_width.unChange();
			p_height.unChange();

			p_max_width.unChange();
			p_max_height.unChange();

			p_min_width.unChange();
			p_min_height.unChange();

			p_position.unChange();

			p_visibility.unChange();

			p_float.unChange();

			p_background_image.unChange();
			p_background_color.unChange();
			p_background_position_x.unChange();
			p_background_position_y.unChange();
			p_background_attachment.unChange(); //
			p_background_repeat.unChange(); //
			p_background_size_x.unChange();
			p_background_size_y.unChange();


			p_border_top_style.unChange(); //
			p_border_top_width.unChange();
			p_border_top_color.unChange();
			p_border_top_left_radius.unChange();
			p_border_top_right_radius.unChange();
			p_border_top_left_method.unChange();
			p_border_top_right_method.unChange();

			p_border_right_style.unChange(); //
			p_border_right_width.unChange();
			p_border_right_color.unChange();

			p_border_bottom_style.unChange(); //
			p_border_bottom_width.unChange();
			p_border_bottom_color.unChange();
			p_border_bottom_left_radius.unChange();
			p_border_bottom_right_radius.unChange();
			p_border_bottom_left_method.unChange();
			p_border_bottom_right_method.unChange();

			p_border_left_style.unChange(); //
			p_border_left_width.unChange();
			p_border_left_color.unChange();


			p_font_name.unChange();
			p_font_size.unChange();
			p_font_style.unChange();
			p_font_weight.unChange();

			p_text_align.unChange();
			p_text_decoration_line.unChange();

			p_text_shadow_offset_x.unChange();
			p_text_shadow_offset_y.unChange();
			p_text_shadow_blur_radius.unChange();
			p_text_shadow_color.unChange();

			p__gui_text_appearance.unChange();
			p__gui_text_cursor.unChange();
			p__gui_text_format.unChange();
			p__gui_z.unChange();
			p__gui_spin_percent.unChange();
			p_transform.unChange();
		}

#define CSS_CAPTURE(p) if(!((p) == pOldStyles->p)){(p).setChanged(true);(p).setupTransition(&pOldStyles->p, pDoc);}else{(p).setChanged(false);}
		void CCSSstyle::captureChanges(const ICSSstyle * _pOldStyles, dom::CDOMdocument *pDoc)
		{
			m_pDoc = pDoc;

			const CCSSstyle * pOldStyles = (const CCSSstyle *)_pOldStyles;
			CSS_CAPTURE(p_display);

			CSS_CAPTURE(p_color);

			CSS_CAPTURE(p_cursor);

			CSS_CAPTURE(p_direction);

			CSS_CAPTURE(p_opacity);

			CSS_CAPTURE(p_z_index);

			CSS_CAPTURE(p_overflow_x);
			CSS_CAPTURE(p_overflow_y);

			CSS_CAPTURE(p_margin_left);
			CSS_CAPTURE(p_margin_right);
			CSS_CAPTURE(p_margin_top);
			CSS_CAPTURE(p_margin_bottom);

			CSS_CAPTURE(p_padding_left);
			CSS_CAPTURE(p_padding_right);
			CSS_CAPTURE(p_padding_top);
			CSS_CAPTURE(p_padding_bottom);

			CSS_CAPTURE(p_top);
			CSS_CAPTURE(p_left);
			CSS_CAPTURE(p_right);
			CSS_CAPTURE(p_bottom);

			CSS_CAPTURE(p_width);
			CSS_CAPTURE(p_height);

			CSS_CAPTURE(p_max_width);
			CSS_CAPTURE(p_max_height);

			CSS_CAPTURE(p_min_width);
			CSS_CAPTURE(p_min_height);

			CSS_CAPTURE(p_position);

			CSS_CAPTURE(p_visibility);

			CSS_CAPTURE(p_float);

			CSS_CAPTURE(p_background_image);
			CSS_CAPTURE(p_background_color);
			CSS_CAPTURE(p_background_position_x);
			CSS_CAPTURE(p_background_position_y);
			CSS_CAPTURE(p_background_attachment); //
			CSS_CAPTURE(p_background_repeat); //
			CSS_CAPTURE(p_background_size_x);
			CSS_CAPTURE(p_background_size_y);


			CSS_CAPTURE(p_border_top_style); //
			CSS_CAPTURE(p_border_top_width);
			CSS_CAPTURE(p_border_top_color);
			CSS_CAPTURE(p_border_top_left_radius);
			CSS_CAPTURE(p_border_top_right_radius);
			CSS_CAPTURE(p_border_top_left_method);
			CSS_CAPTURE(p_border_top_right_method);

			CSS_CAPTURE(p_border_right_style); //
			CSS_CAPTURE(p_border_right_width);
			CSS_CAPTURE(p_border_right_color);

			CSS_CAPTURE(p_border_bottom_style); //
			CSS_CAPTURE(p_border_bottom_width);
			CSS_CAPTURE(p_border_bottom_color);
			CSS_CAPTURE(p_border_bottom_left_radius);
			CSS_CAPTURE(p_border_bottom_right_radius);
			CSS_CAPTURE(p_border_bottom_left_method);
			CSS_CAPTURE(p_border_bottom_right_method);

			CSS_CAPTURE(p_border_left_style); //
			CSS_CAPTURE(p_border_left_width);
			CSS_CAPTURE(p_border_left_color);


			CSS_CAPTURE(p_font_name);
			CSS_CAPTURE(p_font_size);
			CSS_CAPTURE(p_font_style);
			CSS_CAPTURE(p_font_weight);

			CSS_CAPTURE(p_text_align);
			CSS_CAPTURE(p_text_decoration_line);

			CSS_CAPTURE(p_text_shadow_offset_x);
			CSS_CAPTURE(p_text_shadow_offset_y);
			CSS_CAPTURE(p_text_shadow_blur_radius);
			CSS_CAPTURE(p_text_shadow_color);

			CSS_CAPTURE(p__gui_text_appearance);
			CSS_CAPTURE(p__gui_text_cursor);

			CSS_CAPTURE(p__gui_text_format);
			CSS_CAPTURE(p__gui_z);
			CSS_CAPTURE(p__gui_spin_percent);
			CSS_CAPTURE(p_transform);
		}
#undef CSS_CAPTURE

		void CCSSstyle::reset()
		{
			p_display.unset();

			p_color.unset();

			p_cursor.unset();

			p_direction.unset();

			p_opacity.unset();

			p_z_index.unset();

			p_overflow_x.unset();
			p_overflow_y.unset();

			p_margin_left.unset();
			p_margin_right.unset();
			p_margin_top.unset();
			p_margin_bottom.unset();

			p_padding_left.unset();
			p_padding_right.unset();
			p_padding_top.unset();
			p_padding_bottom.unset();

			p_top.unset();
			p_left.unset();
			p_right.unset();
			p_bottom.unset();

			p_width.unset();
			p_height.unset();

			p_max_width.unset();
			p_max_height.unset();

			p_min_width.unset();
			p_min_height.unset();

			p_position.unset();

			p_visibility.unset();

			p_float.unset();

			p_background_image.unset();
			p_background_color.unset();
			p_background_position_x.unset();
			p_background_position_y.unset();
			p_background_attachment.unset(); //
			p_background_repeat.unset(); //
			p_background_size_x.unset();
			p_background_size_y.unset();


			p_border_top_style.unset(); //
			p_border_top_width.unset();
			p_border_top_color.unset();
			p_border_top_left_radius.unset();
			p_border_top_right_radius.unset();
			p_border_top_left_method.unset();
			p_border_top_right_method.unset();

			p_border_right_style.unset(); //
			p_border_right_width.unset();
			p_border_right_color.unset();

			p_border_bottom_style.unset(); //
			p_border_bottom_width.unset();
			p_border_bottom_color.unset();
			p_border_bottom_left_radius.unset();
			p_border_bottom_right_radius.unset();
			p_border_bottom_left_method.unset();
			p_border_bottom_right_method.unset();

			p_border_left_style.unset(); //
			p_border_left_width.unset();
			p_border_left_color.unset();


			p_font_name.unset();
			p_font_size.unset();
			p_font_style.unset();
			p_font_weight.unset();

			p_text_align.unset();
			p_text_decoration_line.unset();

			p_text_shadow_offset_x.unset();
			p_text_shadow_offset_y.unset();
			p_text_shadow_blur_radius.unset();
			p_text_shadow_color.unset();

			p__gui_text_appearance.unset();
			p__gui_text_cursor.unset();
			p__gui_text_format.unset();
			p__gui_z.unset();
			p__gui_spin_percent.unset();
			p_transform.unset();
		}

		void CCSSstyle::fixupTransitions()
		{
			for(int i = m_aTransitions.size() - 1; i >= 0; --i)
			{
				if(m_aTransitions[i].m_szProperty.length() == 0)
				{
					m_aTransitions.erase(i);
				}
			}
		}

		/*void ICSS::ReloadAllStyles()
		{
			for(AssotiativeArray<StringW, ICSSstyleSet>::Iterator i = m_styleSets.begin(); i; ++i)
			{
				ICSSstyleSet css(this);
				css.LoadFromString(*i.first);
				m_styleSets[*i.first] = css;
			}
		}*/
	};
};