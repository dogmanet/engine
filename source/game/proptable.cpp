
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include <cstdarg>
#include <memory>

#include "proptable.h"

#include "BaseEntity.h"

prop_editor_t _GetEditorCombobox(int start, ...)
{
	prop_editor_t out;
	out.type = PDE_COMBOBOX;
	editor_kv kvs[ED_COMBO_MAXELS];

	va_list va;
	va_start(va, start);

	const char * el;
	int i = 0;
	while(i < ED_COMBO_MAXELS)
	{
		if(!(el = va_arg(va, const char *)))
		{
			break;
		}
		kvs[i].key = el;
		if(!(el = va_arg(va, const char *)))
		{
			break;
		}
		kvs[i].value = el;
		++i;
	}
	kvs[i].value = kvs[i].key = 0;
	++i;

	va_end(va);

	out.pData = new editor_kv[i];
	memcpy(out.pData, kvs, sizeof(editor_kv)* i);
	return(out);
}

prop_editor_t _GetEditorFilefield(int start, ...)
{
	prop_editor_t out;
	out.type = PDE_FILEFIELD;
	editor_kv kvs[ED_COMBO_MAXELS];

	va_list va;
	va_start(va, start);

	const char * el;
	int i = 0;
	while(i < ED_COMBO_MAXELS)
	{
		if(!(el = va_arg(va, const char *)))
		{
			break;
		}
		kvs[i].key = el;
		if(!(el = va_arg(va, const char *)))
		{
			break;
		}
		kvs[i].value = el;
		++i;
	}
	kvs[i].value = kvs[i].key = 0;
	++i;

	va_end(va);

	out.pData = new editor_kv[i];
	memcpy(out.pData, kvs, sizeof(editor_kv)* i);
	return(out);
}

const char* GetEmptyString()
{
	static const char * str = "";
	return(str);
}

void output_t::fire(CBaseEntity *pInflictor, CBaseEntity *pActivator, inputdata_t *pInputData)
{
	inputdata_t data = {0};
	data.pActivator = pActivator;
	data.pInflictor = pInflictor;
	for(UINT i = 0, l = aOutputs.size(); i < l; ++i)
	{
		named_output_t &out = aOutputs[i];
		data.type = PDF_NONE;
		if(out.fDelay == 0.0f && !out.useRandomDelay)
		{
			for(int j = 0, jl = out.aOutputs.size(); j < jl; ++j)
			{
				data.type = out.aOutputs[j].data.type;
				
				if(data.type == PDF_STRING)
				{
					data.parameter.str = NULL;
				}

				if(out.aOutputs[j].useOverrideData)
				{
					data.setParameter(out.aOutputs[j].data);
				}
				else if(pInputData)
				{
					data.setParameter(*pInputData);
				}

				(out.aOutputs[j].pTarget->*(out.aOutputs[j].fnInput))(&data);

				if(data.type == PDF_STRING && data.parameter.str != GetEmptyString())
				{
					delete[] data.parameter.str;
				}
			}
		}
		else
		{
			if(pInputData)
			{
				data.type = pInputData->type;
				data.setParameter(*pInputData);
			}
			pActivator->getManager()->setOutputTimeout(&out, &data);
		}
	}
}

void named_output_t::init(CBaseEntity *pThisEntity)
{
	listTargets.init(pThisEntity);
	listTargets.setEntityName(szTargetName);
}

void named_output_t::onEntityAdded(CBaseEntity *pEnt)
{
	propdata_t * pField = pEnt->getField(szTargetInput);
	if(!pField || !(pField->flags & PDFF_INPUT))
	{
		printf(COLOR_CYAN "Class '%s' has no input '%s', obj '%s'\n" COLOR_RESET, pEnt->getClassName(), szTargetInput, szTargetName);
		return;
	}

	input_t &out = aOutputs[aOutputs.size()];

	out.fnInput = pField->fnInput;
	out.pTarget = pEnt;
	memset(&out.data, 0, sizeof(out.data));
	out.data.type = pField->type;
	if((out.useOverrideData = szTargetData != NULL))
	{
		float3_t f3;
		float4_t f4;
		SMQuaternion q;
		int d;
		UINT u;
		float f;
		const char *value = szTargetData;
		bool bParsed = false;
		switch(pField->type)
		{
		case PDF_NONE:
			bParsed = true;
			break;
		case PDF_INT:
			if(1 == sscanf(value, "%d", &d))
			{
				out.data.parameter.i = d;
				bParsed = true;
			}
			break;
		case PDF_UINT:
			if(1 == sscanf(value, "%u", &u))
			{
				out.data.parameter.u = u;
				bParsed = true;
			}
			break;
		case PDF_FLOAT:
			if(1 == sscanf(value, "%f", &f))
			{
				out.data.parameter.f = f;
				bParsed = true;
			}
			break;
		case PDF_VECTOR:
			if(3 == sscanf(value, "%f %f %f", &f3.x, &f3.y, &f3.z))
			{
				out.data.v3Parameter = f3;
				bParsed = true;
			}
			break;
		case PDF_VECTOR4:
			{
				int iPrm = sscanf(value, "%f %f %f %f", &f4.x, &f4.y, &f4.z, &f4.w);
				if(iPrm > 2)
				{
					if(iPrm == 3)
					{
						f4.w = 1.0f;
					}
					out.data.v4Parameter = f4;
					bParsed = true;
				}
			}
			break;
		case PDF_BOOL:
			if(1 == sscanf(value, "%d", &d))
			{
				out.data.parameter.b = d != 0;
				bParsed = true;
			}
			break;
		case PDF_STRING:
			_setStrVal(&out.data.parameter.str, value);
			bParsed = true;
			break;
		}

		if(!bParsed)
		{
			printf(COLOR_CYAN "Cannot parse input parameter '%s', class '%s', input '%s', obj '%s'\n" COLOR_RESET, value, pEnt->getClassName(), szTargetInput, szTargetName);
			aOutputs.erase(aOutputs.size() - 1);
		}
	}
}
void named_output_t::onEntityRemoved(CBaseEntity *pEnt)
{
	int idx = aOutputs.indexOf(pEnt, [](const input_t &a, CBaseEntity *b){
		return(a.pTarget == b);
	});
	assert(idx >= 0);
	if(idx >= 0)
	{
		if(aOutputs[idx].useOverrideData && aOutputs[idx].data.type == PDF_STRING)
		{
			const char *estr = GetEmptyString();
			if(estr != aOutputs[idx].data.parameter.str)
			{
				mem_delete_a(aOutputs[idx].data.parameter.str);
			}
		}
		aOutputs[idx] = aOutputs[aOutputs.size() - 1];
		aOutputs.erase(aOutputs.size() - 1);
	}
}

void _setStrVal(const char **to, const char *value)
{
	char * str = (char*)*to;
	if(str && !fstrcmp(str, value))
	{
		return;
	}
	if(!value[0])
	{
		if(str && str != GetEmptyString())
		{
			delete[] str;
		}
		*to = GetEmptyString();
	}
	else
	{
		size_t len = strlen(value);
		if(!str || strlen(str) < len)
		{
			if(str && str != GetEmptyString())
			{
				delete[] str;
			}
			str = new char[len + 1];
		}
		memcpy(str, value, sizeof(char)* (len + 1));
		*to = str;
	}
}

void inputdata_t::setParameter(const inputdata_t &other)
{
	if(type == other.type)
	{
		if(type == PDF_VECTOR)
		{
			v3Parameter = other.v3Parameter;
		}
		else if(type == PDF_VECTOR4)
		{
			v4Parameter = other.v4Parameter;
		}
		else if(type == PDF_STRING)
		{
			_setStrVal(&parameter.str, other.parameter.str);
		}
		else
		{
			parameter = other.parameter;
		}
		return;
	}

	switch(type)
	{
	case PDF_BOOL:
		switch(other.type)
		{
		case PDF_INT:
			parameter.b = other.parameter.i != 0;
			break;
		case PDF_UINT:
			parameter.b = other.parameter.u != 0;
			break;
		case PDF_FLOAT:
			parameter.b = other.parameter.f != 0.0f;
			break;
		case PDF_VECTOR:
			parameter.b = SMVector3Length2(other.v3Parameter) > 0.0f;
			break;
		case PDF_VECTOR4:
			parameter.b = SMVector4Length(other.v4Parameter) > 0.0f;
			break;
		case PDF_STRING:
			parameter.b = other.parameter.str
				&& other.parameter.str[0]
				&& fstrcmp(other.parameter.str, "false")
				&& fstrcmp(other.parameter.str, "0")
				&& fstrcmp(other.parameter.str, "null");
			break;
		}
		break;

	case PDF_INT:
		switch(other.type)
		{
		case PDF_BOOL:
			parameter.i = other.parameter.b ? 1 : 0;
			break;
		case PDF_FLOAT:
			parameter.i = (int)other.parameter.f;
			break;
		case PDF_UINT:
			parameter.i = (int)other.parameter.u;
			break;
		case PDF_VECTOR:
			parameter.i = (int)other.v3Parameter.x;
			break;
		case PDF_VECTOR4:
			parameter.i = (int)other.v4Parameter.x;
			break;
		case PDF_STRING:
			if(!sscanf(other.parameter.str, "%d", &parameter.i))
			{
				parameter.i = 0;
			}
			break;
		}
		break;

	case PDF_UINT:
		switch(other.type)
		{
		case PDF_BOOL:
			parameter.u = other.parameter.b ? 1 : 0;
			break;
		case PDF_FLOAT:
			parameter.u = (UINT)other.parameter.f;
			break;
		case PDF_INT:
			parameter.u = (UINT)other.parameter.i;
			break;
		case PDF_VECTOR:
			parameter.u = (UINT)other.v3Parameter.x;
			break;
		case PDF_VECTOR4:
			parameter.u = (UINT)other.v4Parameter.x;
			break;
		case PDF_STRING:
			if(!sscanf(other.parameter.str, "%u", &parameter.u))
			{
				parameter.u = 0;
			}
			break;
		}
		break;

	case PDF_FLOAT:
		switch(other.type)
		{
		case PDF_BOOL:
			parameter.f = other.parameter.b ? 1.0f : 0.0f;
			break;
		case PDF_INT:
			parameter.f = (float)other.parameter.i;
			break;
		case PDF_UINT:
			parameter.f = (float)other.parameter.u;
			break;
		case PDF_VECTOR:
			parameter.f = other.v3Parameter.x;
			break;
		case PDF_VECTOR4:
			parameter.f = other.v4Parameter.x;
			break;
		case PDF_STRING:
			if(!sscanf(other.parameter.str, "%f", &parameter.f))
			{
				parameter.f = 0.0f;
			}
			break;
		}
		break;

	case PDF_VECTOR:
		switch(other.type)
		{
		case PDF_BOOL:
			v3Parameter = float3(other.parameter.b ? 1.0f : 0.0f);
			break;
		case PDF_INT:
			v3Parameter = float3((float)other.parameter.i);
			break;
		case PDF_UINT:
			v3Parameter = float3((float)other.parameter.u);
			break;
		case PDF_FLOAT:
			v3Parameter = float3(other.parameter.f);
			break;
		case PDF_VECTOR4:
			v3Parameter = float3(other.v4Parameter);
			break;
		case PDF_STRING:
			if(!sscanf(other.parameter.str, "%f %f %f", &v3Parameter.x, &v3Parameter.y, &v3Parameter.z))
			{
				v3Parameter = float3(0.0f);
			}
			break;
		}
		break;

	case PDF_VECTOR4:
		switch(other.type)
		{
		case PDF_BOOL:
			v4Parameter = float4(other.parameter.b ? 1.0f : 0.0f);
			break;
		case PDF_INT:
			v4Parameter = float4((float)other.parameter.i);
			break;
		case PDF_UINT:
			v4Parameter = float4((float)other.parameter.u);
			break;
		case PDF_FLOAT:
			v4Parameter = float4(other.parameter.f);
			break;
		case PDF_VECTOR:
			v4Parameter = float4(other.v3Parameter);
			break;
		case PDF_STRING:
			if(!sscanf(other.parameter.str, "%f %f %f %f", &v4Parameter.x, &v4Parameter.y, &v4Parameter.z, &v4Parameter.w))
			{
				v4Parameter = float4(0.0f);
			}
			break;
		}
		break;

	case PDF_STRING:
		char tmp[64];

		switch(other.type)
		{
		case PDF_BOOL:
			sprintf(tmp, "%d", other.parameter.b ? 1 : 0);
			break;
		case PDF_INT:
			sprintf(tmp, "%d", other.parameter.i);
			break;
		case PDF_UINT:
			sprintf(tmp, "%u", other.parameter.u);
			break;
		case PDF_FLOAT:
			sprintf(tmp, "%f", other.parameter.f);
			break;
		case PDF_VECTOR:
			sprintf(tmp, "%f %f %f", other.v3Parameter.x, other.v3Parameter.y, other.v3Parameter.z);
			break;
		case PDF_VECTOR4:
			sprintf(tmp, "%f %f %f %f", other.v4Parameter.x, other.v4Parameter.y, other.v4Parameter.z, other.v4Parameter.w);
			break;
		}

		_setStrVal(&parameter.str, tmp);
		break;
	}
}
