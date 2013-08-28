Name: vms-empire
Version: 1.2
Release: 1
Source: locke.ccil.org:/pub/esr/vms-empire-1.2.tar.gz
Copyright: BSD-like
Group: Games
Summary: Solitaire Empire (sometimes called `VMS Empire')

%description
Solitaire Empire (sometimes called `VMS Empire')

%prep
%setup

%build
make

%install
cp vms-empire /usr/bin
cp vms-empire.6 /usr/man/man6/vms-empire.6

%files
/usr/man/man6/vms-empire.6
/usr/bin/vms-empire
