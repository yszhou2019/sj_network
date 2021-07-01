# import fallocate
import os

f = open('test.txt', 'rb+')
f.seek(0, 0)
f.write(b'www')
f.close()


