<?php
 $feedback = $_GET["feedback"];
 if (strlen($feedback) > 3 && strlen($feedback) < 300) {
   $fd = fopen("/tmp/feedbackfile.txt","a");
   fwrite($fd, $feedback . "\n");
   fclose($fd);
  }
?>
