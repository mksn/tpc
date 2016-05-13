program main;
var i;

function fib (n) : foo;
begin
   if  n = 0
   then
      fib := 0
   else
      if n = 1 then
         fib := 1
      else fib := fib (n-1) + fib (n-2);
end;

begin
  i := 1;
  repeat
    WriteLn (i);
   WriteLn (fib (i));
   i := i + 1
  until i = 10;
  i := 1;
  while i<10 do
    begin
      WriteLn (i);
      WriteLn (fib (i));
      i := i + 1
    end
end.
