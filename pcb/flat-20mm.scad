/** \file
 * Brackets for 20mm extrusion.
 *
 * These hold the panels a few mm away from the bracket in alternating
 * orientation.
 */

width=20;

module baseplate()
{
render() difference()
{
	translate([0,0,3]) cube([32,28,6], center=true);

	translate([0,-10,-1]) cylinder(r=3.75/2, h=6, $fs=1);
	translate([0,-10,2]) cylinder(r=6/2, h=5, $fs=1);

	translate([0,+10,-1]) cylinder(r=3.75/2, h=6, $fs=1);
	translate([0,+10,2]) cylinder(r=6/2, h=5, $fs=1);
}
}

module upright()
{
	rotate([0,-90,0]) render() difference() {
		linear_extrude(height=6)
			polygon([[-1,-14], [0,-14], [20,-5], [20,+5], [0,+14], [-1,+14]]);
		translate([10,0,-1]) cylinder(r=4.5/2, h=8, $fs=1);
	}
}

module bracket()
{
	baseplate();
	translate([20/2+6,0,6]) upright();
	translate([-20/2,0,6]) upright();
}


for (i=[-3:4])
{
	translate([0,i*30,0]) bracket();
}

%color([0,1,0]) translate([0,0,width/2+6]) extrusion(100);

module extrusion(len)
{
	render() difference()
	{
		cube([width,len,width], center=true);
		translate([width/2-5/2+1,0,0]) cube([5,len+2,5], center=true);
		translate([-width/2+5/2-1,0,0]) cube([5,len+2,5], center=true);

		translate([0,0,-width/2+5/2-1]) cube([5,len+2,5], center=true);
		translate([0,0,width/2-5/2+1]) cube([5,len+2,5], center=true);
	}
}
