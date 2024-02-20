#ifndef _CDOMdocument_H_
#define _CDOMdocument_H_

#include "IDOM.h"
#include "IRenderFrame.h"
#include "IDOMdocument.h"

#define SCROLL_SPEED 32
#define SCROLL_SPEED_MAX 64

namespace gui
{
	class CDesktop;

	class CTranslationManager
	{
	public:
		CTranslationManager(CDesktopStack *pDesktopStack):
			m_pDesktopStack(pDesktopStack)
		{
		}
		void pushMatrix(const SMMATRIX & m);
		void popMatrix();
		SMMATRIX getCurrentMatrix();

		SX_ALIGNED_OP_MEM();

	protected:
		CDesktopStack *m_pDesktopStack;
		SMMATRIX m_result;
		Stack<SMMATRIX, 256, 16> m_stack;
	};


	namespace dom
	{
		class CDOMdocument: public IDOMdocument
		{
		public:
			CDOMdocument(CDesktopStack *pDesktopStack):
				m_pDesktopStack(pDesktopStack)
			{
				m_pCSS = new css::ICSS(this, pDesktopStack);
				m_pTranslationManager = new CTranslationManager(pDesktopStack);
			}

			~CDOMdocument()
			{
				mem_delete(m_pTranslationManager);
				mem_delete(m_pCSS);
			}

			IDOMnode* createNode(const wchar_t *tag);
			IDOMnode* createNode(UINT nid);

			void setRootNode(IDOMnode *pNode);

			IDOMnode* getElementById(const StringW &id);
			IDOMnode* getElementById(UINT iid);

			void indexBuild();
			void indexAdd(IDOMnode *pNode);
			void indexRemove(IDOMnode *pNode);

			UINT getIndexForIdString(const StringW &id);
			UINT getIndexForClassString(const StringW &id);
			int getIndexForTagString(const StringW &id);

			static void buildIndexFunc(IDOMdocument *doc, IDOMnode *node);

			const IDOMnodeCollection* getElementsByClass(const StringW &id);
			const IDOMnodeCollection* getElementsByClass(UINT cid);

			const IDOMnodeCollection* getElementsByPseudoClass(UINT cid);

			const IDOMnodeCollection* getElementsByTag(const StringW &id);
			const IDOMnodeCollection* getElementsByTag(UINT tid);

			IDOMnodeCollection querySelectorAll(const css::ICSSRuleSet * rules);

			CDesktopStack* getDesktopStack()
			{
				return(m_pDesktopStack);
			}

			css::ICSS* getCSS();

			CTranslationManager* getTranslationManager()
			{
				return(m_pTranslationManager);
			}

			void loadStyles();

			void calculateStyles();

			void indexUnsetId(UINT id);
			void indexSetId(UINT id, IDOMnode *pNode);

			void indexUnsetClass(UINT id, IDOMnode *pNode);
			void indexSetClass(UINT id, IDOMnode *pNode);

			void indexUnsetPseudoClass(UINT id, IDOMnode *pNode);
			void indexSetPseudoClass(UINT id, IDOMnode *pNode);

			void indexUnsetNode(CDOMnode *pNode);
			void IndexSetNode(CDOMnode *pNode);

			void updateStyleSubtree(IDOMnode *pStartNode);
			void updateStyles(float fTimeDelta);

			IDOMnodeCollection querySelectorSimple(const css::ICSSrule::ICSSselectorItem *sel);

			void buildRenderThree();

			void update(float fTimeDelta);
			void render();

			IDOMnode* getElementByXY(int x, int y, bool sendEnterLeave = false);

			//	IDesktop * GetDesktop();
			void setDesktop(IDesktop *pdp);

			void requestFocus(IDOMnode *pn);
			IDOMnode* getFocus();

			void addReflowItem(render::IRenderFrame *rf, bool forceParent=false);
			void reflow();

			void addTransitionProperty(css::CCSSproperty *pProp);
			void removeTransitionProperty(css::CCSSproperty *pProp);

			bool updateTransitions(float fTimeDelta);

			IDOMnode* getRootNode();

			void forgotNode(IDOMnode *pNode);
			void forgotRenderFrame(render::IRenderFrame *pRF);
			bool isDirty();
			void markDirty();

			const IDOMnodeCollection& createFromText(const StringW &html);

		protected:
			CDesktopStack *m_pDesktopStack;
			IDOMnode *m_pRootNode = NULL;
			CTranslationManager *m_pTranslationManager = NULL;

			css::ICSS *m_pCSS;

			CDesktop *m_pDesktop = NULL;

			//index
			AssotiativeArray<StringW, UINT> m_IndexStringById;
			AssotiativeArray<StringW, UINT> m_IndexStringByClass;
			IDOMnodeCollection m_IndexById;
			Array<IDOMnodeCollection> m_IndexByClass;
			Array<IDOMnodeCollection> m_IndexByPseudoClass;
			Array<IDOMnodeCollection> m_IndexByTagName;

			Array<render::IRenderFrame*> m_ReflowQueue;

			IDOMnodeCollection m_UpdateStyleQueue;

			//void ApplyCSSrules(const css::ICSSstyle * style, IDOMnode * pNode);

			render::IRenderFrame *m_pRTroot = NULL;

			Array<css::CCSSproperty*> m_aTransitionUpdateList;

			bool m_isDirty = true;

			IDOMnodeCollection m_cTmpNodes;
		};
	};
};

#endif
