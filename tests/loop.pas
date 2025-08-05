program Fibonacci;

var
  a: integer;
  b: integer;
  c: integer;
  i: integer;
begin
  a := 0;
  b := 1;
  i := 0;

  while i < 10 do
  begin
    writeln('%d', a);
    c := a + b;
    a := b;
    b := c;
    i := i + 1;
  end;
end.
