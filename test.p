program  mika;
var
   a;
procedure b (var i, c);
var
   g,h,i;
begin
   g := 1;
   while g < 10 do
      begin
         h := g*g;
      end;
end;
function c (var k, L) : integer;
var
  g, h;
begin
   h := 2;
   for g := 1 to 5 do
      h := g*h;
  c := h;
end;

begin
   a := -42 + 23 / 5 - 8; b (123, a);
   c (3, 3)
end.
