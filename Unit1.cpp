//	////////////////////////////////////////////////////////////////////////////
#define NOMINMAX
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#define max(a,b)	(((a) > (b)) ? (a) : (b))

#include	<vcl.h> 
#pragma		hdrstop
#include	"Unit1.h"
//	////////////////////////////////////////////////////////////////////////////
#pragma		package(smart_init)
#pragma		resource "*.dfm"

#include	<cmath>
#include	<math.h>
#include	<time.h>
#include	<vector> 
#include	<string>
#include	<cstdio>
#include	<cstdlib>
#include	<fstream>
#include	<cstring>
#include 	<sstream>

//	////////////////////////////////////////////////////////////////////////////

TWindow*			Window	= NULL;
Graphics::TBitmap*	mBuffer	= NULL;

//	////////////////////////////////////////////////////////////////////////////

union MColor
{
	unsigned int Value;
	struct
	{
		unsigned char R;
		unsigned char G;
		unsigned char B;
	};
};

//	////////////////////////////////////////////////////////////////////////////

namespace Bounce
{
	float easeIn (float t,float b , float c, float d)
	{
		return c - easeOut (d-t, 0, c, d) + b;
	}

	float easeOut(float t,float b , float c, float d)
	{
		if ((t/=d) < (1/2.75f))
		{
			return c*(7.5625f*t*t) + b;
		}
		else if (t < (2/2.75f))
		{
			float postFix = t-=(1.5f/2.75f);
			return c*(7.5625f*(postFix)*t + .75f) + b;
		}
		else if (t < (2.5/2.75))
		{
			float postFix = t-=(2.25f/2.75f);
			return c*(7.5625f*(postFix)*t + .9375f) + b;
		}
		else
		{
			float postFix = t-=(2.625f/2.75f);
			return c*(7.5625f*(postFix)*t + .984375f) + b;
		}
	}

	float easeInOut(float t,float b , float c, float d)
	{
		if (t < d/2) return easeIn (t*2, 0, c, d) * .5f + b;
		else return easeOut (t*2-d, 0, c, d) * .5f + c*.5f + b;
	}
};

//	////////////////////////////////////////////////////////////////////////////

template <typename T>
T Lerp(const T& start, const T& end, const float& percent)
{
	return (start + percent*(end - start));
}

//	////////////////////////////////////////////////////////////////////////////

namespace App
{
	Graphics::TBitmap*	mBitmaps[26];

	bool	bDebug			= false;
	
	int		ScreenX			= 0;
	int 	ScreenY			= 0;
	int 	mUpdateCount	= 0;
};

//	////////////////////////////////////////////////////////////////////////////

namespace PowerUp
{
	enum PowerUpType
	{
		PlusLife,
		MinusLife,
		LargePad,
		SmallPad,
		Catching,
		COUNT
	};

	struct Bonus
	{
		int			PosX;
		int			PosY;
		PowerUpType Type;
	};
};

namespace Game
{
	std::string	mLastLevel;

	float		BALL_MAX_SPEED		= 10.f;
	float		BALL_MIN_SPEED		= -10.f;

	const short BITMAP_PADDLE		= 16;
	const short BITMAP_WELCOME		= 17;
	const short BITMAP_GAME_OVER	= 18;
	const short BITMAP_WIN			= 19;
	const short BITMAP_BACKGROUND	= 20;
	const short BITMAP_POWERUP		= 21;

	const short LEVEL_WIDTH			= 16;					//	640	px
	const short LEVEL_HEIGHT		= 16;					//	448	px
	const short BLOCK_WIDTH			= 768 / LEVEL_WIDTH;	//	40	px
	const short BLOCK_HEIGHT		= 448 / LEVEL_HEIGHT;	//	28	px

	const short	POWER_UP_CHANCE		= 80;
	const short	POWER_UP_SPEED		= 10;

	std::vector<PowerUp::Bonus>	mBonuses;

//	////////////////////////////////////////////////////////////////////////////
		
	struct Block
	{
		Block()
			:	mHitCount(0),
				mBitmap(-1) {}
				
		Block(short hit, short bitmap = 0)
			:	mHitCount(hit),
				mBitmap(bitmap)
			{}
		
		short	mHitCount;
		short	mBitmap;
	};

	Block*	mBlocks[LEVEL_WIDTH][LEVEL_HEIGHT] = {NULL};

//	////////////////////////////////////////////////////////////////////////////

	enum State
	{
		WELCOME,
		START,
		PLAY,
		WIN,
		LOSE,
		GAME_OVER
	}	GameState = WELCOME;
	
	bool			bGameComplete	= false;
	unsigned		mBlocksToDestroy= 0;
	unsigned		mCurrentLevel	= 0;
	unsigned		mBlocksToWin	= 0;
	unsigned		mPoints			= 0;
	unsigned char	mLives			= 3;
	
//	////////////////////////////////////////////////////////////////////////////

	bool	bBallCatched	= true;
	bool	PowerCatch		= false;

	int		PadPosX;
	int		PadPosY;
	int		PadWidth		= 100;
	int		PadHeight		= 20;
	int		PadDelta		= 15;

//	////////////////////////////////////////////////////////////////////////////
	
	TColor	BallColor;
	
	int		BallX			= 10;
	int		BallY			= 10;
	int		BallRadius		= 20;

	float	BallVelX = 0;
	float	BallVelY = 0;
	
//	////////////////////////////////////////////////////////////////////////////
};

//	////////////////////////////////////////////////////////////////////////////

struct Line2D
{
	Line2D()
	{
		mBegin	[0] = 0;
		mBegin	[1] = 0;
		mEnd	[0]	= 0;
		mEnd	[1] = 0;
	}
	
	Line2D(float x1, float y1, float x2, float y2)
	{
		mBegin	[0] = x1;
		mBegin	[1] = y1;
		mEnd	[0]	= x2;
		mEnd	[1] = y2;
	}

	float	mBegin	[2];
	float	mEnd	[2];
	
	float Signed2DTriArea(float x1, float y1, float x2, float y2, float x3, float y3)//Point a, Point b, Point c)
	{
		return (x1 - x3) * (y2 - y3) - (y1 - y3) * (x2 - x3);
	}

	bool Colliding(Line2D& other)//Point a, Point b, Point c, Point d, float &t, Point &p)
	{
		// signs of areas correspond to which side of ab points c and d are
		float a1 = Signed2DTriArea(mBegin[0], mBegin[1], mEnd[0], mEnd[1], other.mEnd[0], other.mEnd[1]);//a,b,d); // Compute winding of abd (+ or -)
		float a2 = Signed2DTriArea(mBegin[0], mBegin[1], mEnd[0], mEnd[1], other.mBegin[0], other.mBegin[1]); // To intersect, must have sign opposite of a1

		// If c and d are on different sides of ab, areas have different signs
		if( a1 * a2 < 0.0f ) // require unsigned x & y values.
		{
			float a3 = Signed2DTriArea(other.mBegin[0], other.mBegin[1], other.mEnd[0], other.mEnd[1], mBegin[0], mBegin[1]); // Compute winding of cda (+ or -)
			float a4 = a3 + a2 - a1; // Since area is constant a1 - a2 = a3 - a4, or a4 = a3 + a2 - a1

			// Points a and b on different sides of cd if areas have different signs
			if( a3 * a4 < 0.0f )
			{
				// Segments intersect. Find intersection point along L(t) = a + t * (b - a).
				// t = a3 / (a3 - a4);
				// p = a + t * (b - a); // the point of intersection
				return true;
			}
		}

		// Segments not intersecting or collinear
		return false;
	}
};

//	////////////////////////////////////////////////////////////////////////////

bool intersects(float circleX, float circleY, float circleR, 
				float left, float width, float top, float height)
{
	float circleDistanceX = abs(circleX - left);
	float circleDistanceY = abs(circleY - top);

	if (circleDistanceX > (width/2 + circleR))	{ return false; }
	if (circleDistanceY > (height/2 + circleR))	{ return false; }

	if (circleDistanceX <= (width/2))			{ return true; } 
	if (circleDistanceY <= (height/2))			{ return true; }

	float cornerDistance_sq =	(circleDistanceX - width/2)	*(circleDistanceX - width/2) +
								(circleDistanceY - height/2)*(circleDistanceY - height/2);

	return (cornerDistance_sq <= (circleR * circleR));
}

//	////////////////////////////////////////////////////////////////////////////

void	TWindow::ClearBuffer(TColor color)
{
	mBuffer->Canvas->Brush->Color	= color;
	mBuffer->Canvas->Pen->Color		= color;

	mBuffer->Width	= PaintBox->Width;
	mBuffer->Height	= PaintBox->Height;

	mBuffer->Canvas->FillRect(mBuffer->Canvas->ClipRect);
}

void	TWindow::SwapBuffers()
{
	PaintBox->Canvas->Draw(0, 0, mBuffer);
}

//	////////////////////////////////////////////////////////////////////////////

__fastcall TWindow::TWindow(TComponent* Owner)
	: TForm(Owner)
{
	srand((unsigned)time(NULL));

	mBuffer = new Graphics::TBitmap();
	Window->DoubleBuffered = true;
	LoadBitmaps();
	ResetGame();
}

//	////////////////////////////////////////////////////////////////////////////

void	TWindow::LoadBitmaps()
{
	for(unsigned short i = 1; i < 16; i++)
	{
		App::mBitmaps[i] = new Graphics::TBitmap();
		AnsiString patch; patch.sprintf("gfx/%u.bmp", i);

		std::fstream file;
		file.open(patch.c_str());
		if(file)
		{
			file.close();
			const char* file_name = patch.c_str();
			App::mBitmaps[i]->LoadFromFile(file_name);
		}
	}

	App::mBitmaps[Game::BITMAP_PADDLE] = new Graphics::TBitmap();
	App::mBitmaps[Game::BITMAP_PADDLE]->LoadFromFile("gfx/Paddle.bmp");

	App::mBitmaps[Game::BITMAP_WELCOME] = new Graphics::TBitmap();
	App::mBitmaps[Game::BITMAP_WELCOME]->LoadFromFile("gfx/Welcome.bmp");

	App::mBitmaps[Game::BITMAP_GAME_OVER] = new Graphics::TBitmap();
	App::mBitmaps[Game::BITMAP_GAME_OVER]->LoadFromFile("gfx/Game_Over.bmp");

	App::mBitmaps[Game::BITMAP_WIN] = new Graphics::TBitmap();
	App::mBitmaps[Game::BITMAP_WIN]->LoadFromFile("gfx/Win.bmp");
	
	App::mBitmaps[Game::BITMAP_BACKGROUND] = new Graphics::TBitmap();
	App::mBitmaps[Game::BITMAP_BACKGROUND]->LoadFromFile("gfx/Background.bmp");
	
	for(unsigned int i = 0; i < PowerUp::COUNT; i++)
	{
		App::mBitmaps[Game::BITMAP_POWERUP + i] = new Graphics::TBitmap();
		AnsiString patch; patch.sprintf("gfx/power_%u.bmp", i);

		std::fstream file;
		file.open(patch.c_str());
		if(file)
		{
			file.close();
			const char* file_name = patch.c_str();
			App::mBitmaps[Game::BITMAP_POWERUP + i]->LoadFromFile(file_name);
		}
	}
}

//	////////////////////////////////////////////////////////////////////////////

void	TWindow::ResetGame()
{
	Game::bGameComplete	= false;
	Game::mCurrentLevel = 0;
	Game::mPoints		= 0;
	Game::mLives		= 3;
	Game::GameState		= Game::WELCOME;
	LoadNextLevel();
	ResetLevel();
}

void	TWindow::ResetLevel()
{
	Game::mBlocksToWin	= Game::mBlocksToDestroy;
	Game::bBallCatched	= true;
	Game::BallX			= Game::PadPosX * 0.5 - Game::BallRadius;
	Game::BallY			= Game::PadPosY - (2 * Game::BallRadius);
	Game::BallVelX		= 0;
	Game::BallVelY		= 0;
	Game::PadPosY		= PaintBox->Height;
	Game::PadPosX		= (PaintBox->Width - Game::PadWidth) / 2;
}

//	////////////////////////////////////////////////////////////////////////////

bool	TWindow::LoadLevel(const char* name)
{
	std::ifstream file;
	file.open(name);
	if(!file)
	{
		//	Plik nie istnieje, lub jest uszkodzony.
		return false;
	}

	const unsigned short buffer_size = 1024;
	char buffer[buffer_size];

	unsigned int j = 0;

	Game::mBlocksToWin = 0;

	while(!file.eof() && j < Game::LEVEL_HEIGHT)
	{
		file.getline(buffer, buffer_size);
		std::string line(buffer);
		
		for(unsigned int i = 0; i < Game::LEVEL_WIDTH; i++)
		{
			if(Game::mBlocks[i][j] != NULL)
				delete Game::mBlocks[i][j];

			switch(line[i])
			{
				//	Bitmap -1 -> dobierz automatycznie na podstawie hitcount
				//	w innym wypadku bierz podan¹
				case '1':	Game::mBlocks[i][j] = new Game::Block(1,	-1);	Game::mBlocksToWin++; break;
				case '2':	Game::mBlocks[i][j] = new Game::Block(2,	-1);	Game::mBlocksToWin++; break;
				case '3':	Game::mBlocks[i][j] = new Game::Block(3,	-1);	Game::mBlocksToWin++; break;
				case '4':	Game::mBlocks[i][j] = new Game::Block(4,	-1);	Game::mBlocksToWin++; break;
				case '5':	Game::mBlocks[i][j] = new Game::Block(5,	-1);	Game::mBlocksToWin++; break;
				case 'O':	Game::mBlocks[i][j] = new Game::Block(1,	 1);	Game::mBlocksToWin++; break;
				case '@':	Game::mBlocks[i][j] = new Game::Block(1,	 2);	Game::mBlocksToWin++; break;
				case '*':	Game::mBlocks[i][j] = new Game::Block(1,	 3);	Game::mBlocksToWin++; break;
				case '#':	Game::mBlocks[i][j] = new Game::Block(-1,	15);	break;
				default :	Game::mBlocks[i][j]	= new Game::Block(0,	-1);	break;
			}
		}
		j++;
	}
	Game::mBlocksToDestroy = Game::mBlocksToWin;
	return true;
}

bool	TWindow::LoadNextLevel()
{
	Game::mCurrentLevel++;

	AnsiString file_name;
	file_name.sprintf("levels/%u.lvl", Game::mCurrentLevel);
	Game::mLastLevel = std::string(file_name.c_str());

	return this->LoadLevel(Game::mLastLevel.c_str());
}

//	////////////////////////////////////////////////////////////////////////////

void __fastcall TWindow::mTimerTimer(TObject *Sender)
{
	ClearBuffer(clBlack);

	switch(Game::GameState)
	{
	case Game::WELCOME:
		UpdateWelcome();
		break;
	case Game::START:
		UpdateStart();
		break;
	case Game::PLAY:
		UpdatePlay();
		break;
	case Game::WIN:
		UpdateWin();
		break;
	case Game::LOSE:
		UpdateLose();
		break;
	case Game::GAME_OVER:
		UpdateGameOver();
		break;
	}

	SwapBuffers();
}
//	////////////////////////////////////////////////////////////////////////////

void	TWindow::ReleaseBall()
{
	Game::bBallCatched	= false;
	Game::BallVelY		= -7.5f;
	Game::BallVelX		= 0;
	Game::BallX			= Game::PadPosX + (Game::PadWidth * 0.5f) - (Game::BallRadius * 0.5f);// (Game::BallRadius*2);
	Game::BallY			= Game::PadPosY - (Game::BallRadius * 2) - 2;
}

void	TWindow::SpawnBonus(int X, int Y)
{
	int powerup_chance = rand() % 100 + 1;
	if(powerup_chance > Game::POWER_UP_CHANCE)
	{
		int powerup = rand() % PowerUp::COUNT;

		PowerUp::Bonus bon;

		bon.Type = powerup;
		bon.PosX = (X * Game::BLOCK_WIDTH) - Game::BLOCK_HEIGHT;
		bon.PosY =  Y * Game::BLOCK_HEIGHT;
		
		Game::mBonuses.push_back(bon);
	}
}

void	TWindow::UpdatePlay()
{
	std::stringstream ss;

	TCanvas* pCanvas = PaintBox->Canvas;

//	////////////////////////////////////////////////////////////////////////////

	if((GetKeyState(VK_LEFT)& 0x0080) == 0x0080)
	{
		Game::PadPosX -= Game::PadDelta;
	}

	if((GetKeyState(VK_RIGHT)& 0x0080) == 0x0080)
	{
		Game::PadPosX += Game::PadDelta;
	}

	if((GetKeyState(VK_ESCAPE)& 0x0080) == 0x0080)
	{
		exit(0);
	}

	if((GetKeyState(VK_SPACE)& 0x0080) == 0x0080)
	{
		if(Game::bBallCatched)
		{
			ReleaseBall();
		}
	}
	
	if((GetKeyState(VK_F1)	&	0x0080) == 0x0080)
	{
		App::bDebug = !App::bDebug;
	}
	
	if(App::bDebug)
	{
		if((GetKeyState(VK_F2) & 0x0080) == 0x0080)
		{	
			Game::GameState = Game::START;
			LoadLevel(Game::mLastLevel.c_str());
			ResetLevel();
			return;
		}
		
		if((GetKeyState(VK_F3) & 0x0080) == 0x0080)
		{
			if(!LoadNextLevel())
			{
				Game::GameState = Game::WIN;
			}
			else
			{
				ResetLevel();
				Game::GameState = Game::START;
				return;
			}
		}
		if((GetKeyState(VK_F4) & 0x0080) == 0x0080)
		{
			Game::PadWidth *= 2;
		}
		if((GetKeyState(VK_F5) & 0x0080) == 0x0080)
		{
			Game::PadWidth *= 0.5f;
		}	
		if((GetKeyState(VK_F6) & 0x0080) == 0x0080)
		{
			Game::PowerCatch = !Game::PowerCatch;
		}
		if((GetKeyState(VK_F7) & 0x0080) == 0x0080)
		{
			Game::BallVelX	*= 2.f;
			Game::BallVelY	*= 2.f;
		}
		if((GetKeyState(VK_F8) & 0x0080) == 0x0080)
		{
			Game::BallVelX	*= 0.5f;
			Game::BallVelY	*= 0.5f;
		}
	}

//	////////////////////////////////////////////////////////////////////////////

	if(Game::PadPosX + Game::PadWidth >= PaintBox->Width)
	{
		Game::PadPosX = PaintBox->Width - Game::PadWidth;
	}

	if(Game::PadPosX <= 0)
	{
		Game::PadPosX = 0;
	}

	if(!Game::bBallCatched)
	{
		for(unsigned int i = 0; i < Game::mBonuses.size(); i++)
		{
			Game::mBonuses[i].PosY += Game::POWER_UP_SPEED;
			if(Game::mBonuses[i].PosY > PaintBox->Height + Game::BLOCK_HEIGHT)
			{
				std::swap(Game::mBonuses[i], Game::mBonuses.back());
				Game::mBonuses.pop_back();
			}
		}

		float tmpx_1	= Game::BallVelX;
		Game::BallVelX	= Game::BallVelX > Game::BALL_MAX_SPEED ? Game::BALL_MAX_SPEED : (Game::BallVelX < Game::BALL_MIN_SPEED ? Game::BALL_MIN_SPEED : Game::BallVelX);
		float tmpy_1	= Game::BallVelY;
		Game::BallVelY	= Game::BallVelY > Game::BALL_MAX_SPEED ? Game::BALL_MAX_SPEED : (Game::BallVelY < Game::BALL_MIN_SPEED ? Game::BALL_MIN_SPEED : Game::BallVelY);

		float tmpx_2 = Game::BallVelX;
		float tmpy_2 = Game::BallVelY;

		Game::BallX	+=	Game::BallVelX;
		Game::BallY	+=	Game::BallVelY;
		
		if( (Game::BallX + (Game::BallRadius) > PaintBox->Width) ||
			(Game::BallX < 0) )
		{
			Game::BallX = Game::BallX < 0 ? (0) : (PaintBox->Width - (Game::BallRadius));
			Game::BallVelY += rand() % 10 - 5;
			Game::BallVelX *= -1.f;
		}
		if( (Game::BallY < 0) )
		{
			Game::BallVelY *= -1.f;
			Game::BallVelX += rand() % 10 - 5;
		}
		else if(Game::BallY > PaintBox->Height)
		{
			Game::GameState = Game::LOSE;
		}
		else
		{
			float ball_cur	[2]	= {Game::BallX + (0.5f * Game::BallRadius),	Game::BallY};									//	the current point
			float ball_next	[2]	= {ball_cur[0] + Game::BallVelX,	ball_cur[1] + (Game::BallRadius) + Game::BallVelY};		//	the next point

			Line2D	path		(	ball_cur[0], ball_cur[1], ball_next[0], ball_next[1]); 									//	the ball's path

			Line2D	path_left	(	Game::BallX, 					Game::BallY + (0.5f * Game::BallRadius),
									Game::BallX + Game::BallVelX,	Game::BallY + (0.5f * Game::BallRadius) + Game::BallVelY);
										
			Line2D	path_right	(	Game::BallX + (Game::BallRadius),					Game::BallY + (0.5f * Game::BallRadius),
									Game::BallX + (Game::BallRadius) + Game::BallVelX,	Game::BallY + (0.5f * Game::BallRadius) + Game::BallVelY);
			
			Line2D	path_down	(	Game::BallX + (0.5f * Game::BallRadius),					Game::BallY,
									Game::BallX + (0.5f * Game::BallRadius) + Game::BallVelX,	Game::BallY + Game::BallVelY);
			Line2D	path_up		(	Game::BallX + (0.5f * Game::BallRadius), 					Game::BallY + (Game::BallRadius),
									Game::BallX + (0.5f * Game::BallRadius) + Game::BallVelX,	Game::BallY + (Game::BallRadius) + Game::BallVelY);
			
			if(App::bDebug)
			{
				pCanvas->Pen->Color = clWhite;

				pCanvas->MoveTo(path.mBegin[0], 		path.mBegin[1]);
				pCanvas->LineTo(path.mEnd[0],			path.mEnd[1]);
			
				pCanvas->MoveTo(path_left.mBegin[0],	path_left.mBegin[1]);
				pCanvas->LineTo(path_left.mEnd[0],		path_left.mEnd[1]);
				
				pCanvas->MoveTo(path_right.mBegin[0],	path_right.mBegin[1]);
				pCanvas->LineTo(path_right.mEnd[0],		path_right.mEnd[1]);
				
				pCanvas->MoveTo(path_down.mBegin[0],	path_down.mBegin[1]);
				pCanvas->LineTo(path_down.mEnd[0],		path_down.mEnd[1]);
				
				pCanvas->MoveTo(path_up.mBegin[0],		path_up.mBegin[1]);
				pCanvas->LineTo(path_up.mEnd[0],		path_up.mEnd[1]);
			}
			
			Line2D leftWall		(	Game::PadPosX, Game::PadPosY - Game::PadHeight,
									Game::PadPosX, Game::PadPosY);
			Line2D topWall		(	Game::PadPosX, Game::PadPosY - Game::PadHeight,
									Game::PadPosX + Game::PadWidth, Game::PadPosY - Game::PadHeight);
			Line2D rightWall	(	Game::PadPosX + Game::PadWidth, Game::PadPosY - Game::PadHeight,
									Game::PadPosX + Game::PadWidth, Game::PadPosY);
			Line2D bottomWall	(	Game::PadPosX, Game::PadPosY,
									Game::PadPosX + Game::PadWidth, Game::PadPosY);

			for(unsigned int i = 0; i < Game::mBonuses.size(); i++)
			{
				Line2D leftEdge		(	Game::mBonuses[i].PosX,  Game::mBonuses[i].PosY,
										Game::mBonuses[i].PosX,  Game::mBonuses[i].PosY + Game::BLOCK_HEIGHT);
				Line2D rightEdge	(	Game::mBonuses[i].PosX + Game::BLOCK_HEIGHT, Game::mBonuses[i].PosY,
										Game::mBonuses[i].PosX + Game::BLOCK_HEIGHT, Game::mBonuses[i].PosY + Game::BLOCK_HEIGHT);
				Line2D topEdge		(	Game::mBonuses[i].PosX,  Game::mBonuses[i].PosY,
										Game::mBonuses[i].PosX + Game::BLOCK_HEIGHT, Game::mBonuses[i].PosY);
				Line2D bottomEdge	(	Game::mBonuses[i].PosX,  Game::mBonuses[i].PosY + Game::BLOCK_HEIGHT,
										Game::mBonuses[i].PosX + Game::BLOCK_HEIGHT, Game::mBonuses[i].PosY + Game::BLOCK_HEIGHT);
			
				if(	topWall.Colliding(leftEdge)		|| topWall.Colliding(rightEdge)		|| topWall.Colliding(bottomEdge)	|| topWall.Colliding(topEdge) ||
					leftWall.Colliding(leftEdge)	|| leftWall.Colliding(rightEdge)	|| leftWall.Colliding(bottomEdge)	|| leftWall.Colliding(topEdge) ||
					rightWall.Colliding(leftEdge)	|| rightWall.Colliding(rightEdge)	|| rightWall.Colliding(bottomEdge)	|| rightWall.Colliding(topEdge)	)
				{
					switch(Game::mBonuses[i].Type)
					{
						//	PlusLife,
					case PowerUp::PlusLife:
						Game::mLives++;
						break;
						//	MinusLife,
					case PowerUp::MinusLife:
						Game::mLives--;
						if(Game::mLives <= 0)
							Game::GameState = Game::LOSE;
						break;
						//	LargePad,
					case PowerUp::LargePad:
						Game::PadWidth *= 2;
						break;
						//	SmallPad,
					case PowerUp::SmallPad:
						Game::PadWidth *= 0.5f;
						break;
						//	Catching,
					case PowerUp::Catching:
						Game::PowerCatch = true;
						break;
					}
					std::swap(Game::mBonuses[i], Game::mBonuses.back());
					Game::mBonuses.pop_back();
				}
			}
			
			Game::PadWidth = Game::PadWidth > PaintBox->Width * 0.5f ? PaintBox->Width * 0.5f : Game::PadWidth < 25 ? 25 : Game::PadWidth;
			
			if(	intersects(	Game::BallX,	Game::BallY,	0.5f * Game::BallRadius,
							Game::PadPosX,	Game::PadWidth, Game::PadPosY,	Game::PadHeight)
			||	intersects(	Game::BallX +	Game::BallVelX, Game::BallY +	Game::BallVelY, 0.5f * Game::BallRadius,
							Game::PadPosX,	Game::PadWidth, Game::PadPosY,	Game::PadHeight)	)
			{
				if(Game::PowerCatch)
				{
					Game::bBallCatched = true;
				}
				else
				{
					float distance	= abs(	(Game::BallY	+ Game::BallRadius) - (Game::PadPosY - (0.5f * Game::PadHeight)));		//	y1 - y2
					
					float length	= sqrt( (Game::BallX	+ Game::BallRadius - Game::PadPosX - (Game::PadWidth * 0.5f))	*
											(Game::BallX	+ Game::BallRadius - Game::PadPosX - (Game::PadWidth * 0.5f))	+ 
											(Game::BallY	+ Game::BallRadius - Game::PadPosY - (Game::PadHeight * 0.5f))	*
											(Game::BallY	+ Game::BallRadius - Game::PadPosY - (Game::PadHeight * 0.5f))	);

					float angle		= sin(distance / length);

					if(angle > 0.f && angle < 3.15f)
					{
						//	1 polowka
						Game::BallVelX += (rand() % 10) - 5;
						Game::BallVelY *= -1;
					}
					else if(angle > 3.15f)
					{
						//	2 polowka
						Game::BallVelY += (rand() % 10) - 5;
						Game::BallVelX *= -1;
					}
				}
			}
			else if(	path.Colliding(leftWall)		||
						path_right.Colliding(leftWall)	||	path_left.Colliding(leftWall)	||
						path_up.Colliding(leftWall) 	||	path_down.Colliding(leftWall)	||
						path.Colliding(rightWall)		||
						path_right.Colliding(rightWall)	||	path_left.Colliding(rightWall)	||
						path_up.Colliding(rightWall)	||	path_down.Colliding(rightWall))
			
			{
				if(Game::PowerCatch){	Game::bBallCatched = true;		}
				else				{	Game::BallVelX *= -1;			}
			}
			
			else if(	path.Colliding(topWall)			||
						path_right.Colliding(topWall)	||	path_left.Colliding(topWall)	||
						path_up.Colliding(topWall)		||	path_down.Colliding(topWall)	||
						path.Colliding(bottomWall)		||	
						path_right.Colliding(bottomWall)||	path_left.Colliding(bottomWall)	||
						path_up.Colliding(bottomWall)	||	path_down.Colliding(bottomWall)	)
			
			{
				if(Game::PowerCatch){	Game::bBallCatched = true;		}
				else				{	Game::BallVelY *= -1;			}
			}

			else
			{
				for(unsigned int i = 0; i < Game::LEVEL_WIDTH; i++)
				{
					for(unsigned int j = 0; j < Game::LEVEL_HEIGHT; j++)
					{
						if(Game::mBlocks[i][j] != NULL)
							if(Game::mBlocks[i][j]->mHitCount != 0)
							{
							
								Line2D leftEdge		(	Game::BLOCK_WIDTH * i, Game::BLOCK_HEIGHT * j,
														Game::BLOCK_WIDTH * i, Game::BLOCK_HEIGHT * (j+1));
								Line2D topEdge		(	Game::BLOCK_WIDTH * i, Game::BLOCK_HEIGHT * j,
														Game::BLOCK_WIDTH * (i+1), Game::BLOCK_HEIGHT * j);
								Line2D rightEdge	(	Game::BLOCK_WIDTH * (i+1), Game::BLOCK_HEIGHT * j,
														Game::BLOCK_WIDTH * (i+1), Game::BLOCK_HEIGHT * (j+1));
								Line2D bottomEdge	(	Game::BLOCK_WIDTH * i, Game::BLOCK_HEIGHT * (j+1),
														Game::BLOCK_WIDTH * (i+1), Game::BLOCK_HEIGHT * (j+1));
								
								if(	intersects(	Game::BallX,	Game::BallY, 0.5f * Game::BallRadius,
												Game::BLOCK_WIDTH * i,	Game::BLOCK_WIDTH, Game::BLOCK_HEIGHT * j, Game::BLOCK_HEIGHT)
								||	intersects(	Game::BallX +	Game::BallVelX, Game::BallY + Game::BallVelY, 0.5f * Game::BallRadius,
												Game::BLOCK_WIDTH * i,	Game::BLOCK_WIDTH, Game::BLOCK_HEIGHT * j, Game::BLOCK_HEIGHT)	)
								{
									float distance	= abs(	(Game::BallY	+ Game::BallRadius) - (Game::PadPosY - (0.5f * Game::PadHeight)));		//	y1 - y2
				
									float length	= sqrt( (Game::BallX	+ Game::BallRadius - Game::PadPosX - (Game::PadWidth * 0.5f))	*
															(Game::BallX	+ Game::BallRadius - Game::PadPosX - (Game::PadWidth * 0.5f))	+ 
															(Game::BallY	+ Game::BallRadius - Game::PadPosY - (Game::PadHeight * 0.5f))	*
															(Game::BallY	+ Game::BallRadius - Game::PadPosY - (Game::PadHeight * 0.5f))	);

									float angle		= sin(distance / length);

									Game::BallX -= Game::BallVelX;
									Game::BallY -= Game::BallVelY;

									if(angle > 0.f && angle < 3.15f)
									{
										//	1 polowka
										Game::BallVelX += (rand() % 10) - 5;
										Game::BallVelY *= -1;
									}
									else if(angle > 3.15f)
									{
										//	2 polowka
										Game::BallVelY += (rand() % 10) - 5;
										Game::BallVelX *= -1;
									}
									
									if(Game::mBlocks[i][j]->mHitCount > 0)
									{
										SpawnBonus(i, j);
										Game::mPoints += 50;
									}
									
									Game::mBlocks[i][j]->mHitCount--;

									if(Game::mBlocks[i][j]->mHitCount == 0)
									{
										Game::mBlocksToWin--;
									}
								}
								else if(path.Colliding		(leftEdge)	||
										path_right.Colliding(leftEdge)	||	path_left.Colliding(leftEdge)	||
										path_up.Colliding	(leftEdge) 	||	path_down.Colliding(leftEdge)	||
										path.Colliding		(rightEdge)	||
										path_right.Colliding(rightEdge)	||	path_left.Colliding(rightEdge)	||
										path_up.Colliding	(rightEdge)	||	path_down.Colliding(rightEdge)	)
								{

									Game::BallX -= Game::BallVelX;
									Game::BallY -= Game::BallVelY;

									Game::BallVelY += (rand() % 10) - 5;
									Game::BallVelX *= -1;
									Game::Block& block = *Game::mBlocks[i][j];
									
									if(Game::mBlocks[i][j]->mHitCount > 0)
									{
										SpawnBonus(i, j);
										Game::mPoints += 50;
									}
									
									Game::mBlocks[i][j]->mHitCount--;

									if(Game::mBlocks[i][j]->mHitCount == 0)
									{
										Game::mBlocksToWin--;
									}
								}
								else if(path.Colliding		(topEdge)	||
										path_right.Colliding(topEdge)	||	path_left.Colliding(topEdge)	||
										path_up.Colliding	(topEdge)	||	path_down.Colliding(topEdge)	||
										path.Colliding		(bottomEdge)||	
										path_right.Colliding(bottomEdge)||	path_left.Colliding(bottomEdge)	||
										path_up.Colliding	(bottomEdge)||	path_down.Colliding(bottomEdge)	)
								{
									Game::BallX -= Game::BallVelX;
									Game::BallY -= Game::BallVelY;

									Game::BallVelX += (rand() % 10) - 5;
									Game::BallVelY *= -1;
									Game::Block& block = *Game::mBlocks[i][j];
									
									if(Game::mBlocks[i][j]->mHitCount > 0)
									{
										SpawnBonus(i, j);
										Game::mPoints += 50;
									}

									Game::mBlocks[i][j]->mHitCount--;

									if(Game::mBlocks[i][j]->mHitCount == 0)
									{
										Game::mBlocksToWin--;
									}
								}
							}
					}
				}
			}
		}
	}

	if(Game::mBlocksToWin == 0)
	{
		if(!LoadNextLevel())
		{
			Game::GameState = Game::WIN;
		}
		else
		{
			ResetLevel();
			Game::GameState = Game::START;
			return;
		}
	}

//	////////////////////////////////////////////////////////////////////////////

	OnDraw(0,0);

//	////////////////////////////////////////////////////////////////////////////
}

void TWindow::UpdateStart()
{
//	////////////////////////////////////////////////////////////////////////////

	Game::BallColor	= clRed;

	if((GetKeyState(VK_ESCAPE)& 0x0080) == 0x0080)
	{
		exit(0);
	}

//	////////////////////////////////////////////////////////////////////////////

	App::mUpdateCount++;

	float amount = float(App::mUpdateCount) / 100.f;

	TCanvas* pCanvas = mBuffer->Canvas;

	pCanvas->Brush->Color	= clBlack;
	pCanvas->Pen->Color		= clBlack;
	pCanvas->Rectangle(0, 0, PaintBox->Width, PaintBox->Height);

//	////////////////////////////////////////////////////////////////////////////

	// float test = Bounce::easeOut(amount, float(-PaintBox->Height), 0.f, 1.f);
	// App::ScreenY = test;
	App::ScreenY = Lerp(-PaintBox->Height, 0, amount);
	OnDraw(App::ScreenX, App::ScreenY);

//	////////////////////////////////////////////////////////////////////////////

	if(App::ScreenY == 0)
	{
		App::mUpdateCount = 0;
		Game::GameState = Game::PLAY;
	}

//	////////////////////////////////////////////////////////////////////////////
}

void TWindow::UpdateWelcome()
{
//	////////////////////////////////////////////////////////////////////////////

	TCanvas* pCanvas = mBuffer->Canvas;
	pCanvas->Draw(0,0,App::mBitmaps[Game::BITMAP_WELCOME]);

//	////////////////////////////////////////////////////////////////////////////
}

void TWindow::UpdateWin()
{
	if((GetKeyState(VK_ESCAPE)& 0x0080) == 0x0080)
	{
		exit(0);
	}
//	////////////////////////////////////////////////////////////////////////////

	TCanvas* pCanvas = mBuffer->Canvas;
	pCanvas->Draw(0,0,App::mBitmaps[Game::BITMAP_WIN]);

//	////////////////////////////////////////////////////////////////////////////
}

void TWindow::UpdateLose()
{
//	////////////////////////////////////////////////////////////////////////////

	OnDraw(0,0);

	App::mUpdateCount++;

//	////////////////////////////////////////////////////////////////////////////

	TCanvas* pCanvas = mBuffer->Canvas;
	TRect rect_left, rect_right;
	
	rect_left.left		= (PaintBox->Width * 0.5f * (float(App::mUpdateCount)/50.f));	// rosnie do 0.5
	rect_left.right		= PaintBox->Width * 0.5f - App::mBitmaps[Game::BITMAP_BACKGROUND]->Width;
	rect_left.top		= 0;
	rect_left.bottom	= App::mBitmaps[Game::BITMAP_BACKGROUND]->Height;
	
	rect_right.left		= PaintBox->Width - (PaintBox->Width * 0.5f * (float(App::mUpdateCount)/50.f));	// maleje do 0.5
	rect_right.right	= PaintBox->Width * 0.5f + App::mBitmaps[Game::BITMAP_BACKGROUND]->Width;
	rect_right.top		= 0;
	rect_right.bottom	= App::mBitmaps[Game::BITMAP_BACKGROUND]->Height;
	
	pCanvas->StretchDraw(rect_left,		App::mBitmaps[Game::BITMAP_BACKGROUND]);
	pCanvas->StretchDraw(rect_right,	App::mBitmaps[Game::BITMAP_BACKGROUND]);
		
//	////////////////////////////////////////////////////////////////////////////

	if(App::mUpdateCount == 50)
	{
		App::mUpdateCount = 0;
		if(Game::mLives > 0)
		{
			Game::PadWidth		= 100;
			Game::PowerCatch	= false;
			Game::GameState = Game::START;
			Game::mLives--;
			LoadLevel(Game::mLastLevel.c_str());
			ResetLevel();
		}
		else
		{
			Game::GameState = Game::GAME_OVER;
		}
	}

//	////////////////////////////////////////////////////////////////////////////
}

void TWindow::UpdateGameOver()
{
	if((GetKeyState(VK_ESCAPE)& 0x0080) == 0x0080)
	{
		exit(0);
	}

	if((GetKeyState(VK_SPACE)& 0x0080) == 0x0080)
	{
		ResetGame();
	}

//	////////////////////////////////////////////////////////////////////////////

	TCanvas* pCanvas = mBuffer->Canvas;
	pCanvas->Draw(0,0, App::mBitmaps[Game::BITMAP_GAME_OVER]);

//	////////////////////////////////////////////////////////////////////////////
}

void __fastcall TWindow::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Game::GameState == Game::WELCOME)
		Game::GameState = Game::START;
}

void	TWindow::OnDraw(int X, int Y)
{
	TCanvas* pCanvas = mBuffer->Canvas;

//	////////////////////////////////////////////////////////////////////////////

	for(unsigned int i = 0; i < Game::LEVEL_WIDTH; i++)
	{
		for(unsigned int j = 0; j < Game::LEVEL_HEIGHT; j++)
		{
			if(Game::mBlocks[i][j] != NULL)
				if(Game::mBlocks[i][j]->mHitCount != 0)
			{
				Game::Block& block		= *Game::mBlocks[i][j];

				TRect rect;
				rect.left	=	X +	(Game::BLOCK_WIDTH	* i);
				rect.top	=	Y +	(Game::BLOCK_HEIGHT	* j); 
				rect.right	=	X +	(Game::BLOCK_WIDTH	* (i+1));
				rect.bottom	=	Y +	(Game::BLOCK_HEIGHT	* (j+1));

				if(block.mBitmap == -1)
					if(block.mHitCount < 0)
						pCanvas->StretchDraw(rect, App::mBitmaps[15]);
					else
						pCanvas->StretchDraw(rect, App::mBitmaps[block.mHitCount]);
				else
					pCanvas->StretchDraw(rect, App::mBitmaps[block.mBitmap]);
			}
		}
	}

//	////////////////////////////////////////////////////////////////////////////

	for(unsigned int i = 0; i < Game::mBonuses.size(); i++)
	{
		TRect rect;
		rect.left	=	X +	Game::mBonuses[i].PosX;
		rect.top	=	Y +	Game::mBonuses[i].PosY; 
		rect.right	=	X +	Game::mBonuses[i].PosX + (Game::BLOCK_HEIGHT);
		rect.bottom	=	Y +	Game::mBonuses[i].PosY + (Game::BLOCK_HEIGHT);
		pCanvas->StretchDraw(rect, App::mBitmaps[Game::BITMAP_POWERUP + Game::mBonuses[i].Type]);
	}

//	////////////////////////////////////////////////////////////////////////////

	// pCanvas->Draw(X + Game::PadPosX, Y + Game::PadPosY - Game::PadHeight, App::mBitmaps[Game::BITMAP_PADDLE]);
	TRect pad_rect;

	pad_rect.left	=	X +	Game::PadPosX;
	pad_rect.top	=	Y + Game::PadPosY - Game::PadHeight; 
	pad_rect.right	=	X +	Game::PadPosX + Game::PadWidth;
	pad_rect.bottom	=	Y +	Game::PadPosY;

	pCanvas->StretchDraw(pad_rect, App::mBitmaps[Game::BITMAP_PADDLE]);

//	////////////////////////////////////////////////////////////////////////////

	pCanvas->Brush->Color	= clRed;
	pCanvas->Pen->Color		= Game::BallColor;

	if(Game::bBallCatched)
	{
		pCanvas->Ellipse(
			X + Game::PadPosX + (Game::PadWidth * 0.5f) - (Game::BallRadius * 0.5f),
			Y + Game::PadPosY - (Game::BallRadius * 2),
			X + Game::PadPosX + (Game::PadWidth * 0.5f) + (Game::BallRadius * 0.5f),
			Y + Game::PadPosY - (Game::BallRadius * 2) + Game::BallRadius);
	}
	else
	{
		pCanvas->Ellipse(X + Game::BallX, Y + Game::BallY, X + Game::BallX + Game::BallRadius, Y + Game::BallY + Game::BallRadius);
	}

	pCanvas->Font->Name		= "Arial";
	pCanvas->Font->Size		= 14;
	pCanvas->Font->Color	= clRed;
	pCanvas->Brush->Style	= bsClear;
	
	std::stringstream ss;
	
	ss << "Level: " << Game::mCurrentLevel;
	pCanvas->TextOutA((0.5f * PaintBox->Width) - (0.5f * (10 * ss.str().length() )), 0, ss.str().c_str());
	
	pCanvas->Font->Color	= clWhite;
	ss.str("");
	
	ss << "  Lives: " << Game::mLives + 1;
	pCanvas->TextOutA(0, PaintBox->Height - (pCanvas->Font->Size * 2), ss.str().c_str());
	
	ss.str("");
	
	ss << "Points: " << Game::mPoints;
	pCanvas->TextOutA(PaintBox->Width - (10 * ss.str().length()), PaintBox->Height - (pCanvas->Font->Size * 2), ss.str().c_str());

//	////////////////////////////////////////////////////////////////////////////
}
//	////////////////////////////////////////////////////////////////////////////
