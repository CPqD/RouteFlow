from wsgiref.simple_server import make_server
import rfweb

httpd = make_server("", 8080, rfweb.application)
print "Serving on 0.0.0.0:8080"
httpd.serve_forever()
