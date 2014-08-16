#pragma once

#define MAX_IO_THREAD	4

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER
};

extern __declspec(thread) int LThreadType;
extern __declspec(thread) int LIoThreadId;