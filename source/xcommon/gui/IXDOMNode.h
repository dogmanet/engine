#ifndef __IXDOMNODE_H
#define __IXDOMNODE_H

#include <gdefines.h>
#include "IXDOMString.h"
#include "IXHTMLElement.h"
#include "IXCSSStyle.h"

XENUM(XDOM_NODE_TYPE,
	XNODE_TYPE_ELEMENT = 1,
	//XNODE_TYPE_ATTRIBUTE = 2, // obsolete
	XNODE_TYPE_TEXT = 3,
	XNODE_TYPE_CDATASECTION = 4,
	//XNODE_TYPE_ENTITYREFERENCE = 5, // historical
	//XNODE_TYPE_ENTITY = 6, // historical
	//XNODE_TYPE_PROCESSINGINSTRUCTION = 7, // xml only
	XNODE_TYPE_COMMENT = 8,
	XNODE_TYPE_DOCUMENT = 9,
	XNODE_TYPE_DOCUMENTTYPE = 10,
	XNODE_TYPE_DOCUMENTFRAGMENT = 11,
	//XNODE_TYPE_NOTATION = 12, // historical
	XNODE_TYPE_BASE = 21//,
	//XNODE_TYPE_CHARACTERDATA = 22 // abstract
	);

XENUM(XDOM_ELEMENT_POSITION,
	XDOM_ELEMENT_POSITION_BEFOREBEGIN,
	XDOM_ELEMENT_POSITION_AFTERBEGIN,
	XDOM_ELEMENT_POSITION_BEFOREEND,
	XDOM_ELEMENT_POSITION_AFTEREND
	);

class IXDOMDocument;
class IXDOMDocumentFragment;
class IXDOMText;
class IXDOMElement;
class IXDOMNodeList;
class IXDOMElementList;

class IXDOMNode: public IXUnknown
{
public:
	// virtual ??? XMETHODCALLTYPE attributes() const = 0; //?
	virtual void XMETHODCALLTYPE childNodes(IXDOMNodeList **ppOut) const = 0;
	virtual void XMETHODCALLTYPE clear() = 0;
	virtual void XMETHODCALLTYPE cloneNode(IXDOMNode **ppOut, bool deep = true) const = 0;
	virtual bool XMETHODCALLTYPE hasAttributes() const = 0;
	virtual bool XMETHODCALLTYPE hasChildNodes() const = 0;

	virtual bool XMETHODCALLTYPE isAttr() const = 0;
	virtual bool XMETHODCALLTYPE isCDATASection() const = 0;
	//virtual bool XMETHODCALLTYPE isCharacterData() const = 0;
	virtual bool XMETHODCALLTYPE isComment() const = 0;
	virtual bool XMETHODCALLTYPE isDocument() const = 0;
	virtual bool XMETHODCALLTYPE isDocumentFragment() const = 0;
	virtual bool XMETHODCALLTYPE isDocumentType() const = 0;
	virtual bool XMETHODCALLTYPE isElement() const = 0;
	//virtual bool XMETHODCALLTYPE isEntity() const = 0;
	//virtual bool XMETHODCALLTYPE isEntityReference() const = 0;
	//virtual bool XMETHODCALLTYPE isNotation() const = 0;
	//virtual bool XMETHODCALLTYPE isProcessingInstruction() const = 0;
	virtual bool XMETHODCALLTYPE isText() const = 0;

	virtual XDOM_NODE_TYPE XMETHODCALLTYPE getNodeType() const = 0;
	virtual void XMETHODCALLTYPE getOwnerDocument(IXDOMDocument **ppOut) const = 0;
	virtual void XMETHODCALLTYPE getParentNode(IXDOMNode **ppOut) const = 0;

	virtual void XMETHODCALLTYPE getPreviousSibling(IXDOMNode **ppOut) const = 0;
	virtual void XMETHODCALLTYPE getPreviousSiblingElement(IXDOMElement **ppOut, const char *szTagName = NULL) const = 0;
	virtual void XMETHODCALLTYPE getFirstChild(IXDOMNode **ppOut) const = 0;
	virtual void XMETHODCALLTYPE getFirstChildElement(IXDOMElement **ppOut, const char *szTagName = NULL) const = 0;
	virtual void XMETHODCALLTYPE getLastChild(IXDOMNode **ppOut) const = 0;
	virtual void XMETHODCALLTYPE getLastChildElement(IXDOMElement **ppOut, const char *szTagName = NULL) const = 0;
	virtual void XMETHODCALLTYPE getNextSibling(IXDOMNode **ppOut) const = 0;
	virtual void XMETHODCALLTYPE getNextSiblingElement(IXDOMElement **ppOut, const char *szTagName = NULL) const = 0;

	virtual void XMETHODCALLTYPE appendChild(IXDOMNode *pNewChild) = 0;
	virtual void XMETHODCALLTYPE removeChild(IXDOMNode *pOldChild) = 0;
	virtual void XMETHODCALLTYPE replaceChild(IXDOMNode *pNewChild, IXDOMNode *pOldChild) = 0;
	virtual void XMETHODCALLTYPE insertAfter(IXDOMNode *pNewChild, IXDOMNode *pRefChild) = 0;
	virtual void XMETHODCALLTYPE insertBefore(IXDOMNode *pNewChild, IXDOMNode *pRefChild) = 0;

//	//virtual IXDOMAttr XMETHODCALLTYPE asAttr() const                                                               = 0;
//	//virtual IXDOMCDATASection XMETHODCALLTYPE asCDATASection() const                                               = 0;
//	//virtual IXDOMCharacterData XMETHODCALLTYPE asCharacterData() const                                              = 0;
//	//virtual IXDOMComment XMETHODCALLTYPE asComment() const                                                            = 0;
//	virtual IXDOMDocumentFragment* XMETHODCALLTYPE asDocumentFragment() const = 0;
//	//virtual IXDOMdocumentType* XMETHODCALLTYPE asDocumentType() const                                               = 0;
//	virtual IXDOMElement* XMETHODCALLTYPE asElement() const = 0;
//	//virtual IXDOMEntity XMETHODCALLTYPE asEntity() const                                                             = 0;
//	//virtual IXDOMEntityReference XMETHODCALLTYPE asEntityReference() const                                            = 0;
//	//virtual IXDOMNotation XMETHODCALLTYPE asNotation() const                                                       = 0;
//	//virtual IXDOMProcessingInstruction XMETHODCALLTYPE asProcessingInstruction() const                              = 0;
//	virtual IXDOMText* XMETHODCALLTYPE asText() const = 0;

	virtual bool XMETHODCALLTYPE isChildOf(IXDOMNode *pNode) = 0;
	virtual bool XMETHODCALLTYPE isAfter(IXDOMNode *pNode) = 0;

	virtual void setTextContent(const char *szText) = 0;
	virtual void getTextContent(IXDOMString **ppOut) = 0;
};

class IXDOMElement: public IXDOMNode
{
public:
	virtual void XMETHODCALLTYPE children(IXDOMElementList **ppOut) const = 0;

	virtual void XMETHODCALLTYPE insertAdjacentElement(XDOM_ELEMENT_POSITION position, IXDOMNode *pNewChild) = 0;
	virtual void XMETHODCALLTYPE insertAdjacentHTML(XDOM_ELEMENT_POSITION position, const char *szHTML) = 0;
	virtual void XMETHODCALLTYPE insertAdjacentText(XDOM_ELEMENT_POSITION position, const char *szText) = 0;

	virtual void XMETHODCALLTYPE setInnerHTML(const char *szText) = 0;
	virtual void XMETHODCALLTYPE getInnerHTML(IXDOMString **ppOut) = 0;

	virtual void XMETHODCALLTYPE setAttribute(const char *szName, const char *szValue) = 0;
	virtual void XMETHODCALLTYPE removeAttribute(const char *szName) = 0;
	virtual bool XMETHODCALLTYPE hasAttribute(const char *szName) = 0;
	virtual const char* XMETHODCALLTYPE getAttribute(const char *szName) = 0;

	virtual void XMETHODCALLTYPE getStyle(IXCSSStyle **ppOut) = 0;
	virtual void XMETHODCALLTYPE getComputedStyle(const IXCSSStyle **ppOut) = 0;

	virtual void XMETHODCALLTYPE setClassName(const char *szClassName) = 0;
	virtual const char* XMETHODCALLTYPE getClassName() = 0;

	virtual void XMETHODCALLTYPE classAdd(const char *szClass) = 0;
	virtual void XMETHODCALLTYPE classRemove(const char *szClass) = 0;
	virtual void XMETHODCALLTYPE classToggle(const char *szClass) = 0;
	virtual bool XMETHODCALLTYPE classExists(const char *szClass) = 0;

	virtual void XMETHODCALLTYPE setId(const char *szName) = 0;
	virtual const char* XMETHODCALLTYPE getId() = 0;

	virtual void XMETHODCALLTYPE setPointerCapture() = 0;
	virtual void XMETHODCALLTYPE releasePointerCapture() = 0;


	//! Returns a Number representing the inner height of the element.
	virtual UINT XMETHODCALLTYPE getClientHeight() = 0;
	//! Returns a Number representing the width of the left border of the element.
	virtual UINT XMETHODCALLTYPE getClientLeft() = 0;
	//! Returns a Number representing the width of the top border of the element.
	virtual UINT XMETHODCALLTYPE getClientTop() = 0;
	//! Returns a Number representing the inner width of the element.
	virtual UINT XMETHODCALLTYPE getClientWidth() = 0;


	//!  Returns a Number representing the maximum left scroll offset possible for the element.
	virtual UINT XMETHODCALLTYPE getScrollLeftMax() = 0;
	//!  Is a Number representing the left scroll offset of the element.
	virtual UINT XMETHODCALLTYPE getScrollLeft() = 0;
	virtual void XMETHODCALLTYPE setScrollLeft(UINT uVal) = 0;

	//!  Returns a Number representing the maximum top scroll offset possible for the element.
	virtual UINT XMETHODCALLTYPE getScrollTopMax() = 0;
	//!  A Number representing number of pixels the top of the document is scrolled vertically.
	virtual UINT XMETHODCALLTYPE getScrollTop() = 0;
	virtual void XMETHODCALLTYPE setScrollTop(UINT uVal) = 0;

	virtual void XMETHODCALLTYPE getHTMLElement(IXHTMLElement **ppOut) = 0;
};

class IXDOMText: public IXDOMNode
{
public:
};

class IXDOMDocumentFragment: public IXDOMNode
{
public:
};

class IXDOMNodeList: public IXUnknown
{
public:
	virtual bool XMETHODCALLTYPE getItem(UINT uIndex, IXDOMNode **ppOut) = 0;
	virtual UINT XMETHODCALLTYPE getLength() = 0;
	virtual bool XMETHODCALLTYPE nextNode(IXDOMNode **ppOut) = 0;
	virtual void XMETHODCALLTYPE reset() = 0;
};

class IXDOMElementList: public IXUnknown
{
public:
	virtual bool XMETHODCALLTYPE getItem(UINT uIndex, IXDOMElement **ppOut) = 0;
	virtual UINT XMETHODCALLTYPE getLength() = 0;
	virtual bool XMETHODCALLTYPE nextNode(IXDOMElement **ppOut) = 0;
	virtual void XMETHODCALLTYPE reset() = 0;
};

#endif
