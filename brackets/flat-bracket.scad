/** \file
 * mounting bracket for 15mm extrusion.
 */


module bracket()
{
	translate([0,0,-3]) linear_extrude(height=3)
		polygon([[0,0], [8,0], [8,3], [3,15], [0,15]]);

	cube([8,3,8]);
	translate([0,0,-10]) cube([3,15,8]);

}
	
/*
translate([15/2,0,3/2]) render() difference() {
	cube([15,10,3], center=true);
	translate([7.5/2,1,-2]) cylinder(r=4/2, h=5, $fs=1);
	translate([-7.5,1,3]) rotate([0,90,0]) cylinder(r=4,h=4);
}

translate([0,0,10/2]) rotate([0,90,0]) render() difference() {
	cube([10,10,3], center=true);
	translate([1,1,-2]) cylinder(r=4/2, h=5, $fs=1);
}
*/


module right()
{
render() difference()
{
	bracket();
	translate([2.5,4,8/2]) rotate([90,0,0]) cylinder(r=3.8/2, h=5, $fs=1);
	translate([-1,7.5,-10+4]) rotate([0,90,0]) cylinder(r=4/2, h=5, $fs=1);
}
}

right();

translate([0,0,20]) scale([1,1,-1]) right();
