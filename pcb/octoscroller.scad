/** \file
 * 3D printed brackets for N sided LED matrix displays.
 *
 * Horizontal or vertical connections are possible.
 * The spacing between the holes differs for each.
 */
sides = 16;
orientation = 1;


// horizontal has 13 mm from edge to center of hole.
module horizontal_bracket()
{
	rotate([0,0,-360/sides/2])
	render() difference() {
		translate([-20,0,]) cube([20,6,10]);
		translate([-13,15,5]) rotate([90,0,0]) cylinder(r=3,h=20);
	}

	rotate([0,0,+360/sides/2])
	render() difference() {
		translate([0,0,]) cube([20,6,10]);
		translate([13,15,5]) rotate([90,0,0]) cylinder(r=3,h=20);
	}
}

// vetical has only 10mm
module vertical_bracket()
{
	rotate([0,0,-360/sides/2])
	render() difference() {
		translate([-16,0,]) cube([16,8,10]);
		translate([-10,15,5]) rotate([90,0,0]) cylinder(r=2,h=20, $fs=1);
	}

	rotate([0,0,+360/sides/2])
	render() difference() {
		translate([0,0,]) cube([16,8,10]);
		translate([10,15,5]) rotate([90,0,0]) cylinder(r=2,h=20, $fs=1);
	}
}


for (i = [0:3])
{
	for (j = [0:3])
	{
		translate([i*21, j*37,0])
		rotate([0,0,90])
		vertical_bracket();
	}
}
