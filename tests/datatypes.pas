program Adder;

var
  a: integer;
  b: integer;
  c: integer;
  d: real;
  e: real;
  f: real;
  name: string[16];
  foo: string;
begin
  a := 10;
  a := a + 2;
  b := 15;
  b := 20;
  c := a + b;

  d := 23.5;
  e := 35.4;
  f := d + e;

  writeln('%d', c);
  writeln('%f', f);

  name := 'John doe';

  writeln(name);

  foo := name;
  name := 'Sid';

  writeln(foo);
  writeln(name);
end.
