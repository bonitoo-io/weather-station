/* 
 Logo Influxdata, 3D OpenSCAD model
 author: @bonitoo.io
 MIT License
*/

/* use this module and rotate the logo by 17 degrees */

module logo_influxdata(size) {

  difference() //the basic 6-gon
  {
     translate([0,0,-1])
     cylinder(h=2, r=size*1.2-0.001*size*size, $fn = 6); 
     translate([0, 0, -2])
     cylinder(h=4, r=size, $fn = 6);
  }

  module rectangle()
  {
    difference()
     {
       /* height is the longest side of the half of the  
          isosceles triangle (size*0.5)/x = cos(30)
       */
       height = (size*0.5)/cos(30);

       translate([0,height,0])
         cube([size, height+0.1*size, 2], center=true);

       translate([0,height,0])
         cube([size-0.1*size, height , 3], center=true);     
     }
  }

  rectangle();

  rotate(120)
  rectangle();

  rotate(240)
  rectangle();

}

logo_influxdata(15);
