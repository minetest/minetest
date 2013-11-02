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

Then install doT.js and it's dependencies:
    $ cd ~/code
    $ git clone https://github.com/olado/doT.git
    $ cd doT
    $ npm install

And finally compile the template:
    $ cd ~/minetest/util/master
    $ ~/code/doT/bin/dot-packer -s . -d .


