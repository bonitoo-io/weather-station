// case Roman - WIP!

use<logoInfluxdata.scad>; // use logo

use<case.scad>; // use logo
*
box_orig();

// vnitřní rozměry v mm
w = 58.5;  // inner Width
d = 58.5;  // depth
h = 29;    // height

shell_width = 2;

outer_fillet = 2;
inner_fillet = 2;

main_board_width = 43.75;
main_board_depth = 57;
corner_width = 5;
corner_support = 7;

button_height = 3;
button_height2 = 3;
button_width = 3;
button_width2 = 4;
button_height_full = 23;
button_clearance = .15;
button_socket_width = 1;


shell_width = .4*4;
outer_fillet = 2.5;
inner_fillet = 1;

ws_box();

translate([38,0,0])
color("red")
  button();


// Single layer experiment

// shell_width = .5;
// outer_fillet = 1;
// inner_fillet = 4;

// rest_height = .16;
// bottom_support_height = 1;
// bottom_support_Width = .8;
// bottom_support_fillet = 4;

// translate([0,0, - 1.5 + rest_height])
// difference() {
  
//   union() {
//     ws_box();
//     translate([38,0,0])
//     color("red")
//       button();

//     translate([ 0, 0, 1.5 - .1 + rest_height ])
//     difference(){
//       $fn= 50;
//       minkowski() {
//           xy_center_cube([ 
//             w - inner_fillet * 2, 
//             d - inner_fillet * 2, 
//             bottom_support_height - .01
//           ]);
//         cylinder(r = inner_fillet, h = .01);
//       }
//       translate([0,0,-1])
//       minkowski() {
//           xy_center_cube([ 
//             w - bottom_support_fillet * 2 - bottom_support_Width * 2, 
//             d - bottom_support_fillet * 2 - bottom_support_Width * 2, 
//             bottom_support_height + 2
//           ]);
//         cylinder(r = bottom_support_fillet, h = .01);
//       }
//     }
//   }

//   translate([0,0,-1])
//     xy_center_cube([100, 100, 1.5 + 1 - rest_height]);
// }



module xy_center_cube(sizes) {
  translate([ -sizes[0] / 2, -sizes[1] / 2, 0 ]) cube(sizes);
}


module shell() {
  difference() {
    union() {
      difference() {
        $fn = 50;
        
        minkowski() {
          xy_center_cube([ w + shell_width * 2 - outer_fillet * 2, d + shell_width * 2 - outer_fillet * 2, h + 1 - 1 ]);
          cylinder(r = outer_fillet, h = 1);
        }

        minkowski() {
          translate([ 0, 0, 1.5 ])
            xy_center_cube([ w - inner_fillet * 2, d - inner_fillet * 2, h - 1 ]);
          cylinder(r = inner_fillet, h = 1);
        }

        // display window
        translate([ -20.5, 2, -0.7 ]) 
          cube([ 25, 14, 10 ]);

        // usb socket
        translate([ -32, -0.8, 22.3 ]) 
          cube([ 5, 9.5, 3.8 ]);

        // ventilating holes side
        translate([ 28, -25, 5 ]) vent();
        translate([ 28, -25, 8 ]) vent();
        translate([ 28, -25, 11 ]) vent();
        translate([ 28, -15, 5 ]) vent();
        translate([ 28, -15, 8 ]) vent();
        translate([ 28, -15, 11 ]) vent();
        // ventilating holes upper
        translate([ 18, 27, 5 ]) vent();
        translate([ 18, 27, 8 ]) vent();
        translate([ 18, 27, 11 ]) vent();

        textPosY = -14;
        textScale = 0.73;

        mirror([ 1, 0, 0 ])
        {
          translate([ -10, textPosY, -.3 ])
            rotate(180 - 17) logo_influxdata(12);
        }

        translate([ 0, d/2-1, 28 ])
          cube([ 12, shell_width + 2, 3 ]);
        translate([ -6, -d/2-1-shell_width, 28 ]) cube([ 8, shell_width+2, 3 ]);
        translate([ -w / 2-.75, -1, h - 2 ]) cube([ 2, 2, 2 ]);
        translate([ 28, -1, h - 2 ]) cube([ 2, 2, 2 ]);
      }
    }

    // button hole
    translate([ 12, 19, -1 ]) 
      cylinder(r = 4, h = 25, $fn = 20);
  }

  difference(){
    translate([ 12, 19, 0 ]) 
      button_socket();

    translate([0, 10, 0]) {
      cube([7.5,20,20]);
    }
  }
}

module sloupek_deska() {
  cylinder(r = 2.2, h = 24, $fn = 20);
  cylinder(r = 1.1, h = 26.5);
}
module vent() {
  cube([ 7, 7, 1.5 ]);
}

module display_socket() {
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

module main_board_socket() {
  height = 3;

  difference(){
    xy_center_cube([ main_board_width+2, main_board_depth+2, height ]);

    translate([0,0,-.5])
      xy_center_cube([ main_board_width, main_board_depth, height+1 ]);
    
    translate([0,0,-.5])
    {
      xy_center_cube([ main_board_width - corner_width * 2, main_board_depth+5, height+1 ]);
      xy_center_cube([ main_board_width+5, main_board_depth - corner_width * 2, height+1 ]);
    }
  
  }

  base_transform = [main_board_width / 2 + 1, main_board_depth / 2 + 1, 0];

  translate(base_transform)
    main_board_socket_corner();
  
  mirror([1,0,0])
  translate(base_transform)
    main_board_socket_corner();
  
  mirror([0,1,0]) {
    translate(base_transform)
      main_board_socket_corner();
    
    mirror([1,0,0])
    translate(base_transform)
      main_board_socket_corner();
  }
}

module main_board_socket_corner () {
  rotate(a= 180, v= [0,1,0])
  rotate(-90)
  linear_extrude(corner_support, scale = [0,1], slices= corner_support * 3)
    polygon([[0, 0], [corner_width + 1, 0], [0, corner_width + 1]]);
}

module button(){
  $fs = .5;
  $fa = .1;

  union(){
    cylinder(button_height, button_width, button_width);

    translate([ 0, 0, button_height ])
      cylinder(button_height2, button_width, button_width2);

    cylinder(button_height_full,2,2);
  }
}

module button_socket() {
  $fs = .5;
  $fa = .1;

  hole = button_width + button_clearance;
  hole_upper = button_width2 + button_clearance;
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

module ws_box() {
  shell();

  difference(){
    translate([ -7.5, 11, 1 ]) 
      display_socket();

    translate([ 12,19,0 ]) 
      cylinder(20,5,6);
  }

  translate([ 18, -28, 1 ])
    cube([ 3, 18, 10 ]);

  translate([ -w / 2 + main_board_width / 2 + 1, 0, h - 4 ])
    main_board_socket();
}
