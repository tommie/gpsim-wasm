#!perl

# caps.pl - cheesy ass perl script :) [I'm serious]...
#
# When writing lcd.c, I was faced with the arduous task
# of creating the font for the display. So rather than use
# an error prone method of typing in the bit maps for
# the display, I decided to write this really cheesy
# perl script instead. Peer Ou?????land's LCD web page
# has a .gif file with all of the characters of the LCD
# display. So I downloaded that and converted it to an
# .xpm (using gimp).  The purpose of this script is to
# confer that xpm into a file that can be included by lcd.c
#
# ps. This is my first perl program more than three lines
# long - you'll do yourself injustice trying to learn
# anything from it.


open(LCDXPM, "lcd.xpm") ||
    die("unable to open lcd.xpm\n");
print("Opened lcd.xpm\n");

$first = 0;
$chars = 0;
$line = <LCDXPM>;

while ($first eq 0) {
    chop ($line);
    $_ = $line;
    $dots = tr/././;
    $line = $_;
    if ( $dots > 196) {
	$first = 1;
	$start_col = index($line, "......");
	$end_col = rindex($line, ".");
	print("First, start column $start_col\n"); 
	$line_no = 0;

    }
    $line = <LCDXPM>;

}


for($row = 0; $row<16; $row++) {


    $line = <LCDXPM>;
    for($line_no = 1; $line_no < 8; $line_no++) {
	$line = <LCDXPM>;

	$col = 0;
	for($i = $start_col; $i < $end_col; $i+=15) {
	    $rc = "";
	    for($j=3; $j<13; $j+=2) {
		$rc .= substr($line, $i+$j, 1);
	    }
	    print("$rc");
	    $el[$col*16 + $row][$line_no] = $rc;
	    $col++;
	    
	    if($col eq 1) {$col++;}
	    if($col eq 8) {$col = $col + 2;}
	}
	$line = <LCDXPM>;
	print("\n");
    }

    $line = <LCDXPM>;
    $line = <LCDXPM>;
    $line = <LCDXPM>;
    $line = <LCDXPM>;
    $line = <LCDXPM>;
    $line = <LCDXPM>;
    $line = <LCDXPM>;

}

# at this point, the .xpm file has been parsed and the
# array $el[][] contains the font
#
# As a test, write this string using the font:

$s= "LCD display";

for ($j=1; $j<8; $j++) {

    for ($i=0; $i<length($s); $i++) {
	$c = ord(substr($s,$i,1));
	print("$el[$c][$j]");
    }
    print ("\n");
    
}

# Now create the font file

unless (open (LCDFONTFILE, ">lcdfont.h")) {
    die ("can't open lcdfont.h\n");
}

print LCDFONTFILE ("#define FONT_LEN 256\n") ;


print LCDFONTFILE ("_5X7 test[FONT_LEN] = {\n");
for($i=0; $i<256; $i++) {
    #if( (i>=0x20) && (i<0x80))
      printf LCDFONTFILE (" { /* %x */\n",$i);
    for($j=1; $j<8; $j++) {
	print LCDFONTFILE ("  \"$el[$i][$j]\",\n");
    }
    print LCDFONTFILE (" },\n");
}
print LCDFONTFILE (" };\n");

# Just for kicks, create an include file for 
# a pic:

unless (open (LCDFONTINCFILE, ">lcdfont.inc")) {
    die ("can't open lcdfont.inc\n");
}

print LCDFONTINCFILE ("\n;;\n;; LCD Font file for 7x5 characters\n");
print LCDFONTINCFILE (";; Automatically created from a perl script\n") ;
print LCDFONTINCFILE (";; T. Scott Dattalo http://www.dattalo.com/gnupic/lcd.html\n") ;
print LCDFONTINCFILE (";;\n\n");


for($i=0; $i<256; $i++) {

    print LCDFONTINCFILE ("\n\tdt  ");
    for($k = 0; $k <= 4; $k++) {
	$c = 0;
	for($j=1; $j<8; $j++) {
	    $c = $c * 2;
	    if(substr($el[$i][$j], $k, 1) eq ".") {$c = $c+1;}
	    $t = substr($el[$i][$j], $k, 1);
	    #print "$t $c";
	}
	printf LCDFONTINCFILE ("0x%02x",$c);
	if($k eq 4) {
	    print LCDFONTINCFILE ("  ; $i");
	} else {
	    print LCDFONTINCFILE (", ");
	}
    }
}
print LCDFONTINCDFILE ("\n");

