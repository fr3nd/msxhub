#!/usr/bin/env python

"""
Simple webserver to be able to get hub.com from the msx
Run it with:
    python serv.py
"""

import SimpleHTTPServer
import SocketServer
import logging

PORT = 8000

class GetHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_GET(self):
        logging.error(self.headers)
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)


Handler = GetHandler
httpd = SocketServer.TCPServer(("", PORT), Handler)

httpd.serve_forever()
