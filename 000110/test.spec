Name:           test_1752240          
Version:        1.0.0             
Release:        1%{?dist}         
Summary:        test_1752240   
License:        GPL                           
Packager:       abel 
Source0:        test.tar.bz2
%description                                 
This is a fucking RPM package. Running a daemon test.

%prep                                         
%setup -q                                               
%build  
make -C 1.0.0

%install
make install DIRROOT=%{buildroot} -C 1.0.0

%pre        
echo "start installing test_1752240"

%post       
echo "installed test_1752240"

%preun      
echo "start uninstalling test_1752240"

%postun     
echo "uninstalled test_1752240"
if [ "$(ls -A /home/stu/u1752240/)" ]; then
    rmdir /home/stu/u1752240/
fi

%files 
%{_sbindir}/stu/test_1752240
%{_libdir}/stu/lib_1752240.so
/home/stu/u1752240/1752240.dat 
%{_sysconfdir}/stu/stu_1752240.conf
/usr/lib/systemd/system/test_1752240.service
%changelog 