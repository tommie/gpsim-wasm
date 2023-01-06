<?php

function chapter($text, $name)
{
  echo "<h2>".$text."</h2>\n";
}


function display_files($dir, $subdir)
{
  $numfiles = 0;
  $thissubdir = @opendir("$dir/$subdir");
  while (false!=($file = @readdir($thissubdir))) {
    if ($file != "." && $file != ".." && $file != "HEADER.html" && $file != ".htaccess") {
      $file_name[$numfiles] = $file;
      $file_size[$numfiles] = filesize("$dir/$subdir/$file");
      $file_date[$numfiles] = filemtime("$dir/$subdir/$file");
      $numfiles++;
    }
  }
  if ($numfiles > 0) {
    array_multisort($file_date, SORT_DESC, $file_name, $file_size);
  }

  echo "<h3><img src=\"./images/dir.gif\" width=\"20\" height=\"22\" alt=\"Directory\" />";
  if (is_file("$dir/../snapshots.desc/$subdir.desc")) {
    include "$dir/../snapshots.desc/$subdir.desc";
  }
  echo "($subdir)</h3>\n";
  echo "<blockquote>\n";
  echo "<table class=\"dir_list\">\n";
  echo "<tr>\n";
  echo "  <th class=\"dir_list\">Filename</th>\n";
  echo "  <th class=\"dir_list\">Size</th>\n";
  echo "  <th class=\"dir_list\">Snapshot Date</th>\n";
  echo "</tr>\n";
  for ($i=0; $i<$numfiles; $i++) {
    $fs = round($file_size[$i] / 1024,0);
    $subdirpp = preg_replace("/\s/","%20","$dir/$subdir");
    $filep = preg_replace("/\s/","%20",$file_name[$i]);
    $modDate = date("F j, Y", $file_date[$i]);
    echo "<tr>\n";
    echo "  <td class=\"dir_list\"><a href=\"$subdirpp/$filep\">$file_name[$i]</a></td>\n";
    echo "  <td class=\"dir_list\">$fs K</td>\n";
    echo "  <td class=\"dir_list\">$modDate</td>\n";
    echo "</tr>\n";
  }
  echo "</table>\n";
  echo "</blockquote>\n";
}


function parse_dir($scanthis)
{
  $linux_num = $windows_num = $docs_num = $source_num = $other_num = 0;

  $dir = @opendir($scanthis);
  while (false!=($file = @readdir($dir))) {
    if (is_dir($scanthis."/".$file) && $file != "." && $file != "..") {
      if (preg_match('/linux/', $file)) {
        $linux_dir[$linux_num++] = $file;
      } elseif (preg_match('/win32/', $file)) {
        $windows_dir[$windows_num++] = $file;
      } elseif (preg_match('/docs/', $file)) {
        $docs_dir[$docs_num++] = $file;
      } elseif (preg_match('/-src/', $file)) {
        $source_dir[$source_num++] = $file;
      } else {
        $other_dir[$other_num++] = $file;
      }
    }//end-if
  }//end-while

  //  chapter("Linux Binaries", "Linux");
  //  for ($i=0; $i<$linux_num; $i++) {
  //    display_files($scanthis, $linux_dir[$i]);
  //  }
  chapter("Windows Binaries", "Windows");
  for ($i=0; $i<$windows_num; $i++) {
    display_files($scanthis, $windows_dir[$i]);
  }
  //chapter("Documentation", "Docs");
  //for ($i=0; $i<$docs_num; $i++) {
  //  display_files($scanthis, $docs_dir[$i]);
  //}
  //chapter("Source Code", "Source");
  //for ($i=0; $i<$source_num; $i++) {
  //  display_files($scanthis, $source_dir[$i]);
  //}
  //if ($other_num > 0) chapter("Other Files", "Other");
  //for ($i=0; $i<$other_num; $i++) {
  //  display_files($scanthis, $other_dir[$i]);
  //}

}//end-function declaration

require 'snap_header.html';

parse_dir("snapshots");

require 'snap_footer.html';

?>
