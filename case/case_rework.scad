// case Roman - WIP!

use<logoInfluxdata.scad>; // use logo

use_cover_text = true;

shell_width = 1.6;

outer_fillet = 2.5;
inner_fillet = 1;

main_board_width = 43.75;
main_board_depth = 57;
corner_width = 5;
corner_support = 7;

button_height = 3;
button_height2 = 3;
button_width = 3;
button_width2 = 4;
button_width3 = 4;
button_height_full = 23;
button_clearance = .15;
button_socket_width = .8;

vent_upper_width = 7;
vent_upper_dist_side = 2;

sensor_wall_width = 1.5;
sensor_wall_hole_width = 3;
sensor_wall_hole_height = 50;

cover_frame_polygon = [[0, 0], [3, 0], [0, 5]];
box_cover_lock = [20, shell_width, .8];


module _() {}
// vnitřní rozměry v mm
w = 58.5;  // inner Width
d = 58.5;  // depth
h = 29;    // height

// *
translate([-(w / 2) - shell_width - 1, 0, 0])
{
  ws_box();

  translate([w / 2 + shell_width + button_width2 + 2, h + button_width2 + 2, 0])
  color("red")
    button();

  translate([w+shell_width+2,0,0])
    cover();
}

// socket test 1
*
translate([0,0,0]) {
  display_socket();

  translate([0,0,-1.8])
  difference() {
    cube_center_xy([32,32,1.8]);
    translate([0,0,-1])
      cube_center_xy([27,27,5]);
  }
}

// socket test 2
*
translate([7, -10, 0]) {
  intersection() {
    ws_box();
    translate([-24,-5,0])
      cube([33,32,8]);
  }
}



// test printer for clearances

*
box_cover_test();



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
//           cube_center_xy([ 
//             w - inner_fillet * 2, 
//             d - inner_fillet * 2, 
//             bottom_support_height - .01
//           ]);
//         cylinder(r = inner_fillet, h = .01);
//       }
//       translate([0,0,-1])
//       minkowski() {
//           cube_center_xy([ 
//             w - bottom_support_fillet * 2 - bottom_support_Width * 2, 
//             d - bottom_support_fillet * 2 - bottom_support_Width * 2, 
//             bottom_support_height + 2
//           ]);
//         cylinder(r = bottom_support_fillet, h = .01);
//       }
//     }
//   }

//   translate([0,0,-1])
//     cube_center_xy([100, 100, 1.5 + 1 - rest_height]);
// }



module cube_center_xy(sizes, fillet, $fa = 1, $fs = .1) {
  if (!fillet) {
    translate([ -sizes[0] / 2, -sizes[1] / 2, 0 ]) cube(sizes);
  } else {
    minkowski() {
      cube_center_xy([ sizes[0] - fillet * 2, sizes[1] - fillet * 2, sizes[2] - .0001]);
      cylinder(r = fillet, h = .0001);
    }
  }
}


module shell() {
  difference() {
    union() {
      difference() {
        cube_center_xy([ w + shell_width * 2, d + shell_width * 2, h + 1 - 2 ], fillet= outer_fillet);

        translate([ 0, 0, 1.5 ])
          cube_center_xy([ w, d, h ], fillet= inner_fillet);

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

        for (side = [-1:2:1]) {
          translate([ side * (w / 2 - vent_upper_width / 2 - vent_upper_dist_side) - vent_upper_width / 2, d / 2 - 1, 5 ])
          for (i = [0:2]) {
            translate([0, 0, 3 * i])
              cube([ vent_upper_width, shell_width + 2, 1.5 ]);
          }
        }

        textPosY = -14;
        textScale = 0.73;

        mirror([ 1, 0, 0 ])
        {
          translate([ -10, textPosY, -.3 ])
            rotate(180 - 17) logo_influxdata(12);
        }
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

  translate([0, 0, h-2.5+1.5]) {
    box_cover_frame();
  }
}

module vent() {
  cube([ 7, 7, 1.5 ]);
}

module display_socket() {
  top_scale = 0.07;
  display = [29, 28, 1.8];
  display_height = display[2];
  display_dist = 2;
  lock_height = 1.5;


  translate([0,0,display_height+display_dist])
  for (i = [0:1])
    mirror([i, 0, 0])
    translate([-display[0]/2, display[1]/2, 0])
    rotate(-90)
    linear_extrude(lock_height)
        polygon([[0, 0], [3, 0], [0, 4.5]]);

  difference()
  {
    linear_extrude(display_height + display_dist + lock_height)
      square([ 29 + 2.5, 28 + 2.5 ], true);

    translate([ 0, 0, display_dist ]) {
      translate([0,-10/2,0])
        cube_center_xy([display[0], display[1]+10, display[2] +10]);
    }

    translate([ 0, 0, -1 ]) {
      cube_center_xy([ 27, 26, 10 ]);
      cube_center_xy([ 20, 40, 15 ]);
      cube_center_xy([ 40, 20, 15 ]);
    }
  }
}

module main_board_socket() {
  height = 3;

  difference(){
    cube_center_xy([ main_board_width + 2, main_board_depth + 2, height ]);

    translate([0,0,-.5])
      cube_center_xy([ main_board_width, main_board_depth, height + 1 ]);
    
    translate([0,0,-.5])
    {
      cube_center_xy([ main_board_width - corner_width * 2, main_board_depth+5, height+1 ]);
      cube_center_xy([ main_board_width+5, main_board_depth - corner_width * 2, height+1 ]);
    }
  }

  for (v = [[0,0], [0,1], [1,0], [1,1]]) {
    mirror([0, v[1], 0])
    mirror([v[0], 0, 0])
    translate([main_board_width / 2 + 1, main_board_depth / 2 + 1, 0])
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

    translate([ 0, 0, button_height_full - 5])
      cylinder(5, 2, button_width3);

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
    translate([ 0, 0, -2 ])
    cylinder(button_height + 5, hole, hole);
  }

    translate([ 0, 0, button_height ])
    {
      difference(){
        cylinder(button_height2, socket_outer, socket_outer_upper);
        cylinder(button_height2, hole, hole_upper);
      }
    }
  
}

module sensor_wall() {
  difference() {
    translate([ 18, -d / 2, 1.5 ])
      cube([ sensor_wall_width, d, h-1.5-1 ]);
    translate([ 18-1, d / 2 - sensor_wall_hole_width, 1 ])
      cube([ sensor_wall_width + 2, sensor_wall_hole_width, sensor_wall_hole_height ]);
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

  sensor_wall();

  translate([ -w / 2 + main_board_width / 2 + 1, 0, h - 4 ])
    main_board_socket();
}


module box_cover_frame(
) {
  difference() {
    cube_center_xy([w + shell_width * 2, d + shell_width * 2, 2], outer_fillet);
    difference() {
      translate([0,0,-1])
      union() {
        cube_center_xy([w, d, 2 + 2], fillet= inner_fillet);
        translate([0, -10, 0])
          cube_center_xy([w, d, 2 + 2], fillet= inner_fillet);
      }

      for (i = [0,1])
        mirror([i,0,0])
        translate([0,0,5])
        translate([w / 2, d / 2 + shell_width, 0])
        rotate(180, [0,1,0])
        rotate(90, [1,0,0])
          linear_extrude(w+shell_width * 2)
            polygon(cover_frame_polygon);
    }
  }

  intersection() {
    union() {
      for (i = [0,1])
        mirror([i,0,0])
        translate([0,0,0])
        translate([w / 2, d / 2 + shell_width, 0])
        rotate(180, [0,1,0])
        rotate(90, [1,0,0])
          linear_extrude(w+shell_width * 2)
            polygon([[0, 0], [1, 0], [0, 2]]);
    }

    translate([0,0,-2])
      cube_center_xy([w + .2, d + .2, 2 + 2], fillet= inner_fillet);
  }

  translate([-box_cover_lock[0] / 2, -d / 2 - shell_width, 0])
    cube(box_cover_lock);
}

module cover() {
  difference() {
    union() {
      cube_center_xy([w, d, 2], fillet= inner_fillet);
      
      translate([0, -shell_width, 0])
        cube_center_xy([w, d, 2]);
    }

    for (i = [0:3])
      translate([0, - w / 2 + 6 + (i * 2), 1])
        cube_center_xy([15, 1, 3]);

    for (i = [0:1])
      mirror([i,0,0])
      translate([0,0,5])
      translate([w / 2, d / 2 + shell_width, 0])
      rotate(180, [0,1,0])
      rotate(90, [1,0,0])
        linear_extrude(w + shell_width * 2 + 2)
          offset(delta= .05)
            polygon(cover_frame_polygon);
    
    // lock
    translate([-box_cover_lock[0]/2 -1, -d/2 - shell_width -.1, -.1])
      cube([box_cover_lock[0]+2,box_cover_lock[1]+.5+.1,box_cover_lock[2]+.1 + .3]);
  
    if (use_cover_text)
    for(i = [["github.com/",0], ["bonitoo-io/",1], ["weather-station",2]])
      translate([-w/2 + 5, d/2-9-i[1]*9, 2-.3])
        linear_extrude(4)
          text(i[0], size= 5, halign ="left");
  }
}

module box_cover_test() {
  difference(){
    cover();
    translate([0, 0, -1]) 
      cube_center_xy([w-10,d-10,20]);
    translate([0, -1, -1]) 
      cube_center_xy([5,d-9,20]);
  }

  translate([w + shell_width * 2 + 2, 0, 0])
  {
    translate([0,0,2])
      box_cover_frame();
      
    difference() {
      cube_center_xy([w + shell_width * 2, d + shell_width * 2, 2], 2);
      difference() {
        translate([0,0,-1])
          cube_center_xy([w, d, 2 + 2], fillet= 2);
      }
    }
  }
}

