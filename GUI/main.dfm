object Form1: TForm1
  Left = 470
  Top = 284
  Width = 384
  Height = 333
  Caption = 'Centrale'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 48
    Top = 24
    Width = 239
    Height = 107
    Caption = '0 KW'
    Font.Charset = ANSI_CHARSET
    Font.Color = clWindowText
    Font.Height = -96
    Font.Name = 'Arial'
    Font.Style = []
    ParentFont = False
  end
  object Label2: TLabel
    Left = 88
    Top = 0
    Width = 201
    Height = 13
    Caption = 'Ne pas fermer ce programme, merci'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clRed
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object LabelPort: TLabel
    Left = 144
    Top = 284
    Width = 87
    Height = 13
    Caption = 'Port du web: 8080'
  end
  object PB: TPaintBox
    Left = 4
    Top = 180
    Width = 365
    Height = 93
  end
  object Label3: TLabel
    Left = 152
    Top = 148
    Width = 73
    Height = 29
    Caption = 'Label3'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -24
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ParentFont = False
  end
  object Timer1: TTimer
    OnTimer = Timer1Timer
    Left = 320
    Top = 8
  end
end
