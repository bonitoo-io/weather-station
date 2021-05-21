//open weather case

//internal scales in mm

s = 58.5;       //width
d = 58.5;       //length
v = 7;          //height

module case()
difference()
{
    $fn = 50;
    minkowski()
    {
        translate([-d / 2, -s / 2, 0])
        cube([d, s, v]);
        //side thickness
        cylinder(r = 2, h = 1);
    }
    translate([-d / 2, -s / 2, 1.5])
    cube([d, s, v]);

    translate([-13, 1, -0.7])//window
    cube([25, 13, 10]);

    translate([-32, 2, 3])//usb hole
    cube([5, 8, 3]);

    translate([22, -15, -3]) // air
    cylinder(r = 1, h = 7);
    translate([24, -18, -3])
    cylinder(r = 1, h = 7);
    translate([20, -18, -3])
    cylinder(r = 1, h = 7);

    mirror()//
    {
        translate([0, -7, -1])//shift letters
        linear_extrude(1)//font thickness
        scale([.4, .35, 0])//font size
        text("influxdata.com", font = "Arial:style=Regular", valign = "center", halign = "center");
    }
}
module pillar1()
{
    cylinder(r = 2.5, h = 5, $fn = 20);
    cylinder(r = 1, h = 7);
}
module pillar2()
{
    cylinder(r = 2.5, h = 5, $fn = 20);
    cylinder(r = 1, h = 7);
}
module pillar3()
{
    cylinder(r = 2.5, h = 2.5, $fn = 20);
    cylinder(r = 1, h = 5);
}
translate([13, 26, 0])//pillars, wall
pillar1();
translate([-26, 26, 0])
pillar1();
translate([-26, -26, 0])
pillar1();
translate([13, -26, 0])
pillar1();

translate([22, 1, 1.5]) //pillar sensor
pillar2();

translate([-11.5, 21, 0])//pillar display
pillar3();
translate([11.5, 21, 0])
pillar3();
translate([-11.5, -3, 0])
pillar3();
translate([11.5, -3, 0])
pillar3();
  
case ();
