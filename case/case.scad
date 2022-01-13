//Krabicka Ivan

//vnitřní rozměry v mm
s = 58.5;//sirka
d = 58.5;//delka
v = 29;//vyska
use <logoInfluxdata.scad>; //use logo

module krabicka()

//plášť krabičky
difference()
{
union()
{
difference()
{$fn = 50;
 minkowski()   
  {translate([-d/2,-s/2,0])
    cube([d,s,v]);
     cylinder(r=2,h=1);}//tloušťka stěny
    translate([-d/2,-s/2,1.5])
   cube([d,s,v]);
     
     translate([-20.5,2,-0.7])//okénko
     cube([25,14,10]);
     
  translate([-32,-0.8,22.3])//díra na usb
     cube([5,9.5,3.8]); 
     
 translate([28,-25,5])//díry větraní
     vetr();
 
  translate([28,-25,8])
     vetr();
 
 translate([28,-25,11])
     vetr(); 
   
      translate([28,-15,5])
     vetr();
 
  translate([28,-15,8])
     vetr();
 
 translate([28,-15,11])
     vetr();  
 
 textPosY = -14;
 textScale = 0.73;

 mirror([1,0,0])//tisk na spodní stranu   
 {      
   translate([-10,textPosY,-.3]) //logo influx
    rotate(180-17)
    logo_influxdata(12);//velikost loga
}    
 translate([0,29,28])//zámky
  cube([12,3,3]);
 translate([-6,-32,28])
 cube([8,3,3]);
 translate([-30,-1,27])
 cube([2,2,2]);
translate([28,-1,27])
 cube([2,2,2]);
}
 translate([12,19,1])//sloupek boot
  cylinder(r=3,h=19,$fn=20);
 }  
  translate([12,19,-1])//díra boot
  cylinder(r=2,h=25,$fn=20);
 }
 //vestavby
 translate([13,26,1])//sloupky deska
  sloupek1();
 translate([-26,26,1])
  sloupek1();
 translate([-26,-26,1])
  sloupek1();
 translate([13,-26,1]) 
  sloupek1();
 translate([-30,-30,0])//opory sloupků
  cube([4,4,15]);
 translate([-30,26,0]) 
  cube([4,4,15]);
 
 translate([18,-28,1]) //opěrka čidlo
 dist1();
 
 translate([-20,22.5,1])//sloupky display
  sloupek3();
translate([3.2,22.5,1])
  sloupek3();
 translate([-20,-1,1]) 
  sloupek3();
 translate([3.2,-1,1])
  sloupek3();

translate([-21,23,1])//opěrky display
  dist2();
translate([1.5,23,1])
  dist2();
 translate([-21,-2.5,1]) 
  dist2();
 translate([1.5,-2.5,1])
  dist2();
 
 module sloupek1()
{
cylinder(r=2.2,h=24,$fn=20);
cylinder(r=1.1,h=26.5);
 } 
 module dist1()
{
cube([3,18,10]);
  }  
 module sloupek3()
{
cylinder(r=0.7,h=5,$fn=20);
 } 
 module dist2()
{
cube([3,1,2]);
 } 
 module vetr()
{
 cube([5,7,1.5]);
 }
 krabicka();