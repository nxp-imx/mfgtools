


open (INFILE,"<version.h") || die "Cannot open version.h!";

$version = "";
until (eof INFILE) {
	chomp($tmp = readline INFILE);
	if ($tmp =~ m/\STMP_BUILD_VERSION\b/) {
		@holder = split(/=/,$tmp);
		$version = $holder[1];
		print "from version.h\n";
		print "current version: $version\n";
	}
}
close (INFILE);

# make a bat file to set the STMP_BUILD_VERSION environment variable
open (OUTFILE,">ver.bat") || die "Cannot open ver.bat!";
print "opened ver.bat\n";
print OUTFILE "set STMP_BUILD_VERSION=$version\n";
close (OUTFILE);

