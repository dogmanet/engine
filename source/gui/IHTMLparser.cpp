#include "GUIbase.h"

#include <cstdio>

#include "IHTMLparser.h"

#include "ITEXT.h"

namespace gui
{
	namespace dom
	{
		AssotiativeArray<StringW, wchar_t> IHTMLparser::m_mMetaSyms;
		void IHTMLparser::fillMetaTable()
		{
			m_mMetaSyms[L"nbsp"] = 160;
			m_mMetaSyms[L"iexcl"] = 161;
			m_mMetaSyms[L"cent"] = 162;
			m_mMetaSyms[L"pound"] = 163;
			m_mMetaSyms[L"curren"] = 164;
			m_mMetaSyms[L"yen"] = 165;
			m_mMetaSyms[L"brvbar"] = 166;
			m_mMetaSyms[L"sect"] = 167;
			m_mMetaSyms[L"uml"] = 168;
			m_mMetaSyms[L"copy"] = 169;
			m_mMetaSyms[L"ordf"] = 170;
			m_mMetaSyms[L"laquo"] = 171;
			m_mMetaSyms[L"not"] = 172;
			m_mMetaSyms[L"shy"] = 173;
			m_mMetaSyms[L"reg"] = 174;
			m_mMetaSyms[L"macr"] = 175;
			m_mMetaSyms[L"deg"] = 176;
			m_mMetaSyms[L"plusmn"] = 177;
			m_mMetaSyms[L"sup2"] = 178;
			m_mMetaSyms[L"sup3"] = 179;
			m_mMetaSyms[L"acute"] = 180;
			m_mMetaSyms[L"micro"] = 181;
			m_mMetaSyms[L"para"] = 182;
			m_mMetaSyms[L"middot"] = 183;
			m_mMetaSyms[L"cedil"] = 184;
			m_mMetaSyms[L"sup1"] = 185;
			m_mMetaSyms[L"ordm"] = 186;
			m_mMetaSyms[L"raquo"] = 187;
			m_mMetaSyms[L"frac14"] = 188;
			m_mMetaSyms[L"frac12"] = 189;
			m_mMetaSyms[L"frac34"] = 190;
			m_mMetaSyms[L"iquest"] = 191;
			m_mMetaSyms[L"agrave"] = 192;
			m_mMetaSyms[L"aacute"] = 193;
			m_mMetaSyms[L"acirc"] = 194;
			m_mMetaSyms[L"atilde"] = 195;
			m_mMetaSyms[L"auml"] = 196;
			m_mMetaSyms[L"aring"] = 197;
			m_mMetaSyms[L"aelig"] = 198;
			m_mMetaSyms[L"ccedil"] = 199;
			m_mMetaSyms[L"egrave"] = 200;
			m_mMetaSyms[L"eacute"] = 201;
			m_mMetaSyms[L"ecirc"] = 202;
			m_mMetaSyms[L"euml"] = 203;
			m_mMetaSyms[L"igrave"] = 204;
			m_mMetaSyms[L"iacute"] = 205;
			m_mMetaSyms[L"icirc"] = 206;
			m_mMetaSyms[L"iuml"] = 207;
			m_mMetaSyms[L"eth"] = 208;
			m_mMetaSyms[L"ntilde"] = 209;
			m_mMetaSyms[L"ograve"] = 210;
			m_mMetaSyms[L"oacute"] = 211;
			m_mMetaSyms[L"ocirc"] = 212;
			m_mMetaSyms[L"otilde"] = 213;
			m_mMetaSyms[L"ouml"] = 214;
			m_mMetaSyms[L"times"] = 215;
			m_mMetaSyms[L"oslash"] = 216;
			m_mMetaSyms[L"ugrave"] = 217;
			m_mMetaSyms[L"uacute"] = 218;
			m_mMetaSyms[L"ucirc"] = 219;
			m_mMetaSyms[L"uuml"] = 220;
			m_mMetaSyms[L"yacute"] = 221;
			m_mMetaSyms[L"thorn"] = 222;
			m_mMetaSyms[L"szlig"] = 223;
			m_mMetaSyms[L"agrave"] = 224;
			m_mMetaSyms[L"aacute"] = 225;
			m_mMetaSyms[L"acirc"] = 226;
			m_mMetaSyms[L"atilde"] = 227;
			m_mMetaSyms[L"auml"] = 228;
			m_mMetaSyms[L"aring"] = 229;
			m_mMetaSyms[L"aelig"] = 230;
			m_mMetaSyms[L"ccedil"] = 231;
			m_mMetaSyms[L"egrave"] = 232;
			m_mMetaSyms[L"eacute"] = 233;
			m_mMetaSyms[L"ecirc"] = 234;
			m_mMetaSyms[L"euml"] = 235;
			m_mMetaSyms[L"igrave"] = 236;
			m_mMetaSyms[L"iacute"] = 237;
			m_mMetaSyms[L"icirc"] = 238;
			m_mMetaSyms[L"iuml"] = 239;
			m_mMetaSyms[L"eth"] = 240;
			m_mMetaSyms[L"ntilde"] = 241;
			m_mMetaSyms[L"ograve"] = 242;
			m_mMetaSyms[L"oacute"] = 243;
			m_mMetaSyms[L"ocirc"] = 244;
			m_mMetaSyms[L"otilde"] = 245;
			m_mMetaSyms[L"ouml"] = 246;
			m_mMetaSyms[L"divide"] = 247;
			m_mMetaSyms[L"oslash"] = 248;
			m_mMetaSyms[L"ugrave"] = 249;
			m_mMetaSyms[L"uacute"] = 250;
			m_mMetaSyms[L"ucirc"] = 251;
			m_mMetaSyms[L"uuml"] = 252;
			m_mMetaSyms[L"yacute"] = 253;
			m_mMetaSyms[L"thorn"] = 254;
			m_mMetaSyms[L"yuml"] = 255;
			m_mMetaSyms[L"fnof"] = 402;
			m_mMetaSyms[L"alpha"] = 913;
			m_mMetaSyms[L"beta"] = 914;
			m_mMetaSyms[L"gamma"] = 915;
			m_mMetaSyms[L"delta"] = 916;
			m_mMetaSyms[L"epsilon"] = 917;
			m_mMetaSyms[L"zeta"] = 918;
			m_mMetaSyms[L"eta"] = 919;
			m_mMetaSyms[L"theta"] = 920;
			m_mMetaSyms[L"iota"] = 921;
			m_mMetaSyms[L"kappa"] = 922;
			m_mMetaSyms[L"lambda"] = 923;
			m_mMetaSyms[L"mu"] = 924;
			m_mMetaSyms[L"nu"] = 925;
			m_mMetaSyms[L"xi"] = 926;
			m_mMetaSyms[L"omicron"] = 927;
			m_mMetaSyms[L"pi"] = 928;
			m_mMetaSyms[L"rho"] = 929;
			m_mMetaSyms[L"sigma"] = 931;
			m_mMetaSyms[L"tau"] = 932;
			m_mMetaSyms[L"upsilon"] = 933;
			m_mMetaSyms[L"phi"] = 934;
			m_mMetaSyms[L"chi"] = 935;
			m_mMetaSyms[L"psi"] = 936;
			m_mMetaSyms[L"omega"] = 937;
			m_mMetaSyms[L"alpha"] = 945;
			m_mMetaSyms[L"beta"] = 946;
			m_mMetaSyms[L"gamma"] = 947;
			m_mMetaSyms[L"delta"] = 948;
			m_mMetaSyms[L"epsilon"] = 949;
			m_mMetaSyms[L"zeta"] = 950;
			m_mMetaSyms[L"eta"] = 951;
			m_mMetaSyms[L"theta"] = 952;
			m_mMetaSyms[L"iota"] = 953;
			m_mMetaSyms[L"kappa"] = 954;
			m_mMetaSyms[L"lambda"] = 955;
			m_mMetaSyms[L"mu"] = 956;
			m_mMetaSyms[L"nu"] = 957;
			m_mMetaSyms[L"xi"] = 958;
			m_mMetaSyms[L"omicron"] = 959;
			m_mMetaSyms[L"pi"] = 960;
			m_mMetaSyms[L"rho"] = 961;
			m_mMetaSyms[L"sigmaf"] = 962;
			m_mMetaSyms[L"sigma"] = 963;
			m_mMetaSyms[L"tau"] = 964;
			m_mMetaSyms[L"upsilon"] = 965;
			m_mMetaSyms[L"phi"] = 966;
			m_mMetaSyms[L"chi"] = 967;
			m_mMetaSyms[L"psi"] = 968;
			m_mMetaSyms[L"omega"] = 969;
			m_mMetaSyms[L"thetasym"] = 977;
			m_mMetaSyms[L"upsih"] = 978;
			m_mMetaSyms[L"piv"] = 982;
			m_mMetaSyms[L"bull"] = 8226;
			m_mMetaSyms[L"hellip"] = 8230;
			m_mMetaSyms[L"prime"] = 8242;
			m_mMetaSyms[L"Prime"] = 8243;
			m_mMetaSyms[L"oline"] = 8254;
			m_mMetaSyms[L"frasl"] = 8260;
			m_mMetaSyms[L"weierp"] = 8472;
			m_mMetaSyms[L"image"] = 8465;
			m_mMetaSyms[L"real"] = 8476;
			m_mMetaSyms[L"trade"] = 8482;
			m_mMetaSyms[L"alefsym"] = 8501;
			m_mMetaSyms[L"larr"] = 8592;
			m_mMetaSyms[L"uarr"] = 8593;
			m_mMetaSyms[L"rarr"] = 8594;
			m_mMetaSyms[L"darr"] = 8595;
			m_mMetaSyms[L"harr"] = 8596;
			m_mMetaSyms[L"crarr"] = 8629;
			m_mMetaSyms[L"larr"] = 8656;
			m_mMetaSyms[L"uarr"] = 8657;
			m_mMetaSyms[L"rarr"] = 8658;
			m_mMetaSyms[L"darr"] = 8659;
			m_mMetaSyms[L"harr"] = 8660;
			m_mMetaSyms[L"forall"] = 8704;
			m_mMetaSyms[L"part"] = 8706;
			m_mMetaSyms[L"exist"] = 8707;
			m_mMetaSyms[L"empty"] = 8709;
			m_mMetaSyms[L"nabla"] = 8711;
			m_mMetaSyms[L"isin"] = 8712;
			m_mMetaSyms[L"notin"] = 8713;
			m_mMetaSyms[L"ni"] = 8715;
			m_mMetaSyms[L"prod"] = 8719;
			m_mMetaSyms[L"sum"] = 8721;
			m_mMetaSyms[L"minus"] = 8722;
			m_mMetaSyms[L"lowast"] = 8727;
			m_mMetaSyms[L"radic"] = 8730;
			m_mMetaSyms[L"prop"] = 8733;
			m_mMetaSyms[L"infin"] = 8734;
			m_mMetaSyms[L"ang"] = 8736;
			m_mMetaSyms[L"and"] = 8743;
			m_mMetaSyms[L"or"] = 8744;
			m_mMetaSyms[L"cap"] = 8745;
			m_mMetaSyms[L"cup"] = 8746;
			m_mMetaSyms[L"int"] = 8747;
			m_mMetaSyms[L"there4"] = 8756;
			m_mMetaSyms[L"sim"] = 8764;
			m_mMetaSyms[L"cong"] = 8773;
			m_mMetaSyms[L"asymp"] = 8776;
			m_mMetaSyms[L"ne"] = 8800;
			m_mMetaSyms[L"equiv"] = 8801;
			m_mMetaSyms[L"le"] = 8804;
			m_mMetaSyms[L"ge"] = 8805;
			m_mMetaSyms[L"sub"] = 8834;
			m_mMetaSyms[L"sup"] = 8835;
			m_mMetaSyms[L"nsub"] = 8836;
			m_mMetaSyms[L"sube"] = 8838;
			m_mMetaSyms[L"supe"] = 8839;
			m_mMetaSyms[L"oplus"] = 8853;
			m_mMetaSyms[L"otimes"] = 8855;
			m_mMetaSyms[L"perp"] = 8869;
			m_mMetaSyms[L"sdot"] = 8901;
			m_mMetaSyms[L"lceil"] = 8968;
			m_mMetaSyms[L"rceil"] = 8969;
			m_mMetaSyms[L"lfloor"] = 8970;
			m_mMetaSyms[L"rfloor"] = 8971;
			m_mMetaSyms[L"lang"] = 9001;
			m_mMetaSyms[L"rang"] = 9002;
			m_mMetaSyms[L"loz"] = 9674;
			m_mMetaSyms[L"spades"] = 9824;
			m_mMetaSyms[L"clubs"] = 9827;
			m_mMetaSyms[L"hearts"] = 9829;
			m_mMetaSyms[L"diams"] = 9830;
			m_mMetaSyms[L"quot"] = 34;
			m_mMetaSyms[L"amp"] = 38;
			m_mMetaSyms[L"lt"] = 60;
			m_mMetaSyms[L"gt"] = 62;
			m_mMetaSyms[L"OElig"] = 338;
			m_mMetaSyms[L"oelig"] = 339;
			m_mMetaSyms[L"Scaron"] = 352;
			m_mMetaSyms[L"scaron"] = 353;
			m_mMetaSyms[L"yuml"] = 376;
			m_mMetaSyms[L"circ"] = 710;
			m_mMetaSyms[L"tilde"] = 732;
			m_mMetaSyms[L"ensp"] = 8194;
			m_mMetaSyms[L"emsp"] = 8195;
			m_mMetaSyms[L"thinsp"] = 8201;
			m_mMetaSyms[L"zwnj"] = 8204;
			m_mMetaSyms[L"zwj"] = 8205;
			m_mMetaSyms[L"lrm"] = 8206;
			m_mMetaSyms[L"rlm"] = 8207;
			m_mMetaSyms[L"ndash"] = 8211;
			m_mMetaSyms[L"mdash"] = 8212;
			m_mMetaSyms[L"lsquo"] = 8216;
			m_mMetaSyms[L"rsquo"] = 8217;
			m_mMetaSyms[L"sbquo"] = 8218;
			m_mMetaSyms[L"ldquo"] = 8220;
			m_mMetaSyms[L"rdquo"] = 8221;
			m_mMetaSyms[L"bdquo"] = 8222;
			m_mMetaSyms[L"dagger"] = 8224;
			m_mMetaSyms[L"Dagger"] = 8225;
			m_mMetaSyms[L"permil"] = 8240;
			m_mMetaSyms[L"lsaquo"] = 8249;
			m_mMetaSyms[L"rsaquo"] = 8250;
			m_mMetaSyms[L"euro"] = 8364;
		}

		IDOMnode * IHTMLparser::createFromFile(const wchar_t * file)
		{
			FILE * pF;
			_wfopen_s(&pF, file, L"rt, ccs=UTF-8");
			if(!pF)
			{
				printf("[Error]: Unable to open \"%s\" file for reading\n", String(StringW(file)).c_str());
				return(NULL);
			}

			fseek(pF, 0, SEEK_END);
			UINT fs = ftell(pF);
			fseek(pF, 0, SEEK_SET);
			wchar_t * html = new wchar_t[fs + 1];
			if(!html)
			{
				printf("[Error]: Unable to allocate memory\n");
				return(NULL);
			}
			html[fread(html, sizeof(wchar_t), fs, pF)] = 0;
			fclose(pF);
			IDOMnode * root = createFromString(html, NULL);
			mem_delete_a(html);
			return(root);
		}

		wchar_t IHTMLparser::parseMetaSym(const StringW & str, bool isNumeric)
		{
			wchar_t c = 0;
			if(isNumeric)
			{
				c = str.toInt();
			}
			else
			{
				if(IHTMLparser::m_mMetaSyms.Size() == 0)
				{
					IHTMLparser::fillMetaTable();
				}
				WCHAR * t = new WCHAR[str.length() + 1];
				memcpy(t, str.c_str(), sizeof(WCHAR) * (str.length() + 1));
				_wcslwr(t);
				if(IHTMLparser::m_mMetaSyms.KeyExists(t))
				{
					c = IHTMLparser::m_mMetaSyms[t];
				}
				mem_delete_a(t);
			}
			return(c);
		}

		struct state_s
		{
			//bool search_tag;
			//bool read_args;
			bool in_tag;
			bool read_tag;
			bool in_close_tag;
			bool in_tag_first;
			bool self_close_tag;

			bool read_attr_name;
			bool read_attr_value;
			bool read_attr_value_pre;

			bool read_metasym;
			bool read_metasym_first;
			bool read_metasym_num;

			bool in_comment;
		};

		IDOMnode * IHTMLparser::createFromString(const wchar_t * str, int * pNextSym)
		{
			if(!str)
			{
				return(NULL);
			}
			state_s state;
			//state = {0};
			memset(&state, 0, sizeof(state_s));

			StringW tagname;
			StringW textnode;

			StringW key;
			StringW val;
			StringW metasym;

			Stack<CDOMnode*> nodes;
			CDOMnode * root = NULL;
			CDOMnode * pCur = NULL;

			AssotiativeArray<StringW, StringW> attrs;

			UINT len = lstrlenW(str);
			bool bHandled;
			for(UINT i = 0; i < len; i++)
			{
				wchar_t c = str[i];
				bHandled = false;
				if(c == '>' && state.in_comment)
				{
					if(str[i - 1] == '-' && str[i - 2] == '-')
					{
						state.in_comment = false;
					}
				}
				else if(!state.in_comment)
				{
					switch(c)
					{
					case L'<':
						if(!state.in_tag)
						{
							state.in_tag = true;
							state.read_tag = true;
							state.in_tag_first = true;
							if(textnode.length())
							{
								bool bUse = false;
								for(UINT il = 0; il < textnode.length(); il++)
								{
									if(!iswspace(textnode[il]))
									{
										bUse = true;
										break;
									}
								}
								if(bUse)
								{
									CDOMnode * pNode = (CDOMnode*)CDOMnode::createNode(L"text");
									pNode->setDocument((CDOMdocument*)m_pDocument);
									((IDOMnodeText*)pNode)->setText(textnode);
									if(pCur)
									{
										pCur->appendChild(pNode);
									}
									else
									{
										if(pNextSym)
										{
											*pNextSym = i;
										}
										return(pNode);
									}
								}
								//							wprintf(L"TextNode: %s\n", textnode);
								textnode = L"";
							}
						}
						else
						{
							printf("[Error]: Unexpected symbol '&lt;' in pos %d\n", i);
						}
						bHandled = true;
						break;

					case L'>':
						if(state.in_tag || state.in_close_tag)
						{

							//if(state.read_tag)
							{
								state.read_tag = false;
								if(state.in_tag)
								{
									//if(!root)
									CDOMnode * tmp = pCur;
									if(pCur)
									{
										nodes.push(pCur);
									}
											{
												pCur = (CDOMnode*)CDOMnode::createNode(tagname.c_str());
												if(pCur)
												{
													if(!root)
													{
														root = pCur;
													}
													pCur->setDocument((CDOMdocument*)m_pDocument);
													if(tmp)
													{
														tmp->appendChild(pCur);
													}
												}
												else
												{
													printf("Unknown tag %S\n", tagname.c_str());
													if(!nodes.IsEmpty())
													{
														nodes.pop(&pCur);
													}
													else
													{
														if(pNextSym)
														{
															*pNextSym = i + 1;
														}
														return(root);
													}
												}
											}
											for(AssotiativeArray<StringW, StringW>::Iterator j = attrs.begin(); j; ++j)
											{
												pCur->setAttribute(*j.first, *j.second);
											}
											attrs.clear();
											//								wprintf(L"open tag %s\n", tagname);
											if(state.self_close_tag)
											{
												//									wprintf(L"close tag %s\n", tagname);
												if(!nodes.IsEmpty())
												{
													nodes.pop(&pCur);

												}
												else
												{
													if(pNextSym)
													{
														*pNextSym = i + 1;
													}
													return(root);
												}
											}
								}
								else
								{
									//								wprintf(L"close tag %s\n", tagname);
									if(!nodes.IsEmpty())
									{
										nodes.pop(&pCur);
									}
									else
									{
										if(pNextSym)
										{
											*pNextSym = i + 1;
										}
										return(root);
									}
								}
								tagname = L"";
							}
							state.in_tag = false;
							state.in_close_tag = false;
							state.self_close_tag = false;
						}
						else
						{
							printf("[Error]: Unexpected symbol '&gt;' in pos %d\n", i);
						}
						bHandled = true;
						break;
					case L'/':
						if(state.in_tag_first)
						{
							state.in_tag_first = false;
							state.in_tag = false;
							state.in_close_tag = true;
							bHandled = true;
						}
						if(state.in_tag && str[i + 1] == '>')
						{
							state.self_close_tag = true;
							bHandled = true;
						}
						break;

					case L'!':
						if(state.in_tag_first && str[i + 1] == '-' && str[i + 2] == '-')
						{
							state.in_tag_first = false;
							state.in_tag = false;
							state.in_close_tag = false;
							state.read_tag = false;

							state.in_comment = true;

							bHandled = true;
						}
						break;

					case L'&':
						if(!state.read_metasym)
						{
							state.read_metasym = true;
							state.read_metasym_first = true;
							bHandled = true;
						}
						break;

					case L'=':
						if(state.read_attr_name)
						{
							state.read_attr_name = false;
							state.read_attr_value_pre = true;
							bHandled = true;
						}
						break;


					case L'#':
						if(state.read_metasym_first)
						{
							state.read_metasym_num = true;
							bHandled = true;
						}
						break;

					case L'"':
						if(state.read_attr_value_pre)
						{
							state.read_attr_value = true;
							state.read_attr_value_pre = false;
							bHandled = true;
						}
						else if(state.read_attr_value)
						{
							state.read_attr_value = false;
							//						wprintf(L"Attr: %s=\"%s\"\n", key, val);
							const WCHAR * s = key.c_str();
							UINT p = 0;
							while(iswspace(s[p]))
							{
								p++;
							}
							key.remove(0, p);
							attrs[key] = val;
							key = L"";
							val = L"";
							bHandled = true;
						}
						break;

					case L';':
						if(state.read_metasym)
						{
							c = parseMetaSym(metasym, state.read_metasym_num);
							state.read_metasym = false;
							state.read_metasym_first = false;
							state.read_metasym_num = false;

							if(!c)
							{
								printf("[Error]: Unknown meta char \"%s\"\n", String(metasym).c_str());
								bHandled = true;
							}
							metasym = L"";
						}
						break;
					}
					if(!bHandled)
					{
						state.in_tag_first = false;
						state.read_metasym_first = false;
						if(state.read_tag)
						{
							if(iswspace(c))
							{
								if(tagname.length() == 0)
								{
									printf("[Error]: Expected tag name but found ' ' in pos %d\n", i);
								}
								else
								{
									state.read_tag = false;
								}
							}
							else
							{

								tagname += c;

							}
						}
						else
						{
							if(state.read_metasym)
							{
								metasym += c;
							}
							else if(state.in_tag)
							{
								if(!state.read_attr_name && !state.read_attr_value)
								{
									state.read_attr_name = true;
								}
								if(state.read_attr_name)
								{
									key += c;
								}
								if(state.read_attr_value)
								{
									val += c;
								}
							}
							else
							{
								textnode += c;
							}
						}
					}
				}
			}
			if(!root && textnode.length())
			{
				root = (CDOMnode*)CDOMnode::createNode(L"text");
				root->setDocument((CDOMdocument*)m_pDocument);
				((IDOMnodeText*)root)->setText(textnode);
			}
			if(pNextSym)
			{
				*pNextSym = len;
			}
			return(root);
		}
	};
};