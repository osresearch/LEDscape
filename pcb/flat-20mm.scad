/** \file
 * Brackets for 20mm extrusion.
 *
 * These hold the panels a few mm away from the bracket.
 */

length=26;
width=20;

module bracket()
{
	translate([0,0,4]) cube([28,length,8], center=true);
	translate([12,0,(width+8)/2]) cube([4,length,width+8], center=true);
	translate([-12,0,(width+8)/2]) cube([4,length,width+8], center=true);
}

color([0,1,0]) translate([0,0,width/2+8]) extrusion(100);

render() difference()
{
	bracket();
	translate([-10,10,0]) {
		translate([0,0,-1]) cylinder(r=3.5/2, h=6, $fs=1);
		translate([0,0,4]) cylinder(r=6/2, h=4, $fs=1);
		translate([0,0,8]) cylinder(r=12/2, h=8, $fs=1);
	}
}

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
