<?php
/*
 * small PHP to post SMS into smstools3/smsd
 * make sure you have an sms-flat if this is open ;)
 * (C) 2012 Michael Markstaller and see below: signed off by me: either freeware or GPL
 * 
 * this script (www-data) needs write-access to /var/spool/sms/outgoing!
 *
 * based on various sources on the net, see:
 * http://smstools3.kekekasvi.com/index.php?p=fileformat
 * http://smstools3.kekekasvi.com/topic.php?id=384
 * http://smstools3.kekekasvi.com/topic.php?id=320
 * http://mteixeira.wordpress.com/2010/03/11/email-to-sms-gateway-in-linux/
 */
$to = "017x"; // default-number if not in post/get
$use_utf = true;

if ($use_utf)
  header('Content-type: text/html; charset=UTF-8');
print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">
<html>
<head>
<title>SMS Test</title>
<!-- <meta http-equiv=\"Content-Type\" content=\"text/html;charset=iso-8859-15\"> -->
</head>
<body>
";
if ($use_utf)
  print "<font size='1'>Raw header used to set charset to UTF-8</font><br><br>\n";
$charset = $_POST['charset'] ? $_POST['charset'] : $_GET['charset'];
$text = $_POST['text'] ? $_POST['text'] : $_GET['text'];
$to = $_POST['to'] ? $_POST['to'] : $_GET['to'];

if ($charset != "" && $text != "")
{
  print "
  charset: $charset<br>
  text: $text<br>
";

  $filename = "/var/spool/sms/outgoing/smstest-" .str_replace(".", "", uniqid(mt_rand(), true));
  if (($handle = fopen($filename .".LOCK", "w")) != false)
  {
    $l = strlen($st = "To: $to\n");
    fwrite($handle, $st);
    if ($charset == "UNICODE")
    {
      $l += strlen($st = "Alphabet: UCS\n");
      fwrite($handle, $st);
      $text = mb_convert_encoding($text, "UCS-2BE", "UTF-8");
    }
    else
    if ($charset == "ISO")
      $text = mb_convert_encoding($text, "ISO-8859-15", "UTF-8");
    if ($_POST['flash'] != "")
    {
      $l += strlen($st = "Flash: yes\n");
      fwrite($handle, $st);
    }
    /* do we need this? questionable.. */
    $l += strlen($st = "Adjustment: +");
    fwrite($handle, $st);
    $pad = 14 - $l % 16 + 16;
    while ($pad-- > 0)
      fwrite($handle, "+");
    fwrite($handle, "\n\n$text");
    fclose($handle);

    if (($handle = fopen($filename .".LOCK", "r")) == false)
      print "Unable to read message file.<br>";
    else
    {
      print "<hr><pre>\n";
      while (!feof($handle))
        echo fgets($handle, 1024);
      print "</pre>\n";
      fclose($handle);
    }

    $tmpfilename = tempnam("/tmp", "smstest-");
    exec("/usr/bin/hexdump -C < $filename.LOCK > $tmpfilename");
    if (($handle = fopen($tmpfilename, "r")) == false)
      print "Unable to create dump.<br>";
    else
    {
      print "<hr><pre>\n";
      while (!feof($handle))
        echo fgets($handle, 1024);
      print "</pre>\n";
      fclose($handle);
      unlink($tmpfilename);
    }

    if ($_POST['showonly'] == "")
    {
      if (rename($filename .".LOCK", $filename) == true)
        print "Message placed to the spooler,<br>filename: $filename<br>\n";
      else
        print "FAILED!<br>\n";
    }
    else
      unlink($filename .".LOCK");
  }
  else
    print "FAILED!<br>\n";

  if (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on')
    $result = 'https://';
  else
    $result = 'http://';
  $result .= $_SERVER['HTTP_HOST'] .$_SERVER['PHP_SELF'];
  print "<hr><a href='$result'>Back</a>\n";
}
else
{
  print "  <form name='test' id='test' method='post'";
  if ($use_utf)
    print " accept-charset='UTF-8'";
  print ">
    Send a message using:<br>
    <input type='radio' name='charset' value='ISO' checked>ISO<br>
    <input type='radio' name='charset' value='UNICODE'>UNICODE<br>
    <input type='radio' name='charset' value='Unmodified'>No charset conversion here<br>
    <br>
    <input type='checkbox' name='flash' value='Y'>Flash<br>
    <input type='checkbox' name='showonly' value='Y' checked>Show only the message file dump<br>
    <br>
    To:<br>
    <input type='text' name='to' maxlength='70' value='$to' size='40'>
    Text:<br>
    <input type='text' name='text' maxlength='150' value='This is a test message.' size='40'>
    <br>
    <input type='submit' value='Submit'>
  </form>
";
}
print "
</body>
</html>
";
?>
