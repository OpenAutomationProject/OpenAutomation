# GoogleCalendar Helper for CometVisu

This is a simple helper for displaying Google Calendar events in CometVisu.
It's composed of two parts:
- an iCal reader php class to parse an ics file
- a php script (that also holds the private calendar URL) fetching iCal calendar from Google, calling the parser class and nicely format the content CometVisu-friendly

## Requirements

The assumption is that you already have a working CometVisu, served by a web server that is also php-enabled (i.e. Apache, nginx or other).

## Installation
Copy the php files into a web-browsable and php-executable directory on this web server. If you use any subdirectories you will have to specify it in the URL later.

## Configuration
1. Fetch your private Google Calendar URL: ![alt text](https://raw.githubusercontent.com/OpenAutomationProject/OpenAutomation/master/GoogleCalendarHelper/image_46687.jpg "copy link")
2. Adjust configuration in googlecalendar_ical_parser.php header (use link(s) fetched in step 1)

## Usage

Use the following syntax in your CometVisu configuration xml:
```xml
<group name="Kalender">
  <layout colspan="8" rowspan="3"/>
  <web src="http://<URL>/googlecalendar_ical_parser.php" height="250px" frameborder="false" refresh="10800" scrolling="no">
    <layout rowspan="3" colspan="8"/>
  </web>
</group>
```

Where `<URL>` is your web server (i.e. CometVisu on Wiregate) specified with IP (192.168.0.xxx) or name (wiregate123.local) plus any subdirectories if you created any to the files copied above.
Do not use 127.0.0.1 / localhost as the client accessing the webpage will fetch the content, not the server itself.

## Further information

[KNXUF Thread](https://knx-user-forum.de/forum/supportforen/cometvisu/33952-google-kalender-in-der-cometvisu/)

## License
tbd
based on:
- ics-parser, author Martin Thoma <info@martin-thoma.de>, licensed under MIT license http://www.opensource.org/licenses/mit-license.php
