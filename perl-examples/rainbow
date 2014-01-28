#!/usr/bin/perl
use warnings;
use strict;
use IO::Socket;
use Time::HiRes 'usleep';

# Make a smooth gradient pattern
my $host = '192.168.7.2:9999';

my $sock = IO::Socket::INET->new(
	PeerAddr => $host,
	Proto => 'udp',
) or die "Socket failed: $!\n";


my $width = 64;
my $height = 210;
my $offset = 0;

while (1)
{
	$offset++;
	my $s = chr(1);
	my $bright = 0x30;

	for(my $y = 0 ; $y < $height ; $y++)
	{
		for(my $x = 0; $x < $width ; $x++)
		{
			my $r = (($x + $offset) % $width) * $bright / $width;
			my $g = 0;
			my $b = (($height - $y - 1 + $offset) % $height) * $bright / $height;

			$s .= chr($r);
			$s .= chr($g);
			$s .= chr($b);
		}
	}

	$sock->send($s);
	usleep(20000);
}
