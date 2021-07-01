# import fallocate
import os

f = open('test.txt', 'wb+')
os.posix_fallocate(f, 0, 512)