/*
  Influxdata Weather Station - Case
  An Open SCAD project for bonitoo.io by petr.kudibal
*/

//internal parameters in mm

s = 58.5; // width
d = 58.5; // length
v = 30;

module case()

difference() {

  union() {

    difference() {
      
      $fn = 50;
      minkowski() {

        translate([-d/2,-s/2,0])
        cube([d,s,v]);

        // side thickness
        cylinder(r=2,h=1);
      }

      translate([-d/2,-s/2,1.5])
      cube([d,s,v]);
   
      // window
      translate([-22.5,5,-0.7])
      cube([25,13,10]);
   
      // usb
      translate([-32,-0.5,24.5])
      cube([5,9,3.5]);
   
      // ventilation
      translate([21,-15,-3])
      cylinder(r=1.5,h=7);
      translate([25,-11,-3])
      cylinder(r=1.5,h=7);
      translate([17,-11,-3])
      cylinder(r=1.5,h=7);
      translate([25,-19,-3])
      cylinder(r=1.5,h=7);
      translate([17,-19,-3])
      cylinder(r=1.5,h=7);
  
      //text on front side
      mirror() {

//    translate([0,25,-1.2])//offset
//     linear_extrude(2)//thickness
//      scale([.6,.6,0])//velikost
//     text("bonitoo", font="Arial:style=Regular",valign="center", halign="center");

        // text
        translate([8,-14,-1.2])//offset 
        linear_extrude(2)//thickness
        scale([.6,.4,0])//velikost
        text("weather", font="Monotype Corsiva:style=Regular",valign="center", halign="center");

        translate([8,-24,-1.2])//offset
        linear_extrude(2)//thickness
        scale([.6,.4,0])//velikost
        text("station", font="Monotype Corsiva:style=Regular",valign="center", halign="center");
        translate([-22,18,-1.2])//offset
        linear_extrude(2)//thickness
        scale([.35,.35,0])//velikost
        text("SET", font="Arial:style=Regular",valign="center", halign="center");
        /*
        translate([-22,8,-1.2])//offset
        linear_extrude(2)//thickness
        scale([.35,.35,0])//size
        text("RS", font="Arial:style=Regular",valign="center", halign="center");
        */
      }
    }

    //pillars for boot and reset
    
    translate([11.5,18,1])
    cylinder(r=2.5,h=20,$fn=20);
/*
    translate([11.5,8,1])
    cylinder(r=2.5,h=20,$fn=20);
 */
  }
  
  // holes boot reset
  translate([11.5,18,-1])
  cylinder(r=1.5,h=25,$fn=20);
  /*
  translate([11.5,8,-1])
  cylinder(r=1.5,h=25,$fn=20);
  */
}
 
//pillars board

translate([13,26,1])
pillar1();
translate([-26,26,1])
pillar1();
translate([-26,-26,1])
pillar1();
translate([13,-26,1])
pillar1();

// harden pillars
translate([-30,-30,0])
cube([4,4,15]);

translate([-30,26,0])
cube([4,4,15]);

// pillar sensor
translate([22,1,1])
pillar2();

// pillars display
translate([-21,25,1.5])
pillar3();
translate([2.5,25.1,1.5])
pillar3();
translate([-21.5,0.9,1.5])
pillar3();
translate([2,1,1.5])
pillar3();
 
module pillar1() {
  cylinder(r=2.2,h=26,$fn=20);
  cylinder(r=1,h=29);
}

module pillar2() {
  cylinder(r=2.5,h=6,$fn=20);
  cylinder(r=1,h=9);
}

module pillar3() {
  cylinder(r=1.5,h=1,$fn=20);
  cylinder(r=0.8,h=4.5);
}

case();
