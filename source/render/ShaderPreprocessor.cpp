
/*
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017
license MIT see in LICENSE or
https://s4g.su/
*/

#include "ShaderPreprocessor.h"
#include <core/IFile.h>

typedef enum token_e
{
	TOK_NOP = 0,

	TOK_LBRACKET,
	TOK_RBRACKET,

	TOK_OPERAND,

	TOK_OPS_START,

	TOK_PR_0_S,
	TOK_OP_EQUAL,
	TOK_OP_GREATER,
	TOK_OP_GREATER_EQUAL,
	TOK_OP_LESSER,
	TOK_OP_LESSER_EQUAL,
	TOK_OP_NOT_EQUAL,
	TOK_PR_0_E,

	TOK_PR_1_S,
	TOK_OP_AND,
	TOK_PR_1_E,

	TOK_PR_2_S,
	TOK_OP_OR,
	TOK_PR_2_E,

	TOK_PR_M1_S,
	TOK_OP_ADD,
	TOK_OP_SUB,
	TOK_PR_M1_E,


	TOK_PR_M2_S,
	TOK_OP_DIV,
	TOK_OP_MUL,
	TOK_PR_M2_E,

	TOK_PR_M3_S,
	TOK_OP_NOT,
	TOK_PR_M3_E,

	TOK_OPS_END

	//TOK_FNS_START,



	//TOK_FNS_END

}
token_t;

typedef struct citem_s
{
	token_t tok;
	struct citem_s * pNext;
	struct citem_s * pPrev;
	union
	{
		int i;
		float f;
	} val;
}
citem_t;

static void InsertToken(citem_t **ppFirst, citem_t **ppLast, token_t tok);
static void InsertTokenI(citem_t **ppFirst, citem_t **ppLast, token_t tok, int val);
static void InsertTokenF(citem_t **ppFirst, citem_t **ppLast, token_t tok, float val);

static bool tokcmp(const char *pHaystack, const char *pNeedle);

static citem_t* Calculate(citem_t *pStart);




void CShaderPreprocessor::define(const char *def, int len, bool bTemp)
{
	if(len == 0)
	{
		len = (int)strlen(def);
	}

	String name;
	String arg;

	bool sBegan = false;
	bool sName = false;
	bool sArglist = false;
	bool sWaitArg = false;

	_define _def;
	_def.isMacro = false;
	_def.isUndef = false;

	for(int i = 0; i < len; ++i)
	{
		if(sBegan)
		{
			if(sName) // read name
			{
				if(isspace(def[i] & 0xFF))
				{
					sName = false;
					sWaitArg = true;
				}
				else
				{
					if(def[i] == '(')
					{
						sName = false;
						sArglist = true;
						_def.isMacro = true;
						sWaitArg = true;
					}
					else
					{
						name += def[i];
					}
				}
			}
			else if(sArglist) // read args
			{
				if(!isspace(def[i] & 0xFF))
				{
					if(def[i] == ')')
					{
						if(!sWaitArg)
						{
							_def.vArgs.push_back(arg);
							arg = "";
						}
						sWaitArg = true;
						sArglist = false;
					}
					else if(def[i] == ',')
					{
						if(!sWaitArg)
						{
							_def.vArgs.push_back(arg);
							arg = "";
						}
						sWaitArg = true;
					}
					else
					{
						arg += def[i];
						sWaitArg = false;
					}
				}
			}
			else // read value
			{
				if(!isspace(def[i] & 0xFF) || !sWaitArg)
				{
					sWaitArg = false;
					arg += def[i];
				}
			}
		}
		else
		{
			if(!isspace(def[i] & 0xFF))
			{
				sBegan = true;
				sName = true;
				name += def[i];
			}
		}
	}
	_def.sVal = arg;
	_def.isTemp = bTemp;
	m_mDefs[name] = _def;
}
void CShaderPreprocessor::undef(const String &def)
{
	if(m_mDefs.KeyExists(def))
	{
		m_mDefs[def].isUndef = true;
	}
}
void CShaderPreprocessor::addIncPath(const String &s)
{
	m_vIncludes.push_back({s});
}

void CShaderPreprocessor::undefTemp()
{
	for(AssotiativeArray<String, _define>::Iterator i = m_mDefs.begin(); i; ++i)
	{
		if(i.second->isTemp)
		{
			i.second->isUndef = true;
		}
	}
}

String CShaderPreprocessor::process(const char *src, const char *file)
{
//	AssotiativeArray<String, _define> mDefs = m_mDefs;
	int len = (int)strlen(src);

	Array<char, 1024> out;
	out.reserve(len);

	int sNewLine = true;
	int sDirective = false;

	String dir;

	bool bCondState = true;
	Stack<bool> bCondStack;
	UINT uInCount = 0;
	bool wasElifSucceded = false;
	Stack<bool> wasElifSuccededStack;

	if(file)
	{
		String line = String("#line 1 \"") + file + "\"\n";
		for(int j = 0, l = line.length(); j < l; ++j)
		{
			out.push_back(line[j]);
		}
	}

	String str;

	bool isOnelineComment = false;
	int iMultilineCommentCount = 0;

	char cp = 0;
	int line = 0;
	for(int i = 0; i <= len; ++i)
	{
		char ch = src[i];
		if(!ch)
		{
			ch = '\n';
		}


		if(ch == '\r')
		{
			continue;
		}
		if(ch == '\n')
		{
			++line;
		}
		if(isOnelineComment)
		{
			if(ch == '\n')
			{
				isOnelineComment = false;
				// out.push_back(ch);
			}
			else
			{
				continue;
			}
		}
		if(iMultilineCommentCount > 0)
		{
			if(ch == '*' && i + 1 < len && src[i + 1] == '/')
			{
				--iMultilineCommentCount;
				++i;
			}
			/*else if(ch == '/' && i + 1 < len && src[i + 1] == '*')
			{
				++iMultilineCommentCount;
			}*/
			if(ch == '\n')
			{
				out.push_back(ch);
			}
			continue;
		}
		if(ch == '/' && i + 1 < len)
		{
			if(src[i + 1] == '/')
			{
				isOnelineComment = true;
				continue;
			}
			if(src[i + 1] == '*')
			{
				++iMultilineCommentCount;
				continue;
			}
		}

		if(!isspace(0xFF & ch) || !sNewLine)
		{
			if(sNewLine)
			{
				sNewLine = false;
				if(ch == '#')
				{
					sDirective = true;
				}
				else if(bCondState)
				{
					str += ch;
					//out.push_back(ch);
				}
			}
			else
			{
				if(sDirective)
				{
					if(ch != '\n' || cp == '\\')
					{
						dir += ch;
					}
				}
				else if((ch != '\n' || cp == '\\') && bCondState)
				{
					str += ch;
					//out.push_back(ch);
				}
			}

			if(ch == '\n' && cp != '\\')
			{
				/*if(!sNewLine)
				{

				}*/

				if(sDirective) // parse directive
				{
					bool sDName = false;
					bool sDPrm = false;
					String dName, dArgs;
					//out.push_back('>');
					for(int j = 0, l = dir.length(); j < l - 1; ++j)
					{
						if(dir[j] == '\\' && dir[j + 1] == '\n')
						{
							dir[j] = ' ';
							dir[j + 1] = ' ';
						}
					}
					for(int j = 0, l = dir.length(); j < l; ++j)
					{
						char c = dir[j];
						if(!isspace(0xFF & c) || sDPrm || true)
						{
							if(!isspace(0xFF & c) && !sDName && !sDPrm)
							{
								sDName = true;
							}
							if(sDName)
							{
								if(isspace(0xFF & c))
								{
									sDName = false;
									sDPrm = true;
								}
								else
								{
									dName += c;
								}
							}
							else if(sDPrm)
							{
								dArgs += c;
							}
						}
						//out.push_back(dir[j]);
					}
					dArgs = dArgs.trim();
					if(dName == "define")
					{
						if(bCondState)
						{
							define(dArgs.c_str());
						}
					}
					else if(dName == "undef")
					{
						if(bCondState)
						{
							undef(dArgs);
						}
					}
					else if(dName == "ifdef")
					{
						if(!bCondState)
						{
							uInCount++;
						}
						else
						{
							bCondStack.push(bCondState);
							bCondState = isDefined(dArgs);
						}
					}
					else if(dName == "ifndef")
					{
						if(!bCondState)
						{
							uInCount++;
						}
						else
						{
							bCondStack.push(bCondState);
							bCondState = !isDefined(dArgs);
						}
					}
					else if(dName == "if")
					{
						if(!bCondState)
						{
							uInCount++;
						}
						else
						{
							bCondStack.push(bCondState);
							//bCondState = ...;
							bCondState = isTrue(dArgs);

							wasElifSuccededStack.push(wasElifSucceded);
							wasElifSucceded = bCondState;
						}
					}
					else if(dName == "else")
					{
						if(bCondState || uInCount == 0) // &&?
						{
							if(wasElifSucceded)
							{
								bCondState = false;
							}
							else
							{
								bCondState = !bCondState;
							}
						}
					}
					else if(dName == "elif")
					{
						if(uInCount == 0)
						{
							if(wasElifSucceded)
							{
								bCondState = false;
							}
							else
							{
								wasElifSucceded = bCondState = isTrue(dArgs);
							}
						}
					}
					else if(dName == "endif")
					{
						if(!bCondState && uInCount != 0)
						{
							uInCount--;
						}
						else
						{
							bCondStack.pop(&bCondState);
							wasElifSuccededStack.pop(&wasElifSucceded);
						}
					}
					else if(dName == "include")
					{
						if(bCondState)
						{
							String path;
							dArgs = makeExpansion(dArgs).trim();
							bool isLocal = dArgs[0] == '"';
							String inc = getInclude(dArgs.substr(1, dArgs.length() - 2), file, &path, isLocal);
							inc = process(inc.c_str(), path.c_str());
							for(int j = 0, l = inc.length(); j < l; ++j)
							{
								out.push_back(inc[j]);
							}
							if(file)
							{
								inc = String("#line ") + line + " \"" + file + "\"\n";
							}
							for(int j = 0, l = inc.length(); j < l; ++j)
							{
								out.push_back(inc[j]);
							}
						}
					}
					else
					{
						if(bCondState)
						{
							out.push_back('#');
							for(int j = 0, l = dir.length(); j < l; ++j)
							{
								out.push_back(dir[j]);
							}
						}
					}
					//out.push_back('<');
					out.push_back('\n');
				}
				else
				{
					str += ch;
					str = makeExpansion(str);
					for(int ii = 0, ll = str.length(); ii < ll; ++ii)
					{
						out.push_back(str[ii] == '\r' ? '\n' : str[ii]);
					}
					str = "";
				}
				sNewLine = true;
				sDirective = false;
				dir = "";


			}
		}
		else if((ch == '\n'/* || ch == '\r'*/) && cp != '\\')
		{
			str = "";
			out.push_back(ch);
		}
		else
		{
			str += ch;
		}
	}

	String sOut;
	if(out.size())
	{
		sOut.reserve(out.size());
		memcpy((void*)(sOut.c_str()), &(out[0]), out.size() * sizeof(char));
		((char*)(sOut.c_str()))[out.size()] = 0;
	}

	/*bool a;
	a = istrue("true && false");//
	a = istrue("true || false");
	a = istrue("(true || false) || false");//
	a = istrue("(true && false) || true");//
	a = istrue("(true || false) && true");
	*/

//	m_mDefs = mDefs; ///< restore original defines

	return(sOut);
}

bool CShaderPreprocessor::isDefined(const String &def)
{
	if(!m_mDefs.KeyExists(def))
	{
		return(false);
	}
	return(!m_mDefs[def].isUndef);
}

bool CShaderPreprocessor::isTrue(const String &_expr)
{
	String expr = makeExpansion(checkDefines(_expr));

	citem_t * pFirst = NULL;
	citem_t * pLast = NULL;
	int iLen, i, iBracketLvl;
	bool bReadingNumber = false;
	bool bPointFound = false;
	float fVal = 0.0f;
	float fMult = 1.0f;
	
	const char * szStr = expr.c_str();

	iLen = expr.length();

	iBracketLvl = 0;

	InsertTokenF(&pFirst, &pLast, TOK_OPERAND, 0.0f);
	InsertToken(&pFirst, &pLast, TOK_OP_OR);

	for(i = 0; i < iLen; i++)
	{
		if(szStr[i] >= '0' && szStr[i] <= '9' || szStr[i] == '.')
		{
			if(!bReadingNumber)
			{
				bReadingNumber = true;
				fMult = 1.0f;
				if(szStr[i] == '.')
				{
					bPointFound = true;
					fVal = 0.0f;
				}
				else
				{
					bPointFound = false;
					fVal = (float)(szStr[i] - '0');
				}
			}
			else
			{
				if(szStr[i] == '.')
				{
					bPointFound = true;//@@TODO: check: only one point per number
				}
				else if(bPointFound)
				{
					fMult *= 0.1f;
					fVal += (szStr[i] - '0') * fMult;
				}
				else
				{
					fVal *= 10.0f;
					fVal += szStr[i] - '0';
				}
			}
		}
		else if(bReadingNumber)
		{
			bReadingNumber = false;
			InsertTokenF(&pFirst, &pLast, TOK_OPERAND, fVal);
		}
		if(bReadingNumber && i == iLen - 1)
		{
			bReadingNumber = false;
			InsertTokenF(&pFirst, &pLast, TOK_OPERAND, fVal);
		}

		/*if(tokcmp(&szStr[i], "sin"))
		{
		InsertToken(&pFirst, &pLast, TOK_FN_SIN);
		}
		if(tokcmp(&szStr[i], "cos"))
		{
		InsertToken(&pFirst, &pLast, TOK_FN_COS);
		}
		if(tokcmp(&szStr[i], "log"))
		{
		InsertToken(&pFirst, &pLast, TOK_FN_LOG);
		}
		if(tokcmp(&szStr[i], "exp"))
		{
		InsertToken(&pFirst, &pLast, TOK_FN_EXP);
		}
		if(tokcmp(&szStr[i], "sqrt"))
		{
		InsertToken(&pFirst, &pLast, TOK_FN_SQRT);
		}*/



		switch(szStr[i])
		{
		case '(':
			InsertTokenI(&pFirst, &pLast, TOK_LBRACKET, iBracketLvl);
			InsertTokenF(&pFirst, &pLast, TOK_OPERAND, 0.0f);
			InsertToken(&pFirst, &pLast, TOK_OP_ADD);
			iBracketLvl++;
			break;

		case ')':
			iBracketLvl--;
			//InsertToken(&pFirst, &pLast, TOK_OP_ADD);
			//InsertTokenF(&pFirst, &pLast, TOK_OPERAND, 0.0f);
			InsertTokenI(&pFirst, &pLast, TOK_RBRACKET, iBracketLvl);
			break;

		case '+':
			InsertToken(&pFirst, &pLast, TOK_OP_ADD);
			break;

		case '-':
			InsertToken(&pFirst, &pLast, TOK_OP_SUB);
			break;

		case '*':
			InsertToken(&pFirst, &pLast, TOK_OP_MUL);
			break;

		case '/':
			InsertToken(&pFirst, &pLast, TOK_OP_DIV);
			break;

		case '>':
			if(szStr[i + 1] == '=')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_GREATER_EQUAL);
			}
			else
			{
				InsertToken(&pFirst, &pLast, TOK_OP_GREATER);
			}
			break;

		case '<':
			if(szStr[i + 1] == '=')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_LESSER_EQUAL);
			}
			else
			{
				InsertToken(&pFirst, &pLast, TOK_OP_LESSER);
			}
			break;

		case '!':
			if(szStr[i + 1] == '=')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_NOT_EQUAL);
			}
			else
			{
				InsertToken(&pFirst, &pLast, TOK_OP_NOT);
			}
			break;

		case '=':
			if(szStr[i + 1] == '=')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_EQUAL);
			}
			break;

		case '&':
			if(szStr[i + 1] == '&')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_AND);
			}
			break;

		case '|':
			if(szStr[i + 1] == '|')
			{
				InsertToken(&pFirst, &pLast, TOK_OP_OR);
			}
			break;

		case ' ':
		case '\t':
			break;

		}
	}
	//InsertToken(&pFirst, &pLast, TOK_OP_ADD);
	//InsertTokenF(&pFirst, &pLast, TOK_OPERAND, 0.0f);
	if(pFirst)
	{
		pFirst = Calculate(pFirst);
		bool ret = pFirst->val.f != 0.0f;
		//printf("%f\n", pFirst->val.f);
		free(pFirst);
		return(ret);
	}
	else
	{
		return(false);
	}
	//system("pause");


	pFirst = NULL;
	pLast = NULL;

	return(true);
}

String CShaderPreprocessor::makeExpansion(const String &expr)
{
	if(!expr.length())
	{
		return("");
	}
	ReplList rlist;
	return(makeExpansion(expr, rlist));
}

String CShaderPreprocessor::makeExpansion(const String &_expr, ReplList &rlist)
{
	if(!_expr.length())
	{
		return("");
	}
	String tok;
	String expr = _expr;
	if(!isspace(0xFF & _expr[_expr.length() - 1]))
	{
		expr += ' ';
	}
	_define * pDef;
	bool sWaitArgs = false;
	UINT sPos = ~0;

	Array<String> szArgs;
	String sArg;
	UINT uPCount = 0;
	bool sStringify = false;
	bool sStripNext = false;

	for(UINT i = 0, l = expr.length(); i < l; ++i)
	{
		char ch = expr[i];
		if(sWaitArgs)
		{
			if(ch == '(')
			{
				if(uPCount > 1)
				{
					sArg += ch;
				}
				++uPCount;
			}
			else if(ch == ')')
			{
				--uPCount;
				if(uPCount == 0)
				{
					szArgs.push_back(sArg.trim());
					sArg = "";
					//make expansion with args

					//replace args with toks

					rlist[tok] = true;
					expr = (sPos == 0 ? "" : expr.substr(0, sPos)) + makeExpansion(replArgs(pDef->sVal, pDef, szArgs), rlist) + expr.substr(i + 1);
					//rlist[tok] = false;
					l = expr.length();
					i = sPos;
					sPos = ~0;
					tok = "";
					sWaitArgs = false;
				}
				if(uPCount > 1)
				{
					sArg += ch;
				}
			}
			else if(ch == ',' && uPCount == 1)
			{
				szArgs.push_back(sArg.trim());
				sArg = "";
			}
			else
			{
				sArg += ch;
			}
		}
		else if(
			(ch >= 'a' && ch <= 'z')
			|| (ch >= 'A' && ch <= 'Z')
			|| ch == '_'
			|| (ch >= '0' && ch <= '9')
			)
		{
			sStripNext = false;
			if(sPos > i)
			{
				sPos = i;
			}
			tok += ch;
		}
		else
		{
			if(ch == '#')
			{
				if(i + 1 < l && expr[i + 1] == '#')
				{
					//wait for 
					sStripNext = false; ///< mark to merge (true)
				}
				else
				{
					sStringify = true;
				}
			}
			if(!sStripNext)
			{
				if(tok.length() && m_mDefs.KeyExists(tok) && !(pDef = &(m_mDefs[tok]))->isUndef && (!rlist.KeyExists(tok) || !rlist[tok]))
				{
					// pDef = &(m_mDefs[tok]);
					if(pDef->isMacro)
					{
						sWaitArgs = true;
						--i;
					}
					else
					{
						rlist[tok] = true;
						expr = (sPos == 0 ? "" : expr.substr(0, sPos)) + makeExpansion(pDef->sVal, rlist) + expr.substr(sPos + tok.length());
						rlist[tok] = false;
						l = expr.length();
						i = sPos;
						sPos = ~0;
						tok = "";
					}
				}
				else if(tok.length())
				{
					tok = "";
					sPos = ~0;
				}
			}
		}
	}
	return(expr);
}

String CShaderPreprocessor::replArgs(const String &_expr, const _define *pDef, const Array<String> &szArgs)
{
	if(!_expr.length())
	{
		return("");
	}
	String tok, tok_prev;
	String expr = _expr;
	if(!isspace(0xFF & _expr[_expr.length() - 1]))
	{
		expr += ' ';
	}
	UINT sPos = ~0;
	UINT sSharpPos = ~0;
	UINT slfsPos = 0;
	bool slfsSet = false;
	UINT sMergeStart = 0;

	bool sStripNext = false;
	bool sStringify = false;

	for(UINT i = 0, l = expr.length(); i < l; ++i)
	{
		char ch = expr[i];
		if((isspace(0xFF & ch) || ch == '#'))
		{
			if(!slfsSet)
			{
				slfsPos = i;
				slfsSet = true;
			}
		}
		else
		{
			slfsSet = false;
		}
		if(ch == '#')
		{
			if(i + 1 < l && expr[i + 1] == '#')
			{
				//wait for 
				sStripNext = true; ///< mark to merge (true)
				sMergeStart = slfsPos;
			}
			else if(!sStripNext)
			{
				sStringify = true;
				sSharpPos = i;
			}
		}
		else if(
			(ch >= 'a' && ch <= 'z')
			|| (ch >= 'A' && ch <= 'Z')
			|| ch == '_'
			|| (ch >= '0' && ch <= '9')
			)
		{
			if(sPos > i)
			{
				sPos = i;
			}
			tok += ch;
		}
		else
		{
			if(tok.length())
			{
				UINT argPos = ~0;
				for(UINT j = 0; j < pDef->vArgs.size(); ++j)
				{
					if(tok == pDef->vArgs[j])
					{
						argPos = j;
						break;
					}
				}
				//search tok in arg list
				if(argPos != ~0)
				{
					String arg = szArgs[argPos];

					UINT delta = sPos + tok.length();
					if(sStringify)
					{
						arg = stringify(arg);
						sPos = sSharpPos;
						sStringify = false;
					}
					if(sStripNext)
					{
						sPos = sMergeStart;
					}
					expr = (sPos == 0 ? "" : expr.substr(0, sPos)) + arg + expr.substr(delta);
					slfsSet = false;
					l = expr.length();
					i = sPos + arg.length() - 1;
				}
				sPos = ~0;
				//tok_prev = tok;
				tok = "";
			}
		}
	}
	return(expr);
}

String CShaderPreprocessor::checkDefines(const String &_expr)
{
	String expr = _expr;
	UINT deflen = (UINT)strlen("defined");
	bool sDFound = false;
	bool sPFound = false;
	String key;
	UINT sPos = ~0;
	for(UINT i = 0, l = expr.length(); i < l; ++i)
	{
		if(expr[i] == 'd' && i + deflen - 1 < l && expr.substr(i, deflen) == "defined")
		{
			sPos = i;
			i += deflen - 1;
			sDFound = true;
			continue;
		}
		if(sDFound)
		{
			if(isspace(0xFF & expr[i]))
			{
				if(key.length() != 0)
				{
					//make replacement
					expr = (sPos == 0 ? "" : expr.substr(0, sPos)) + (int)isDefined(key) + expr.substr(i + 1);
				}
			}
			else
			{
				if(expr[i] == '(')
				{
					sDFound = true;
				}
				else if(expr[i] == ')')
				{
					expr = (sPos == 0 ? "" : expr.substr(0, sPos)) + (int)isDefined(key) + expr.substr(sDFound ? i + 1 : i);
					//make replacement
				}
				else
				{
					key += expr[i];
				}
			}
		}
	}
	return(expr);
}

String CShaderPreprocessor::getInclude(const String &name, const char *szLocalPath, String *outPath, bool useLocalPath)
{
	if(m_mIncludes.KeyExists(name))
	{
		_include * inc = &m_mIncludes[name];
		if(outPath)
		{
			*outPath = inc->sPath;
		}
		inc->isUsed = true;
		return(inc->sText);
	}
	String path;
	IFile *pFile = NULL;
	if(useLocalPath)
	{
		char *szDir = (char*)alloca(sizeof(char) * (strlen(szLocalPath) + 1));
		strcpy(szDir, szLocalPath);
		dirname(szDir);
		path = (String(szDir) + '/' + name);
		if((pFile = m_pFileSystem->openFile(path.c_str())))
		{
			int iSize = (int)pFile->getSize();
			String s;
			s.reserve(iSize + 2);
			pFile->readBin((void*)(s.c_str()), iSize);
			s[iSize] = '\n';
			s[iSize + 1] = 0;
			m_mIncludes[name] = {s, path, true};
			mem_release(pFile);
			if(outPath)
			{
				*outPath = path;
			}
			return(s);
		}
	}
	else
	{
		for(int i = 0, l = m_vIncludes.size(); i < l; ++i)
		{
			path = (m_vIncludes[i].sPath + '/' + name);
			if((pFile = m_pFileSystem->openFile(path.c_str())))
			{
				int iSize = (int)pFile->getSize();
				String s;
				s.reserve(iSize + 2);
				pFile->readBin((void*)(s.c_str()), iSize);
				s[iSize] = '\n';
				s[iSize + 1] = 0;
				m_mIncludes[name] = {s, path, true};
				mem_release(pFile);
				if(outPath)
				{
					*outPath = path;
				}
				return(s);
			}
		}
	}
	m_sError += String("Unable to include \"") + name + "\" file\n    Included from file: " + szLocalPath + "\n";
	return("");
};

const char* CShaderPreprocessor::getError()
{
	return(m_sError.c_str());
}
bool CShaderPreprocessor::isError()
{
	return(m_sError.length() != 0);
}

static bool tokcmp(const char *pHaystack, const char *pNeedle)
{
	char * pStr;
	bool b;
	int len = (int)strlen(pNeedle);

	pStr = (char*)malloc(sizeof(char) * (len + 1));

	memcpy(pStr, pHaystack, sizeof(char) * len);
	pStr[len] = 0;
	b = strcasecmp(pStr, pNeedle) == 0;
	free(pStr);
	return(b);
}

static void InsertToken(citem_t **ppFirst, citem_t **ppLast, token_t tok)
{
	if(!*ppFirst)
	{
		*ppFirst = (citem_t*)malloc(sizeof(citem_t));
		(*ppFirst)->tok = tok;
		*ppLast = *ppFirst;
		(*ppFirst)->pNext = NULL;
		(*ppFirst)->pPrev = NULL;
	}
	else
	{
		(*ppLast)->pNext = (citem_t*)malloc(sizeof(citem_t));
		(*ppLast)->pNext->tok = tok;
		(*ppLast)->pNext->pNext = NULL;
		(*ppLast)->pNext->pPrev = NULL;
		(*ppLast)->pNext->pPrev = (*ppLast);
		*ppLast = (*ppLast)->pNext;
	}
}

static void InsertTokenI(citem_t **ppFirst, citem_t **ppLast, token_t tok, int val)
{
	InsertToken(ppFirst, ppLast, tok);
	(*ppLast)->val.i = val;
}

static void InsertTokenF(citem_t **ppFirst, citem_t **ppLast, token_t tok, float val)
{
	InsertToken(ppFirst, ppLast, tok);
	(*ppLast)->val.f = val;
}

static citem_t* Calculate(citem_t *pStart)
{
	citem_t * pCur;
	citem_t * tmp, *tmp2, *tmp3, *pSP = NULL;
	int iBrLvl = -1;
	float result = 0.0f;
	token_t tok = TOK_NOP;
	int priority = -3;

	// DebugPrint(pStart);

	while(priority <= 2)
	{
		pCur = pStart;
		if(pCur->tok == TOK_LBRACKET)
		{
			iBrLvl = pCur->val.i;
		}
		while(pCur)
		{
			if(pCur->tok == TOK_RBRACKET && pCur->val.i == iBrLvl)
			{
				if(priority == 2)
				{
					tmp = pStart->pNext;
					pStart->val.f = result;
					pStart->tok = TOK_OPERAND;
					pStart->pNext = pCur->pNext;
					if(pCur->pNext)
					{
						pCur->pNext->pPrev = pStart;
					}
					pCur->pNext = NULL;
					tmp2 = tmp;
					while(tmp)
					{
						tmp3 = tmp->pNext;
						free(tmp);
						tmp = tmp3;
					}
					//free(tmp2);
					//stop calc, insert result
					return(pStart);
				}
				break;
			}
			if(pCur->tok == TOK_LBRACKET && pCur->val.i != iBrLvl)
			{
				pCur = Calculate(pCur);
			}

			if(pCur->tok == TOK_OPERAND)
			{
				if(tok == TOK_NOP)
				{
					result = pCur->val.f;
				}
				else
				{
					switch(priority)
					{
					case -3:
						if(tok == TOK_OP_NOT)
						{
							result = pCur->val.f == 0.0f ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						break;

					case -2:
						if(tok == TOK_OP_MUL)
						{
							result *= pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_DIV)
						{
							result /= pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						break;

					case -1:
						if(tok == TOK_OP_ADD)
						{
							result += pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_SUB)
						{
							result -= pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						break;

					case 0:
						if(tok == TOK_OP_GREATER)
						{
							result = (result > pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_GREATER_EQUAL)
						{
							result = (result >= pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_LESSER_EQUAL)
						{
							result = (result <= pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_LESSER)
						{
							result = (result > pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_EQUAL)
						{
							result = (result == pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						if(tok == TOK_OP_NOT_EQUAL)
						{
							result = (result != pCur->val.f) ? 1.0f : 0.0f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						break;

					case 1:
						if(tok == TOK_OP_AND)
						{
							result *= pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}

						break;

					case 2:
						if(tok == TOK_OP_OR)
						{
							result += pCur->val.f;
							if(pSP == NULL)
							{
								pSP = pCur->pPrev;
							}
						}
						break;
					}

					tok = TOK_NOP;
					if(pSP)
					{
						if(!pSP->pPrev)
						{
							pStart->tok = TOK_OPERAND;
							pStart->val.f = result;
							pStart->pNext = pCur->pNext;
							if(pCur->pNext)
							{
								pCur->pNext->pPrev = pStart;
							}
							pCur = pStart;
						}
						else
						{
							pSP->pPrev->pNext = pCur->pNext;
							if(pCur->pNext)
							{
								pCur->pNext->pPrev = pSP->pPrev;
							}
							pSP->pPrev->tok = TOK_OPERAND;
							pSP->pPrev->val.f = result;
						}
						pSP = NULL;
					}

				}
			}

			if(pCur->tok > TOK_OPS_START && pCur->tok < TOK_OPS_END)
			{
				switch(priority)
				{
				case -3:
					if(pCur->tok > TOK_PR_M3_S && pCur->tok < TOK_PR_M3_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;
				case -2:
					if(pCur->tok > TOK_PR_M2_S && pCur->tok < TOK_PR_M2_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;
				case -1:
					if(pCur->tok > TOK_PR_M1_S && pCur->tok < TOK_PR_M1_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;

				case 0:
					if(pCur->tok > TOK_PR_0_S && pCur->tok < TOK_PR_0_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;

				case 1:
					if(pCur->tok > TOK_PR_1_S && pCur->tok < TOK_PR_1_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;

				case 2:
					if(pCur->tok > TOK_PR_2_S && pCur->tok < TOK_PR_2_E)
					{
						tok = pCur->tok;
					}
					else
					{
						tok = TOK_NOP;
					}
					break;
				}


				//tok = pCur->tok;
			}
			pCur = pCur->pNext;
			// DebugPrint(pStart);
		}

		priority++;
	}
	tmp = pStart->pPrev;
	InsertTokenF(&tmp, &tmp, TOK_OPERAND, result);
	if(pCur)
	{
		tmp->pNext = pCur->pNext;
		if(pCur->pNext)
		{
			pCur->pNext->pPrev = tmp;
		}
		pCur->pNext = NULL;
	}
	tmp2 = pStart;
	while(tmp2)
	{
		pCur = tmp2;
		tmp2 = tmp2->pNext;
		free(pCur);
	}

	return(tmp);
}

String CShaderPreprocessor::stringify(const String &expr)
{
	return(String("\"") + expr + "\"");
}

void CShaderPreprocessor::reset()
{
	for(AssotiativeArray<String, _define>::Iterator i = m_mDefs.begin(); i; ++i)
	{
		i.second->isUndef = true;
	}

	for(AssotiativeArray<String, _include>::Iterator i = m_mIncludes.begin(); i; i++)
	{
		i.second->isUsed = false;
	}

	m_sError = "";
}

void CShaderPreprocessor::clearIncludeCache()
{
	m_mIncludes.clear();
}

UINT CShaderPreprocessor::getIncludesCount()
{
	UINT u = 0;
	for(AssotiativeArray<String, _include>::Iterator i = m_mIncludes.begin(); i; ++i)
	{
		if(i.second->isUsed)
		{
			++u;
		}
	}
	return(u);
}
void CShaderPreprocessor::getIncludes(const char **pszIncludes)
{
	UINT u = 0;
	for(AssotiativeArray<String, _include>::Iterator i = m_mIncludes.begin(); i; ++i)
	{
		if(i.second->isUsed)
		{
			pszIncludes[u] = i.second->sPath.c_str();
			++u;
		}
	}
}
