
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Файл определений для работы с файлами моделей
*/

/*! \defgroup sxgcore_mdl_defs Определения для работы с файлами моделей
	\ingroup sxgcore
@{
*/

#include <gdefines.h>

#ifndef __MODEL_FILE_H
#define __MODEL_FILE_H

#define SX_MODEL_VERSION 7     /*!< Актуальная версия файла модели */
#define SX_MODEL_MAGICK 30510779525321540 /*!< Магическая константа для определения файла */

#define MODEL_MAX_NAME 32       /*!< Максимальная длина имени в файле модели */
#define MODEL_CTRL_MAX_BONES 64 /*!< Максимальное количество костей, затрагиваемых контроллером */

#define MODEL_MAX_FILE	128 /*!< Максимальная длина имени файла модели */

struct model_vertex
{
	float3_t Pos;      /*!< Позиция */
	float2_t Tex;      /*!< Текстурные координаты */
	float3_t Norm;     /*!< Нормаль */
};

/*! Вертекс для статической модели
*/
struct vertex_static: public model_vertex
{
};

/*! Вертекс для статической модели расширенный
*/
struct vertex_static_ex: public model_vertex
{
	vertex_static_ex() = default;
	vertex_static_ex(float3_t vPos, float2_t vTex, float3_t vNormal, float3_t vTangent, float3_t vBinormal)
	{
		Pos = vPos; Tex = vTex; Norm = vNormal; Tangent = vTangent; Binorm = vBinormal;
	}
	float3_t Tangent;  /*!< Тангент */
	float3_t Binorm;   /*!< Бинормаль */
};

/*! Вертекс для анимированной модели
*/
struct vertex_animated: public model_vertex
{
	byte BoneIndices[4];  /*!< Индексы костей */
	float4_t BoneWeights; /*!< Веса костей */
};

/*! Вертекс для анимированной модели расширенный
*/
struct vertex_animated_ex: public vertex_static_ex
{
	byte BoneIndices[4];  /*!< Индексы костей */
	float4_t BoneWeights; /*!< Веса костей */
};

/*! Типы вертексов
*/
enum MODEL_VERTEX_TYPE
{
	MVT_STATIC = 0, /*!< Вертекс для статической модели */
	MVT_STATIC_EX,  /*!< Вертекс для статической модели расширенный */
	MVT_DYNAMIC,    /*!< Вертекс для анимированной модели */
	MVT_DYNAMIC_EX, /*!< Вертекс для анимированной модели расширенный */

	//count
	MVT_SIZE        /*!< Количество типов вертексов */ 
};

/*! Флаги файлов модели
*/
enum MODEL_FLAG
{
	MODEL_FLAG_COMPILED = 0x001,            /*!< Скомпилированная модель */
	MODEL_FLAG_SOURCED = 0x002,             /*!< Модель источник */

	MODEL_FLAG_SOURCE_ANIMATION = 0x004,    /*!< Источник анимации */
	MODEL_FLAG_SOURCE_MESH = 0x008,         /*!< Источник сетки */
	// MODEL_FLAG_SOURCE_LOD_MESH = 0x010,
	// MODEL_FLAG_SOURCE_PHYSBOX = 0x020,
	MODEL_FLAG_ANIMATED = 0x040,            /*!< Анимированная модель */
	MODEL_FLAG_PHYSIC = 0x080,              /*!< Есть физическая модель */
	MODEL_FLAG_STATIC = 0x100,              /*!< Статическая модель */
	MODEL_FLAG_HAS_BONES = 0x200,           /*!< В файле есть кости */
	MODEL_FLAG_HAS_GIBS = 0x400,            /*!< Unused */
	MODEL_FLAG_HAS_PHYSBOX = 0x800,         /*!< Имеется физическая оболочка (Unused?) */
	MODEL_FLAG_HAS_HITBOXES = 0x1000,       /*!< Имеются хитбоксы */
	MODEL_FLAG_HAS_TANGENT_BINORM = 0x2000, /*!< Используются расширенные типы вертексов */
	MODEL_FLAG_NEW_STYLE_DEPS = 0x4000,     /*!< Используется новый тип импортированных моделей */
	MODEL_FLAG_NEW_PACKED_ANIMS = 0x8000,   /*!< Сохранено с использованием исправленного размера структуры ModelSequence */
	MODEL_FLAG_NORMALIZED_NORMALS = 0x10000 /*!< нормали нормализованы */
};

/*! Тип топологий модели в файле
*/
enum MODEL_PT_TOPOLOGY
{
	MDLPT_TRIANGLELIST = 0,  /*!< Списки треугольников */
	MDLPT_TRIANGLESTRIP = 1, /*!< Полоски треугольников */
	//MDLPT_TRIANGLEFAN = 2,   /*!< Фаны треугольников */

	MDLPT_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
};

/*! Первый заголовок файла модели
*/
struct ModelHeader
{
	uint64_t Magick;
	uint32_t iVersion; //!< версия файла
	uint32_t iFlags; //!< флаги
	uint32_t iSkinCount; //!< Количество скинов в модели
	uint32_t iMaterialCount; //!< Количество сабсетов
	uint64_t iMaterialsOffset; //!< Смещение до блока списка материалов
	uint32_t iLODcount; //!< Количество лодов
	uint64_t iLODoffset; //!< Смещение до блока лодов
	uint32_t iBoneCount; //!< Количество костей в скелете
	uint64_t iBonesOffset; //!< Смещение до блока костей
	uint32_t iAnimationCount; //!< Количество анимаций в файле
	uint64_t iAnimationsOffset; //!< Смещение до блока анимаций
	uint64_t iSecondHeaderOffset; //!< Смещение до второго заголовка
};

/*! Второй заголовок файла модели
*/
struct ModelHeader2
{
	uint32_t iControllersCount; //!< Количество контроллеров
	uint64_t iControllersOffset; //!< Смещение до блока контроллеров
	uint32_t iDepsCount; //!< Количество включаемых файлов
	uint64_t iDependensiesOffset; //!< Смещение до блока включаемых файлов

	uint32_t iBoneTableCount; //!< Количество костей в таблице костей
	uint64_t iBoneTableOffset; //!< Смещение до таблицы костей
	uint32_t iActivitiesTableCount; //!< Количество записей в таблице активностей
	uint64_t iActivitiesTableOffset; //!< Смещение до таблицы активностей

	uint64_t iPhysicsDataOffset; //!< Смещение до блока физических данных

	MODEL_PT_TOPOLOGY topology; //!< Используемая топология примитивов

	uint32_t iHitboxCount; //!< Количество хитбоксов
	uint64_t iHitboxesOffset; //!< Смещение до блока хитбоксов

	uint64_t iBboxInfoOffset; //!< Смещение до блока информации об ограничивающем объеме

	uint64_t iThirdHeaderOffset; //!< Смещение до третьего заголовка
};

/*! Третий заголовок файла модели
*/
struct ModelHeader3
{
	uint32_t uChunksCount; //!< Количество чанков
	uint64_t uChunksOffset; //!< Смещение до списка чанков

	uint64_t uFourthHeaderOffset; //!< Смещение до четвертого заголовка
};

/*! Заголовок чанка
*/
struct ModelChunkHeader
{
	XGUID guidType; //!< Идентификатор типа чанка
	uint64_t uDataSize; //!< Размер данных чанка
	uint64_t uDataOffset; //!< Смещение до данных чанка
};


/*! Структура сабсета лода модели
*/
struct ModelLoDSubset
{
	uint32_t iMaterialID; //!< Идентификатор материала
	uint32_t iVectexCount; //!< Количество вертексов в сабсете
	uint32_t iIndexCount; //!< Количество индексов в сабсете
	model_vertex * pVertices; //!< Указатель на массив вертексов
	UINT * pIndices; //!< Массив индексов
	UINT iStartIndex; //!< Начальный индекс отрисовки сабсеа
	UINT iStartVertex; //!< Начальный вертекс отрисовки сабсета
};

/*! Дескриптор лода модели
*/
struct ModelLoD
{
	unsigned int iSubMeshCount = 0; //!< Количество сабсетов
	ModelLoDSubset * pSubLODmeshes = NULL; //!< Массив сабсетов
};// typedef SXmodelLODheader;
#define MODEL_LOD_STRUCT_SIZE sizeof(unsigned int) /*!< Файловый размер структуры дескриптора лода */




/*! Структура контроллера
*/
struct ModelBoneController
{
	uint32_t iBoneCount; /*!< Количество костей, которыми управляет контроллер */
	float3_t fMinRot; /*!< Вращение ОТ */
	float3_t fMaxRot; /*!< Вращение ДО */
	float3_t fMinTrans; /*!< Перемещение ОТ */
	float3_t fMaxTrans; /*!< Перемещение ДО */
	char szName[MODEL_MAX_NAME];  /*!< Имя контроллера */
	uint32_t bones[MODEL_CTRL_MAX_BONES];  /*!< Массив индексов костей */
};

/*! Структура кости
*/
struct ModelBone
{
	int32_t id;          /*!< Номер кости */
	int32_t pid;         /*!< Номер родительской кости */
	SMQuaternion orient; /*!< Вращение */
	float3_t position;   /*!< Позиция */
};

/*! Структура с именем кости
*/
struct ModelBoneName
{
	ModelBone bone; /*!< Кость */
	char szName[MODEL_MAX_NAME]; /*!< Имя */
};

/*! Структура дескриптор анимации
*/
#pragma pack(push, 1)
struct ModelSequence
{
	char name[MODEL_MAX_NAME]; /*!< Имя */
	byte bLooped; /*!< Анимация зациклена */
	char _padding[3];
	int32_t framerate; /*!< Скорость кадров в секунду */
	uint32_t activity; /*!< Идентификатор активности */
	uint32_t iNumFrames; /*!< Количество кадров */
	uint32_t act_chance; /*!< Шанс воспроизведения этой анимации в активности */
	ModelBone ** m_vmAnimData; /*!< Данные анимации */
};
#pragma pack(pop)
#define MODEL_SEQUENCE_STRUCT_SIZE_OLD (sizeof(char) * MODEL_MAX_NAME + sizeof(byte) + sizeof(uint32_t) * 4) /*!< Файловый размер структуры дескриптора анимации */
#define MODEL_SEQUENCE_STRUCT_SIZE (MODEL_SEQUENCE_STRUCT_SIZE_OLD + 3)

/*! Устарела, используется для обратной совместимости
*/
struct ModelDependensy
{
	char szName[MODEL_MAX_NAME];
};

/*! Структура описатель активности
*/
struct ModelActivity
{
	char szName[MODEL_MAX_NAME];
	//uint32_t chance;
};

/*! Управление частями контроллера
*/
enum MODEL_BONE_CTL
{
	MBCTL_ROT_X = 0, /*!< Вращение по X */
	MBCTL_ROT_Y,     /*!< Вращение по Y */
	MBCTL_ROT_Z,     /*!< Вращение по Z */
	MBCTL_TRANS_X,   /*!< Перемещение по X */
	MBCTL_TRANS_Y,   /*!< Перемещение по Y */
	MBCTL_TRANS_Z    /*!< Перемещение по Z */
};

#if 0
/*! Значение контроллера кости
*/
struct ModelBoneCrontrollerValue
{
	SMQuaternion rot;
	float3_t tr;
};
#endif

/*! Флаги импорта файла моделм анимации
*/
enum MODEL_IMPORT
{
	MI_MESH = 0x00000001, /*!< Импортировать сетку */
	MI_ANIMATIONS = 0x00000002, /*!< Импортировать анимации */
	MI_SKINS = 0x00000004, /*!< Импортировать скины */
	MI_CONTROLLERS = 0x00000008, /*!< Импортировать контроллеры */
	MI_HITBOXES = 0x00000010, /*!< Импортировать хитбоксы */

	MI_ALL = 0xFFFFFFFF  /*!< Импортировать все */
};

/*! Типы соединения частей модели
*/
enum MODEL_ATTACH
{
	MA_SKIN = 0x00000001, /*!< Объединение скелетов */
	MA_BONE = 0x00000002  /*!< Присоединение иерархии к одной из костей */
};

/*! Изменение состояния воспроизведения анимации
*/
enum ANIM_STATE
{
	AS_STOP, /*!< Остановлено */
	AS_PLAY, /*!< Воспроизведено */
	AS_LOOP  /*!< Повтор заного */
};

/*! Флаги управления частями модели
*/
enum MODEL_PART_FLAGS
{
	MP_NONE = 0x00000001,       /*!< нет */
	MP_HIDDEN = 0x00000002,     /*!< Часть спрятана */
	MP_COLLISIONS = 0x00000004, /*!< Проверять столкновения */
	MP_RAYTRACE = 0x00000008,   /*!< Проверять трассировки луча */
	MP_IMPORTED = 0x00000010    /*!< Часть импортируется из внешнего файла */
};

/*! Дескриптор аттачмента модели
*/
struct ModelAttachDesc
{
	MODEL_ATTACH type; /*!< Тип соединения */
	float3_t v3Offs;   /*!< Смещение */
	char szBone[MODEL_MAX_NAME]; /*!< Кость для присоединения (только для MA_BONE) */
};

/*! Дескриптор части модели
*/
struct ModelPart
{
	char _reserved[4];          /*!< Зарезервировано */
	UINT uImportFlags;          /*!< Флаги импорта MODEL_IMPORT */
	UINT uFlags;                /*!< Флаги части MODEL_PART_FLAGS */
	ModelAttachDesc attachDesc; /*!< Дескриптор аттачмента */
	char name[MODEL_MAX_NAME];  /*!< Имя части */
	char file[MODEL_MAX_FILE];  /*!< Имя файла модели */
};
#if 0
/*! Типы ошибок объединения скелетов
*/
enum MBERR
{
	MBE_SUCCESS = 0, /*!< Нет ошибки */
	MBE_NO_ROOT,     /*!< Не найден корень */
	MBE_BAD_HIERARCY /*!< Несовместимые иерархии костей */
};
#endif
/*! Типы хитбоксов
*/
enum HITBOX_TYPE
{
	HT_BOX = 0,  /*!< Параллелепипед */
	HT_CYLINDER, /*!< Цилиндр */
	HT_CAPSULE,  /*!< Капсула */
	HT_ELIPSOID,  /*!< Эллипсойд */
	HT_CONVEX
};

/*! Типы частей тела для хитбоксов
*/
XENUM(HITBOX_BODYPART,
	HBP_DEFAULT = 0, /*!< нет */
	HBP_HEAD,        /*!< Голова */
	HBP_CHEST,       /*!< Грудь */
	HBP_STOMACH,     /*!< Живот */
	HBP_LEFTARM,     /*!< Левая рука */
	HBP_RIGHTARM,    /*!< Правая рука */
	HBP_LEFTLEG,     /*!< Левая нога */
	HBP_RIGHTLEG     /*!< Правая нога */
);

/*! Дескриптор хитбокса (устарел, не использовать)
*/
struct ModelHitbox
{
	HITBOX_TYPE type;               /*!< Тип хитбокса */
	HITBOX_BODYPART part;           /*!< Часть тела */
	float3_t lwh;                   /*!< Размеры */
	float3_t pos;                   /*!< Положение */
	float3_t rot;                   /*!< Ориентация */
	char bone[MODEL_MAX_NAME];      /*!< Имя кости */
	int bone_id;                    /*!< Идентификатор кости */
	char name[MODEL_MAX_NAME];      /*!< Имя хитбокса */
};



struct ModelPhyspartData
{
};

#pragma pack(push, 1)
struct ModelPhyspartDataConvex: public ModelPhyspartData
{
	uint32_t iVertCount = 0;
	float3_t *pVerts = NULL;
};
#pragma pack(pop)
#define MODEL_PHYSPART_DATA_CONVEX_STRUCT_SIZE sizeof(uint32_t)

/*! Дескриптор хитбокса
*/
#pragma pack(push, 1)
struct ModelPhyspart
{
	HITBOX_TYPE type;               /*!< Тип хитбокса */
	float3_t lwh;                   /*!< Размеры */
	float3_t pos;                   /*!< Положение */
	float4_t rot;                   /*!< Ориентация */
	char bone[MODEL_MAX_NAME];      /*!< Имя кости */
	int bone_id;                    /*!< Идентификатор кости */
	char name[MODEL_MAX_NAME];      /*!< Имя хитбокса */
	uint64_t iDataOffset = 0;
	//-----------------------
	ModelPhyspartData *pData = NULL;
};
#pragma pack(pop)
#define MODEL_PHYSPART_STRUCT_SIZE (sizeof(ModelPhyspart) - sizeof(ModelPhyspartData*))

#pragma pack(push, 1)
struct ModelPhysData
{
	float fWeight = 1.0f;
	uint32_t iPhyspartCount = 0;
	uint64_t iPartsOffset = 0;
	//-----------------------
	ModelPhyspart *pParts = NULL;
};
#pragma pack(pop)
#define MODEL_PHYSDATA_STRUCT_SIZE (sizeof(ModelPhysData) - sizeof(ModelPhyspart*))



// {A373CAED-1B4C-4055-AA6E-E74348A40842}
#define DSE_CHUNK_HITBOXES_EX_GUID DEFINE_XGUID(0xa373caed, 0x1b4c, 0x4055, 0xaa, 0x6e, 0xe7, 0x43, 0x48, 0xa4, 0x8, 0x42)

struct ModelHitboxExChunk
{
	uint32_t uVersion = 1;
	uint32_t uHitboxExCount;
};

#pragma pack(push, 1)
struct ModelHitboxEx
{
	HITBOX_BODYPART part;           /*!< Часть тела */
	ModelPhyspart hitbox;
};
#pragma pack(pop)
#define MODEL_HITBOXEX_STRUCT_SIZE (sizeof(ModelHitboxEx) - sizeof(ModelPhyspartData*))

/*! @} sxgcore_mdl_defs */

#endif

/*! @} */
