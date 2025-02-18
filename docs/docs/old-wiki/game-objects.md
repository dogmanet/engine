---
---

# Игровые объекты

## Введение

Игровой объект (Entity) - базовый объект для создания интерактивных уровней. 
Это может быть что угодно: кнопка, дверь, лампочка. Так же они могут не иметь визуальной части в игровом мире. 
Каждый энтить имеет набор свойств, которые можно настраивать через редактор уровней, а так же набор входов и выходов, 
через которые осуществляется взаимодействие. Подробнее в статье создание entity. 

### Точечные энтити

Не имеют объема в мире, но могут иметь визуальный эффект и/или область действия. Сюда входят:
* Логические энтити (logic_*);
* Свет;
* Камера.

### С моделью

Имеют модель, и соответственно объем в мире. Сюда входят:
* Модели (prop_*);
* NPC (npc_*);
* Оружие (wpn_*);
* Припасы (ammo_*);
* Триггеры (trigger_*).

## Свойства

Набор переменных энтити, которые можно редактировать через [редактор уровней](level-editor.md). Примером одного их свойств является цвет лампочки. 

## Флаги

Набор булевых переменных, которые определяют базовое поведение объекта. Флаги не могут быть изменены в процессе игры. 
Например, флаг Initially dark у лампочки указывает, что при загрузке уровня лампочка должна быть выключена. 
Флаги можно редактировать через [редактор уровней](level-editor.md). 

## Входы и выходы

Входы и выходы - механизм взаимодействия различных entity в процессе игры. 
Выходы одних объектов напрямую связываются с входами других, и когда выход активируется - это приводит к активации связанных входов. 

## Использование энтитей

Базовый туториал - [Управление светом кнопкой](tutor-light-button.md)
