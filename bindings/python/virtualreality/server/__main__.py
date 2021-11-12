# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) 2020 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>
# Copyright (C) 2020 Josh Miklos <josh.miklos@hobovrlabs.org>

"""
pyvr server.

usage:
  server [--show-messages]

options:
    -h --help               shows this message
    -d --show-messages      show messages

"""
from . import server

from . import __version__

from docopt import docopt

args = docopt(__doc__, version=__version__)

my_server = server.Server()
my_server.debug = args["--show-messages"]

server.run_til_dead(conn_handle=my_server)
