#include "KTreeRouter.h"


class KTreeRouterTest
{
private:

	KTreeRouter* kTreeRouter;

public:

	KTreeRouterTest(KTreeRouter kTreeRouter)
	{
		this->kTreeRouter = &kTreeRouter;
	}

	void test() {

		kTreeRouter->Update(2, 30);
		kTreeRouter->Update(1, 43);
		kTreeRouter->Update(2, 42);
		kTreeRouter->Update(3, 41);
		kTreeRouter->Update(10, 110);

		kTreeRouter->Query(26);
		kTreeRouter->Query(1);
		kTreeRouter->Query(30);
		kTreeRouter->Query(31);
		kTreeRouter->Query(47);
	}
	
};

