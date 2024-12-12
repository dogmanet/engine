#ifndef __IXRESOURCEMODEL_H
#define __IXRESOURCEMODEL_H

#include <gdefines.h>

#define XMODEL_MAX_NAME 32

/*! Типы физбоксов
*/
enum XPHYSBOXTYPE
{
	XPBT_BOX = 0,     /*!< Параллелепипед */
	XPBT_SPHERE,      /*!< Сфера */
	XPBT_CONVEX,
	XPBT_CYLINDER,    /*!< Цилиндр */
	XPBT_CAPSULE      /*!< Капсула */
};

/*! Типы хитбоксов
*/
enum XHITBOXTYPE
{
	XHT_BOX = 0,  /*!< Параллелепипед */
	XHT_CYLINDER, /*!< Цилиндр */
	XHT_CAPSULE,  /*!< Капсула */
	XHT_SPHERE    /*!< Сфера */
};

/*! Типы частей тела для хитбоксов
*/
enum XHITBOXBODYPART
{
	XHBP_DEFAULT = 0, /*!< нет */
	XHBP_HEAD,        /*!< Голова */
	XHBP_CHEST,       /*!< Грудь */
	XHBP_STOMACH,     /*!< Живот */
	XHBP_LEFTARM,     /*!< Левая рука */
	XHBP_RIGHTARM,    /*!< Правая рука */
	XHBP_LEFTLEG,     /*!< Левая нога */
	XHBP_RIGHTLEG     /*!< Правая нога */
};

/*! Типы топологий примитивов в модели
*/
enum XPT_TOPOLOGY
{
	XPT_TRIANGLELIST = 0,  /*!< Списки треугольников */
	XPT_TRIANGLESTRIP = 1, /*!< Полоски треугольников */
};


/*! Типы ресурсов моделей
*/
enum XMODELTYPE
{
	XMT_STATIC = 0,
	XMT_ANIMATED = 1,
};

///

/*! Флаги импорта файла моделм анимации
*/
enum XMODEL_IMPORT
{
	XMI_MESH = 0x00000001, /*!< Импортировать сетку */
	XMI_ANIMATIONS = 0x00000002, /*!< Импортировать анимации */
	XMI_SKINS = 0x00000004, /*!< Импортировать скины */ // ?
	XMI_CONTROLLERS = 0x00000008, /*!< Импортировать контроллеры */
	XMI_HITBOXES = 0x00000010, /*!< Импортировать хитбоксы */
	XMI_PHYSBOXES = 0x00000020, /*!< Импортировать физбоксы */
	
	XMI_ALL = 0xFFFFFFFF  /*!< Импортировать все */
};
DEFINE_ENUM_FLAG_OPERATORS(XMODEL_IMPORT);

/*! Флаги управления частями модели
*/
enum XMODEL_PART_FLAGS
{
	XMP_NONE = 0x00000001,       /*!< нет */
	XMP_HIDDEN = 0x00000002,     /*!< Часть спрятана */
	XMP_COLLISIONS = 0x00000004, /*!< Проверять столкновения */
	XMP_RAYTRACE = 0x00000008,   /*!< Проверять трассировки луча */
	XMP_IMPORTED = 0x00000010    /*!< Часть импортируется из внешнего файла */
};
DEFINE_ENUM_FLAG_OPERATORS(XMODEL_PART_FLAGS);


//##########################################################################

struct XResourceModelStaticVertex
{
	float3_t vPos;      /*!< Позиция */
	float2_t vTex;      /*!< Текстурные координаты */
	float3_t vNorm;     /*!< Нормаль */
	float3_t vTangent;  /*!< Тангент */
	float3_t vBinorm;   /*!< Бинормаль */
};

struct XResourceModelStaticVertexGPU
{
	float3_t vPos;			/*!< Позиция */
	float2_t vTex;      /*!< Текстурные координаты */
	short vNorm[4];     /*!< Нормаль */
	short vTangent[4];  /*!< Тангент */
	short vBinorm[4];   /*!< Бинормаль */
};

struct XResourceModelStaticSubset
{
	uint32_t iMaterialID = 0; //!< Идентификатор материала
	uint32_t iVertexCount; //!< Количество вертексов в сабсете
	uint32_t iIndexCount; //!< Количество индексов в сабсете
	XResourceModelStaticVertex *pVertices; //!< Указатель на массив вертексов
	UINT *pIndices; //!< Массив индексов
};

struct XResourceModelAnimatedVertex
{
	float3_t vPos;      /*!< Позиция */
	float2_t vTex;      /*!< Текстурные координаты */
	float3_t vNorm;     /*!< Нормаль */
	float3_t vTangent;  /*!< Тангент */
	float3_t vBinorm;   /*!< Бинормаль */
	byte u8BoneIndices[4];  /*!< Индексы костей */
	float4_t vBoneWeights; /*!< Веса костей */
};

struct XResourceModelAnimatedVertexGPU
{
	float3_t vPos;      /*!< Позиция */
	float2_t vTex;      /*!< Текстурные координаты */
	short vNorm[4];     /*!< Нормаль */
	short vTangent[4];  /*!< Тангент */
	short vBinorm[4];   /*!< Бинормаль */
	byte u8BoneIndices[4];  /*!< Индексы костей */
	byte vBoneWeights[4]; /*!< Веса костей */
};

struct XResourceModelAnimatedSubset
{
	uint32_t iMaterialID = 0; //!< Идентификатор материала
	uint32_t iVertexCount; //!< Количество вертексов в сабсете
	uint32_t iIndexCount; //!< Количество индексов в сабсете
	XResourceModelAnimatedVertex * pVertices; //!< Указатель на массив вертексов
	UINT * pIndices; //!< Массив индексов
};

struct XResourceModelHitbox
{
	XHITBOXTYPE type;      /*!< Тип хитбокса */
	XHITBOXBODYPART part;  /*!< Часть тела */
	float3_t lwh;          /*!< Размеры */
	float3_t pos;          /*!< Положение */
	SMQuaternion rot;      /*!< Ориентация */
	int bone_id;           /*!< Идентификатор кости */
	char szName[XMODEL_MAX_NAME];    /*!< Имя хитбокса */
};

struct XResourceModelBone
{
	SMQuaternion orient; /*!< Вращение */
	float3_t position;   /*!< Позиция */
};

struct XResourceModelSequence
{
	char szName[XMODEL_MAX_NAME]; /*!< Имя */
	bool isLooped; /*!< Анимация зациклена */
	int iFramerate; /*!< Скорость кадров в секунду */
	int iActivity; /*!< Идентификатор активности */
	UINT uNumFrames; /*!< Количество кадров */
	UINT uActivityChance; /*!< Шанс воспроизведения этой анимации в активности */
	XResourceModelBone **m_ppSequenceData; /*!< Данные анимации */
};

struct XResourceModelController
{
	char szName[XMODEL_MAX_NAME];    /*!< Имя контроллера */

	SMQuaternion qMinRot;  /*!< Вращение ОТ */
	SMQuaternion qMaxRot;  /*!< Вращение ДО */
	float3_t vMinTrans;    /*!< Перемещение ОТ */
	float3_t vMaxTrans;    /*!< Перемещение ДО */
	UINT uBoneCount;       /*!< Количество костей, которыми управляет контроллер */
	UINT *pBones;          /*!< Массив индексов костей */
};

//##########################################################################

class IModelPhysboxBox;
class IModelPhysboxSphere;
class IModelPhysboxConvex;
class IModelPhysboxCylinder;
class IModelPhysboxCapsule;
class IModelPhysbox: public IXUnknown
{
public:
	virtual XPHYSBOXTYPE XMETHODCALLTYPE getType() const = 0;

	virtual const IModelPhysboxBox* XMETHODCALLTYPE asBox() const = 0;
	virtual IModelPhysboxBox* XMETHODCALLTYPE asBox() = 0;
	virtual const IModelPhysboxSphere* XMETHODCALLTYPE asSphere() const = 0;
	virtual IModelPhysboxSphere* XMETHODCALLTYPE asSphere() = 0;
	virtual const IModelPhysboxConvex* XMETHODCALLTYPE asConvex() const = 0;
	virtual IModelPhysboxConvex* XMETHODCALLTYPE asConvex() = 0;
	virtual const IModelPhysboxCylinder* XMETHODCALLTYPE asCylinder() const = 0;
	virtual IModelPhysboxCylinder* XMETHODCALLTYPE asCylinder() = 0;
	virtual const IModelPhysboxCapsule* XMETHODCALLTYPE asCapsule() const = 0;
	virtual IModelPhysboxCapsule* XMETHODCALLTYPE asCapsule() = 0;

	virtual float3 XMETHODCALLTYPE getPosition() const = 0;
	virtual void XMETHODCALLTYPE setPosition(const float3 &vPos) = 0;

	virtual SMQuaternion XMETHODCALLTYPE getOrientation() const = 0;
	virtual void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) = 0;
};

class IModelPhysboxBox: public IModelPhysbox
{
public:
	virtual float3 XMETHODCALLTYPE getSize() const = 0;
	virtual void XMETHODCALLTYPE setSize(const float3 &vSize) = 0;
};

class IModelPhysboxSphere: public IModelPhysbox
{
public:
	virtual float XMETHODCALLTYPE getRadius() const = 0;
	virtual void XMETHODCALLTYPE setRadius(float fRadius) = 0;
};

class IModelPhysboxConvex: public IModelPhysbox
{
public:
	virtual UINT XMETHODCALLTYPE getVertexCount() const = 0;
	virtual const float3_t * XMETHODCALLTYPE getData() const = 0;

	virtual void XMETHODCALLTYPE initData(UINT uVertexCount, const float3_t *pData = NULL) = 0;
	virtual float3_t * XMETHODCALLTYPE getData() = 0;
};

class IModelPhysboxCylinder: public IModelPhysbox
{
public:
	virtual float XMETHODCALLTYPE getRadius() const = 0;
	virtual void XMETHODCALLTYPE setRadius(float fRadius) = 0;

	virtual float XMETHODCALLTYPE getHeight() const = 0;
	virtual void XMETHODCALLTYPE setHeight(float fHeight) = 0;
};

class IModelPhysboxCapsule: public IModelPhysbox
{
public:
	virtual float XMETHODCALLTYPE getRadius() const = 0;
	virtual void XMETHODCALLTYPE setRadius(float fRadius) = 0;

	virtual float XMETHODCALLTYPE getHeight() const = 0;
	virtual void XMETHODCALLTYPE setHeight(float fHeight) = 0;
};

//##########################################################################

class IXResourceModelStatic;
class IXResourceModelAnimated;
// Implemented in core
class IXResourceModel: public IXUnknown
{
public:
	virtual UINT XMETHODCALLTYPE getPhysboxCount() const = 0;
	virtual const IModelPhysbox * XMETHODCALLTYPE getPhysbox(UINT uPart) const = 0;
	virtual IModelPhysbox* XMETHODCALLTYPE getPhysbox(UINT uPart) = 0;
	virtual int XMETHODCALLTYPE getPhysboxBone(UINT uPart) const = 0;
	virtual void XMETHODCALLTYPE addPhysbox(IModelPhysbox *pPhysbox, int iBone = -1) = 0;


	virtual UINT XMETHODCALLTYPE getMaterialCount() const = 0;
	virtual UINT XMETHODCALLTYPE getSkinCount() const = 0;
	virtual const char * XMETHODCALLTYPE getMaterial(UINT uMaterial, UINT uSkin = 0) const = 0;
	virtual void XMETHODCALLTYPE setMaterialCount(UINT uMaterialCount, UINT uSkinCount) = 0;
	virtual void XMETHODCALLTYPE setMaterial(UINT uMaterial, UINT uSkin, const char *szName) = 0;


	virtual float XMETHODCALLTYPE getMass() const = 0;
	virtual void XMETHODCALLTYPE setMass(float fMass) = 0;


	virtual UINT XMETHODCALLTYPE getGibsCount() const = 0;
	virtual const char * XMETHODCALLTYPE getGibName(UINT uIndex) const = 0;
	virtual UINT XMETHODCALLTYPE addGibName(const char *szFileName) = 0;

	virtual const IXResourceModel * XMETHODCALLTYPE getGib(UINT uIndex) const = 0;
	virtual void XMETHODCALLTYPE setGib(UINT uIndex, IXResourceModel *pResource) = 0;


	virtual XMODELTYPE XMETHODCALLTYPE getType() const = 0;
	virtual const IXResourceModelStatic * XMETHODCALLTYPE asStatic() const = 0;
	virtual const IXResourceModelAnimated * XMETHODCALLTYPE asAnimated() const = 0;
	virtual IXResourceModelStatic * XMETHODCALLTYPE asStatic() = 0;
	virtual IXResourceModelAnimated * XMETHODCALLTYPE asAnimated() = 0;


	virtual UINT XMETHODCALLTYPE getSubsetCount(UINT uLod) const = 0;
	virtual UINT XMETHODCALLTYPE addLod(UINT uSubsetCount, UINT *puVertexCount, UINT *puIndexCount) = 0;
	virtual UINT XMETHODCALLTYPE getLodCount() const = 0;

	virtual void XMETHODCALLTYPE setLocalAABB(const float3 &vMin, const float3 &vMax) = 0;

	virtual bool XMETHODCALLTYPE validate() const = 0;

	virtual IModelPhysboxBox    * XMETHODCALLTYPE newPhysboxBox() const = 0;
	virtual IModelPhysboxSphere * XMETHODCALLTYPE newPhysboxSphere() const = 0;
	virtual IModelPhysboxConvex * XMETHODCALLTYPE newPhysboxConvex() const = 0;
	virtual IModelPhysboxCylinder * XMETHODCALLTYPE newPhysboxCylinder() const = 0;
	virtual IModelPhysboxCapsule * XMETHODCALLTYPE newPhysboxCapsule() const = 0;

	virtual void XMETHODCALLTYPE makeReadOnly() = 0;
};

// Implemented in core
class IXResourceModelStatic: public virtual IXResourceModel
{
public:
	virtual void XMETHODCALLTYPE setPrimitiveTopology(XPT_TOPOLOGY topology) = 0;
	virtual XPT_TOPOLOGY XMETHODCALLTYPE getPrimitiveTopology() const = 0;

	virtual const XResourceModelStaticSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) const = 0;
	virtual XResourceModelStaticSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) = 0;
};

// Implemented in core
class IXResourceModelAnimated: public virtual IXResourceModel
{
public:
	virtual const XResourceModelAnimatedSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) const = 0;
	virtual XResourceModelAnimatedSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) = 0;


	virtual UINT XMETHODCALLTYPE getImportsCount() const = 0;
	virtual const char * XMETHODCALLTYPE getImportName(UINT uIndex) const = 0;
	virtual XMODEL_IMPORT XMETHODCALLTYPE getImportImportFlags(UINT uIndex) const = 0;
	virtual UINT XMETHODCALLTYPE addImportName(const char *szFileName, XMODEL_IMPORT importFlags) = 0;

	virtual const IXResourceModel * XMETHODCALLTYPE getImport(UINT uIndex) const = 0;
	virtual IXResourceModel * XMETHODCALLTYPE getImport(UINT uIndex) = 0;
	virtual void XMETHODCALLTYPE setImport(UINT uIndex, IXResourceModel *pResource) = 0;


	virtual UINT XMETHODCALLTYPE getPartsCount() const = 0;
	virtual const char * XMETHODCALLTYPE getPartFileName(UINT uIndex) const = 0;
	virtual const char * XMETHODCALLTYPE getPartName(UINT uIndex) const = 0;
	virtual XMODEL_IMPORT XMETHODCALLTYPE getPartImportFlags(UINT uIndex) const = 0;
	virtual XMODEL_PART_FLAGS XMETHODCALLTYPE getPartFlags(UINT uIndex) const = 0;
	virtual UINT XMETHODCALLTYPE addPartName(const char *szFileName, const char *szName, XMODEL_IMPORT importFlags, XMODEL_PART_FLAGS partFlags) = 0;

	virtual const IXResourceModel * XMETHODCALLTYPE getPart(UINT uIndex) const = 0;
	virtual IXResourceModel * XMETHODCALLTYPE getPart(UINT uIndex) = 0;
	virtual void XMETHODCALLTYPE setPart(UINT uIndex, IXResourceModel *pResource) = 0;


	virtual void XMETHODCALLTYPE setBoneCount(UINT uCount) = 0;
	virtual UINT XMETHODCALLTYPE getBoneCount() const = 0;

	virtual void XMETHODCALLTYPE setBoneInfo(int iBone, const char *szName, int iParent, const float3 &vTranslation, const SMQuaternion &vRotation) = 0;
	virtual int XMETHODCALLTYPE getBoneParent(int iBone) const = 0;
	virtual const char * XMETHODCALLTYPE getBoneName(int iBone) const = 0;
	virtual float3 XMETHODCALLTYPE getBoneTranslation(int iBone) const = 0;
	virtual SMQuaternion XMETHODCALLTYPE getBoneRotation(int iBone) const = 0;


	virtual UINT XMETHODCALLTYPE getSequenceCount() const = 0;
	virtual const XResourceModelSequence * XMETHODCALLTYPE getSequence(UINT uIndex) const = 0;

	virtual void XMETHODCALLTYPE setSequenceCount(UINT uCount) = 0;
	virtual XResourceModelSequence * XMETHODCALLTYPE getSequence(UINT uIndex) = 0;
	virtual void XMETHODCALLTYPE setSequenceFrameCount(UINT uIndex, UINT uFrameCount) = 0;
	

	virtual UINT XMETHODCALLTYPE addActivity(const char *szName) = 0;
	virtual const char * XMETHODCALLTYPE getActivityName(UINT uIndex) const = 0;
	virtual UINT XMETHODCALLTYPE getActivitiesCount() const = 0;


	virtual UINT XMETHODCALLTYPE getControllersCount() const = 0;
	virtual void XMETHODCALLTYPE setControllersCount(UINT uCount) = 0;
	virtual void XMETHODCALLTYPE setControllerBoneCount(UINT uIndex, UINT uAffectedBonesCount) = 0;
	virtual const XResourceModelController * XMETHODCALLTYPE getController(UINT uIndex) const = 0;
	virtual XResourceModelController * XMETHODCALLTYPE getController(UINT uIndex) = 0;


	virtual UINT XMETHODCALLTYPE getHitboxCount() const = 0;
	virtual void XMETHODCALLTYPE setHitboxCount(UINT uCount) = 0;
	virtual void XMETHODCALLTYPE setHitbox(UINT uIndex, IModelPhysbox *pPhysbox, int iBone, const char *szName, XHITBOXBODYPART part) = 0;
	virtual const IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT uIndex) const = 0;
	virtual IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT uIndex) = 0;
	virtual const char* XMETHODCALLTYPE getHitboxName(UINT uIndex) const = 0;
	virtual int XMETHODCALLTYPE getHitboxBone(UINT uIndex) const = 0;
	virtual XHITBOXBODYPART XMETHODCALLTYPE getHitboxBodyPart(UINT uIndex) const = 0;
};


#endif
