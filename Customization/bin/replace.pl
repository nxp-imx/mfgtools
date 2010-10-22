###########################################################################
# replace.pl
###########################################################################

# Make sure arguments are supplied
if ($ARGV[0] eq "") {
    print "Usage: replace.pl <filename>\n";
    die;
}

# Read in the strings
open (STRINGS, "<../strings.txt") or die "Cannot open ../strings.txt";

$var_count=-1;
while ( <STRINGS> ) 
{
	# for each line of strings.txt, make a new var if the line starts with <
    # otherwise, append the line from the file to the end of the last variable
	if ( $_ =~ /^</ )
	{
		++$var_count;
	}
	@vars[$var_count] .= $_;
}


# go through the array of variables, split them at the equal sign
# record the key field in the Name array
# get rid of any trailing white space in the value field
# record the value field in the RepNmae array.
foreach $var (@vars)
{
	@holder = split(/=/,$var);
    
    $Name{$holder[0]} = $holder[0];

    $tmp = $holder[1];
	$tmp =~ s/\s+$//;                # get rid of trailing whitespace
	$RepName{$holder[0]} = $tmp;
    push (@keylist, $holder[0]);
}
close (STRINGS);

# Replace subroutine - replaces to_replace with replace_with
sub replace {
    ($text, $to_replace, $replace_with) = @_;
    $text =~ s/$to_replace/$replace_with/g;
    return $text;
}

$f = "$ARGV[0]";
open (INFILE, $f) or die "Cannot open $f";
	$i = 0;
	until (eof INFILE) {
	    $filearr[$i] = readline INFILE;
        foreach $k (@keylist) {
		    if ($filearr[$i] =~ (m/$Name{$k}/)) {
		        $filearr[$i] = replace ($filearr[$i], $Name{$k}, $RepName{$k});
		    }
		}
	    $i++;
	}
close (INFILE);

$f = "../$ARGV[0]";
open (OUTFILE, "+>$f") or die "Cannot open $f";
    foreach $n(@filearr) {
        print OUTFILE $n;
    }
close (OUTFILE);
