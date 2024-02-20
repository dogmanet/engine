#ifndef __UICOMMAND_H 
#define __UICOMMAND_H 

#include "IUICommand.h"

class CUICommand : public IXUnknownImplementation<IUICommand>
{
private:

	class BaseFunc
	{
	public:
		BaseFunc() {}

		virtual ~BaseFunc() {}

		virtual void execute() = 0;
	};

	template <typename FuncT>
	class DerivedFunc : public BaseFunc
	{
	private:
		FuncT function;

	public:
		DerivedFunc(FuncT f)
			: BaseFunc(), function(f)
		{}

		virtual void execute()
		{
			return function();
		}
	};

	BaseFunc *func;

public:

	CUICommand()
		: func() {}

	template <typename FuncT>
	CUICommand(FuncT f)
		: func(new DerivedFunc<FuncT>(f)) {}

	void execute()
	{
		return func->execute();
	}

protected:
	~CUICommand()
	{
		mem_delete(func);
	}
};

#endif
