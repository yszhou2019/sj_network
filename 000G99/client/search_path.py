import os

s_path = input('input path:')
for path, dirs, files in os.walk(s_path):
    path_2 = path[len(s_path):] + '/'
    print("path:{0}, dirs:{1}, files:{2}".format(path_2, dirs, files))

