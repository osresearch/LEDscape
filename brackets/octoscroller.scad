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
module vertical_bracket(depth)
{
	rotate([0,0,-360/sides/2])
	render() difference() {
		translate([-16,0,]) cube([16,depth,10]);
		translate([-10,1+depth,5]) rotate([90,0,0]) cylinder(r=3/2+0.4,h=depth+2, $fs=1);
	}

	rotate([0,0,+360/sides/2])
	render() difference() {
		translate([0,0,]) cube([16,depth,10]);
		translate([10,1+depth,5]) rotate([90,0,0]) cylinder(r=3/2+0.4,h=depth+2, $fs=1);
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


// bracket 3 has the normal vertical bracket, with an additional
// bit to secure the 15mm extrusion.
module vertical_bracket3()
{
	vertical_bracket(12);

	render() difference() {
		union() {
			rotate([0,0,+360/sides/2]) translate([0,0,9.9]) cube([16,12,13+5+15-10]);
			rotate([0,0,-360/sides/2]) translate([-16,0,10]) cube([16,12,13+5+15-10]);
		}

		// subtract the extrusion
		translate([-15/2,-10,5+13]) cube([15,50,25]);

		// m3 screw holes to secure the extrusion to the bracket
		translate([+18,7,13+15/2+5]) rotate([0,90,180]) union() {
			cylinder(r=3/2+0.4, h=12, $fs=1);
			cylinder(r=6/2+0.4, h=4, $fs=1);
		}
		translate([-18,7,13+15/2+5]) rotate([0,90,0]) union() {
			cylinder(r=3/2+0.4, h=12, $fs=1);
			cylinder(r=6/2+0.4, h=4, $fs=1);
		}
	}
}


if (0)
{
for (i = [0:3])
{
	for (j = [0:3])
	{
		translate([i*15, j*37,0])
		rotate([0,0,90])
		vertical_bracket2();
	}
}
} else {
	//vertical_bracket2();
	vertical_bracket3();
}
