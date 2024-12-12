#ifndef _ICSS_H_
#define _ICSS_H_

#include "GUIbase.h"
#include "ICSSstyle.h"
#include <math.h>

namespace gui
{
	namespace dom
	{
		class CDOMdocument;
	};
	namespace css
	{
		class ICSS;
		class ICSSstyle;
		class CCSSstyle;
		
		class ICSSrule
		{
			friend class dom::CDOMdocument;
		public:

			ICSSrule()
			{
			}
			/*enum SELECTOR
			{
			SEELCTOR_ID = 1,
			SELECTOR_CLASS,
			SELECTOR_TAG,
			SELECTOR_PSEUDOCLASS,
			SELECTOR_ANY,

			COMBINATOR_BOTH = 8,
			COMBINATOR_SIMPLE,
			COMBINATOR_PARENT
			};*/

			void release()
			{
				m_selectors.clear();
			}

			enum PSEUDOCLASS
			{
				PSEUDOCLASS_NONE = 0x00000,
				PSEUDOCLASS_ACTIVE = 0x00001,
				PSEUDOCLASS_CHECKED = 0x00002,
				PSEUDOCLASS_DISABLED = 0x00004,
				PSEUDOCLASS_EMPTY = 0x00008,
				PSEUDOCLASS_ENABLED = 0x00010,
				PSEUDOCLASS_FIRST_CHILD = 0x00020,
				PSEUDOCLASS_FIRST_OF_TYPE = 0x00040,
				PSEUDOCLASS_HOVER = 0x00080,
				PSEUDOCLASS_LAST_CHILD = 0x00100,
				PSEUDOCLASS_LAST_OF_TYPE = 0x00200,
				PSEUDOCLASS_ONLY_OF_TYPE = 0x00400,
				PSEUDOCLASS_ONLY_CHILD = 0x00800,
				PSEUDOCLASS_READ_ONLY = 0x01000,
				PSEUDOCLASS_READ_WRITE = 0x02000,
				//requires arg
				PSEUDOCLASS_NTH_CHILD = 0x04000,
				PSEUDOCLASS_NTH_LAST_CHILD = 0x08000,
				PSEUDOCLASS_NTH_OF_TYPE = 0x10000,
				PSEUDOCLASS_NTH_LAST_OF_TYPE = 0x20000,
				PSEUDOCLASS_NTH_REPEAT = 0x40000,
				//
				PSEUDOCLASS_FOCUS = 0x80000
			};

			static const UINT PSEUDOCLASS_COUNT;

			enum PSEUDOELEMENT
			{
				PSEUDOELEMENT_NONE,
				PSEUDOELEMENT_FIRST_LETTER,
				PSEUDOELEMENT_FIRST_LINE,
				PSEUDOELEMENT_SELECTION
			};

			enum CONNECTOR
			{
				CONNECTOR_NONE,
				CONNECTOR_SIMPLE, // ' '
				CONNECTOR_CHILD, // '>'
				CONNECTOR_NEXT, // '+'
				CONNECTOR_AFTER // '~'
			};

			struct ICSSselectorItem
			{
				UINT flags = 0;
				CONNECTOR connector = CONNECTOR_NONE;

				StringW s_id;
				StringW s_tag;
				Array<StringW> s_class;
				UINT dom_id = 0;
				int dom_tag = -1;
				Array<UINT> dom_class;

				UINT nth_num = 0; // :nth-child(2n+1) nth_num = 2
				UINT nth_offset = 0; // :nth-child(2n+1) nth_offset = 1

				bool bSelectAny = false; // is '*' selector

				UINT pseudoclass = PSEUDOCLASS_NONE;
				PSEUDOELEMENT pseudoelement = PSEUDOELEMENT_NONE;

				void setPseudoClass(const StringW & str);

				void setPseudoElement(const StringW & str);

				void addClass(const StringW & str)
				{
					s_class.push_back(str);
				}

				void buildIndex(ICSS * css);
			};

			void addSelector(const ICSSselectorItem & item)
			{
				m_selectors.push_back(item);
			}

#ifdef _DEBUG
			void debugDumpStyles();
#endif


		protected:
			Array<ICSSselectorItem> m_selectors;
		};

		typedef Array<ICSSrule> ICSSRuleSet;

		class ICSSstyleSet
		{
			friend class dom::CDOMdocument;
		public:
			ICSSstyleSet():m_pCSS(NULL)
			{
			}
			ICSSstyleSet(ICSS * css)
			{
				m_pCSS = css;
			}
			void loadFromString(const WCHAR * str);

			void setMaxWidth(int iMaxWidth)
			{
				m_iMaxWidth = iMaxWidth;
			}

			bool isEnabledForWidth(int iWidth)
			{
				return(m_iMaxWidth < 0 || m_iMaxWidth >= iWidth);
			}

#ifdef _DEBUG
			void debugDumpStyles();
#endif

		protected:
			Array<CCSSstyle> m_pRules;

			ICSS * m_pCSS;

			int m_iMaxWidth = -1;
		};

		class ICSS
		{
			friend class dom::CDOMdocument;
		public:
			ICSS(dom::CDOMdocument *doc, CDesktopStack *pDesktopStack):
				m_pDocument(doc),
				m_pDesktopStack(pDesktopStack)
			{
				dropStyles();
			}

			void addFile(const WCHAR * szFile, int iMaxWidth = -1);
			void addStyle(const WCHAR *wszText, int iMaxWidth = -1);
			void dropStyles();

			//	void RebuildIndex();
			//	void ParseCSSstring(const WCHAR * str);

#ifdef _DEBUG
			void debugDumpStyles();
#endif

			//	void ReloadAllStyles();

			dom::CDOMdocument * getDoc()
			{
				return(m_pDocument);
			}


		protected:
			AssotiativeArray<StringW, ICSSstyleSet> m_styleSets;
			Array<ICSSstyleSet*> m_styleOrder;

			CDesktopStack *m_pDesktopStack;
			dom::CDOMdocument *m_pDocument;
			// AssotiativeArray<ICSSrule, ICSSstyle> m_rules;
			//	AssotiativeArray<String, String> m_mProperties;
		};
	};
};

#endif
