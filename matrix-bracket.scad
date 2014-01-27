/** \file
 * Bracket to hold a 16x2 LED panel in an Octoscroller configuration.
 */

module bracket_half()
{
	render() difference()
	{
		cube([18,7,10]);

		translate([13,10,5])
		rotate([90,0,0])
		cylinder(r=2, h=20, $fs=1);
	}
}

module bracket()
{
	rotate([0,0,+45/2]) bracket_half();
	rotate([0,0,-45/2-180]) translate([0,-7,0]) bracket_half();
}

for (i = [0:1])
{
	translate([i*35,0,0]) {
		for (j = [0:7]) {
			translate([0,10*j,0]) bracket();
		}
	}
}

%translate([0,0,-1]) cube([200,200,2], center=true);
