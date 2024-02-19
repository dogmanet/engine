#include "RenderGraph.h"


CRenderGraph::CRenderGraph(CRender *pRender):
	m_pRender(pRender)
{
}

CRenderGraph::~CRenderGraph()
{
	fora(i, m_aNodes)
	{
		mem_release(m_aNodes[i].pNode);
	}
}

void CRenderGraph::setName(const char *szName)
{
	m_sName = szName;
	//LogInfo("setName(%s)\n", szName);
}

const char* CRenderGraph::getName()
{
	return(m_sName.c_str());
}

void CRenderGraph::addNode(const char *szInstanceName, const char *szNodeName)
{
	//LogInfo("addNode(%s, %s)\n", szInstanceName, szNodeName);
	
	Node &node = m_aNodes[m_aNodes.size()];
	if(!m_pRender->getGraphNode(szNodeName, &node.pNode))
	{
		LogError("[%s] Couldn't find node '%s' for instance %s\n", m_sName.c_str(), szNodeName, szInstanceName);
		m_hasError = true;
		m_aNodes.erase(m_aNodes.size() - 1);
		return;
	}

	node.sName = szInstanceName;
}
void CRenderGraph::connect(const char *szOutputNode, const char *szOutputName, const char *szInputNode, const char *szInputName)
{
	//LogInfo("connect %s/%s -> %s/%s\n", szOutputNode, szOutputName, szInputNode, szInputName);

	int iInput = m_aNodes.indexOf(szInputNode, [](const Node &node, const char *szName){
		return(!strcmp(node.sName.c_str(), szName));
	});
	if(iInput < 0)
	{
		LogError("[%s] Cannot find node instance '%s'\n", m_sName.c_str(), szInputNode);
		m_hasError = true;
		return;
	}

	int iOutput = m_aNodes.indexOf(szOutputNode, [](const Node &node, const char *szName){
		return(!strcmp(node.sName.c_str(), szName));
	});
	if(iOutput < 0)
	{
		LogError("[%s] Cannot find node instance '%s'\n", m_sName.c_str(), szOutputNode);
		m_hasError = true;
		return;
	}

	Node &inputNode = m_aNodes[iInput];
	UINT uInputsDescCount = 0;
	const XRenderGraphNodeInputDesc *pInputsDesc = inputNode.pNode->getInputsDesc(&uInputsDescCount);
	int iInputIndex = -1;
	for(UINT i = 0; i < uInputsDescCount; ++i)
	{
		if(!strcmp(pInputsDesc[i].szName, szInputName))
		{
			iInputIndex = (int)i;
			break;
		}
	}
	if(iInputIndex < 0)
	{
		LogError("[%s] Cannot find input '%s' for node instance '%s'\n", m_sName.c_str(), szInputName, szInputNode);
		m_hasError = true;
		return;
	}

	Node &outputNode = m_aNodes[iOutput];
	UINT uOutputsDescCount = 0;
	const XRenderGraphNodeOutputDesc *pOutputsDesc = outputNode.pNode->getOutputsDesc(&uOutputsDescCount);
	int iOutputIndex = -1;
	for(UINT i = 0; i < uOutputsDescCount; ++i)
	{
		if(!strcmp(pOutputsDesc[i].szName, szOutputName))
		{
			iOutputIndex = (int)i;
			break;
		}
	}
	if(iOutputIndex < 0)
	{
		LogError("[%s] Cannot find output '%s' for node instance '%s'\n", m_sName.c_str(), szOutputName, szOutputNode);
		m_hasError = true;
		return;
	}
	
	int idx = inputNode.aInputs.indexOf(iInputIndex, [](const Input &inp, int iIndex){
		return(inp.iInput == iIndex);
	});
	if(idx >= 0)
	{
		inputNode.aInputs[idx].iOutput = iOutputIndex;
		inputNode.aInputs[idx].uOutputNode = (UINT)iOutput;
	}
	else
	{
		int iPassthroughTo = -1;
		if(pInputsDesc[iInputIndex].isPassthrough)
		{
			UINT uCount = 0;
			const XRenderGraphNodeOutputDesc *pOutsDesc = inputNode.pNode->getOutputsDesc(&uCount);
			for(UINT i = 0; i < uCount; ++i)
			{
				if(!fstrcmp(pOutsDesc[i].szName, pInputsDesc[iInputIndex].szName))
				{
					iPassthroughTo = (int)i;
					break;
				}
			}
		}
		inputNode.aInputs.push_back({(UINT)iOutput, iOutputIndex, iInputIndex, iPassthroughTo, pInputsDesc[iInputIndex].isRequired});
	}
}

void CRenderGraph::setOutput(const char *szOutputNode, const char *szOutputName)
{
	//LogInfo("setOutput(%s, %s)\n", szOutputNode, szOutputName);

	int iOutput = m_aNodes.indexOf(szOutputNode, [](const Node &node, const char *szName){
		return(!strcmp(node.sName.c_str(), szName));
	});
	if(iOutput < 0)
	{
		LogError("[%s] Cannot find node instance '%s'\n", m_sName.c_str(), szOutputNode);
		m_hasError = true;
		return;
	}

	Node &outputNode = m_aNodes[iOutput];
	UINT uOutputsDescCount = 0;
	const XRenderGraphNodeOutputDesc *pOutputsDesc = outputNode.pNode->getOutputsDesc(&uOutputsDescCount);
	int iOutputIndex = -1;
	for(UINT i = 0; i < uOutputsDescCount; ++i)
	{
		if(!strcmp(pOutputsDesc[i].szName, szOutputName))
		{
			iOutputIndex = (int)i;
			break;
		}
	}
	if(iOutputIndex < 0)
	{
		LogError("[%s] Cannot find output '%s' for node instance '%s'\n", m_sName.c_str(), szOutputName, szOutputNode);
		m_hasError = true;
		return;
	}

	m_iFinalNode = (int)iOutput;
	m_iFinalNodeOutput = iOutputIndex;
}

bool CRenderGraph::compile()
{
	if(m_hasError)
	{
		return(false);
	}

	if(m_iFinalNode < 0)
	{
		LogError("[%s] Graph output is not specified!\n", m_sName.c_str());
		m_hasError = true;
	}

	// test all non-optional inputs for each node is present
	fora(i, m_aNodes)
	{
		Node &node = m_aNodes[i];
		UINT uCount = 0;
		const XRenderGraphNodeInputDesc *pInputsDesc = node.pNode->getInputsDesc(&uCount);
		for(UINT j = 0; j < uCount; ++j)
		{
			if(!pInputsDesc[j].isOptional && node.aInputs.indexOf(j, [](const Input &inp, UINT uId){
				return(inp.iInput == uId);
			}) < 0)
			{
				LogError("[%s] Required input '%s' is not connected for node instance '%s'\n", m_sName.c_str(), pInputsDesc[j].szName, node.sName.c_str());
				m_hasError = true;
			}
		}
	}
	if(m_hasError)
	{
		return(false);
	}

	m_aExecutionOrder.clearFast();
	m_aExecutionOrder.reserve(m_aNodes.size());
	Array<UINT> aPath(m_aNodes.size());
	
	// test there are no cyclic references
	// determine execution order
	buildExecutionOrder(m_iFinalNode, aPath);

	// populate aOutputs
	fora(i, m_aNodes)
	{
		Node &node = m_aNodes[i];

		fora(j, node.aInputs)
		{
			Input &inp = node.aInputs[j];

			m_aNodes[inp.uOutputNode].aOutputs[inp.iOutput].aConnectedInputs.push_back({inp, i});
		}
	}

	return(!m_hasError);
}

bool CRenderGraph::isValid()
{
	return(compile());
}

void CRenderGraph::buildExecutionOrder(UINT uStartFrom, Array<UINT> &aPath)
{
	if(m_hasError)
	{
		return;
	}

	if(m_aExecutionOrder.indexOf(uStartFrom) >= 0)
	{
		return;
	}
	
	aPath.push_back(uStartFrom);


	Node &node = m_aNodes[uStartFrom];
	fora(i, node.aInputs)
	{
		if(aPath.indexOf(node.aInputs[i].uOutputNode) >= 0)
		{
			// cycle found!
			LogError("[%s] Cycle found:", m_sName.c_str());
			fora(j, aPath)
			{
				LogError(" %s <-", m_aNodes[aPath[j]].sName.c_str());
			}
			LogError(" %s\n", m_aNodes[node.aInputs[i].uOutputNode].sName.c_str());
			m_hasError = true;
		}
		buildExecutionOrder(node.aInputs[i].uOutputNode, aPath);
	}

	m_aExecutionOrder.push_back(uStartFrom);

	aPath.erase(aPath.size() - 1);
}

void CRenderGraph::render(float fDeltaTime, CFinalTarget *pFinalTarget, CRenderGraphData *pGraphData)
{
	IGXContext *pCtx = m_pRender->getDevice()->getThreadContext();

	fora(i, m_aExecutionOrder)
	{
		UINT uNode = m_aExecutionOrder[i];
		Node &node = m_aNodes[uNode];

		CRenderGraphData::NodeData &nodeData = pGraphData->m_aNodeData[uNode];

		fora(j, nodeData.aPassthroughInputs)
		{
			CRenderGraphData::PassthroughInput &passthroughInput = nodeData.aPassthroughInputs[j];
			// copy passthroughInput.pTex to nodeData.aOutputs[passthroughInput.iOutput]
			pCtx->copyRenderTarget(passthroughInput.pTex, nodeData.aOutputs[passthroughInput.iOutput]);
		}
		
		node.pNode->process(fDeltaTime, pFinalTarget, nodeData.aInputs, nodeData.aInputs.size(), nodeData.pNodeData, nodeData.aOutputs, nodeData.aOutputs.size());

		fora(j, nodeData.aPassthroughOutputs)
		{
			CRenderGraphData::PassthroughOutput &passthroughOutput = nodeData.aPassthroughOutputs[j];
			pCtx->copyRenderTarget(passthroughOutput.pTex, passthroughOutput.pNextRT);
		}
	}
}

void CRenderGraph::updateVisibility(CFinalTarget *pFinalTarget, CRenderGraphData *pGraphData)
{
	fora(i, m_aExecutionOrder)
	{
		UINT uNode = m_aExecutionOrder[i];
		Node &node = m_aNodes[uNode];

		CRenderGraphData::NodeData &nodeData = pGraphData->m_aNodeData[uNode];

		node.pNode->updateVisibility(pFinalTarget, nodeData.pNodeData);
	}
}

template<typename T>
static void PadWithNull(Array<T> &a, UINT uSize)
{
	for(UINT i = a.size(); i < uSize; ++i)
	{
		a[i] = NULL;
	}
}

void CRenderGraph::newGraphData(CFinalTarget *pFinalTarget, CRenderGraphData **ppOut)
{
	assert(!m_hasError);
	if(m_hasError)
	{
		return;
	}

	UINT uWidth;
	UINT uHeight;

	pFinalTarget->getSize(&uWidth, &uHeight);

	CRenderGraphData *pGraphData = new CRenderGraphData();

	pGraphData->m_aNodeData.resize(m_aNodes.size());

	*ppOut = pGraphData;

	forar(i, m_aExecutionOrder)
	{
		UINT uNode = m_aExecutionOrder[i];
		Node &node = m_aNodes[uNode];
		CRenderGraphData::NodeData &nodeData = pGraphData->m_aNodeData[uNode];

		node.pNode->newData(&nodeData.pNodeData);
		nodeData.pNodeData->resize(uWidth, uHeight);

		UINT uCount;
		const XRenderGraphNodeInputDesc *pInputDesc = node.pNode->getInputsDesc(&uCount);
		if(nodeData.aInputs.size() == 0)
		{
			Array<IGXBaseTexture*>(NULL, uCount).swap(nodeData.aInputs);
		}
		else
		{
			PadWithNull(nodeData.aInputs, uCount);
		}


		node.pNode->getOutputsDesc(&uCount);
		if(nodeData.aOutputs.size() == 0)
		{
			Array<IGXSurface*>(NULL, uCount).swap(nodeData.aOutputs);
		}
		else
		{
			PadWithNull(nodeData.aOutputs, uCount);
		}

		if(uNode == m_iFinalNode)
		{
			bool res = pFinalTarget->getTarget(&nodeData.aOutputs[m_iFinalNodeOutput]);
			assert(res);
		}
		
		fora(j, node.aInputs)
		{
			Input &inp = node.aInputs[j];
			Output &outp = m_aNodes[inp.uOutputNode].aOutputs[inp.iOutput];
			//const XRenderGraphNodeInputDesc &desc = pInputDesc[inp.iInput];
			CRenderGraphData::NodeData &prevNodeData = pGraphData->m_aNodeData[inp.uOutputNode];

			PadWithNull(prevNodeData.aOutputs, (UINT)inp.iOutput);

			if(inp.iPassthroughTo >= 0 && outp.aConnectedInputs.size() == 1)
			{
				// passthrough allowed!

				if(inp.isRequired)
				{
					IGXBaseTexture *pInpTex = findTextureInChain(uNode, inp, pGraphData);
					if(pInpTex)
					{
						PadWithNull(nodeData.aInputs, (UINT)inp.iInput);

						add_ref(pInpTex);
						nodeData.aInputs[inp.iInput] = pInpTex;
					}
					else
					{
						IGXBaseTexture *pRT = NULL;
						m_aNodes[inp.uOutputNode].pNode->newOutputTarget(inp.iOutput, uWidth, uHeight, &pRT);
						nodeData.aInputs[inp.iInput] = pRT;

						// save nodeData.aOutputs[inp.iPassthroughTo] as pNextRT
						// after node processing copy from pRT to pNextRT

						add_ref(pRT);

						nodeData.aPassthroughOutputs.push_back({pRT, nodeData.aOutputs[inp.iPassthroughTo]});

						nodeData.aOutputs[inp.iPassthroughTo] = pRT->asRenderTarget();
					}
				}

				add_ref(nodeData.aOutputs[inp.iPassthroughTo]);
				prevNodeData.aOutputs[inp.iOutput] = nodeData.aOutputs[inp.iPassthroughTo];
			}
			else if(prevNodeData.aOutputs.size() <= (UINT)inp.iOutput || !prevNodeData.aOutputs[inp.iOutput])
			{
				// standard connection
				IGXBaseTexture *pRT = NULL;
				m_aNodes[inp.uOutputNode].pNode->newOutputTarget(inp.iOutput, uWidth, uHeight, &pRT);

				prevNodeData.aOutputs[inp.iOutput] = pRT->asRenderTarget();

				fora(k, outp.aConnectedInputs)
				{
					InOut &tmpInp = outp.aConnectedInputs[k];
					add_ref(pRT);

					if(tmpInp.inp.iPassthroughTo >= 0)
					{
						pGraphData->m_aNodeData[tmpInp.uInputNode].aPassthroughInputs.push_back({pRT, tmpInp.inp.iPassthroughTo});

						if(tmpInp.inp.isRequired)
						{
							IGXBaseTexture *pInpTex = findTextureInChain(tmpInp.uInputNode, tmpInp.inp, pGraphData);
							if(pInpTex)
							{
								PadWithNull(pGraphData->m_aNodeData[tmpInp.uInputNode].aInputs, (UINT)tmpInp.inp.iInput);
								add_ref(pInpTex);
								pGraphData->m_aNodeData[tmpInp.uInputNode].aInputs[inp.iInput] = pInpTex;
							}
							else
							{
								IGXBaseTexture *pRT = NULL;
								m_aNodes[tmpInp.inp.uOutputNode].pNode->newOutputTarget(inp.iOutput, uWidth, uHeight, &pRT);
								pGraphData->m_aNodeData[tmpInp.uInputNode].aInputs[inp.iInput] = pRT;
								add_ref(pRT);
								pGraphData->m_aNodeData[tmpInp.uInputNode].aPassthroughOutputs.push_back({pRT, pGraphData->m_aNodeData[tmpInp.uInputNode].aOutputs[inp.iPassthroughTo]});
								pGraphData->m_aNodeData[tmpInp.uInputNode].aOutputs[inp.iPassthroughTo] = pRT->asRenderTarget();
							}
						}
					}
					else
					{
						PadWithNull(pGraphData->m_aNodeData[tmpInp.uInputNode].aInputs, (UINT)tmpInp.inp.iInput);

						pGraphData->m_aNodeData[tmpInp.uInputNode].aInputs[tmpInp.inp.iInput] = pRT;
					}
				}

				mem_release(pRT);
			}
		}
	}
}

IGXBaseTexture* CRenderGraph::findTextureInChain(UINT uNode, const Input &inp, CRenderGraphData *pGraphData)
{
	if(uNode == m_iFinalNode)
	{
		return(NULL);
	}

	Node &node = m_aNodes[uNode];

	// m_aNodes[inp.uOutputNode].aInputs[inp.
	Output &outp = node.aOutputs[inp.iPassthroughTo];
	if(outp.aConnectedInputs.size() == 1)
	{
		InOut &inout = outp.aConnectedInputs[0];

		if(pGraphData->m_aNodeData[inout.uInputNode].aInputs[inout.inp.iInput])
		{
			return(pGraphData->m_aNodeData[inout.uInputNode].aInputs[inout.inp.iInput]);
		}
		
		if(inout.inp.iPassthroughTo >= 0)
		{
			return(findTextureInChain(inout.uInputNode, inout.inp, pGraphData));
		}

		return(NULL);
	}
	else
	{
		fora(i, outp.aConnectedInputs)
		{
			InOut &inout = outp.aConnectedInputs[0];

			if(inout.inp.iPassthroughTo < 0)
			{
				return(pGraphData->m_aNodeData[inout.uInputNode].aInputs[inout.inp.iInput]);
			}
		}
	}

	return(NULL);
}

bool XMETHODCALLTYPE CRenderGraph::getNodeInstance(const char *szNodeInstance, IXRenderGraphNodeInstance **ppNodeInstance)
{
	int idx = m_aNodes.indexOf(szNodeInstance, [](const Node &node, const char *szName){
		return(!strcmp(node.sName.c_str(), szName));
	});

	if(idx < 0)
	{
		return(false);
	}

	*ppNodeInstance = new CNodeInstance(this, (UINT)idx, m_aNodes[idx].pNode);
	return(true);
}

//##########################################################################

CNodeInstance::CNodeInstance(CRenderGraph *pGraph, UINT uNode, IXRenderGraphNode *pNode):
	m_pGraph(pGraph),
	m_uNodeId(uNode),
	m_pNode(pNode)
{
	add_ref(m_pGraph);
	add_ref(m_pNode);
}
CNodeInstance::~CNodeInstance()
{
	mem_release(m_pGraph);
	mem_release(m_pNode);
}

IXRenderGraphNode* XMETHODCALLTYPE CNodeInstance::getNode()
{
	return(m_pNode);
}

bool XMETHODCALLTYPE CNodeInstance::getNodeData(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData **ppOut)
{
	CFinalTarget *pFT = (CFinalTarget*)pFinalTarget;
	if(pFT->getGraph() == m_pGraph)
	{
		IXRenderGraphNodeData *pNodeData = pFT->getGraphData()->m_aNodeData[m_uNodeId].pNodeData;
		add_ref(pNodeData);
		*ppOut = pNodeData;
		return(true);
	}
	return(false);
}

void XMETHODCALLTYPE CNodeInstance::setTexture(IXRenderTarget *pFinalTarget, UINT uIdx, IXTexture *pTexture)
{
	CFinalTarget *pFT = (CFinalTarget*)pFinalTarget;
	if(pFT->getGraph() == m_pGraph)
	{
		m_pNode->setTexture(pFT->getGraphData()->m_aNodeData[m_uNodeId].pNodeData, uIdx, pTexture);
	}
}
void XMETHODCALLTYPE CNodeInstance::setProp(IXRenderTarget *pFinalTarget, UINT uIdx, const float4_t &vProp)
{
	CFinalTarget *pFT = (CFinalTarget*)pFinalTarget;
	if(pFT->getGraph() == m_pGraph)
	{
		m_pNode->setProp(pFT->getGraphData()->m_aNodeData[m_uNodeId].pNodeData, uIdx, vProp);
	}
}
void XMETHODCALLTYPE CNodeInstance::setFlag(IXRenderTarget *pFinalTarget, UINT uIdx, bool bVal)
{
	CFinalTarget *pFT = (CFinalTarget*)pFinalTarget;
	if(pFT->getGraph() == m_pGraph)
	{
		m_pNode->setFlag(pFT->getGraphData()->m_aNodeData[m_uNodeId].pNodeData, uIdx, bVal);
	}
}

//##########################################################################

CRenderGraphData::~CRenderGraphData()
{
	fora(i, m_aNodeData)
	{
		NodeData &nodeData = m_aNodeData[i];

		fora(j, nodeData.aOutputs)
		{
			mem_release(nodeData.aOutputs[j]);
		}

		fora(j, nodeData.aInputs)
		{
			mem_release(nodeData.aInputs[j]);
		}


		fora(j, nodeData.aPassthroughInputs)
		{
			mem_release(nodeData.aPassthroughInputs[j].pTex);
		}


		fora(j, nodeData.aPassthroughOutputs)
		{
			mem_release(nodeData.aPassthroughOutputs[j].pTex);
			mem_release(nodeData.aPassthroughOutputs[j].pNextRT);
		}
	}
}
