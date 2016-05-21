#ifndef Unit1H
#define Unit1H
//	////////////////////////////////////////////////////////////////////////////
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Dialogs.hpp>
#include <ExtDlgs.hpp>
#include <ComCtrls.hpp>
//	////////////////////////////////////////////////////////////////////////////
namespace Bounce
{
	float easeIn (float t,float b , float c, float d);
	float easeOut(float t,float b , float c, float d);
	float easeInOut(float t,float b , float c, float d);
};
//	////////////////////////////////////////////////////////////////////////////
class TWindow : public TForm
{
__published:

	TPanel		*mMasterPanel;
	TPaintBox	*PaintBox;
	TTimer		*mTimer;
	TStatusBar	*mStatusBar;

	void __fastcall mTimerTimer(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);

public:
	__fastcall TWindow(TComponent* Owner);

	void	LoadBitmaps();
	
	void	ClearBuffer(TColor color);
	void	SwapBuffers();

	void	ResetGame();
	void	ResetLevel();
	
	void	SpawnBonus(int X, int Y);
	
	void	OnDraw(int X, int Y);

	bool	LoadLevel(const char*);
	bool	LoadNextLevel();

	void	ReleaseBall();
	
	void	UpdateWelcome();
	void	UpdateStart();
	void	UpdatePlay();
	void	UpdateWin();
	void	UpdateLose();
	void	UpdateGameOver();
};
//	////////////////////////////////////////////////////////////////////////////
extern PACKAGE TWindow *Window;
//	////////////////////////////////////////////////////////////////////////////
#endif

