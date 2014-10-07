/** \file
 * 3D printed brackets for spiral matrix display
 *
 * Horizontal or vertical connections are possible.
 * The spacing between the holes differs for each.
 */
use <Write.scad>


// vertical has only 10mm
module vertical_bracket(sides, depth)
{
	render() difference()
	{

	union() {
	rotate([0,0,-360/sides/2])
	render() difference() {
		translate([-16,0,]) cube([16,depth,10]);
		translate([-10,1+depth,5]) rotate([90,0,0])
		{
			cylinder(r=3/2+0.4,h=depth+2, $fs=1);
			cylinder(r=6/2+0.4,h=3, $fs=1);
		}
	}

	rotate([0,0,+360/sides/2])
	render() difference() {
		translate([0,0,]) cube([16,depth,10]);
		translate([10,1+depth,5]) rotate([90,0,0])
		{
			cylinder(r=3/2+0.4,h=depth+2, $fs=1);
			cylinder(r=6/2+0.4,h=3, $fs=1);
		}
	}
	}

	translate([0,4.5,10]) rotate([0,0,180]) scale(1.5) write(str(sides), center=true);
	}
}

for (i=[0:23])
{
	translate([(i%6)*35, floor(i/6)*30, 0])
	{
		translate([0,-6,0]) vertical_bracket(i+5, 8);
		translate([0,+6,0]) vertical_bracket(i+5, 8);
	}
}

%translate([0,0,-0.5]) cube([285,153,1]);
