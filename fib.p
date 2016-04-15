program main;
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
   fib (5)
end.
