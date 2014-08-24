#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "DBHelper.h"

//todo: DbHelper의 static 멤버변수 초기화  <- 구현
SQLHENV		DbHelper::mSqlHenv = nullptr;
SQL_CONN*	DbHelper::mSqlConnPool = nullptr;
int			DbHelper::mDbWorkerThreadCount = 0;


//////////////////////////////////////////////////////////////////////////
// 개요 설명
//
// static 함수 Initialize / Finalize 로 DB 풀은 정적 포인터에 동적으로 미리 할당 해둔다.
//
// 필요할 때 DbHelper() 클래스를 생성해서 DB 커넥션 풀에서 꺼내서 사용함
// 생성자와 소멸자는 해당 역할 수행
//////////////////////////////////////////////////////////////////////////

DbHelper::DbHelper()
{
	CRASH_ASSERT(mSqlConnPool[LWorkerThreadId].mUsingNow == false);

	mCurrentSqlHstmt = mSqlConnPool[LWorkerThreadId].mSqlHstmt;
	mCurrentResultCol = 1;
	mCurrentBindParam = 1;
	CRASH_ASSERT(mCurrentSqlHstmt != nullptr);

	mSqlConnPool[LWorkerThreadId].mUsingNow = true;
}

DbHelper::~DbHelper()
{
	//todo: SQLFreeStmt를 이용하여 현재 SQLHSTMT 해제(unbind, 파라미터리셋, close 순서로)  <- 구현

	// SQLBindCol() 호출에 의해 바인드 된 모든 열 해제
	SQLFreeStmt( mCurrentSqlHstmt, SQL_UNBIND );

	// SQLBindParam() 호출에 의해 설정된 모든 매개변수 해제
	SQLFreeStmt( mCurrentSqlHstmt, SQL_RESET_PARAMS );

	// 연관된 핸들(hstmt)과 연관된 커서(있는 경우)가 닫히고 모든 미결인 결과가 삭제
	SQLFreeStmt( mCurrentSqlHstmt, SQL_CLOSE );

	// 현재 스레드의 풀이 사용 중이 아님
	mSqlConnPool[LWorkerThreadId].mUsingNow = false;
}

bool DbHelper::Initialize(const wchar_t* connInfoStr, int workerThreadCount)
{
	//todo: mSqlConnPool, mDbWorkerThreadCount를 워커스레스 수에 맞추어 초기화  <- 구현

	// 방어 코드
	CRASH_ASSERT( workerThreadCount > 0 );

	mDbWorkerThreadCount = workerThreadCount;
	mSqlConnPool = new SQL_CONN[mDbWorkerThreadCount];
	
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &mSqlHenv))
	{
		printf_s("DbHelper Initialize SQLAllocHandle failed\n");
		return false;
	}

	if (SQL_SUCCESS != SQLSetEnvAttr(mSqlHenv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER))
	{
		printf_s("DbHelper Initialize SQLSetEnvAttr failed\n");
		return false;
	}	

	/// 스레드별로 SQL connection을 풀링하는 방식. 즉, 스레드마다 SQL서버로의 연결을 갖는다.
	for (int i = 0; i < mDbWorkerThreadCount; ++i)
	{
		//todo: SQLAllocHandle을 이용하여 SQL_CONN의 mSqlHdbc 핸들 사용가능하도록 처리  <- 구현
		if ( SQLAllocHandle( SQL_HANDLE_DBC, mSqlHenv, &mSqlConnPool[i].mSqlHdbc ) )
		{
			printf_s( "DbHelper Initialize SQLConnPool Alloc failed\n" );
			return false;
		}

		SQLSMALLINT resultLen = 0;
		
		//todo: SQLDriverConnect를 이용하여 SQL서버에 연결하고 그 핸들을 SQL_CONN의 mSqlHdbc에 할당  <- 구현
		// SQLRETURN ret = 0; // =  SQLDriverConnect(...);

		SQLRETURN ret = SQLDriverConnect( mSqlConnPool[i].mSqlHdbc, NULL, (SQLWCHAR*)connInfoStr,
										  SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );

		if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
		{
			SQLWCHAR sqlState[1024] = { 0, } ;
			SQLINTEGER nativeError = 0;
			SQLWCHAR msgText[1024] = { 0, } ;
			SQLSMALLINT textLen = 0 ;

			// http://msdn.microsoft.com/en-us/library/windows/desktop/ms716256(v=vs.85).aspx
			// ODBC 함수가 SQL_SUCCESS 이외의 상태를 반환 했을 때 오류 정보 가져오기
			SQLGetDiagRec(SQL_HANDLE_DBC, mSqlConnPool[i].mSqlHdbc, 1, sqlState, &nativeError, msgText, 1024, &textLen);

			wprintf_s(L"DbHelper Initialize SQLDriverConnect failed: %s \n", msgText);

			return false;
		}

		//todo: SQLAllocHandle를 이용하여 SQL_CONN의 mSqlHstmt 핸들 사용가능하도록 처리  <- 구현
		if ( SQL_SUCCESS != 
			 SQLAllocHandle( SQL_HANDLE_STMT, mSqlConnPool[i].mSqlHdbc, &mSqlConnPool[i].mSqlHstmt ) )
		{
			SQLWCHAR sqlState[1024] = { 0, };
			SQLINTEGER nativeError = 0;
			SQLWCHAR msgText[1024] = { 0, };
			SQLSMALLINT textLen = 0;

			SQLGetDiagRec( SQL_HANDLE_DBC, mSqlConnPool[i].mSqlHdbc, 1, sqlState, &nativeError, msgText, 1024, &textLen );

			wprintf_s( L"DbHelper Initialize STMT SQLAllocHandle failed: %s \n", msgText );

			return false;
		}
	
	}

	return true;
}


void DbHelper::Finalize()
{
	for (int i = 0; i < mDbWorkerThreadCount; ++i)
	{
		SQL_CONN* currConn = &mSqlConnPool[i];
		if (currConn->mSqlHstmt)
			SQLFreeHandle(SQL_HANDLE_STMT, currConn->mSqlHstmt);

		if (currConn->mSqlHdbc)
			SQLFreeHandle(SQL_HANDLE_DBC, currConn->mSqlHdbc);
	}

	delete[] mSqlConnPool;


}

bool DbHelper::Execute(const wchar_t* sqlstmt)
{
	//todo: mCurrentSqlHstmt핸들 사용하여 sqlstmt를 수행.  <- 구현
	// SQLRETURN ret = 0; //= SQLExecDirect(...);
	SQLRETURN ret = SQLExecDirect( mCurrentSqlHstmt, (SQLWCHAR*)sqlstmt, SQL_NTS );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::FetchRow()
{
	//todo: mCurrentSqlHstmt가 들고 있는 내용 fetch  <- 구현
	// SQLRETURN ret = 0; 
	SQLRETURN ret = SQLFetch( mCurrentSqlHstmt );
	
	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		if (SQL_NO_DATA != ret)
		{
			PrintSqlStmtError();
		}
		
		return false;
	}

	return true;
}



bool DbHelper::BindParamInt(int* param)
{
	//todo: int형 파라미터 바인딩  <- 구현
	// SQLRETURN ret = 0; // = SQLBindParameter(...);
	SQLRETURN ret = SQLBindParameter( mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_LONG, SQL_INTEGER, 4, 0, param, 0, NULL );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamFloat(float* param)
{
	SQLRETURN ret = SQLBindParameter( mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_FLOAT, SQL_REAL, 15, 0, param, 0, NULL );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamBool(bool* param)
{
	//todo: bool형 파라미터 바인딩  <- 구현
	// SQLRETURN ret = 0; // = SQLBindParameter(...);
	SQLRETURN ret = SQLBindParameter( mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_TINYINT, SQL_TINYINT, 1, 0, param, 0, NULL );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamText(const wchar_t* text)
{

	//todo: 유니코드 문자열 바인딩  <- 구현
	// SQLRETURN ret = 0; // = SQLBindParameter(...);
	size_t len = wcslen( text );
	SQLRETURN ret = SQLBindParameter( mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_WCHAR, SQL_WVARCHAR, len, 0, (SQLPOINTER)text, 0, NULL );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}


void DbHelper::BindResultColumnInt(int* r)
{
	SQLLEN len = 0;
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_LONG, r, 4, &len );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}
void DbHelper::BindResultColumnFloat(float* r)
{
	SQLLEN len = 0;
	//todo: float형 결과 컬럼 바인딩  <- 구현
	// SQLRETURN ret = 0;
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_FLOAT, r, 15, &len );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}

void DbHelper::BindResultColumnBool(bool* r)
{
	SQLLEN len = 0;
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_TINYINT, r, 1, &len );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}
void DbHelper::BindResultColumnText(wchar_t* text, size_t count)
{
	SQLLEN len = 0;
	//todo: wchar_t*형 결과 컬럼 바인딩  <- 구현
	// SQLRETURN ret = 0;
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_WCHAR, text, count, &len );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}


void DbHelper::PrintSqlStmtError()
{
	SQLWCHAR sqlState[1024] = { 0, };
	SQLINTEGER nativeError = 0;
	SQLWCHAR msgText[1024] = { 0, };
	SQLSMALLINT textLen = 0;

	SQLGetDiagRec(SQL_HANDLE_STMT, mCurrentSqlHstmt, 1, sqlState, &nativeError, msgText, 1024, &textLen);

	wprintf_s(L"DbHelper SQL Statement Error: %ls \n", msgText);
}