<?php
/*****************************************************************************/
/* rsslog.php - A simple log message reciever and sender via RSS             */
/*                                                                           */
/* (c) 2011 by Christian Mayer                                               */
/* Licenced under the GPLv3                                                  */
/*****************************************************************************/

// There are diffentent modes of operation
// 1. Creating a new log line:
//    URL parameter "c":   the content of the log
//    URL parameter "t[]": a tag for later filtering. Multiple might be given
// 2. Recieve the log as RSS:
//    URL parameter "f":   The (optional) filter, only log lines with a tag
//                         that fit this string are sent
if( isset($_GET['c']) )
{ 
  // store a new log
  $store = true;
  $log_content = $_GET['c'] ? $_GET['c'] : '<no content>';
  $log_tags    = $_GET['t'] ? $_GET['t'] : array();
} else {
  // send logs
  $store = false;
  $log_filter  = $_GET['f'] ? $_GET['f'] : '';
}

// create database connection
$db = sqlite_open('rsslog.db', 0666, $error);
if (!$db) die ($error);

// create table if it doesn't exists
$q = "SELECT name FROM sqlite_master WHERE type='table' AND name='Logs';";
$result = sqlite_query( $db, $q, SQLITE_NUM);
if (!$result) die("Cannot execute query.");
if( !sqlite_has_more($result) )
{
  // no table found - create it
  $q = "CREATE TABLE Logs(" .
       "  id INTEGER PRIMARY KEY," . 
       "  content TEXT NOT NULL," .
       "  t TIMESTAMP" .
       ");";
  $ok = sqlite_exec($db, $q, $error);
  
  if (!$ok)
    die("Cannot execute query. $error");
}

if( $store )
{
  // store a new log line
  $q = "INSERT INTO Logs(content, t) VALUES( " .
       "  '" . sqlite_escape_string( $log_content ) . "'," .
       "  datetime('now','localtime')" .
       ")";
  
  $ok = sqlite_exec($db, $q, $error);
  
  if (!$ok)
    die("Cannot execute query. $error");
} else {
  // retrieve data
  $q = "SELECT * FROM Logs";
  $result = sqlite_query( $db, $q, SQLITE_ASSOC);
// phpinfo(); 
  echo '<?xml version="1.0"?>';
  ?>
<rss version="2.0">
  <channel>
    <title>RSS supplied logs</title>
    <link><?php echo 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['SCRIPT_NAME']; ?></link>
  <?php
  // echo '<description>foo</description>';
  while( sqlite_has_more($result) )
  {
    $row = sqlite_fetch_array($result, SQLITE_ASSOC); 
    echo '<item>';
    echo '<title>' . $row['content'] . '</title>';
    echo '<pubDate>' . $row['t'] . '</pubDate>';
    echo '</item>';
  }
  ?>
  </channel>
</rss>
  <?php
}

sqlite_close($db);
?>