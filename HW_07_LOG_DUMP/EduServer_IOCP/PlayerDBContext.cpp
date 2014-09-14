#include "stdafx.h"
#include "SQLStatement.h"
#include "Log.h"
#include "PlayerDBContext.h"
#include "DBHelper.h"
#include "ClientSession.h"


//todo: CreatePlayerDataContext 구현  -> 구현
bool CreatePlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;

	// SP에서 SELECT @@ 한 결과물을 BindResultColumn... 으로 가져온다.
	int result = 0;

	dbHelper.BindParamText( mPlayerName );
	dbHelper.BindResultColumnInt( &result );

	if ( dbHelper.Execute( SQL_CreatePlayer ) )
	{
		if ( dbHelper.FetchRow() )
		{
			return result != 0;
		}
	}

	return false;
}

void CreatePlayerDataContext::OnSuccess()
{
	// 원래는 성공시 할 일 넣기
	mSessionObject->mPlayer.ResponseCreatePlayerData( mPlayerName );
}


//todo: DeletePlayerDataContext 구현
bool DeletePlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;

	int result = 0;

	dbHelper.BindParamInt( &mPlayerId );
	dbHelper.BindResultColumnInt( &result );

	if ( dbHelper.Execute( SQL_DeletePlayer ) )
	{
		if ( dbHelper.FetchRow() )
		{
			// 결과 적용이 된 행이 하나도 없다면(0이라면) 실패한 것
			return result != 0;
		}
	}

	return false;
}

void DeletePlayerDataContext::OnSuccess()
{
	mSessionObject->mPlayer.ResponseDeletePlayerData( mPlayerId );
}

void DeletePlayerDataContext::OnFail()
{
	EVENT_LOG( "LoadPlayerDataContext fail", mPlayerId );
}

bool LoadPlayerDataContext::OnSQLExecute()
{
	DBHelper dbHelper;

	dbHelper.BindParamInt(&mPlayerId);

	dbHelper.BindResultColumnText(mPlayerName, MAX_NAME_LEN);
	dbHelper.BindResultColumnFloat(&mPosX);
	dbHelper.BindResultColumnFloat(&mPosY);
	dbHelper.BindResultColumnFloat(&mPosZ);
	dbHelper.BindResultColumnBool(&mIsValid);
	dbHelper.BindResultColumnText(mComment, MAX_COMMENT_LEN);

	if (dbHelper.Execute(SQL_LoadPlayer))
	{
		if (dbHelper.FetchRow())
		{
			return true;
		}
	}

	return false;
}

void LoadPlayerDataContext::OnSuccess()
{
	//todo: 플레이어 로드 성공시 처리하기  -> 구현
	mSessionObject->mPlayer.ResponseLoad( mPlayerId, mPosX, mPosY, mPosZ, mIsValid, mPlayerName, mComment );
}

void LoadPlayerDataContext::OnFail()
{
	EVENT_LOG("LoadPlayerDataContext fail", mPlayerId);
}

bool UpdatePlayerPositionContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;

	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamFloat(&mPosX);
	dbHelper.BindParamFloat(&mPosY);
	dbHelper.BindParamFloat(&mPosZ);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerPosition))
	{
		if (dbHelper.FetchRow())
		{
			return result != 0;
		}
	}

	return false;
}

void UpdatePlayerPositionContext::OnSuccess()
{
	mSessionObject->mPlayer.ResponseUpdatePosition(mPosX, mPosY, mPosZ);
}

void UpdatePlayerPositionContext::SetNewPosition( float x, float y, float z )
{
	mPosX = x;
	mPosY = y;
	mPosZ = z;
}


bool UpdatePlayerCommentContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;
	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamText(mComment);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerComment))
	{
		if (dbHelper.FetchRow())
		{
			return result != 0;
		}
	}

	return false;
}

void UpdatePlayerCommentContext::SetNewComment(const wchar_t* comment)
{
	wcscpy_s(mComment, comment);
}

void UpdatePlayerCommentContext::OnSuccess()
{
	mSessionObject->mPlayer.ResponseUpdateComment(mComment);
}



bool UpdatePlayerValidContext::OnSQLExecute()
{
	DBHelper dbHelper;
	int result = 0;

	dbHelper.BindParamInt(&mPlayerId);
	dbHelper.BindParamBool(&mIsValid);

	dbHelper.BindResultColumnInt(&result);

	if (dbHelper.Execute(SQL_UpdatePlayerValid))
	{
		if (dbHelper.FetchRow())
		{
			return result !=0 ;
		}
	}

	return false;
}

void UpdatePlayerValidContext::OnSuccess()
{
	mSessionObject->mPlayer.ResponseUpdateValidation(mIsValid);
}


