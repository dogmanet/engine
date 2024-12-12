#ifndef __UICLASSCOMMAND_H 
#define __UICLASSCOMMAND_H 

#include "IUICommand.h"

class CUIClassCommand : public IXUnknownImplementation<IUICommand>
{
private:

	class BaseFunc
	{
	public:
		virtual ~BaseFunc() = default;

		virtual void execute(IUIControl *pTarget) = 0;
	};

	template <typename FuncT, typename Class>
	class DerivedFunc : public BaseFunc
	{
	private:
		FuncT function;
		Class *obj;

	public:
		DerivedFunc(FuncT f, Class *obj): 
			function(f), 
			obj(obj)
		{}

		void execute(IUIControl *pTarget) override
		{
			(obj->*function)(pTarget);
		}
	};

	BaseFunc *func;

public:

	template <typename Class>
	CUIClassCommand(void(Class::*fuction)(IUIControl*), Class *obj):
		func(new DerivedFunc<void(Class::*)(IUIControl*), Class>(fuction, obj))
	{}

	void execute(IUIControl *pTarget)
	{
		func->execute(pTarget);
	}

	~CUIClassCommand()
	{
		mem_delete(func);
	}
};

#endif
