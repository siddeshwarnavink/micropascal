program Adder;

var
  a: integer;
  b: integer;
  c: integer;
  d: real;
  e: real;
  f: real;
begin
  a := 10;
  a := 12;
  b := 15;
  c := a + b;

  d := 23.5;
  e := 35.4;
  f := d + e;

  writeln('%d', c);
  writeln('%f', f);
end.
