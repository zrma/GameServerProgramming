// PriorityQueue.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

class Test
{
public:
	Test(int i):mData(i) {}
	~Test() {}

	int mData = 0;
};

struct Comparator
{
	bool operator()(const Test& lhs, const Test& rhs)
	{
		return lhs.mData > rhs.mData;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::priority_queue<Test, std::vector<Test>, Comparator> queue;

	for ( int i = 0; i < 100; i += 2 )
	{
		queue.push( Test( i ) );
	}

	const Test& t = queue.top();
	printf_s( "%d \n", t.mData );

	for ( int i = 100; i < 200; i += 2 )
	{
		queue.push( Test( i ) );
		const Test& t2 = queue.top();
		printf_s( "%d, %d \n", t.mData, t2.mData );
	}

	getchar();
	return 0;
}

