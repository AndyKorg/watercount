unit Unit2;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes,
  System.IOUtils, System.iniFiles,
  Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls, Vcl.ExtCtrls;

type
  TForm2 = class(TForm)
    Button1: TButton;
    OpenDialog1: TOpenDialog;
    SaveDialog1: TSaveDialog;
    Panel1: TPanel;
    Label3: TLabel;
    edDir: TEdit;
    Button2: TButton;
    Panel2: TPanel;
    Label1: TLabel;
    edFile: TEdit;
    Button3: TButton;
    Panel3: TPanel;
    edSize: TEdit;
    Label2: TLabel;
    Label4: TLabel;
    edPart: TEdit;
    Label5: TLabel;
    Button4: TButton;
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    procedure SaveIni;
    procedure LoadIni;
  end;

var
  Form2: TForm2;

implementation

{$R *.dfm}

procedure TForm2.Button1Click(Sender: TObject);
 var
   SR: TSearchRec;
   FIn, Fout : TFileStream;
   fnAnsi: AnsiString;
   i, count, magicLen: Integer;
const
   MAGIC = 'MYFS1';
begin
if (edFile.Text <> '') and (edDir.Text <> '') then
  begin
  if FindFirst(edDir.Text + '*.*', faAnyFile, SR) = 0 then
     begin
     Fout := TFileStream.Create(edFile.Text, fmCreate);
     try
        count := 0;
        fnAnsi := Utf8ToAnsi(MAGIC);                                        //Уникальная метка файловой системы
        magicLen := Length(fnAnsi)*SizeOf(fnAnsi[1]);
        Fout.Write(fnAnsi[1], magicLen);
        Fout.Write(count, 1);                                               //Количество файлов
        repeat
          if (SR.Attr <> faDirectory) then
             begin
             fnAnsi := Utf8ToAnsi(SR.Name);                                 //Имя файла
             i := Length(fnAnsi)+1+SR.Size;                                 //Длина имени+1 символ конца имени и сам файла
             Fout.Write(i, SizeOf(integer));                                //длина массива с файлом и именем
             Fout.Write(fnAnsi[1], Length(fnAnsi)*SizeOf(fnAnsi[1]));       //Имя файла
             i := 0;
             Fout.Write(i, 1);                                              //Конец имени
             FIn := TFileStream.Create(edDir.Text+SR.Name, fmOpenRead);     //Сам файл
             try
               Fout.CopyFrom(FIn, SR.Size);
               count := count+1;
               finally
                 FreeAndNil(FIn);
               end;
             end;
        until FindNext(SR) <> 0;
        Fout.Seek(magicLen, soBeginning);
        Fout.Write(count, 1);
        FindClose(SR);
        ShowMessage('ok, write '+IntToStr(count)+' files');
      finally
        Fout.Free;
      end;
     end;
  end;
end;

procedure TForm2.SaveIni;
var
  MyIni: TIniFile;
begin
MyIni := TIniFile.Create(ExtractFileDir(Application.ExeName)+TPath.DirectorySeparatorChar+'Combine.ini');
try
  MyIni.WriteString('setting', 'dir', edDir.Text);
  MyIni.WriteString('setting', 'file', edFile.Text);
  MyIni.WriteString('setting', 'size', edSize.Text);
  MyIni.WriteString('setting', 'part', edPart.Text);
  finally
    MyIni.Free;
  end;
end;

procedure TForm2.LoadIni;
var
  MyIni: TIniFile;
begin
MyIni := TIniFile.Create(ExtractFileDir(Application.ExeName)+TPath.DirectorySeparatorChar+'Combine.ini');
try
  edDir.Text := MyIni.ReadString('setting', 'dir', '');
  edFile.Text := MyIni.ReadString('setting', 'file', '');
  edSize.Text := MyIni.ReadString('setting', 'size', '512K');
  edPart.Text := MyIni.ReadString('setting', 'part', '');
  finally
    MyIni.Free;
  end;
end;

procedure TForm2.Button2Click(Sender: TObject);
begin
if OpenDialog1.Execute then
   begin
   edDir.Text := ExtractFileDir(OpenDialog1.FileName)+TPath.DirectorySeparatorChar;
   SaveIni;
   end;
end;

procedure TForm2.Button3Click(Sender: TObject);
begin
if SaveDialog1.Execute then
   begin
   edFile.Text := SaveDialog1.FileName;
   SaveIni;
   end;
end;

procedure TForm2.FormShow(Sender: TObject);
begin
LoadIni;
end;

end.
