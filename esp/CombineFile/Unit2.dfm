object Form2: TForm2
  Left = 0
  Top = 0
  Caption = 'Combine files to my format'
  ClientHeight = 157
  ClientWidth = 539
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Button1: TButton
    Left = 456
    Top = 124
    Width = 75
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = Button1Click
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 539
    Height = 41
    Align = alTop
    TabOrder = 1
    object Label3: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 16
      Width = 69
      Height = 21
      Margins.Top = 15
      Align = alLeft
      Caption = 'Directory from'
      ExplicitHeight = 13
    end
    object edDir: TEdit
      Left = 87
      Top = 11
      Width = 358
      Height = 21
      TabOrder = 0
    end
    object Button2: TButton
      Left = 451
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Dir'
      TabOrder = 1
      OnClick = Button2Click
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 41
    Width = 539
    Height = 41
    Align = alTop
    TabOrder = 2
    object Label1: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 16
      Width = 29
      Height = 21
      Margins.Top = 15
      Align = alLeft
      Caption = 'File to'
      ExplicitHeight = 13
    end
    object edFile: TEdit
      Left = 87
      Top = 11
      Width = 358
      Height = 21
      TabOrder = 0
    end
    object Button3: TButton
      Left = 451
      Top = 9
      Width = 75
      Height = 25
      Caption = 'File'
      TabOrder = 1
      OnClick = Button3Click
    end
  end
  object Panel3: TPanel
    Left = 0
    Top = 82
    Width = 539
    Height = 41
    Align = alTop
    TabOrder = 3
    object Label2: TLabel
      Left = 12
      Top = 9
      Width = 45
      Height = 13
      Caption = 'Size flash'
    end
    object Label4: TLabel
      Left = 155
      Top = 9
      Width = 102
      Height = 13
      Caption = 'Size partition storage'
    end
    object Label5: TLabel
      Left = 335
      Top = 9
      Width = 60
      Height = 13
      Caption = 'K - Kb M -Mb'
    end
    object edSize: TEdit
      Left = 63
      Top = 6
      Width = 82
      Height = 21
      TabOrder = 0
    end
    object edPart: TEdit
      Left = 263
      Top = 6
      Width = 66
      Height = 21
      TabOrder = 1
    end
    object Button4: TButton
      Left = 451
      Top = 6
      Width = 75
      Height = 25
      Caption = 'cmd flash'
      TabOrder = 2
      OnClick = Button1Click
    end
  end
  object OpenDialog1: TOpenDialog
    Left = 192
    Top = 16
  end
  object SaveDialog1: TSaveDialog
    Left = 256
    Top = 16
  end
end
