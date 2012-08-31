unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Grids;

type
  TForm1 = class(TForm)
    edt1: TEdit;
    edt2: TEdit;
    btn1: TButton;
    btn2: TButton;
    btn3: TButton;
    dlgOpen1: TOpenDialog;
    dlgSave1: TSaveDialog;
    lbl1: TLabel;
    lbl2: TLabel;
    edt3: TEdit;
    lbl3: TLabel;
    btn4: TButton;
    lbl4: TLabel;
    lbl5: TLabel;
    dlgOpen2: TOpenDialog;
    procedure btn3Click(Sender: TObject);
    procedure btn4Click(Sender: TObject);
    procedure btn1Click(Sender: TObject);
    procedure btn2Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;
  function nulz(ss:integer):string;
implementation

{$R *.dfm}

procedure TForm1.btn3Click(Sender: TObject);
var
        File1: File of byte;
        File2: File of byte;

        count: Integer;
    l,i,j,k,m: Integer;
            x: Word;

        r,g,b: byte;
        pL,pH: byte;
        bb,b2: byte;

         baze: string;
     reserved: array[0..511] of byte;
        mass1: array[0..17] of byte;
        buff1: array[0..137152] of Byte;
begin

  count:=0;

  //edt1.text:='i:\fraps\vidd';
  //edt2.text:='i:\fraps\vidd\herr.tgv';
  //edt3.text:='i:\fraps\vidd\audio.mp3';

  SetCurrentDir(edt1.text);
  baze:='i';

  AssignFile(File2, edt2.Text);
  Rewrite(File2);
//-----------------------------------------------------------------------------
  lbl2.Caption:=edt1.Text + baze + nulz(count);
 
//----Header:------------
  if FileExists(baze + nulz(count))
  then begin
       AssignFile(File1, baze + nulz(count));
       Reset(file1);

       BlockRead(file1, mass1, 18);
       CloseFile(file1);

       j:=mass1[14]+mass1[15]*256;
       l:=j*256;
       end;

  BlockWrite(file2, reserved, 512);
//----Reading Audio:-----
  k:=0;
  if FileExists(edt3.text)
  then begin
       i:=0;
       AssignFile(File1, edt3.Text);
       Reset(file1);
       while not Eof(file1)
       do begin
          read(file1, r);
          write(file2, r);
          Inc(i);
          end;

       r:=0;
       m:=(512-(i mod 512));
       if m<>512 then for j:=1 to m
                      do begin
                         write(file2, r);
                         end;

       k:=(i div 512)+1-(1*(m div 512));
       end;
//-----------------------
  while FileExists(baze + nulz(count))
  do begin
     AssignFile(File1, baze + nulz(count));
     Reset(File1);
//----Header:------------
     BlockRead(File1, buff1, 18);
//----Pallete:-----------
     for i:=0 to 255
     do begin
        Read(File1, b);
        Read(File1, g);
        Read(File1, r);

        x:=Trunc(r/10)*1024 + Trunc(g/10)*32 + Trunc(b/10);

        pL:=Trunc(x);
        pH:=Trunc(x/256);

        Write(File2, pL);
        Write(File2, pH);
        end;
//----Data:--------------
     BlockRead(file1, buff1, l);
     BlockWrite(file2, buff1, l);
//-----------------------
     CloseFile(File1);
     Inc(count);
  end;
  CloseFile(File2);
  Reset(file2);
//----Generating Header:-

//+0(16):
       BlockWrite(File2, 'TGA Video v0.2  ', 16);
//+16(16):
       pL:=Trunc(k);
       pH:=Trunc(k/256);
       Write(file2, pL);
       Write(file2, pH);
       BlockWrite(file2, reserved, 14);
//+32(2):
       Write(file2, mass1[14]);
       Write(file2, mass1[15]);
//+34(14):
//BlockWrite(file2, reserved, 14);
//+48(464):
//BlockWrite(file2, reserved, 464);
//-----------------------------------------------------------------------------
  CloseFile(File2);
  lbl1.Caption:='done: ' + IntToStr(count);
end;

function nulz(ss: integer):string;
begin
  if ss<10 then result:='0000' + IntToStr(ss) + '.tga'
  else if ss<100 then result:='000' + IntToStr(ss) + '.tga'
       else if ss<1000 then result:='00' + IntToStr(ss) + '.tga'
            else if ss<10000 then result:='0' + IntToStr(ss) + '.tga'
                 else if ss<100000 then result:=IntToStr(ss) + '.tga';
end;

procedure TForm1.btn4Click(Sender: TObject);
begin
 dlgOpen1.Filter:='MP3 Files|*.mp3';

 if dlgOpen1.Execute
 then edt3.text:=dlgOpen1.FileName;
end;

procedure TForm1.btn1Click(Sender: TObject);
begin
 dlgOpen2.Filter:='TGA 8bit|*.tga';

 if dlgOpen2.Execute
 then edt1.text:=dlgOpen2.FileName;
 edt1.text:=ExtractFilePath(edt1.text);
end;

procedure TForm1.btn2Click(Sender: TObject);
begin
 dlgSave1.Filter:='TGV Files|*.tgv';
 if dlgSave1.Execute
 then edt2.text:=dlgSave1.FileName;
end;

end.
