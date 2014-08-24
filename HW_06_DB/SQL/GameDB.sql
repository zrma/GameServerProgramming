use GameDB
go

SET NOCOUNT ON
GO

SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

--todo: if exists를 사용하여 PlayerTable 테이블이 존재한다면 해당 테이블 드랍  <- 구현
IF EXISTS (SELECT [name] FROM sys.tables WHERE [name] = 'PlayerTable')
	BEGIN
		DROP TABLE [dbo].[PlayerTable]
	END
GO

CREATE TABLE [dbo].[PlayerTable](
	[playerUID] [int] NOT NULL PRIMARY KEY IDENTITY(100, 1),
	[playerName] [nvarchar](32) NOT NULL DEFAULT (N'noname'),
	[currentPosX] [float] NOT NULL DEFAULT ((0)),
	[currentPosY] [float] NOT NULL DEFAULT ((0)),
	[currentPosZ] [float] NOT NULL DEFAULT ((0)),
	[createTime] [datetime] NOT NULL,
	[isValid] [tinyint] NOT NULL,
	[comment] [nvarchar](256) NULL
)

GO

IF EXISTS ( select * from sys.procedures where name='spCreatePlayer' )
	DROP PROCEDURE [dbo].[spCreatePlayer]
GO

CREATE PROCEDURE [dbo].[spCreatePlayer]
	@name	NVARCHAR(32)
AS
BEGIN
	SET NOCOUNT ON
    --todo: 해당 이름의 플레이어를 생성하고 플레이어의 identity를 리턴, [createTime]는 현재 생성 날짜로 설정  <- 구현
	INSERT INTO [dbo].[PlayerTable]([playerName], [createTime], [isValid]) VALUES(@name, CURRENT_TIMESTAMP, 0)
	SELECT @@IDENTITY
END
GO

IF EXISTS ( select * from sys.procedures where name='spDeletePlayer' )
	DROP PROCEDURE [dbo].[spDeletePlayer]
GO

CREATE PROCEDURE [dbo].[spDeletePlayer]
	@playerUID	INT
AS
BEGIN
	SET NOCOUNT ON
	--todo: 해당 플레이어 삭제  <- 구현
	DELETE FROM [dbo].[PlayerTable] WHERE [playerUID] = @playerUID
	SELECT @@ROWCOUNT
END
GO

IF EXISTS ( select * from sys.procedures where name='spUpdatePlayerPosition' )
	DROP PROCEDURE [dbo].[spUpdatePlayerPosition]
GO

CREATE PROCEDURE [dbo].[spUpdatePlayerPosition]
	@playerUID	INT,
	@posX	FLOAT,
	@posY	FLOAT,
	@posZ	FLOAT
AS
BEGIN
	SET NOCOUNT ON
    -- todo: 해당 플레이어의 정보(x,y,z) 업데이트  <- 구현
	UPDATE [dbo].[PlayerTable]
		SET [currentPosX] = @posX,
			[currentPosY] = @posY,
			[currentPosZ] = @posZ
		WHERE [playerUID] = @playerUID
	SELECT @@ROWCOUNT
END
GO

IF EXISTS ( select * from sys.procedures where name='spUpdatePlayerComment' )
	DROP PROCEDURE [dbo].[spUpdatePlayerComment]
GO

CREATE PROCEDURE [dbo].[spUpdatePlayerComment]
	@playerUID	INT,
	@comment	NVARCHAR(256)
AS
BEGIN
	SET NOCOUNT ON
	UPDATE [dbo].[PlayerTable] SET comment=@comment WHERE playerUID=@playerUID
	SELECT @@ROWCOUNT
END
GO

IF EXISTS ( select * from sys.procedures where name='spUpdatePlayerValid' )
	DROP PROCEDURE [dbo].[spUpdatePlayerValid]
GO

CREATE PROCEDURE [dbo].[spUpdatePlayerValid]
	@playerUID	INT,
	@valid		TINYINT
AS
BEGIN
	SET NOCOUNT ON
	UPDATE PlayerTable SET isValid=@valid WHERE playerUID=@playerUID
	SELECT @@ROWCOUNT
END
GO


IF EXISTS ( select * from sys.procedures where name='spLoadPlayer' )
	DROP PROCEDURE [dbo].[spLoadPlayer]
GO

CREATE PROCEDURE [dbo].[spLoadPlayer]
	@playerUID	INT
AS
BEGIN
	SET NOCOUNT ON
    --todo: 플레이어 정보  [playerName], [currentPosX], [currentPosY], [currentPosZ], [isValid], [comment]  얻어오기  <- 구현
	SELECT [playerName],[currentPosX],[currentPosY],[currentPosZ],[isValid],[comment]
		FROM [dbo].[PlayerTable]
		WHERE [playerUID] = @playerUID
END		   
GO		   




--저장 프로시저 테스트

EXEC spCreatePlayer '테스트플레이어'
GO

EXEC spUpdatePlayerComment 100, "가나다라 플레이어 코멘트 테스트 kekeke"
GO

EXEC spUpdatePlayerValid 100, 1
GO

EXEC spLoadPlayer 100
GO

EXEC spDeletePlayer 100
GO