Minetest server list
====================

Setting up the webpage
----------------------
You will have to install node.js, doT.js and their dependencies to compile
the serverlist webpage template.

First install node.js, eg:

    # apt-get install nodejs
    # pacman -S nodejs
    # emerge nodejs

Then install doT.js and its dependencies:

    $ cd ~/code
    $ git clone https://github.com/olado/doT.git
    $ cd doT
    $ npm install

Or with npm:

    $ npm install dot commander mkdirp

And finally compile the template:

    $ cd ~/minetest/util/master
    $ ~/code/doT/bin/dot-packer -s . -d .

or

    $ ~/node_modules/dot/bin/dot-packer -s . -d .


Embedding to any page
----------------------

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
    <script src="http://servers.minetest.net/list.js"></script>
