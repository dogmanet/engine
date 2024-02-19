#ifndef __RENDERGRAPH_H
#define __RENDERGRAPH_H

#include "Render.h"

class CRenderGraphData final: public IXUnknownImplementation<IXUnknown>
{
public:
	~CRenderGraphData();

public:
	struct PassthroughInput
	{
		IGXBaseTexture *pTex;
		int iOutput;
	};
	struct PassthroughOutput
	{
		IGXBaseTexture *pTex;
		IGXSurface *pNextRT;
	};

	struct NodeData
	{
		Array<IGXBaseTexture*> aInputs;
		Array<IGXSurface*> aOutputs;

		Array<PassthroughInput> aPassthroughInputs;

		Array<PassthroughOutput> aPassthroughOutputs;

		IXRenderGraphNodeData *pNodeData = NULL;
	};

	Array<NodeData> m_aNodeData;
};

//##########################################################################

class CRenderGraph;
class CNodeInstance final: public IXUnknownImplementation<IXRenderGraphNodeInstance>
{
public:
	CNodeInstance(CRenderGraph *pGraph, UINT uNode, IXRenderGraphNode *pNode);
	~CNodeInstance();

	IXRenderGraphNode* XMETHODCALLTYPE getNode() override;

	bool XMETHODCALLTYPE getNodeData(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData **ppOut) override;

	void XMETHODCALLTYPE setTexture(IXRenderTarget *pFinalTarget, UINT uIdx, IXTexture *pTexture) override;
	void XMETHODCALLTYPE setProp(IXRenderTarget *pFinalTarget, UINT uIdx, const float4_t &vProp) override;
	void XMETHODCALLTYPE setFlag(IXRenderTarget *pFinalTarget, UINT uIdx, bool bVal) override;

private:
	CRenderGraph *m_pGraph;
	UINT m_uNodeId;
	IXRenderGraphNode *m_pNode;
};

//##########################################################################

class CRenderGraph final: public IXUnknownImplementation<IXRenderGraph>
{
public:
	CRenderGraph(CRender *pRender);
	~CRenderGraph();

	bool XMETHODCALLTYPE getNodeInstance(const char *szNodeInstance, IXRenderGraphNodeInstance **ppNodeInstance) override;
	
	void setName(const char *szName);
	const char* getName();

	void addNode(const char *szInstanceName, const char *szNodeName);
	void connect(const char *szOutputNode, const char *szOutputName, const char *szInputNode, const char *szInputName);

	void setOutput(const char *szOutputNode, const char *szOutputName);

	bool compile();

	bool isValid();

	void render(float fDeltaTime, CFinalTarget *pFinalTarget, CRenderGraphData *pGraphData);

	void updateVisibility(CFinalTarget *pFinalTarget, CRenderGraphData *pGraphData);

	void newGraphData(CFinalTarget *pFinalTarget, CRenderGraphData **ppOut);

private:
	struct Input
	{
		UINT uOutputNode;
		int iOutput;
		int iInput;

		int iPassthroughTo;
		bool isRequired;
	};

	struct InOut
	{
		Input inp;
		UINT uInputNode;
	};

	struct Output
	{
		Array<InOut> aConnectedInputs;
	};

	struct Node
	{
		String sName;
		IXRenderGraphNode *pNode;
		Array<Input> aInputs;
		Array<Output> aOutputs;
	};

	void buildExecutionOrder(UINT uStartFrom, Array<UINT> &aPath);

	IGXBaseTexture* findTextureInChain(UINT uNode, const Input &inp, CRenderGraphData *pGraphData);

private:
	CRender *m_pRender;
	String m_sName;
	Array<Node> m_aNodes;
	Array<UINT> m_aExecutionOrder;

	bool m_hasError = false;

	int m_iFinalNode = -1;
	int m_iFinalNodeOutput = -1;
};

#endif
