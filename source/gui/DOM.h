#ifndef _CDOM_H_
#define _CDOM_H_

#include "IDOM.h"
#include "CSSstyle.h"

#define LINK_DOM_NODE_TO_TAG(cls, name) static IDOMnodeFactory<cls> C##cls(name)
#define DECLARE_DOM_NODE(cls) static CDOMnode * getInstance(UINT nid);
#define DEFINE_DOM_NODE(cls)							\
	CDOMnode * cls::getInstance(UINT nid)				\
	{													\
		CDOMnode * node = new cls();					\
		node->setNodeID(nid);							\
		return(node);									\
	}

namespace gui
{
	namespace dom
	{
		class CDOMdocument;
		class CDOMnode;
		namespace render
		{
			class IRenderFrame;
		};

		struct vTable
		{
			CDOMnode*(*getInstance)(UINT);
			StringW wsNodeName;
		};
		class CDOMnode: public IDOMnode
		{
		public:
			friend class CDOMdocument;

			CDOMnode():
				m_bDelegateEvents(false),
				m_bToggleOnly(false),
				m_bToggleable(false),
				m_bToggleState(false),
				m_pParent(NULL),
				m_iNodeId(0),
				m_pDocument(NULL),
				m_iDOMid(0),
				m_pPrevSibling(NULL),
				m_pNextSibling(NULL),
				m_pseudoclasses(0),
				m_pRenderFrame(NULL),
				m_bEditable(false),
				m_bFocusable(false),
				m_bIgnHotkeys(false)
			{};
			~CDOMnode();
			void appendChild(IDOMnode * pEl, bool regen = true, IDOMnode *pInsertBefore = NULL);
			void appendHTML(const StringW &wsHTML, bool regen = true, IDOMnode *pInsertBefore = NULL);
			void setHTML(const StringW &wsHTML, bool regen = true);
			void removeChild(IDOMnode * pEl, bool regen = true);
			IDOMnode * parentNode();

			void setDocument(CDOMdocument * doc);

			void setNodeID(UINT nid)
			{
				m_iNodeId = nid;
			}

#ifdef _DEBUG
			virtual void debugPrintMe(UINT lvl = 0);
#endif

			static const StringW& getNodeNameById(UINT nid);
			static int getNodeIdByName(const StringW &tag);

			virtual void setText(const StringW &text, BOOL build = FALSE);
			const StringW& getText();


			static IDOMnode* createNode(const wchar_t * tag)
			{
				WCHAR *t = new WCHAR[wcslen(tag) + 1];
				memcpy(t, tag, (wcslen(tag) + 1) * sizeof(wchar_t));
				_wcslwr_s(t, wcslen(tag) + 1);
				StringW s(t);
				mem_delete_a(t);
				if(GetNodeIDs().KeyExists(s))
				{
					int nid = GetNodeIDs()[s];
					vTable *vt = &GetNodeVTbls()[nid];
					return(vt->getInstance(nid));
				}
				return(NULL);
			}

			static IDOMnode* createNode(UINT nid)
			{
				if(GetNodeIDs().Size() > nid/* && nid > 0*/)
				{
					vTable * vt = &GetNodeVTbls()[nid];
					return(vt->getInstance(nid));
				}
				return(NULL);
			}

			virtual bool isTextNode()
			{
				return(false);
			}

			virtual void setAttribute(const StringW &name, const StringW &value);
			const StringW& getAttribute(const StringW &name);

			static AssotiativeArray<StringW, UINT>& GetNodeIDs()
			{
				static AssotiativeArray<StringW, UINT> nodeIDs;
				return(nodeIDs);
			}
			static Array<vTable>& GetNodeVTbls()
			{
				static Array<vTable> nodes;
				return(nodes);
			}

			void applyChildStyle(bool noSelf = false);
			void resetStyles();
			void resetStyleChanges();

			bool isChildOf(IDOMnode *pNode);
			bool isAfter(IDOMnode * pNode);

			IDOMnode* prevSibling()
			{
				return(m_pPrevSibling);
			}

			IDOMnode* nextSibling()
			{
				return(m_pNextSibling);
			}

			css::ICSSstyle* getStyle()
			{
				return(&m_css);
			}

			css::ICSSstyle* getStyleSelf()
			{
				return(&m_css_self);
			}

			const Array<IDOMnode*>* getChilds()
			{
				return(&m_vChilds);
			}

			UINT getNodeId()
			{
				return(m_iNodeId);
			}

			void addPseudoclass(UINT id);
			bool pseudoclassExists(UINT id);
			void removePseudoclass(UINT id);

			void storeStyles();
			void captureStyleChanges(CDOMdocument *pDoc);

			CDOMdocument* getDocument()
			{
				return(m_pDocument);
			}

			render::IRenderFrame* getRenderFrame()
			{
				return(m_pRenderFrame);
			}
			void setRenderFrame(render::IRenderFrame *prf);
			void updateStyles();
			void updateLayout(bool bForce=false);


			void dispatchEvent(IEvent &ev) override;
			void dispatchClientEvent(IEvent ev, bool *preventDefault) override;

			static void applyCSSrules(const css::ICSSstyle *style, CDOMnode *pNode);



			void classAdd(const StringW &cls);
			//void ClassAdd(UINT cls);

			void classRemove(const StringW &cls);
			//void ClassRemove(UINT cls);

			void classToggle(const StringW &cls, int set = -1);
			//void ClassToggle(UINT cls, int set = -1);

			BOOL classExists(const StringW &cls);
			//BOOL ClassExists(UINT cls);

			void setUserData(void *pData) override;
			void* getUserData() override;

			void skipStructureChanges()
			{
				m_bSkipStructureChanges = true;
			}

			bool isStructureChangesSkipped()
			{
				return(m_bSkipStructureChanges);
			}

		protected:
			IDOMnode *m_pParent;
			IDOMnode *m_pPrevSibling;
			IDOMnode *m_pNextSibling;
			Array<IDOMnode*> m_vChilds;
			CDOMdocument *m_pDocument;
			UINT m_iNodeId;

			UINT m_iDOMid;
			Array<UINT> m_vDOMcls;

			UINT m_pseudoclasses;

			AssotiativeArray<StringW, StringW> m_mAttributes;

			css::CCSSstyle m_css;
			css::CCSSstyle m_cssOld;
			css::CCSSstyle m_css_self;

			render::IRenderFrame *m_pRenderFrame;

			bool m_bToggleable;
			bool m_bToggleState;
			bool m_bToggleOnly;
			bool m_bEditable;
			bool m_bFocusable;

			bool m_bIgnHotkeys;

			bool m_bDelegateEvents;

		private:
			void *m_pUserData = NULL;

			bool m_bSkipStructureChanges = false;
		};


		typedef Array<IDOMnode*> IDOMnodeCollection;


		class IDOMnodeTag: public CDOMnode
		{
		public:
			DECLARE_CLASS(IDOMnodeTag, CDOMnode);

		//	css::ICSSstyle * GetStyle();

			/*const String & GetID();
			void SetID(const String & str);*/

		//protected:
		//	css::ICSSstyle * m_pStyle;

		};


		template<class T> class IDOMnodeFactory
		{
		public:

			IDOMnodeFactory(const wchar_t * name)
			{
				vTable vt;
				vt.getInstance = &(T::getInstance);
				vt.wsNodeName = name;
				UINT nid = CDOMnode::GetNodeVTbls().size();
				CDOMnode::GetNodeIDs()[vt.wsNodeName] = nid;
				CDOMnode::GetNodeVTbls()[nid] = vt;
			};
		};
	};
};

#endif
