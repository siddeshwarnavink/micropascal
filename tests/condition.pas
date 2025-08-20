program Condition;

var
  age: integer;
begin
  age := 23;

  if age > 18 then
  begin
    writeln('Welcome to the club!');
    writeln('Have a drink!');
    if age > 21 then
      writeln('You are still young in some places.');
  end
  else
  begin
    writeln('Stay away kid!');
  end;

  writeln('Have a nice day!');
end.
