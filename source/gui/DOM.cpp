#include "GUIbase.h"

#include "DOM.h"
#include "DOMdocument.h"

namespace gui
{
	namespace dom
	{
		/*CSS::ICSSstyle * IDOMnodeTag::GetStyle()
		{
			return(m_pStyle);
		}
		
		const String & IDOMnodeTag::GetID()
		{
		return("");
		}

		void IDOMnodeTag::SetID(const String & str)
		{

		}*/

		//##########################################################################

		CDOMnode::~CDOMnode()
		{
			while(m_vChilds.size())
			{
				IDOMnode * pNode = m_vChilds[0];
				removeChild(pNode);
				mem_delete(pNode);
			}
		}
		
		IDOMnode * CDOMnode::parentNode()
		{
			return(m_pParent);
		}

		const StringW & CDOMnode::getNodeNameById(UINT nid)
		{
			/*if(nid == 0)
			{
			static StringW text(L"text");
			return(text);
			}
			else */if(IDOMnodeTag::GetNodeVTbls().size() > nid)
			{
				return(IDOMnodeTag::GetNodeVTbls()[nid].wsNodeName);
			}
			else
			{
				static StringW empty(L"");
				return(empty);
			}
		}

		int CDOMnode::getNodeIdByName(const StringW &tag)
		{
			if(IDOMnodeTag::GetNodeIDs().KeyExists(tag))
			{
				return(IDOMnodeTag::GetNodeIDs()[tag]);
			}
			return(-1);
		}

#ifdef _DEBUG
		void CDOMnode::debugPrintMe(UINT lvl)
		{
			static UINT nTEXT = CDOMnode::getNodeIdByName(L"text");

			if(m_iNodeId == nTEXT)
			{
				return;
			}
			for(UINT i = 0; i < lvl; i++)
			{
				wprintf(L"  ");
			}
			wprintf(L"<%s", CDOMnode::getNodeNameById(m_iNodeId).c_str());
			for(AssotiativeArray<StringW, StringW>::Iterator i = m_mAttributes.begin(); i; ++i)
			{
				wprintf(L" %s=\"%s\"", i.first->c_str(), i.second->c_str());
			}
			wprintf(L">\n");
			for(int i = 0, s = m_vChilds.size(); i < s; i++)
			{
				((CDOMnode*)m_vChilds[i])->debugPrintMe(lvl + 1);
			}
			for(UINT i = 0; i < lvl; i++)
			{
				wprintf(L"  ");
			}
			wprintf(L"</%s>\n", CDOMnode::getNodeNameById(m_iNodeId).c_str());
		}
#endif
		
		void CDOMnode::resetStyles()
		{
			m_css.reset();
			for(int i = 0, s = m_vChilds.size(); i < s; i++)
			{
				((CDOMnode*)m_vChilds[i])->resetStyles();
			}
		}

		void CDOMnode::storeStyles()
		{
			m_cssOld = m_css;
			for(int i = 0, s = m_vChilds.size(); i < s; i++)
			{
				((CDOMnode*)m_vChilds[i])->storeStyles();
			}
		}

		void CDOMnode::captureStyleChanges(CDOMdocument *pDoc)
		{
			m_css.captureChanges(&m_cssOld, pDoc);
			for(int i = 0, s = m_vChilds.size(); i < s; i++)
			{
				((CDOMnode*)m_vChilds[i])->captureStyleChanges(pDoc);
			}
		}

		void CDOMnode::resetStyleChanges()
		{
			m_bSkipStructureChanges = false;
			m_css.resetChanges();
			for(int i = 0, s = m_vChilds.size(); i < s; i++)
			{
				((CDOMnode*)m_vChilds[i])->resetStyleChanges();
			}
		}
		
		const StringW & CDOMnode::getAttribute(const StringW & name)
		{
			if(m_mAttributes.KeyExists(name))
			{
				return(m_mAttributes[name]);
			}
			static StringW empt(L"");
			return(empt);
		}

#define APPLY_RULE(rule) if(!node->m_css.rule.isSet()){node->m_css.rule = m_css.rule;}
		void CDOMnode::applyChildStyle(bool noSelf)
		{
			if(!noSelf)
			{
				CDOMnode::applyCSSrules(&m_css_self, this);
			}
			UINT c = m_vChilds.size();
			for(UINT i = 0; i < c; i++)
			{
				CDOMnode * node = (CDOMnode*)m_vChilds[i];
				APPLY_RULE(p_color);
				APPLY_RULE(p_cursor);
				APPLY_RULE(p_direction);

				APPLY_RULE(p_font_name);
				APPLY_RULE(p_font_size);
				APPLY_RULE(p_font_style);
				APPLY_RULE(p_font_weight);

				APPLY_RULE(p_text_align);
				APPLY_RULE(p_text_decoration_line);
				APPLY_RULE(p_visibility);

				node->m_css.inheritTransitions(&m_css);
				node->m_css.setupTransitions(&node->m_css);
				//APPLY_RULE(p_transition_property);
				//APPLY_RULE(p_transition_duration);

				node->applyChildStyle();
			}
		}
#undef APPLY_RULE

		bool CDOMnode::isChildOf(IDOMnode * pNode)
		{
			IDOMnode * pCur = parentNode();
			while(pCur)
			{
				if(pCur == pNode)
				{
					return(true);
				}
				pCur = pCur->parentNode();
			}
			return(false);
		}

		bool CDOMnode::isAfter(IDOMnode * pNode)
		{
			IDOMnode * pCur = prevSibling();
			while(pCur)
			{
				if(pCur == pNode)
				{
					return(true);
				}
				pCur = pCur->prevSibling();
			}
			return(false);
		}

		void CDOMnode::setDocument(CDOMdocument * doc)
		{
			if(!m_pDocument)
			{
				doc->IndexSetNode(this);
			}

			m_pDocument = doc;


			for(int i = 0, l = m_vChilds.size(); i < l; ++i)
			{
				CDOMnode *pNode = (CDOMnode*)m_vChilds[i];
				pNode->setDocument(doc);
			}
		}
	};
};