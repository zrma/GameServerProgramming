#pragma once

//todo: 아래의 SP를 호출하는 SQL 스트링을 제대로 정의  <- 구현

const wchar_t* SQL_LoadPlayer = L"{ call dbo.spLoadPlayer (?) }"; ///< uid

const wchar_t* SQL_CreatePlayer = L"{ call dbo.spCreatePlayer (?) }";//  spCreatePlayer

const wchar_t* SQL_DeletePlayer = L"{ call dbo.spDeletePlayer (?) }";// spDeletePlayer

const wchar_t* SQL_UpdatePlayerPosition = L"{ call dbo.spUpdatePlayerPosition (?, ?, ?, ?) }";// spUpdatePlayerPosition

const wchar_t* SQL_UpdatePlayerComment = L"{ call dbo.spUpdatePlayerComment (?, ?) }"; //spUpdatePlayerComment

const wchar_t* SQL_UpdatePlayerValid = L"{ call dbo.spUpdatePlayerValid (?, ?) }"; // spUpdatePlayerValid

