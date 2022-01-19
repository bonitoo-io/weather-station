// case Roman - WIP!

// vnitřní rozměry v mm
w = 58.5;                 // Width
d = 58.5;                 // depth
h = 29;                   // height
use<logoInfluxdata.scad>; // use logo

module
shell(){
    // plášť krabičky
    difference()
{
    union()
    {
        difference()
        {
            $fn = 50;
            minkowski()
            {
                translate([ -d / 2, -w / 2, 0 ]) cube([ w, d, h ]);
                cylinder(r = 2, h = 1);
            } // tloušťka stěny

            minkowski()
            {
                translate([ -d / 2 + 2, -w / 2 + 2, 1.5 ])
                    cube([ w - 4, d - 4, h ]);
                cylinder(r = 2, h = 1);
            }

            translate([ -20.5, 2, -0.7 ]) // okénko
                cube([ 25, 14, 10 ]);

            translate([ -32, -0.8, 22.3 ]) // díra na usb
                cube([ 5, 9.5, 3.8 ]);

            translate([ 28, -25, 5 ]) // díry větraní
                vetr();

            translate([ 28, -25, 8 ]) vetr();

            translate([ 28, -25, 11 ]) vetr();

            translate([ 28, -15, 5 ]) vetr();

            translate([ 28, -15, 8 ]) vetr();

            translate([ 28, -15, 11 ]) vetr();

            translate([ 18, 27, 5 ]) vetr();

            translate([ 18, 27, 8 ]) vetr();

            translate([ 18, 27, 11 ]) vetr();

            textPosY = -14;
            textScale = 0.73;

            mirror([ 1, 0, 0 ]) // tisk na spodní stranu
            {
                translate([ -10, textPosY, -.3 ])         // logo influx
                    rotate(180 - 17) logo_influxdata(12); // velikost loga
            }
            translate([ 0, 29, 28 ]) // zámky
                cube([ 12, 3, 3 ]);
            translate([ -6, -32, 28 ]) cube([ 8, 3, 3 ]);
            translate([ -30, -1, 27 ]) cube([ 2, 2, 2 ]);
            translate([ 28, -1, 27 ]) cube([ 2, 2, 2 ]);
        }
    }

    translate([ 12, 19, -1 ]) // díra boot
        cylinder(r = 4, h = 25, $fn = 20); // r = 4.7

}

difference(){
        translate([ 12, 19, 0 ]) // sloupek boot
        button_socket();

  translate([0, 10, 0]) {
  cube([7.5,20,20]);
  }
}
}
module
sloupek_deska()
{
    cylinder(r = 2.2, h = 24, $fn = 20);
    cylinder(r = 1.1, h = 26.5);
}
module
senzor_stena()
{
    cube([ 3, 18, 10 ]);
}
module
vetr()
{
    cube([ 7, 7, 1.5 ]);
}

module
ws_box()
{
    shell();

    difference(){
      translate([ -7.5, 11, 1 ]) display_socket();

      translate([ 12,19,0 ]) 
      cylinder(20,5,6);
    }

    // vestavby
    difference(){
    translate([ 13, 26, 1 ]) // sloupky deska
        sloupek_deska();
    translate([ 12,19,5 ]) 
      cylinder(15,5.4,4);

    }
    translate([ -26, 26, 1 ]) sloupek_deska();
    translate([ -26, -26, 1 ]) sloupek_deska();
    translate([ 13, -26, 1 ]) sloupek_deska();
    translate([ -30, -30, 0 ]) // opory sloupků
        cube([ 4, 4, 15 ]);
    translate([ -30, 26, 0 ]) cube([ 4, 4, 15 ]);

    translate([ 18, -28, 1 ]) // opěrka čidlo
        senzor_stena();
}


module
xy_center_cube(sizes)
{
    translate([ -sizes[0] / 2, -sizes[1] / 2, 0 ]) cube(sizes);
}

module
display_socket()
{
    top_scale = 0.07;

    difference()
    {
        // xy_center_cube([29+2,28+2,10]);
        linear_extrude(10, scale = [ 1 + top_scale, 1 + top_scale ])
            square([ 29 + 2.5, 28 + 2.5 ], true);

        translate([ 0, 0, 2 ])
            linear_extrude(10, true, scale = [ 1 + top_scale, 1 + top_scale ])
                square([ 29, 28 ], true);
        translate([ 0, 0, -1 ]) xy_center_cube([ 27, 26, 2 + 2 ]);

        xy_center_cube([ 20, 40, 15 ]);
        xy_center_cube([ 40, 20, 15 ]);
    }
}

module
_display_socket_print_test(){ intersection(){

    union(){ translate([ -1, -9, 0 ]) cube([ 10, 10, 1 ]);
ws_box();
}
translate([ -24.5, -5, 0 ]) cube([ 34, 33, 12 ]);
}
}
*
_display_socket_print_test();


button_height = 3;
button_height2 = 3;
button_width = 3;
button_width2 = 4;
button_height_full = 23;

clearance = .15;


module button(){
  $fs = .5;
  $fa = .1;

  union(){
    cylinder(button_height, button_width, button_width);

    translate([0,0,button_height])
    {
      cylinder(button_height2, button_width, button_width2);
    }

    cylinder(button_height_full,2,2);
  }
}


translate([38,0,0])
color("red")
  button();

button_socket_width = 1;

module button_socket() {
  $fs = .5;
  $fa = .1;

  hole = button_width + clearance;
  hole_upper = button_width2 + clearance;
  socket_outer = hole + button_socket_width;
  socket_outer_upper = hole_upper + button_socket_width;
  
  difference(){
    cylinder(button_height, socket_outer, socket_outer);
    translate([0,0,-2])
    cylinder(button_height+5, hole, hole);
  }

    translate([0,0,button_height])
    {
      difference(){
        cylinder(button_height2, socket_outer, socket_outer_upper);
        cylinder(button_height2, hole, hole_upper);
      }
    }
  
}

// translate([20,0,0])
// button_socket();

module _button_test() {
  translate([3,-20,0])
  intersection(){
    ws_box();

    translate([5,13,0])
      cube([15,15,8]);
  }
  button();
}

*
_button_test();
*
ws_box();
