# Rothaga
Encrypted Chat and File Transfer

<06/22/2016>  New features in 0.9 Alpha!

		* MAC Address checking			Go ahead, try to make a clone!

		* /kg <name> <amount>			Karma gifting, give a friend some Karma.

		* /pm <name> <message>			Send private messages. 

<05/19/2016>  We are making progress daily on both the client and server code!
	      
		* Name lengths are now restricted to 16 characters by default.

		* Karma has been introduced as part of our initiative to create
			a self-moderating chat environment.

		* Commands that currently work:

			/sn <name>	Sets your current name on the network.
			/ping		Shows you your ping in Milliseconds with 3 digits of ns precision.
			/sm <message>	Sends a chat message to all connected clients (This is the default action if you don't use a "/" command).
			/quit		Exit Rothaga.

		* Commands in development:

			/rp <name>	Allows you to report someone to the server.  This initiates a server wide vote on that person.
					We want to be fair about the usage of this very powerful command.  People who use it correctly
					will gain a small Karma bonus; people who abuse it will lose massive Karma.

		* Future plans:

			1.) server -> server communication, to grow the network.  We always want Rothaga to be a single meshed network.
			2.) More fun with Karma.  We will make as many fun ways to gain (or lose) Karma as we can think of.
			3.) Portals, Sections, Chambers, Channels, Groups, something...  we need a name for topics of discussion within Rothaga.
			4.) The PKI Encryption scheme from the submission we made to the IANA.  We need to link in libgpg and start working with it.
			5.) File transfer.  To keep our HIPA compliance goal, files must be encrypted both at rest, and in transfer.
