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
  age: integer;
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

  age := 23;

  if true then
  begin
    writeln('Welcome to the club!');
    writeln('Have a drink!');

    if true then
      writeln('You are still young in some countries.');
  end
  else
  begin
    writeln('Stay away kid!');
    writeln('You are too young in all countries.');
  end;

  writeln('Have a nice day!');
end.
