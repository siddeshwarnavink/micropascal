program Loop;

var
  i: integer;
begin
  i := 0;

  while i < 10 do
  begin
    write(i, '-');
    i := i + 1;
  end;

  write('\n');
end.
