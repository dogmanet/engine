
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __SHADERS_H
#define __SHADERS_H

#include <xcommon/render/IXRender.h>
#include <xcommon/IXCore.h>
#include <common/string_utils.h>

/*! \name Ограничения на размеры
@{*/

//! максимальный размер имени директории (до _)
#define SXGC_SHADER_MAX_SIZE_DIR 64

//! максимальный размер имени с расширением (после _)
#define SXGC_SHADER_MAX_SIZE_NAME 64

//! общий максимальный  размер имени текстуры с расширением
#define SXGC_SHADER_MAX_SIZE_DIRNAME (SXGC_SHADER_MAX_SIZE_DIR + SXGC_SHADER_MAX_SIZE_NAME)

//! максимальный размер пути до файла шейдера (без имени файла)
#define SXGC_SHADER_MAX_SIZE_STDPATH 256

//! максимальный размер полного пути до шейдера (включая имя шейдера)
#define SXGC_SHADER_MAX_SIZE_FULLPATH (SXGC_SHADER_MAX_SIZE_STDPATH + SXGC_SHADER_MAX_SIZE_DIRNAME)


//! максимальная длина имени переменной в шейдере
#define SXGC_SHADER_VAR_MAX_SIZE 64

//! максимальное количество переменных в шейдере
#define SXGC_SHADER_VAR_MAX_COUNT 64

//! количество макросов в массиве макросов
#define SXGC_SHADER_COUNT_MACRO 32

//!@}

//! базовый класс шейдера
struct CShader
{
	CShader()
	{ 
		m_szPath[0] = 0; 

		ZeroMemory(m_aMacros, sizeof(GXMacro)* SXGC_SHADER_COUNT_MACRO);
	}

	//! имя файла шейдера
	char m_szPath[SXGC_SHADER_MAX_SIZE_DIR];

	//! массив макросов (данные последнего макроса должны быть NULL)
	GXMacro m_aMacros[SXGC_SHADER_COUNT_MACRO];
};

template<class T>
struct CShaderImpl: public CShader
{
	CShaderImpl<T>()
	{
		m_pGXShader = 0;
	};
	CShaderImpl<T>(const CShaderImpl<T> &pOther) :
		m_pGXShader(pOther.m_pGXShader)
	{
		add_ref(m_pGXShader);
	};
	~CShaderImpl<T>()
	{
		mem_release(m_pGXShader);
	};
	CShaderImpl<T> &operator=(const CShaderImpl<T> &pOther)
	{
		m_pGXShader = pOther.m_pGXShader;
		add_ref(m_pGXShader);
	}

	bool isSame(const char *szPath, GXMacro *aMacros)
	{
		if(!strcmp(m_szPath, szPath))
		{
			GXMacro def = {NULL, NULL};
			if(!aMacros)
			{
				aMacros = &def;
			}
			UINT i = 0, j;
			bool bFound = false;
			while(aMacros[i].szName)
			{
				j = 0;
				bFound = false;
				while(m_aMacros[j].szName)
				{
					if(!strcmp(aMacros[i].szName, m_aMacros[j].szName))
					{
						if(strcmp(aMacros[i].szDefinition, m_aMacros[j].szDefinition))
						{
							return(false);
						}
						bFound = true;
						break;
					}
					++j;
				}
				if(!bFound)
				{
					return(false);
				}
				++i;
			}
			j = 0;
			while(m_aMacros[j].szName)
			{
				++j;
			}
			if(i != j)
			{
				return(false);
			}
			return(true);
		}
		return(false);
	}

	T *m_pGXShader;
};

//! вершинный шейдер
typedef CShaderImpl<IGXVertexShader> CShaderVS;

//! пиксельный шейдер
typedef CShaderImpl<IGXPixelShader> CShaderPS;

//! геометрический шейдер
typedef CShaderImpl<IGXGeometryShader> CShaderGS;

//! вычислительный шейдер
typedef CShaderImpl<IGXComputeShader> CShaderCS;

struct CShaderKit
{
	CShaderKit() = default;
	CShaderKit(ID idVertexShader, ID idPixelShader, IGXShaderSet *pShaderKit)
	{
		m_idVertexShader = idVertexShader;
		m_idPixelShader = idPixelShader; 
		m_pShaderKit = pShaderKit;
	}
	CShaderKit(const CShaderKit &pOther):
		m_idVertexShader(pOther.m_idVertexShader),
		m_idPixelShader(pOther.m_idPixelShader),
		m_idGeometryShader(pOther.m_idGeometryShader),
		m_idComputeShader(pOther.m_idComputeShader),
		m_pShaderKit(pOther.m_pShaderKit)
	{
		add_ref(m_pShaderKit);
	}
	~CShaderKit()
	{
		mem_release(m_pShaderKit);
	}
	
	CShaderKit &operator=(const CShaderKit &pOther)
	{
		m_idVertexShader = pOther.m_idVertexShader;
		m_idPixelShader = pOther.m_idPixelShader;
		m_idGeometryShader = pOther.m_idGeometryShader;
		m_idComputeShader = pOther.m_idComputeShader;
		m_pShaderKit = pOther.m_pShaderKit;
		add_ref(m_pShaderKit);
		return(*this);
	}

	ID m_idVertexShader = -1;
	ID m_idPixelShader = -1;
	ID m_idGeometryShader = -1;
	ID m_idComputeShader = -1;
	IGXShaderSet *m_pShaderKit = NULL;
};


//**************************************************************************

#define LOAD_SHADER_FAIL 0
#define LOAD_SHADER_COMPLETE 1
#define LOAD_SHADER_CACHE 2

//**************************************************************************

class CShaderManager;
class CShaderReloadCommand: public IXConsoleCommand
{
public:
	CShaderReloadCommand(CShaderManager *pShaderManager);

	void XMETHODCALLTYPE execute(int argc, const char **argv) override;

private:
	CShaderManager *m_pManager;
};

//##########################################################################

class CShaderPreprocessor;
//! менеджер шейдеров
class CShaderManager
{
public:
	CShaderManager(IXCore *pCore, IGXDevice *pDevice);
	~CShaderManager();

	//! добавление шейдера в очередь
	ID preLoad(SHADER_TYPE type, const char *szPath, GXMacro *aMacros = 0);

	//! загрузка всех шейдеров
	void allLoad(bool bReload = false);

	//! перезагрузить все шейдеры, с учетом макросов
	void reloadAll();

	ID createKit(ID idVertexShader, ID idPixelShader, ID idGeometryShader, ID idComputeShader);

	//! бинд шейдеров по id
	void bind(ID idShaderKit, IGXContext *pCtx);

	//! обнуление биндов шейдеров
	void unbind(IGXContext *pCtx);

	//! существует ли шейдер с именем файла и набором макросов, если да то возвращает id
	ID existsPathMacro(SHADER_TYPE type, const char *szPath, GXMacro *aMacros);

protected:
	IGXDevice *m_pDevice = NULL;
	IXCore *m_pCore = NULL;

	Array<CShaderVS*> m_aVS;	//!< массивы vs шейдеров
	Array<CShaderPS*> m_aPS;	//!< массивы ps шейдеров
	Array<CShaderGS*> m_aGS;	//!< массивы gs шейдеров
	Array<CShaderCS*> m_aCS;	//!< массивы cs шейдеров

	Array<CShaderKit*> m_aShaderKit;
	
	SpinLock m_spLock;

	int m_iLastAllLoadVS = 0;		//! общее количество загруженных vs шейдеров, с прошлого раза
	int m_iLastAllLoadPS = 0;		//! общее количество загруженных ps шейдеров, с прошлого раза
	int m_iLastAllLoadGS = 0;		//! общее количество загруженных gs шейдеров, с прошлого раза
	int m_iLastAllLoadCS = 0;		//! общее количество загруженных gs шейдеров, с прошлого раза

	CShaderPreprocessor *m_pPreprocessor = NULL;

	CShaderReloadCommand m_shaderReloadCommand;
};

#endif
