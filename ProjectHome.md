# Introduction #

MPI ( Mike's Packet Editor ) is a tool for sniffing packets in a given process. It works via an injected payload which communicates with the client which then processes the packets. Currently it is developed on and off whenever I have the spare time. Code criticism is more than welcome. The code for the payload resides here:
http://code.google.com/p/mpi-payload/

For the time being, because of the lack of work that has gone into this project it is more a proof of concept work. Features intended for the future are:
  * Allows users to define custom regex patterns
  * TreeView to display these patterns and their relationship
  * Packet injection
  * Name the project something less lame...
  * Suggestions... ?

# Details #

Current progress..
  * Send is hooked
  * Receive is hooked
  * Communications are working
  * Image lists are implemented
  * Plain tab displays packets in hex
  * Formatted tab displays packets in plaintext

The following screenshots show the tool injected into MSN.

![http://localhostr.com/file/Q2yGjY0/Untitled.png](http://localhostr.com/file/Q2yGjY0/Untitled.png)

![http://localhostr.com/file/JvZzCwA/Untitled2.png](http://localhostr.com/file/JvZzCwA/Untitled2.png)

# To Be Completed #

  * Option to view a packet in detail
  * Use virtual ListView
  * ... ?!