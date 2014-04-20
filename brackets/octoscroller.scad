/** \file
 * 3D printed brackets for N sided LED matrix displays.
 *
 * Horizontal or vertical connections are possible.
 * The spacing between the holes differs for each.
 */
sides = 32;

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

// vertical has only 10mm
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

// combo bracket has only 10mm, with 13*2 mm spacing
module vertical_bracket2()
{
	rotate([0,0,-360/sides/2])
	render() difference() {
		translate([-16,0,]) cube([16,8,26+5+5]);
		translate([-10,15,5]) rotate([90,0,0]) cylinder(r=3.5/2,h=20, $fs=1);
		translate([-10,15,26+5]) rotate([90,0,0]) cylinder(r=3.5/2,h=20, $fs=1);
	}

	rotate([0,0,+360/sides/2])
	render() difference() {
		translate([0,0,]) cube([16,8,26+5+5]);
		translate([10,15,5]) rotate([90,0,0]) cylinder(r=3.5/2,h=20, $fs=1);
		translate([10,15,26+5]) rotate([90,0,0]) cylinder(r=3.5/2,h=20, $fs=1);
	}

	translate([-7,3.5,0]) cube([14,5,26+5+5]);
}


module vertical_bracket3()
{
	linear_extrude(height=8) polygon([
		[0,0],
		//[20,cos(360/sides/2)*10],
		[-20,10],
		[20,10],
		//[-cos(360/sides/2)*10,10],
	]);
}


if (0)
{
for (i = [0:7])
{
	for (j = [0:3])
	{
		translate([i*15, j*37,0])
		rotate([0,0,90])
		vertical_bracket();
	}
}
} else {
	vertical_bracket2();
	//vertical_bracket3();
}
