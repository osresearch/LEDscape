/** \file
 * Brackets for 20mm extrusion.
 *
 * These hold the panels a few mm away from the bracket.
 */

length=20;
width=20;

render() difference () {
	translate([0,0,4]) cube([28,length,6], center=true);

	translate([10,5,-1]) cylinder(r=3.5/2, h=6, $fs=1);
	translate([10,5,4]) cylinder(r=6/2, h=4, $fs=1);

	translate([-10,5,-1]) cylinder(r=3.5/2, h=6, $fs=1);
	translate([-10,5,4]) cylinder(r=6/2, h=4, $fs=1);
}

translate([width/2+2,-4,width/2+6]) render() difference() {
	cube([4,length-8,width], center=true);
	translate([-3,0,0]) rotate([0,90,0]) cylinder(r=4.5/2, h=6, $fs=1);
}

translate([-(width/2+2),-4,width/2+6]) render() difference() {
	cube([4,length-8,width], center=true);
	translate([-3,0,0]) rotate([0,90,0]) cylinder(r=4.5/2, h=6, $fs=1);
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
