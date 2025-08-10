program Adder;

var
  a: integer;
  b: integer;
  c: integer;
  d: integer;
  e: real;
  f: real;
  g: boolean;
  h: boolean;
begin
  a := 1+2*3+4;
  a := 12;
  b := 15;
  c := 20;
  d := a + b + c;
  writeln('%d', d);
  f := 35.35;
  e := 34.34;
  f := f + e;
  writeln('%f', f);
  g := true;
  g := !g;
  writeln('%d', g);
end.
