Name:	meguro	
Version: 0.5.5
Release: 1
Summary: A Javascript Map/Reduce Engine
Source: %{name}-%{version}.tar.gz

Group: Development/Tools		
License: MIT-LICENSE	
URL:	http://github.com/jubos/meguro	
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
A Javascript Map/Reduce Engine

%prep
%setup -n %{name}-%{version}

%build
./configure --prefix=$RPM_BUILD_ROOT/usr
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}
make install

%files
%defattr(-,root,root)
%attr(755,root,root) /usr/bin/meguro

%clean
rm -rf $RPM_BUILD_ROOT
