program Condition;

var
  age: integer;
begin
  age := 23;

  if age > 18 then
  begin
    writeln('Welcome to the club!');
    writeln('Have a drink!');
  end
  else
    writeln('Stay away kid!');

  writeln('Have a nice day!');
end.
