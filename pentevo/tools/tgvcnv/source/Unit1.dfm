object Form1: TForm1
  Left = 794
  Top = 139
  Width = 465
  Height = 224
  Caption = 'TGV Convertor'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object lbl1: TLabel
    Left = 19
    Top = 144
    Width = 45
    Height = 21
  end
  object lbl2: TLabel
    Left = 16
    Top = 168
    Width = 49
    Height = 13
  end
  object lbl3: TLabel
    Left = 8
    Top = 56
    Width = 50
    Height = 13
    Caption = 'Audio File:'
  end
  object lbl4: TLabel
    Left = 8
    Top = 104
    Width = 62
    Height = 13
    Caption = 'Save To File:'
  end
  object lbl5: TLabel
    Left = 8
    Top = 8
    Width = 192
    Height = 13
    Caption = 'DIR With TGA Images (8bit 256x2-192):'
  end
  object edt1: TEdit
    Left = 8
    Top = 24
    Width = 393
    Height = 21
    TabOrder = 0
  end
  object edt2: TEdit
    Left = 8
    Top = 120
    Width = 393
    Height = 21
    TabOrder = 1
  end
  object btn1: TButton
    Left = 408
    Top = 24
    Width = 33
    Height = 21
    Caption = '...'
    TabOrder = 2
    OnClick = btn1Click
  end
  object btn2: TButton
    Left = 408
    Top = 120
    Width = 33
    Height = 21
    Caption = '...'
    TabOrder = 3
    OnClick = btn2Click
  end
  object btn3: TButton
    Left = 304
    Top = 152
    Width = 137
    Height = 25
    Caption = 'START CONVERTING'
    TabOrder = 4
    OnClick = btn3Click
  end
  object edt3: TEdit
    Left = 8
    Top = 72
    Width = 393
    Height = 21
    TabOrder = 5
  end
  object btn4: TButton
    Left = 408
    Top = 72
    Width = 33
    Height = 21
    Caption = '...'
    TabOrder = 6
    OnClick = btn4Click
  end
  object dlgOpen1: TOpenDialog
    Left = 264
    Top = 152
  end
  object dlgSave1: TSaveDialog
    DefaultExt = 'tgv'
    Left = 200
    Top = 152
  end
  object dlgOpen2: TOpenDialog
    Left = 232
    Top = 152
  end
end
