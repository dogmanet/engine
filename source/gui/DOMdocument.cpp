#include "GUIbase.h"
#include "DOMdocument.h"
#include "ITEXT.h"

#include "IScrollBar.h"

#include "IHTMLparser.h"

//#include "CSSstyle.h"


namespace gui
{
	namespace dom
	{
		IDOMnode* CDOMdocument::createNode(const wchar_t *tag)
		{
			CDOMnode* pNode = (CDOMnode*)IDOMnodeTag::createNode(tag);
			pNode->setDocument(this);
			return(pNode);
		}

		IDOMnode* CDOMdocument::createNode(UINT nid)
		{
			CDOMnode* pNode = (CDOMnode*)CDOMnode::createNode(nid);
			pNode->setDocument(this);
			return(pNode);
		}

		void CDOMdocument::setRootNode(IDOMnode *pNode)
		{
			mem_delete(m_pRootNode);
			m_pRootNode = pNode;
			IndexSetNode((CDOMnode*)pNode);
			//	IndexBuild();
			loadStyles();
			//	GetCSS()->DebugDumpStyles();
			calculateStyles();
			buildRenderThree();
		}

		void CDOMdocument::indexBuild()
		{
			//	m_IndexStringById.clear();
			m_IndexById.clear();
			//	m_IndexStringByClass.clear();
			m_IndexByTagName.clear();

			//@@TODO:Invalidate CSS cache

			CDOMdocument::buildIndexFunc(this, m_pRootNode);
		}

		void CDOMdocument::indexAdd(IDOMnode *pNode)
		{
			indexBuild();
		}

		void CDOMdocument::indexRemove(IDOMnode *pNode)
		{
			indexBuild();
		}

		UINT CDOMdocument::getIndexForIdString(const StringW &id)
		{
			if(id.length() == 0)
			{
				return(0);
			}
			if(m_IndexStringById.KeyExists(id))
			{
				return(m_IndexStringById[id]);
			}
			else
			{
				UINT i = m_IndexStringById.Size() + 1;
				m_IndexStringById[id] = i;
				return(i);
			}
		}

		UINT CDOMdocument::getIndexForClassString(const StringW &id)
		{
			if(id.length() == 0)
			{
				return(0);
			}
			if(m_IndexStringByClass.KeyExists(id))
			{
				return(m_IndexStringByClass[id]);
			}
			else
			{
				UINT i = m_IndexStringByClass.Size() + 1;
				m_IndexStringByClass[id] = i;
				return(i);
			}
		}

		int CDOMdocument::getIndexForTagString(const StringW &id)
		{
			return(CDOMnode::getNodeIdByName(id));
		}

		void CDOMdocument::buildIndexFunc(IDOMdocument *_doc, IDOMnode *__node)
		{
			return;
			CDOMdocument * doc = (CDOMdocument*)_doc;
			CDOMnode * _node = (CDOMnode*)__node;
			if(_node->isTextNode())
			{
				return;
			}
			//IDOMnodeTag * pNode = (IDOMnodeTag*)_node;

			StringW id = _node->getAttribute(L"id");
			StringW cls = _node->getAttribute(L"class");

			if(id.length())
			{
				UINT iid = doc->getIndexForIdString(id);
				doc->m_IndexById[iid] = _node;
			}

			if(cls.length())
			{
				UINT pos = 0;
				while(true)
				{
					pos = cls.find(L" ");
					if(pos != (UINT)(-1))
					{
						StringW c = cls.substr(0, pos);
						UINT icls = doc->getIndexForClassString(cls);
						doc->m_IndexByClass[icls].push_back(_node);
						cls = cls.substr(pos);
					}
					else
					{
						break;
					}
				}
				//	UINT icls = doc->GetIndexForClassString(cls);
				//	doc->m_IndexByClass[icls].push_back(_node);
			}

			UINT itid = _node->m_iNodeId;
			doc->m_IndexByTagName[itid].push_back(_node);

			for(UINT i = 0, c = _node->m_vChilds.size(); i < c; i++)
			{
				CDOMdocument::buildIndexFunc(doc, _node->m_vChilds[i]);
			}
		}

		IDOMnode* CDOMdocument::getElementById(const StringW &id)
		{
			if(m_IndexStringById.KeyExists(id))
			{
				return(getElementById(m_IndexStringById[id]));
			}
			return(NULL);
		}

		IDOMnode* CDOMdocument::getElementById(UINT iid)
		{
			if(m_IndexById.size() > iid)
			{
				return(m_IndexById[iid]);
			}
			return(NULL);
		}

		const IDOMnodeCollection* CDOMdocument::getElementsByClass(const StringW &id)
		{
			if(m_IndexStringByClass.KeyExists(id))
			{
				return(getElementsByClass(m_IndexStringByClass[id]));
			}
			return(NULL);
		}

		const IDOMnodeCollection* CDOMdocument::getElementsByClass(UINT cid)
		{
			if(m_IndexByClass.size() > cid)
			{
				return(&m_IndexByClass[cid]);
			}
			return(NULL);
		}

		void CDOMdocument::setDesktop(IDesktop *pdp)
		{
			m_pDesktop = (CDesktop*)pdp;
		}

		const IDOMnodeCollection* CDOMdocument::getElementsByPseudoClass(UINT _cid)
		{
			UINT cid = _log2(_cid);
			if(m_IndexByPseudoClass.size() > cid)
			{
				return(&m_IndexByPseudoClass[cid]);
			}
			return(NULL);
		}

		const IDOMnodeCollection* CDOMdocument::getElementsByTag(const StringW &id)
		{
			UINT _nid = CDOMnode::getNodeIdByName(id);
			return(getElementsByTag(_nid));
		}

		const IDOMnodeCollection* CDOMdocument::getElementsByTag(UINT tid)
		{
			if(m_IndexByTagName.size() > tid)
			{
				return(&m_IndexByTagName[tid]);
			}
			return(NULL);
		}

		css::ICSS * CDOMdocument::getCSS()
		{
			return(m_pCSS);
		}

		void CDOMdocument::loadStyles()
		{
			const IDOMnodeCollection *nodes = getElementsByTag(CDOMnode::getNodeIdByName(L"link"));
			if(nodes)
			{
				UINT c = nodes->size();
				for(UINT i = 0; i < c; i++)
				{
					IDOMnode * node = (*nodes)[i];
					if(node->getAttribute(L"rel") == L"stylesheet")
					{
						const StringW &wsMaxWidth = node->getAttribute(L"max-width");
						int iMaxWidth = -1;
						if(wsMaxWidth.length() > 0)
						{
							iMaxWidth = wsMaxWidth.toInt();
						}
						m_pCSS->addFile(node->getAttribute(L"href").c_str(), iMaxWidth);
					}
				}
			}
		}

		void CDOMdocument::calculateStyles()
		{
			updateStyleSubtree(m_pRootNode);
			updateStyles(0);
		}

		void CDOMdocument::indexUnsetId(UINT id)
		{
			m_IndexById[id] = NULL;
		}

		void CDOMdocument::indexSetId(UINT id, IDOMnode *pNode)
		{
			m_IndexById[id] = pNode;
		}

		void CDOMdocument::indexUnsetClass(UINT id, IDOMnode *pNode)
		{
			IDOMnodeCollection * els = &m_IndexByClass[id];
			for(UINT i = 0; i < els->size(); i++)
			{
				if((*els)[i] == pNode)
				{
					els->erase(i);
					break;
				}
			}
		}

		void CDOMdocument::indexSetClass(UINT id, IDOMnode *pNode)
		{
			m_IndexByClass[id].push_back(pNode);
		}

		void CDOMdocument::indexUnsetPseudoClass(UINT _id, IDOMnode *pNode)
		{
			UINT id = _log2(_id);
			IDOMnodeCollection * els = &m_IndexByPseudoClass[id];
			for(UINT i = 0; i < els->size(); i++)
			{
				if((*els)[i] == pNode)
				{
					els->erase(i);
					break;
				}
			}
		}

		void CDOMdocument::indexSetPseudoClass(UINT _id, IDOMnode *pNode)
		{
			UINT id = _log2(_id);
			m_IndexByPseudoClass[id].push_back(pNode);
		}

		void CDOMdocument::indexUnsetNode(CDOMnode *pNode)
		{
			IDOMnodeCollection *els = &m_IndexByTagName[pNode->m_iNodeId];
			for(UINT i = 0; i < els->size(); i++)
			{
				if((*els)[i] == pNode)
				{
					els->erase(i);
					break;
				}
			}
		}

		void CDOMdocument::IndexSetNode(CDOMnode *pNode)
		{
			m_IndexByTagName[pNode->m_iNodeId].push_back(pNode);
		}

		IDOMnodeCollection CDOMdocument::querySelectorSimple(const css::ICSSrule::ICSSselectorItem * sel)
		{
			IDOMnodeCollection current;
			if(sel->dom_id > 0)
			{
				current.push_back(getElementById(sel->dom_id));
			}
			if(sel->dom_class.size())
			{
				for(UINT k = 0; k < sel->dom_class.size(); k++)
				{
					UINT cid = sel->dom_class[k];
					const IDOMnodeCollection * c = getElementsByClass(cid);
					if(c)
					{
						for(UINT l = 0; l < c->size(); l++)
						{
							IDOMnode * n = (*c)[l];
							bool bFound = false;
							for(UINT m = 0; m < current.size(); m++)
							{
								if(current[m] == n)
								{
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								current.push_back(n);
							}
						}
					}
				}
			}
			if(sel->dom_tag >= 0)
			{
				const IDOMnodeCollection * c = getElementsByTag(sel->dom_tag);
				if(c)
				{
					for(UINT l = 0; l < c->size(); l++)
					{
						IDOMnode * n = (*c)[l];
						bool bFound = false;
						for(UINT m = 0; m < current.size(); m++)
						{
							if(current[m] == n)
							{
								bFound = true;
								break;
							}
						}
						if(!bFound)
						{
							current.push_back(n);
						}
					}
				}
			}

			if(sel->pseudoclass != 0)
			{
				for(UINT k = 0; k < css::ICSSrule::PSEUDOCLASS_COUNT; k++)
				{
					UINT cid = 1 << k;
					if(sel->pseudoclass & cid)
					{
						const IDOMnodeCollection * c = getElementsByPseudoClass(cid);
						if(c)
						{
							for(UINT l = 0; l < c->size(); l++)
							{
								IDOMnode * n = (*c)[l];
								bool bFound = false;
								for(UINT m = 0; m < current.size(); m++)
								{
									if(current[m] == n)
									{
										bFound = true;
										break;
									}
								}
								if(!bFound)
								{
									current.push_back(n);
								}
							}
						}
					}
				}
			}

			//@@TODO: make selection by pseudoclass

			if(sel->dom_id > 0)
			{
				for(UINT k = 0; k < current.size(); k++)
				{
					if(!current[k] || ((CDOMnode*)current[k])->m_iDOMid != sel->dom_id)
					{
						current.erase(k);
						k--;
					}
				}
			}

			if(sel->dom_class.size())
			{
				for(UINT k = 0; k < current.size(); k++)
				{
					UINT matches = 0;
					for(UINT l = 0; l < sel->dom_class.size(); l++)
					{
						UINT cid = sel->dom_class[l];
						for(UINT m = 0; m < ((CDOMnode*)current[k])->m_vDOMcls.size(); m++)
						{
							if(((CDOMnode*)current[k])->m_vDOMcls[m] == cid)
							{
								matches++;
								break;
							}
						}
					}
					if(matches != sel->dom_class.size())
					{
						current.erase(k);
						k--;
					}
				}
			}

			if(sel->pseudoclass != 0)
			{
				for(UINT k = 0; k < current.size(); k++)
				{
					if((((CDOMnode*)current[k])->m_pseudoclasses & sel->pseudoclass) != sel->pseudoclass)
					{
						current.erase(k);
						k--;
					}
				}
			}


			if(sel->dom_tag >= 0)
			{
				for(UINT k = 0; k < current.size(); k++)
				{
					if(((CDOMnode*)current[k])->m_iNodeId != sel->dom_tag)
					{
						current.erase(k);
						k--;
					}
				}
			}
			return(current);
		}

		IDOMnodeCollection CDOMdocument::querySelectorAll(const css::ICSSRuleSet * rules)
		{
			IDOMnodeCollection result;

			for(UINT i = 0; i < rules->size(); i++)
			{
				const css::ICSSrule * rule = &(*rules)[i];

				IDOMnodeCollection current, prev;
				css::ICSSrule::CONNECTOR connector = css::ICSSrule::CONNECTOR_NONE;
				for(UINT j = 0; j < rule->m_selectors.size(); j++)
				{
					const css::ICSSrule::ICSSselectorItem * sel = &rule->m_selectors[j];
					if(connector == css::ICSSrule::CONNECTOR_NONE)
					{
						current = querySelectorSimple(sel);
					}
					else
					{
						if(!prev.size())
						{
							return(prev);
						}
						current = querySelectorSimple(sel);
						for(UINT k = 0; k < current.size(); k++)
						{
							bool bFound = false;
							switch(connector)
							{
							case css::ICSSrule::CONNECTOR_SIMPLE:
								for(UINT l = 0; l < prev.size(); l++)
								{
									if(current[k]->isChildOf(prev[l]))
									{
										bFound = true;
										break;
									}
								}
								if(!bFound)
								{
									current.erase(k);
									k--;
								}
								break;
							case css::ICSSrule::CONNECTOR_AFTER:
								for(UINT l = 0; l < prev.size(); l++)
								{
									if(current[k]->isAfter(prev[l]))
									{
										bFound = true;
										break;
									}
								}
								if(!bFound)
								{
									current.erase(k);
									k--;
								}
								break;
								break;
							case css::ICSSrule::CONNECTOR_CHILD:
								for(UINT l = 0; l < prev.size(); l++)
								{
									if(current[k]->parentNode() == prev[l])
									{
										bFound = true;
										break;
									}
								}
								if(!bFound)
								{
									current.erase(k);
									k--;
								}
								break;
							case css::ICSSrule::CONNECTOR_NEXT:
								for(UINT l = 0; l < prev.size(); l++)
								{
									if(current[k]->prevSibling() == prev[l])
									{
										bFound = true;
										break;
									}
								}
								if(!bFound)
								{
									current.erase(k);
									k--;
								}
								break;
							}
						}
					}
					connector = sel->connector;
					prev = current;
				}
				for(UINT k = 0; k < current.size(); k++)
				{
					result.push_back(current[k]);
				}
			}

			return(result);
		}

		void CDOMdocument::updateStyleSubtree(IDOMnode * pStartNode)
		{
			bool f = false;
			for(int i = 0, l = m_UpdateStyleQueue.size(); i < l; ++i)
			{
				if(m_UpdateStyleQueue[i] == pStartNode || pStartNode->isChildOf(m_UpdateStyleQueue[i]))
				{
					return;
				}
				if(m_UpdateStyleQueue[i]->isChildOf(pStartNode))
				{
					if(!f)
					{
						f = true;
						m_UpdateStyleQueue[i] = pStartNode;
					}
					else
					{
						m_UpdateStyleQueue.erase(i);
						--l;
						--i;
					}
				}
			}
			if(!f)
			{
				m_UpdateStyleQueue.push_back(pStartNode);
			}
			return;
		}

		void CDOMdocument::updateStyles(float fTimeDelta)
		{
			if(!m_pRootNode || m_UpdateStyleQueue.size() == 0)
			{
				// if has transitions then update transitions and update render tree
				if(m_pRTroot && updateTransitions(fTimeDelta))
				{
					m_pRTroot->updateStyles();
				}
				return;
			}
			bool IsIncremental = m_UpdateStyleQueue[0] != m_pRootNode; // incremental update
			for(int i = 0, l = m_UpdateStyleQueue.size(); i < l; ++i)
			{
				if(IsIncremental)
				{
					((CDOMnode*)m_UpdateStyleQueue[i])->storeStyles();
				}
				((CDOMnode*)m_UpdateStyleQueue[i])->resetStyles();
			}

			UINT icount = m_pCSS->m_styleOrder.size();
			css::ICSSstyleSet * css;
			for(UINT i = 0; i < icount; i++)
			{
				css = m_pCSS->m_styleOrder[i];
				if(css->isEnabledForWidth(m_pDesktopStack->getScreenWidth()))
				{
					UINT iRuleCount = css->m_pRules.size();
					css::ICSSstyle * pStyle;
					for(UINT j = 0; j < iRuleCount; j++)
					{
						pStyle = &css->m_pRules[j];
						IDOMnodeCollection els = querySelectorAll(&((css::CCSSstyle*)pStyle)->m_pRules);
						UINT iNodeCount = els.size();
						for(UINT k = 0; k < iNodeCount; k++)
						{
							bool cf = false;
							for(int ii = 0, l = m_UpdateStyleQueue.size(); ii < l && !cf; ++ii)
							{
								if(!IsIncremental || els[k] == m_UpdateStyleQueue[ii] || els[k]->isChildOf(m_UpdateStyleQueue[ii]))
								{
									CDOMnode::applyCSSrules(pStyle, (CDOMnode*)els[k]);
									cf = true;
								}
							}
						}
					}
				}
			}
			for(int i = 0, l = m_UpdateStyleQueue.size(); i < l; ++i)
			{
				if(m_UpdateStyleQueue[i]->parentNode())
				{
					((CDOMnode*)m_UpdateStyleQueue[i]->parentNode())->applyChildStyle(true);
				}
				else
				{
					((CDOMnode*)m_UpdateStyleQueue[i])->applyChildStyle();
				}
			}


			if(IsIncremental) // incremental update
			{
				if(m_pRTroot)
				{
					for(int i = 0, l = m_UpdateStyleQueue.size(); i < l; ++i)
					{
						((CDOMnode*)m_UpdateStyleQueue[i])->captureStyleChanges(this);
					}
					// update transitions
					updateTransitions(fTimeDelta);
					m_pRTroot->updateStyles();

					//					m_pRTroot->DebugPrint();
				}
			}
			((CDOMnode*)m_pRootNode)->resetStyleChanges();
			m_UpdateStyleQueue.clearFast();
		}

		void CDOMdocument::buildRenderThree()
		{
			mem_delete(m_pRTroot);
			m_pRTroot = render::IRenderFrame::createNode((CDOMnode*)m_pRootNode, NULL);
			m_pRTroot->onCreated();
			m_pRTroot->setWidth(m_pDesktopStack->getScreenWidth());
			m_pRTroot->setHeight(m_pDesktopStack->getScreenHeight());
			m_pRTroot->layout();
			//m_pRTroot->DebugPrint();
		}

		void CDOMdocument::update(float fTimeDelta)
		{
			updateStyles(fTimeDelta);
			reflow();
		}

		void CDOMdocument::render()
		{
			m_isDirty = false;
			m_pDesktopStack->getTextureManager()->bindTexture(NULL);
			m_pRTroot->render(0);
		}

		IDOMnode * CDOMdocument::getElementByXY(int x, int y, bool sendEnterLeave)
		{
			if(!m_pRTroot)
			{
				return(NULL);
			}
			IDOMnode * node = NULL;
			m_pRTroot->findElementByXY(x, y, &node, sendEnterLeave);
			return(node);
		}

		void CDOMdocument::addReflowItem(render::IRenderFrame *rf, bool forceParent)
		{
#if 1
			while(rf->getParent() && !(rf->getNode() && (rf->getNode()->getStyle()->position->getInt() == css::ICSSproperty::POSITION_ABSOLUTE || rf->getNode()->getStyle()->position->getInt() == css::ICSSproperty::POSITION_FIXED)))
			{
				rf = rf->getParent();
			}
			for(UINT i = 0, l = m_ReflowQueue.size(); i < l; ++i)
			{
				if(m_ReflowQueue[i] == rf)
				{
					return;
				}
			}
			m_ReflowQueue.push_back(rf);
			return;
#endif
			//Render::IRenderFrame * pLayoutBlock = rf->GetParent();
			render::IRenderFrame * pLayoutBlock = forceParent ? rf->getParent() : rf;
			while(pLayoutBlock && pLayoutBlock->getParent() && !pLayoutBlock->hasFixedSize() && !pLayoutBlock->isOutOfFlow() && !(pLayoutBlock->getParent()->hasFixedSize() && pLayoutBlock->isLastChild() && !pLayoutBlock->getNode()->isTextNode()))
			{
				pLayoutBlock = pLayoutBlock->getParent();
			}
			if(!pLayoutBlock)
			{
				pLayoutBlock = ((CDOMnode*)m_pRootNode)->getRenderFrame();
			}

			for(UINT i = 0; i < m_ReflowQueue.size(); i++)
			{
				if(pLayoutBlock == m_ReflowQueue[i])
				{
					return;
				}
				if(pLayoutBlock->isChildOf(m_ReflowQueue[i]))
				{
					return;
				}
				if(m_ReflowQueue[i]->isChildOf(pLayoutBlock))
				{
					m_ReflowQueue[i] = pLayoutBlock;
					return;
				}
			}

			m_ReflowQueue.push_back(pLayoutBlock);
		}

		void CDOMdocument::reflow()
		{
			if(!m_ReflowQueue.size())
			{
				return;
			}
			//	wprintf(L"I: Layout changed\n");
			for(UINT i = 0; i < m_ReflowQueue.size(); i++)
			{
				render::IRenderFrame * pLayoutBlock = m_ReflowQueue[i];
				bool bHardReset = false;
				if(!pLayoutBlock->getParent())
				{
					pLayoutBlock->setWidth(m_pDesktopStack->getScreenWidth());
					pLayoutBlock->setHeight(m_pDesktopStack->getScreenHeight());
					bHardReset = true;
				}
				pLayoutBlock->resetLayout(true, bHardReset);
				pLayoutBlock->layout();
			}
			m_ReflowQueue.clear();
			markDirty();
		}

		void CDOMdocument::addTransitionProperty(css::CCSSproperty *pProp)
		{
			removeTransitionProperty(pProp);
			for(int i = 0, l = m_aTransitionUpdateList.size(); i < l; ++i)
			{
				if(!m_aTransitionUpdateList[i])
				{
					m_aTransitionUpdateList[i] = pProp;
					return;
				}
			}
			m_aTransitionUpdateList.push_back(pProp);
		}

		void CDOMdocument::removeTransitionProperty(css::CCSSproperty *pProp)
		{
			for(int i = 0, l = m_aTransitionUpdateList.size(); i < l; ++i)
			{
				if(m_aTransitionUpdateList[i] == pProp)
				{
					m_aTransitionUpdateList[i] = NULL;
					return;
				}
			}
		}

		bool CDOMdocument::updateTransitions(float fTimeDelta)
		{
			bool ret = false;
			for(int i = 0, l = m_aTransitionUpdateList.size(); i < l; ++i)
			{
				if(m_aTransitionUpdateList[i])
				{
					if(m_aTransitionUpdateList[i]->updateTransition(fTimeDelta))
					{
						m_aTransitionUpdateList[i] = NULL;
					}
					ret = true;
				}
			}
			return(ret);
		}

		IDOMnode* CDOMdocument::getRootNode()
		{
			return(m_pRootNode);
		}

		void CDOMdocument::forgotNode(IDOMnode * pNode)
		{
			for(int i = 0, l = m_UpdateStyleQueue.size(); i < l; ++i)
			{
				if(m_UpdateStyleQueue[i] == pNode)
				{
					m_UpdateStyleQueue.erase(i);
					--i; --l;
				}
			}
		}

		void CDOMdocument::forgotRenderFrame(render::IRenderFrame *pRF)
		{
			int idx = m_ReflowQueue.indexOf(pRF);
			if(idx >= 0)
			{
				m_ReflowQueue.erase(idx);
			}
		}

		bool CDOMdocument::isDirty()
		{
			return(m_isDirty);
		}

		void CDOMdocument::markDirty()
		{
			m_isDirty = true;
		}

		const IDOMnodeCollection& CDOMdocument::createFromText(const StringW & html)
		{
			IHTMLparser p;
			p.setDocument(this);
			UINT next = 0;
			int cur;
			dom::IDOMnode * root;
			//dom::IDOMnodeCollection c;
			m_cTmpNodes.clearFast();
			do
			{
				root = p.createFromString(html.c_str() + next, &cur);
				next += cur;
				m_cTmpNodes.push_back(root);
			}
			while(next < html.length());
			return(m_cTmpNodes);
		}

//##########################################################################

		void CDOMnode::setText(const StringW &text, BOOL build)
		{
			if(m_vChilds.size())
			{
				if(m_vChilds[0]->isTextNode())
				{
					((IDOMnodeText*)(m_vChilds[0]))->setText(text);
					if(build)
					{
						updateLayout();
					}
				}
			}
			else
			{
				appendHTML(text, !!build);
			}
		}

		const StringW& CDOMnode::getText()
		{
			if(m_vChilds.size() && m_vChilds[0]->isTextNode())
			{
				return(((IDOMnodeText*)(m_vChilds[0]))->getText());
			}
			static StringW str = L"";
			return(str);
		}

//#define GEVT_DPTC(TYPE, name) case TYPE: if(name){(name->isUnicode ? name->fnptrW(&ev, 0, NULL) : name->fnptr(&ev, 0, NULL));} break;

		void CDOMnode::dispatchClientEvent(IEvent ev, bool * preventDefault)
		{
			//ev.propagate = true;
			StringW cmd;
			switch(ev.type)
			{
#define GEVT_DPTC(TYPE, name) case TYPE: cmd = getAttribute(L"on" name); break;
				GEVT_DPTC(GUI_EVENT_TYPE_CLICK, L"click");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEDOWN, L"mousedown");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEUP, L"mouseup");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEENTER, L"mouseover");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSELEAVE, L"mouseout");
				GEVT_DPTC(GUI_EVENT_TYPE_KEYDOWN, L"keydown");
				GEVT_DPTC(GUI_EVENT_TYPE_KEYUP, L"keyup");
				GEVT_DPTC(GUI_EVENT_TYPE_KEYPRESS, L"keypress");
				//GEVT_DPTC(GUI_EVENT_TYPE_SCROLL, onScroll);
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEMOVE, L"mousemove");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEWHEELUP, L"mousewheelup");
				GEVT_DPTC(GUI_EVENT_TYPE_MOUSEWHEELDOWN, L"mousewheeldown");
				GEVT_DPTC(GUI_EVENT_TYPE_FOCUS, L"focus");
				GEVT_DPTC(GUI_EVENT_TYPE_BLUR, L"blur");
				GEVT_DPTC(GUI_EVENT_TYPE_CHANGE, L"change");
			}
			if(cmd.length())
			{
				m_pDocument->getDesktopStack()->execCallback(cmd, &ev);
			}
#undef GEVT_DPTC

			if(preventDefault)
			{
				*preventDefault = ev.preventDefault;
			}

			if(ev.propagate && m_pParent)
			{
				ev.currentTarget = m_pParent;
				m_pParent->dispatchClientEvent(ev, NULL);
			}
		}

		//void CDOMdocument::ExecCallback(const StringW cmd, IEvent * ev)
		//{
			//GetGUI()->
		//}

		void CDOMnode::setRenderFrame(render::IRenderFrame *prf)
		{
			m_pRenderFrame = prf;
			if(prf == NULL && m_pDocument->getFocus() == this)
			{
				static UINT nBODY = CDOMnode::getNodeIdByName(L"body");

				m_pDocument->requestFocus(m_pDocument->getElementsByTag(nBODY)[0][0]);
			}
		}

		void CDOMnode::updateStyles()
		{
			m_pDocument->updateStyleSubtree(this);
		}

		void CDOMnode::dispatchEvent(IEvent & ev)
		{
			if(pseudoclassExists(css::ICSSrule::PSEUDOCLASS_DISABLED))
			{
				return;
			}
			if(ev.target == this)
			{
				dispatchClientEvent(ev, &ev.preventDefault);
			}
			if(ev.preventDefault)
			{
				return;
			}
			if(m_pRenderFrame && m_pRenderFrame->m_pScrollBarVert)
			{
				m_pRenderFrame->m_pScrollBarVert->dispatchEvent(ev);
			}
			switch(ev.type)
			{
			case GUI_EVENT_TYPE_MOUSEDOWN:
				m_pDocument->requestFocus(this);
				if(ev.key == KEY_LBUTTON)
				{
					if(m_bToggleable)
					{
						if(m_bToggleState && !m_bToggleOnly)
						{
							removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
							m_bToggleState = false;
						}
						else
						{
							addPseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
							m_bToggleState = true;
							if(m_bToggleOnly && m_vDOMcls.size())
							{
								const IDOMnodeCollection * deactnode = getDocument()->getElementsByClass(m_vDOMcls[0]);
								for(UINT i = 0; i < deactnode->size(); i++)
								{
									IDOMnode * cn = (*deactnode)[i];
									if(cn != this && ((CDOMnode*)cn)->m_bToggleState)
									{
										cn->removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
										((CDOMnode*)cn)->m_bToggleState = false;
									}
								}
							}
						}

						IEvent ev2 = ev;
						ev2.type = GUI_EVENT_TYPE_CHANGE;
						ev2.target = this;
						ev2.relatedTarget = NULL;
						dispatchEvent(ev2);
					}
					else
					{
						addPseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
					}

					if(m_vChilds.size() && m_vChilds[0]->isTextNode())
					{
						((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->placeCaretByXY(ev.offsetX - getRenderFrame()->getInnerLeft() + getRenderFrame()->getScrollLeft(), ev.offsetY - getRenderFrame()->getInnerTop() + getRenderFrame()->getScrollTop());
						((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->selectionStart();
					}
				}
				ev.stopPropagation();
				break;
			case GUI_EVENT_TYPE_MOUSEUP:
				if(ev.key == KEY_LBUTTON)
				{
					
					if(!m_bToggleable)
					{
						removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
					}
					if(m_vChilds.size() && m_vChilds[0]->isTextNode())
					{
						((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->selectionEnd();
					}
				}
				break;
			case GUI_EVENT_TYPE_CLICK:
				//wprintf(L"Clicked\n");
				break;

			case GUI_EVENT_TYPE_FOCUS:
				addPseudoclass(css::ICSSrule::PSEUDOCLASS_FOCUS);
				ev.stopPropagation();
				break;

			case GUI_EVENT_TYPE_BLUR:
				removePseudoclass(css::ICSSrule::PSEUDOCLASS_FOCUS);
				ev.stopPropagation();
				break;

			case GUI_EVENT_TYPE_MOUSEWHEELUP:
				if(m_pRenderFrame->m_iScrollTopMax != 0)
				{
					if(m_pRenderFrame->m_iScrollTop != 0)
					{
						ev.stopPropagation();
						m_pDocument->markDirty();
					}
					//m_pRenderFrame->m_iScrollTop -= 32;
							
					if(m_pRenderFrame->m_iScrollSpeedY > 0)
					{
						m_pRenderFrame->m_iScrollSpeedY = -SCROLL_SPEED;
					}
					else
					{
						m_pRenderFrame->m_iScrollSpeedY -= SCROLL_SPEED;
						if(m_pRenderFrame->m_iScrollSpeedY < -SCROLL_SPEED_MAX)
						{
							m_pRenderFrame->m_iScrollSpeedY = -SCROLL_SPEED_MAX;
						}
					}
				}
				break;
			case GUI_EVENT_TYPE_MOUSEWHEELDOWN:
				if(m_pRenderFrame->m_iScrollTopMax != 0)
				{
					if(m_pRenderFrame->m_iScrollTopMax != m_pRenderFrame->m_iScrollTop)
					{
						ev.stopPropagation();
						m_pDocument->markDirty();
					}
					//m_pRenderFrame->m_iScrollTop += 32;
					if(m_pRenderFrame->m_iScrollSpeedY < 0)
					{
						m_pRenderFrame->m_iScrollSpeedY = SCROLL_SPEED;
					}
					else
					{
						m_pRenderFrame->m_iScrollSpeedY += SCROLL_SPEED;
						if(m_pRenderFrame->m_iScrollSpeedY > SCROLL_SPEED_MAX)
						{
							m_pRenderFrame->m_iScrollSpeedY = SCROLL_SPEED_MAX;
						}
					}
					
				}
				break;

			case GUI_EVENT_TYPE_KEYPRESS:
				//((Render::IRenderTextNew*)(m_pRenderFrame->GetChild(0)))->
				/*if(!m_vChilds.size())
				{
					IDOMnode * pTextNode = IDOMnode::createNode(L"text");
					pTextNode->SetDocument(m_pDocument);
					pTextNode->
				}*/
				if(ev.key == KEY_SPACE || ev.key == KEY_ENTER)
				{
					if(m_bToggleable)
					{
						if(m_bToggleState && !m_bToggleOnly)
						{
							removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
							m_bToggleState = false;
						}
						else
						{
							addPseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
							m_bToggleState = true;
							if(m_bToggleOnly && m_vDOMcls.size())
							{
								const IDOMnodeCollection * deactnode = getDocument()->getElementsByClass(m_vDOMcls[0]);
								for(UINT i = 0; i < deactnode->size(); i++)
								{
									CDOMnode * cn = (CDOMnode*)((*deactnode)[i]);
									if(cn != this && cn->m_bToggleState)
									{
										cn->removePseudoclass(css::ICSSrule::PSEUDOCLASS_ACTIVE);
										cn->m_bToggleState = false;
									}
								}
							}
						}
					}
					else
					{

					}
				}
				if(m_bEditable && m_vChilds.size() && m_vChilds[0]->isTextNode())
				{
					if(ev.syskey)
					{
						bool isShift = CKeyMap::keyState(KEY_SHIFT);
						switch(ev.key)
						{
						case KEY_LEFT:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->moveCaretPos(-1, isShift);
							break;
						case KEY_RIGHT:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->moveCaretPos(1, isShift);
							break;

						case KEY_UP:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->moveCaretLine(-1, isShift);
							break;
						case KEY_DOWN:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->moveCaretLine(1, isShift);
							break;

						case KEY_HOME:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->setCaretPos(0, false, isShift);
							break;

						case KEY_END:
							((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->setCaretPos(((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->getCaretMaxPos(), false, isShift);
							break;


						case KEY_DELETE:
							if(((IDOMnodeText*)(m_vChilds[0]))->hasSelection())
							{
								((IDOMnodeText*)(m_vChilds[0]))->deleteSelection();
							}
							else
							{
								((IDOMnodeText*)(m_vChilds[0]))->deleteChar(ev.key == KEY_BACKSPACE);
							}

							getDocument()->addReflowItem(((CDOMnode*)m_vChilds[0])->getRenderFrame());

							break;
						
						}
					}
					else
					{
						if(CKeyMap::keyState(KEY_CTRL))
						{
							switch(ev.key + 'A' - 1)
							{
							case 'V': //Ctrl + V
								if(CKeyMap::keyState(KEY_CTRL))
								{
									((IDOMnodeText*)(m_vChilds[0]))->deleteSelection();
									((IDOMnodeText*)(m_vChilds[0]))->fromClipboard();
								}
								break;

							case 'C': //Ctrl + C
								if(CKeyMap::keyState(KEY_CTRL))
								{
									((IDOMnodeText*)(m_vChilds[0]))->toClipboard(false);
								}
								break;

							case 'X': //Ctrl + X
								if(CKeyMap::keyState(KEY_CTRL))
								{
									((IDOMnodeText*)(m_vChilds[0]))->toClipboard(true);
								}
								break;

							case 'A': //Ctrl + A
								if(CKeyMap::keyState(KEY_CTRL))
								{
									((IDOMnodeText*)(m_vChilds[0]))->selectAll();
								}
								break;
							}
						}
						else
						{
							switch(ev.key)
							{
							case KEY_BACKSPACE:
								if(((IDOMnodeText*)(m_vChilds[0]))->hasSelection())
								{
									((IDOMnodeText*)(m_vChilds[0]))->deleteSelection();
								}
								else
								{
									((IDOMnodeText*)(m_vChilds[0]))->deleteChar(ev.key == KEY_BACKSPACE);
								}
								break;

							case KEY_TAB:
								if(m_bIgnHotkeys)
								{

									break;
								}

							case KEY_ENTER:
								if(m_bIgnHotkeys)
								{

									break;
								}

							default:
								((IDOMnodeText*)(m_vChilds[0]))->deleteSelection();
								((IDOMnodeText*)(m_vChilds[0]))->insertChar(ev.key);
							}
						}
						
					//	((IDOMnodeText*)(m_vChilds[0]))->SetText(((IDOMnodeText*)(m_vChilds[0]))->GetText() + (WCHAR)ev.key);
						((IDOMnodeText*)(m_vChilds[0]))->updateLayout();
					}
					//m_pRenderFrame->Layout();
				}
				{
					static UINT tagButton = getNodeIdByName(L"button");
					if(getNodeId() == tagButton)
					{
						if(ev.syskey)
						{
							CDOMnode * pEl = NULL;
							switch(ev.key)
							{
							case KEY_LEFT:
								pEl = (CDOMnode*)prevSibling();
								while(pEl)
								{
									if(pEl->getNodeId() == tagButton)
									{
										getDocument()->requestFocus(pEl);
										break;
									}
									pEl = (CDOMnode*)pEl->prevSibling();
								}
								break;
							case KEY_RIGHT:
								pEl = (CDOMnode*)nextSibling();
								while(pEl)
								{
									if(pEl->getNodeId() == tagButton)
									{
										getDocument()->requestFocus(pEl);
										break;
									}
									pEl = (CDOMnode*)pEl->nextSibling();
								}
								break;
							}
						}
						if(ev.key == KEY_ENTER)
						{
							IEvent ev2;
							ev2.clientX = ev2.clientY = 0;
							ev2.type = GUI_EVENT_TYPE_CLICK;
							ev2.key = KEY_ENTER;
							ev2.target = this;
							dispatchEvent(ev2);
						}
					}
				}
				break;

			case GUI_EVENT_TYPE_MOUSEENTER:
				addPseudoclass(css::ICSSrule::PSEUDOCLASS_HOVER);

				//if(m_css.p_cursor.IsSet())
				{
					/*switch(m_css.p_cursor.GetInt())
					{
					case css::ICSSproperty::CURSOR_AUTO:
					case css::ICSSproperty::CURSOR_DEFAULT:
						SetCursor(LoadCursor(NULL, IDC_ARROW));
						break;
					case css::ICSSproperty::CURSOR_POINTER:
						SetCursor(LoadCursor(NULL, IDC_HAND));
						break;

					case css::ICSSproperty::CURSOR_TEXT:
						SetCursor(LoadCursor(NULL, IDC_WAIT));
						break;
					}*/
				}
				
				ev.stopPropagation();
				break;

			case GUI_EVENT_TYPE_MOUSELEAVE:
				removePseudoclass(css::ICSSrule::PSEUDOCLASS_HOVER);
				ev.stopPropagation();
				break;

			case GUI_EVENT_TYPE_MOUSEMOVE:
				if(m_vChilds.size() && m_vChilds[0]->isTextNode())
				{
					if(m_pDocument->getFocus() == this && CKeyMap::keyState(KEY_LBUTTON))
					{
						((render::IRenderTextNew*)(((CDOMnode*)m_vChilds[0])->getRenderFrame()))->placeCaretByXY(ev.offsetX - getRenderFrame()->getInnerLeft() + getRenderFrame()->getScrollLeft(), ev.offsetY - getRenderFrame()->getInnerTop() + getRenderFrame()->getScrollTop());
					}
				}
				break;
			}
					
					
			if(m_bDelegateEvents)
			{
				StringW to = getAttribute(L"for");
				if(to.length())
				{
					IDOMnode * pTO = m_pDocument->getElementById(to);
					if(pTO)
					{
						pTO->dispatchEvent(ev);
					}
				}
			}
			if(ev.propagate && m_pParent)
			{
				m_pParent->dispatchEvent(ev);
			}
		}

		void CDOMnode::updateLayout(bool bForce)
		{
			getDocument()->addReflowItem(getRenderFrame());
			if(bForce)
			{
				getRenderFrame()->m_bHasFixedSize = false;
			}
		}
		
#define APPLY_RULE(rule) if(style->rule.isSet()){pNode->m_css.rule = style->rule;}
		void CDOMnode::applyCSSrules(const css::ICSSstyle * _style, CDOMnode * pNode)
		{
			const css::CCSSstyle * style = (const css::CCSSstyle *)_style;
			APPLY_RULE(p_display);
			APPLY_RULE(p_color);
			APPLY_RULE(p_cursor);

			APPLY_RULE(p_direction);

			APPLY_RULE(p_opacity);

			APPLY_RULE(p_z_index);

			APPLY_RULE(p_overflow_x);
			APPLY_RULE(p_overflow_y);

			APPLY_RULE(p_margin_left);
			APPLY_RULE(p_margin_right);
			APPLY_RULE(p_margin_top);
			APPLY_RULE(p_margin_bottom);

			APPLY_RULE(p_padding_left);
			APPLY_RULE(p_padding_right);
			APPLY_RULE(p_padding_top);
			APPLY_RULE(p_padding_bottom);

			APPLY_RULE(p_top);
			APPLY_RULE(p_left);
			APPLY_RULE(p_right);
			APPLY_RULE(p_bottom);

			APPLY_RULE(p_width);
			APPLY_RULE(p_height);

			APPLY_RULE(p_max_width);
			APPLY_RULE(p_max_height);

			APPLY_RULE(p_min_width);
			APPLY_RULE(p_min_height);

			APPLY_RULE(p_position);

			APPLY_RULE(p_visibility);

			APPLY_RULE(p_float);

			APPLY_RULE(p_background_image);
			APPLY_RULE(p_background_color);
			APPLY_RULE(p_background_position_x);
			APPLY_RULE(p_background_position_y);
			APPLY_RULE(p_background_attachment);
			APPLY_RULE(p_background_repeat);
			APPLY_RULE(p_background_size_x);
			APPLY_RULE(p_background_size_y);


			APPLY_RULE(p_border_top_style); //
			APPLY_RULE(p_border_top_width);
			APPLY_RULE(p_border_top_color);
			APPLY_RULE(p_border_top_left_radius);
			APPLY_RULE(p_border_top_right_radius);

			APPLY_RULE(p_border_right_style); //
			APPLY_RULE(p_border_right_width);
			APPLY_RULE(p_border_right_color);

			APPLY_RULE(p_border_bottom_style); //
			APPLY_RULE(p_border_bottom_width);
			APPLY_RULE(p_border_bottom_color);
			APPLY_RULE(p_border_bottom_left_radius);
			APPLY_RULE(p_border_bottom_right_radius);

			APPLY_RULE(p_border_top_left_method);
			APPLY_RULE(p_border_top_right_method);
			APPLY_RULE(p_border_bottom_left_method);
			APPLY_RULE(p_border_bottom_right_method);

			APPLY_RULE(p_border_left_style); //
			APPLY_RULE(p_border_left_width);
			APPLY_RULE(p_border_left_color);


			APPLY_RULE(p_font_name);
			APPLY_RULE(p_font_size);
			APPLY_RULE(p_font_style);
			APPLY_RULE(p_font_weight);

			APPLY_RULE(p_text_align);
			APPLY_RULE(p_text_decoration_line);

			APPLY_RULE(p_text_shadow_offset_x);
			APPLY_RULE(p_text_shadow_offset_y);
			APPLY_RULE(p_text_shadow_blur_radius);
			APPLY_RULE(p_text_shadow_color);

			APPLY_RULE(p__gui_text_appearance);
			APPLY_RULE(p__gui_text_cursor);
			APPLY_RULE(p__gui_text_format);
			APPLY_RULE(p__gui_z);
			APPLY_RULE(p__gui_spin_percent);
			APPLY_RULE(p_transform);


			pNode->m_css.setupTransitions(style);
		}
#undef APPLY_RULE

		void CDOMnode::setAttribute(const StringW & name, const StringW & value)
		{
			if(name == L"id")
			{
				m_pDocument->indexUnsetId(m_iDOMid);
				m_iDOMid = m_pDocument->getIndexForIdString(value);
				m_pDocument->indexSetId(m_iDOMid, this);
			}

			else if(name == L"style")
			{
				UINT n;
				m_css_self.loadStyleFromString(value.c_str(), &n);
			}


			else if(name == L"class")
			{
				StringW cls = value+L" ";
				UINT pos = 0;
				Array<UINT> vNewCls;
				Array<UINT> vAddCls;
				Array<UINT> vRemoveCls;
				while(true)
				{
					pos = cls.find(L" ");
					if(pos != ~0)
					{
						if(pos != 0)
						{
							StringW c = cls.substr(0, pos);
							UINT icls = m_pDocument->getIndexForClassString(c);
							vNewCls.push_back(icls);
							bool bFound = false;
							for(UINT i = 0; i < m_vDOMcls.size(); i++)
							{
								if(m_vDOMcls[i] == icls)
								{
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								vAddCls.push_back(icls);
							}
						}
						//doc->m_IndexByClass[icls].push_back(_node);
						cls = cls.substr(pos + 1);
					}
					else
					{
						break;
					}
				}
				for(UINT j = 0; j < m_vDOMcls.size(); j++)
				{
					UINT icls = m_vDOMcls[j];
					bool bFound = false;
					for(UINT i = 0; i < vNewCls.size(); i++)
					{
						if(vNewCls[i] == icls)
						{
							bFound = true;
							break;
						}
					}
					if(!bFound)
					{
						vRemoveCls.push_back(icls);
					}
				}
				m_vDOMcls = vNewCls;
				for(UINT i = 0; i < vRemoveCls.size(); i++)
				{
					m_pDocument->indexUnsetClass(vRemoveCls[i], this);
				}
				for(UINT i = 0; i < vAddCls.size(); i++)
				{
					m_pDocument->indexSetClass(vAddCls[i], this);
				}
				//m_pDocument->IndexUnsetClass(m_iDOMid);
				//m_iDOMid = m_pDocument->GetIndexForIdString(value);
				//m_pDocument->IndexSetId(m_iDOMid, this);
			}

			m_mAttributes[name] = value;
		}

		void CDOMnode::appendHTML(const StringW &wsHTML, bool regen, IDOMnode *pInsertBefore)
		{
			gui::dom::IDOMnodeCollection newItems = m_pDocument->createFromText(wsHTML);
			for(UINT i = 0, l = newItems.size(); i < l; i++)
			{
				appendChild(newItems[i], regen, pInsertBefore);
			}
		}

		void CDOMnode::setHTML(const StringW &wsHTML, bool regen)
		{
			for(int i = (int)m_vChilds.size() - 1; i >= 0; --i)
			{
				removeChild(m_vChilds[i], regen);
			}
			appendHTML(wsHTML, regen);
		}

		void CDOMnode::appendChild(IDOMnode * _pEl, bool regen, IDOMnode *pInsertBefore)
		{
			CDOMnode * pEl = (CDOMnode*)_pEl;
			if(!pEl)
			{
				return;
			}
			if(pInsertBefore)
			{
				for(int i = 0, l = m_vChilds.size(); i < l; ++i)
				{
					if(m_vChilds[i] == pInsertBefore)
					{
						((CDOMnode*)m_vChilds[i])->m_pPrevSibling = pEl;
						pEl->m_pNextSibling = m_vChilds[i];
						if(i > 0)
						{
							((CDOMnode*)m_vChilds[i - 1])->m_pNextSibling = pEl;
							pEl->m_pPrevSibling = m_vChilds[i - 1];
						}
						else
						{
							pEl->m_pPrevSibling = NULL;
						}

						for(int j = l - 1; j >= i; --j)
						{
							m_vChilds[j + 1] = m_vChilds[j];
						}

						m_vChilds[i] = pEl;

						break;
					}
				}
			}
			else
			{
				if(m_vChilds.size() > 0)
				{
					CDOMnode * last = (CDOMnode*)m_vChilds[m_vChilds.size() - 1];
					last->m_pNextSibling = pEl;
					pEl->m_pPrevSibling = last;
				}
				m_vChilds.push_back(pEl);
			}
			pEl->m_pParent = this;
			if(m_pDocument)
			{
				//m_pDocument->IndexSetNode(pEl);
			}
			if(regen && getRenderFrame())
			{
				assert(!pInsertBefore && "Implement me!");
				getDocument()->updateStyleSubtree(pEl);
				getDocument()->updateStyles(0);

				if(!pEl->getRenderFrame())
				{
					render::IRenderFrame * pNewRF;
					if(pEl->isTextNode())
					{
						pNewRF = render::IRenderFrame::createNode(NULL, getRenderFrame()->m_pRootNode);
						pNewRF->addChild(render::IRenderFrame::createNode(pEl, getRenderFrame()->m_pRootNode));
					}
					else
					{
						pNewRF = render::IRenderFrame::createNode(pEl, getRenderFrame()->m_pRootNode);
					}
					if(pNewRF)
					{
						getRenderFrame()->addChild(pNewRF);
						pNewRF->onCreated();
						getDocument()->addReflowItem(pNewRF, true);
					}
				}
			}
		}

		void CDOMnode::classAdd(const StringW & cls)
		{
			setAttribute(L"class", getAttribute(L"class")+L" "+cls);

		}
		/*void CDOMnode::ClassAdd(UINT cls)
		{

		}*/

		void CDOMnode::classRemove(const StringW & cls)
		{
			//@TODO: Implement me
		}
		/*void CDOMnode::ClassRemove(UINT cls)
		{

		}*/

		void CDOMnode::classToggle(const StringW & cls, int set)
		{
			//@TODO: Implement me
		}
		/*void CDOMnode::ClassToggle(UINT cls, int set = -1)
		{

		}*/

		BOOL CDOMnode::classExists(const StringW & cls)
		{
			//@TODO: Implement me
			return(FALSE);
		}
		void CDOMnode::setUserData(void *pData) 
		{
			m_pUserData = pData;
		}
		void* CDOMnode::getUserData() 
		{
			return(m_pUserData);
		}
		/*BOOL CDOMnode::ClassExists(UINT cls)
		{

		}*/

		void CDOMnode::removeChild(IDOMnode * _pEl, bool regen)
		{
			CDOMnode * pEl = (CDOMnode*)_pEl;
			for(UINT i = 0, l = m_vChilds.size(); i < l; ++i)
			{
				if(m_vChilds[i] == pEl)
				{
					if(getDocument()->getFocus() == pEl)
					{
						getDocument()->requestFocus(getDocument()->getElementsByTag(L"body")[0][0]);
					}
					m_pDocument->indexUnsetNode(pEl);
					m_pDocument->indexUnsetId(pEl->m_iDOMid);
					for(UINT j = 0; j < pEl->m_vDOMcls.size(); j++)
					{
						m_pDocument->indexUnsetClass(pEl->m_vDOMcls[j], pEl);
					}
					for(UINT k = 0; k < css::ICSSrule::PSEUDOCLASS_COUNT; k++)
					{
						pEl->removePseudoclass(1 << k);
					}
					while(pEl->m_vChilds.size() > 0)
					{
						pEl->removeChild(pEl->m_vChilds[0], false);
					}
					if(i > 0)
					{
						((CDOMnode*)m_vChilds[i - 1])->m_pNextSibling = ((CDOMnode*)m_vChilds[i])->m_pNextSibling;
					}
					if(i + 1 < m_vChilds.size())
					{
						((CDOMnode*)m_vChilds[i + 1])->m_pPrevSibling = ((CDOMnode*)m_vChilds[i])->m_pPrevSibling;
					}
					if(pEl->getRenderFrame() && regen)
					{
						pEl->getRenderFrame()->getParent()->removeChild(pEl->getRenderFrame());
					}
					m_pDocument->forgotNode(m_vChilds[i]);
					mem_delete(m_vChilds[i]);
					m_vChilds.erase(i);
					break;
				}
			}
		}
		
		void CDOMnode::addPseudoclass(UINT id)
		{
			if(!(m_pseudoclasses & id))
			{
				m_pseudoclasses |= id;
				m_pDocument->indexSetPseudoClass(id, this);
				m_pDocument->updateStyleSubtree(this);
			}
		}

		void CDOMnode::removePseudoclass(UINT id)
		{
			if(m_pseudoclasses & id)
			{
				m_pseudoclasses &= ~id;
				m_pDocument->indexUnsetPseudoClass(id, this);
				m_pDocument->updateStyleSubtree(this);
			}
		}

		bool CDOMnode::pseudoclassExists(UINT id)
		{
			return((m_pseudoclasses & id) != 0);
		}

	};
};