CC= gcc
EXENAME= test_1752240
INCLUDE=-I../include
LIBDIR=/usr/lib64/stu/
all:lib test

lib:readconfig.c
	$(CC) -fPIC -shared $^ -o lib_1752240.so
	mkdir -p $(DIRROOT)/usr/lib64/stu
	cp lib_1752240.so $(DIRROOT)/usr/lib64/stu

test: test.c 
	$(CC) $^ -o $@ -L$(LIBDIR) -l_1752240 -Wl,-rpath=$(LIBDIR) 

install	: all
#	1 copy exe file, and rename
	mkdir -p $(DIRROOT)/usr/sbin/stu
	cp test $(DIRROOT)/usr/sbin/stu/$(EXENAME)
#	2 cp lib
	mkdir -p $(DIRROOT)/usr/lib64/stu
	cp lib_1752240.so $(DIRROOT)/usr/lib64/stu
#	3 cp dat
	mkdir -p $(DIRROOT)/home/stu/u1752240
	cp 1752240.dat $(DIRROOT)/home/stu/u1752240
#	4 cp conf
	mkdir -p $(DIRROOT)/etc/stu
	cp stu_1752240.conf $(DIRROOT)/etc/stu
#	5 cp service
	mkdir -p $(DIRROOT)/usr/lib/systemd/system
	cp test_1752240.service $(DIRROOT)/usr/lib/systemd/system/
# ifeq ($(DIRROOT), )
# 	systemctl daemon-reload
# 	systemctl enable test_1752240
# 	systemctl start test_1752240
# endif

rpm:
	mkdir -p ~/rpmbuild/BUILD
	mkdir -p ~/rpmbuild/BUILDROOT
	mkdir -p ~/rpmbuild/RPMS 
	mkdir -p ~/rpmbuild/SRPMS 
	mkdir -p ~/rpmbuild/SOURCES/test_1752240-1.0.0/1.0.0
	mkdir -p ~/rpmbuild/SPECS
	cp -f makefile test.c readconfig.c 1752240.dat stu_1752240.conf test_1752240.service ~/rpmbuild/SOURCES/test_1752240-1.0.0/1.0.0
	cp -f test.spec ~/rpmbuild/SPECS
	tar -cvjf ~/rpmbuild/SOURCES/test.tar.bz2 -C ~/rpmbuild/SOURCES test_1752240-1.0.0
#	mv ~/rpmbuild/test.tar.bz2 ~/rpmbuild/SOURCES
#	run SPEC
	rpmbuild -bb test.spec --nodebuginfo
#	cp to pwd, and rename
	mv ~/rpmbuild/RPMS/x86_64/test_1752240-1.0.0-1.el8.x86_64.rpm ./test_1752240.rpm

uninstall:
#	systemctl -q stop test_1752240 
	rm -f $(DIRROOT)/usr/sbin/stu/$(EXENAME)
	rm -f $(DIRROOT)/usr/lib64/stu/lib_1752240.so 
	rm -f $(DIRROOT)/etc/stu/stu_1752240.conf
	rm -f $(DIRROOT)/home/stu/u1752240/1752240.dat
	rm -df $(DIRROOT)/home/stu/u1752240
	rm -f $(DIRROOT)/usr/lib/systemd/system/test_1752240.service
#	systemctl daemon-reload

clean:
	rm -f *.o test lib_1752240.so
	rm -rf ~/rpmbuild/BUILD 
	rm -rf ~/rpmbuild/BUILDROOT 
	rm -rf ~/rpmbuild/RPMS 
	rm -rf ~/rpmbuild/SRPMS 
	rm -rf ~/rpmbuild/SOURCES 
	rm -rf ~/rpmbuild/SPECS 
	rm -rf ~/rpmbuild/test_1752240-1.0.0 
	rm -f *.rpm