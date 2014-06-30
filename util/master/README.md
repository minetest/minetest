Minetest server list
====================

Setting up the webpage
----------------------

You will have to install node.js, doT.js and their dependencies to compile
the serverlist webpage template.

First install node.js, e.g.:

	# apt-get install nodejs
	# # OR:
	# pacman -S nodejs
	# # OR:
	# emerge nodejs

Then install doT.js and its dependencies:

	$ cd ~
	$ npm install dot commander mkdirp

And finally compile the template:

	$ cd util/master/static
	$ ~/node_modules/dot/bin/dot-packer -s . -d .

You can now serve the webpage by copying the files in static/ to your web root, or by [starting the master server](#setting-up-the-server).


Embedding the server list in a page
-----------------------------------

	<head>
		...
		<script>
			var master = {
				root: 'http://servers.minetest.net/',
				limit: 10,
				clients_min: 1,
				no_flags: 1,
				no_ping: 1,
				no_uptime: 1
			};
		</script>
		...
	</head>
	<body>
		...
		<div id="server_list"></div>
		...
	</body>
	<script src="list.js"></script>


Setting up the server
---------------------

  1. Install Python 3 and pip:

	# pacman -S python python-pip
	# # OR:
	# apt-get install python3 python3-pip

  2. Install Flask, APSchedule, and (if using in production) uwsgi:

	# # You might have to use pip3 if your system defaults to Python 2
	# pip install APSchedule flask uwsgi

  3. Configure the server by changing options in config.py, which is a Flask
	configuration file.

  4. Start the server:

	$ ./server.py
	$ # Or for production:
	$ uwsgi -s /tmp/serverlist.sock -w server:app
	$ # Then configure according to http://flask.pocoo.org/docs/deploying/uwsgi/

