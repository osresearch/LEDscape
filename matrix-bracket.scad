/** \file
 * Bracket to hold a 16x2 LED panel in an Octoscroller configuration.
 */

module bracket()
{
	render() difference()
	{
		cube([16,5,10]);

		translate([13,10,5])
		rotate([90,0,0])
		cylinder(r=2, h=20, $fs=1);
	}
}

rotate([0,0,+45/2]) bracket();
rotate([0,0,-45/2-180]) translate([0,-5,0]) bracket();

%translate([0,0,-1]) cube([200,200,2], center=true);
