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
  isTrue: boolean;
begin
  a := 10;
  a := a + 2;
  b := 15;
  b := 20;
  c := a + b;

  d := 23.5;
  e := 35.4;
  f := d + e;

  isTrue := false;
  isTrue := !isTrue;

  writeln(c, ' ', f);

  name := 'John doe';

  writeln('Hello ', name);

  foo := name;
  name := 'Sid';

  writeln(foo, name);
end.
