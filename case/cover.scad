// Krabicka Ivan víčko

// rozměry v mm
s = 56;  // sirka
d = 56;  // delka
v = 1.9; // vyska

module
vicko()

    difference()
{
    $fn = 25;
    minkowski()
    {
        translate([ -d / 2, -s / 2, 0 ]) cube([ d, s, v ]);
        cylinder(r = 0.8, h = 0.1); // zaoblení rohů
    }
    translate([ 21, -1, -0.7 ]) // díra
        cube([ 4, 2, 3 ]);
}
translate([ -5.8, -31, 0 ]) // zámky
    cube([ 7.6, 3, 2 ]);
translate([ -0, 28, 0 ]) cube([ 11.6, 3, 2 ]);

translate([ -29.5, -0.8, 0 ]) cube([ 2, 1.5, 1.5 ]);
translate([ 27.5, -0.8, 0 ]) cube([ 2, 1.5, 1.5 ]);
vicko();