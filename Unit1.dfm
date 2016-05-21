object Window: TWindow
  Left = 288
  Top = 116
  BorderStyle = bsSingle
  Caption = 'Window'
  ClientHeight = 661
  ClientWidth = 770
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object mMasterPanel: TPanel
    Left = 0
    Top = 0
    Width = 770
    Height = 642
    Align = alClient
    TabOrder = 0
    object PaintBox: TPaintBox
      Left = 1
      Top = 1
      Width = 768
      Height = 640
      Align = alClient
    end
  end
  object mStatusBar: TStatusBar
    Left = 0
    Top = 642
    Width = 770
    Height = 19
    Panels = <>
    SimplePanel = False
  end
  object mTimer: TTimer
    Interval = 30
    OnTimer = mTimerTimer
    Left = 8
    Top = 8
  end
end
