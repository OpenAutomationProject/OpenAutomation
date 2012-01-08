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
// 3. Dump all the content in a HTML page:
//    URL parameter "dump" - no value needed
// 4. Remove old content:
//    URL parameter "r":   the timestamp (seconds since 1970) of the oldest log
//                         line to keep
// 5. Get content as JSON:
//    URL parameter "j"

// create database connection
$db = sqlite_open('rsslog.db', 0666, $error);
if (!$db) die ($error);

// create table if it doesn't exists
create( $db );

if( isset($_GET['c']) )
{ 
  // store a new log
  $store = true;
  $log_content = $_GET['c'] ? $_GET['c'] : '<no content>';
  if( mb_detect_encoding($log_content, 'UTF-8', true) != 'UTF-8' )
    $log_content = utf8_encode($log_content);
  $log_tags    = $_GET['t'] ? $_GET['t'] : array();
  
  insert( $db, $log_content, $log_tags );
} else if( isset($_GET['dump']) )
{
  $result = retrieve( $db, $log_filter );
  ?>
<html><head><meta http-equiv="Content-Type" content="text/html;charset=utf-8" /></head><body>
<table border="1">
  <?php
  while( sqlite_has_more($result) )
  {
    $row = sqlite_fetch_array($result, SQLITE_ASSOC ); 
    echo '<tr>';
    echo '<td>' . date( DATE_ATOM, $row['t'] ) . '</td>';
    echo '<td>' . $row['t'] . '</td>';
    echo '<td>' . $row['content'] . '</td>';
    echo "</tr>\n";
  }
  ?>
</table>
</body></html>
  <?php
} else if( isset($_GET['r']) )
{
  $timestamp  = $_GET['r'] ? $_GET['r'] : '';
  delete( $db, $timestamp );
} else if( isset($_GET['j']) )
{
  ?>
{
  "responseData" : {
    "feed" : {
      "feedUrl": "<?php echo 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['SCRIPT_NAME']; ?>",
      "title": "RSS supplied logs",
      "link": "<?php echo 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['SCRIPT_NAME']; ?>",
      "author": "",
      "description": "RSS supplied logs",
      "type": "rss20",
      "entries": [
<?php
  $result = retrieve( $db, $log_filter );
  $first = true;
  while( sqlite_has_more($result) )
  {
    $row = sqlite_fetch_array($result, SQLITE_ASSOC ); 
    if( !$first ) echo ",\n";
    echo '{';
    echo '"title": "' . $row['content'] . '",';
    echo '"content": "' . $row['content'] . '",';
    echo '"publishedDate": "' . date( DATE_ATOM, $row['t'] ) . '"';
    echo '}';
    $first = false;
  }
?>
      ]
    }
  },
  "responseDetails" : null,
  "responseStatus" : 200
}
  <?php
} else {
  // send logs
  $log_filter  = $_GET['f'] ? $_GET['f'] : '';

  // retrieve data
  $result = retrieve( $db, $log_filter );
  echo '<?xml version="1.0"?>';
  ?>
<rss version="2.0">
  <channel>
    <title>RSS supplied logs</title>
    <link><?php echo 'http://' . $_SERVER['HTTP_HOST'] . $_SERVER['SCRIPT_NAME']; ?></link>
    <description>RSS supplied logs</description>
  <?php
  // echo '<description>foo</description>';
  while( sqlite_has_more($result) )
  {
    $row = sqlite_fetch_array($result, SQLITE_ASSOC ); 
    echo '<item>';
    echo '<title>' . $row['content'] . '</title>';
    echo '<description>' . $row['content'] . '</description>';
    echo '<pubDate>' . date( DATE_ATOM, $row['t'] ) . '</pubDate>';
    echo '</item>' . "\n";
  }
  ?>
  </channel>
</rss>
  <?php
}

sqlite_close($db);

///////////////////////////////////////////////////////////////////////////////
// create tables if they don't exist
function create( $db )
{
  $q = "SELECT name FROM sqlite_master WHERE type='table' AND name='Logs';";
  $result = sqlite_query( $db, $q, SQLITE_NUM );
  if (!$result) die("Cannot execute query.");
  if( !sqlite_has_more($result) )
  {
    // no table found - create it
    $q = 'CREATE TABLE Logs(' .
        '  id INTEGER PRIMARY KEY,' . 
        '  content TEXT NOT NULL,' .
        '  t TIMESTAMP' .
        ');';
    $ok = sqlite_exec($db, $q, $error);
    
    if (!$ok)
      die("Cannot execute query. $error");
  }
}

// insert a new log line
function insert( $db, $content, $tags )
{
  // store a new log line
  $q = 'INSERT INTO Logs(content, t) VALUES( ' .
       "  '" . sqlite_escape_string( $content ) . "'," .
//       "  datetime('now','localtime')" .
       "  datetime('now')" .
       ')';
  
  $ok = sqlite_exec($db, $q, $error);
  
  if (!$ok)
    die("Cannot execute query. $error");
}

// return a handle to all the data
function retrieve( $db, $filter )
{
//  $q = "SELECT content, strftime('%s', t, 'localtime') AS t FROM Logs";
  $q = "SELECT content, strftime('%s', t) AS t FROM Logs";
  return sqlite_query( $db, $q, SQLITE_ASSOC );
}

// delete all log lines older than the timestamp
function delete( $db, $timestamp )
{
  //$q = "DELETE from Logs WHERE t < datetime($timestamp, 'unixepoch', 'localtime')";
  $q = "DELETE from Logs WHERE t < datetime($timestamp, 'unixepoch')";
  $ok = sqlite_exec($db, $q, $error);
  
  if (!$ok)
    die("Cannot execute query. $error");
}
?>
