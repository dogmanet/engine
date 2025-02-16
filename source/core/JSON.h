#ifndef __JSON_H
#define __JSON_H

#include <xcommon/IXJSON.h>
#include <common/string.h>

template<typename T>
class CJSONItem: public IXUnknownImplementation<T>
{
public:
	XJSON_ITEM_TYPE XMETHODCALLTYPE getType() const override
	{
		return(m_type);
	}
	
	IXJSONArray* XMETHODCALLTYPE asArray() override
	{
		return(NULL);
	}
	const IXJSONArray* XMETHODCALLTYPE asArray() const override
	{
		return(NULL);
	}
	
	IXJSONObject* XMETHODCALLTYPE asObject() override
	{
		return(NULL);
	}
	const IXJSONObject* XMETHODCALLTYPE asObject() const override
	{
		return(NULL);
	}
	
	bool XMETHODCALLTYPE isNull() const override
	{
		return(m_type == XJI_NULL);
	}
	bool XMETHODCALLTYPE asBool() const override
	{
		switch(m_type)
		{
		case XJI_NULL:
			return(false);
		case XJI_BOOL:
		case XJI_NUMBER:
			return(m_i64Val != 0);
		case XJI_STRING:
			return(!strcmpi(m_sVal.c_str(), "yes") || !strcmpi(m_sVal.c_str(), "on") || !strcmpi(m_sVal.c_str(), "true"));
		case XJI_OBJECT:
			return(asObject()->size() != 0);
		case XJI_ARRAY:
			return(asArray()->size() != 0);
		}
		return(false);
	}
	bool XMETHODCALLTYPE getFloat(float *pOut) const override
	{
		switch(m_type)
		{
		case XJI_NUMBER:
			*pOut = m_i64Val + m_fVal;
			return(true);
		case XJI_STRING:
			//! FIXME add some checks!
			*pOut = m_sVal.toFloat();
			return(true);
		
		case XJI_NULL:
		case XJI_BOOL:
		case XJI_OBJECT:
		case XJI_ARRAY:
			break;
		}
		return(false);
	}
	bool XMETHODCALLTYPE getInt64(int64_t *pOut) const override
	{
		switch(m_type)
		{
		case XJI_NUMBER:
			*pOut = m_i64Val;
			return(true);
		case XJI_STRING:
			//! FIXME add some checks!
			//! FIXME fix type!
			*pOut = m_sVal.toInt64();
			return(true);

		case XJI_NULL:
		case XJI_BOOL:
		case XJI_OBJECT:
		case XJI_ARRAY:
			break;
		}
		return(false);
	}
	const char* XMETHODCALLTYPE getString() const override
	{
		switch(m_type)
		{
		case XJI_NUMBER:
			m_sVal = m_i64Val + m_fVal;
			break;
		case XJI_STRING:
			break;

		case XJI_BOOL:
			return(m_i64Val ? "true" : "false");

		case XJI_NULL:
		case XJI_OBJECT:
		case XJI_ARRAY:
			return(NULL);
		}
		return(m_sVal.c_str());
	}

	bool load(const char **str)
	{
		while(**str && isspace((byte)**str))
		{
			++*str;
		}
		
		bool isNull = false;

		switch(**str)
		{
		case L'"':
			if(!loadString(str))
			{
				return(false);
			}
			break;
		case L'{':
			if(!loadObj(str))
			{
				return(false);
			}
			break;
		case L'[':
			if(!loadArr(str))
			{
				return(false);
			}
			break;
		case L'-':
		case L'0':
		case L'1':
		case L'2':
		case L'3':
		case L'4':
		case L'5':
		case L'6':
		case L'7':
		case L'8':
		case L'9':
			if(!loadNum(str))
			{
				return(false);
			}
			break;
		case L't':
		case L'f':
			if(!loadBool(str))
			{
				return(false);
			}
			break;
		case L'n':
			if(!loadNull(str))
			{
				return(false);
			}
			isNull = true;
			break;
		default:
			return(false);
		}

		while(**str && isspace((byte)**str))
		{
			++*str;
		}

		if(!isNull && m_type == XJI_NULL)
		{
			// nothing was parsed!
			return(false);
		}

		return(true);
	}

private:
	bool loadString(const char **prm)
	{
		m_type = XJI_STRING;
		Array<char> tmp;
		++*prm;
		while(**prm && **prm != '"')
		{
			if(**prm == '\\')
			{
				++*prm;
				switch(**prm)
				{
				case L'"':
				case L'\\':
				case L'/':
					tmp.push_back(**prm);
					break;
				case L'b':
					tmp.push_back('\b');
					break;
				case L'f':
					tmp.push_back('\f');
					break;
				case L'n':
					tmp.push_back('\n');
					break;
				case L'r':
					tmp.push_back('\r');
					break;
				case L't':
					tmp.push_back('\t');
					break;
				case L'u':
				{
					UINT code = 0;
					for(UINT i = 0; i < 4; ++i)
					{
						code *= 16;
						++*prm;
						if(**prm >= '0' && **prm <= '9')
						{
							code += **prm - '0';
						}
						else if(**prm >= 'a' && **prm <= 'f')
						{
							code += **prm - 'a' + 10;
						}
						else if(**prm >= 'A' && **prm <= 'F')
						{
							code += **prm - 'A' + 10;
						}
						else
						{
							return(false);
						}
					}
					writeChar(code, &tmp);
				}
				break;
				default:
					return(false);
				}
			}
			else
			{
				tmp.push_back(**prm);
			}
			++*prm;
		}
		if(**prm != '"')
		{
			return(false);
		}
		++*prm;
		tmp.push_back(0);
		m_sVal = tmp;
		return(true);
	}
	bool loadNum(const char **prm)
	{
		m_type = XJI_NUMBER;
		m_i64Val = 0;
		m_fVal = 0;

		bool bNeg = false;

		if(**prm == '-')
		{
			bNeg = true;
			++*prm;
		}

		while(**prm >= '0' && **prm <= '9')
		{
			m_i64Val *= 10;
			m_i64Val += **prm - '0';
			++*prm;
		}
		
		if(**prm == '.')
		{
			++*prm;
			int digc = 0;
			while(**prm >= '0' && **prm <= '9')
			{
				m_fVal *= 10.0f;
				m_fVal += (float)(**prm - '0');
				++*prm;
				++digc;
			}
			while(digc--)
			{
				m_fVal /= 10.0f;
			}
		}
		if(**prm == 'e' || **prm == 'E')
		{
			++*prm;
			int exp = 0;
			bool bExpNeg = false;
			if(**prm == L'+')
			{
				++*prm;
			}
			if(**prm == L'-')
			{
				bExpNeg = true;
				++*prm;
			}
			bool isRun = false;
			while(**prm >= L'0' && **prm <= L'9')
			{
				exp *= 10;
				exp += **prm - L'0';
				++*prm;
				isRun = true;
			}
			if(!isRun)
			{
				return(false);
			}
			m_fVal += (float)m_i64Val;
			while(exp--)
			{
				if(bExpNeg)
				{
					m_fVal /= 10.0f;
				}
				else
				{
					m_fVal *= 10.0f;
				}
			}

			m_i64Val = (int64_t)m_fVal;
			m_fVal -= (float)m_i64Val;
		}

		if(bNeg)
		{
			m_i64Val *= -1;
			m_fVal *= -1.0f;
		}

		return(true);
	}
	bool loadBool(const char **str)
	{
		m_type = XJI_BOOL;
		if(!memcmp(*str, "true", 4))
		{
			m_i64Val = 1;
			*str += 4;
			return(true);
		}
		if(!memcmp(*str, "false", 5))
		{
			m_i64Val = 0;
			*str += 5;
			return(true);
		}

		return(false);
	}
	bool loadNull(const char **str)
	{
		m_type = XJI_NULL;
		if(!memcmp(*str, "null", 4))
		{
			*str += 4;
			return(true);
		}

		return(false);
	}
	virtual bool loadObj(const char **str)
	{
		assert(!"Called loadObj on non-CJSONObject");
		return(false);
	}
	virtual bool loadArr(const char **str)
	{
		assert(!"Called loadArr on non-CJSONArray"); 
		return(false);
	}

	void writeChar(UINT c, Array<char> *pOut)
	{
		UINT codepoint = 0;
		short *in = (short*)&c;
		for(int i = 0; i < 2; ++i, ++in)
		{
			if(c >= 0xd800 && *in <= 0xdbff)
			{
				codepoint = ((*in - 0xd800) << 10) + 0x10000;
			}
			else
			{
				if(*in >= 0xdc00 && *in <= 0xdfff)
				{
					codepoint |= *in - 0xdc00;
				}
				else
				{
					codepoint = *in;
				}

				if(codepoint <= 0x7f)
				{
					pOut->push_back(codepoint);
					break;
				}
				else if(codepoint <= 0x7ff)
				{
					pOut->push_back(0xc0 | ((codepoint >> 6) & 0x1f));
					pOut->push_back(0x80 | (codepoint & 0x3f));
					break;
				}
				else if(codepoint <= 0xffff)
				{
					pOut->push_back(0xe0 | ((codepoint >> 12) & 0x0f));
					pOut->push_back(0x80 | ((codepoint >> 6) & 0x3f));
					pOut->push_back(0x80 | (codepoint & 0x3f));
					break;
				}
				else
				{
					pOut->push_back(0xf0 | ((codepoint >> 18) & 0x07));
					pOut->push_back(0x80 | ((codepoint >> 12) & 0x3f));
					pOut->push_back(0x80 | ((codepoint >> 6) & 0x3f));
					pOut->push_back(0x80 | (codepoint & 0x3f));
					break;
				}
			}
		}
	}

protected:
	XJSON_ITEM_TYPE m_type = XJI_NULL;

	int64_t m_i64Val = 0;
	float m_fVal = 0.0f;
	mutable String m_sVal;
};

class CJSONValue: public CJSONItem<IXJSONItem>
{
};

class CJSONArray: public CJSONItem<IXJSONArray>
{
public:
	~CJSONArray();

	UINT XMETHODCALLTYPE size() const override;
	IXJSONItem* XMETHODCALLTYPE at(UINT idx) const override;

	IXJSONArray* XMETHODCALLTYPE asArray() override
	{
		return(this);
	}
	const IXJSONArray* XMETHODCALLTYPE asArray() const override
	{
		return(this);
	}

private:
	bool loadArr(const char **str) override;

private:
	Array<IXJSONItem*> m_aItems;
};

class CJSONObject: public CJSONItem<IXJSONObject>
{
public:
	~CJSONObject();

	UINT XMETHODCALLTYPE size() const override;
	IXJSONItem* XMETHODCALLTYPE at(UINT idx) const override;
	
	const char* XMETHODCALLTYPE getKey(UINT idx) const override;
	
	IXJSONItem* XMETHODCALLTYPE getItem(const char *szKey) const override;

	IXJSONObject* XMETHODCALLTYPE asObject() override
	{
		return(this);
	}
	const IXJSONObject* XMETHODCALLTYPE asObject() const override
	{
		return(this);
	}

private:
	bool loadObj(const char **str) override;
	
	struct KeyValue
	{
		String sKey;
		IXJSONItem *pVal;
	};

	Array<KeyValue> m_aPairs;
};

class CJSON: public IXUnknownImplementation<IXJSON>
{
public:
	bool XMETHODCALLTYPE parse(const char *szString, IXJSONItem **ppOut, void *pReserved = NULL) const override;

	static bool Parse(const char **str, IXJSONItem **ppOut);
};

#endif
