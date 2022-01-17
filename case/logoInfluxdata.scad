// Logo Influx

module
logo_influxdata(k)
{

    difference() // základní šestihran
    {
        translate([ 0, 0, -1 ]) cylinder(h = 2, r = k + 0.2 * k, $fn = 6);
        translate([ 0, 0, -2 ]) cylinder(h = 4, r = k, $fn = 6);
    }

    module obdelnik()
    {
        difference() // základní obdélník
        {
            x = (k * 0.5) / cos(30);

            translate([ 0, x, 0 ]) cube([ k, x + 1, 2 ], center = true);
            translate([ 0, x, 0 ]) cube([ k - 1.5, x - .5, 3 ], center = true);
        }
    }

    obdelnik();

    rotate(120) obdelnik();

    rotate(240) obdelnik();
}

logo_influxdata(15);
